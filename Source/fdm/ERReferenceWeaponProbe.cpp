#include "ERReferenceWeaponProbe.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERWeaponBallistics.h"
#include "ERPlayerController.h"
#include "ERMatch.h"
#include "ERMovement.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ShaderCompiler.h"
#include "HAL/IConsoleManager.h"

AERReferenceWeaponProbe::AERReferenceWeaponProbe()
{ bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true; }
void AERReferenceWeaponProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AERReferenceWeaponProbe,Phase);}
void AERReferenceWeaponProbe::Check(bool Value,const FString& Label)
{Failed|=!Value;UE_LOG(LogTemp,Display,TEXT("ER_REFERENCE_%s %s"),Value?TEXT("PASS"):TEXT("FAIL"),*Label);}
void AERReferenceWeaponProbe::Advance(int32 Next,float Delay)
{Phase=Next;Deadline=GetWorld()->GetTimeSeconds()+Delay;bSlotObserved=false;bReloadIssued=false;ForceNetUpdate();}
void AERReferenceWeaponProbe::ServerAcknowledgeSlot_Implementation(int32 RequestedPhase)
{
#if !UE_BUILD_SHIPPING
    if(RequestedPhase==Phase && ((RequestedPhase>=1 && RequestedPhase<=10) || (RequestedPhase>=40 && RequestedPhase<=46)))AcknowledgedPhase=RequestedPhase;
#endif
}
void AERReferenceWeaponProbe::BeginCombatChecks()
{
    auto* H=CastChecked<AERCharacter>(Host->GetPawn());
    H->ServerSelectWeapon(3);Host->SetControlRotation(FRotator(80,0,0));
    H->Weapon->StartFire();H->Weapon->StopFire();
    Check(H->Weapon->LastShotTraceCount==6 && H->Weapon->MagazineAmmo==6,TEXT("shotgun_six_server_pellets_one_shell"));
    H->ServerSelectWeapon(1);H->Weapon->MagazineAmmo=6;H->Weapon->Reload();
    Advance(20,.3f);
}
void AERReferenceWeaponProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::F,EKeys::G};
    const bool Gallery=FParse::Param(FCommandLine::Get(),TEXT("ERReferenceGallery"));
    if(!HasAuthority())
    {
        auto* PC=Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController());
        auto* C=PC?Cast<AERCharacter>(PC->GetPawn()):nullptr;
        if(!C || !C->Weapon || C->Inventory.Num()!=10)return;
        if(ClientPhase!=Phase)
        {
            ClientPhase=Phase;ClientChangedAt=GetWorld()->GetTimeSeconds();ReloadStartedAt=0;
            if(Gallery)
            {
                if(auto* Variable=IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicRes.OperationMode")))Variable->Set(0,ECVF_SetByCode);
                if(auto* Variable=IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))Variable->Set(100.f,ECVF_SetByCode);
            }
            if((Phase>=1 && Phase<=10) || (Phase>=40 && Phase<=46))
            {
                FInputKeyEventArgs Args;Args.Key=Keys[Phase>=40 ? Phase-39 : Phase-1];Args.Event=IE_Pressed;Args.AmountDepressed=1;Args.EventTimestamp=FPlatformTime::Cycles64();
                PC->InputKey(Args);Args.Event=IE_Released;Args.AmountDepressed=0;PC->InputKey(Args);
            }
        }
        if(Phase>=1 && Phase<=10 && CapturedPhase!=Phase && GetWorld()->GetTimeSeconds()-ClientChangedAt>.35 && C->Weapon==C->Inventory[Phase-1] &&
            (!Gallery || !GShaderCompilingManager || !GShaderCompilingManager->IsCompiling()))
        {
            CapturedPhase=Phase;
            UE_LOG(LogTemp,Display,TEXT("ER_REFERENCE_REMOTE slot=%d mesh=%s armor=%.0f"),Phase-1,*GetNameSafe(C->FirstPersonRifle->GetSkeletalMeshAsset()),C->Health->Armor);
            if(Gallery)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/ReferenceWeapons/Slot_%02d.png"),Phase-1),true,false);
            ServerAcknowledgeSlot(Phase);
        }
        if(Gallery && Phase>=40 && Phase<=46 && CapturedReload!=Phase && C->Weapon==C->Inventory[Phase-39] && C->Weapon->bReloading)
        {
            if(ReloadStartedAt==0)ReloadStartedAt=GetWorld()->GetTimeSeconds();
            if(GetWorld()->GetTimeSeconds()-ReloadStartedAt>C->Weapon->Data->ReloadDuration*.45f && (!GShaderCompilingManager || !GShaderCompilingManager->IsCompiling()))
            {
                CapturedReload=Phase;
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/ReferenceWeapons/Reload_%02d.png"),Phase-39),true,false);
                UE_LOG(LogTemp,Display,TEXT("ER_REFERENCE_REMOTE_RELOAD slot=%d"),Phase-39);
                ServerAcknowledgeSlot(Phase);
            }
        }
        return;
    }
    if(Phase==0)
    {
        if(Gallery && (GetWorld()->GetTimeSeconds()<20 || (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())))return;
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(auto* PC=Cast<AERPlayerController>(It->Get())){if(PC->IsLocalController())Host=PC;else Client=PC;}
        if(!Host || !Client || !Host->GetPawn() || !Client->GetPawn() || !GetWorld()->GetAuthGameMode<AERGameMode>()->IsMatchInProgress())return;
        SetOwner(Client);
        auto* H=CastChecked<AERCharacter>(Host->GetPawn());auto* C=CastChecked<AERCharacter>(Client->GetPawn());
        Check(H->Inventory.Num()==10 && C->Inventory.Num()==10,TEXT("ten_independent_server_weapons"));
        if(H->Inventory.Num()!=10 || C->Inventory.Num()!=10){FPlatformMisc::RequestExitWithStatus(false,1);return;}
        Check(H->Health->Armor==100 && C->Health->Armor==100,TEXT("armor_initialized_on_server"));
        H->TeleportTo(FVector(-1300,-1500,95),FRotator::ZeroRotator);C->TeleportTo(FVector(-700,-1500,95),FRotator(0,180,0));
        Host->SetControlRotation(FRotator::ZeroRotator);Client->SetControlRotation(FRotator(0,180,0));
        for(int32 I=0;I<10;++I)
        {
            auto* Data=H->Inventory[I]->Data.Get();
            Check(Data && Data->Mesh && Data->IdleAnimation,FString::Printf(TEXT("model_and_fitted_skin_pose_%d"),I));
            if(I>=1 && I<=7)Check(Data && Data->WeaponReloadAnimation && Data->ReloadAnimation,FString::Printf(TEXT("authored_weapon_and_retargeted_reload_%d"),I));
        }
        Advance(1,Gallery?2.f:.8f);return;
    }
    if(GetWorld()->GetTimeSeconds()<Deadline && !(Gallery && Phase>=40 && Phase<=46 && bReloadIssued && AcknowledgedPhase==Phase))return;
    auto* H=Cast<AERCharacter>(Host->GetPawn());auto* C=Cast<AERCharacter>(Client->GetPawn());
    if(!H || !C || !H->Weapon || !C->Weapon)return;
    if(Phase>=1 && Phase<=10)
    {
        if(AcknowledgedPhase!=Phase && GetWorld()->GetTimeSeconds()<Deadline+45)return;
        // Material/texture creation can stall the graphical client on its first
        // equip. Keep the phase until its actual owning-client input arrives.
        if(!bSlotObserved)
        {
            if(C->Weapon!=C->Inventory[Phase-1] && GetWorld()->GetTimeSeconds()<Deadline+30)return;
            bSlotObserved=true;Deadline=GetWorld()->GetTimeSeconds()+(Gallery?1.2f:.3f);return;
        }
        Check(C->Weapon==C->Inventory[Phase-1],FString::Printf(TEXT("remote_real_key_selects_slot_%d"),Phase-1));
        if(Phase<10){Advance(Phase+1,Gallery?2.f:.8f);return;}
        C->ServerSelectWeapon(-1);C->ServerSelectWeapon(10);
        Check(C->Weapon==C->Inventory[9],TEXT("invalid_slots_rejected"));
        if(Gallery){Advance(40,1);return;}
        BeginCombatChecks();return;
    }
    if(Gallery && Phase>=40 && Phase<=46)
    {
        if(!bReloadIssued)
        {
            if(C->Weapon!=C->Inventory[Phase-39] && GetWorld()->GetTimeSeconds()<Deadline+30)return;
            C->Weapon->MagazineAmmo=C->Weapon->Data->MagazineSize-1;
            C->Weapon->Reload();bReloadIssued=true;Deadline=GetWorld()->GetTimeSeconds()+C->Weapon->Data->ReloadDuration+5;return;
        }
        if(AcknowledgedPhase!=Phase && GetWorld()->GetTimeSeconds()<Deadline)return;
        Check(AcknowledgedPhase==Phase,FString::Printf(TEXT("remote_authored_reload_captured_%d"),Phase-39));
        C->Weapon->Cancel();
        if(Phase<46)Advance(Phase+1,.5f);else BeginCombatChecks();
        return;
    }
    switch(Phase)
    {
    case 20:
        Check(H->Weapon->bReloading,TEXT("reload_is_authoritative_and_timed"));
        H->ServerSelectWeapon(4);
        Check(!H->Inventory[1]->bReloading && H->Inventory[1]->MagazineAmmo==6,TEXT("holster_cancels_reload_without_refill"));
        H->ServerSelectWeapon(1);H->Weapon->Reload();Advance(21,2.5f);break;
    case 21:
        Check(H->Weapon->MagazineAmmo==7 && H->Weapon->ReserveAmmo==34,TEXT("completed_reload_transfers_reserve"));
        H->ServerSelectWeapon(0);H->Weapon->StartFire();H->Weapon->StopFire();
        Check(H->Weapon->MagazineAmmo==1 && H->Weapon->LastShotTraceCount==1,TEXT("knife_is_short_query_and_consumes_no_ammo"));
        H->ServerSelectWeapon(7);H->Weapon->StartFire();H->Weapon->StopFire();
        {bool Physical=false;for(TActorIterator<AERProjectile> It(GetWorld());It;++It)
            if(It->GetOwner()==H->Weapon && It->Payload==EERPayload::LaunchedHE)
                Physical|=It->Movement->Velocity.Size()>3000 && It->ThrownMesh!=nullptr;
        Check(Physical && H->Weapon->MagazineAmmo==5,TEXT("launcher_uses_model_projectile_and_high_range_physics"));}
        H->ServerSelectWeapon(4);Host->SetControlRotation(FRotator::ZeroRotator);
        C->Health->ProtectionEndServerTime=0;C->Health->Health=100;C->Health->Armor=100;
        H->Health->ProtectionEndServerTime=0;
        // A close torso shot: compare the actual hit position with the source formula.
        H->Weapon->StartFire();H->Weapon->StopFire();
        {float Armor=100;const float Raw=ERWeaponBallistics::AtRange(36,.98f,FVector::Distance(H->Weapon->LastShotTraceStart,H->Weapon->LastShotTraceEnd));
         const float Expected=ERWeaponBallistics::AbsorbArmor(Raw,1.55f,Armor);
         Check(FMath::IsNearlyEqual(C->Health->Health,100-Expected,.05f) && FMath::IsNearlyEqual(C->Health->Armor,Armor,.05f),TEXT("real_ak_hit_uses_source_range_and_armor"));}
        Advance(22,.7f);break;
    case 22:
        Check(!C->Health->bDead,TEXT("armor_does_not_instantly_kill_target"));
        UE_LOG(LogTemp,Display,TEXT("ER_REFERENCE_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));
        FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);Advance(99,1000);break;
    }
#endif
}
