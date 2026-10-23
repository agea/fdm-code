#include "ERReconProbe.h"
#include "GameFramework/HUD.h"
#include "ERRecon.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERPlayerController.h"
#include "ERProceduralMap.h"
#include "ERSmokeCloud.h"
#include "ERBlockDebris.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

AERReconProbe::AERReconProbe() { bReplicates=true; bAlwaysRelevant=true; PrimaryActorTick.bCanEverTick=true; }
void AERReconProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERReconProbe,Phase);
}
void AERReconProbe::Check(bool Passed,const TCHAR* Label)
{
    UE_LOG(LogTemp,Display,TEXT("ER_RECON_PROBE_%s %s phase=%d"),Passed?TEXT("PASS"):TEXT("FAIL"),Label,Phase);
    Failed|=!Passed;
}
void AERReconProbe::Advance(int32 Next,float Delay) { Phase=Next; Deadline=GetWorld()->GetTimeSeconds()+Delay; ForceNetUpdate(); }
void AERReconProbe::ClientStep()
{
    auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    if (!PC || !PC->Recon || !PC->GetPawn() || !PC->GetPlayerState<AERPlayerState>()) return;
    const double Now=GetWorld()->GetTimeSeconds();
    if (ClientPhase!=Phase)
    {
        ClientPhase=Phase; ClientFireAt=Now+(Phase==14 ? 1.25 : Phase==15 ? 3. : .6); ClientFired=false; ClientValidated=false;
        const auto& S=PC->Recon->Snapshot;
        if (S.Epoch && S.Team==PC->GetPlayerState<AERPlayerState>()->Team)
            UE_LOG(LogTemp,Display,TEXT("ER_RECON_CLIENT_PASS phase=%d team=%d bytes=%d contacts=%d remaining=%d"),Phase,int32(S.Team),S.Explored.Num(),S.Contacts.Num(),S.Remaining);
    }
    if (!ClientValidated && Now>=ClientFireAt && (Phase==2 || Phase==3 || Phase==4 || Phase==5 || Phase==6 || Phase==7 || Phase==8 || Phase==9))
    {
        const auto& S=PC->Recon->Snapshot; bool Seen=false;
        for (const auto& C:S.Contacts) Seen|=!C.Drone && C.Team!=S.Team;
        const bool Expected=Phase==6 || Phase==8 ? false : Phase==4 ? S.Team==EERTeam::Omega : true;
        const bool Good=S.Epoch && S.Team==PC->GetPlayerState<AERPlayerState>()->Team && Seen==Expected;
        UE_LOG(LogTemp,Display,TEXT("ER_RECON_CLIENT_%s sightings phase=%d seen=%d expected=%d"),Good?TEXT("PASS"):TEXT("FAIL"),Phase,Seen,Expected);
        ClientValidated=true;
    }
    if(!ClientValidated && (Phase==14 || Phase==15) && Now>=ClientFireAt)
    {
        bool Good=false;
        for(TActorIterator<AERReconBase> It(GetWorld());It;++It)
            if(It->Team==PC->GetPlayerState<AERPlayerState>()->Team)
                Good=It->Emblem->GetDecalMaterial()!=nullptr &&
                    (Phase==14 ? FMath::Abs(It->Lid->GetRelativeRotation().Roll)>30 : It->Lid->GetRelativeRotation().IsNearlyZero(.1));
        UE_LOG(LogTemp,Display,TEXT("ER_RECON_CLIENT_%s case_animation phase=%d"),Good?TEXT("PASS"):TEXT("FAIL"),Phase);
        ClientValidated=true;
    }
    if (Phase==10 && PC->GetPlayerState<AERPlayerState>()->Team==EERTeam::Omega)
    {
        if (TActorIterator<AERReconDrone> It(GetWorld()); It)
        {
            for (;It;++It) if (It->Flight.Team==EERTeam::Alpha)
            {
                FVector Eye; FRotator Aim; PC->GetPawn()->GetActorEyesViewPoint(Eye,Aim);
                PC->SetControlRotation((It->GetActorLocation()-Eye).Rotation()); break;
            }
        }
        if (Now>=ClientFireAt && !ClientFired) { CastChecked<AERCharacter>(PC->GetPawn())->ServerStartFire(); ClientFired=true; }
    }
    else if (ClientFired) CastChecked<AERCharacter>(PC->GetPawn())->ServerStopFire();
}
void AERReconProbe::ShootDrone()
{
    if (auto* Drone=Recon->ActiveDrone(EERTeam::Alpha))
    {
        auto* C=CastChecked<AERCharacter>(Host->GetPawn());
        FPointDamageEvent Point(10,FHitResult(),FVector::UpVector,nullptr);
        Drone->TakeDamage(10,Point,Host,C->Weapon);
    }
}
void AERReconProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    if (!HasAuthority()) { ClientStep(); return; }
    const double Now=GetWorld()->GetTimeSeconds();
    if (Phase==0)
    {
        if (TActorIterator<AERReconDirector> It(GetWorld()); It) Recon=*It;
        for (FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
        {
            auto* PC=Cast<AERPlayerController>(It->Get()); if (!PC || !PC->GetPawn()) continue;
            if (PC->IsLocalController()) Host=PC;
            else if (PC->GetPlayerState<AERPlayerState>()->Team==EERTeam::Omega) Enemy=PC;
            else Ally=PC;
        }
        if (Recon && Recon->Frame.Epoch && Host && FParse::Param(FCommandLine::Get(),TEXT("ERReconGallery")))
        {
            auto* Base=Recon->Teams[0].Base.Get(); if (!Base) return;
            const FVector P=Base->GetActorLocation();
            auto* Camera=GetWorld()->SpawnActor<ACameraActor>(); GalleryCamera=Camera;
            Camera->GetCameraComponent()->SetFieldOfView(65);
            Camera->SetActorLocation(P+FVector(150,-180,115));
            Camera->SetActorRotation((P+FVector(0,0,60)-Camera->GetActorLocation()).Rotation());
            Host->SetViewTarget(Camera);
            auto* Drone=Recon->ActiveDrone(EERTeam::Alpha);
            Drone->Flight.Launch=P+FVector(0,0,140); Drone->Flight.StartedAt=Now+1000;
            if (FParse::Param(FCommandLine::Get(),TEXT("ERReconBeaconGallery")))
            { Drone->Flight.Center=Drone->Flight.Launch; Drone->Flight.Radius=FVector2D::ZeroVector; }
            Advance(100,5); return;
        }
        if (!Recon || !Recon->Frame.Epoch || !Host || !Enemy || !Ally) return;
        Check(Recon->ActiveDrone(EERTeam::Alpha) && Recon->ActiveDrone(EERTeam::Omega),TEXT("both_teams_launch"));
        Check(Recon->Remaining(EERTeam::Alpha)==5,TEXT("five_including_active"));
        Check(Recon->DebrisMeshes.Num()==5,TEXT("five_source_fragment_defaults_loaded"));
        for (auto& Team:Recon->Teams) if (Team.Drone.IsValid())
        {
            Team.Drone->Flight.Launch=FVector(50000,50000,50000); Team.Drone->Flight.StartedAt=Now+1000;
            Team.Drone->ForceNetUpdate();
        }
        Host->GetPawn()->TeleportTo(FVector(-2000,-1500,100),FRotator::ZeroRotator);
        Ally->GetPawn()->TeleportTo(FVector(-2100,-1200,100),FRotator::ZeroRotator);
        Enemy->GetPawn()->TeleportTo(FVector(2000,-1500,100),FRotator(0,180,0));
        for (auto* PC : {Host.Get(),Ally.Get(),Enemy.Get()})
        {
            CastChecked<AERCharacter>(PC->GetPawn())->GetCharacterMovement()->DisableMovement();
            PC->SetControlRotation(PC==Enemy?FRotator(0,180,0):FRotator::ZeroRotator);
            PC->ClientSetRotation(PC->GetControlRotation());
        }
        Cover=GetWorld()->SpawnActor<AActor>();
        auto* Box=NewObject<UBoxComponent>(Cover); Cover->SetRootComponent(Box); Cover->AddInstanceComponent(Box);
        Box->SetBoxExtent(FVector(40,500,250)); Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Box->SetCollisionObjectType(ECC_WorldStatic); Box->SetCollisionResponseToAllChannels(ECR_Block); Box->RegisterComponent();
        Cover->SetActorLocation(FVector(0,-1350,250));
        Advance(1,1.5); return;
    }
    if (Phase==10 && FApp::CanEverRender() && Now>=Deadline-2.f && !CapturedDebris)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Recon10_DebrisAir.png"),false,false);
        CapturedDebris=true;
    }
    if (Now<Deadline) return;
    auto HasEnemy=[&](AERPlayerController* PC)
    {
        for (const auto& Contact:PC->Recon->Snapshot.Contacts) if (!Contact.Drone && Contact.Team!=PC->GetPlayerState<AERPlayerState>()->Team) return true;
        return false;
    };
    switch (Phase)
    {
    case 100:
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Recon100_CaseDrone.png"),false,false);
        Advance(101,2); break;
    case 101:
    {
        const FVector P=Recon->Teams[0].Base->GetActorLocation();
        const bool BeaconView=FParse::Param(FCommandLine::Get(),TEXT("ERReconBeaconGallery"));
        if (BeaconView)
        {
            Recon->ActiveDrone(EERTeam::Alpha)->Flight.StartedAt=Now+1.95;
            if (Host->GetHUD()) Host->GetHUD()->bShowHUD=false;
        }
        GalleryCamera->SetActorLocation(P+FVector(100,-110,BeaconView ? 95 : 210));
        GalleryCamera->SetActorRotation((P+FVector(0,0,140)-GalleryCamera->GetActorLocation()).Rotation());
        Advance(102,2); break;
    }
    case 102:
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Recon102_Drone.png"),false,false);
        Advance(103,2); break;
    case 103:
        UE_LOG(LogTemp,Display,TEXT("ER_RECON_GALLERY_COMPLETE")); FPlatformMisc::RequestExit(false); break;
    case 1:
        Check(!HasEnemy(Host) && !HasEnemy(Ally) && !HasEnemy(Enemy),TEXT("cover_blocks_both_team_feeds"));
        Check(!Recon->CanSee(FVector(-1000,-1500,170),FVector::ForwardVector,FVector(1000,-1500,170),false),TEXT("physical_cover"));
        Cover->Destroy(); Advance(2,1.5); break;
    case 2:
        Check(HasEnemy(Host) && HasEnemy(Enemy),TEXT("open_sight_spots_enemy"));
        Check(FVector::Distance(Host->GetPawn()->GetActorLocation(),Enemy->GetPawn()->GetActorLocation())>3900,TEXT("visible_enemy_beyond_old_30m_limit"));
        if (FApp::CanEverRender()) Check(Recon->LastDrawnEnemies>0,TEXT("enemy_marker_actually_drawn"));
        SavedFog=Host->Recon->Snapshot.Explored;
        Host->SetControlRotation(FRotator(0,180,0)); Host->ClientSetRotation(Host->GetControlRotation());
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Recon02_Minimap.png"),false,false);
        Advance(3,1.5); break;
    case 3:
        Check(HasEnemy(Host),TEXT("teammate_shares_sight"));
        Ally->SetControlRotation(FRotator(0,180,0)); Ally->ClientSetRotation(Ally->GetControlRotation());
        Advance(4,1.5); break;
    case 4:
    {
        Check(!HasEnemy(Host) && !HasEnemy(Ally),TEXT("lost_contact_has_no_last_position"));
        bool Retained=SavedFog.Num()==ERRecon::FogBytes;
        for (int32 I=0; I<SavedFog.Num(); ++I) Retained &= (SavedFog[I]&Host->Recon->Snapshot.Explored[I])==SavedFog[I];
        Check(Retained,TEXT("exploration_persists"));
        // A 360-degree source behind the observer sees the enemy with cover gone.
        auto* Drone=Recon->ActiveDrone(EERTeam::Alpha);
        Drone->Flight.Launch=Enemy->GetPawn()->GetActorLocation()+FVector(-3600,0,2500); Drone->ForceNetUpdate();
        Advance(5,1.5); break;
    }
    case 5:
    {
        Check(HasEnemy(Host),TEXT("drone_adds_360_sight"));
        Check(FVector::Distance(Recon->ActiveDrone(EERTeam::Alpha)->GetActorLocation(),Enemy->GetPawn()->GetActorLocation())>4200,TEXT("drone_altitude_does_not_shrink_ground_footprint"));
        Host->SetControlRotation(FRotator::ZeroRotator); Host->ClientSetRotation(FRotator::ZeroRotator);
        Ally->SetControlRotation(FRotator::ZeroRotator); Ally->ClientSetRotation(FRotator::ZeroRotator);
        Smoke=GetWorld()->SpawnActor<AERSmokeCloud>(LoadClass<AERSmokeCloud>(nullptr,TEXT("/Game/EclipseRecoil/Effects/Smoke/BP_ERSmokeCloud.BP_ERSmokeCloud_C")),Enemy->GetPawn()->GetActorLocation()-FVector(0,0,92),FRotator::ZeroRotator);
        Check(IsValid(Smoke),TEXT("real_smoke_cloud_spawned"));
        if (Smoke) { Smoke->StartedAtServerTime=Now-3; Smoke->ForceNetUpdate(); }
        Advance(6,1.5); break;
    }
    case 6:
        Check(!HasEnemy(Host) && !HasEnemy(Ally) && !HasEnemy(Enemy),TEXT("smoke_blocks_player_teammate_and_drone_contacts"));
        if (FApp::CanEverRender())
        {
            Check(Recon->LastDrawnEnemies==0,TEXT("smoke_removes_rendered_enemy_marker"));
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Recon06_Smoke.png"),false,false);
        }
        if (Smoke) Smoke->AddShot(Enemy->GetPawn()->GetActorLocation()-FVector(0,0,60),Enemy->GetPawn()->GetActorLocation()+FVector(0,0,60),400,12);
        Advance(7,1.5); break;
    case 7:
    {
        Check(HasEnemy(Host) && HasEnemy(Ally) && HasEnemy(Enemy),TEXT("blast_dispersal_restores_recon"));
        if (Smoke) Smoke->Destroy();
        Smoke=GetWorld()->SpawnActor<AERSmokeCloud>(LoadClass<AERSmokeCloud>(nullptr,TEXT("/Game/EclipseRecoil/Effects/Smoke/BP_ERSmokeCloud.BP_ERSmokeCloud_C")),Enemy->GetPawn()->GetActorLocation()-FVector(0,0,92),FRotator::ZeroRotator);
        if (Smoke) { Smoke->StartedAtServerTime=Now-3; Smoke->ForceNetUpdate(); }
        Advance(8,1.5); break;
    }
    case 8:
        Check(!HasEnemy(Host) && !HasEnemy(Enemy),TEXT("fresh_smoke_hides_contacts_again"));
        if (Smoke) Smoke->Destroy();
        Advance(9,1.5); break;
    case 9:
    {
        Check(HasEnemy(Host) && HasEnemy(Enemy),TEXT("smoke_removal_restores_recon"));
        auto* Drone=Recon->ActiveDrone(EERTeam::Alpha);
        FDamageEvent Generic;
        Check(Drone->TakeDamage(100,Generic,Host,nullptr)==0,TEXT("non_bullet_cannot_destroy_drone"));
        Check(!Recon->Teams[0].Base->CanBeDamaged() && Recon->Teams[0].Base->Case->GetStaticMesh(),TEXT("closed_case_has_art_and_is_indestructible"));
        // Real remote input fires the ordinary rifle at the opposing drone.
        Drone->Flight.Launch=Enemy->GetPawn()->GetActorLocation()+FVector(-800,0,100); Drone->ForceNetUpdate();
        Host->SetControlRotation(FRotator(0,180,0)); Host->ClientSetRotation(Host->GetControlRotation());
        Ally->SetControlRotation(FRotator(0,180,0)); Ally->ClientSetRotation(Ally->GetControlRotation());
        FVector Eye; FRotator Aim; Enemy->GetPawn()->GetActorEyesViewPoint(Eye,Aim);
        Enemy->SetControlRotation((Drone->Flight.Launch-Eye).Rotation()); Enemy->ClientSetRotation(Enemy->GetControlRotation());
        if (FApp::CanEverRender())
        {
            auto* Camera=GetWorld()->SpawnActor<ACameraActor>(); GalleryCamera=Camera;
            Camera->GetCameraComponent()->SetFieldOfView(65);
            Camera->SetActorLocation(Drone->Flight.Launch+FVector(250,-300,120));
            Camera->SetActorRotation((Drone->Flight.Launch-FVector(0,0,70)-Camera->GetActorLocation()).Rotation());
            Host->SetViewTarget(Camera);
        }
        Advance(10,3); break;
    }
    case 10:
        CastChecked<AERCharacter>(Enemy->GetPawn())->ServerStopFire();
        Check(!Recon->ActiveDrone(EERTeam::Alpha),TEXT("remote_authorized_bullet_destroys_drone"));
        Check(Recon->BreakEvents==1,TEXT("one_break_event_for_authorized_hit"));
        if (FApp::CanEverRender())
        {
            bool Fallen=false;
            for (AERBlockDebris* Piece:Recon->LocalDebris) if (IsValid(Piece)) Fallen|=Piece->GetActorLocation().Z<Recon->LastBreakPosition.Z-30;
            Check(Recon->LocalDebris.Num()==5 && Fallen,TEXT("five_real_drone_fragments_fall_with_gravity"));
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Recon10_Debris.png"),false,false);
        }
        Host->SetViewTarget(Host->GetPawn());
        Check(Recon->Remaining(EERTeam::Alpha)==4 && Recon->NextLaunch(EERTeam::Alpha)>Now+25,TEXT("replacement_scheduled_30_seconds"));
        ShotAt=Recon->NextLaunch(EERTeam::Alpha)-30;
        Check(!HasEnemy(Host),TEXT("destroyed_drone_stops_spotting_immediately"));
        UE_LOG(LogTemp,Display,TEXT("ER_RECON_LATE_JOIN_READY"));
        Advance(11,25); break;
    case 11:
        Check(!Recon->ActiveDrone(EERTeam::Alpha),TEXT("no_early_replacement"));
        Advance(12,FMath::Max(1.,ShotAt+31-Now)); break;
    case 12:
        Check(Recon->ActiveDrone(EERTeam::Alpha) && Recon->Remaining(EERTeam::Alpha)==4,TEXT("replacement_after_elapsed_30_seconds"));
        for (int32 Number=2; Number<=5; ++Number)
        {
            ShootDrone();
            if (Number<5) { Recon->Teams[0].NextLaunch=Now-.01; Recon->LaunchDrone(0); }
        }
        Check(!Recon->ActiveDrone(EERTeam::Alpha) && Recon->Remaining(EERTeam::Alpha)==0 && Recon->NextLaunch(EERTeam::Alpha)==0,TEXT("five_losses_exhaust_stock"));
        Advance(13,1); break;
    case 13:
    {
        const uint32 Old=Recon->Frame.Epoch; Recon->ResetRecon();
        Check(Recon->Frame.Epoch>Old && Recon->Remaining(EERTeam::Alpha)==5 && Recon->Teams[0].Explored.FilterByPredicate([](uint8 B){return B!=0;}).IsEmpty(),TEXT("round_reset_clears_fog_and_restores_stock"));
        Check(Recon->LocalDebris.IsEmpty(),TEXT("round_reset_clears_cosmetic_fragments"));
        Advance(14,1.5); break;
    }
    case 14:
    {
        auto* Base=Recon->Teams[0].Base.Get();
        Check(Base && Base->Lid->GetStaticMesh() && Base->Lock->GetStaticMesh() && Base->LidOpening.Num()==101,
            TEXT("case_keeps_original_animated_parts_and_keys"));
        Check(Base && FMath::Abs(Base->Lid->GetRelativeRotation().Roll)>30,
            TEXT("case_opens_during_real_round_reset_launch"));
        Check(Base && Base->Emblem->GetDecalMaterial()==Base->AlphaGraffiti && Base->AlphaGraffiti && Base->OmegaGraffiti,
            TEXT("case_has_team_emblem_material"));
        Check(Base && Recon->ActiveDrone(EERTeam::Alpha) &&
            FMath::IsNearlyEqual(Recon->ActiveDrone(EERTeam::Alpha)->Flight.StartedAt-Base->LaunchAt,1.2,.001),
            TEXT("drone_ascent_waits_for_case_opening"));
        Advance(15,4); break;
    }
    case 15:
        Check(Recon->Teams[0].Base.IsValid() && Recon->Teams[0].Base->Lid->GetRelativeRotation().IsNearlyZero(.1),
            TEXT("case_closes_after_launch_animation"));
        UE_LOG(LogTemp,Display,TEXT("ER_RECON_PROBE_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));
        FPlatformMisc::RequestExitWithStatus(false,Failed?1:0); break;
    }
#endif
}
