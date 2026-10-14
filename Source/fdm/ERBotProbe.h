#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBotProbe.generated.h"

// Integration probe; only spawned with -ERBotProbe and never in Shipping.
UCLASS(NotBlueprintable)
class AERBotProbe : public AActor
{
    GENERATED_BODY()
public:
    AERBotProbe();
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) int32 Phase = 0;
    UPROPERTY() TObjectPtr<class AERPlayerController> Host;
    UPROPERTY() TObjectPtr<class AERBotController> Bot;
    TWeakObjectPtr<class AERCharacter> DeadPawn;
    FVector InitialPosition;
    double Deadline = 0;
    int32 AmmoBeforeWall = 0;
    bool bFailed = false, bTargetMode = false;
    bool bReplicaSeen = false, bReplicaDeathSeen = false;
    void Check(bool Value, const TCHAR* Label);
    void Advance(int32 Next, double Delay);
    void PlacePlayers(bool bAcrossWall);
};
