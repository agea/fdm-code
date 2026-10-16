#include "ERTeamGallery.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "ERCorpse.h"
#include "ERWeapon.h"
#include "ERMatch.h"
#include "ERBotController.h"
#include "Camera/CameraActor.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/SkeletalMeshActor.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "GameFramework/HUD.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "SkeletalRenderPublic.h"
#include "Rendering/SkeletalMeshRenderData.h"

#if !UE_BUILD_SHIPPING
static void LogFirstPersonVertices(AERCharacter* Player)
{
    auto* Body = Player->FirstPersonBody.Get();
    TArray<FFinalSkinVertex> Vertices;
    Body->GetCPUSkinnedVertices(Vertices,0);
    FBox Box(ForceInit);
    const FTransform View = Player->Camera->GetComponentTransform();
    const auto& LOD = Body->GetSkeletalMeshAsset()->GetResourceForRendering()->LODRenderData[0];
    int32 Count = 0;
    for (const auto& Section : LOD.RenderSections)
    {
        if (Body->IsMaterialSectionShown(Section.MaterialIndex,0))
            for (uint32 V=Section.BaseVertexIndex; V<Section.BaseVertexIndex+Section.NumVertices; ++V)
                if (Vertices.IsValidIndex(V))
                {
                    Box += View.InverseTransformPosition(Body->GetComponentTransform().TransformPosition(FVector(Vertices[V].Position)));
                    ++Count;
                }
    }
    UE_LOG(LogTemp,Display,TEXT("ER_FPS_GEOMETRY_POSED mesh=%s vertices=%d min=%s max=%s"),
        *Body->GetSkeletalMeshAsset()->GetName(),Count,*Box.Min.ToString(),*Box.Max.ToString());
}
#endif

void AERTeamGallery::TickForearms(AERCharacter* Player, APlayerController* PC)
{
#if !UE_BUILD_SHIPPING
    auto* State = Player ? Player->GetPlayerState<AERPlayerState>() : nullptr;
    if (!State || !PC || !Player->Weapon || !Player->Weapon->Data ||
        !Player->Weapon->Data->Mesh || !Player->Weapon->Data->IdleAnimation) return;
    if (Stage == 0 && Age > 3.f)
    {
        Player->SetActorLocationAndRotation(FVector(-1400,-1700,92.5), FRotator::ZeroRotator);
        PC->SetControlRotation(FRotator::ZeroRotator);
        PC->SetIgnoreLookInput(true);
        PC->SetIgnoreMoveInput(true);
        const auto* Defaults = Player->GetClass()->GetDefaultObject<AERCharacter>();
        auto* Body = Player->FirstPersonBody.Get();
        Body->SetSkeletalMeshAsset(Defaults->FirstPersonBody->GetSkeletalMeshAsset());
        Body->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
        Body->EmptyOverrideMaterials();
        for (int32 LOD = 0; LOD < Body->GetSkeletalMeshAsset()->GetLODNum(); ++LOD)
            for (int32 Slot = 0; Slot < Body->GetSkeletalMeshAsset()->GetMaterials().Num(); ++Slot)
                Body->ShowMaterialSection(Slot,INDEX_NONE,Player->FirstPersonVisibleMaterialSlots.IsEmpty() || Player->FirstPersonVisibleMaterialSlots.Contains(Slot),LOD);
        for (int32 Slot=0; Slot<Player->AlphaFirstPersonMaterials.Num(); ++Slot)
            Body->SetMaterial(Slot,Player->AlphaFirstPersonMaterials[Slot]);
        Body->HideBoneByName(TEXT("head"),EPhysBodyOp::PBO_None);
        Body->PlayAnimation(Player->Weapon->Data->IdleAnimation,true);
        Stage = 1;
        NextForearmCapture = Age + 5.f;
    }
    // A screenshot is rendered after Tick. Keep its skin until that frame is saved.
    if (bForearmChangePending && Age >= NextForearmCapture - 1.75f)
    {
        const int32 Sample = Stage - 2;
        State->Team = Sample >= ERSkins::Count ? EERTeam::Alpha : EERTeam::Omega;
        State->ActiveSkin = Sample % ERSkins::Count;
        bForearmChangePending = false;
    }
    if (Stage == 0 || Age < NextForearmCapture) return;
    const TArray<FName> Bones = {TEXT("lowerarm_l"),TEXT("hand_l"),TEXT("lowerarm_r"),TEXT("hand_r")};
    const FTransform View = Player->Camera->GetComponentTransform();
    if (Stage == 1)
    {
        LogFirstPersonVertices(Player);
        ForearmReferenceRifle = Player->FirstPersonRifle->GetComponentTransform().GetRelativeTransform(View);
        for (FName Bone : Bones)
            ForearmReferenceBones.Add(Player->FirstPersonBody->GetSocketTransform(Bone).GetRelativeTransform(View));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/FPS-Forearms-Reference.png"),false,false);
        Stage = 2;
        bForearmChangePending = true;
        NextForearmCapture = Age + 2.f;
        return;
    }
    if (Stage >= 2 && Stage <= 2 * ERSkins::Count + 1)
    {
        const int32 Sample = Stage - 2;
        const bool Alpha = Sample >= ERSkins::Count;
        LogFirstPersonVertices(Player);
        bool bSameFrame = ForearmReferenceRifle.Equals(
            Player->FirstPersonRifle->GetComponentTransform().GetRelativeTransform(View),.01f);
        for (int32 I=0; I<Bones.Num(); ++I)
            bSameFrame &= ForearmReferenceBones[I].Equals(
                Player->FirstPersonBody->GetSocketTransform(Bones[I]).GetRelativeTransform(View),.01f);
        UE_LOG(LogTemp,Display,TEXT("ER_FPS_FOREARM_FRAME_%s team=%s skin=%d mesh=%s"),
            bSameFrame ? TEXT("PASS") : TEXT("FAIL"), Alpha ? TEXT("Alpha") : TEXT("Omega"),
            Sample % ERSkins::Count, *Player->FirstPersonBody->GetSkeletalMeshAsset()->GetName());
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(
            TEXT("Testing/FPS-Forearms-%s-%d.png"),Alpha ? TEXT("Alpha") : TEXT("Omega"),Sample % ERSkins::Count + 1),false,false);
        ++Stage;
        bForearmChangePending = Stage <= 2 * ERSkins::Count + 1;
        NextForearmCapture = Age + 2.f;
        return;
    }
    if (Stage == 2 * ERSkins::Count + 2)
    {
        UE_LOG(LogTemp,Display,TEXT("ER_FPS_FOREARM_GALLERY_COMPLETE"));
        FPlatformMisc::RequestExit(false);
        Stage = 2 * ERSkins::Count + 3;
    }
#endif
}

void AERTeamGallery::TickSkins(AERCharacter* Player, APlayerController* PC)
{
#if !UE_BUILD_SHIPPING
    if (!Player || !PC || Player->AlphaSkins.Num() != ERSkins::Count || Player->OmegaSkins.Num() != ERSkins::Count) return;
    if (Stage == 0 && Age > 3)
    {
        Player->SetActorLocation(FVector(-2400,-1600,92.5));
        auto* View = GetWorld()->SpawnActor<ACameraActor>(FVector(-1420,-1400,115), FRotator(0,0,0));
        View->GetCameraComponent()->SetFieldOfView(76); GalleryCamera = View;
        PC->SetViewTarget(View);
        if (PC->GetHUD()) PC->GetHUD()->bShowHUD = false;
        UWidgetLayoutLibrary::RemoveAllWidgets(this);
        Stage = 1;
    }
    int32 Page = 0; FParse::Value(FCommandLine::Get(), TEXT("ERSkinPage="), Page);
    Page = FMath::Clamp(Page, 0, (ERSkins::Count - 1) / 5);
    int32 DetailSkin = INDEX_NONE;
    FParse::Value(FCommandLine::Get(), TEXT("ERSkinDetail="), DetailSkin);
    const int32 TeamIndex = Stage == 1 ? 0 : 1;
    if (SkinActors.IsEmpty() && (Stage == 1 || Stage == 3))
    {
        const auto& Options = TeamIndex == 0 ? Player->AlphaSkins : Player->OmegaSkins;
        for (int32 I=Page*5; I<FMath::Min(Page*5+5,Options.Num()); ++I)
        {
            const auto& Appearance = Options[I];
            const FVector Location(-900,-1400+(I-Page*5-2)*135,92.5f+Appearance.FeetOffset);
            auto* Actor = GetWorld()->SpawnActor<ASkeletalMeshActor>(Location, FRotator(0,90,0));
            auto* Body = Actor->GetSkeletalMeshComponent();
            Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Appearance.Apply(Body, true);
            Actor->SetActorLocationAndRotation(Location, FRotator(0,90,0));
            Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            SkinActors.Add(Actor);
            auto* Label = NewObject<UTextRenderComponent>(Actor);
            Actor->AddInstanceComponent(Label); Label->RegisterComponent();
            Label->SetWorldLocationAndRotation(FVector(-910,Location.Y,207),FRotator(0,180,0));
            Label->SetText(FText::FromString(FString::Printf(TEXT("%d"),I+1)));
            Label->SetHorizontalAlignment(EHTA_Center);Label->SetWorldSize(16);
            Label->SetTextRenderColor(TeamIndex==0 ? FColor(110,180,255) : FColor(255,150,70));
            Label->SetVisibility(DetailSkin == INDEX_NONE);
            UE_LOG(LogTemp,Display,TEXT("ER_SKIN_GALLERY_%s team=%d index=%d name=%s"),Appearance.Matches(Body,true)?TEXT("PASS"):TEXT("FAIL"),TeamIndex+1,I,*Appearance.DisplayName);
        }
    }
    // Optional head close-up for checking hair/helmet intersections in real rendering.
    float ViewYaw = 0.f;
    FParse::Value(FCommandLine::Get(), TEXT("ERSkinDetailYaw="), ViewYaw);
    if (SkinActors.IsValidIndex(DetailSkin - Page*5) && GalleryCamera)
    {
        const auto* Body = SkinActors[DetailSkin - Page*5]->GetSkeletalMeshComponent();
        const FVector Head = Body->GetSocketLocation(TEXT("head")) + FVector(0,0,7);
        const FVector Forward = FRotator(0,ViewYaw,0).Vector();
        GalleryCamera->SetActorLocationAndRotation(Head - Forward*65.f, Forward.Rotation());
        GalleryCamera->GetCameraComponent()->SetFieldOfView(45.f);
    }
    const bool bDetail = SkinActors.IsValidIndex(DetailSkin - Page*5);
    const auto Capture = [&](const TCHAR* Team)
    {
        const FString Name = bDetail
            ? FString::Printf(TEXT("Testing/Head-%s-%d-%d.png"),Team,DetailSkin+1,FMath::RoundToInt(ViewYaw))
            : FString::Printf(TEXT("Testing/Skins-%s-%d.png"),Team,Page);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/Name,false,false);
    };
    if (Stage == 1 && Age > (bDetail ? 10.f : 30.f))
    { Capture(TEXT("Alpha")); Stage = 2; }
    else if (Stage == 2 && Age > (bDetail ? 13.f : 33.f) && SkinActors.Num())
    { for (auto Actor : SkinActors) Actor->Destroy();SkinActors.Reset(); Stage = 3; }
    if (Stage == 3 && Age > (bDetail ? 25.f : 55.f))
    { Capture(TEXT("Omega")); Stage=4; }
    if (Stage == 4 && Age > (bDetail ? 28.f : 58.f))
    { UE_LOG(LogTemp,Display,TEXT("ER_SKIN_GALLERY_COMPLETE"));FPlatformMisc::RequestExit(false); }
#endif
}

void AERTeamGallery::TickProtectionRespawn(AERCharacter* Player, APlayerController* PC)
{
#if !UE_BUILD_SHIPPING
    if (!Player || !PC) return;
    TArray<AERCharacter*> Bots;
    for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It)
        if (It->GetPlayerState<AERPlayerState>() && It->GetPlayerState<AERPlayerState>()->IsABot()) Bots.Add(*It);
    if (Bots.Num()!=4) return;
    if (Stage==0 && Age>5)
    {
        Player->Health->ProtectionEndServerTime=GetWorld()->GetTimeSeconds()+60;
        if (PC->GetHUD()) PC->GetHUD()->bShowHUD=false;
        UWidgetLayoutLibrary::RemoveAllWidgets(this);
        GalleryCamera=GetWorld()->SpawnActor<ACameraActor>();
        GalleryCamera->GetCameraComponent()->SetFieldOfView(60); PC->SetViewTarget(GalleryCamera);
        for (auto* Bot : Bots) { Bot->Health->ProtectionEndServerTime=0; Bot->Health->ReceiveDamage(100,nullptr); }
        Stage=1;
    }
    if (Stage==1)
    {
        for (auto* Bot : Bots) if (Bot->Health->bDead) return;
        ProtectionRespawnSeenAt=Age;
        const FVector Center=Bots[0]->GetActorLocation()+FVector(0,0,20);
        FVector View=Center+FVector(0,0,80);
        // Keep the review camera inside authored rooms rather than behind a wall.
        for (int32 Direction=0;Direction<12;++Direction)
        {
            const FVector Candidate=Center+FRotator(0,Bots[0]->GetActorRotation().Yaw+Direction*30.f,0).Vector()*300+FVector(0,0,30);
            FHitResult Hit;
            if (!GetWorld()->LineTraceSingleByObjectType(Hit,Center,Candidate,FCollisionObjectQueryParams(ECC_WorldStatic)))
            { View=Candidate; break; }
        }
        GalleryCamera->SetActorLocationAndRotation(View,(Center-View).Rotation());
        Stage=2;
    }
    const bool Half=Stage==2 && Age>ProtectionRespawnSeenAt+.5f;
    const bool Normal=Stage==3 && Age>ProtectionRespawnSeenAt+4.5f;
    if (Half || Normal)
    {
        bool Valid=true; int32 Slots=0;
        for (auto* Bot : Bots)
        {
            Valid &= Bot->Health->IsProtected()==Half;
            for (int32 Slot=0;Slot<Bot->GetMesh()->GetNumMaterials();++Slot)
            {
                auto* Current=Bot->GetMesh()->GetMaterial(Slot);
                auto* Original=Bot->GetUnprotectedMaterial(Bot->GetMesh(),Slot);
                Valid &= Current==(Half ? static_cast<UMaterialInterface*>(Bot->ProtectionSurface.Get()) : Original); ++Slots;
            }
            UE_LOG(LogTemp,Display,TEXT("ER_PROTECTION_RESPAWN bot=%s class=%s protected=%d remaining=%.2f materials=%d"),
                *Bot->GetName(),*Bot->GetClass()->GetName(),Bot->Health->IsProtected(),
                Bot->Health->ProtectionEndServerTime-GetWorld()->GetTimeSeconds(),Bot->GetMesh()->GetNumMaterials());
        }
        UE_LOG(LogTemp,Display,TEXT("ER_PROTECTION_RESPAWN_%s gray=%d bots=%d slots=%d"),Valid?TEXT("PASS"):TEXT("FAIL"),Half,Bots.Num(),Slots);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/Protection-Bots-Respawn-%s.png"),Half?TEXT("Gray"):TEXT("Normal")),false,false);
        Stage=Half ? 3 : 4;
    }
    if (Stage==4 && Age>ProtectionRespawnSeenAt+6) FPlatformMisc::RequestExit(false);
#endif
}

void AERTeamGallery::TickProtection(AERCharacter* Player, APlayerController* PC)
{
#if !UE_BUILD_SHIPPING
    if (!Player || !PC || !Player->GetPlayerState<AERPlayerState>()) return;
    auto* State = Player->GetPlayerState<AERPlayerState>();
    if (Stage == 0 && Age > 3)
    {
        Player->TeleportTo(FVector(-1000,-1500,95), FRotator(0,180,0));
        State->Team = EERTeam::Alpha; State->ActiveSkin = 8;
        Player->Health->ProtectionEndServerTime = GetWorld()->GetTimeSeconds()+100;
        GalleryCamera = GetWorld()->SpawnActor<ACameraActor>(FVector(-1350,-1500,135),FRotator(-5,0,0));
        GalleryCamera->GetCameraComponent()->SetFieldOfView(45);
        PC->SetViewTarget(GalleryCamera);
        if (PC->GetHUD()) PC->GetHUD()->bShowHUD = false;
        UWidgetLayoutLibrary::RemoveAllWidgets(this);
        Stage=1;
    }
    Player->GetMesh()->SetOwnerNoSee(false);
    Player->WorldRifle->SetOwnerNoSee(false);
    TInlineComponentArray<UMeshComponent*> Meshes(Player);
    for (auto* Mesh : Meshes)
        if (Mesh->ComponentHasTag(TEXT("ERSkinGear"))) Mesh->SetOwnerNoSee(false);
    const auto Capture = [&](const TCHAR* Name)
    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/Protection-%s.png"),Name),false,false); };
    if (Stage==1 && Age>12) { Capture(TEXT("Alpha-Gray")); Stage=2; }
    else if (Stage==2 && Age>14) { Player->Health->ProtectionEndServerTime=0; Stage=3; }
    else if (Stage==3 && Age>17) { Capture(TEXT("Alpha-Normal")); Stage=4; }
    else if (Stage==4 && Age>19)
    {
        State->Team=EERTeam::Omega; State->ActiveSkin=9;
        Player->Health->ProtectionEndServerTime=GetWorld()->GetTimeSeconds()+100; Stage=5;
    }
    else if (Stage==5 && Age>26) { Capture(TEXT("Omega-Gray")); Stage=6; }
    else if (Stage==6 && Age>28) { Player->Health->ProtectionEndServerTime=0; Stage=7; }
    else if (Stage==7 && Age>31) { Capture(TEXT("Omega-Normal")); Stage=8; }
    else if (Stage==8 && Age>33)
    { UE_LOG(LogTemp,Display,TEXT("ER_PROTECTION_GALLERY_COMPLETE")); FPlatformMisc::RequestExit(false); }
#endif
}

AERTeamGallery::AERTeamGallery() { PrimaryActorTick.bCanEverTick = true; }
void AERTeamGallery::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    Age += DeltaTime;
    auto* PC = GetWorld()->GetFirstPlayerController();
    auto* Player = PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
    if (FParse::Param(FCommandLine::Get(), TEXT("ERProtectionRespawnGallery"))) { TickProtectionRespawn(Player,PC); return; }
    if (FParse::Param(FCommandLine::Get(), TEXT("ERProtectionGallery"))) { TickProtection(Player,PC); return; }
    if (FParse::Param(FCommandLine::Get(), TEXT("ERCharacterShots"))) { TickCharacterShots(Player,PC); return; }
    if (FParse::Param(FCommandLine::Get(), TEXT("ERPostureGallery"))) { TickPosture(Player,PC); return; }
    if (FParse::Param(FCommandLine::Get(), TEXT("ERFPSForearmsGallery"))) { TickForearms(Player,PC); return; }
    if (FParse::Param(FCommandLine::Get(), TEXT("ERSkinGallery"))) { TickSkins(Player,PC); return; }
    const bool bCorpseGallery = FParse::Param(FCommandLine::Get(), TEXT("ERCorpseGallery"));
    if (Stage == 0 && Age > 3 && Player && Player->AlphaAppearance.Mesh && Player->OmegaAppearance.Mesh)
    {
        Stage = 1;
        Player->SetActorLocation(FVector(-2200, -1600, 92.5));
        for (int32 I = 0; I < 2; ++I)
        {
            const auto& Appearance = I == 0 ? Player->AlphaAppearance : Player->OmegaAppearance;
            if (bCorpseGallery)
            {
                FTransform Spawn(FRotator(0, 180, 0), FVector(-900, -1500 + (I == 0 ? -120 : 120), 92.5));
                auto* Corpse = GetWorld()->SpawnActorDeferred<AERCorpse>(Player->CorpseClass, Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
                Corpse->Team = I == 0 ? EERTeam::Alpha : EERTeam::Omega;
                int32 Pair = 0; FParse::Value(FCommandLine::Get(), TEXT("ERSkinPair="), Pair);
                Corpse->SkinIndex = ERSkins::ValidIndex(Pair);
                Corpse->ImpactDirection = FVector(-1, 0, 0);
                Corpse->FinishSpawning(Spawn);
                Corpses.Add(Corpse);
                AddTickPrerequisiteActor(Corpse);
                FString Bones;
                for (const auto& Setup : Corpse->Body->GetPhysicsAsset()->SkeletalBodySetups) Bones += Setup->BoneName.ToString() + TEXT(",");
                UE_LOG(LogTemp, Display, TEXT("ER_CORPSE_RIG team=%d bodies=%s"), I, *Bones);
                continue;
            }
            auto* Actor = GetWorld()->SpawnActor<ASkeletalMeshActor>(FVector(-900, -1500 + (I == 0 ? -140 : 140), 92.5), FRotator(0, 180, 0));
            auto* Body = Actor->GetSkeletalMeshComponent();
            Appearance.Apply(Body, true);
            // ASkeletalMeshActor's body is its root, unlike the character body.
            Actor->SetActorLocation(FVector(-900, -1500 + (I == 0 ? -140 : 140), 92.5f + Appearance.FeetOffset));
            Actor->SetActorRotation(FRotator(0, 90, 0));
            Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            if (!Player->Arsenal.IsEmpty())
            {
                const auto* Data = Player->Arsenal[I == 0 ? 1 : 0].Get();
                auto* Rifle = NewObject<USkeletalMeshComponent>(Actor);
                Actor->AddInstanceComponent(Rifle);
                Rifle->SetSkeletalMeshAsset(Data->Mesh);
                Rifle->RegisterComponent();
                Rifle->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("HandGrip_R"));
                Rifle->SetRelativeTransform(Data->GripTransform);
                Rifle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
        }
        auto* View = GetWorld()->SpawnActor<ACameraActor>(bCorpseGallery ? FVector(-1180, -1500, 260) : FVector(-1330, -1500, 135), bCorpseGallery ? FRotator(-35, 0, 0) : FRotator(-5, 0, 0));
        View->GetCameraComponent()->SetFieldOfView(64);
        GalleryCamera = View;
        PC->SetViewTarget(View);
        if (PC->GetHUD()) PC->GetHUD()->bShowHUD = false;
        UWidgetLayoutLibrary::RemoveAllWidgets(this);
        UE_LOG(LogTemp, Display, TEXT("ER_TEAM_GALLERY_READY"));
    }
    if (bCorpseGallery)
    {
        if (Stage >= 1 && Stage <= 3 && Age > (Stage == 1 ? 5.f : Stage == 2 ? 10.f : 15.f))
        {
            FVector Center = FVector::ZeroVector;
            for (auto Corpse : Corpses) Center += Corpse->Body->GetSocketLocation(TEXT("pelvis"));
            Center /= Corpses.Num();
            const FVector ViewLocation = Center + FVector(-260, 0, 330);
            GalleryCamera->SetActorLocationAndRotation(ViewLocation, (Center - ViewLocation).Rotation());
            for (auto Corpse : Corpses)
            {
                UE_LOG(LogTemp, Display, TEXT("ER_CORPSE_GALLERY_%s team=%d sample=%d"), (Corpse->HasValidRagdollPose() && (Stage == 1 || !Corpse->Body->IsAnyRigidBodyAwake())) ? TEXT("PASS") : TEXT("FAIL"), static_cast<int32>(Corpse->Team), Stage);
                const auto& Appearance = Corpse->GetTeamAppearance();
                for (const FName Bone : Appearance.HiddenBones)
                    UE_LOG(LogTemp, Display, TEXT("ER_CORPSE_HIDDEN team=%d bone=%s scale=%s"), static_cast<int32>(Corpse->Team), *Bone.ToString(), *Corpse->Body->GetBoneTransform(Corpse->Body->GetBoneIndex(Bone)).GetScale3D().ToString());
            }
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Testing/Team-Corpses-%d.png"), Stage), false, false);
            ++Stage;
        }
        if (Stage == 4 && Age > 19)
        {
            UE_LOG(LogTemp, Display, TEXT("ER_CORPSE_GALLERY_COMPLETE"));
            FPlatformMisc::RequestExit(false);
        }
        return;
    }
    if (Stage == 1 && Age > 30)
    {
        Stage = 2;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Testing/Team-Characters.png"), false, false);
    }
    if (Stage == 2 && Age > 34)
    {
        Stage = 3;
        UE_LOG(LogTemp, Display, TEXT("ER_TEAM_GALLERY_COMPLETE"));
        FPlatformMisc::RequestExit(false);
    }
#endif
}

void AERTeamGallery::TickPosture(AERCharacter* Player, APlayerController* PC)
{
#if !UE_BUILD_SHIPPING
    if (!Player || !PC || !Player->Weapon) return;
    auto* M = Cast<UERMovement>(Player->GetCharacterMovement());
    if (Stage == 0 && Age > 3)
    {
        Player->TeleportTo(FVector(-10000,-10000,92.5),FRotator::ZeroRotator);
        Player->GetCharacterMovement()->DisableMovement();
        Player->TeamMarker->SetVisibility(false);
        auto* Floor = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-10000,-10000,-15),FRotator::ZeroRotator);
        Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Floor->SetActorScale3D(FVector(40,40,.3));
        PC->SetControlRotation(FRotator::ZeroRotator);
        PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true);
        M->bProneWanted = true;
        Stage = 1;
    }
    if (Stage == 1 && Age > 8)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Posture-FPS.png"),false,false);
        Stage = 2;
    }
    if (Stage == 2 && Age > 9)
    {
        const FVector Center = Player->GetActorLocation() + FVector(30,0,-5);
        const FVector View = Center + FVector(50,-280,130);
        GalleryCamera = GetWorld()->SpawnActor<ACameraActor>(View,(Center-View).Rotation());
        GalleryCamera->GetCameraComponent()->SetFieldOfView(60);
        PC->SetViewTarget(GalleryCamera);
        Player->GetMesh()->SetOwnerNoSee(false); Player->WorldRifle->SetOwnerNoSee(false);
        Player->FirstPersonBody->SetVisibility(false); Player->FirstPersonRifle->SetVisibility(false);
        if (PC->GetHUD()) PC->GetHUD()->bShowHUD=false;
        UWidgetLayoutLibrary::RemoveAllWidgets(this);
        Stage = 3;
    }
    if (Stage == 3 && Age > 12)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Posture-Alpha.png"),false,false);
        Stage = 4;
    }
    if (Stage == 4 && Age > 13)
    {
        if (auto* State = Player->GetPlayerState<AERPlayerState>()) State->Team = EERTeam::Omega;
        Stage = 5;
    }
    if (Stage == 5 && Age > 16)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Posture-Omega.png"),false,false);
        Stage = 6;
    }
    if (Stage == 6 && Age > 18)
    {
        UE_LOG(LogTemp,Display,TEXT("ER_POSTURE_GALLERY_COMPLETE"));
        FPlatformMisc::RequestExit(false); Stage = 7;
    }
#endif
}

// Twelve looks, each held unchanged while the camera orbits for three views.
void AERTeamGallery::TickCharacterShots(AERCharacter* Player, APlayerController* PC)
{
#if !UE_BUILD_SHIPPING
    auto* State = Player ? Player->GetPlayerState<AERPlayerState>() : nullptr;
    if (!State || !PC || !Player->Weapon || Age < 5 || Age < NextShot) return;
    if (ShotIndex >= 36)
    {
        UE_LOG(LogTemp,Display,TEXT("ER_CHARACTER_SHOTS_COMPLETE count=36"));
        FPlatformMisc::RequestExit(false); return;
    }
    if (!GalleryCamera)
    {
        PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true);
        Player->TeleportTo(FVector(-10000,-10000,92.5),FRotator::ZeroRotator);
        Player->GetCharacterMovement()->DisableMovement();
        Player->TeamMarker->SetVisibility(false);
        auto* Floor = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-10000,-10000,-15),FRotator::ZeroRotator);
        Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Floor->SetActorScale3D(FVector(40,40,.3));
        PC->SetControlRotation(FRotator::ZeroRotator);
        AddTickPrerequisiteActor(Player);
        GalleryCamera = GetWorld()->SpawnActor<ACameraActor>();
        GalleryCamera->GetCameraComponent()->SetFieldOfView(50);
        GalleryCamera->GetCameraComponent()->SetAspectRatio(4.f/3.f);
        for (const FVector Offset : {FVector(250,250,220), FVector(-250,250,180), FVector(0,-300,250)})
        {
            auto* Light = GetWorld()->SpawnActor<APointLight>(Player->GetActorLocation()+Offset,FRotator::ZeroRotator);
            Light->SetMobility(EComponentMobility::Movable);
            Light->PointLightComponent->SetIntensity(35000.f);
            Light->PointLightComponent->SetAttenuationRadius(1800.f);
            Light->PointLightComponent->SetCastShadows(false);
        }
        PC->SetViewTarget(GalleryCamera);
        if (PC->GetHUD()) PC->GetHUD()->bShowHUD=false;
        UWidgetLayoutLibrary::RemoveAllWidgets(this);
    }
    Player->GetMesh()->SetOwnerNoSee(false);
    Player->WorldRifle->SetOwnerNoSee(false);
    Player->FirstPersonBody->SetVisibility(false);
    Player->FirstPersonRifle->SetVisibility(false);
    Player->FirstPersonSight->SetVisibility(false);
    Player->WorldSight->SetOwnerNoSee(false);
    const int32 Look = ShotIndex / 3, View = ShotIndex % 3;
    const int32 Model = Look / 3, Variant = Look % 3;
    const int32 Skin = (Model % 2)*5 + Variant*2;
    const TCHAR* Models[] = {TEXT("Twinblast"),TEXT("Belica"),TEXT("Drongo"),TEXT("Phase")};
    const TCHAR* Views[] = {TEXT("fronte"),TEXT("lato"),TEXT("retro")};
    if (Stage == 0)
    {
        if (View == 0)
        {
            State->Team = Model < 2 ? EERTeam::Alpha : EERTeam::Omega;
            State->ActiveSkin = Skin;
            Player->ServerSelectWeapon(FMath::RandRange(1,7));
        }
        const FVector Center = Player->GetActorLocation();
        const FVector Offset = View == 0 ? FVector(620,0,5) : View == 1 ? FVector(0,620,5) : FVector(-620,0,5);
        GalleryCamera->SetActorLocationAndRotation(Center+Offset,(-Offset).Rotation());
        Stage=1; NextShot=Age+(ShotIndex==0 ? 25.f : 3.f); return;
    }
    const FString File = FString::Printf(TEXT("Screenshots/CharacterSet/%s/variante-%d-%s.png"),Models[Model],Variant+1,Views[View]);
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/File,false,false);
    UE_LOG(LogTemp,Display,TEXT("ER_CHARACTER_SHOT model=%s skin=%d view=%s weapon=%s file=%s"),
        Models[Model],Skin,Views[View],*Player->Weapon->Data->DisplayName,*File);
    ++ShotIndex; Stage=0; NextShot=Age+1.f;
#endif
}
