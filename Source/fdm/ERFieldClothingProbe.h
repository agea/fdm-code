#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERFieldClothingProbe.generated.h"

// Local, opt-in simulation review; never spawned during ordinary matches.
UCLASS(NotBlueprintable)
class AERFieldClothingProbe : public AActor
{
    GENERATED_BODY()
public:
    AERFieldClothingProbe();
    virtual void Tick(float Delta) override;
private:
    UPROPERTY() TObjectPtr<class AERCharacter> Subject;
    UPROPERTY() TObjectPtr<class ACameraActor> Camera;
    UPROPERTY() TObjectPtr<class AERMenuPreview> Preview;
    void TickPreview();
    int32 PreviewFrames=0;
    float PreviewNext=0;
    int32 Sample=-1,Frames=0,Points=0,Deep=0,Missing=0;
    float Age=0,Started=0,Worst=0;
    bool Failed=false,AdvancePending=false;
    void Configure();
    void Measure();
};
