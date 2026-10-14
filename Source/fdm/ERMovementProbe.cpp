#include "ERMovementProbe.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "HAL/PlatformMisc.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "GameFramework/PlayerController.h"
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
        if (C && Phase >= 33 && Phase <= 39)
        {
            auto* M = Cast<UERMovement>(C->GetCharacterMovement());
            if (LocalPhase != Phase)
            {
                LocalPhase = Phase; Elapsed = 0; Peak = 0;
                M->bWalkHeld = false; M->bClimbHeld = false;
                M->bSprintHeld = Phase == 33 || Phase == 34 || Phase == 36;
                M->bProneWanted = Phase == 37;
                C->UnCrouch();
            }
            Elapsed += DeltaTime;
            if (Phase == 37 && Elapsed > 1.6f && Peak == 0)
            {
                const bool OwnerReady = FMath::IsNearlyEqual(C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),34.f,.1f) && M->IsProne();
                bool ProxyReady = false;
                for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It)
                    if (It->GetLocalRole() == ROLE_SimulatedProxy)
                        ProxyReady |= FMath::IsNearlyEqual(It->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),34.f,.1f) && Cast<UERMovement>(It->GetCharacterMovement())->IsProne();
                UE_LOG(LogTemp,Display,TEXT("ER_MOVEMENT_CLIENT_%s owner_and_simulated_proxy_prone"),OwnerReady && ProxyReady ? TEXT("PASS") : TEXT("FAIL"));
                Peak = 1;
            }
            if (Phase >= 33 && Phase <= 36 && (Phase != 34 || Elapsed < 1.f || Elapsed > 9.f)) C->AddMovementInput(FVector(1,0,0));
        }
        if (C && Phase >= 40 && Phase <= 43)
        {
            auto* PC = Cast<APlayerController>(C->Controller);
            if (!PC) return;
            if (LocalPhase != Phase) { LocalPhase=Phase; Elapsed=0; Peak=0; PC->SetControlRotation(FRotator::ZeroRotator); }
            const float Previous = Elapsed; Elapsed += DeltaTime;
            auto Send = [PC,Previous,this](float At, FKey Key, EInputEvent Event)
            {
                if (Previous < At && Elapsed >= At)
                {
                    FInputKeyEventArgs Args;
                    Args.Key=Key; Args.Event=Event; Args.AmountDepressed=Event==IE_Released ? 0.f : 1.f;
                    Args.EventTimestamp=FPlatformTime::Cycles64();
                    PC->InputKey(Args);
                }
            };
            const FKey Key = Phase==40 ? EKeys::W : (Phase==43 ? EKeys::LeftControl : EKeys::C);
            Send(.05f,Key,IE_Pressed); Send(.12f,Key,IE_Released);
            if (Phase != 42) { Send(.22f,Key,IE_Pressed); Send(Phase==40 ? 1.f : .3f,Key,IE_Released); }
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
        if (FParse::Param(FCommandLine::Get(),TEXT("ERPostureProbe")))
        {
            Phase = 33; ForceNetUpdate();
            Host->TeleportTo(FVector(-900,-1900,95),FRotator::ZeroRotator);
        }
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
        if (Phase == 1 && Peak == 0 && Move->Velocity.Size2D() > 0 && Move->Velocity.Size2D() < Move->MaxWalkSpeed-1) Peak = Move->Velocity.Size2D();
        if (Elapsed > 1.5f)
        {
            Check(FMath::IsNearlyEqual(Move->Velocity.Size2D(), Phase == 1 ? 1000.f/3.6f : 500.f/3.6f,1),Phase == 1 ? TEXT("10 km/h run") : TEXT("5 km/h Shift walk"));
            if (Phase == 1) Check(Peak > 0 && Peak < Move->MaxWalkSpeed,TEXT("progressive acceleration"));
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
        if (Elapsed > .55f)
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
        if (Elapsed > .55f)
        {
            FVector Eye; FRotator View; Host->GetActorEyesViewPoint(Eye,View);
            Check(!Host->bIsCrouched && FMath::IsNearlyEqual(Host->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),92.5f) && FVector::Dist(Eye,Host->Camera->GetComponentLocation()) < 1,
                TEXT("185cm standing restored; camera and server aim agree"));
            Advance();
        }
        return;
    }
    if (Phase >= 33 && Phase <= 39)
    {
        auto* Remote = Cast<UERMovement>(Client->GetCharacterMovement());
        if (LocalPhase != Phase)
        {
            LocalPhase = Phase;
            if (Phase == 33 || Phase == 34 || Phase == 36 || Phase == 37)
            {
                Client->TeleportTo(FVector(-900,1900,95), FRotator::ZeroRotator);
                Client->Controller->SetControlRotation(FRotator::ZeroRotator);
            }
            if (Phase == 37) Move->bProneWanted = true;
            if (Phase == 39) Move->bProneWanted = false;
            if (Phase == 38)
            {
                PostureRoof = GetWorld()->SpawnActor<AActor>();
                auto* Box = NewObject<UBoxComponent>(PostureRoof);
                PostureRoof->SetRootComponent(Box); Box->SetBoxExtent(FVector(250,250,10));
                Box->SetCollisionProfileName(TEXT("BlockAll")); Box->RegisterComponent();
                const FVector P = Client->GetActorLocation();
                PostureRoof->SetActorLocation(FVector(P.X,P.Y,90));
            }
        }
        if (Phase == 33)
        {
            if (Elapsed > 1.5f && Peak == 0) { Check(Remote->IsSprinting() && FMath::IsNearlyEqual(Remote->Velocity.Size2D(),500.f,2),TEXT("remote double-W intent: 18 km/h sprint")); Peak=1; }
            if (Elapsed > 5.8f) { Check(Peak==1 && !Remote->IsSprinting() && Remote->GetRecoveryRemaining()>8 && Remote->GetRecoveryRemaining()<10,TEXT("sprint ends at 5 seconds; 10 second recovery")); Advance(); }
        }
        else if (Phase == 34 && Elapsed > 10.5f)
        {
            Check(!Remote->IsSprinting() && Remote->GetRecoveryRemaining()==0 && Remote->Velocity.Size2D()<280,TEXT("held sprint cannot restart after cooldown")); Advance();
        }
        else if (Phase == 35 && Elapsed > .5f) Advance();
        else if (Phase == 36 && Elapsed > 1)
        {
            Check(Remote->IsSprinting(),TEXT("fresh remote gesture restarts recovered sprint")); Advance();
        }
        else if (Phase == 37)
        {
            if (Elapsed > .25f && Peak == 0) { Check(Remote->Locomotion.HalfHeight > 34 && Remote->Locomotion.HalfHeight < 92.5f,TEXT("remote prone transition takes time")); Peak=1; }
            if (Elapsed > 1.8f)
            {
                FVector Eye; FRotator View; Client->GetActorEyesViewPoint(Eye,View);
                FHitResult BodyHit;
                const FVector BodyCenter = Client->GetActorLocation() + Client->GetActorQuat().RotateVector(FVector(100,0,-9));
                const FVector Across = Client->GetActorRightVector()*100;
                GetWorld()->LineTraceSingleByChannel(BodyHit,BodyCenter-Across,BodyCenter+Across,ECC_GameTraceChannel1);
                Check(BodyHit.GetActor()==Client && BodyHit.GetComponent()==Client->ProneHitVolume,TEXT("prone head/body beyond movement capsule remains hittable"));
                Check(Peak==1 && Remote->IsProne() && FMath::IsNearlyEqual(Remote->Locomotion.HalfHeight,34.f,.1f),TEXT("remote prone capsule reaches 68cm"));
                Check(FVector::Dist(Eye,Client->Camera->GetComponentLocation())<1,TEXT("prone camera equals authoritative eye"));
                Check(!Remote->IsSprinting() && Remote->GetRecoveryRemaining()>7,TEXT("posture interrupts sprint and starts recovery"));
                Advance();
            }
        }
        else if (Phase == 38 && Elapsed > 1.5f)
        {
            Check(Remote->IsProne() && FMath::IsNearlyEqual(Remote->Locomotion.HalfHeight,34.f,.1f),TEXT("blocked ceiling prevents standing from prone"));
            PostureRoof->Destroy(); Advance();
        }
        else if (Phase == 39 && Elapsed > 1.8f)
        {
            if (Peak == 0) { Check(!Remote->IsProne() && FMath::IsNearlyEqual(Remote->Locomotion.HalfHeight,92.5f,.1f),TEXT("remote stands gradually when ceiling clears")); Peak=1; }
            if (Remote->GetRecoveryRemaining()==0) Advance();
        }
        return;
    }
    if (Phase >= 40 && Phase <= 43)
    {
        auto* Remote = Cast<UERMovement>(Client->GetCharacterMovement());
        if (Phase == 40 && Elapsed > .85f && Peak == 0)
        {
            Check(Remote->IsSprinting() && Remote->Velocity.Size2D()>350,TEXT("InputKey double W reaches real sprint/movement bindings")); Peak=1;
        }
        if (Elapsed > 1.8f)
        {
            if (Phase == 40) Check(!Remote->IsSprinting() && Remote->GetRecoveryRemaining()>8,TEXT("InputKey W release ends sprint"));
            else Check(Phase == 42 ? !Remote->IsProne() && Remote->Locomotion.HalfHeight>92 : Remote->IsProne() && Remote->Locomotion.HalfHeight<35,
                Phase == 41 ? TEXT("InputKey double C reaches prone binding") : (Phase == 42 ? TEXT("InputKey single C rises from prone") : TEXT("InputKey double Ctrl reaches prone binding")));
            Advance();
        }
        return;
    }
    UE_LOG(LogTemp,Display,TEXT("ER_MOVEMENT_COMPLETE %s"),Failed ? TEXT("FAIL") : TEXT("PASS"));
    FPlatformMisc::RequestExit(false);
#endif
}
