#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERProceduralProbe.generated.h"
UCLASS()
class FDM_API AERProceduralProbe : public AActor
{
    GENERATED_BODY()
public:
    AERProceduralProbe();
    virtual void Tick(float DeltaSeconds) override;
private:
    int32 Phase=0;
    float Elapsed=0;
    bool Failed=false;
    FVector Destination=FVector::ZeroVector;
    void Check(bool Value,const TCHAR* Label);
};
