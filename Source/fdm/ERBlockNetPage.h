#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBlockNetPage.generated.h"
USTRUCT()
struct FERBlockPageState
{
    GENERATED_BODY()
    UPROPERTY() uint32 Epoch=0;
    UPROPERTY() int32 Index=0;
    UPROPERTY() uint32 Version=0;
    UPROPERTY() TArray<float> Health;
    UPROPERTY() TArray<uint8> States;
};
// One bounded snapshot per channel, never one actor per block. Independent
// channels keep a fully destroyed late-join snapshot below bunch size limits.
UCLASS(NotBlueprintable)
class FDM_API AERBlockNetPage:public AActor
{
    GENERATED_BODY()
public:
    static constexpr int32 Capacity=128;
    AERBlockNetPage();
    UPROPERTY(ReplicatedUsing=OnRep_State) TObjectPtr<class AERBlockRoom> Room;
    UPROPERTY(ReplicatedUsing=OnRep_State) FERBlockPageState State;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION() void OnRep_State();
};
