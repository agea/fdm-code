#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBlockLabProbe.generated.h"

// Opt-in real-game regression probe, never part of normal play.
UCLASS(NotBlueprintable)
class AERBlockLabProbe : public AActor
{
    GENERATED_BODY()
public:
    AERBlockLabProbe();
    virtual void Tick(float DeltaSeconds) override;
private:
    int32 Phase = 0;
    int32 BaselineBlastDestroyed = 0;
    float Elapsed = 0;
    bool Failed = false;
    void Check(bool Value, const TCHAR* Label);
    void TickCombat(class AERCharacter* Character, class AERPlayerController* PC, class AERBlockRoom* Room, float DeltaSeconds);
};
