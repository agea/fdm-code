#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERAccuracyProbe.generated.h"

// Opt-in test harness; not created during normal hosting.
UCLASS(NotBlueprintable)
class AERAccuracyProbe : public AActor
{
    GENERATED_BODY()
public:
    AERAccuracyProbe();
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) int32 Phase = 0;
    UPROPERTY() TObjectPtr<class AERPlayerController> Host;
    UPROPERTY() TObjectPtr<class AERPlayerController> Client;
    double Deadline = 0;
    float Standing = 0, Walking = 0, Running = 0;
    bool Failed = false, SawOwnerBase = false, SawOwnerBloom = false, SawOwnerRecovery = false;
    int32 ClientPhase = -1, AmmoBeforeBurst = 0;
    FVector FirstDirection = FVector::ZeroVector;
    int32 ConeSamples = 0;
    bool SamplesWithinCone = true, SamplesVary = false;
    void Advance(int32 Next, float Delay);
    void Check(bool Value, const TCHAR* Label);
};
