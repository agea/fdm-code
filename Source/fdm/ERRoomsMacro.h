#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ERRoomsMacro.generated.h"

struct FERRoomsMacroVariant;
class UERRoomsMacroTable;

// Slots name Rooms connection points across seeds. Authoring and runtime share them,
// so the gameplay module never reads PCGEx types.
namespace ERRoomsMacro
{
    constexpr int32 AlphaBase=0,OmegaBase=1,LeftRow=10,RightRow=20,LeftMid=30,RightMid=31,CentreLeft=40,CentreRight=41;
    // Half-turn partner of a slot in a plan with N rooms per lane.
    int32 Mirror(int32 Slot,int32 N);
    // Lane room or junction that a centre slot may join at Y=0, by plan.
    int32 LaneMiddle(bool Left,int32 N);
    bool IsRealizable(FIntPoint Link,int32 N);
    // Centre slots stand this far from the axis, clear of a straight base-to-base line.
    float CentreOffset(float LaneX);
    // Fixed order from the seed, independent of geometry RNG. Empty when the plan has no entry.
    TArray<const FERRoomsMacroVariant*> Candidates(const UERRoomsMacroTable* Table,int32 N,int32 Seed);
}

// Authoring only: runs the PCGEx graph and stores its links as plain data.
UCLASS()
class FDM_API UERRoomsMacroBake : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // PCG data assets have no asset factory: this owns the one input package.
    UFUNCTION(BlueprintCallable, Category="Template|PCG") static class UPCGDataAsset* FindOrCreateInput(const FString& PackageName);
    // Slots of real legacy plans, jittered in half-turn pairs, as one point set per sample.
    UFUNCTION(BlueprintCallable, Category="Template|PCG") static bool FillInput(class UERMapTemplate* Template,class UPCGDataAsset* Asset,int32 SamplesPerPlan=8);
    // Returns the variants written to Table, or -1 when the graph fails.
    UFUNCTION(BlueprintCallable, Category="Template|PCG") static int32 Bake(class UPCGGraphInterface* Graph,UERRoomsMacroTable* Table);
};
