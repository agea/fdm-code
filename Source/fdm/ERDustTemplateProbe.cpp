#include "ERDustTemplateProbe.h"
#include "ERProceduralMap.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UnrealClient.h"
#include "Misc/App.h"
#include "Misc/Paths.h"

AERDustTemplateProbe::AERDustTemplateProbe()
{
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;
}
void AERDustTemplateProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AERDustTemplateProbe,SnapshotPhase);DOREPLIFETIME(AERDustTemplateProbe,ExpectedDigest);DOREPLIFETIME(AERDustTemplateProbe,DeckPlayer);
}
void AERDustTemplateProbe::Check(bool Good,const TCHAR* Label)
{
    Failed|=!Good;UE_LOG(LogTemp,Display,TEXT("ER_DUST_%s %s"),Good ? TEXT("PASS"):TEXT("FAIL"),Label);
}
void AERDustTemplateProbe::Publish(int32 Next,AERProceduralMap* Map)
{
    Phase=Next;SnapshotPhase=Next;ExpectedDigest=Map->GetStateDigest();ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("ER_DUST_STATE phase=%d digest=%u epoch=%u"),Phase,ExpectedDigest,Map->GetMapEpoch());
}
// The Short A deck over the underpass is a BlockAll template FixedMeshes group.
// A remote pawn put there is corrected relative to it, so the client must
// resolve that runtime component by name.
static FVector DeckStand(const UWorld* World,const FVector& P)
{
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ERDustDeck),false);FHitResult Hit;
    for(TActorIterator<APawn> Pawn(World);Pawn;++Pawn)Q.AddIgnoredActor(*Pawn);
    return World->LineTraceSingleByChannel(Hit,P+FVector(0,0,500),P+FVector(0,0,200),ECC_WorldStatic,Q) ? Hit.ImpactPoint+FVector(0,0,94):P+FVector(0,0,414);
}
void AERDustTemplateProbe::PlaceRemote(bool Latest,const FVector& P)
{
    // Remote controllers in join order: the initial client first, the late one last.
    AERCharacter* Target=nullptr;
    for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
    {
        auto* C=It->Get() && !It->Get()->IsLocalController() ? Cast<AERCharacter>(It->Get()->GetPawn()):nullptr;
        if(C){Target=C;if(!Latest)break;}
    }
    if(!Target || !Target->GetPlayerState())return;
    auto* Move=Target->GetCharacterMovement();Target->TeleportTo(P,FRotator::ZeroRotator);Move->StopMovementImmediately();
    Move->SetMovementMode(MOVE_Walking);Move->ForceClientAdjustment();DeckPlayer=Target->GetPlayerState();ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("ER_DUST_REMOTE_PLACED name=%s pos=%s"),*DeckPlayer->GetPlayerName(),*P.ToString());
}
void AERDustTemplateProbe::Tick(float Delta)
{
    Super::Tick(Delta);
#if !UE_BUILD_SHIPPING
    Elapsed+=Delta;TActorIterator<AERProceduralMap> It(GetWorld());if(!It)return;
    auto* Map=*It;if(!Map->IsMapReady())return;
    const auto& L=Map->GetLayout();
    if(!HasAuthority())
    {
        if((SnapshotPhase==1 || SnapshotPhase==6 || SnapshotPhase==7) && SeenPhase!=SnapshotPhase && Map->IsSnapshotCurrent() && Map->GetStateDigest()==ExpectedDigest)
        {
            FCollisionQueryParams Q(SCENE_QUERY_STAT(ERDustPeer),false);FHitResult Hit;const FVector P=L.TownCrossing;
            // Sample replicated environment collision, excluding the host or
            // another pawn currently walking through the tested underpass.
            for(TActorIterator<APawn> Pawn(GetWorld());Pawn;++Pawn)Q.AddIgnoredActor(*Pawn);
            bool Good=Map->ValidateCells() && L.TownBuildings.Num()>=12 && L.TownGates.Num()==3;
            Good&=GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,500),P+FVector(0,0,200),ECC_WorldStatic,Q) && FMath::IsNearlyEqual(Hit.ImpactPoint.Z,320.f,1.f);
            Good&=!GetWorld()->SweepSingleByChannel(Hit,P+FVector(-500,0,100),P+FVector(500,0,100),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92.5),Q);
            UE_LOG(LogTemp,Display,TEXT("ER_DUST_CLIENT_%s phase=%d digest=%u epoch=%u"),Good ? TEXT("PASS"):TEXT("FAIL"),SnapshotPhase,ExpectedDigest,Map->GetMapEpoch());SeenPhase=SnapshotPhase;
        }
        // Phase 6 puts the initial client on the deck, phase 7 (after the seed rebuild) the late one.
        auto* PC=GetWorld()->GetFirstPlayerController();auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()):nullptr;
        if((SnapshotPhase==6 || SnapshotPhase==7) && SeenPhase==SnapshotPhase && BaseChecked!=SnapshotPhase && C && PC->PlayerState && DeckPlayer==PC->PlayerState)
        {
            if(BaseClock<0)BaseClock=Elapsed;
            const auto* Base=C->GetCharacterMovement()->GetMovementBase();const FVector Target=DeckStand(GetWorld(),L.TownCrossing);
            const bool On=Base && Base->GetOwner()==Map && C->GetCharacterMovement()->IsMovingOnGround() && FVector::Dist2D(C->GetActorLocation(),Target)<40 && FMath::Abs(C->GetActorLocation().Z-Target.Z)<30;
            if(On || Elapsed-BaseClock>10)
            {
                UE_LOG(LogTemp,Display,TEXT("ER_DUST_CLIENT_BASE_%s phase=%d name=%s base=%s pos=%s target=%s"),On ? TEXT("PASS"):TEXT("FAIL"),SnapshotPhase,*PC->PlayerState->GetPlayerName(),Base ? *Base->GetName():TEXT("none"),*C->GetActorLocation().ToString(),*Target.ToString());
                BaseChecked=SnapshotPhase;BaseClock=-1;
            }
        }
        return;
    }
    auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()):nullptr;
    if(!C || !C->Weapon || L.TownGates.Num()!=3){if(Elapsed>50)FPlatformMisc::RequestExitWithStatus(false,1);return;}
    auto* Move=C->GetCharacterMovement();
    auto Place=[&](FVector P,float Yaw){Move->StopMovementImmediately();C->TeleportTo(P,FRotator(0,Yaw,0));PC->SetControlRotation(FRotator(0,Yaw,0));Move->SetMovementMode(MOVE_Walking);};
    auto Capture=[&](const FString& Name){if(FApp::CanEverRender())FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing")/Name,true,false);};
    auto Follow=[&](const TArray<FVector>& Path,float Reach=38.f)
    {
        if(Waypoint>=Path.Num()){Move->StopMovementImmediately();return true;}
        const FVector Target=Path[Waypoint],Position=C->GetActorLocation();
        if(FVector::Dist2D(Position,Target)<Reach && FMath::Abs(Position.Z-Target.Z)<45){++Waypoint;return Waypoint>=Path.Num();}
        FVector Direction=Target-Position;Direction.Z=0;Direction.Normalize();
        C->AddMovementInput(Direction);PC->SetControlRotation(Direction.Rotation());return false;
    };
    if(Phase==0 && Elapsed>4)
    {
        Check(Map->ValidateCells() && L.TownBuildings.Num()>=12 && L.TownCourts.Num()==6,TEXT("plan_city_parcels_and_courts"));
        PC->SetIgnoreLookInput(true);
        const auto& G=L.TownGates[0];Place(G.Passage[0],G.Frame.Rotator().Yaw+90);
        Publish(1,Map);StageTime=Elapsed;
    }
    else if(Phase==1)
    {
        if(Elapsed-StageTime<1.6f)PC->SetControlRotation(FRotator(0,L.TownGates[0].Frame.Rotator().Yaw+90,0));
        // Give the player camera a rendered frame after teleporting before
        // capturing the gate; movement must not carry it through the leaves.
        if(!CapturedGate)
        {
            if(Elapsed-StageTime<.6f)return;
            Capture(TEXT("DustTown-WoodenGates.png"));CapturedGate=true;return;
        }
        // A screenshot can be consumed by the render thread after this tick.
        // Hold the frontal camera before starting the sideways approach.
        if(Elapsed-StageTime<1.6f)return;
        const auto& G=L.TownGates[GateIndex];
        if(Follow(G.Passage,10.f))
        {
            Check(true,TEXT("actual_pawn_passes_opposite_swing_arched_gate"));Waypoint=0;
            if(++GateIndex<3){const auto& Next=L.TownGates[GateIndex];Place(Next.Passage[0],Next.Frame.Rotator().Yaw+90);}
            else{Place(L.LowerRoute[0],0);Phase=2;StageTime=Elapsed;}
        }
        if(Elapsed-StageTime>18){Check(false,TEXT("arched_gate_timeout"));Phase=2;Waypoint=0;Place(L.LowerRoute[0],0);StageTime=Elapsed;}
    }
    else if(Phase==2)
    {
        if(Follow(L.LowerRoute) || Elapsed-StageTime>12)
        {
            Check(FVector::Dist2D(C->GetActorLocation(),L.LowerRoute.Last())<50 && C->GetActorLocation().Z<110,TEXT("actual_pawn_walks_CT_under_Short_A"));
            Capture(TEXT("DustTown-Underpass.png"));Place(L.UpperRoute[0],0);Waypoint=0;Phase=3;StageTime=Elapsed;
        }
    }
    else if(Phase==3)
    {
        if(!ReachedUpper && C->GetActorLocation().Z>408)
        {ReachedUpper=true;Capture(TEXT("DustTown-ShortA.png"));}
        if(Follow(L.UpperRoute) || Elapsed-StageTime>70)
        {
            Check(ReachedUpper && FVector::Dist2D(C->GetActorLocation(),L.UpperRoute.Last())<60 && C->GetActorLocation().Z<110,TEXT("actual_pawn_climbs_and_follows_bent_Short_A_to_A_court"));
            Move->StopMovementImmediately();Phase=4;StageTime=Elapsed;
        }
    }
    else if(Phase==4)
    {
        FCollisionQueryParams Q(SCENE_QUERY_STAT(ERDustShot),false,C);FHitResult Hit;FERCellId Id;bool Found=false;
        const auto& Cells=Map->GetCellWorld();
        for(int32 Index=0;Index<Cells.GetPanels().Num();++Index)
        {
            // Lower part of a side board of the project cover.
            const auto& Panel=Cells.GetPanels()[Index];const FERCellId Low{Index,Panel.Dims.X/2,0,FMath::Min(Panel.Dims.Z/2,10)};
            if(Panel.Transform.GetRotation().GetAxisZ().Z<.99 || !Cells.IsAlive(Low))continue;
            const FVector Target=Map->GetActorTransform().TransformPosition(Cells.GetCellCenter(Low));
            if(Target.Z>100)continue;
            for(const FVector D:{FVector(1,0,0),FVector(-1,0,0),FVector(0,1,0),FVector(0,-1,0)})
            {
                const FVector P(Target.X+D.X*300,Target.Y+D.Y*300,100);
                if(GetWorld()->OverlapBlockingTestByChannel(P,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92.5),Q))continue;
                Place(P,0);const FVector Eye=C->Camera->GetComponentLocation();PC->SetControlRotation((Target-Eye).Rotation());
                if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Target,ECC_GameTraceChannel1,Q) && Map->ResolveCell(Hit,Id)){Found=true;break;}
            }
            if(Found)break;
        }
        Check(Found,TEXT("project_court_cover_resolves_authoritative_block"));
        if(Found)
        {
            TMap<FERCellId,float> Prep;const auto& Panel=Cells.GetPanels()[Id.Panel];
            for(int32 Z=0;Z<Panel.Dims.Z;++Z)for(int32 Y=0;Y<Panel.Dims.Y;++Y)for(int32 X=0;X<Panel.Dims.X;++X)if(Cells.IsAlive({Id.Panel,X,Y,Z}))Prep.Add({Id.Panel,X,Y,Z},Cells.GetHealth({Id.Panel,X,Y,Z})-1);
            Map->ApplyCellBatch(Prep);
            const uint32 Before=C->Weapon->AuthorizedShotCount;C->Weapon->StartFire();C->Weapon->StopFire();
            Check(C->Weapon->AuthorizedShotCount==Before+1 && Map->GetCellWorld().GetRemovedCount()>0,TEXT("real_weapon_damages_project_wood"));
            AERBlockRoom::ApplyWorldExplosion(GetWorld(),Hit.ImpactPoint+Hit.ImpactNormal*30,220,2400,C);
        }
        Phase=5;StageTime=Elapsed;
    }
    else if(Phase==5 && Elapsed-StageTime>6)
    {
        Check(Map->ValidateCells() && Map->GetCellWorld().GetRemovedCount()>0,TEXT("court_damage_and_collapse_mappings"));Publish(6,Map);StageTime=Elapsed;
        if(FApp::CanEverRender())
        {
            auto* Camera=GetWorld()->SpawnActor<ACameraActor>(FVector(0,0,11000),FRotator(-90,-90,0));
            Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
            Camera->GetCameraComponent()->OrthoWidth=FMath::Max(L.ArenaSize.X,L.ArenaSize.Y*16/9)*1.08;
            PC->SetViewTarget(Camera);Capture(TEXT("DustTown-Plan-42.png"));
        }
    }
    if((Phase==6 || Phase==7) && PlacedPhase!=Phase && Elapsed-StageTime>3)
    {PlaceRemote(Phase==7,DeckStand(GetWorld(),L.TownCrossing));PlacedPhase=Phase;}
    if(Phase==6 && Elapsed-StageTime>30)
    {
        Map->LayoutConfig.Seed=7919;Check(Map->Regenerate() && Map->GetCellWorld().GetRemovedCount()==0,TEXT("seed_rebuilds_city_and_restores_cover"));Publish(7,Map);StageTime=Elapsed;
        Capture(TEXT("DustTown-Plan-7919.png"));
    }
    else if(Phase==7 && Elapsed-StageTime>16)
    {
        UE_LOG(LogTemp,Display,TEXT("ER_DUST_COMPLETE result=%s"),Failed ? TEXT("FAIL"):TEXT("PASS"));
        FPlatformMisc::RequestExitWithStatus(false,Failed ? 1:0);Phase=8;
    }
#endif
}
