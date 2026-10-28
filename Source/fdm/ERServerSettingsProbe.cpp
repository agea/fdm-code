#include "ERServerSettingsProbe.h"
#include "ERPlayerController.h"
#include "ERMatch.h"
#include "ERMovement.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERClaymore.h"
#include "ERSmokeCloud.h"
#include "ERBlockRoom.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"

AERServerSettingsProbe::AERServerSettingsProbe()
{ bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.1f; }
void AERServerSettingsProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AERServerSettingsProbe,Phase);}
void AERServerSettingsProbe::Observed_Implementation(int32 Step){if(Step==Phase)Acknowledged=Step;}
void AERServerSettingsProbe::Check(bool Passed,const TCHAR* Label)
{if(!Passed){Failed=true;UE_LOG(LogTemp,Error,TEXT("ER_SERVER_RULES_FAIL phase=%d %s"),Phase,Label);}}
bool AERServerSettingsProbe::Observe()
{
    auto Wait=[this](const TCHAR* Reason){const double Now=GetWorld()->GetTimeSeconds();if(Now>=NextAudit){UE_LOG(LogTemp,Display,TEXT("ER_SERVER_RULES_WAIT side=%s phase=%d reason=%s"),HasAuthority()?TEXT("host"):TEXT("client"),Phase,Reason);NextAudit=Now+5;}return false;};
    const auto* GS=GetWorld()->GetGameState<AERGameState>();if(!GS || !GS->ServerRules.Ready)return Wait(TEXT("rules_ready"));
    if(GS->ServerRules.Get("Move.SprintDuration")!=7 || GS->ServerRules.Get("Move.Run")!=21)return Wait(TEXT("server_values"));
    int32 Pawns=0;
    for(TActorIterator<AERCharacter> It(GetWorld());It;++It)
    {
        auto* C=*It;if(!C->GetPlayerState<AERPlayerState>() || C->Health->bDead)continue;
        const auto* M=Cast<UERMovement>(C->GetCharacterMovement());
        if(!FMath::IsNearlyEqual(M->MaxWalkSpeed,21.f/.036f,.1f) || C->Health->Health!=150 || C->Inventory.Num()!=5){if(GetWorld()->GetTimeSeconds()>=NextAudit)UE_LOG(LogTemp,Display,TEXT("ER_SERVER_RULES_PAWN hp=%.1f speed=%.2f slots=%d"),C->Health->Health,M->MaxWalkSpeed,C->Inventory.Num());return Wait(TEXT("pawn_health_movement_inventory"));}
        const auto* W=C->Inventory[0].Get();if(!W || !W->Data || W->Tuned()->Damage!=37 || ((HasAuthority() || C->IsLocallyControlled()) && (W->MagazineAmmo!=20 || W->ReserveAmmo!=66))){if(W && W->Data && GetWorld()->GetTimeSeconds()>=NextAudit)UE_LOG(LogTemp,Display,TEXT("ER_SERVER_RULES_WEAPON damage=%.1f loaded=%d reserve=%d"),W->Tuned()->Damage,W->MagazineAmmo,W->ReserveAmmo);return Wait(TEXT("effective_weapon_and_ammo"));}
        if(M->SprintDuration()!=7 || M->RecoveryDuration()!=11)return Wait(TEXT("stamina"));
        ++Pawns;
    }
    if(Pawns!=2)return Wait(TEXT("two_pawns"));
    const auto* Kit=GS->LoadoutCatalog.FindByPredicate([](const auto& K){return K.Preset==FName("rifleman") && K.Variant==FName("offense") && !K.bOmega;});
    if(!Kit || Kit->Items[0].Loaded!=20 || Kit->Items[0].Reserve!=66 || Kit->Items[3].Loaded!=5)return Wait(TEXT("server_kit_catalog"));
    bool Smoke=false,ArmedMine=false,Material=false;
    for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It)Smoke|=It->Duration==18 && FMath::IsNearlyEqual(It->SimSize.X,1200.f,.01f);
    for(TActorIterator<AERClaymore> It(GetWorld());It;++It)ArmedMine|=It->State==EERClaymoreState::Armed && FMath::IsNearlyEqual(It->DetectionRange,300.f,.01f) && It->DetectionHalfAngle==20;
    for(TActorIterator<AERBlockRoom> It(GetWorld());It;++It)Material|=It->IsMapReady() && It->MaterialRules.Get("Structure.Health")==1.5f;
    if(!Smoke)return Wait(TEXT("smoke"));if(!ArmedMine)return Wait(TEXT("armed_mine"));if(!Material)return Wait(TEXT("material_snapshot"));
    return true;
}
void AERServerSettingsProbe::Tick(float Delta)
{
    Super::Tick(Delta);
    const double Now=GetWorld()->GetTimeSeconds();if(!Started)Started=Now;
    const bool Gallery=FParse::Param(FCommandLine::Get(),TEXT("ERServerRulesGallery"));
    if(Gallery)
    {
        if(Phase==0 && Now-Started>2)
        {
            if(auto* PC=Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController()))
            {PC->ERMenu(10);Phase=1;Next=Now+2;}
        }
        else if(Phase==1 && Now>=Next)
        {FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/ServerRules-Menu.png"),true,false);Phase=2;Next=Now+2;}
        else if(Phase==2 && Now>=Next)
        {if(auto* PC=Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController())){PC->ServerRulesSection=TEXT("Claymore");PC->ERMenu(10);Phase=3;Next=Now+2;}}
        else if(Phase==3 && Now>=Next)
        {FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/ServerRules-Claymore.png"),true,false);Phase=4;Next=Now+2;}
        else if(Phase==4 && Now>=Next)
        {if(auto* PC=Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController())){PC->ServerRulesSection=TEXT("Weapon - Assault Rifle");PC->bServerRulesAdvanced=true;PC->ERMenu(10);Phase=5;Next=Now+2;}}
        else if(Phase==5 && Now>=Next)
        {FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/ServerRules-Weapon.png"),true,false);Phase=6;Next=Now+2;}
        else if(Phase==6 && Now>=Next)FPlatformMisc::RequestExitWithStatus(false,0);
        return;
    }
    if(Phase==99)return;
    if(!HasAuthority())
    {
        if(auto* PC=Cast<AERPlayerController>(GetOwner());PC && PC->IsLocalController() && Phase>0 && Acknowledged!=Phase)
        {
            if(Phase==1 && Observe())
            {UE_LOG(LogTemp,Display,TEXT("ER_SERVER_RULES_REMOTE phase=1 movement_weapons_kits_smoke_claymore_materials=PASS"));Observed(1);Acknowledged=1;}
            else if(Phase==2)
            {
                if(const auto* C=Cast<AERCharacter>(PC->GetPawn());C && C->Health->Health>0 && C->Health->Health<150)
                {UE_LOG(LogTemp,Display,TEXT("ER_SERVER_RULES_REMOTE phase=2 low_damage_mine_survival=PASS"));Observed(2);Acknowledged=2;}
            }
        }
        return;
    }
    if(Now-Started>70)
    {Check(false,TEXT("timeout"));Phase=99;FPlatformMisc::RequestExitWithStatus(false,1);return;}
    if(Phase==0)
    {
        if(!FixtureCreated){GetWorld()->SpawnActor<AERBlockRoom>(FVector(0,6000,0),FRotator::ZeroRotator);FixtureCreated=true;}
        AERPlayerController* Host=nullptr;
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(auto* PC=Cast<AERPlayerController>(It->Get());PC && PC->GetPawn())
            {if(PC->IsLocalController())Host=PC;else Remote=PC;}
        if(!Host || !Remote.IsValid())return;
        auto* H=Cast<AERCharacter>(Host->GetPawn());auto* T=Cast<AERCharacter>(Remote->GetPawn());
        if(!H || !T || !H->Weapon || !H->Weapon->ProjectileClass)return;
        H->Health->ProtectionEndServerTime=0;T->Health->ProtectionEndServerTime=0;
        H->TeleportTo(FVector(-1200,-1500,95),FRotator::ZeroRotator);Host->SetControlRotation(FRotator::ZeroRotator);
        T->TeleportTo(FVector(1200,-1500,95),FRotator(0,180,0));
        SetOwner(Remote.Get());
        if(!AERClaymore::Place(H)){Check(false,TEXT("placement"));return;}
        for(TActorIterator<AERClaymore> It(GetWorld());It;++It)Mine=*It;
        Cloud=GetWorld()->SpawnActor<AERSmokeCloud>(H->Weapon->ProjectileClass->GetDefaultObject<AERProjectile>()->SmokeCloudClass,FVector(0,1500,20),FRotator::ZeroRotator);
        Phase=1;Next=Now+3;ForceNetUpdate();return;
    }
    if(Phase==1 && Now>=Next && Acknowledged==1)
    {
        Check(Observe(),TEXT("host_tuning_snapshot"));
        auto* Target=Cast<AERCharacter>(Remote->GetPawn());
        Check(Target && Mine.IsValid(),TEXT("mine_and_target"));if(!Target || !Mine.IsValid())return;
        Target->TeleportTo(Mine->GetActorLocation()+FVector(250,0,82.5),FRotator(0,180,0));
        Phase=2;Next=Now+1;ForceNetUpdate();return;
    }
    if(Phase==2 && Now>=Next && Acknowledged==2)
    {
        auto* T=Cast<AERCharacter>(Remote->GetPawn());
        Check(T && !T->Health->bDead && T->Health->Health>=130 && T->Health->Health<150,TEXT("damage_is_radial_without_lethal_override"));
        Check(!Mine.IsValid() || Mine->State==EERClaymoreState::Terminal,TEXT("one_detonation"));
        UE_LOG(LogTemp,Display,TEXT("ER_SERVER_RULES_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));
        Phase=99;ForceNetUpdate();FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);
    }
}
