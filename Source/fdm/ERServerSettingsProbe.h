#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERServerSettingsProbe.generated.h"
UCLASS()
class AERServerSettingsProbe : public AActor
{
    GENERATED_BODY()
public:
    AERServerSettingsProbe();
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(Server,Reliable) void Observed(int32 Step);
    UPROPERTY(Replicated) int32 Phase=0;
private:
    int32 Acknowledged=0;
    bool Failed=false,FixtureCreated=false;
    double Started=0,Next=0,NextAudit=0;
    TWeakObjectPtr<class AERPlayerController> Remote;
    TWeakObjectPtr<class AERClaymore> Mine;
    TWeakObjectPtr<class AERSmokeCloud> Cloud;
    void Check(bool Passed,const TCHAR* Label);
    bool Observe();
};
