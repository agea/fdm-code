#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERReconProbe.generated.h"

// Explicit development flag only. Real owner replication, cover, weapon RPC,
// elapsed 30-second replacement, exhaustion, epoch/round reset and late join.
UCLASS(NotBlueprintable)
class AERReconProbe : public AActor
{
    GENERATED_BODY()
public:
    AERReconProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) int32 Phase=0;
    UPROPERTY() TObjectPtr<class AERPlayerController> Host;
    UPROPERTY() TObjectPtr<class AERPlayerController> Enemy;
    UPROPERTY() TObjectPtr<class AERPlayerController> Ally;
    UPROPERTY() TObjectPtr<class AERReconDirector> Recon;
    UPROPERTY() TObjectPtr<class AActor> Cover;
    UPROPERTY() TObjectPtr<class AActor> GalleryCamera;
    UPROPERTY() TObjectPtr<class AERSmokeCloud> Smoke;
    double Deadline=0, ShotAt=0, ClientFireAt=0;
    int32 ClientPhase=-1;
    bool Failed=false, ClientFired=false;
    bool ClientValidated=false;
    bool CapturedDebris=false;
    TArray<uint8> SavedFog;
    void Check(bool Passed,const TCHAR* Label);
    void Advance(int32 Next,float Delay);
    void ClientStep();
    void ShootDrone();
};
