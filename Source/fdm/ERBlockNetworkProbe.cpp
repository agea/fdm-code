#include "ERBlockNetworkProbe.h"
#include "ERProceduralMap.h"
#include "ERPlayerController.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERWeaponBallistics.h"
#include "ERBlockCombat.h"
#include "ERMatch.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
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
namespace
{
// Room 0's walls of the first floor: 5 east, 7 west. Inner = the panel facing the room.
int32 WallPanel(const FERCellWorld& W,int32 Wall,bool bInner)
{
    int32 Found=INDEX_NONE;
    for(int32 P=0;P<W.GetPanels().Num();++P)if(W.GetPanels()[P].Id/8==Wall && (Found==INDEX_NONE || bInner))Found=P;
    return Found;
}
FVector WorldCell(const AERProceduralMap* Map,const FERCellId& Id){return Map->GetActorTransform().TransformPosition(Map->GetCellWorld().GetCellCenter(Id));}
FVector Inward(const AERProceduralMap* Map,int32 Panel){return Map->GetActorTransform().TransformVectorNoScale(Map->GetCellWorld().GetPanels()[Panel].Transform.GetRotation().GetAxisY());}
// Every cell of the given walls inside [X0,X1]x[Z0,Z1]; ring keeps only the perimeter.
TMap<FERCellId,float> Region(const FERCellWorld& W,int32 Wall,int32 X0,int32 X1,int32 Z0,int32 Z1,bool bRing,float Amount=TNumericLimits<float>::Max())
{
    TMap<FERCellId,float> Out;
    for(int32 P=0;P<W.GetPanels().Num();++P)if(W.GetPanels()[P].Id/8==Wall)
        for(int32 Z=Z0;Z<=Z1;++Z)for(int32 Y=0;Y<W.GetPanels()[P].Dims.Y;++Y)for(int32 X=X0;X<=X1;++X)
            if(!bRing || X==X0 || X==X1 || Z==Z0 || Z==Z1)Out.Add({P,X,Y,Z},Amount);
    return Out;
}
// The breach keeps a 25cm stub (cells Z 0..4): a remote pawn steps over it and is
// then teleported onto it, so server corrections are relative to a cell chunk.
bool StandsOnStub(const AERProceduralMap* Map,const AERCharacter* C)
{
    FERCellId Id;const auto* Move=C->GetCharacterMovement();
    const int32 Panel=WallPanel(Map->GetCellWorld(),5,true);
    const FVector Stub=WorldCell(Map,{Panel,45,0,2});
    return Move->IsMovingOnGround() && Map->ResolveCell(Move->CurrentFloor.HitResult,Id) && Map->GetCellWorld().GetPanels()[Id.Panel].Id/8==5 &&
        Id.Z<=4 && Id.X>=30 && Id.X<=59 && FVector::Dist2D(C->GetActorLocation(),Stub)<30;
}
}
AERBlockNetworkProbe::AERBlockNetworkProbe()
{
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;SetNetUpdateFrequency(20);
}
void AERBlockNetworkProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AERBlockNetworkProbe,Map);DOREPLIFETIME(AERBlockNetworkProbe,Phase);
    DOREPLIFETIME(AERBlockNetworkProbe,ExpectedEpoch);DOREPLIFETIME(AERBlockNetworkProbe,ExpectedSequence);DOREPLIFETIME(AERBlockNetworkProbe,ExpectedDigest);
    DOREPLIFETIME(AERBlockNetworkProbe,ShotPanel);DOREPLIFETIME(AERBlockNetworkProbe,ShotCell);DOREPLIFETIME(AERBlockNetworkProbe,FireFrom);DOREPLIFETIME(AERBlockNetworkProbe,WalkEnd);DOREPLIFETIME(AERBlockNetworkProbe,FireAim);DOREPLIFETIME(AERBlockNetworkProbe,StairPath);
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
    if(!Map || !Map->ValidateCells()){UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_PARITY_MISMATCH chunks_invalid"));return false;}
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ERBlockNetworkParity),false);FHitResult Hit;
    // Cell parity only; a pawn standing in the breach must not count as a cell.
    for(TActorIterator<ACharacter> It(GetWorld());It;++It)Q.AddIgnoredActor(*It);
    const auto& W=Map->GetCellWorld();
    for(int32 P=0;P<W.GetPanels().Num();++P)
    {
        const auto& Panel=W.GetPanels()[P];
        if(Panel.Dims.X<3 || Panel.Dims.Z<3)continue;
        for(int32 K=0;K<4;++K)
        {
            // Outer-layer interior cells, deterministic per panel.
            const FERCellId Id{P,1+(K*7919+P*31)%(Panel.Dims.X-2),K%2 ? 0:Panel.Dims.Y-1,1+(K*104729+P*17)%(Panel.Dims.Z-2)};
            const FVector N=Inward(Map,P)*(Id.Y==0 ? -1:1),C=WorldCell(Map,Id),Face=C+N*Panel.Cell.Y*.5;
            // Faces resting against another panel (branch walls, returns) are not exposed.
            const FVector Start=Map->GetActorTransform().InverseTransformPosition(Face+N*.5);
            bool bCovered=false;
            for(int32 O=0;O<W.GetPanels().Num() && !bCovered;++O)bCovered=O!=P && W.GetPanels()[O].Bounds.ExpandBy(.01).IsInside(Start);
            if(bCovered)continue;
            const bool Found=GetWorld()->LineTraceSingleByChannel(Hit,Face+N*.5,C,ECC_GameTraceChannel1,Q);
            FERCellId Resolved;
            // An absent cell may show another panel behind it (a window pane in a board's
            // plane): only collision of the cell's own panel would be a ghost.
            const bool bGhost=Found && Map->ResolveCell(Hit,Resolved) && Resolved.Panel==P;
            if(W.IsAlive(Id) ? !(Found && Map->ResolveCell(Hit,Resolved) && Resolved==Id):bGhost)
            {
                UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_PARITY_MISMATCH panel=%d id=%d cell=%d,%d,%d alive=%d found=%d hit=%s at=%s"),P,Panel.Id,Id.X,Id.Y,Id.Z,W.IsAlive(Id),Found,*GetNameSafe(Hit.GetComponent()),*Hit.ImpactPoint.ToString());
                return false;
            }
        }
    }
    return true;
}
void AERBlockNetworkProbe::ClientTick(float DeltaSeconds)
{
    if(Role.IsEmpty())FParse::Value(FCommandLine::Get(),TEXT("ERBlockNetRole="),Role);
    auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()):nullptr;
    // The join-time default name can arrive after ours: resend until the host shows the role.
    const FString Wanted=Role==TEXT("initial")?TEXT("Initial"):Role==TEXT("late")?TEXT("Late"):TEXT("Dense");
    if(PC && PC->PlayerState && !Role.IsEmpty() && PC->PlayerState->GetPlayerName()!=Wanted && GetWorld()->GetTimeSeconds()>NextName){PC->ServerSetName(Wanted);NextName=GetWorld()->GetTimeSeconds()+1;}
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
                TMap<FERCellId,float> Forged;Forged.Add({ShotPanel,ShotCell.X,ShotCell.Y,ShotCell.Z},10000);
                Passed&=Map->ApplyCellBatch(Forged).Damaged==0 && !Map->Regenerate();
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
        if(Phase==5 && InitialRole)
        {
            // Give the teleport correction time to arrive (40 ms lag under --loss).
            if(PhaseElapsed<1.5f)return;
            const auto* Base=C->GetCharacterMovement()->GetMovementBase();
            const bool OnStub=StandsOnStub(Map,C);Passed&=OnStub;
            UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_CLIENT_%s phase=5 role=%s block_movement_base pos=%s base=%s"),OnStub ? TEXT("PASS"):TEXT("FAIL"),*Role,*C->GetActorLocation().ToString(),Base ? *Base->GetName():TEXT("none"));
        }
        const auto& W=Map->GetCellWorld();
        if(Phase==5)Passed&=W.GetRemovedCount()>0;
        if(Phase==6 || Phase==10)Passed&=W.GetRemovedCount()==0;
        if(Phase==9)Passed&=W.GetRemovedCount()==W.GetCellCount();
        if(Phase==12)Passed&=W.GetRemovedCount()>0;
        if((Role==TEXT("late") && Phase==5) || (Role==TEXT("dense") && Phase==9))
        {int32 Alive=0,Physics=0;Map->GetDebrisCounts(Alive,Physics);Passed&=Alive==0;}
        PC->ServerBlockProbeReport(this,Phase,ExpectedEpoch,ExpectedSequence,ExpectedDigest,Passed);Reports.Add(FString::FromInt(Phase));
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_CLIENT_%s phase=%d role=%s epoch=%u digest=%u bodies=%d removed=%d"),Passed ? TEXT("PASS"):TEXT("FAIL"),Phase,*Role,ExpectedEpoch,ExpectedDigest,Map->GetMemoryProfile().CollisionBodies,int32(Map->GetCellWorld().GetRemovedCount()));
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
        const auto& W=Map->GetCellWorld();
        ShotPanel=WallPanel(W,5,true);
        if(ShotPanel==INDEX_NONE){Check(false,TEXT("shot_fixture"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
        ShotCell=FIntVector(25,W.GetPanels()[ShotPanel].Dims.Y-1,32);
        const FVector N=Inward(Map,ShotPanel);
        FireFrom=WorldCell(Map,{ShotPanel,ShotCell.X,ShotCell.Y,ShotCell.Z})+N*150;FireFrom.Z=100;FireAim=(-N).Rotation();Place(Initial.Get(),FireFrom,FireAim);
        // A band around the aim 1 HP from removal: real spread on 5cm cells still breaks one.
        TMap<FERCellId,float> Prepare=Region(W,5,15,35,26,38,false);
        for(auto& Pair:Prepare)Pair.Value=W.GetHealth(Pair.Key)-1;
        Map->ApplyCellBatch(Prepare);RemovedBefore=W.GetRemovedCount();
        ShotsBefore=CastChecked<AERCharacter>(Initial->GetPawn())->Weapon->AuthorizedShotCount;
        Check(Map->LayoutConfig.Floors==2 && Map->LayoutConfig.AdditionalMaterials && Map->LayoutConfig.WindowGlass,TEXT("two_floor_network_fixture"));Advance(1);return;
    }
    if(GetWorld()->GetTimeSeconds()>Deadline)
    {Check(false,TEXT("phase_timeout"));UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_PROBE_COMPLETE result=FAIL"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
    auto* C=Initial.IsValid()?Cast<AERCharacter>(Initial->GetPawn()):nullptr;
    if(!C || !C->Weapon)return;
    if(Phase==1 && Reported(TEXT("Initial"),1) && C->Weapon->AuthorizedShotCount>ShotsBefore)
    {
        const auto& W=Map->GetCellWorld();
        Check(C->Weapon->AuthorizedShotCount==ShotsBefore+1 && W.GetRemovedCount()>RemovedBefore,TEXT("remote_authorized_weapon_destroys_cells"));
        Map->ApplyCellBatch(Region(W,5,30,59,5,44,false));
        auto P=WorldCell(Map,{WallPanel(W,5,true),45,0,17});P.Z=100;FireFrom=P-FVector(100,0,0);WalkEnd=P+FVector(100,0,0);FireAim=FRotator::ZeroRotator;
        Place(Initial.Get(),FireFrom,FireAim);Place(Host.Get(),P+FVector(220,0,0),FRotator(0,180,0));{auto* H=CastChecked<AERCharacter>(Host->GetPawn())->Health.Get();H->ProtectionEndServerTime=0;H->Health=100;H->Armor=100;}
        ShotsBefore=C->Weapon->AuthorizedShotCount;Advance(2);
    }
    else if(Phase==2 && Reported(TEXT("Initial"),2) && C->Weapon->AuthorizedShotCount>ShotsBefore)
    {
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_BREACH_SHOT host_health=%.1f %s last=%s"),CastChecked<AERCharacter>(Host->GetPawn())->Health->Health,*Map->LastCombat,*Map->LastHit);
        // One torso rifle hit with the reference loadout's kevlar and source range falloff.
        const auto* Gun=C->Weapon.Get();float Armor=100;
        const float Expected=ERWeaponBallistics::AbsorbArmor(ERWeaponBallistics::AtRange(Gun->Data->Damage,Gun->Data->RangeModifier,
            FVector::Distance(Gun->LastShotTraceStart,Gun->LastShotTraceEnd)),Gun->Data->ArmorRatio,Armor);
        Check(FMath::IsNearlyEqual(CastChecked<AERCharacter>(Host->GetPawn())->Health->Health,100-Expected,.05f),TEXT("remote_hitscan_through_replicated_breach"));Advance(3);
    }
    else if(Phase==3 && Reported(TEXT("Initial"),3))
    {
        Check(FVector::Dist2D(C->GetActorLocation(),WalkEnd)<60 && C->GetActorLocation().Z>65,TEXT("authoritative_remote_breach_crossing"));
        const auto& W=Map->GetCellWorld();
        ExpectedDetached=W.GetDetachedCount();
        for(int32 P=0;P<W.GetPanels().Num();++P)if(W.GetPanels()[P].Id/8==7)ExpectedDetached+=39*24*W.GetPanels()[P].Dims.Y;
        Map->ApplyCellBatch(Region(W,7,30,70,25,50,true));
        const FVector Stub=WorldCell(Map,{WallPanel(W,5,true),45,0,2});
        Place(Initial.Get(),FVector(Stub.X,Stub.Y,Map->GetActorLocation().Z+25+C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2),FRotator::ZeroRotator);Advance(4);
    }
    else if(Phase==4 && !Map->IsSupportPending())
    {Check(Map->GetCellWorld().GetDetachedCount()==ExpectedDetached && CollisionParity(),TEXT("authority_collapse_and_collision"));Advance(5,45);}
    else if(Phase==5)
    {
        if(Reported(TEXT("Initial"),5) && !LateAnnounced){LateAnnounced=true;UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_READY_FOR_LATE_JOIN"));}
        if(!Reported(TEXT("Initial"),5) || !Reported(TEXT("Late"),5))return;
        Check(Late.IsValid() && Map->ValidateCells(),TEXT("late_join_matches_damaged_collapsed_snapshot"));
        Check(StandsOnStub(Map,C),TEXT("authoritative_remote_rests_on_cell_chunk"));
        Map->LayoutConfig.Seed=7919;Map->LayoutConfig.Floors=3;Check(Map->Regenerate(),TEXT("authority_epoch_regeneration"));Advance(6,30);
    }
    else if(Phase==6 && Reported(TEXT("Initial"),6) && Reported(TEXT("Late"),6))
    {
        Check(Map->GetCellWorld().GetRemovedCount()==0 && Map->LayoutConfig.Floors==3,TEXT("both_clients_new_epoch_no_stale_deltas"));
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
        // Every chunk modified: the densest late-join snapshot, applied panel by panel.
        const auto& W=Map->GetCellWorld();
        for(int32 P=0;P<W.GetPanels().Num();++P)
        {
            TMap<FERCellId,float> All;const auto& Panel=W.GetPanels()[P];
            for(int32 Z=0;Z<Panel.Dims.Z;++Z)for(int32 Y=0;Y<Panel.Dims.Y;++Y)for(int32 X=0;X<Panel.Dims.X;++X)if(W.IsAlive({P,X,Y,Z}))All.Add({P,X,Y,Z},TNumericLimits<float>::Max());
            Map->ApplyCellBatch(All);
        }
        Advance(9,60);UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_READY_FOR_DENSE_JOIN cells=%lld chunks=%d"),W.GetCellCount(),W.GetChunks().Num());
    }
    else if(Phase==9 && Reported(TEXT("Initial"),9) && Reported(TEXT("Late"),9) && Reported(TEXT("Dense"),9))
    {Check(Map->GetCellWorld().GetRemovedCount()==Map->GetCellWorld().GetCellCount() && Dense.IsValid(),TEXT("dense_late_join_full_removed_map"));Check(Map->Regenerate(),TEXT("second_epoch_regeneration"));Advance(10,40);}
    else if(Phase==10 && Reported(TEXT("Initial"),10) && Reported(TEXT("Late"),10) && Reported(TEXT("Dense"),10))
    {
        Check(CollisionParity() && Map->GetCellWorld().GetRemovedCount()==0,TEXT("three_clients_restore_dense_snapshot"));
        const int32 Panel=WallPanel(Map->GetCellWorld(),5,true);
        const auto Blast=AERBlockRoom::ApplyWorldExplosion(GetWorld(),WorldCell(Map,{Panel,45,0,17})+Inward(Map,Panel)*20,250,2400);
        Check(Blast.Damaged>10 && Blast.Destroyed>0,TEXT("authority_explosion_publishes_batched_state"));Advance(11,30);
    }
    else if(Phase==11 && !Map->IsSupportPending()){Advance(12,30);}
    else if(Phase==12 && Reported(TEXT("Initial"),12) && Reported(TEXT("Late"),12) && Reported(TEXT("Dense"),12))
    {
        Check(CollisionParity(),TEXT("three_clients_match_explosion_and_support_delta"));
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_PROBE_COMPLETE result=%s epoch=%u cells=%lld chunks=%d shards=%d"),Failed ? TEXT("FAIL"):TEXT("PASS"),Map->GetMapEpoch(),Map->GetCellWorld().GetCellCount(),Map->GetChunkCount(),Map->GetNetworkPageCount());
        FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);Phase=13;
    }
#endif
}
