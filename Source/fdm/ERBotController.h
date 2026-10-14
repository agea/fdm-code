#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "GameFramework/Actor.h"
#include "ERBotController.generated.h"

// Editor-authored route for the current graybox, without a runtime NavMesh dependency.
UCLASS()
class FDM_API AERBotPatrolPoint : public AActor
{
    GENERATED_BODY()
public:
    AERBotPatrolPoint();
    UPROPERTY(EditAnywhere) int32 RouteIndex = 0;
};

UCLASS()
class FDM_API AERBotController : public AAIController
{
    GENERATED_BODY()
public:
    AERBotController();
    bool bTrainingTarget = false;
    UPROPERTY() TObjectPtr<class AERTeamPlayerStart> TrainingStart;
    bool bAwaitingRespawn = false;
    double RespawnAllowedAt = 0;
    virtual void OnPossess(APawn* Pawn) override;
    virtual void Tick(float DeltaTime) override;
    void ResetLife();
private:
    UPROPERTY() TArray<TObjectPtr<AERBotPatrolPoint>> Route;
    TWeakObjectPtr<class AERCharacter> Target;
    int32 RoutePosition = 0, RouteDirection = 1;
    double NextPerceptionAt = 0, NextBurstAt = 0, BurstEndsAt = 0;
    double NextProgressCheckAt = 0;
    FVector LastProgressPosition = FVector::ZeroVector;
    bool HasSight(class AERCharacter* Other) const;
    void Patrol(class AERCharacter* Character, float DeltaTime);
};
