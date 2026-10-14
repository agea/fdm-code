#include "ERAccuracyProbe.h"
#include "ERCharacter.h"
#include "ERPlayerController.h"
#include "ERWeapon.h"
#include "ERMovement.h"
#include "ERMatch.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "HAL/PlatformMisc.h"

AERAccuracyProbe::AERAccuracyProbe() { bReplicates = true; bAlwaysRelevant = true; PrimaryActorTick.bCanEverTick = true; }
void AERAccuracyProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERAccuracyProbe,Phase);
}
void AERAccuracyProbe::Advance(int32 Next, float Delay) { Phase=Next; Deadline=GetWorld()->GetTimeSeconds()+Delay; ForceNetUpdate(); }
void AERAccuracyProbe::Check(bool Value, const TCHAR* Label)
{
    Failed |= !Value;
    UE_LOG(LogTemp,Display,TEXT("ER_ACCURACY_%s %s"),Value ? TEXT("PASS") : TEXT("FAIL"),Label);
}
void AERAccuracyProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    if (!HasAuthority())
    {
        auto* Pawn = Cast<AERCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
        if (!Pawn || !Pawn->Weapon || !Pawn->Weapon->Data) return;
        auto* Weapon = Pawn->Weapon.Get();
        if (!SawOwnerBase && FMath::IsNearlyEqual(Weapon->GetSpreadDegrees(),Weapon->Data->StandingSpreadDegrees,.01f))
        {
            SawOwnerBase=true; UE_LOG(LogTemp,Display,TEXT("ER_ACCURACY_CLIENT_PASS owner_base_data"));
        }
        if (ClientPhase != Phase)
        {
            ClientPhase=Phase;
            if (Phase==7) Pawn->ServerStartFire();
            if (Phase==8) Pawn->ServerStopFire();
        }
        if (!SawOwnerBloom && Phase>=7 && Weapon->GetBloomDegrees()>.2f && Weapon->MagazineAmmo<30)
        {
            SawOwnerBloom=true;
            UE_LOG(LogTemp,Display,TEXT("ER_ACCURACY_CLIENT_PASS owner_ammo_bloom_replicated ammo=%d spread=%.3f"),Weapon->MagazineAmmo,Weapon->GetSpreadDegrees());
        }
        if (!SawOwnerRecovery && SawOwnerBloom && Phase>=9 && Weapon->GetBloomDegrees()<.001f)
        {
            SawOwnerRecovery=true; UE_LOG(LogTemp,Display,TEXT("ER_ACCURACY_CLIENT_PASS owner_clock_recovers_bloom"));
        }
        return;
    }
    auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    if (!Mode || !Mode->IsMatchInProgress()) return;
    if (!Host || !Client)
    {
        for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (auto* PC=Cast<AERPlayerController>(It->Get())) { if (PC->IsLocalController()) Host=PC; else Client=PC; }
        if (!Host || !Client) return;
    }
    auto* H=Cast<AERCharacter>(Host->GetPawn()); auto* C=Cast<AERCharacter>(Client->GetPawn());
    if (!H || !C || !H->Weapon || !C->Weapon) return;
    const double Now=GetWorld()->GetTimeSeconds();
    auto* Move=Cast<UERMovement>(H->GetCharacterMovement());
    if (Phase==0)
    {
        H->TeleportTo(FVector(-1000,-1500,95),FRotator::ZeroRotator);
        C->TeleportTo(FVector(1000,-1500,95),FRotator(0,180,0));
        Host->SetControlRotation(FRotator::ZeroRotator); Host->ClientSetRotation(FRotator::ZeroRotator);
        Client->SetControlRotation(FRotator(0,180,0)); Client->ClientSetRotation(FRotator(0,180,0));
        H->Health->ProtectionEndServerTime=C->Health->ProtectionEndServerTime=Now+60;
        Advance(1,.5f); return;
    }
    if (Phase==3 || Phase==4) H->AddMovementInput(FVector(1,0,0));
    if (Phase==6 && ConeSamples<10 && Now>=Deadline)
    {
        H->ServerStartFire(); H->ServerStopFire();
        const auto* Weapon=H->Weapon.Get();
        const float Angle=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Weapon->LastShotAimDirection,Weapon->LastShotDirection),-1.,1.)));
        SamplesWithinCone &= Angle<=Weapon->LastShotSpreadDegrees+.01f;
        if (ConeSamples==0) FirstDirection=Weapon->LastShotDirection;
        else SamplesVary |= !FirstDirection.Equals(Weapon->LastShotDirection,.0001f);
        const FVector TraceDirection = (Weapon->LastShotTraceEnd - Weapon->LastShotTraceStart).GetSafeNormal();
        SamplesWithinCone &= Weapon->LastShotTraceCount == 1 && TraceDirection.Equals(Weapon->LastShotDirection, .001f);
        for (TActorIterator<AERProjectile> It(GetWorld()); It; ++It)
            if (It->GetOwner() == Weapon && It->Payload == EERPayload::Bullet) SamplesWithinCone = false;
        ++ConeSamples; Deadline=Now+.11; return;
    }
    if (Now<Deadline) return;
    switch (Phase)
    {
    case 1:
        Standing=H->Weapon->GetSpreadDegrees();
        Check(FMath::IsNearlyEqual(Standing,H->Weapon->Data->StandingSpreadDegrees,.001f),TEXT("standing_base_cone"));
        H->Crouch(); Advance(2,.5f); break;
    case 2:
        Check(H->bIsCrouched && H->Weapon->GetSpreadDegrees()<Standing,TEXT("crouch_narrows_cone"));
        H->UnCrouch(); Move->bWalkHeld=true; Advance(3,.7f); break;
    case 3:
        Walking=H->Weapon->GetSpreadDegrees();
        Check(H->GetVelocity().Size2D()>130 && Walking>Standing,TEXT("walking_adds_actual_movement_spread"));
        Move->bWalkHeld=false; Advance(4,.7f); break;
    case 4:
        Running=H->Weapon->GetSpreadDegrees();
        Check(H->GetVelocity().Size2D()>270 && Running>Walking,TEXT("running_wider_than_walking"));
        H->Jump(); Advance(5,.2f); break;
    case 5:
        Check(Move->IsFalling() && H->Weapon->GetSpreadDegrees()>Running,TEXT("jump_adds_airborne_spread"));
        Advance(6,.7f); break;
    case 6:
        Check(ConeSamples==10 && SamplesWithinCone,TEXT("ten_hitscan_traces_within_server_cone_no_bullet_actors"));
        Check(SamplesVary,TEXT("server_samples_different_directions"));
        Check(H->Weapon->GetBloomDegrees()<=H->Weapon->Data->MaxBloomDegrees+.001f,TEXT("burst_bloom_bounded"));
        H->Health->ProtectionEndServerTime=Now+60;
        AmmoBeforeBurst=C->Weapon->MagazineAmmo; Advance(7,.65f); break;
    case 7:
        Check(C->Weapon->MagazineAmmo<AmmoBeforeBurst && C->Weapon->GetSpreadDegrees()>Standing+.2f,TEXT("remote_rpc_burst_adds_server_bloom"));
        Advance(8,.4f); break;
    case 8:
        Check(!C->Weapon->bReloading && C->Weapon->GetBloomDegrees()>0,TEXT("stopped_burst_recovers_gradually"));
        Advance(9,1.4f); break;
    case 9:
        Check(C->Weapon->GetBloomDegrees()<.001f && FMath::IsNearlyEqual(C->Weapon->GetSpreadDegrees(),Standing,.01f),TEXT("server_bloom_returns_to_base"));
        C->Weapon->MagazineAmmo=0; C->Weapon->ForceNetUpdate();
        C->ServerStartFire(); C->ServerStopFire(); Advance(10,.5f); break;
    case 10:
        Check(C->Weapon->GetBloomDegrees()<.001f,TEXT("rejected_empty_shot_adds_no_bloom"));
        UE_LOG(LogTemp,Display,TEXT("ER_ACCURACY_COMPLETE result=%s"),Failed ? TEXT("FAIL") : TEXT("PASS"));
        FPlatformMisc::RequestExitWithStatus(false,Failed ? 1 : 0); Advance(11,1000); break;
    }
#endif
}
