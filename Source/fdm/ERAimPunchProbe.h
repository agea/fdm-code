#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERAimPunchProbe.generated.h"
UCLASS(NotBlueprintable)
class AERAimPunchProbe : public AActor
{
    GENERATED_BODY()
public:
    AERAimPunchProbe();
    void ConfirmReady(class AERPlayerController* Player,int32 Stage);
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) int32 Phase=0;
    UPROPERTY() TObjectPtr<class AERPlayerController> Host;
    UPROPERTY() TObjectPtr<class AERPlayerController> Client;
    UPROPERTY() TObjectPtr<class AERCharacter> Bot;
    int32 Case=0,OwnerChecks=0,ReadyPhase=-1,LocalPhase=-1,StableFrames=0,SentPhase=-1;
    double ReadyAfter=0;
    uint32 ClientSequence=0;
    double Deadline=0;
    bool Failed=false,Recovered=false;
    void Check(bool Value,const TCHAR* Label);
    void Advance(int32 Next,float Delay);
    float Hit(class AERCharacter* Victim,float HP,bool Head=false);
};
