#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERCapacityProbe.generated.h"

UCLASS(Transient)
class AERCapacityProbe : public AActor
{
    GENERATED_BODY()
public:
    AERCapacityProbe();
    virtual void Tick(float DeltaTime) override;
private:
    int32 Phase = 0;
    double Deadline = 0;
    bool Failed = false;
    void Check(bool Value, const TCHAR* Label);
};
