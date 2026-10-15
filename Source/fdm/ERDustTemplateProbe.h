#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERDustTemplateProbe.generated.h"

// Opt-in local validation only; ordinary sessions never create this actor.
UCLASS()
class FDM_API AERDustTemplateProbe : public AActor
{
    GENERATED_BODY()
public:
    AERDustTemplateProbe();
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    int32 Phase=0;
    UPROPERTY(Replicated) int32 SnapshotPhase=0;
    UPROPERTY(Replicated) uint32 ExpectedDigest=0;
    int32 SeenPhase=0;
    float Elapsed=0;
    float StageTime=0;
    int32 GateIndex=0,Waypoint=0;
    bool ReachedUpper=false;
    bool CapturedGate=false;
    bool Failed=false;
    void Check(bool Good,const TCHAR* Label);
    void Publish(int32 Next,class AERProceduralMap* Map);
};
