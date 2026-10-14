#include "ERGrenadeExplosion.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"

AERGrenadeExplosion::AERGrenadeExplosion()
{
    PrimaryActorTick.bCanEverTick = true;
    Debris = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Debris"));
    SetRootComponent(Debris);
    Debris->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Debris->SetCastShadow(false);
    Debris->bComponentUseFixedSkelBounds = true;
    Debris->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    Debris->SetCullDistance(4000);
    Flash = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flash"));
    Flash->SetupAttachment(Debris);
    Flash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Flash->SetCastShadow(false);
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
    Light->SetupAttachment(Debris);
    Light->SetCastShadows(false);
    Light->SetLightColor(FLinearColor(1.f, .4f, .08f));
    Light->SetAttenuationRadius(450);
}

bool AERGrenadeExplosion::AssignDebrisMaterials(USkeletalMesh* Mesh, UMaterialInterface* Surface, UMaterialInterface* Side)
{
#if WITH_EDITOR
    if (!Mesh || !Surface || !Side || !Mesh->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Effects/Grenade/"))) return false;
    Mesh->Modify();
    TArray<FSkeletalMaterial> Materials = Mesh->GetMaterials();
    for (auto& Slot : Materials)
        Slot.MaterialInterface = Slot.MaterialSlotName.ToString().Contains(TEXT("Side")) ? Side : Surface;
    Mesh->SetMaterials(Materials);
    Mesh->PostEditChange();
    Mesh->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

void AERGrenadeExplosion::PrecacheEffect(TSubclassOf<AERGrenadeExplosion> EffectClass)
{
    if (!EffectClass || !FApp::CanEverRender()) return;
    static TSet<TWeakObjectPtr<UClass>> Prepared;
    if (Prepared.Contains(EffectClass.Get())) return;
    Prepared.Add(EffectClass.Get());
    // Blueprint component defaults contain the art before the first grenade is
    // fired, so the engine can prepare its rendering pipelines during loadout.
    auto* Defaults = EffectClass->GetDefaultObject<AERGrenadeExplosion>();
    Defaults->Debris->PrecachePSOs();
    Defaults->Flash->PrecachePSOs();
}

void AERGrenadeExplosion::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer || !DebrisMesh || !DebrisAnimation) { Destroy(); return; }
    SetActorScale3D(FVector(FMath::Clamp(BurstScale, .1f, 2.f)));
    Debris->SetSkeletalMeshAsset(DebrisMesh);
    Debris->PlayAnimation(DebrisAnimation, false);
    Debris->SetPlayRate(FMath::Max(.1f, PlayRate));
    Duration = DebrisAnimation->GetPlayLength() / FMath::Max(.1f, PlayRate);
    for (int32 Slot = 0; Slot < Debris->GetNumMaterials(); ++Slot)
        DebrisMaterials.Add(Debris->CreateDynamicMaterialInstance(Slot));
    Flash->SetStaticMesh(FlashMesh);
    Flash->SetMaterial(0, FlashMaterial);
    Flash->SetRelativeScale3D(FVector(.35f));
    Light->SetIntensity(30000);
    SetLifeSpan(Duration + .05f);
    UE_LOG(LogTemp, Display, TEXT("ER_GRENADE_BURST mesh=%s animation=%s duration=%.2f cosmetic_collision=none"),
        *GetNameSafe(DebrisMesh), *GetNameSafe(DebrisAnimation), Duration);
}

void AERGrenadeExplosion::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    if (Elapsed < .12f)
    {
        const float Pulse = FMath::Clamp(1.f - Elapsed / .12f, 0.f, 1.f);
        Flash->SetRelativeScale3D(FVector(.35f + (1.f - Pulse) * .55f));
        Light->SetIntensity(30000 * Pulse * Pulse);
    }
    else if (Flash->IsVisible()) { Flash->SetVisibility(false); Light->SetVisibility(false); }
    const float Fade = FMath::Clamp((Duration - Elapsed) / .4f, 0.f, 1.f);
    for (const auto& Material : DebrisMaterials) if (Material) Material->SetScalarParameterValue(TEXT("DebrisFade"), Fade);
}
