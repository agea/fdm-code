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
    UPROPERTY(EditAnywhere, meta=(ClampMin="5", ClampMax="90")) float AcceptanceRadius = 90;
};

UCLASS()
class FDM_API AERBotController : public AAIController
{
    GENERATED_BODY()
public:
    AERBotController();
    bool bMovementOnly = false;
    float MovementPhaseOffset=0;
    bool bTrainingTarget = false;
    int32 SpawnWeaponSlot = INDEX_NONE;
    UPROPERTY() TObjectPtr<class AERTeamPlayerStart> TrainingStart;
    bool bAwaitingRespawn = false;
    double RespawnAllowedAt = 0;
    virtual void OnPossess(APawn* PossessedPawn) override;
    virtual void Tick(float DeltaTime) override;
    void ResetLife();
private:
    UPROPERTY() TArray<TObjectPtr<AERBotPatrolPoint>> Route;
    TWeakObjectPtr<class AERCharacter> Target;
    int32 RoutePosition = 0, RouteDirection = 1;
    double NextPerceptionAt = 0, NextBurstAt = 0, BurstEndsAt = 0;
    double NextProgressCheckAt = 0;
    FVector LastProgressPosition = FVector::ZeroVector;
    double MotionEpoch=0,NextDemoJump=0,JumpReleaseAt=0,NextObstacleSearch=0,ObstacleUntil=0,MotionPauseUntil=0,PosturePauseUntil=0;
    int32 ObstacleSearch=0,MotionPosture=0;
    bool SawMotionTraversal=false;
    FVector ObstacleFace=FVector::ZeroVector;
    bool HasSight(class AERCharacter* Other) const;
    void Patrol(class AERCharacter* BotCharacter, float DeltaTime);
    void MovementDemo(class AERCharacter* BotCharacter,float DeltaTime);
    bool FindMotionObstacle(class AERCharacter* BotCharacter);
};
