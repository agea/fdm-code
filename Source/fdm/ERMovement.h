#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ERMovement.generated.h"

// Intent travels with Unreal's saved moves, including correction/replay.
UCLASS()
class FDM_API UERMovement : public UCharacterMovementComponent
{
    GENERATED_BODY()
public:
    UERMovement();
    bool bWalkHeld = false;
    bool bClimbHeld = false;
    bool bHighClimb = false;
    virtual float GetMaxSpeed() const override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;
    virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
    virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
    virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
    bool FindLedge(FVector& Destination, float& Height) const;
    bool IsTraversing() const { return MovementMode == MOVE_Custom && CustomMovementMode == 1; }
// Saved moves restore this state when Unreal replays a corrected prediction.
    FVector TraversalStart = FVector::ZeroVector, TraversalEnd = FVector::ZeroVector;
    float TraversalElapsed = 0, TraversalDuration = 0;
    double RetryAfter = 0;
};
