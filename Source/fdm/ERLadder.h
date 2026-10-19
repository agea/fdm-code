#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERLadder.generated.h"

// Climbing geometry is real destructible map cells. Local +X faces the climber.
UCLASS()
class FDM_API AERLadder : public AActor
{
    GENERATED_BODY()
public:
    AERLadder();
    UPROPERTY(EditAnywhere) float Height=400;
    UPROPERTY(EditAnywhere) float DeckHeight=320;
    bool HasRung(float FeetZ) const;
    FVector ClimbPosition(float FeetZ,float HalfHeight) const;
    FVector Landing(float HalfHeight) const;
};
