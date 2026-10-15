#include "ERBlockNetworkProbe.h"
#include "ERProceduralMap.h"
#include "ERBlockNetPage.h"
#include "ERPlayerController.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERBlockCombat.h"
#include "ERMatch.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Algo/Reverse.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
AERBlockNetworkProbe::AERBlockNetworkProbe()
{
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;SetNetUpdateFrequency(20);
}
void AERBlockNetworkProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AERBlockNetworkProbe,Map);DOREPLIFETIME(AERBlockNetworkProbe,Phase);
    DOREPLIFETIME(AERBlockNetworkProbe,ExpectedEpoch);DOREPLIFETIME(AERBlockNetworkProbe,ExpectedSequence);DOREPLIFETIME(AERBlockNetworkProbe,ExpectedDigest);
    DOREPLIFETIME(AERBlockNetworkProbe,ShotId);DOREPLIFETIME(AERBlockNetworkProbe,FireFrom);DOREPLIFETIME(AERBlockNetworkProbe,WalkEnd);DOREPLIFETIME(AERBlockNetworkProbe,FireAim);DOREPLIFETIME(AERBlockNetworkProbe,StairPath);
}
void AERBlockNetworkProbe::Check(bool Value,const TCHAR* Label)
{
    Failed|=!Value;UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_PROBE_%s %s phase=%d"),Value ? TEXT("PASS"):TEXT("FAIL"),Label,Phase);
}
void AERBlockNetworkProbe::Advance(int32 Next,float Seconds)
{
    Phase=Next;ExpectedEpoch=Map->GetMapEpoch();ExpectedSequence=Map->GetMapSequence();ExpectedDigest=Map->GetStateDigest();
    Deadline=GetWorld()->GetTimeSeconds()+Seconds;ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_PHASE %d epoch=%u sequence=%u digest=%u"),Phase,ExpectedEpoch,ExpectedSequence,ExpectedDigest);
}
void AERBlockNetworkProbe::ReceiveReport(AERPlayerController* PC,int32 Stage,uint32 Epoch,uint32 Sequence,uint32 Digest,bool Passed)
{
    if(!HasAuthority() || Stage!=Phase || !PC || !PC->PlayerState)return;
    const FString Name=PC->PlayerState->GetPlayerName();
    const bool Valid=Passed && Epoch==ExpectedEpoch && Sequence==ExpectedSequence && Digest==ExpectedDigest;
    Check(Valid,TEXT("remote_snapshot_collision_report"));Reports.Add(Name+FString::FromInt(Stage));
    UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_REPORT name=%s phase=%d valid=%d epoch=%u sequence=%u digest=%u"),*Name,Stage,Valid,Epoch,Sequence,Digest);
}
bool AERBlockNetworkProbe::Reported(const TCHAR* Name,int32 Stage) const {return Reports.Contains(FString(Name)+FString::FromInt(Stage));}
void AERBlockNetworkProbe::Place(AERPlayerController* PC,const FVector& P,const FRotator& Aim)
{
    if(!PC || !PC->GetPawn())return;PC->GetPawn()->TeleportTo(P,Aim);
    auto* C=CastChecked<AERCharacter>(PC->GetPawn());C->GetCharacterMovement()->StopMovementImmediately();C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);C->GetCharacterMovement()->ForceClientAdjustment();
    PC->SetControlRotation(Aim);PC->ClientSetRotation(Aim);
}
bool AERBlockNetworkProbe::CollisionParity() const
{
    if(!Map || !Map->ValidateMappings() || Map->GetInstanceCount()!=Map->BlockWorld->GetBlocks().Num()-Map->BlockWorld->GetRemovedCount())return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ERBlockNetworkParity),false);FHitResult Hit;
    const auto& Blocks=Map->BlockWorld->GetBlocks();
    for(int32 I=0;I<Blocks.Num();I+=13)
    {
        const auto& B=Blocks[I];const auto T=Map->GetBlockTransform(B.Id);const auto N=T.GetRotation().GetAxisY()*T.GetScale3D().Y*40;
        const bool Found=GetWorld()->LineTraceSingleByChannel(Hit,T.GetLocation()+N,T.GetLocation()-N,ECC_GameTraceChannel1,Q);
        FERBlockId Id;
        if(B.IsAttached() ? !(Found && Map->ResolveHit(Hit,Id) && Id==B.Id):Found)return false;
    }
    return true;
}
void AERBlockNetworkProbe::ClientTick(float DeltaSeconds)
{
    if(Role.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("ERBlockNetRole="),Role);
    auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()):nullptr;
    if(PC && PC->PlayerState && !NameSent && !Role.IsEmpty()){PC->ServerSetName(Role==TEXT("initial")?TEXT("Initial"):Role==TEXT("late")?TEXT("Late"):TEXT("Dense"));NameSent=true;}
    if(PC && C && Map && Phase>0 && GetWorld()->GetTimeSeconds()>NextDiagnostic)
    {
        NextDiagnostic=GetWorld()->GetTimeSeconds()+2;
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_WAIT phase=%d role=%s ready=%d current=%d epoch=%u/%u sequence=%u applied=%u expected=%u digest=%u/%u pos=%s from=%s mode=%d tick=%d"),Phase,*Role,Map->IsMapReady(),Map->IsSnapshotCurrent(),Map->GetMapEpoch(),ExpectedEpoch,Map->GetMapSequence(),Map->GetAppliedMapSequence(),ExpectedSequence,Map->GetStateDigest(),ExpectedDigest,*C->GetActorLocation().ToString(),*FireFrom.ToString(),int32(C->GetCharacterMovement()->MovementMode),C->GetCharacterMovement()->IsComponentTickEnabled());
    }
    if(!PC || !C || !C->Weapon || !Map || !Map->IsSnapshotCurrent() || Map->GetMapEpoch()!=ExpectedEpoch || Map->GetMapSequence()!=ExpectedSequence || Map->GetStateDigest()!=ExpectedDigest)return;
    if(ClientPhase!=Phase){ClientPhase=Phase;PhaseElapsed=0;Waypoint=0;}PhaseElapsed+=DeltaSeconds;
    const bool InitialRole=Role==TEXT("initial");
    if((Phase==1 || Phase==2) && InitialRole)
    {
        if(PhaseElapsed<.5 || FVector::Dist2D(C->GetActorLocation(),FireFrom)>20)return;
        PC->SetControlRotation(FireAim);
        if(!Reports.Contains(FString::FromInt(Phase)))
        {
            bool Passed=CollisionParity();
            if(Phase==1)
            {
                const uint32 Before=Map->GetStateDigest();
                Passed&=!Map->ApplyBlockDamage(ShotId,10000,true) && !Map->Regenerate();
                Passed&=AERBlockRoom::ApplyWorldExplosion(GetWorld(),FireFrom,500,10000).Damaged==0 && Before==Map->GetStateDigest();
            }
            C->ServerStartFire();C->ServerStopFire();Reports.Add(FString::FromInt(Phase));
            PC->ServerBlockProbeReport(this,Phase,ExpectedEpoch,ExpectedSequence,ExpectedDigest,Passed);
            UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_CLIENT_%s phase=%d role=%s authority_guard_and_real_fire"),Passed ? TEXT("PASS"):TEXT("FAIL"),Phase,*Role);
        }
    }
    else if(Phase==3 && InitialRole)
    {
        if(Reports.Contains(TEXT("3")))return;
        const FVector D=WalkEnd-C->GetActorLocation();
        if(FVector2D(D.X,D.Y).Size()>15){C->AddMovementInput(FVector(D.X,D.Y,0).GetSafeNormal(),1);return;}
        C->GetCharacterMovement()->StopMovementImmediately();const bool Passed=C->GetActorLocation().Z>65 && CollisionParity();
        PC->ServerBlockProbeReport(this,3,ExpectedEpoch,ExpectedSequence,ExpectedDigest,Passed);Reports.Add(TEXT("3"));
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_CLIENT_%s phase=3 role=%s actual_breach_traversal pos=%s"),Passed ? TEXT("PASS"):TEXT("FAIL"),*Role,*C->GetActorLocation().ToString());
    }
    else if((Phase==7 || Phase==8) && InitialRole)
    {
        if(Reports.Contains(FString::FromInt(Phase)))return;
        while(Waypoint<StairPath.Num())
        {
            const FVector D=StairPath[Waypoint]-C->GetActorLocation();
            if(FVector2D(D.X,D.Y).Size()<20 && FMath::Abs(D.Z)<25){++Waypoint;continue;}
            C->AddMovementInput(FVector(D.X,D.Y,0).GetSafeNormal(),1);return;
        }
        C->GetCharacterMovement()->StopMovementImmediately();const bool Passed=CollisionParity() && FMath::Abs(C->GetActorLocation().Z-StairPath.Last().Z)<25;
        PC->ServerBlockProbeReport(this,Phase,ExpectedEpoch,ExpectedSequence,ExpectedDigest,Passed);Reports.Add(FString::FromInt(Phase));
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_CLIENT_%s phase=%d role=%s actual_stair_walk pos=%s"),Passed ? TEXT("PASS"):TEXT("FAIL"),Phase,*Role,*C->GetActorLocation().ToString());
    }
    else if(Phase==5 || Phase==6 || Phase==9 || Phase==10 || Phase==12)
    {
        if(Reports.Contains(FString::FromInt(Phase)))return;
        bool Passed=CollisionParity();
        if(Phase==5)Passed&=Map->BlockWorld->GetDetachedCount()==9;
        if(Phase==6 || Phase==10)Passed&=Map->BlockWorld->GetRemovedCount()==0;
        if(Phase==9)Passed&=Map->GetInstanceCount()==0;
        if(Phase==12)Passed&=Map->BlockWorld->GetRemovedCount()>0;
        if((Role==TEXT("late") && Phase==5) || (Role==TEXT("dense") && Phase==9))
        {int32 Alive=0,Physics=0;Map->GetDebrisCounts(Alive,Physics);Passed&=Alive==0;}
        PC->ServerBlockProbeReport(this,Phase,ExpectedEpoch,ExpectedSequence,ExpectedDigest,Passed);Reports.Add(FString::FromInt(Phase));
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_CLIENT_%s phase=%d role=%s epoch=%u digest=%u bodies=%d removed=%d"),Passed ? TEXT("PASS"):TEXT("FAIL"),Phase,*Role,ExpectedEpoch,ExpectedDigest,Map->GetMemoryProfile().CollisionBodies,Map->BlockWorld->GetRemovedCount());
    }
}
void AERBlockNetworkProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    if(!HasAuthority()){ClientTick(DeltaSeconds);return;}
    if(!Map){TActorIterator<AERProceduralMap> It(GetWorld());if(It)Map=*It;}
    if(!Map)return;
    for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
        if(auto* PC=Cast<AERPlayerController>(It->Get()))if(PC->PlayerState)
        {
            const FString Name=PC->PlayerState->GetPlayerName();
            if(PC->IsLocalController())Host=PC;
            else if(Name==TEXT("Initial"))Initial=PC;
            else if(Name==TEXT("Late"))Late=PC;
            else if(Name==TEXT("Dense"))Dense=PC;
        }
    if(Phase==0)
    {
        if(!Host.IsValid() || !Initial.IsValid() || !Initial->GetPawn() || !GetWorld()->GetAuthGameMode<AERGameMode>()->IsMatchInProgress())return;
        const auto* R=Map->BlockWorld->GetBlocks().FindByPredicate([](const auto& B){return B.Id.Wall==5 && B.Id.Row==6 && B.Id.Column==2;});
        if(!R){Check(false,TEXT("shot_fixture"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
        ShotId=R->Id;const auto T=Map->GetBlockTransform(ShotId);const FVector N=T.GetRotation().GetAxisY();
        FireFrom=T.GetLocation()+N*150;FireFrom.Z=100;FireAim=(-N).Rotation();Place(Initial.Get(),FireFrom,FireAim);
        Map->ApplyBlockDamage(ShotId,R->Health-1);ShotsBefore=CastChecked<AERCharacter>(Initial->GetPawn())->Weapon->AuthorizedShotCount;
        Check(Map->LayoutConfig.Floors==2 && Map->LayoutConfig.AdditionalMaterials && Map->LayoutConfig.WindowGlass,TEXT("two_floor_network_fixture"));Advance(1);return;
    }
    if(GetWorld()->GetTimeSeconds()>Deadline)
    {Check(false,TEXT("phase_timeout"));UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_PROBE_COMPLETE result=FAIL"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
    auto* C=Initial.IsValid()?Cast<AERCharacter>(Initial->GetPawn()):nullptr;
    if(!C || !C->Weapon)return;
    if(Phase==1 && Reported(TEXT("Initial"),1) && C->Weapon->AuthorizedShotCount>ShotsBefore)
    {
        Check(C->Weapon->AuthorizedShotCount==ShotsBefore+1 && Map->BlockWorld->Find(ShotId)->State==EERBlockState::Destroyed,TEXT("remote_authorized_weapon_destroys_block"));
        TArray<FERBlockDamageRequest> Breach;for(int32 Col=3;Col<=5;++Col)for(int32 Row=0;Row<8;++Row)Breach.Add({{5,Col,Row},0,true});Map->ApplyDamageBatch(Breach);
        auto P=Map->GetBlockTransform({5,4,3}).GetLocation();P.Z=100;FireFrom=P-FVector(100,0,0);WalkEnd=P+FVector(100,0,0);FireAim=FRotator::ZeroRotator;
        Place(Initial.Get(),FireFrom,FireAim);Place(Host.Get(),P+FVector(220,0,0),FRotator(0,180,0));CastChecked<AERCharacter>(Host->GetPawn())->Health->ProtectionEndServerTime=0;
        ShotsBefore=C->Weapon->AuthorizedShotCount;Advance(2);
    }
    else if(Phase==2 && Reported(TEXT("Initial"),2) && C->Weapon->AuthorizedShotCount>ShotsBefore)
    {Check(CastChecked<AERCharacter>(Host->GetPawn())->Health->Health==64,TEXT("remote_hitscan_through_replicated_breach"));Advance(3);}
    else if(Phase==3 && Reported(TEXT("Initial"),3))
    {
        Check(FVector::Dist2D(C->GetActorLocation(),WalkEnd)<60 && C->GetActorLocation().Z>65,TEXT("authoritative_remote_breach_crossing"));
        TArray<FERBlockDamageRequest> Ring;for(int32 Col=3;Col<=7;++Col)for(int32 Row=5;Row<=9;++Row)if(Col==3 || Col==7 || Row==5 || Row==9)Ring.Add({{7,Col,Row},0,true});Map->ApplyDamageBatch(Ring);Advance(4);
    }
    else if(Phase==4 && !Map->IsSupportPending())
    {Check(Map->BlockWorld->GetDetachedCount()==9 && CollisionParity(),TEXT("authority_collapse_and_collision"));Advance(5,45);}
    else if(Phase==5)
    {
        if(Reported(TEXT("Initial"),5) && !LateAnnounced){LateAnnounced=true;UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_READY_FOR_LATE_JOIN"));}
        if(!Reported(TEXT("Initial"),5) || !Reported(TEXT("Late"),5))return;
        Check(Late.IsValid() && Map->ValidateMappings(),TEXT("late_join_matches_damaged_collapsed_snapshot"));
        Map->LayoutConfig.Seed=7919;Map->LayoutConfig.Floors=3;Check(Map->Regenerate(),TEXT("authority_epoch_regeneration"));Advance(6,30);
    }
    else if(Phase==6 && Reported(TEXT("Initial"),6) && Reported(TEXT("Late"),6))
    {
        Check(Map->BlockWorld->GetRemovedCount()==0 && Map->LayoutConfig.Floors==3,TEXT("both_clients_new_epoch_no_stale_deltas"));
        StairPath.Reset();for(const auto& R:Map->GetLayout().StairRoutes)StairPath.Append(R);
        FireFrom=StairPath[0];Place(Initial.Get(),FireFrom,FRotator(0,180,0));Advance(7,45);
    }
    else if(Phase==7 && Reported(TEXT("Initial"),7))
    {
        Check(C->GetActorLocation().Z>700,TEXT("authoritative_remote_climbs_two_storeys"));
        Algo::Reverse(StairPath);Advance(8,45);
        if(FApp::CanEverRender())
        {
            const FVector View(-1800,-1300,900),Target(600,0,430);
            Place(Host.Get(),View,(Target-View).Rotation());CastChecked<AERCharacter>(Host->GetPawn())->GetCharacterMovement()->SetMovementMode(MOVE_None);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Procedural-Multifloor.png"),true,false);
        }
    }
    else if(Phase==8 && Reported(TEXT("Initial"),8))
    {
        Check(C->GetActorLocation().Z>90 && C->GetActorLocation().Z<110,TEXT("authoritative_remote_descends_to_ground"));
        TArray<FERBlockDamageRequest> All;for(const auto& B:Map->BlockWorld->GetBlocks())if(B.IsAttached())All.Add({B.Id,0,true});Map->ApplyDamageBatch(All);
        Advance(9,40);UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_READY_FOR_DENSE_JOIN blocks=%d"),Map->BlockWorld->GetBlocks().Num());
    }
    else if(Phase==9 && Reported(TEXT("Initial"),9) && Reported(TEXT("Late"),9) && Reported(TEXT("Dense"),9))
    {Check(Map->GetInstanceCount()==0 && Dense.IsValid(),TEXT("dense_late_join_full_removed_map"));Check(Map->Regenerate(),TEXT("second_epoch_regeneration"));Advance(10,40);}
    else if(Phase==10 && Reported(TEXT("Initial"),10) && Reported(TEXT("Late"),10) && Reported(TEXT("Dense"),10))
    {
        Check(CollisionParity() && Map->BlockWorld->GetRemovedCount()==0,TEXT("three_clients_restore_dense_snapshot"));
        const auto T=Map->GetBlockTransform({5,4,3});
        const auto Blast=AERBlockRoom::ApplyWorldExplosion(GetWorld(),T.GetLocation()+T.GetRotation().GetAxisY()*20,250,2400);
        Check(Blast.Damaged>10 && Blast.Destroyed>0,TEXT("authority_explosion_publishes_batched_state"));Advance(11,30);
    }
    else if(Phase==11 && !Map->IsSupportPending()){Advance(12,30);}
    else if(Phase==12 && Reported(TEXT("Initial"),12) && Reported(TEXT("Late"),12) && Reported(TEXT("Dense"),12))
    {
        Check(CollisionParity(),TEXT("three_clients_match_explosion_and_support_delta"));
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_PROBE_COMPLETE result=%s epoch=%u blocks=%d pages=%d"),Failed ? TEXT("FAIL"):TEXT("PASS"),Map->GetMapEpoch(),Map->BlockWorld->GetBlocks().Num(),FMath::DivideAndRoundUp(Map->BlockWorld->GetBlocks().Num(),128));
        FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);Phase=13;
    }
#endif
}
