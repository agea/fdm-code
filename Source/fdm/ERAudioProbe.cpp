#include "ERAudioProbe.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERMovement.h"
#include "ERMatch.h"
#include "ERFootsteps.h"
#include "ERPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

AERAudioProbe::AERAudioProbe(){PrimaryActorTick.bCanEverTick=true;bReplicates=true;bAlwaysRelevant=true;}
void AERAudioProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AERAudioProbe,Target);DOREPLIFETIME(AERAudioProbe,ExpectedData);DOREPLIFETIME(AERAudioProbe,SelectionToken);
}
void AERAudioProbe::ConfirmReady(AERPlayerController* Player,uint32 Token)
{
    if(HasAuthority() && Client && Client->Controller==Player && Token==SelectionToken)AckedToken=Token;
}
void AERAudioProbe::Check(bool Value,const TCHAR* Label)
{
    Failed|=!Value;
    UE_LOG(LogTemp,Display,TEXT("ER_AUDIO_%s %s side=%d slot=%d"),Value?TEXT("PASS"):TEXT("FAIL"),Label,Side,Gun);
}
void AERAudioProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    if(!HasAuthority())
    {
        auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
        if(!PC || !Target || !ExpectedData || !SelectionToken)return;
        const double Now=GetWorld()->GetTimeSeconds();
        if(LocalToken!=SelectionToken){LocalToken=SelectionToken;StableFrames=0;LocalReadyAfter=Now+1.5;}
        const bool ModelReady=Target->Weapon && Target->Weapon->Data==ExpectedData &&
            Target->WorldRifle->GetSkeletalMeshAsset()==ExpectedData->Mesh &&
            (!Target->IsLocallyControlled() || Target->FirstPersonRifle->GetSkeletalMeshAsset()==ExpectedData->Mesh);
        if(!ModelReady){StableFrames=0;LocalReadyAfter=Now+1.5;return;}
        if(++StableFrames>=20 && Now>=LocalReadyAfter && SentToken!=SelectionToken)
        {
            PC->ServerAudioProbeReady(this,SelectionToken);SentToken=SelectionToken;
            UE_LOG(LogTemp,Display,TEXT("ER_AUDIO_CLIENT_READY token=%u weapon=%s"),SelectionToken,*ExpectedData->GetName());
        }
        return;
    }
    auto* Mode=GetWorld()->GetAuthGameMode<AERGameMode>();if(!Mode || !Mode->IsMatchInProgress())return;
    if(!Host || !Client)
    {
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(auto* PC=Cast<AERPlayerController>(It->Get()))
            {
                if(PC->IsLocalController())Host=Cast<AERCharacter>(PC->GetPawn());
                else Client=Cast<AERCharacter>(PC->GetPawn());
            }
        if(!Host || !Client)return;
        Host->TeleportTo(FVector(-1000,-1500,95),FRotator::ZeroRotator);
        Client->TeleportTo(FVector(-600,-1500,95),FRotator::ZeroRotator);
        for(auto* C:{Host.Get(),Client.Get()})
        {
            C->Health->ProtectionEndServerTime=GetWorld()->GetTimeSeconds()+500;
            C->Controller->SetControlRotation(FRotator(65,0,0));
            Cast<APlayerController>(C->Controller)->ClientSetRotation(FRotator(65,0,0));
            Check(C->Footsteps && C->Footsteps->Audio,TEXT("footsteps_bound_on_humans"));
        }
    }
    const double Now=GetWorld()->GetTimeSeconds();
    if(!Ready)
    {
        // Do not count historical shots fired while the peer is still loading.
        // The normal possession acknowledgment proves its pawn is initialized.
        const auto* PC=Cast<APlayerController>(Client->Controller);
        if(!PC || PC->AcknowledgedPawn!=Client)return;
        Ready=true;Deadline=Now+2;return;
    }
    if(Phase==5)
    {
        if(Now<Deadline){Host->AddMovementInput(FVector(1,0,0),1.f);return;}
        Check(Host->Footsteps->EmittedCount>StepBefore+2,TEXT("owner_steps_from_actual_ground_travel"));
        UE_LOG(LogTemp,Display,TEXT("ER_AUDIO_COMPLETE result=%s"),Failed?TEXT("FAIL"):TEXT("PASS"));
        FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);return;
    }
    if(Now<Deadline)return;
    auto* C=Side==0?Host.Get():Client.Get();
    if(Phase==0)
    {
        C->ServerSelectWeapon(Gun);Target=C;ExpectedData=C->Weapon->Data;++SelectionToken;ForceNetUpdate();
        Shot=0;Phase=1;Deadline=Now+.3;return;
    }
    auto* W=C->Weapon.Get();if(!W || !W->Data)return;
    if(Phase==1)
    {
        if(AckedToken!=SelectionToken)return;
        W->MagazineAmmo=FMath::Max(W->MagazineAmmo,2);
        const auto Count=W->AuthorizedShotCount,Feedback=C->ShotFeedbackCount;
        C->ServerStartFire();C->ServerStopFire();
        Check(W->AuthorizedShotCount==Count+1 && C->ShotFeedbackCount==Feedback+1,TEXT("one_event_per_authorized_shell"));
        if(Gun==3)Check(W->LastShotTraceCount==6,TEXT("six_pellets_one_sound"));
        Check(W->Data->ShotAudio!=nullptr,TEXT("weapon_palette_bound"));
        if(++Shot==20)Phase=2;
        Deadline=Now+W->Data->ShotInterval+.025;return;
    }
    if(Phase==2)
    {
        const auto Count=W->AuthorizedShotCount,Feedback=C->ShotFeedbackCount;
        W->MagazineAmmo=0;C->ServerStartFire();C->ServerStopFire();
        Check(W->AuthorizedShotCount==Count && C->ShotFeedbackCount==Feedback,TEXT("empty_silent"));
        W->MagazineAmmo=200;
        C->ServerStartFire();C->ServerStopFire();
        const auto Accepted=W->AuthorizedShotCount,Events=C->ShotFeedbackCount;
        C->ServerStartFire();C->ServerStopFire();
        Check(W->AuthorizedShotCount==Accepted && C->ShotFeedbackCount==Events,TEXT("cooldown_denied_silent"));
        if(Gun==2 || Gun==4){Phase=3;Deadline=Now+W->Data->ShotInterval+.05;return;}
        Phase=4;
    }
    if(Phase==3)
    {
        Before=W->AuthorizedShotCount;FeedbackBefore=C->ShotFeedbackCount;
        C->ServerStartFire();Phase=4;Deadline=Now+4;return;
    }
    if(Phase==4)
    {
        C->ServerStopFire();
        if(Gun==2 || Gun==4)Check(W->AuthorizedShotCount>Before+20 && W->AuthorizedShotCount-Before==C->ShotFeedbackCount-FeedbackBefore,TEXT("sustained_auto_one_sound_per_shot"));
        if(++Gun<=7){Phase=0;Deadline=Now+.25;return;}
        if(++Side<2){Gun=1;Phase=0;Deadline=Now+.25;return;}
        int32 Bots=0;
        for(TActorIterator<AERCharacter> It(GetWorld());It;++It)
            if(It->Controller && !It->Controller->IsPlayerController())
            {
                ++Bots;Check(It->Footsteps && It->Footsteps->Audio && It->Footsteps->EmittedCount>3,TEXT("bots_emit_grounded_steps"));
            }
        Check(Bots==2,TEXT("both_team_bots_present"));
        // A clear temporary floor isolates actual travel from Depot obstacles.
        FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;
        auto* Floor=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(0,0,5000),FRotator::ZeroRotator,Params);
        auto* Mesh=Floor->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Mesh->SetWorldScale3D(FVector(20,20,.2));Mesh->SetCollisionObjectType(ECC_WorldStatic);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        Host->GetCharacterMovement()->StopMovementImmediately();
        Host->TeleportTo(FVector(-600,0,5104.6),FRotator::ZeroRotator);
        Host->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Phase=5;StepBefore=Host->Footsteps->EmittedCount;Deadline=Now+3;
    }
#endif
}
