#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERReferenceWeaponProbe.generated.h"

UCLASS(NotBlueprintable)
class AERReferenceWeaponProbe : public AActor
{
    GENERATED_BODY()
public:
    AERReferenceWeaponProbe();
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;
    UFUNCTION(Server, Reliable) void ServerAcknowledgeSlot(int32 RequestedPhase);
private:
    UPROPERTY(Replicated) int32 Phase=0;
    UPROPERTY() TObjectPtr<class AERPlayerController> Host;
    UPROPERTY() TObjectPtr<class AERPlayerController> Client;
    int32 ClientPhase=-1, CapturedPhase=-1;
    int32 AcknowledgedPhase=-1;
    int32 CapturedReload=-1;
    double Deadline=0, ClientChangedAt=0;
    double ReloadStartedAt=0;
    bool Failed=false;
    bool bSlotObserved=false;
    bool bReloadIssued=false;
    void Check(bool Value, const FString& Label);
    void Advance(int32 Next,float Delay);
    void BeginCombatChecks();
};
