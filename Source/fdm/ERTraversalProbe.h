#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERTraversalProbe.generated.h"
UCLASS()
class FDM_API AERTraversalProbe : public AActor
{
    GENERATED_BODY()
public:
    AERTraversalProbe();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;
    UPROPERTY(Replicated) int32 Phase=0;
private:
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Fixture;
    UPROPERTY() TObjectPtr<class AERLadder> Ladder;
    UPROPERTY() TObjectPtr<class AERCharacter> Host;
    UPROPERTY() TObjectPtr<class AERCharacter> Client;
    float Time=0,PeakSpeed=0;
    int32 LocalPhase=-1;
    uint8 EquipmentChecks=0;
    bool SawLadder=false,SawFall=false,Failed=false,Sent=false;
    void Next();
    void Check(bool Value,const TCHAR* Label);
    void Place(class AERCharacter* C,FVector Position);
    void Input(class APlayerController* PC,FKey Key,EInputEvent Event);
};
