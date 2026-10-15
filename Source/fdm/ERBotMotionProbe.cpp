#include "ERBotMotionProbe.h"
#include "ERBotController.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "ERMatch.h"
#include "ERWeapon.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

AERBotMotionProbe::AERBotMotionProbe(){bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;}
void AERBotMotionProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AERBotMotionProbe,Phase);}
void AERBotMotionProbe::Tick(float Delta)
{
    Super::Tick(Delta);
#if !UE_BUILD_SHIPPING
    const float Now=GetWorld()->GetTimeSeconds();
    if(HasAuthority() && Phase==0)
    {
        auto* Mode=GetWorld()->GetAuthGameMode<AERGameMode>();
        if(!Mode || Mode->GetNumPlayers()!=2 || Now<5)return;
        // Start fresh lives after the observer joins; loading must not leave
        // a bot halfway through an earlier posture cycle in the fixture.
        int32 Count=0;
        for(TActorIterator<AERBotController> It(GetWorld());It;++It)
        {
            if(APawn* Old=It->GetPawn()){It->UnPossess();Old->Destroy();}
            Mode->RestartPlayer(*It);
            auto* C=Cast<AERCharacter>(It->GetPawn());auto* PS=It->GetPlayerState<AERPlayerState>();if(!C || !PS)return;
            const bool Alpha=PS->Team==EERTeam::Alpha;
            const FVector P=Alpha ? FVector(475,0,94.6):FVector(1125,650,94.6);
            C->GetCharacterMovement()->StopMovementImmediately();C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            C->TeleportTo(P,FRotator::ZeroRotator);It->SetControlRotation(FRotator::ZeroRotator);It->ResetLife();++Count;
        }
        if(Count!=2)return;
        Phase=1;Started=Now;ForceNetUpdate();
    }
    if(Phase<1)return;
    int32 Count=0;
    for(TActorIterator<AERCharacter> It(GetWorld());It;++It)
    {
        auto* C=*It;auto* PS=C->GetPlayerState<AERPlayerState>();if(!PS || !PS->IsABot() || C->Health->bDead)continue;
        // Respawn/replication delivers PlayerState, mesh and weapon separately.
        // Observe after they agree; require both fully ready at final report.
        if(!C->Weapon || !C->Weapon->Data || !C->GetTeamAppearance().Matches(C->GetMesh()))continue;
        ++Count;auto* M=Cast<UERMovement>(C->GetCharacterMovement());const float H=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        const int32 Key=int32(PS->Team);uint32& Bits=Evidence.FindOrAdd(Key);const uint32 Before=Bits;
        if(M->IsFalling() && C->GetVelocity().Z>80)Bits|=1;
        if(M->IsTraversing())Bits|=2;
        if(C->bIsCrouched && FMath::IsNearlyEqual(H,62.5f,.2f) && !M->IsProne())Bits|=4;
        if(M->IsProne() && FMath::IsNearlyEqual(H,34.f,.2f) && M->Locomotion.ProneBlend>.95 &&
            FMath::Abs(C->GetMesh()->GetRelativeTransform().GetRotation().GetAxisZ().Z)<.2 && C->GetMesh()->IsVisible())Bits|=8;
        if((Bits&8) && !M->IsChangingPosture() && FMath::IsNearlyEqual(H,92.5f,.2f))Bits|=16;
        if(M->IsProne() && C->GetVelocity().Size2D()>10)Bits|=32;
        const float Expected=PS->Team==EERTeam::Alpha ? 80:140;
        if(M->IsMovingOnGround() && FMath::IsNearlyEqual(C->GetActorLocation().Z-H,Expected+2.1f,4))Bits|=64;
        const bool Quiet=C->Weapon->MagazineAmmo==C->Weapon->Data->MagazineSize && C->Weapon->AuthorizedShotCount==0;
        if(!Quiet && !Failed)UE_LOG(LogTemp,Display,TEXT("ER_BOT_MOTION_FIRE_FAIL team=%d ammo=%d shots=%u"),Key,C->Weapon->MagazineAmmo,C->Weapon->AuthorizedShotCount);
        Failed|=!Quiet;
        if(Bits!=Before)UE_LOG(LogTemp,Display,TEXT("ER_BOT_MOTION_EVIDENCE side=%s team=%d bits=%u feet=%.1f"),HasAuthority() ? TEXT("server"):TEXT("client"),Key,Bits,C->GetActorLocation().Z-H);
    }
    if((!HasAuthority() && Phase==2 && !Reported) || (HasAuthority() && Phase==1 && Now-Started>27))
    {
        bool Good=Count==2 && Evidence.Num()==2 && !Failed;for(const auto& P:Evidence)Good&=P.Value==127;
        UE_LOG(LogTemp,Display,TEXT("ER_BOT_MOTION_%s%s count=%d"),HasAuthority() ? TEXT(""):TEXT("CLIENT_"),Good ? TEXT("PASS"):TEXT("FAIL"),Count);
        Failed|=!Good;Reported=true;
        if(HasAuthority()){Phase=2;FinishAt=Now+2;ForceNetUpdate();}
    }
    if(HasAuthority() && Phase==2 && Now>=FinishAt)
    {
        UE_LOG(LogTemp,Display,TEXT("ER_BOT_MOTION_COMPLETE result=%s"),Failed ? TEXT("FAIL"):TEXT("PASS"));FPlatformMisc::RequestExitWithStatus(false,Failed ? 1:0);Phase=3;
    }
#endif
}
