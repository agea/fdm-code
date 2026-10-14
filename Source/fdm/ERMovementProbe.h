#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERMovementProbe.generated.h"

// Explicit opt-in integration probe; absent from normal sessions.
UCLASS()
class FDM_API AERMovementProbe : public AActor
{
    GENERATED_BODY()
public:
    AERMovementProbe();
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;
    UPROPERTY(Replicated) int32 Phase = 0;
private:
    UPROPERTY() TObjectPtr<class AERCharacter> Host;
    UPROPERTY() TObjectPtr<class AERCharacter> Client;
    float Elapsed = 0, Peak = 0, GroundZ = 0;
    bool Failed = false, SawTraversal = false;
    int32 LocalPhase = -1;
    void Check(bool Value, const FString& Label);
    void Advance();
};
