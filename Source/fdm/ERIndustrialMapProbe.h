#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERIndustrialMapProbe.generated.h"
UCLASS()
class AERIndustrialMapProbe : public AActor
{
    GENERATED_BODY()
public:
    AERIndustrialMapProbe();
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    UPROPERTY(Replicated) int32 Phase=0;
    UPROPERTY(Replicated) uint32 ExpectedDigest=0;
    float Elapsed=0,WalkStarted=0,WalkPeakZ=0,DamageTime=20;
    float NextDiagnostic=0;
    int32 SeenPhase=0,Flight=0,Gallery=0,LadderIndex=0;
    bool Failed=false,Walking=false,LadderClimbing=false,SawLadder=false;
    void Check(bool Value,const TCHAR* Label);
    void Publish(int32 Next,class AERProceduralMap* Map);
    void Capture(class AERProceduralMap* Map,int32 View=0);
};
