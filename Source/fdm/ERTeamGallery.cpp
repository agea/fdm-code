#include "ERTeamGallery.h"
#include "ERCharacter.h"
#include "ERCorpse.h"
#include "ERWeapon.h"
#include "ERMatch.h"
#include "Camera/CameraActor.h"
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
    if (Stage >= 2 && Stage <= 11)
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
        bForearmChangePending = Stage <= 11;
        NextForearmCapture = Age + 2.f;
        return;
    }
    if (Stage == 12)
    {
        UE_LOG(LogTemp,Display,TEXT("ER_FPS_FOREARM_GALLERY_COMPLETE"));
        FPlatformMisc::RequestExit(false);
        Stage = 13;
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
    const int32 TeamIndex = Stage == 1 ? 0 : 1;
    if (SkinActors.IsEmpty() && (Stage == 1 || Stage == 3))
    {
        const auto& Options = TeamIndex == 0 ? Player->AlphaSkins : Player->OmegaSkins;
        for (int32 I=0; I<Options.Num(); ++I)
        {
            const auto& Appearance = Options[I];
            const FVector Location(-900,-1400+(I-2)*135,92.5f+Appearance.FeetOffset);
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
            UE_LOG(LogTemp,Display,TEXT("ER_SKIN_GALLERY_%s team=%d index=%d name=%s"),Appearance.Matches(Body,true)?TEXT("PASS"):TEXT("FAIL"),TeamIndex+1,I,*Appearance.DisplayName);
        }
    }
    if (Stage == 1 && Age > 30)
    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Skins-Alpha.png"),false,false);Stage = 2; }
    else if (Stage == 2 && Age > 33 && SkinActors.Num())
    { for (auto Actor : SkinActors) Actor->Destroy();SkinActors.Reset(); Stage = 3; }
    if (Stage == 3 && Age > 55)
    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Skins-Omega.png"),false,false); Stage=4; }
    if (Stage == 4 && Age > 58)
    { UE_LOG(LogTemp,Display,TEXT("ER_SKIN_GALLERY_COMPLETE"));FPlatformMisc::RequestExit(false); }
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
                UE_LOG(LogTemp, Display, TEXT("ER_CORPSE_GALLERY_%s team=%d sample=%d"), Corpse->HasValidRagdollPose() ? TEXT("PASS") : TEXT("FAIL"), static_cast<int32>(Corpse->Team), Stage);
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
