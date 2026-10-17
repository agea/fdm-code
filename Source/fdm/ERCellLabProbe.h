#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERCellLabProbe.generated.h"

// Opt-in real-game probe of the 5cm cell prototype in L_DestructionLab
// (-ERCellLab -ERCellLabProbe), never part of normal play.
UCLASS(NotBlueprintable)
class AERCellLabProbe : public AActor
{
    GENERATED_BODY()
public:
    AERCellLabProbe();
    virtual void Tick(float DeltaSeconds) override;
private:
    int32 Phase = 0, Grenades = 0, ResumePhase = 0;
    float Elapsed = 0, TargetBefore = 0;
    bool Failed = false;
    void Check(bool Value, const TCHAR* Label);
    // Rendered runs only: frame the view, capture at the end of this frame, resume later.
    void Capture(class AERCharacter* Character, class AERPlayerController* PC, const TCHAR* Name, FVector At, float Yaw, int32 Next);
    void Metrics(const TCHAR* Label, const struct FERBlockBatchResult& Result, class AERBlockRoom* Room);
};
