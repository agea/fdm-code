#include "ERReferenceWeaponProbe.h"
#include "ERShotTrace.h"
#include "Components/BoxComponent.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERWeaponBallistics.h"
#include "ERPlayerController.h"
#include "ERMatch.h"
#include "ERMovement.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
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
{
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;
    // Project anchors only after the character and camera cache describe the same rendered frame.
    if(FParse::Param(FCommandLine::Get(),TEXT("ERScopeTest")))PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AERReferenceWeaponProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AERReferenceWeaponProbe,Phase);}
void AERReferenceWeaponProbe::Check(bool Value,const FString& Label)
{Failed|=!Value;UE_LOG(LogTemp,Display,TEXT("ER_REFERENCE_%s %s"),Value?TEXT("PASS"):TEXT("FAIL"),*Label);}
void AERReferenceWeaponProbe::Advance(int32 Next,float Delay)
{
    Phase=Next;Deadline=GetWorld()->GetTimeSeconds()+Delay;bSlotObserved=false;bReloadIssued=false;
    if(Next>=1 && Next<=10 && Host && Host->GetPawn())CastChecked<AERCharacter>(Host->GetPawn())->ServerSelectWeapon(Next-1);
    ForceNetUpdate();
}
void AERReferenceWeaponProbe::ServerAcknowledgeSlot_Implementation(int32 RequestedPhase)
{
#if !UE_BUILD_SHIPPING
    if(RequestedPhase==Phase && ((RequestedPhase>=100 && RequestedPhase<=125) || (RequestedPhase>=1 && RequestedPhase<=10) || (RequestedPhase>=40 && RequestedPhase<=46)))AcknowledgedPhase=RequestedPhase;
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
    if(FParse::Param(FCommandLine::Get(),TEXT("ERScopeTest"))) { TickScopes(); return; }
    const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::F,EKeys::G};
    const bool Gallery=FParse::Param(FCommandLine::Get(),TEXT("ERReferenceGallery"));
    if(!HasAuthority())
    {
        auto* PC=Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController());
        auto* C=PC?Cast<AERCharacter>(PC->GetPawn()):nullptr;
        if(!C || !C->Weapon || C->Inventory.Num()!=10)return;
        if(Gallery)PC->SetControlRotation(FRotator(0,180,0));
        if(ClientPhase!=Phase)
        {
            ClientPhase=Phase;ClientChangedAt=GetWorld()->GetTimeSeconds();ReloadStartedAt=0;
            if(Gallery)
            {
                PC->ResetIgnoreLookInput();PC->SetIgnoreLookInput(true);
                PC->ResetIgnoreMoveInput();PC->SetIgnoreMoveInput(true);
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
        if(Gallery){Host->ResetIgnoreLookInput();Host->SetIgnoreLookInput(true);Host->ResetIgnoreMoveInput();Host->SetIgnoreMoveInput(true);}
        auto* H=CastChecked<AERCharacter>(Host->GetPawn());auto* C=CastChecked<AERCharacter>(Client->GetPawn());
        Check(H->Inventory.Num()==10 && C->Inventory.Num()==10,TEXT("ten_independent_server_weapons"));
        if(H->Inventory.Num()!=10 || C->Inventory.Num()!=10){FPlatformMisc::RequestExitWithStatus(false,1);return;}
        Check(H->Health->Armor==100 && C->Health->Armor==100,TEXT("armor_initialized_on_server"));
        H->TeleportTo(FVector(Gallery?-850:-1300,-1500,95),FRotator(0,Gallery?45:0,0));C->TeleportTo(FVector(-700,-1500,95),FRotator(0,180,0));
        Host->SetControlRotation(FRotator(0,Gallery?45:0,0));Client->SetControlRotation(FRotator(0,180,0));
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
        if (Gallery && (Phase==2 || Phase==5) && CapturedPhase!=Phase)
        {
            CapturedPhase=Phase;
            const FTransform Camera=H->Camera->GetComponentTransform();
            const FVector TipCamera=Camera.InverseTransformPosition(H->GetVisualMuzzle(true));
            FVector2D Screen;
            Host->ProjectWorldLocationToScreen(H->GetRenderedMuzzle(true),Screen);
            UE_LOG(LogTemp,Display,TEXT("ER_REFERENCE_FRAMING slot=%d tip_camera=%s screen=%s weapon=%s palm=%s"),Phase-1,
                *TipCamera.ToString(),*Screen.ToString(),
                *H->FirstPersonRifle->GetComponentTransform().GetRelativeTransform(Camera).ToString(),
                *H->FirstPersonBody->GetSocketTransform(TEXT("ER_Palm_R"),RTS_Component).ToString());
            if (Phase==2)
            {
            PistolTipCamera=TipCamera;
            const FVector Bore=Camera.InverseTransformVectorNoScale(H->FirstPersonRifle->GetForwardVector());
            const FVector Aim=TipCamera+Bore*((350.-TipCamera.X)/Bore.X);
            Check(Bore.X>.98 && FVector2D(Aim.Y,Aim.Z).Size()<.01,
                TEXT("runtime_pistol_bore_converges_with_center_aim_ray"));
            Check(FMath::IsNearlyEqual(TipCamera.X,56.,.01) &&
                FMath::Abs(TipCamera.Y/TipCamera.X-.164)<.001 &&
                FMath::Abs(TipCamera.Z/TipCamera.X+.1603125)<.001,
                TEXT("runtime_pistol_matches_supplied_reference_framing"));
            Check(FMath::Abs(H->FirstPersonRifle->GetComponentRotation().Roll)<.001,
                TEXT("runtime_pistol_upright_reference_grip"));
#if WITH_EDITOR
            const FTransform Gun=H->FirstPersonRifle->GetComponentTransform();
            const FTransform Palm=H->FirstPersonBody->GetSocketTransform(TEXT("ER_Palm_R"));
            const FVector GripWrist=Gun.InverseTransformPosition(H->FirstPersonBody->GetBoneLocation(TEXT("hand_r")));
            const FVector GripKnuckle=Gun.InverseTransformPosition(H->FirstPersonBody->GetBoneLocation(TEXT("middle_01_r")));
            Check(FMath::Abs(GripWrist.Y)<2. && GripKnuckle.Y>.5 && GripKnuckle.Y<4.,TEXT("runtime_pistol_wrist_behind_grip_and_knuckles_at_flank"));
            const FVector Handle=AERCharacter::GetWeaponSurfaceContact(H->Weapon->Data->Mesh,FVector(-5,100,2),FVector(-5,-100,2),H->Weapon->Data->WeaponIdleAnimation);
            Check(FVector::Distance(Gun.TransformPosition(Handle+FVector(-.8,-3.4,-.7)),Palm.GetLocation()-Palm.GetUnitAxis(EAxis::Z)*.35)<.05,TEXT("runtime_pistol_fitted_grip_anchor"));
            auto ThumbPoints=AERCharacter::GetPresentationVertices(H->FirstPersonBody->GetSkeletalMeshAsset(),TEXT("thumb_03_r"));
            ThumbPoints.Sort([](const FVector& A,const FVector& B){return A.SizeSquared()<B.SizeSquared();});
            FVector ThumbLocal=FVector::ZeroVector;
            const int32 Count=FMath::Max(3,ThumbPoints.Num()/10);
            if(ThumbPoints.Num()>=Count)
            {
                for(int32 I=ThumbPoints.Num()-Count;I<ThumbPoints.Num();++I)ThumbLocal+=ThumbPoints[I]/Count;
                const FVector Contact=AERCharacter::GetWeaponSurfaceContact(H->Weapon->Data->Mesh,FVector(-1.5,-100,3.8),FVector(-1.5,100,3.8),H->Weapon->Data->WeaponIdleAnimation)+FVector(0,-.35,0);
                Check(FVector::Distance(H->FirstPersonBody->GetSocketTransform(TEXT("thumb_03_r")).TransformPosition(ThumbLocal),Gun.TransformPosition(Contact))<.05,TEXT("runtime_pistol_thumb_wraps_grip"));
            }
            else Check(false,TEXT("runtime_pistol_thumb_geometry_available"));
#endif
            const FVector Elbow=H->FirstPersonBody->GetBoneLocation(TEXT("lowerarm_r"),EBoneSpaces::ComponentSpace);
            const FVector Wrist=H->FirstPersonBody->GetBoneLocation(TEXT("hand_r"),EBoneSpaces::ComponentSpace);
            const FVector Forearm=Wrist-Elbow;
            Check(FMath::Abs(Forearm.Z)<Forearm.Size2D(),TEXT("runtime_pistol_relaxed_forearm"));
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/ReferenceWeapons/Pistol_Alpha.png"),true,false);
            Deadline=GetWorld()->GetTimeSeconds()+.25f;
            return; // Render the current slot before Advance equips the next one.
            }
            else Check(PistolTipCamera.X<TipCamera.X,
                TEXT("runtime_pistol_closer_than_assault"));
        }
        if(H->Weapon->Data->bFitSupportHand)
        {
            const FVector Contact=H->WorldRifle->GetComponentTransform().TransformPosition(H->Weapon->Data->SupportHandOffset);
            const FVector RearContact=H->WorldRifle->GetComponentTransform().TransformPosition(H->Weapon->Data->SupportHandOffset-FVector(10,0,0));
            const FVector Palm=H->GetMesh()->GetSocketLocation(H->Weapon->Data->SupportSurfaceContacts.IsEmpty() ? TEXT("HandGrip_L") : TEXT("ER_Palm_L"));
            float Error=FVector::Distance(Palm,FMath::ClosestPointOnSegment(Palm,Contact,RearContact));
            if (!H->Weapon->Data->SupportSurfaceContacts.IsEmpty())
            {
                Error=UE_BIG_NUMBER;
                for (const FVector& Point : H->Weapon->Data->SupportSurfaceContacts)
                    Error=FMath::Min(Error,float(FVector::Distance(Palm,H->WorldRifle->GetComponentTransform().TransformPosition(Point))));
            }
            UE_LOG(LogTemp,Display,TEXT("ER_WORLD_GRIP slot=%d error=%.3f contact=%s palm=%s right=%s gun=%s class=%s"),Phase-1,
                Error,*Contact.ToString(),
                *H->GetMesh()->GetSocketLocation(H->Weapon->Data->SupportSurfaceContacts.IsEmpty() ? TEXT("HandGrip_L") : TEXT("ER_Palm_L")).ToString(),*H->GetMesh()->GetSocketLocation(TEXT("HandGrip_R")).ToString(),
                *H->WorldRifle->GetComponentLocation().ToString(),*H->GetMesh()->GetClass()->GetName());
            Check(Error<1.f,
                FString::Printf(TEXT("world_support_palm_contact_%d"),Phase-1));
        }
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
                Physical|=FMath::IsNearlyEqual(It->Movement->Velocity.Size(),1750.f,.1f) && It->ThrownMesh!=nullptr;
        Check(Physical && H->Weapon->MagazineAmmo==5,TEXT("launcher_uses_model_projectile_and_reduced_launch_speed"));}
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
        H->TeleportTo(FVector(-400,0,5000),FRotator::ZeroRotator);
        C->TeleportTo(FVector(-270,0,5000),FRotator(0,180,0));
        H->GetCharacterMovement()->DisableMovement();C->GetCharacterMovement()->DisableMovement();
        H->ServerSelectWeapon(3);H->Weapon->MagazineAmmo=7;
        H->Health->ProtectionEndServerTime=C->Health->ProtectionEndServerTime=0;
        C->Health->Health=100;C->Health->Armor=100;
        Advance(23,.6f);break;
    case 23:
        {
            auto* Gun=H->Weapon.Get();
            Check(Gun->Data->Pellets==6 && Gun->Data->Damage==20 && Gun->Data->ArmorRatio==1.6f &&
                Gun->Data->RangeModifier==.7f && Gun->Data->bUseBodyHitZones,TEXT("xm1014_saved_data_matches_requested_profile"));
            Host->SetControlRotation((C->GetMesh()->GetSocketLocation(TEXT("spine_03"))-H->Camera->GetComponentLocation()).Rotation());
            Gun->StartFire();Gun->StopFire();
            float Expected=0,Armor=100;int32 BodyHits=0;
            for(const auto& Hit:Gun->LastShotPelletHits)
                if(Hit.GetActor()==C)
                {
                    ++BodyHits;
                    const auto Zone=ERWeaponBallistics::ZoneForBone(Hit.BoneName);
                    const float Damage=ERWeaponBallistics::AtRange(20*ERWeaponBallistics::ZoneMultiplier(Zone),.7f,FVector::Distance(Gun->LastShotTraceStart,Hit.ImpactPoint));
                    Expected+=ERWeaponBallistics::AbsorbArmor(Damage,Zone==ERWeaponBallistics::EHitZone::Legs ? 2.f : 1.6f,Armor);
                    UE_LOG(LogTemp,Display,TEXT("ER_XM_PELLET bone=%s damage=%.3f"),*Hit.BoneName.ToString(),Damage);
                }
            Check(BodyHits==6 && Gun->LastShotTraceCount==6 && Gun->MagazineAmmo==6,TEXT("xm1014_real_six_anatomical_hits_one_shell"));
            Check(Expected<=96.01f && !C->Health->bDead && FMath::IsNearlyEqual(C->Health->Health,100-Expected,.05f) &&
                FMath::IsNearlyEqual(C->Health->Armor,Armor,.05f),TEXT("xm1014_real_armored_chest_survives_six_pellets"));
            // A world collider must stop the refined anatomical query too.
            auto* Cover=GetWorld()->SpawnActor<AActor>();
            auto* Box=NewObject<UBoxComponent>(Cover);Cover->SetRootComponent(Box);
            Box->SetBoxExtent(FVector(5,100,150));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();
            const FVector Bone=C->GetMesh()->GetSocketLocation(TEXT("spine_03"));
            Box->SetWorldLocation((Gun->LastShotTraceStart+Bone)*.5f);
            FCollisionQueryParams Query(SCENE_QUERY_STAT(XMCover),false,H);Query.AddIgnoredActor(Gun);
            FHitResult CoverHit;
            Check(ERShotTrace::Pellet(GetWorld(),CoverHit,Gun->LastShotTraceStart,Bone,Query) && CoverHit.GetActor()==Cover,
                TEXT("xm1014_anatomical_pellets_respect_world_cover"));
            Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);Cover->Destroy();
            // Exercise point-damage zone/helmet and depleted-armor paths independently of aim jitter.
            const FName Bones[]={TEXT("head"),TEXT("spine_01"),TEXT("spine_03"),TEXT("upperarm_l"),TEXT("calf_r")};
            const float Unarmored[]={80,25,20,20,15},Armored[]={64,20,16,16,15};
            for(int32 I=0;I<5;++I)for(int32 Kit=0;Kit<2;++Kit)
            {
                C->Health->Health=100;C->Health->Armor=Kit ? 100 : 0;C->Health->bHelmet=true;
                FHitResult Hit(C,C->GetMesh(),Gun->LastShotTraceStart,FVector::UpVector);Hit.BoneName=Bones[I];
                UGameplayStatics::ApplyPointDamage(C,20,FVector::ForwardVector,Hit,Host,Gun,UDamageType::StaticClass());
                Check(FMath::IsNearlyEqual(C->Health->Health,100-(Kit?Armored[I]:Unarmored[I]),.01f),FString::Printf(TEXT("xm1014_zone_%s_armor_%d"),*Bones[I].ToString(),Kit));
            }
            C->Health->Health=100;C->Health->Armor=100;C->Health->bHelmet=false;
            FHitResult Head(C,C->GetMesh(),Gun->LastShotTraceStart,FVector::UpVector);Head.BoneName=TEXT("head");
            UGameplayStatics::ApplyPointDamage(C,20,FVector::ForwardVector,Head,Host,Gun,UDamageType::StaticClass());
            Check(FMath::IsNearlyEqual(C->Health->Health,20.f,.01f) && C->Health->Armor==100,TEXT("xm1014_kevlar_without_helmet_does_not_protect_head"));
            C->Health->Health=100;C->Health->Armor=1;
            FHitResult Chest=Head;Chest.BoneName=TEXT("spine_03");
            UGameplayStatics::ApplyPointDamage(C,20,FVector::ForwardVector,Chest,Host,Gun,UDamageType::StaticClass());
            Check(FMath::IsNearlyEqual(C->Health->Health,82.f,.01f) && C->Health->Armor==0,TEXT("xm1014_armor_depletion_applies_per_pellet"));
            C->Health->Health=100;C->Health->Armor=0;
            for(int32 I=0;I<3;++I)UGameplayStatics::ApplyPointDamage(C,20,FVector::ForwardVector,Chest,Host,Gun,UDamageType::StaticClass());
            Check(FMath::IsNearlyEqual(C->Health->Health,40.f,.01f),TEXT("xm1014_three_chest_pellets_deal_sixty"));
            for(int32 I=0;I<3;++I)UGameplayStatics::ApplyPointDamage(C,20,FVector::ForwardVector,Chest,Host,Gun,UDamageType::StaticClass());
            Check(C->Health->bDead,TEXT("xm1014_six_unarmored_chest_pellets_kill"));
        }
        UE_LOG(LogTemp,Display,TEXT("ER_REFERENCE_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));
        FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);Advance(99,1000);break;
    }
#endif
}

void AERReferenceWeaponProbe::TickScopes()
{
#if !UE_BUILD_SHIPPING
    const bool Render=FParse::Param(FCommandLine::Get(),TEXT("ERReferenceGallery"));
    const double Now=GetWorld()->GetTimeSeconds();
    if(!HasAuthority())
    {
        auto* PC=Cast<AERPlayerController>(GetWorld()->GetFirstPlayerController());
        auto* C=PC?Cast<AERCharacter>(PC->GetPawn()):nullptr;
        if(!C || !C->Weapon || Phase<100 || Phase>125)return;
        if(ClientPhase!=Phase)
        {
            ClientPhase=Phase;ClientChangedAt=Now;PC->SetControlRotation(FRotator(0,45,0));
            const FKey Keys[]={EKeys::Five,EKeys::RightMouseButton,EKeys::RightMouseButton,
                EKeys::Six,EKeys::RightMouseButton,EKeys::RightMouseButton,EKeys::RightMouseButton,
                EKeys::Seven,EKeys::RightMouseButton,EKeys::RightMouseButton,EKeys::RightMouseButton,
                EKeys::RightMouseButton,EKeys::LeftMouseButton,EKeys::Six,EKeys::RightMouseButton,
                EKeys::LeftMouseButton,EKeys::R,EKeys::One,EKeys::RightMouseButton,
                EKeys::Eight,EKeys::RightMouseButton,EKeys::LeftMouseButton,EKeys::RightMouseButton,
                EKeys::LeftMouseButton,EKeys::One,EKeys::RightMouseButton};
            FInputKeyEventArgs A;A.Key=Keys[Phase-100];A.Event=IE_Pressed;A.AmountDepressed=1;A.EventTimestamp=FPlatformTime::Cycles64();
            PC->InputKey(A);A.Event=IE_Released;A.AmountDepressed=0;PC->InputKey(A);
        }
        const float Blend=C->GetIronAimBlend();
        if(Render && (Phase==101 || Phase==102 || Phase==120 || Phase==122) &&
            TransitionCapturedPhase!=Phase && Blend>.05f && Blend<.95f)
        {
            TransitionCapturedPhase=Phase;
            UE_LOG(LogTemp,Display,TEXT("ER_SCOPE_TRANSITION phase=%d blend=%.4f fov=%.3f"),Phase,Blend,PC->PlayerCameraManager->GetFOVAngle());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/Scopes/Transition_%d.png"),Phase),true,false);
        }
        if(CapturedPhase!=Phase && Now-ClientChangedAt>(Render?1.5:.5) &&
            (!Render || !GShaderCompilingManager || !GShaderCompilingManager->IsCompiling()))
        {
            CapturedPhase=Phase;
            UE_LOG(LogTemp,Display,TEXT("ER_SCOPE_OWNER phase=%d level=%d fov=%.1f spread=%.3f body=%d rifle=%d"),
                Phase,C->Weapon->ScopeLevel,C->Weapon->GetScopeFOV(),C->Weapon->GetSpreadDegrees(),C->FirstPersonBody->IsVisible(),C->FirstPersonRifle->IsVisible());
            if((Phase==101 || Phase==120) && Render)
            {
                FVector2D Screen;
                const auto& View=PC->PlayerCameraManager->GetCameraCacheView();
                const bool Projected=PC->ProjectWorldLocationToScreen(View.TransformWorldToFirstPerson(C->GetIronSightLocation(),false),Screen);
                int32 Width,Height;PC->GetViewportSize(Width,Height);
                const float Drop=C->Weapon->Data->IronSightDrop;
                const float ExpectedY=Height*.5f+Width*.5f*Drop/C->Weapon->Data->IronSightEyeDistance;
                const float Error=FVector2D::Distance(Screen,FVector2D(Width*.5f,ExpectedY));
                UE_LOG(LogTemp,Display,TEXT("ER_SCOPE_IRON_ALIGN result=%s screen=%.3f,%.3f error=%.4f"),
                    Projected && Error<1.f?TEXT("PASS"):TEXT("FAIL"),Screen.X,Screen.Y,Error);
            }
            if(Render)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/Scopes/Scope_%d.png"),Phase),true,false);
            ServerAcknowledgeSlot(Phase);
        }
        return;
    }
    if(Phase==0)
    {
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(auto* PC=Cast<AERPlayerController>(It->Get())){if(PC->IsLocalController())Host=PC;else Client=PC;}
        if(!Host || !Client || !Host->GetPawn() || !Client->GetPawn() || !GetWorld()->GetAuthGameMode<AERGameMode>()->IsMatchInProgress())return;
        if(Render && (Now<15 || (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())))return;
        SetOwner(Client);
        auto* H=CastChecked<AERCharacter>(Host->GetPawn());auto* C=CastChecked<AERCharacter>(Client->GetPawn());
        H->TeleportTo(FVector(-1300,-1500,95),FRotator::ZeroRotator);C->TeleportTo(FVector(-850,-1500,95),FRotator(0,45,0));
        Host->SetControlRotation(FRotator(65,0,0));Client->SetControlRotation(FRotator(0,45,0));
        H->ServerSelectWeapon(4);const float Hip=H->Weapon->GetSpreadDegrees();H->Weapon->ServerCycleScope();
        Check(H->Weapon->ScopeLevel==1 && FMath::IsNearlyEqual(Hip,H->Weapon->GetSpreadDegrees()),TEXT("assault_ads_does_not_improve_accuracy"));
        H->Weapon->ServerCycleScope();Check(H->Weapon->ScopeLevel==0,TEXT("assault_two_click_toggle"));
        H->ServerSelectWeapon(6);H->Weapon->ServerCycleScope();
        Check(H->Weapon->GetScopeSettle()>.95f && H->Weapon->GetSpreadDegrees()>5.f,TEXT("marksman_initial_server_settle_penalty"));
        H->Weapon->StartFire();H->Weapon->StopFire();
        Check(H->Weapon->LastShotSpreadDegrees>5.f && H->Weapon->ScopeLevel==0,TEXT("early_quickscope_uses_real_server_spread_and_exits"));
        // Exercise the shared authoritative focus formula with controlled movement/posture.
        auto* M=CastChecked<UERMovement>(H->GetCharacterMovement());
        M->SetMovementMode(MOVE_Walking);M->Velocity=FVector(139,0,0);M->bWantsToCrouch=false;
        auto* Bolt=H->Weapon.Get();Bolt->ServerCycleScope();Bolt->ScopeChangedServerTime=Now-10;
        Bolt->UpdateScopeMotion();
        Check(Bolt->GetScopeSettle()==1.f && Bolt->GetSpreadDegrees()>5.f,TEXT("standing_marksman_walk_never_finishes_focus"));
        M->Velocity=FVector::ZeroVector;
        Check(Bolt->GetScopeSettle()>.8f,TEXT("standing_stop_requires_real_focus_recovery"));
        M->bWantsToCrouch=true;M->Velocity=FVector(90,0,0);
        Check(Bolt->GetScopeSettle()==0.f && Bolt->GetSpreadDegrees()==0.f,TEXT("crouch_walking_marksman_is_fully_precise"));
        Bolt->ScopeChangedServerTime=Now-.03;
        Check(Bolt->GetScopeSettle()==0.f,TEXT("crouch_scope_focus_finishes_within_thirty_ms"));
        H->ServerSelectWeapon(5);auto* Auto=H->Weapon.Get();Auto->ServerCycleScope();Auto->ScopeChangedServerTime=Now-10;
        M->bWantsToCrouch=false;M->Velocity=FVector::ZeroVector;
        Auto->BloomAtLastShot=1.5f;Auto->LastSpreadShotServerTime=Now-.1;
        const float StandingBurst=Auto->GetBloomDegrees();
        M->bWantsToCrouch=true;
        Check(Auto->GetBloomDegrees()<StandingBurst && Auto->GetScopeRecoilMultiplier()<=.2f,TEXT("crouch_sniper_recovers_spray_and_reduces_vertical_recoil"));
        M->bWantsToCrouch=false;M->Velocity=FVector(139,0,0);Auto->UpdateScopeMotion();
        Check(Auto->GetScopeBlur()>.7f && Auto->GetSpreadDegrees()>3.f,TEXT("walking_auto_sniper_has_large_real_spread_and_blur"));
        Auto->BloomAtLastShot=0;Auto->LastSpreadShotServerTime=Now-10;
        M->Velocity=FVector::ZeroVector;M->bWantsToCrouch=false;Auto->ClearScope();
        C->Health->ProtectionEndServerTime=Now+120;
        Phase=100;Deadline=Now+60;ForceNetUpdate();return;
    }
    if(Phase<100 || Phase>125)return;
    if(AcknowledgedPhase!=Phase && Now<Deadline)return;
    Check(AcknowledgedPhase==Phase,FString::Printf(TEXT("scope_owner_input_and_replication_%d"),Phase));
    auto* C=CastChecked<AERCharacter>(Client->GetPawn());auto* W=C->Weapon.Get();
    const uint8 Expected[]={0,1,0,0,1,2,0,0,1,2,0,1,0,0,1,1,0,0,0,0,1,1,0,0,0,0};
    Check(W->ScopeLevel==Expected[Phase-100],FString::Printf(TEXT("scope_server_level_%d"),Phase));
    if(Phase==103 || Phase==107)Check(W->GetSpreadDegrees()>=3.f,TEXT("optical_hipfire_has_large_spread"));
    if(Phase==104 || Phase==108)
        Check(W->GetSpreadDegrees()<.001f && W->GetScopeFOV()==40.f,TEXT("settled_stationary_scope_is_straight_fov40"));
    if(Phase==105 || Phase==109)Check(W->GetScopeFOV()==10.f,TEXT("second_zoom_fov10"));
    if(Phase==112)Check(W->AuthorizedShotCount>0 && W->ScopeLevel==0,TEXT("marksman_shot_exits_scope"));
    if(Phase==115)Check(W->AuthorizedShotCount>0 && W->ScopeLevel==1,TEXT("sniper_shot_keeps_scope"));
    if(Phase==120)Check(W->Data->Payload==EERPayload::LaunchedHE && W->Data->ScopedLaunchRangeMultiplier==1.5f && FMath::IsNearlyEqual(W->GetSpreadDegrees(),.3f),TEXT("launcher_iron_sight_keeps_accuracy"));
    if(Phase==121 || Phase==123)
    {
        bool Found=false;
        for(TActorIterator<AERProjectile> It(GetWorld());It;++It)if(It->GetOwner()==W && It->Payload==EERPayload::LaunchedHE)
        {
            if(Phase==121)
            {
                FVector Eye;FRotator Aim;C->GetActorEyesViewPoint(Eye,Aim);Eye=C->Camera->GetComponentLocation();
                const FVector Local=C->Camera->GetComponentTransform().InverseTransformPosition(C->GetVisualMuzzle(true));
                FMinimalViewInfo View;View.Location=Eye;View.Rotation=Aim;View.FOV=W->Data->ZoomFOV;
                View.FirstPersonFOV=90.f;View.FirstPersonScale=.2f;
                const FVector ExpectedOrigin=View.TransformWorldToFirstPerson(Eye+Aim.RotateVector(Local),true);
                UE_LOG(LogTemp,Display,TEXT("ER_SCOPE_ORIGIN actual=%s expected=%s delta=%.3f"),*It->ReleasedFrom.ToString(),*ExpectedOrigin.ToString(),FVector::Distance(It->ReleasedFrom,ExpectedOrigin));
                Check(It->ReleasedFrom.Equals(ExpectedOrigin,.5f),TEXT("scoped_launcher_spawns_at_raised_muzzle"));
                Check(!It->ReleasedFrom.Equals(Eye+Aim.RotateVector(FVector(45,12,-12)),1.f),TEXT("scoped_launcher_does_not_use_hip_origin"));
            }
            FRotator LaunchAim=W->LastShotDirection.Rotation();
            LaunchAim.Pitch=FMath::Clamp(LaunchAim.Pitch+6.f,-89.f,89.f);
            const FVector Base=LaunchAim.Vector()*W->Data->ProjectileSpeed;
            Check(Base.Z>W->LastShotDirection.Z*W->Data->ProjectileSpeed,TEXT("launcher_lobs_above_reticle"));
            const float Ratio=Phase==121?1.5f:1.f;
            const bool Match=FMath::IsNearlyEqual(It->ReleasedVelocity.Size2D(),Base.Size2D()*Ratio,.1f) &&
                FMath::IsNearlyEqual(It->ReleasedVelocity.Z,Base.Z,.1f);
            Found|=Match;It->Damage=0; // Keep later presentation phases safe after the fuse.
        }
        Check(Found,Phase==121?TEXT("real_scoped_launcher_has_fifty_percent_more_horizontal_range"):TEXT("real_launcher_exit_restores_ordinary_launch"));
    }
    if(Phase==125)
    {
        UE_LOG(LogTemp,Display,TEXT("ER_REFERENCE_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));
        FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);return;
    }
    ++Phase;Deadline=Now+60;ForceNetUpdate();
#endif
}
