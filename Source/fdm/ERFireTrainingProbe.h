#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERFireTrainingProbe.generated.h"

// Opt-in local validation only (-ERFireTrainingProbe); ordinary sessions never create it.
UCLASS()
class FDM_API AERFireTrainingProbe : public AActor
{
    GENERATED_BODY()
public:
    AERFireTrainingProbe();
    virtual void Tick(float Delta) override;
private:
    float Elapsed=0,StageTime=0;
    int32 Phase=0,Shot=0,Seed=INDEX_NONE;
    bool Failed=false;
    void Check(bool Good,const TCHAR* Label);
};
