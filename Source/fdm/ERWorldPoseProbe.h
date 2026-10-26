#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERWorldPoseProbe.generated.h"
UCLASS()
class FDM_API AERWorldPoseProbe : public AActor
{
    GENERATED_BODY()
public:
    AERWorldPoseProbe();
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) TObjectPtr<class AERCharacter> Subject;
    UPROPERTY(Replicated) int32 Sample=-1;
    UPROPERTY() TObjectPtr<class ACameraActor> Camera;
    float Age=0,Changed=0,Captured=0,AdvanceAt=0,NetworkReadyAt=0;
    int32 Seen=-1,View=0;
    bool Failed=false,Ready=false;
    bool CheckPose(bool Client);
    void Configure();
    void Capture(int32 Angle);
};
