#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERAudioProbe.generated.h"

UCLASS(NotBlueprintable)
class AERAudioProbe : public AActor
{
    GENERATED_BODY()
public:
    AERAudioProbe();
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    void ConfirmReady(class AERPlayerController* Player, uint32 Token);
private:
    UPROPERTY() TObjectPtr<class AERCharacter> Host;
    UPROPERTY() TObjectPtr<class AERCharacter> Client;
    UPROPERTY(Replicated) TObjectPtr<class AERCharacter> Target;
    UPROPERTY(Replicated) TObjectPtr<class UERWeaponData> ExpectedData;
    UPROPERTY(Replicated) uint32 SelectionToken=0;
    uint32 AckedToken=0, LocalToken=0, SentToken=0;
    double LocalReadyAfter=0;
    int32 StableFrames=0;
    int32 Gun=1, Shot=0, Side=0, Phase=0;
    double Deadline=0;
    uint32 Before=0, FeedbackBefore=0, StepBefore=0;
    bool Failed=false, Ready=false;
    void Check(bool Value,const TCHAR* Label);
};
