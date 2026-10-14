#include "ERMovementProbe.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "HAL/PlatformMisc.h"
AERMovementProbe::AERMovementProbe() { bReplicates = true; bAlwaysRelevant = true; PrimaryActorTick.bCanEverTick = true; }
void AERMovementProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERMovementProbe,Phase); }
void AERMovementProbe::Check(bool Value,const FString& Label)
{
    UE_LOG(LogTemp,Display,TEXT("ER_MOVEMENT_%s %s"),Value ? TEXT("PASS") : TEXT("FAIL"),*Label);
    Failed |= !Value;
}
void AERMovementProbe::Advance() { ++Phase; Elapsed = 0; Peak = 0; SawTraversal = false; ForceNetUpdate(); }
void AERMovementProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    if (!HasAuthority())
    {
        auto* C = Cast<AERCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
        if (C && Phase >= 25 && Phase <= 29)
        {
            auto* M = Cast<UERMovement>(C->GetCharacterMovement());
            M->bWalkHeld = Phase == 26;
            M->bClimbHeld = Phase >= 28;
            if (LocalPhase != Phase) { LocalPhase=Phase; Elapsed=0; C->StopJumping(); }
            const float Previous = Elapsed; Elapsed += DeltaTime;
            if (Phase == 29 && Previous < .25f && Elapsed >= .25f) C->Jump();
            const float TargetX = Phase == 27 ? 650.f : (Phase == 28 ? 1300.f : 1950.f);
            if (Phase < 27 || (Elapsed > .25f && C->GetActorLocation().X < TargetX-30)) C->AddMovementInput(FVector(1,0,0));
        }
        return;
    }
    if (!Host || !Client)
    {
        for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (It->Get()->IsLocalController()) Host = Cast<AERCharacter>(It->Get()->GetPawn());
            else Client = Cast<AERCharacter>(It->Get()->GetPawn());
        }
        if (!Host || !Client) return;
    }
    auto* Move = Cast<UERMovement>(Host->GetCharacterMovement());
    if (Phase == 0)
    {
        Check(Move != nullptr && FMath::IsNearlyEqual(Host->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()*2,185),TEXT("standing capsule 185cm; custom movement loaded"));
        Host->TeleportTo(FVector(-700,-1900,95),FRotator::ZeroRotator);
        Advance(); return;
    }
    Elapsed += DeltaTime;
    if (Phase == 1 || Phase == 2)
    {
        Move->bWalkHeld = Phase == 2; Host->AddMovementInput(FVector(1,0,0));
        if (Phase == 1 && Elapsed > .08f && Elapsed < .12f) Peak = Move->Velocity.Size2D();
        if (Elapsed > 1.5f)
        {
            Check(FMath::IsNearlyEqual(Move->Velocity.Size2D(), Phase == 1 ? 1000.f/3.6f : 500.f/3.6f,1),Phase == 1 ? TEXT("10 km/h run") : TEXT("5 km/h Shift walk"));
            if (Phase == 1) Check(Peak > 0 && Peak < 180,TEXT("progressive acceleration"));
            Advance();
        }
        return;
    }
    if (Phase == 3)
    {
        if (Elapsed < .10f) Peak = FMath::Max(Peak,Move->Velocity.Size2D());
        if (Elapsed > .5f) { Check(Peak > 0 && Move->Velocity.Size2D() < 1,TEXT("inertial braking then stop")); GroundZ=Host->GetActorLocation().Z; Host->Jump(); Advance(); }
        return;
    }
    if (Phase == 4)
    {
        Peak = FMath::Max(Peak,Host->GetActorLocation().Z-GroundZ);
        if (Elapsed > .8f) { Check(Peak > 53 && Peak < 65 && Move->IsMovingOnGround(),FString::Printf(TEXT("jump peak %.1fcm; landed within 0.8s"),Peak)); Host->StopJumping(); Advance(); }
        return;
    }
    if (Phase >= 5 && Phase <= 24)
    {
        const int32 Index = Phase-5, Height=(Index+1)*10;
        const float X=(Index/5)*650.f, Y=(Index%5-2)*650.f;
        if (LocalPhase != Phase)
        {
            LocalPhase = Phase;
            Move->StopMovementImmediately(); Move->SetMovementMode(MOVE_Walking); Move->bWalkHeld=false; Move->bClimbHeld=Height>80;
            Host->StopJumping(); Host->TeleportTo(FVector(X-175,Y,95),FRotator::ZeroRotator);
            Host->Controller->SetControlRotation(FRotator::ZeroRotator);

        }
        if (Height > 150 && Elapsed >= .15f && Elapsed-DeltaTime < .15f) Host->Jump();
        if ((Height <= 150 || Elapsed >= .15f) && Host->GetActorLocation().X < X-30) Host->AddMovementInput(FVector(1,0,0));
        SawTraversal |= Move->IsTraversing();
        if (Elapsed > 2.1f)
        {
            const float Feet=Host->GetActorLocation().Z-92.5f;
            Check(FMath::IsNearlyEqual(Feet,Height+2.1f,4) && Move->IsMovingOnGround(),FString::Printf(TEXT("block %dcm reached (feet %.1fcm)"),Height,Feet));
            Check(SawTraversal == (Height>30),FString::Printf(TEXT("block %dcm traversal threshold"),Height));
            Host->StopJumping(); Move->bClimbHeld=false; Advance();
        }
        return;
    }
    if (Phase == 25 || Phase == 26)
    {
        if (LocalPhase != Phase)
        {
            LocalPhase=Phase;
            Client->TeleportTo(FVector(-700,1900,95),FRotator::ZeroRotator);
            Client->Controller->SetControlRotation(FRotator::ZeroRotator);
        }
        if (Elapsed > 2)
        {
            const auto* Remote = Cast<UERMovement>(Client->GetCharacterMovement());
            Check(FMath::IsNearlyEqual(Remote->Velocity.Size2D(),Phase==25 ? 1000.f/3.6f : 500.f/3.6f,2) && Remote->bWalkHeld==(Phase==26),Phase==25 ? TEXT("remote client run replicated") : TEXT("remote client walk intent replicated"));
            Advance();
        }
        return;
    }
    if (Phase >= 27 && Phase <= 29)
    {
        const int32 Height = Phase == 27 ? 80 : (Phase == 28 ? 150 : 200);
        const float X = Phase == 27 ? 650.f : (Phase == 28 ? 1300.f : 1950.f);
        const float Y = Phase == 27 ? 0.f : 1300.f;
        if (LocalPhase != Phase)
        {
            LocalPhase=Phase;
            Client->GetCharacterMovement()->StopMovementImmediately();
            Client->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            Client->TeleportTo(FVector(X-175,Y,95),FRotator::ZeroRotator);
            Client->ForceNetUpdate();
        }
        SawTraversal |= Cast<UERMovement>(Client->GetCharacterMovement())->IsTraversing();
        if (Elapsed > 3.1f)
        {
            const float Feet=Client->GetActorLocation().Z-92.5f;
            Check(FMath::IsNearlyEqual(Feet,Height+2.1f,4) && Client->GetCharacterMovement()->IsMovingOnGround() && SawTraversal,
                FString::Printf(TEXT("remote block %dcm (feet %.1fcm)"),Height,Feet));
            Advance();
        }
        return;
    }
    if (Phase == 30)
    {
        Move->StopMovementImmediately(); Move->SetMovementMode(MOVE_Walking); Move->bClimbHeld=false;
        Host->TeleportTo(FVector(1300-150,650,95),FRotator::ZeroRotator); // 140cm block, no climb input
        FVector Destination; float Height;
        Check(!Move->FindLedge(Destination,Height),TEXT("high ledge requires climb input"));
        Host->TeleportTo(FVector(1950-150,1300,95),FRotator::ZeroRotator); Move->bClimbHeld=true;
        Check(!Move->FindLedge(Destination,Height),TEXT("200cm unreachable from standing shoulders"));
        Host->TeleportTo(FVector(650-150,0,94.6f),FRotator::ZeroRotator);
        Check(Move->FindLedge(Destination,Height),TEXT("80cm ledge clear before adding ceiling"));
        auto* Roof = GetWorld()->SpawnActor<AActor>();
        auto* Box = NewObject<UBoxComponent>(Roof);
        Roof->SetRootComponent(Box); Box->SetBoxExtent(FVector(100,100,10));
        Box->SetCollisionProfileName(TEXT("BlockAll")); Box->RegisterComponent();
        Roof->SetActorLocation(FVector(650,0,230));
        Check(!Move->FindLedge(Destination,Height),TEXT("standing body clearance rejects low ceiling"));
        Roof->Destroy(); Host->Crouch();
        Advance(); return;
    }
    if (Phase == 31)
    {
        if (Elapsed > .3f)
        {
            FVector Eye; FRotator View; Host->GetActorEyesViewPoint(Eye,View);
            Check(Host->bIsCrouched && FMath::IsNearlyEqual(Host->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),62.5f) && FVector::Dist(Eye,Host->Camera->GetComponentLocation()) < 1,
                TEXT("125cm crouch; camera and server aim agree"));
            Host->UnCrouch(); Advance();
        }
        return;
    }
    if (Phase == 32)
    {
        if (Elapsed > .3f)
        {
            FVector Eye; FRotator View; Host->GetActorEyesViewPoint(Eye,View);
            Check(!Host->bIsCrouched && FMath::IsNearlyEqual(Host->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),92.5f) && FVector::Dist(Eye,Host->Camera->GetComponentLocation()) < 1,
                TEXT("185cm standing restored; camera and server aim agree"));
            Advance();
        }
        return;
    }
    UE_LOG(LogTemp,Display,TEXT("ER_MOVEMENT_COMPLETE %s"),Failed ? TEXT("FAIL") : TEXT("PASS"));
    FPlatformMisc::RequestExit(false);
#endif
}
