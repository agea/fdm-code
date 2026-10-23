#pragma once
#include "CoreMinimal.h"
#include "ERProceduralConfig.generated.h"
USTRUCT(BlueprintType)
struct FERProceduralConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UERMapTemplate> TemplateAsset;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Seed=42;
    // Replicated test witness only. Production always samples the seed (zero).
    UPROPERTY() int32 CrossingOverride=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="6",ClampMax="10")) int32 RoomCount=8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1",ClampMax="3")) int32 Floors=1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool AdditionalMaterials=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool WindowGlass=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MinRoomSize=400;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxRoomSize=800;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CorridorWidth=200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BranchLength=300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 DoorWidth=150;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 DoorHeight=225;
    // Local deterministic retry index (never replicated): every peer repeats
    // the same bounded attempts from the replicated seed.
    int32 LayoutAttempt=0;
};
