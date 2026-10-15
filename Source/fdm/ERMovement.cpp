#include "ERMovement.h"
#include "ERCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

namespace
{
class FERSavedMove : public FSavedMove_Character
{
public:
    using Super = FSavedMove_Character;
    bool Walk = false, Climb = false, High = false, Sprint = false, Prone = false;
    FVector Start, End;
    float Elapsed = 0, Duration = 0;
    double Retry = 0;
    FERLocomotionState Posture;
    bool PreviousSprint = false;
    double SprintRequest = 0;
    virtual void Clear() override { Super::Clear(); Walk = Climb = High = Sprint = Prone = false; Elapsed = Duration = 0; Retry = 0; Start = End = FVector::ZeroVector; Posture = FERLocomotionState(); PreviousSprint = false; SprintRequest = 0; }
    virtual uint8 GetCompressedFlags() const override
    {
        return Super::GetCompressedFlags() | (Walk ? FLAG_Custom_0 : 0) | (Climb ? FLAG_Custom_1 : 0) | (Sprint ? FLAG_Custom_2 : 0) | (Prone ? FLAG_Custom_3 : 0);
    }
    virtual bool CanCombineWith(const FSavedMovePtr& Other, ACharacter* Character, float MaxDelta) const override
    {
        const auto* Move = static_cast<const FERSavedMove*>(Other.Get());
        return Duration == 0 && Move->Duration == 0 && Walk == Move->Walk && Climb == Move->Climb && Sprint == Move->Sprint && Prone == Move->Prone && Super::CanCombineWith(Other, Character, MaxDelta);
    }
    virtual void SetMoveFor(ACharacter* Character, float Delta, FVector const& Accel, FNetworkPredictionData_Client_Character& Data) override
    {
        Super::SetMoveFor(Character, Delta, Accel, Data);
        const auto* Move = Cast<UERMovement>(Character->GetCharacterMovement());
        Posture = Move->Locomotion; PreviousSprint = Move->bPreviousSprintHeld; SprintRequest = Move->SprintRequestUntil;
        Walk = Move->bWalkHeld; Climb = Move->bClimbHeld; Sprint = Move->bSprintHeld; Prone = Move->bProneWanted;
        bForceNoCombine |= Move->IsChangingPosture() || Sprint || Move->GetRecoveryRemaining() > 0;
        Start=Move->TraversalStart; End=Move->TraversalEnd; Elapsed=Move->TraversalElapsed; Duration=Move->TraversalDuration; Retry=Move->RetryAfter; High=Move->bHighClimb;
    }
    virtual void PrepMoveFor(ACharacter* Character) override
    {
        Super::PrepMoveFor(Character);
        auto* Move = Cast<UERMovement>(Character->GetCharacterMovement());
        Move->Locomotion = Posture; Move->bPreviousSprintHeld = PreviousSprint; Move->SprintRequestUntil = SprintRequest;
        Character->GetCapsuleComponent()->SetCapsuleHalfHeight(Posture.HalfHeight, true);
        Move->ApplyPosturePresentation();
        Move->bWalkHeld = Walk; Move->bClimbHeld = Climb; Move->bSprintHeld = Sprint; Move->bProneWanted = Prone;
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
    SetIsReplicatedByDefault(true);
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
    if (IsChangingPosture()) return 45.f;
    if (IsProne()) return 55.f;
    if (Locomotion.HalfHeight < 92.f) return MaxWalkSpeedCrouched;
    if (IsSprinting()) return 1800.f / 3.6f;
    return bWalkHeld ? 500.f / 3.6f : Super::GetMaxSpeed();
}
void UERMovement::UpdateFromCompressedFlags(uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);
    bWalkHeld = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
    bClimbHeld = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
    bSprintHeld = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
    bProneWanted = (Flags & FSavedMove_Character::FLAG_Custom_3) != 0;
}
FNetworkPredictionData_Client* UERMovement::GetPredictionData_Client() const
{
    if (!ClientPredictionData) const_cast<UERMovement*>(this)->ClientPredictionData = new FERPredictionData(*this);
    return ClientPredictionData;
}
bool UERMovement::FindLedge(FVector& Destination, float& Height) const
{
    if (!CharacterOwner || !UpdatedComponent || Locomotion.HalfHeight < 92.f || bProneWanted || IsChangingPosture()) return false;
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
    auto SampleTop = [&](float Along,FHitResult& Hit)
    {
        const FVector P=Feet+Forward*Along;
        return GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,150),P+FVector(0,0,30.1f),ECC_Visibility,Params) && IsWalkable(Hit);
    };
    // Start just inside the face. A fixed probe55cm beyond it skips thin
    // parapets completely, even when they have a valid walkable top.
    const float Near=FVector::DotProduct(Wall.ImpactPoint-Feet,Forward)+2,Preferred=Radius+55;
    if(!SampleTop(Near,Top))return false;
    Height = Top.ImpactPoint.Z - Feet.Z;
    if (Height <= 30 || Height > 150 || (Height > 80 && !bClimbHeld) || (IsFalling() && !bClimbHeld)) return false;
    // Keep the old landing distance on broad obstacles. On a narrow top,
    // stop at its first edge and land inside the supported strip. Sampling
    // the interval also prevents choosing a different platform across a gap.
    float Landing=Near,Last=Near;
    const int32 Samples=FMath::Clamp(FMath::CeilToInt((Preferred-Near)/8),1,16);
    for(int32 I=1;I<=Samples;++I)
    {
        const float Along=FMath::Lerp(Near,Preferred,float(I)/Samples);FHitResult Candidate;
        if(SampleTop(Along,Candidate) && FMath::Abs(Candidate.ImpactPoint.Z-Top.ImpactPoint.Z)<2.5f){Last=Landing=Along;continue;}
        float Edge=Along;
        for(int32 Refine=0;Refine<3;++Refine)
        {
            const float Mid=(Last+Edge)*.5f;
            if(SampleTop(Mid,Candidate) && FMath::Abs(Candidate.ImpactPoint.Z-Top.ImpactPoint.Z)<2.5f)Last=Mid;else Edge=Mid;
        }
        Landing=(Near+Last)*.5f;break;
    }
    const FVector Ahead=Feet+Forward*Landing;
    Destination = FVector(Ahead.X, Ahead.Y, Top.ImpactPoint.Z + Half + 2.4f);
    // Test the full standing body, including ceiling clearance, before moving.
    return !GetWorld()->OverlapBlockingTestByChannel(Destination, FQuat::Identity, UpdatedComponent->GetCollisionObjectType(), FCollisionShape::MakeCapsule(Radius,Half), Params);
}
void UERMovement::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
    UpdatePosture(DeltaSeconds);
    UpdateSprint();
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

double UERMovement::ServerTime() const
{
    const auto* State = GetWorld()->GetGameState();
    return State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}
bool UERMovement::IsSprinting() const { return Locomotion.SprintUntil > ServerTime(); }
float UERMovement::GetSprintRemaining() const { return FMath::Max(0.f, float(Locomotion.SprintUntil - ServerTime())); }
float UERMovement::GetRecoveryRemaining() const { return FMath::Max(0.f, float(Locomotion.RecoverUntil - ServerTime())); }
bool UERMovement::IsChangingPosture() const
{
    const float Target = bProneWanted ? 34.f : (bWantsToCrouch ? 62.5f : 92.5f);
    return !FMath::IsNearlyEqual(Locomotion.HalfHeight, Target, .1f) ||
        !FMath::IsNearlyEqual(Locomotion.ProneBlend, bProneWanted ? 1.f : 0.f, .01f);
}
void UERMovement::UpdateSprint()
{
    const double Now = ServerTime();
    const auto* C = Cast<AERCharacter>(CharacterOwner);
    const bool Allowed = C && !C->Health->bDead && IsMovingOnGround() && !IsTraversing() &&
        !bWalkHeld && !bWantsToCrouch && !bProneWanted && Locomotion.HalfHeight >= 92.f &&
        FVector::DotProduct(Acceleration.GetSafeNormal2D(), C->GetActorForwardVector()) > .5f;
#if !UE_BUILD_SHIPPING
    if (bSprintHeld && !bPreviousSprintHeld && FParse::Param(FCommandLine::Get(), TEXT("ERMovementProbe")))
        UE_LOG(LogTemp,Display,TEXT("ER_SPRINT_REQUEST role=%d allowed=%d ground=%d walk=%d crouch=%d prone=%d half=%.1f acceleration=%s forward=%s recovery=%.1f"),
            int32(C->GetLocalRole()), Allowed, IsMovingOnGround(), bWalkHeld, bWantsToCrouch, bProneWanted, Locomotion.HalfHeight,
            *Acceleration.ToString(), *C->GetActorForwardVector().ToString(), GetRecoveryRemaining());
#endif
    // Only a fresh gesture can start a sprint; holding W through recovery cannot restart it.
    if (bSprintHeld && !bPreviousSprintHeld && Now >= Locomotion.RecoverUntil) SprintRequestUntil = Now + .25;
    if (!bSprintHeld) SprintRequestUntil = 0;
    // The gesture can arrive one saved move before its forward acceleration.
    if (bSprintHeld && SprintRequestUntil > Now && Allowed && Now >= Locomotion.RecoverUntil)
    {
        Locomotion.SprintUntil = Now + 5;
        Locomotion.RecoverUntil = Now + 15;
        SprintRequestUntil = 0;
    }
    if (IsSprinting() && (!bSprintHeld || !Allowed))
    {
        Locomotion.SprintUntil = Now;
        Locomotion.RecoverUntil = Now + 10;
    }
    bPreviousSprintHeld = bSprintHeld;
}
void UERMovement::UpdatePosture(float DeltaSeconds)
{
    if (!CharacterOwner || !UpdatedComponent || CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy) return;
    const auto* C = Cast<AERCharacter>(CharacterOwner);
    if ((C && C->Health->bDead) || IsTraversing()) return;
    // Enter low postures only on the floor; keep an existing posture when walking off an edge.
    if (!IsMovingOnGround() && Locomotion.HalfHeight >= 92.f) return;
    auto* Capsule = CharacterOwner->GetCapsuleComponent();
    const float Old = Capsule->GetUnscaledCapsuleHalfHeight();
    const float Target = bProneWanted ? 34.f : (bWantsToCrouch ? 62.5f : 92.5f);
    const bool ProneTransition = bProneWanted || Locomotion.ProneBlend > 0;
    const float Rate = ProneTransition ? 58.5f / 1.1f : 30.f / .4f;
    const float Next = FMath::FInterpConstantTo(Old, Target, DeltaSeconds, Rate);
    const float Scale = Capsule->GetShapeScale();
    const FVector Shift(0, 0, (Next - Old) * Scale);
    if (Next > Old)
    {
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ERPosture), false, CharacterOwner);
        // Check the eventual posture too: do not half-stand into an obstructing ceiling.
        if (GetWorld()->OverlapBlockingTestByChannel(UpdatedComponent->GetComponentLocation() + FVector(0,0,(Target-Old)*Scale),
            FQuat::Identity, UpdatedComponent->GetCollisionObjectType(),
            FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Target*Scale), Params)) return;
    }
    Capsule->SetCapsuleHalfHeight(Next, true);
    UpdatedComponent->MoveComponent(Shift, UpdatedComponent->GetComponentQuat(), false);
    Locomotion.HalfHeight = Next;
    Locomotion.ProneBlend = FMath::FInterpConstantTo(Locomotion.ProneBlend, bProneWanted ? 1.f : 0.f, DeltaSeconds, 1.f / 1.1f);
    CharacterOwner->bIsCrouched = !bProneWanted && Next < 92.f;
    bForceNextFloorCheck = true;
    ApplyPosturePresentation();
}
float UERMovement::GetEyeOffset() const
{
    // Feet stay fixed. The eye and camera share this physical offset, including transitions.
    const float H = Locomotion.HalfHeight;
    return H >= 62.5f ? FMath::GetMappedRangeValueClamped(FVector2D(62.5f,92.5f), FVector2D(50.f,77.5f), H) :
        FMath::GetMappedRangeValueClamped(FVector2D(34.f,62.5f), FVector2D(11.f,50.f), H);
}
void UERMovement::ApplyPosturePresentation()
{
    auto* C = Cast<AERCharacter>(CharacterOwner);
    if (!C) return;
    C->BaseEyeHeight = GetEyeOffset();
    C->Camera->SetRelativeLocation(FVector(0,0,C->BaseEyeHeight));
    const float Blend = Locomotion.ProneBlend;
    C->ProneHitVolume->SetCollisionEnabled(Blend > .5f && !C->Health->bDead ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    const float FeetOffset = C->GetTeamAppearance().Mesh ? C->GetTeamAppearance().FeetOffset : -92.5f;
    const FVector MeshLocation = FMath::Lerp(FVector(0,0,FeetOffset + 92.5f - Locomotion.HalfHeight), FVector(-55,0,-18), Blend);
    const FQuat Rotation = FQuat(FVector::YAxisVector, Blend * PI * .5f) * FRotator(0,-90,0).Quaternion();
    C->GetMesh()->SetRelativeLocationAndRotation(MeshLocation, Rotation);
    C->CacheInitialMeshOffset(MeshLocation, Rotation.Rotator());
    // Layer posture over Unreal's proxy offsets instead of discarding network smoothing.
    if (!C->IsLocallyControlled() && NetworkSmoothingMode != ENetworkSmoothingMode::Disabled &&
        (C->GetLocalRole() == ROLE_SimulatedProxy || GetNetMode() == NM_ListenServer))
        SmoothClientPosition_UpdateVisuals();
}
void UERMovement::OnRep_Locomotion()
{
    if (CharacterOwner && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
        CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(Locomotion.HalfHeight, true);
    ApplyPosturePresentation();
}
void UERMovement::SimulateMovement(float DeltaSeconds)
{
    Super::SimulateMovement(DeltaSeconds);
    ApplyPosturePresentation();
}
bool UERMovement::DoJump(bool bReplayingMoves, float DeltaTime)
{
    return Locomotion.HalfHeight >= 92.f && !bProneWanted && !IsChangingPosture() && Super::DoJump(bReplayingMoves, DeltaTime);
}
void UERMovement::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UERMovement, Locomotion);
}

bool UERMovement::ClientUpdatePositionAfterServerUpdate()
{
    // Unreal restores its own jump/crouch inputs after replay. Preserve our current
    // physical key intents too: an older saved move must not release a newly held key.
    const bool Walk = bWalkHeld, Climb = bClimbHeld, Sprint = bSprintHeld, Prone = bProneWanted;
    const bool PreviousSprint = bPreviousSprintHeld;
    const double Request = SprintRequestUntil, SprintUntil = Locomotion.SprintUntil, RecoverUntil = Locomotion.RecoverUntil;
    const bool Updated = Super::ClientUpdatePositionAfterServerUpdate();
    bWalkHeld = Walk; bClimbHeld = Climb; bSprintHeld = Sprint; bProneWanted = Prone;
    bPreviousSprintHeld = PreviousSprint; SprintRequestUntil = Request;
    Locomotion.SprintUntil = SprintUntil; Locomotion.RecoverUntil = RecoverUntil;
    return Updated;
}
