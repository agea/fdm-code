#pragma once
#include "CoreMinimal.h"
#include "ERBlockWorld.h"
#include "ERBlockCombat.generated.h"

USTRUCT(BlueprintType)
struct FERBlockShotSettings
{
    GENERATED_BODY()
    // Gameplay energy units, deliberately independent of real joules.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float Energy = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="16")) int32 MaxPenetrations = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=".01", ClampMax="1")) float MinImpactCosine = .15f;
};
struct FERBlockDamageRequest
{
    FERBlockId Id;
    float Damage = 0;
    bool bDestroy = false;
};
struct FERBlockBatchResult
{
    double TotalMs=0, QueryMs=0, ExposureMs=0, MutationMs=0;
    int32 CoverTraces=0;
    int32 Candidates = 0, Occluded = 0, Damaged = 0, Destroyed = 0, GroupsRebuilt = 0;
};
struct FERBlockShotResult
{
    double TotalMs=0;
    FVector End = FVector::ZeroVector;
    FHitResult TerminalHit;
    float RemainingEnergy = 0;
    float TerminalDamage = 0;
    int32 BlockHits = 0, Penetrations = 0, ExtraTraces = 0;
    bool bLimited = false;
};
namespace ERBlockCombat
{
    // Centred 100cm cube transformed into an oriented block volume. Distances
    // remain world centimetres, including rotated/scaled room actors.
    bool IntersectBlock(const FTransform& Transform, const FVector& Start, const FVector& UnitDirection,
        double MaxDistance, double& EntryDistance, double& ExitDistance);
    FERBlockShotResult ContinueShot(UWorld* World, const FHitResult& InitialHit, const FVector& Start,
        const FVector& Direction, float Range, float Damage, const FERBlockShotSettings& Settings,
        const FCollisionQueryParams& Query, AController* Instigator, AActor* Causer);
}
