#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERSessionProbe.generated.h"

// Explicit opt-in integration probe for session settings and laboratory reserve.
UCLASS(NotBlueprintable)
class AERSessionProbe : public AActor
{
    GENERATED_BODY()
public:
    AERSessionProbe();
    virtual void Tick(float DeltaTime) override;
private:
    int32 Phase = 0;
    int32 Reloads = 0;
    double Deadline = 0;
    bool Failed = false;
    TMap<TWeakObjectPtr<class AERBotController>, FVector> Positions;
    void Check(bool Value, const TCHAR* Label);
};
