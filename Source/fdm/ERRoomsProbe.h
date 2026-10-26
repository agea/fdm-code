#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERRoomsProbe.generated.h"

// Explicit development probe, also replicated to initial and late LAN clients.
UCLASS()
class FDM_API AERRoomsProbe : public AActor
{
    GENERATED_BODY()
public:
    AERRoomsProbe();
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    UPROPERTY(Replicated) int32 Phase=0;
    UPROPERTY(Replicated) uint32 ExpectedDigest=0;
    int32 Seen=0,Walk=0,Point=0,Shot=0;
    float Elapsed=0,Stage=0,WalkStarted=0;
    bool Failed=false,Walking=false;
    TArray<FVector> Path;
    void Check(bool Good,const TCHAR* Label);
    void Publish(int32 Next,class AERProceduralMap* Map);
};
