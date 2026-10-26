#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERLoadoutProbe.generated.h"
UCLASS(NotBlueprintable)
class AERLoadoutProbe : public AActor
{
    GENERATED_BODY()
public:
    AERLoadoutProbe();
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(Server,Reliable) void ServerObserved(int32 Step);
private:
    UPROPERTY(Replicated) int32 Phase=0;
    UPROPERTY() TObjectPtr<class AERPlayerController> Host;
    UPROPERTY() TObjectPtr<class AERPlayerController> Client;
    float PreviewYawAtStart=0;
    int32 OriginalTeam=0, OriginalDeaths=0;
    int32 Acknowledged=0,Seen=0,GalleryStep=0;
    double Deadline=0;
    bool Failed=false;
    bool LaserCaptured=false;
    bool MinePlacementPending=false;
    bool MinePlacementPrepared=false;
    int32 MineAmmoBefore=0;
    void Check(bool Value,const TCHAR* Label);
    bool Observe();
    void SpawnKit(int32 Index);
    bool TickMines();
    int32 MineStep=0, MineKillBaseline=0;
    double MineStartedAt=0, MineWaitUntil=0;
    UPROPERTY() TArray<TObjectPtr<class AERClaymore>> Mines;
    UPROPERTY(Replicated) TArray<double> MineDeadlines;
};
