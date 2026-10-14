#include "ERMovement.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

namespace
{
class FERSavedMove : public FSavedMove_Character
{
public:
    using Super = FSavedMove_Character;
    bool Walk = false, Climb = false, High = false;
    FVector Start, End;
    float Elapsed = 0, Duration = 0;
    double Retry = 0;
    virtual void Clear() override { Super::Clear(); Walk = Climb = High = false; Elapsed = Duration = 0; Retry = 0; Start = End = FVector::ZeroVector; }
    virtual uint8 GetCompressedFlags() const override
    {
        return Super::GetCompressedFlags() | (Walk ? FLAG_Custom_0 : 0) | (Climb ? FLAG_Custom_1 : 0);
    }
    virtual bool CanCombineWith(const FSavedMovePtr& Other, ACharacter* Character, float MaxDelta) const override
    {
        const auto* Move = static_cast<const FERSavedMove*>(Other.Get());
        return Duration == 0 && Move->Duration == 0 && Walk == Move->Walk && Climb == Move->Climb && Super::CanCombineWith(Other, Character, MaxDelta);
    }
    virtual void SetMoveFor(ACharacter* Character, float Delta, FVector const& Accel, FNetworkPredictionData_Client_Character& Data) override
    {
        Super::SetMoveFor(Character, Delta, Accel, Data);
        const auto* Move = Cast<UERMovement>(Character->GetCharacterMovement());
        Walk = Move->bWalkHeld; Climb = Move->bClimbHeld;
        Start=Move->TraversalStart; End=Move->TraversalEnd; Elapsed=Move->TraversalElapsed; Duration=Move->TraversalDuration; Retry=Move->RetryAfter; High=Move->bHighClimb;
    }
    virtual void PrepMoveFor(ACharacter* Character) override
    {
        Super::PrepMoveFor(Character);
        auto* Move = Cast<UERMovement>(Character->GetCharacterMovement());
        Move->bWalkHeld = Walk; Move->bClimbHeld = Climb;
        Move->TraversalStart=Start; Move->TraversalEnd=End; Move->TraversalElapsed=Elapsed; Move->TraversalDuration=Duration; Move->RetryAfter=Retry; Move->bHighClimb=High;
    }
};
class FERPredictionData : public FNetworkPredictionData_Client_Character
{
public:
    using Super = FNetworkPredictionData_Client_Character;
    FERPredictionData(const UCharacterMovementComponent& Movement) : Super(Movement) {}
    virtual FSavedMovePtr AllocateNewMove() override { return FSavedMovePtr(new FERSavedMove()); }
};
}
UERMovement::UERMovement()
{
    SetCrouchedHalfHeight(62.5f);
    MaxWalkSpeed = 1000.f / 3.6f; MaxWalkSpeedCrouched = 100.f;
    MaxAcceleration = 1000; GroundFriction = 3;
    bUseSeparateBrakingFriction = true; BrakingFriction = 0;
    BrakingDecelerationWalking = 850;
    MaxStepHeight = 30; JumpZVelocity = 460; GravityScale = 1.8f; AirControl = .2f;
}
float UERMovement::GetMaxSpeed() const
{
    if (IsTraversing()) return 0;
    if (IsCrouching()) return MaxWalkSpeedCrouched;
    return bWalkHeld ? 500.f / 3.6f : Super::GetMaxSpeed();
}
void UERMovement::UpdateFromCompressedFlags(uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);
    bWalkHeld = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
    bClimbHeld = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}
FNetworkPredictionData_Client* UERMovement::GetPredictionData_Client() const
{
    if (!ClientPredictionData) const_cast<UERMovement*>(this)->ClientPredictionData = new FERPredictionData(*this);
    return ClientPredictionData;
}
bool UERMovement::FindLedge(FVector& Destination, float& Height) const
{
    if (!CharacterOwner || !UpdatedComponent || IsCrouching()) return false;
    const auto* Capsule = CharacterOwner->GetCapsuleComponent();
    const float Half = Capsule->GetScaledCapsuleHalfHeight(), Radius = Capsule->GetScaledCapsuleRadius();
    const FVector Position = UpdatedComponent->GetComponentLocation();
    const FVector Feet = Position - FVector(0,0,Half);
    const FVector Forward = CharacterOwner->GetActorForwardVector().GetSafeNormal2D();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ERLedge), false, CharacterOwner);
    FHitResult Wall, Top;
    // A short wall feeler prevents mounting distant blocks or ordinary stairs.
    if (!GetWorld()->LineTraceSingleByChannel(Wall, Feet + FVector(0,0,35), Feet + FVector(0,0,35) + Forward*(Radius+25), ECC_Visibility, Params)) return false;
    if (FMath::Abs(Wall.ImpactNormal.Z) > .2f) return false;
    const FVector Ahead = Feet + Forward*(Radius+55);
    if (!GetWorld()->LineTraceSingleByChannel(Top, Ahead + FVector(0,0,150), Ahead + FVector(0,0,30.1f), ECC_Visibility, Params) || !IsWalkable(Top)) return false;
    Height = Top.ImpactPoint.Z - Feet.Z;
    if (Height <= 30 || Height > 150 || (Height > 80 && !bClimbHeld) || (IsFalling() && !bClimbHeld)) return false;
    Destination = FVector(Ahead.X, Ahead.Y, Top.ImpactPoint.Z + Half + 2.4f);
    // Test the full standing body, including ceiling clearance, before moving.
    return !GetWorld()->OverlapBlockingTestByChannel(Destination, FQuat::Identity, UpdatedComponent->GetCollisionObjectType(), FCollisionShape::MakeCapsule(Radius,Half), Params);
}
void UERMovement::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
    Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
    if (IsTraversing() || (!IsMovingOnGround() && !IsFalling()) || Acceleration.IsNearlyZero() || GetWorld()->GetTimeSeconds() < RetryAfter) return;
    if (FVector::DotProduct(Acceleration.GetSafeNormal2D(), CharacterOwner->GetActorForwardVector()) < .5f) return;
    FVector Destination; float Height;
    if (!FindLedge(Destination, Height)) return;
    TraversalStart = UpdatedComponent->GetComponentLocation(); TraversalEnd = Destination;
    TraversalElapsed = 0; bHighClimb = Height > 80;
    TraversalDuration = bHighClimb ? .75f + Height*.002f : .30f + Height*.003f;
    Velocity = FVector::ZeroVector;
    SetMovementMode(MOVE_Custom, 1);
}
void UERMovement::PhysCustom(float DeltaTime, int32 Iterations)
{
    if (!IsTraversing()) { Super::PhysCustom(DeltaTime, Iterations); return; }
    // Simulated proxies use the server's replicated position and smoothing.
    if (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy) return;
    if (TraversalDuration <= SMALL_NUMBER)
    {
        // A correction can deliver a custom mode before a local trajectory exists.
        // Never sweep to an uninitialized target; let the server correct position.
        bHighClimb = false; SetMovementMode(MOVE_Falling); return;
    }
    TraversalElapsed += DeltaTime;
    const float Alpha = FMath::Clamp(TraversalElapsed / TraversalDuration, 0.f, 1.f);
    FVector Next = TraversalStart;
    Next.Z = FMath::Lerp(TraversalStart.Z, TraversalEnd.Z, FMath::SmoothStep(0.f,.65f,Alpha));
    const float Across = FMath::SmoothStep(.65f,1.f,Alpha);
    Next.X = FMath::Lerp(TraversalStart.X, TraversalEnd.X, Across);
    Next.Y = FMath::Lerp(TraversalStart.Y, TraversalEnd.Y, Across);
    FHitResult Hit;
    SafeMoveUpdatedComponent(Next - UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentQuat(), true, Hit);
    Velocity = FVector::ZeroVector;
    if (Hit.IsValidBlockingHit() || Alpha >= 1)
    {
        RetryAfter = GetWorld()->GetTimeSeconds() + .25;
        bHighClimb = false; TraversalDuration = 0;
        SetMovementMode(MOVE_Falling);
    }
}
