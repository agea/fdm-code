#include "ERMapTemplateProbe.h"
#include "ERProceduralMap.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "ERPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"
#include "Misc/App.h"
#include "Misc/Paths.h"

namespace
{
void Space(APlayerController* PC,EInputEvent Event)
{
    FInputKeyEventArgs Args;Args.Key=EKeys::SpaceBar;Args.Event=Event;Args.AmountDepressed=Event==IE_Released ? 0.f:1.f;
    Args.EventTimestamp=FPlatformTime::Cycles64();PC->InputKey(Args);
}
FVector RoofStart(const FERTemplateBase& Base,int32 Side)
{
    return Base.Point(Side==0 ? FVector(0,Base.Size.Y*.5-55,Base.RoofZ+94.6):Side==1 ? FVector(Base.Size.X*.5-55,0,Base.RoofZ+94.6):Side==2 ? FVector(0,-Base.Size.Y*.5+55,Base.RoofZ+94.6):FVector(-Base.Size.X*.5+55,0,Base.RoofZ+94.6));
}
bool Mounted(AERCharacter* C,const FERTemplateBase& Base,int32 Side)
{
    const FVector P=Base.Frame.InverseTransformPosition(C->GetActorLocation());
    const float Feet=P.Z-C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float Along=Side%2 ? FMath::Abs(P.X):FMath::Abs(P.Y),Boundary=Side%2 ? Base.Size.X*.5:Base.Size.Y*.5;
    return C->GetCharacterMovement()->IsMovingOnGround() && FMath::IsNearlyEqual(Feet,Base.RoofZ+102.1f,4) && Along>Boundary-21 && Along<Boundary+1;
}
}
void AERMapTemplateProbe::TickParapets(float Delta,AERProceduralMap* Map)
{
#if !UE_BUILD_SHIPPING
    auto* Local=Cast<APlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    if(!Map->IsMapReady() || Map->GetLayout().Bases.Num()!=2 || !Local || !Local->GetPawn())return;
    APlayerController* Remote=nullptr;
    if(HasAuthority())for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)if(!It->Get()->IsLocalController())Remote=It->Get();
    if(HasAuthority() && Phase==0)
    {
        if(Elapsed>55){UE_LOG(LogTemp,Error,TEXT("ER_PARAPET_COMPLETE result=FAIL missing remote"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
        if(!Remote || !Remote->GetPawn() || Elapsed<5)return;
        Phase=1;ParapetElapsed=0;ForceNetUpdate();
    }
    if(Phase<1 || Phase>16)return;
    const int32 Trial=(Phase-1)%8,BaseIndex=Trial/4,Side=Trial%4;
    const auto& Base=Map->GetLayout().Bases[BaseIndex];const float Yaw=Base.Frame.Rotator().Yaw+90-Side*90;
    auto* PC=HasAuthority() ? (Phase<=8 ? Local:Remote):Local;
    auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()):nullptr;if(!C)return;
    auto* M=Cast<UERMovement>(C->GetCharacterMovement());
    if(ParapetLocalPhase!=Phase)
    {
        ParapetLocalPhase=Phase;ParapetElapsed=0;ParapetJump=ParapetTraversal=ParapetReported=false;
        if(PC->IsLocalController())Space(PC,IE_Released);
        if(HasAuthority())
        {
            // The host ends case8 on the same ledge the remote needs in
            // case16. Park it before remote trials: occupied landings must
            // remain blocked by normal full-capsule clearance.
            if(Phase==9)
                if(auto* Park=Cast<AERCharacter>(Local->GetPawn()))
                {
                    Park->GetCharacterMovement()->StopMovementImmediately();
                    Park->TeleportTo(Map->GetSpawnLocation(),FRotator::ZeroRotator);
                }
            M->StopMovementImmediately();M->SetMovementMode(MOVE_Walking);
            C->TeleportTo(RoofStart(Base,Side),FRotator(0,Yaw,0));PC->SetControlRotation(FRotator(0,Yaw,0));M->ForceClientAdjustment();
        }
    }
    ParapetElapsed+=Delta;ParapetTraversal|=M->IsTraversing();
    // The host drives only its own cases; the autonomous client supplies its
    // normal Space binding and movement, replicated through saved moves.
    const bool Drive=PC->IsLocalController() && (HasAuthority() ? Phase<=8:Phase>8);
    if(Drive)
    {
        PC->SetControlRotation(FRotator(0,Yaw,0));
        if(ParapetElapsed>.4f && !ParapetJump){Space(PC,IE_Pressed);ParapetJump=true;}
        if(ParapetElapsed>.4f && !ParapetReported)
        {
            if(!M->IsTraversing() && C->GetActorLocation().Z>Base.RoofZ+180)M->StopMovementImmediately();
            else C->AddMovementInput(FRotator(0,Yaw,0).Vector());
        }
        if(ParapetElapsed>2.4f && !ParapetReported)
        {
            const bool Good=Mounted(C,Base,Side) && ParapetTraversal;
            UE_LOG(LogTemp,Display,TEXT("ER_PARAPET_%s%s case=%d feet=%.1f"),HasAuthority() ? TEXT(""):TEXT("CLIENT_"),Good ? TEXT("PASS"):TEXT("FAIL"),Phase,C->GetActorLocation().Z-C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
            Failed|=!Good;ParapetReported=true;Space(PC,IE_Released);M->StopMovementImmediately();
            if(HasAuthority() && Phase==1 && FApp::CanEverRender())FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Killhouse-Parapet.png"),true,false);
        }
    }
    if(HasAuthority() && ParapetElapsed>3.2f)
    {
        const bool Good=Mounted(C,Base,Side) && ParapetTraversal;
        UE_LOG(LogTemp,Display,TEXT("ER_PARAPET_SERVER_%s case=%d team=%d side=%d feet=%.1f traversed=%d"),Good ? TEXT("PASS"):TEXT("FAIL"),Phase,BaseIndex,Side,C->GetActorLocation().Z-C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),ParapetTraversal);
        Failed|=!Good;
        if(Phase==16)
        {
            UE_LOG(LogTemp,Display,TEXT("ER_PARAPET_COMPLETE result=%s"),Failed ? TEXT("FAIL"):TEXT("PASS"));
            FPlatformMisc::RequestExitWithStatus(false,Failed ? 1:0);Phase=17;
        }
        else{++Phase;ForceNetUpdate();}
    }
#endif
}
