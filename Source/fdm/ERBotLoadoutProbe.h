#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBotLoadoutProbe.generated.h"

// Opt-in network fixture for actual initial, respawn and round-reset loadouts.
UCLASS(NotBlueprintable)
class AERBotLoadoutProbe : public AActor
{
    GENERATED_BODY()
public:
    AERBotLoadoutProbe();
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UFUNCTION(Server, Reliable) void ServerObserved(int32 ObservedPhase);
private:
    UPROPERTY(Replicated) int32 Phase = 0;
    int32 Acknowledged = -1, ClientPhase = -1;
    double Deadline = 0;
    bool Failed = false;
    TMap<TWeakObjectPtr<class AERPlayerState>, TWeakObjectPtr<class AERCharacter>> PreviousPawns;
    bool Observe(bool FreshLife);
    void Advance(int32 Next, float Wait);
};
