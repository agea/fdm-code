#include "ERArsenalProbe.h"
#include "ERCharacter.h"
#include "ERPlayerController.h"
#include "ERMatch.h"
#include "ERWeapon.h"
#include "ERGrenadeExplosion.h"
#include "ERSmokeCloud.h"
#include "NiagaraComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "ShaderCompiler.h"
#include "UnrealClient.h"

AERArsenalProbe::AERArsenalProbe() { bReplicates = true; bAlwaysRelevant = true; PrimaryActorTick.bCanEverTick = true; }
void AERArsenalProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERArsenalProbe, Phase); }
void AERArsenalProbe::Check(bool Value, const TCHAR* Label)
{ Failed |= !Value; UE_LOG(LogTemp, Display, TEXT("ER_ARSENAL_%s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), Label); }
void AERArsenalProbe::Advance(int32 Next, float Delay) { Phase=Next; Deadline=GetWorld()->GetTimeSeconds()+Delay; ForceNetUpdate(); }
void AERArsenalProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    const bool SmokeGallery=FParse::Param(FCommandLine::Get(),TEXT("ERSmokeGallery"));
    if (FParse::Param(FCommandLine::Get(), TEXT("ERGrenadeGallery")) || SmokeGallery)
    {
        auto* PC=Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController());
        auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
        const bool NoShots=FParse::Param(FCommandLine::Get(),TEXT("ERGrenadeNoShots"));
        if (NoShots && Phase>0 && Phase<(SmokeGallery ? 6 : 5))
        {
            const double FrameMs=FApp::GetDeltaTime()*1000.;
            ++BurstFrames; BurstFrameTotal+=FrameMs; BurstFrameMax=FMath::Max(BurstFrameMax,FrameMs);
            if(FrameMs>33.34) ++BurstSlowFrames;
        }
        if (!C || !C->Weapon || !PC->IsLocalController() || GetWorld()->GetTimeSeconds()<FMath::Max(SmokeGallery ? 20. : 5.,Deadline)) return;
        if(SmokeGallery && Phase==0 && GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) return;
        if (Phase==0)
        {
            PC->SetControlRotation(FRotator(-10,0,0));
            C->GetCharacterMovement()->StopMovementImmediately();
            const FVector Above = C->Camera->GetComponentLocation()+FVector(400,0,0);
            FHitResult Floor;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(ERGrenadeGallery),false,C);
            const bool Found = GetWorld()->LineTraceSingleByObjectType(Floor, Above, Above-FVector(0,0,1000),FCollisionObjectQueryParams(ECC_WorldStatic),Query);
            const FVector Position = Found ? Floor.ImpactPoint+Floor.ImpactNormal*4 : Above;
            auto* P=GetWorld()->SpawnActorDeferred<AERProjectile>(C->Weapon->ProjectileClass,FTransform(FRotator::ZeroRotator,Position),C->Weapon,C,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            P->Payload=SmokeGallery ? EERPayload::Smoke : EERPayload::HE; P->Damage=0; P->FuseDuration=.5f;
            P->ThrownMesh=C->Arsenal[SmokeGallery ? 9 : 8]->Mesh;
            P->Movement->InitialSpeed=0; P->Movement->MaxSpeed=0; P->Movement->Velocity=FVector::ZeroVector;
            UGameplayStatics::FinishSpawningActor(P,P->GetActorTransform());
            UE_LOG(LogTemp,Display,TEXT("ER_%s_GALLERY_START position=%s"),SmokeGallery ? TEXT("SMOKE") : TEXT("GRENADE"),*Position.ToString());
            Advance(1,SmokeGallery ? 1.f : .56f);
        }
        else
        {
            if (!NoShots) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/(SmokeGallery ? TEXT("Screenshots/Smoke") : TEXT("Screenshots/Grenade"))/FString::Printf(TEXT("Burst_%02d.png"),Phase),true,false);
            UE_LOG(LogTemp,Display,TEXT("ER_%s_GALLERY_FRAME %d"),SmokeGallery ? TEXT("SMOKE") : TEXT("GRENADE"),Phase);
            if(SmokeGallery && Phase==3)
                for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It)
                {
                    const FVector Center=It->GetActorLocation()+FVector(0,0,120);
                    C->MulticastShot(C->Camera->GetComponentLocation(),Center+(Center-C->Camera->GetComponentLocation()).GetSafeNormal()*1000);
                }
            if (Phase==(SmokeGallery ? 6 : 5))
            {
                SetActorTickEnabled(false); UE_LOG(LogTemp,Display,TEXT("ER_%s_GALLERY_COMPLETE"),SmokeGallery ? TEXT("SMOKE") : TEXT("GRENADE"));
                if(NoShots && BurstFrames) UE_LOG(LogTemp,Display,TEXT("ER_%s_FRAME_SAMPLE frames=%d mean_ms=%.2f max_ms=%.2f frames_above_33ms=%d"),
                    SmokeGallery ? TEXT("SMOKE") : TEXT("GRENADE"),BurstFrames,BurstFrameTotal/BurstFrames,BurstFrameMax,BurstSlowFrames);
            }
            else Advance(Phase+1,SmokeGallery ? (Phase==1 ? 2.f : Phase==2 ? 3.f : Phase==3 ? .25f : Phase==4 ? 4.f : 2.5f)
                : (Phase==1 ? .29f : Phase==2 ? .5f : Phase==3 ? .65f : 1.f));
        }
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("ERArsenalGallery")))
    {
        auto* PC=Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController());
        auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
        if (!C || C->Arsenal.Num()!=11 || !C->Weapon || !PC->IsLocalController() || GetWorld()->GetTimeSeconds()<Deadline) return;
        if (GalleryIndex==0 && GetWorld()->GetTimeSeconds()<5.f) return;
        PC->SetControlRotation(FRotator(0,0,0));
        if (GalleryIndex==11) { C->Weapon->Data=C->Arsenal[0]; C->Weapon->MagazineAmmo=30; C->Weapon->ReserveAmmo=90; SetActorTickEnabled(false); return; }
        if (Phase==0) { C->Weapon->Data=C->Arsenal[GalleryIndex]; C->Weapon->MagazineAmmo=C->Weapon->Data->MagazineSize;
            C->Weapon->ReserveAmmo=C->Weapon->Data->InitialReserve; Advance(1,1.f); }
        else
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Arsenal")/FString::Printf(TEXT("Weapon_%02d.png"),GalleryIndex),true,false);
            UE_LOG(LogTemp, Display, TEXT("ER_ARSENAL_GALLERY index=%d name=%s"), GalleryIndex, *C->Weapon->Data->DisplayName);
            ++GalleryIndex; Advance(0,.5f);
        }
        return;
    }
    if (!HasAuthority())
    {
        auto* PC=Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController());
        auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
        if (!C || !C->Weapon || !C->Weapon->Data) return;
        if (Phase!=ClientPhase)
        {
            ClientPhase=Phase;
            if (Phase==1) { C->ServerSelectWeapon(1); C->ServerStartFire(); }
            if (Phase==2) { C->ServerStopFire(); C->ServerReload(); }
            if (Phase==3) PC->ServerSetPrimary(4);
            if (Phase==7)
            {
                PC->ScrollWeapon(1);
                WheelBurstEnd=GetWorld()->GetTimeSeconds()+.4;
                NextWheelPulse=GetWorld()->GetTimeSeconds()+.05;
            }
            if (Phase==8) PC->ScrollWeapon(-1);
            if(Phase==9)
                for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It)
                    if(It->ShotInteractionCount>0) UE_LOG(LogTemp,Display,TEXT("ER_SMOKE_REMOTE_CUT events=%u"),It->ShotInteractionCount);
            UE_LOG(LogTemp, Display, TEXT("ER_ARSENAL_REMOTE phase=%d slots=%d name=%s mesh=%s ammo=%d"), Phase, C->Inventory.Num(),
                *C->Weapon->Data->DisplayName, *GetNameSafe(C->FirstPersonRifle->GetSkeletalMeshAsset()), C->Weapon->MagazineAmmo);
        }
        if (Phase==7 && GetWorld()->GetTimeSeconds()<WheelBurstEnd && GetWorld()->GetTimeSeconds()>=NextWheelPulse)
        {
            PC->ScrollWeapon(1);
            NextWheelPulse=GetWorld()->GetTimeSeconds()+.05;
        }
        if (PC->FlashOpacity>.1f) UE_LOG(LogTemp, Display, TEXT("ER_ARSENAL_REMOTE_FLASH opacity=%.2f"),PC->FlashOpacity);
        return;
    }
    if (Phase==0)
    {
        for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if (auto* PC=Cast<AERPlayerController>(It->Get())) { if (PC->IsLocalController()) Host=PC; else Client=PC; }
        if (!Host || !Client || !Host->GetPawn() || !Client->GetPawn()) return;
        auto* H=CastChecked<AERCharacter>(Host->GetPawn()); auto* C=CastChecked<AERCharacter>(Client->GetPawn());
        Check(H->Inventory.Num()==5 && C->Inventory.Num()==5,TEXT("replicated_loadout_five_slots"));
        bool Valid=H->Arsenal.Num()==11;
        for (auto Data:H->Arsenal) Valid &= Data && Data->Mesh && Data->IdleAnimation && Data->FireAnimation;
        Check(Valid,TEXT("all_eleven_asset_profiles_present"));
        Host->SetControlRotation(FRotator(65,0,0)); Client->SetControlRotation(FRotator(65,180,0));
        H->Health->ProtectionEndServerTime=C->Health->ProtectionEndServerTime=GetWorld()->GetTimeSeconds()+100;
        Advance(1,1.f); return;
    }
    if (GetWorld()->GetTimeSeconds()<Deadline) return;
    auto* H=Cast<AERCharacter>(Host->GetPawn()); auto* C=Cast<AERCharacter>(Client->GetPawn());
    if (!H || !C || !H->Weapon || !C->Weapon) { Check(false,TEXT("valid_pawns")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
    auto SpawnPayload=[this,H](EERPayload Kind, const FVector& Position)
    {
        auto* P=GetWorld()->SpawnActorDeferred<AERProjectile>(H->Weapon->ProjectileClass,FTransform(FRotator::ZeroRotator,Position),H->Weapon,H,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        P->Payload=Kind; P->Damage=100; P->ExplosionRadius=600; P->FuseDuration=.2f;
        P->Movement->InitialSpeed=0; P->Movement->MaxSpeed=0; P->Movement->Velocity=FVector::ZeroVector;
        UGameplayStatics::FinishSpawningActor(P,P->GetActorTransform());
        return P;
    };
    switch (Phase)
    {
    case 1:
        Check(C->Weapon==C->Inventory[1] && C->Weapon->MagazineAmmo==6,TEXT("owner_switch_and_semi_auto_one_round_while_held"));
        Check(C->Inventory[0]->MagazineAmmo==30,TEXT("inactive_primary_keeps_ammo"));
        Advance(2,2.f); break;
    case 2:
        Check(C->Weapon->MagazineAmmo==7 && C->Weapon->ReserveAmmo==20,TEXT("sidearm_reload_consumes_reserve"));
        C->ServerSelectWeapon(99); Check(C->Weapon==C->Inventory[1],TEXT("invalid_slot_rejected"));
        Advance(3,.5f); break;
    case 3:
        Check(Client->GetPlayerState<AERPlayerState>()->PrimaryWeapon==4 && C->Inventory[0]->Data==C->Arsenal[0],TEXT("loadout_choice_waits_for_respawn"));
        C->Health->ProtectionEndServerTime=0; C->Health->ReceiveDamage(100,nullptr);
        Advance(4,3.4f); break;
    case 4:
        GetWorld()->GetAuthGameMode<AERGameMode>()->RequestRespawn(Client);
        Advance(5,.6f); break;
    case 5:
        Check(!C->Health->bDead && C->Weapon->Data==C->Arsenal[4] && C->Weapon->MagazineAmmo==100 && C->Inventory[1]->MagazineAmmo==7,
            TEXT("chosen_m249_and_sidearm_reset_on_respawn"));
        H->Weapon->Data=H->Arsenal[3]; H->Weapon->MagazineAmmo=7;
        Host->SetControlRotation(FRotator(80,0,0)); H->Weapon->StartFire(); H->Weapon->StopFire();
        { int32 Count=0; for(TActorIterator<AERProjectile> It(GetWorld());It;++It) if(It->GetOwner()==H->Weapon && It->Payload==EERPayload::Bullet) ++Count;
          Check(Count==6 && H->Weapon->MagazineAmmo==6,TEXT("shotgun_six_projectiles_one_shell")); }
        Advance(6,.6f); break;
    case 6:
        H->Weapon->Data=H->Arsenal[6]; H->Weapon->MagazineAmmo=1; H->Weapon->StartFire(); H->Weapon->StopFire();
        { bool Rocket=false; for(TActorIterator<AERProjectile> It(GetWorld());It;++It) if(It->Payload==EERPayload::Rocket)
            Rocket |= FMath::IsNearlyEqual(It->Movement->Velocity.Size(),2200.f,1.f);
          Check(Rocket && H->Weapon->MagazineAmmo==0,TEXT("rpg_authorized_slow_rocket_consumes_round")); }
        SpawnPayload(EERPayload::Smoke,H->GetActorLocation()+FVector(100,0,50));
        SpawnPayload(EERPayload::Stun,C->Camera->GetComponentLocation()+FVector(0,80,0));
        SpawnPayload(EERPayload::HE,H->Camera->GetComponentLocation()+FVector(0,80,0));
        Advance(7,1.f); break;
    case 7:
        Check(H->Health->Health<100,TEXT("he_applies_server_area_damage_to_thrower"));
        Check(C->Health->Health==100,TEXT("stun_does_not_deal_health_damage"));
        { bool Smoke=false; for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It)
            Smoke |= It->System && It->GetIsReplicated() && It->GetLifeSpan()>10 && !It->GetActorEnableCollision();
          Check(Smoke,TEXT("smoke_volume_persists_after_fuse")); }
        H->Weapon->Data=H->Arsenal[0]; H->Weapon->MagazineAmmo=30; H->Weapon->ForceNetUpdate();
        Check(C->Weapon==C->Inventory[1],TEXT("wheel_burst_changes_only_one_slot"));
        { bool Burst=false, HEProjectile=false;
          for(TActorIterator<AERGrenadeExplosion> It(GetWorld());It;++It)
              Burst |= It->Debris->GetSkeletalMeshAsset()!=nullptr && It->Debris->IsPlaying() && !It->GetIsReplicated()
                  && It->Debris->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
          for(TActorIterator<AERProjectile> It(GetWorld());It;++It) HEProjectile |= It->Payload==EERPayload::HE;
          Check(Burst && !HEProjectile,TEXT("he_animation_outlives_projectile_without_collision_or_replication")); }
        Advance(8,.6f); break;
    case 8:
        Check(C->Weapon==C->Inventory[0],TEXT("wheel_reverse_after_quiet_changes_one_slot"));
        for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It)
        {
            const FVector Center=It->GetActorLocation()+FVector(0,0,It->SimSize.Z*.5f);
            Host->SetControlRotation((Center-H->Camera->GetComponentLocation()).Rotation());
            H->Weapon->StartFire(); H->Weapon->StopFire();
        }
        Advance(9,.15f); break;
    case 9:
        { bool Cut=false;for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It) Cut|=It->LastColliderCount>0;
          Check(Cut,TEXT("authorized_shot_submits_smoke_cut_positions")); }
        Advance(10,1.2f); break;
    case 10:
        { int32 Bursts=0; for(TActorIterator<AERGrenadeExplosion> It(GetWorld());It;++It) ++Bursts;
          Check(Bursts==0,TEXT("he_animation_cleans_up_after_playback")); }
        Advance(11,10.5f); break;
    case 11:
        { int32 Clouds=0; for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It) ++Clouds;
          Check(Clouds==0,TEXT("smoke_cloud_cleans_up_after_duration")); }
        UE_LOG(LogTemp, Display, TEXT("ER_ARSENAL_COMPLETE result=%s"),Failed ? TEXT("FAIL") : TEXT("PASS"));
        FPlatformMisc::RequestExitWithStatus(false,Failed ? 1 : 0); Advance(12,1000); break;
    }
#endif
}
