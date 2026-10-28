#include "ERLoadoutProbe.h"
#include "ERLoadout.h"
#include "ERClaymore.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "TimerManager.h"
#include "ERCharacter.h"
#include "ERPlayerController.h"
#include "ERMatch.h"
#include "ERWeapon.h"
#include "ERMenuPreview.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Net/UnrealNetwork.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
AERLoadoutProbe::AERLoadoutProbe()
{ bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true; }
void AERLoadoutProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AERLoadoutProbe,Phase);DOREPLIFETIME(AERLoadoutProbe,MineDeadlines); }
void AERLoadoutProbe::ServerObserved_Implementation(int32 Step) { if(Step==Phase)Acknowledged=Step; }
void AERLoadoutProbe::Check(bool Value,const TCHAR* Label)
{ if(!Value) { Failed=true;UE_LOG(LogTemp,Error,TEXT("ER_LOADOUT_FAIL phase=%d %s"),Phase,Label); } }
bool AERLoadoutProbe::Observe()
{
    int32 Count=0;
    const bool Audit=!HasAuthority() && FMath::Fmod(GetWorld()->GetTimeSeconds(),1.f)<.02f;
    for(TActorIterator<AERCharacter> It(GetWorld());It;++It)
    {
        auto* C=*It;const auto* PS=C->GetPlayerState<AERPlayerState>();if(!PS || PS->IsABot() || C->Health->bDead)continue;
        const int32 Index=Phase-1;const auto Local=ERLoadouts::Resolve(ERLoadouts::Presets()[Index/3],ERLoadouts::Variants()[Index%3],PS->Team==EERTeam::Omega,C);
        const auto* GS=GetWorld()->GetGameState<AERGameState>();
        if(!GS || GS->LoadoutCatalog.Num()!=30)return false;
        const auto* Server=GS->LoadoutCatalog.FindByPredicate([&](const auto& K){return K.Preset==Local.Preset && K.Variant==Local.Variant && K.bOmega==Local.bOmega;});
        if(!Server)return false;const auto Kit=*Server;
        if(Audit)UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_WAIT phase=%d pawn=%s active=%s/%s slots=%d/%d weapon=%s skin=%d/%d hp=%.1f ap=%.1f"),Phase,*C->GetName(),*C->ActivePreset.ToString(),*C->ActiveVariant.ToString(),C->Inventory.Num(),Kit.Items.Num(),*GetNameSafe(C->Weapon),PS->ActiveSkin,Kit.Skin,C->Health->Health,C->Health->Armor);
        if(C->ActivePreset!=Kit.Preset || C->ActiveVariant!=Kit.Variant || C->Inventory.Num()!=Kit.Items.Num() || !C->Weapon || C->Weapon!=C->Inventory[0])return false;
        if(HasAuthority())
        {
            int32 Owned=0;for(TActorIterator<AERWeapon> W(GetWorld());W;++W)if(W->GetOwner()==C && !W->IsActorBeingDestroyed())++Owned;
            if(Owned!=Kit.Items.Num())return false;
        }
        if(PS->ActiveSkin!=Kit.Skin || C->Health->Health!=100 || C->Health->Armor!=100 || !C->Health->bHelmet)return false;
        for(int32 I=0;I<Kit.Items.Num();++I)
        {
            const auto* W=C->Inventory[I].Get();const auto& Entry=Kit.Items[I];
            if(!W || !W->Data || W->Data!=Entry.Data || ((HasAuthority() || C->IsLocallyControlled()) && (W->MagazineAmmo!=Entry.Loaded || W->ReserveAmmo!=Entry.Reserve)))
            { if(Audit)UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_WAIT_ITEM slot=%d data=%s expected=%s ammo=%d/%d reserve=%d/%d"),I,*GetNameSafe(W?W->Data.Get():nullptr),*GetNameSafe(Entry.Data),W?W->MagazineAmmo:-1,Entry.Loaded,W?W->ReserveAmmo:-1,Entry.Reserve);return false; }
        }
        ++Count;
    }
    if(Count!=2)return false;
    if(!HasAuthority())
    {
        auto* PC=Cast<AERPlayerController>(GetOwner());if(!PC)return false;
        const auto Kit=PC->ResolveMenuLoadout(ERLoadouts::Presets()[(Phase-1)/3],ERLoadouts::Variants()[(Phase-1)%3]);
        if(Kit.Items.Num()<2 || Kit.Items[1].Reserve!=(Kit.Items[1].WeaponKey==2?150:35))return false;
        if(Phase==15)
        {
            int32 LocalReserve=0;GConfig->GetInt(TEXT("RespawnAndRage.Loadout.smg"),TEXT("Reserve"),LocalReserve,GGameIni);
            if(LocalReserve!=999)return false;
            UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_SERVER_CATALOG local_reserve=%d menu_reserve=%d"),LocalReserve,Kit.Items[1].Reserve);
        }
    }
    if(!HasAuthority())UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_OBSERVED side=%s phase=%d"),HasAuthority()?TEXT("host"):TEXT("client"),Phase);return true;
}
void AERLoadoutProbe::SpawnKit(int32 Index)
{
    auto* Mode=GetWorld()->GetAuthGameMode<AERGameMode>();
    for(auto* PC:{Host.Get(),Client.Get()})
    {
        auto* PS=PC->GetPlayerState<AERPlayerState>();PS->LoadoutPreset=ERLoadouts::Presets()[Index/3];PS->LoadoutVariant=ERLoadouts::Variants()[Index%3];
        if(auto Old=PC->GetPawn()) { PC->UnPossess();Old->Destroy(); }
        Mode->RestartPlayer(PC);
    }
    Phase=Index+1;Deadline=GetWorld()->GetTimeSeconds()+20;ForceNetUpdate();
}
void AERLoadoutProbe::Tick(float Delta)
{
    Super::Tick(Delta);
#if !UE_BUILD_SHIPPING
    const double Now=GetWorld()->GetTimeSeconds();
    const bool Masks=FParse::Param(FCommandLine::Get(),TEXT("ERDrongoMaskGallery"));
    const bool Clothing=Masks || FParse::Param(FCommandLine::Get(),TEXT("ERClothingGallery"));
    if(!HasAuthority())
    {
        if(Phase==30 && MineDeadlines.Num()==3 && Seen!=30)
        {
            TArray<double> Observed;
            for(TActorIterator<AERClaymore> It(GetWorld());It;++It)if(It->State==EERClaymoreState::Armed)Observed.Add(It->ExpiresAt);
            Observed.Sort();
            if(Observed==MineDeadlines) { Seen=30;ServerObserved(30);UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_MINES_REMOTE absolute_deadlines=PASS")); }
        }
        if(Phase>=40 && Phase<=43 && Seen!=Phase)
        {
            auto* PC=Cast<AERPlayerController>(GetOwner());
            auto* PS=PC?PC->GetPlayerState<AERPlayerState>():nullptr;
            auto* C=PC?Cast<AERCharacter>(PC->GetPawn()):nullptr;
            if(!PS || !C)return;
            if(Phase==40)
            {
                OriginalTeam=int32(PS->Team==EERTeam::Omega);OriginalDeaths=PS->Deaths;
                PC->ServerChangeTeam(1-OriginalTeam,false);Seen=Phase;
            }
            else if(Phase==41 && PS->PendingTeam!=EERTeam::None && !C->Health->bDead)
            { PC->ServerChangeTeam(OriginalTeam,false);Seen=Phase; }
            else if(Phase==42 && PS->PendingTeam==EERTeam::None)
            { PC->ServerChangeTeam(1-OriginalTeam,true);Seen=Phase; }
            else if(Phase==43 && !C->Health->bDead && PS->PendingTeam==EERTeam::None && int32(PS->Team==EERTeam::Omega)!=OriginalTeam && PS->Deaths==OriginalDeaths+1)
            { UE_LOG(LogTemp,Display,TEXT("ER_PLAYER_TEAM_REMOTE result=PASS"));ServerObserved(43);Seen=Phase; }
        }
        if(Phase>=1 && Phase<=15 && Seen!=Phase && Observe()) { Seen=Phase;ServerObserved(Phase); }return;
    }
    if(Phase==0)
    {
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(auto* PC=Cast<AERPlayerController>(It->Get())) { if(PC->IsLocalController())Host=PC;else Client=PC; }
        auto* Mode=GetWorld()->GetAuthGameMode<AERGameMode>();
        if(!Host || !Client || !Host->GetPawn() || !Client->GetPawn() || !Mode || !Mode->IsMatchInProgress())return;
        if(FParse::Param(FCommandLine::Get(),TEXT("ERPlayerLayoutCapture")))GalleryStep=3;
        if(Masks)GalleryStep=10;
        Host->ShowMenu(false);Client->ShowMenu(false);SetOwner(Client);SpawnKit(0);return;
    }
    if(Phase>=1 && Phase<=15)
    {
        if(!Observe() || Acknowledged!=Phase)
        {
            if(Now<Deadline)return;Check(false,TEXT("spawn_or_replication_timeout"));FPlatformMisc::RequestExitWithStatus(false,1);return;
        }
        auto* C=CastChecked<AERCharacter>(Host->GetPawn());auto* Gun=C->Weapon.Get();const int32 Ammo=Gun->MagazineAmmo;
        Host->ServerSetLoadout("sniper","defense");
        Check(C==Host->GetPawn() && C->Weapon==Gun && Gun->MagazineAmmo==Ammo,TEXT("confirmation_changed_live_pawn"));
        Host->ServerSetPrimary(0);Host->ServerSetSupplies(20,20,20,20);Host->ServerSetSkins(0,0);
        Check(Host->GetPlayerState<AERPlayerState>()->LoadoutPreset==FName("sniper"),TEXT("legacy_rpc_bypassed_catalog"));
        if(Phase<15) { SpawnKit(Phase);return; }
        Host->ServerSetLoadout("invalid","invalid");
        Check(Host->GetPlayerState<AERPlayerState>()->LoadoutPreset==FName("rifleman") && Host->GetPlayerState<AERPlayerState>()->LoadoutVariant==FName("offense"),TEXT("invalid_fallback"));
        // Real server firing path: one shell, eight traces and one accepted shot.
        SpawnKit(0);Phase=16;
        auto* ShotCharacter=CastChecked<AERCharacter>(Host->GetPawn());
        Host->SetControlRotation(FRotator(80,0,0));
        auto* Shotgun=ShotCharacter->Weapon.Get();const int32 BeforeAmmo=Shotgun->MagazineAmmo;
        const uint32 BeforeShots=Shotgun->AuthorizedShotCount;
        ShotCharacter->ServerStartFire();ShotCharacter->ServerStopFire();
        Check(Shotgun->MagazineAmmo==BeforeAmmo-1 && Shotgun->LastShotTraceCount==8 && Shotgun->AuthorizedShotCount==BeforeShots+1,TEXT("shotgun_runtime8_pellets_one_shell_one_shot"));
        if(FParse::Param(FCommandLine::Get(),TEXT("ERPlayerTeamProbe")))
        {
            auto* PS=Client->GetPlayerState<AERPlayerState>();
            OriginalTeam=int32(PS->Team==EERTeam::Omega);OriginalDeaths=PS->Deaths;
            Phase=40;Deadline=Now+25;ForceNetUpdate();return;
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("ERLoadoutMines")))
        { SpawnKit(14);Phase=30;Deadline=Now+110;return; }
        Phase=20;Deadline=Now+3;
        if(!FParse::Param(FCommandLine::Get(),TEXT("ERLoadoutGallery")))
        { UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);Phase=99; }
        return;
    }
    if(Phase>=40 && Phase<=43)
    {
        if(Now>Deadline) { Check(false,TEXT("player_team_timeout"));FPlatformMisc::RequestExitWithStatus(false,1);Phase=99;return; }
        auto* PS=Client->GetPlayerState<AERPlayerState>();
        auto* C=Cast<AERCharacter>(Client->GetPawn());
        if(Phase==40 && PS->PendingTeam!=EERTeam::None)
        {
            Check(C && !C->Health->bDead && int32(PS->Team==EERTeam::Omega)==OriginalTeam && PS->Deaths==OriginalDeaths,TEXT("deferred_team_changed_live_player"));
            Phase=41;ForceNetUpdate();
        }
        else if(Phase==41 && PS->PendingTeam==EERTeam::None)
        { Check(PS->Deaths==OriginalDeaths,TEXT("cancel_killed_player"));Phase=42;ForceNetUpdate(); }
        else if(Phase==42 && C && C->Health->bDead)
        {
            Check(PS->Deaths==OriginalDeaths+1 && int32(PS->Team==EERTeam::Omega)==OriginalTeam,TEXT("suicide_death_or_team"));
            Client->RespawnAllowedAt=0;GetWorld()->GetAuthGameMode<AERGameMode>()->RequestRespawn(Client);
            Phase=43;ForceNetUpdate();
        }
        else if(Phase==43 && Acknowledged==43)
        {
            UE_LOG(LogTemp,Display,TEXT("ER_PLAYER_TEAM_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));
            UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));
            FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);Phase=99;
        }
        return;
    }
    if(Phase==30)
    {
        if(Now>Deadline) { Check(false,TEXT("mine_fixture_timeout"));FPlatformMisc::RequestExitWithStatus(false,1);Phase=99;return; }
        if(!TickMines())return;
        Check(Acknowledged==30,TEXT("late_relevance_deadlines"));
        if(!FParse::Param(FCommandLine::Get(),TEXT("ERLoadoutGallery")))
        { UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);Phase=99;return; }
        Phase=20;Deadline=Now+3;
    }
    if(Phase==20 && Now>=Deadline)
    {
        // A single live preview scene follows draft selections; gameplay actors never enter it.
        FName Preset=GalleryStep<2?FName("rifleman"):FName("grenadier");
        FName Variant=GalleryStep%2==0?FName("gunner"):FName("defense");
        if(GalleryStep>=4 && !Clothing)
        {
            Preset=ERLoadouts::Presets()[(GalleryStep-4)%5];Variant="defense";
            auto* PS=Host->GetPlayerState<AERPlayerState>();PS->Team=GalleryStep<9?EERTeam::Alpha:EERTeam::Omega;
            PS->LoadoutPreset=Preset;PS->LoadoutVariant=Variant;PS->ForceNetUpdate();
            if(auto Old=Host->GetPawn()) { Host->UnPossess();Old->Destroy(); }
            GetWorld()->GetAuthGameMode<AERGameMode>()->RestartPlayer(Host);
        }
        if(GalleryStep==0 || GalleryStep==4 || GalleryStep==9 || FParse::Param(FCommandLine::Get(),TEXT("ERPlayerLayoutCapture")) || Clothing)Host->ERMenu(6);
        Host->DraftPreset=Preset;Host->DraftVariant=Variant;Host->RefreshLoadoutPreview();
        if(Clothing && Host->MenuPreview)
        {
            const int32 Indices[]={0,2,3,4,0,1,2,3,4,5,6,7,8,9};
            const int32 Case=GalleryStep/2;const bool Omega=Case>=4;
            const auto* C=CastChecked<AERCharacter>(Host->GetPawn());
            const auto& Appearance=Omega ? C->OmegaSkins[Indices[Case]]:C->AlphaSkins[Indices[Case]];
            const auto Kit=ERLoadouts::Resolve("rifleman","gunner",Omega,C);
            Host->MenuPreview->ShowLoadout(Appearance,Kit,C->ReferenceArsenal);
            Host->MenuPreview->Rotate(FMath::FindDeltaAngleDegrees(Host->MenuPreview->GetOrbitYaw(),GalleryStep%2?155.f:-25.f));
            UE_LOG(LogTemp,Display,TEXT("ER_CLOTHING_CAPTURE_SETUP step=%d mesh=%s"),GalleryStep,*GetNameSafe(Appearance.Mesh));
        }
        Check(Host->MenuPreview && Host->MenuPreview->Target && Host->MenuPreview->bPreviewAvailable && Host->MenuPreview->bFramingValid,TEXT("preview_missing_or_unframed"));
        if(Host->MenuPreview)
        {
            Check(Host->MenuPreview->IsWalking() && Host->MenuPreview->Equipment.Num()==1,TEXT("preview_walk_or_primary_only"));
            PreviewYawAtStart=Host->MenuPreview->GetOrbitYaw();
        }
        Phase=21;Deadline=Now+3;return;
    }
    if(Phase==21 && Now>=Deadline)
    {
        Check(Host->MenuPreview && FMath::Abs(FMath::FindDeltaAngleDegrees(PreviewYawAtStart,Host->MenuPreview->GetOrbitYaw()))>5.f,TEXT("preview_camera_not_orbiting"));
        if(GalleryStep>=4 && !Clothing)
        {
            const auto* C=Cast<AERCharacter>(Host->GetPawn());const auto* PS=Host->GetPlayerState<AERPlayerState>();
            if(C && PS)
            {
                const auto K=ERLoadouts::Resolve(Host->DraftPreset,Host->DraftVariant,PS->Team==EERTeam::Omega,C);
                const auto& Appearance=ERSkins::Select(K.bOmega?C->OmegaSkins:C->AlphaSkins,K.bOmega?C->OmegaAppearance:C->AlphaAppearance,K.Skin);
                Check(C->GetMesh()->GetSkeletalMeshAsset()==Appearance.Mesh && C->ActivePreset==K.Preset && PS->ActiveSkin==K.Skin,TEXT("preset_team_skin_spawn_mismatch"));
                UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_SKIN_CAPTURE index=%d team=%s preset=%s mesh=%s"),GalleryStep-4,K.bOmega?TEXT("Omega"):TEXT("Alpha"),*K.Preset.ToString(),*GetNameSafe(Appearance.Mesh));
            }
        }
        const FString Name=FPaths::ProjectSavedDir()/TEXT("Testing/Loadouts")/FString::Printf(TEXT("%s_%d.png"),Clothing?TEXT("Clothing"):TEXT("Loadout"),GalleryStep);
        FScreenshotRequest::RequestScreenshot(Name,true,false);++GalleryStep;Phase=GalleryStep<(Masks?16:Clothing?28:FParse::Param(FCommandLine::Get(),TEXT("ERPlayerLayoutCapture"))?4:14)?20:22;Deadline=Now+2;return;
    }
    if(Phase==22 && Now>=Deadline)
    {
        Host->ShowMenu(false);Check(!Host->MenuPreview,TEXT("preview_keeps_capturing_after_close"));
        UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);Phase=99;
    }
#endif
}

bool AERLoadoutProbe::TickMines()
{
    const double Now=GetWorld()->GetTimeSeconds();auto* PS=Host->GetPlayerState<AERPlayerState>();
    auto* Mode=GetWorld()->GetAuthGameMode<AERGameMode>();
    if(MineStep==0 || MineStep==3 || MineStep==6)
    {
        const int32 Index=MineStep/3;
        if(Index>0 && Now<MineStartedAt+Index*10)return false;
        auto* C=CastChecked<AERCharacter>(Host->GetPawn());
        if(!MinePlacementPending)
        {
            if(!MinePlacementPrepared)
            {
                C->TeleportTo(FVector(-1200,-1500+Index*60,95),FRotator::ZeroRotator);
                MinePlacementPrepared=true;MineWaitUntil=Now+.4;return false;
            }
            if(Now<MineWaitUntil)return false;
            MinePlacementPrepared=false;
            C->Health->ProtectionEndServerTime=Now+10;
            const int32 Slot=C->FindCatalogSlot(10);Check(Slot>=0,TEXT("defense_has_carried_mine"));if(Slot<0)return true;
            C->ServerSelectWeapon(Slot);MineAmmoBefore=C->Weapon->MagazineAmmo;
            C->ServerStartFire();C->ServerStopFire();
            Check(C->Weapon->MagazineAmmo==MineAmmoBefore && !C->Weapon->bPlacingClaymore,TEXT("protected_placement_started_or_consumed"));
            C->Health->ProtectionEndServerTime=0;Host->SetControlRotation(FRotator(0,0,0));
            C->ServerStartFire();C->ServerStopFire();
            Check(C->Weapon->bPlacingClaymore && C->Weapon->MagazineAmmo==MineAmmoBefore,TEXT("placement_must_be_delayed"));
            C->ServerStartFire();Check(C->Weapon->MagazineAmmo==MineAmmoBefore,TEXT("repeated_click_consumed_early"));
            MinePlacementPending=true;MineWaitUntil=Now+2.5;return false;
        }
        if(Now<MineWaitUntil)
        {
            const double Elapsed=C->Weapon->ClaymorePlacementElapsed();
            if(Elapsed>.6 && Elapsed<1.2)Check(C->bIsCrouched && C->Weapon->MagazineAmmo==MineAmmoBefore,TEXT("must_crouch_before_deposit"));
            return false;
        }
        MinePlacementPending=false;
        Check(!C->Weapon->bPlacingClaymore && !C->bIsCrouched,TEXT("placement_did_not_finish_standing"));
        Check(C->Weapon->MagazineAmmo==MineAmmoBefore-1,TEXT("successful_placement_not_consumed_once"));
        AERClaymore* NewMine=nullptr;
        for(TActorIterator<AERClaymore> It(GetWorld());It;++It)if(It->SessionOwner==PS && !Mines.Contains(*It))NewMine=*It;
        Check(NewMine!=nullptr,TEXT("placement_no_world_mine"));if(!NewMine)return true;
        Mines.Add(NewMine);NewMine->SetNetCullDistanceSquared(1);NewMine->ForceNetUpdate();
        if(Index==0)MineStartedAt=Now;
        MineWaitUntil=Now+2.3;++MineStep;return false;
    }
    if(MineStep==1 || MineStep==4 || MineStep==7)
    {
        if(Now<MineWaitUntil)return false;
        const int32 Index=(MineStep-1)/3;
        Check(Mines[Index]->State==EERClaymoreState::Armed,TEXT("mine_did_not_arm"));
        Check(FMath::IsNearlyEqual(Mines[Index]->ExpiresAt-Mines[Index]->ArmAt,60.,.2),TEXT("mine_lifetime_not60"));
        Check(PS->ActiveClaymores==Index+1,TEXT("respawn_replaced_existing_mine"));
        if(Index==0 && !LaserCaptured && FParse::Param(FCommandLine::Get(),TEXT("ERClaymoreLaserCapture")))
        {
            LaserCaptured=true;
            const FVector Aim=Mines[0]->GetActorLocation()+FVector(75,0,0);
            const FVector Eye=Mines[0]->GetActorLocation()+FVector(-140,-280,300);
            auto* Camera=GetWorld()->SpawnActor<ACameraActor>(Eye,(Aim-Eye).Rotation());
            Camera->GetCameraComponent()->SetFieldOfView(60);Host->SetViewTarget(Camera);
            FTimerHandle CaptureTimer,RestoreTimer;
            GetWorldTimerManager().SetTimer(CaptureTimer,[](){ FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Loadouts/ClaymoreLasers.png"),false,false); },.3f,false);
            TWeakObjectPtr<AERPlayerController> PC=Host;TWeakObjectPtr<ACameraActor> View=Camera;
            GetWorldTimerManager().SetTimer(RestoreTimer,[PC,View](){if(PC.IsValid())PC->SetViewTarget(PC->GetPawn());if(View.IsValid())View->Destroy();},.7f,false);
            MineWaitUntil=Now+1;return false;
        }
        if(Index==2)
        {
            for(auto Mine:Mines) { MineDeadlines.Add(Mine->ExpiresAt);Mine->SetNetCullDistanceSquared(1.e10f);Mine->ForceNetUpdate(); }
            MineDeadlines.Sort();ForceNetUpdate();
            Host->ServerSetLoadout("rifleman","offense");
        }
        auto* C=CastChecked<AERCharacter>(Host->GetPawn());C->Health->ProtectionEndServerTime=0;C->Health->ReceiveDamage(1000,nullptr);
        Check(C->Health->bDead,TEXT("intermediate_death_failed"));MineWaitUntil=Now+3.3;++MineStep;return false;
    }
    if(MineStep==2 || MineStep==5 || MineStep==8)
    {
        if(Now<MineWaitUntil)return false;
        Mode->RequestRespawn(Host);const auto* C=Cast<AERCharacter>(Host->GetPawn());
        Check(C && !C->Health->bDead,TEXT("intermediate_respawn_failed"));
        Check(PS->ActiveClaymores==(MineStep+1)/3,TEXT("death_or_respawn_cleared_mines"));
        ++MineStep;return false;
    }
    if(MineStep==9)
    {
        for(int32 I=0;I<3;++I)
        {
            if(Now<MineDeadlines[I]+.2)return false;
            Check(Mines[I]->State==EERClaymoreState::Terminal,TEXT("independent_expiry_failed"));
            if(I<2 && Now<MineDeadlines[I+1]-.1) { Check(Mines[I+1]->State==EERClaymoreState::Armed,TEXT("expiry_removed_other_mine"));Check(PS->ActiveClaymores==2-I,TEXT("active_count_after_expiry")); }
        }
        Check(PS->ActiveClaymores==0,TEXT("active_count_not_zero"));
        UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_MINES staggered_expiry_death_respawn_variant=PASS"));
        const double OriginalDeadline=Deadline;SpawnKit(5);Phase=30;Deadline=OriginalDeadline;MineStep=10;return false;
    }
    if(MineStep==10)
    {
        auto* C=CastChecked<AERCharacter>(Host->GetPawn());
        if(!MinePlacementPending)
        {
            if(!MinePlacementPrepared)
            {
                C->TeleportTo(FVector(-1200,-1500,95),FRotator::ZeroRotator);
                MinePlacementPrepared=true;MineWaitUntil=Now+.4;return false;
            }
            if(Now<MineWaitUntil)return false;
            MinePlacementPrepared=false;C->Health->ProtectionEndServerTime=0;
            C->ServerSelectWeapon(C->FindCatalogSlot(10));Host->SetControlRotation(FRotator::ZeroRotator);
            C->ServerStartFire();C->ServerStopFire();MinePlacementPending=true;MineWaitUntil=Now+2.5;return false;
        }
        if(Now<MineWaitUntil)return false;
        MinePlacementPending=false;
        AERClaymore* NewMine=nullptr;for(TActorIterator<AERClaymore> It(GetWorld());It;++It)if(It->SessionOwner==PS)NewMine=*It;
        Check(NewMine!=nullptr,TEXT("attribution_mine_placement"));if(!NewMine)return true;
        Mines.Add(NewMine);MineWaitUntil=Now+2.3;MineStep=11;return false;
    }
    if(MineStep==11)
    {
        if(Now<MineWaitUntil)return false;
        auto* C=CastChecked<AERCharacter>(Host->GetPawn());C->Health->ProtectionEndServerTime=0;C->Health->ReceiveDamage(1000,nullptr);
        Host->UnPossess();C->Destroy();
        Check(!Host->GetPawn(),TEXT("owner_pawn_not_destroyed_before_mine_trigger"));
        MineKillBaseline=PS->Kills;
        auto* Target=CastChecked<AERCharacter>(Client->GetPawn());Target->Health->ProtectionEndServerTime=0;Target->Health->Health=40;Target->Health->Armor=100;Target->Health->bHelmet=true;
        const FVector SectorEdge=FVector::ForwardVector.RotateAngleAxis(29.9f,FVector::UpVector)*199.f;
        Target->TeleportTo(Mines.Last()->GetActorLocation()+SectorEdge+FVector(0,0,82.5),FRotator::ZeroRotator);
        MineWaitUntil=Now+1;MineStep=12;return false;
    }
    if(MineStep==12)
    {
        if(Now<MineWaitUntil)return false;
        const auto* Target=CastChecked<AERCharacter>(Client->GetPawn());
        Check(Target->Health->bDead && PS->Kills==MineKillBaseline+1,TEXT("wounded_armored_trigger_edge_kill_attribution_after_owner_death"));
        Check(Mines.Last()->State==EERClaymoreState::Terminal && PS->ActiveClaymores==0,TEXT("single_detonation_counter"));
        UE_LOG(LogTemp,Display,TEXT("ER_LOADOUT_MINES posthumous_kill=PASS"));MineWaitUntil=Now+3.3;MineStep=13;return false;
    }
    if(MineStep==13)
    {
        if(Now<MineWaitUntil)return false;Mode->RequestRespawn(Host);Mode->RequestRespawn(Client);
        Host->ServerSetLoadout("rifleman","offense");return true;
    }
    return false;
}
