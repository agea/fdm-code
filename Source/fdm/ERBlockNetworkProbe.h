#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBlockWorld.h"
#include "ERBlockNetworkProbe.generated.h"
// Explicitly enabled three-peer and dense late-join regression, never normal play.
UCLASS(NotBlueprintable)
class AERBlockNetworkProbe:public AActor
{
    GENERATED_BODY()
public:
    AERBlockNetworkProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void ReceiveReport(class AERPlayerController* PC,int32 Stage,uint32 Epoch,uint32 Sequence,uint32 Digest,bool Passed);
private:
    UPROPERTY(Replicated) TObjectPtr<class AERProceduralMap> Map;
    UPROPERTY(Replicated) int32 Phase=0;
    UPROPERTY(Replicated) uint32 ExpectedEpoch=0;
    UPROPERTY(Replicated) uint32 ExpectedSequence=0;
    UPROPERTY(Replicated) uint32 ExpectedDigest=0;
    UPROPERTY(Replicated) FERBlockId ShotId;
    UPROPERTY(Replicated) FVector FireFrom;
    UPROPERTY(Replicated) FVector WalkEnd;
    UPROPERTY(Replicated) FRotator FireAim;
    UPROPERTY(Replicated) TArray<FVector> StairPath;
    TWeakObjectPtr<class AERPlayerController> Host,Initial,Late,Dense;
    TSet<FString> Reports;
    uint32 ShotsBefore=0;
    double Deadline=0,NextDiagnostic=0;
    bool Failed=false,LateAnnounced=false,NameSent=false;
    int32 ClientPhase=-1,Waypoint=0;
    float PhaseElapsed=0;
    FString Role;
    void Advance(int32 Next,float Seconds=20);
    void Check(bool Value,const TCHAR* Label);
    void ClientTick(float DeltaSeconds);
    bool CollisionParity() const;
    void Place(class AERPlayerController* PC,const FVector& P,const FRotator& Aim);
    bool Reported(const TCHAR* Name,int32 Stage) const;
};
