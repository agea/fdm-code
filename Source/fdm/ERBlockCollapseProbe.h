#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBlockCollapseProbe.generated.h"
UCLASS(NotBlueprintable)
class AERBlockCollapseProbe:public AActor
{
    GENERATED_BODY()
public:
    AERBlockCollapseProbe();
    virtual void Tick(float DeltaSeconds) override;
private:
    int32 Phase=0;float Elapsed=0;bool Failed=false;
    TWeakObjectPtr<class AERBlockRoom> ExtraRoom;
    TArray<double> LargeSteps;
    double LargeElapsed=0;
    int32 PreviousSteps=0;
    bool FrameBudgetPassed=true,SlowFrameObserved=false,FpsRestored=false;
    void Check(bool Value,const TCHAR* Label);
};
