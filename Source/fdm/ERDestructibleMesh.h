#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ERDestructibleMesh.generated.h"

// Original LOD0 corners, including UV seams and split shading normals. Positions
// are normalized to the source bounds; runtime fitting uses the existing lots.
USTRUCT()
struct FERMeshCorner
{
    GENERATED_BODY()
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() FVector Normal=FVector::UpVector;
    UPROPERTY() FVector2D UV=FVector2D::ZeroVector;
};
UCLASS(BlueprintType)
class FDM_API UERDestructibleMesh : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMesh> Source;
    UPROPERTY() TArray<FERMeshCorner> Corners;
    UPROPERTY() uint32 GeometryCRC=0;
    UFUNCTION(BlueprintCallable,Category="Template|Maintenance") bool Bake(class UStaticMesh* Mesh);
};
