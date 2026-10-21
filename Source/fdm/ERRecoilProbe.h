#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERRecoilProbe.generated.h"

UCLASS(NotBlueprintable)
class AERRecoilProbe : public AActor
{
    GENERATED_BODY()
public:
    AERRecoilProbe();
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) int32 Phase = 0;
    UPROPERTY() TObjectPtr<class AERPlayerController> Host;
    UPROPERTY() TObjectPtr<class AERPlayerController> Client;
    int32 Case = 0,ClientPhase = -1;
    bool Failed = false,SawOwner = false,SawRecovery = false;
    double Deadline = 0;
    void Check(bool Value,const TCHAR* Label);
    void Advance(int32 Next,float Delay);
};
