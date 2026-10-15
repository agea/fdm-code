#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBotMotionProbe.generated.h"

UCLASS()
class AERBotMotionProbe : public AActor
{
    GENERATED_BODY()
public:
    AERBotMotionProbe();
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props)const override;
private:
    UPROPERTY(Replicated) int32 Phase=0;
    float Started=0,FinishAt=0;
    bool Reported=false,Failed=false;
    TMap<int32,uint32> Evidence;
};
