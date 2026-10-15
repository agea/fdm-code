#include "ERBotController.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "Components/CapsuleComponent.h"

bool AERBotController::FindMotionObstacle(AERCharacter* C)
{
    auto* M=Cast<UERMovement>(C->GetCharacterMovement());
    const auto* Capsule=C->GetCapsuleComponent();
    const FVector P=C->GetActorLocation(),Feet=P-FVector(0,0,Capsule->GetScaledCapsuleHalfHeight());
    const float Radius=Capsule->GetScaledCapsuleRadius();
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ERBotObstacle),false,C);
    // Bounded local detours only: no dependency on a baked NavMesh or map IDs.
    for(int32 I=0;I<12;++I)
    {
        const FVector D=FRotator(0,GetControlRotation().Yaw+((I+ObstacleSearch)%12)*30,0).Vector();
        FHitResult Wall,Top,Hit;
        if(!GetWorld()->LineTraceSingleByChannel(Wall,Feet+FVector(0,0,35),Feet+FVector(0,0,35)+D*700,ECC_Visibility,Q) ||
            Cast<APawn>(Wall.GetActor()) || FMath::Abs(Wall.ImpactNormal.Z)>.2 || FVector::DotProduct(Wall.ImpactNormal,D)>-.5 || Wall.Distance<Radius+30)continue;
        const FVector Inside=Wall.ImpactPoint+D*4;
        if(!GetWorld()->LineTraceSingleByChannel(Top,FVector(Inside.X,Inside.Y,Feet.Z+150),FVector(Inside.X,Inside.Y,Feet.Z+30.1),ECC_Visibility,Q) || !M->IsWalkable(Top))continue;
        const float Height=Top.ImpactPoint.Z-Feet.Z;if(Height<=30 || Height>150)continue;
        const FVector Approach(Wall.ImpactPoint.X-D.X*(Radius+8),Wall.ImpactPoint.Y-D.Y*(Radius+8),P.Z);
        if(GetWorld()->SweepSingleByChannel(Hit,P,Approach,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,Capsule->GetScaledCapsuleHalfHeight()),Q))continue;
        bool Supported=true;const int32 Checks=FMath::CeilToInt(FVector::Dist2D(P,Approach)/100);
        for(int32 J=1;J<=Checks;++J)
        {
            const FVector At=FMath::Lerp(P,Approach,float(J)/Checks);
            if(!GetWorld()->LineTraceSingleByChannel(Hit,At+FVector(0,0,30),At-FVector(0,0,150),ECC_WorldStatic,Q) ||
                !M->IsWalkable(Hit) || FMath::Abs(Hit.ImpactPoint.Z-Feet.Z)>30){Supported=false;break;}
        }
        if(!Supported)continue;
        ObstacleFace=FVector(Wall.ImpactPoint.X,Wall.ImpactPoint.Y,P.Z);
        ObstacleUntil=GetWorld()->GetTimeSeconds()+5;return true;
    }
    return false;
}

void AERBotController::MovementDemo(AERCharacter* C,float DeltaTime)
{
    auto* M=Cast<UERMovement>(C->GetCharacterMovement());const double Now=GetWorld()->GetTimeSeconds();
    if(JumpReleaseAt && Now>=JumpReleaseAt){C->StopJumping();JumpReleaseAt=0;}
    if(M->IsTraversing())
    {
        SawMotionTraversal=true;ObstacleUntil=0;NextProgressCheckAt=Now+2;return;
    }
    if(SawMotionTraversal)
    {
        NextProgressCheckAt=Now+2;
        if(!M->IsMovingOnGround())return;
        SawMotionTraversal=false;MotionPauseUntil=Now+.8;
    }
    const float Cycle=FMath::Fmod(float(Now-MotionEpoch),22.f);
    const bool Crouch=Cycle>=7 && Cycle<10,Prone=Cycle>=12 && Cycle<17;
    const int32 Posture=Prone ? 2:Crouch ? 1:0;
    if(Posture!=MotionPosture){MotionPosture=Posture;PosturePauseUntil=Now+1.25;}
    M->bProneWanted=Prone;M->bClimbHeld=!Crouch && !Prone;
    if(Crouch)C->Crouch();else C->UnCrouch();
    if(Crouch || Prone || M->IsChangingPosture())ObstacleUntil=0;
    if((M->IsChangingPosture() && Now<PosturePauseUntil) || (Prone && Cycle<14) || Now<MotionPauseUntil)
    {
        NextProgressCheckAt=Now+2;
        if(M->IsMovingOnGround())M->StopMovementImmediately();
        return;
    }
    if(!Crouch && !Prone && !M->IsChangingPosture() && M->IsMovingOnGround())
    {
        if(ObstacleUntil<=Now && Now>=NextDemoJump && !M->IsChangingPosture())
        {
            NextDemoJump=Now+6;
            FCollisionQueryParams Q(SCENE_QUERY_STAT(ERBotJump),false,C);FHitResult Hit;
            const auto* Capsule=C->GetCapsuleComponent();const FVector P=C->GetActorLocation();
            if(!GetWorld()->SweepSingleByChannel(Hit,P,P+FVector(0,0,65),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()),Q))
            {C->Jump();JumpReleaseAt=Now+.2;Patrol(C,DeltaTime);return;}
        }
        if(Now>=NextObstacleSearch && ObstacleUntil<=Now)
        {
            NextObstacleSearch=Now+2;FindMotionObstacle(C);++ObstacleSearch;
        }
        if(ObstacleUntil>Now)
        {
            const FVector D=(ObstacleFace-C->GetActorLocation()).GetSafeNormal2D();
            const FRotator Facing=FMath::RInterpTo(GetControlRotation(),D.Rotation(),DeltaTime,8);
            SetControlRotation(Facing);C->SetActorRotation(FRotator(0,Facing.Yaw,0));C->AddMovementInput(D);
            NextProgressCheckAt=Now+2;return;
        }
    }
    Patrol(C,DeltaTime);
}
