#include "ERSpawnRisk.h"
#include "ERCharacter.h"
#include "ERClaymore.h"
#include "ERServerSettings.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void FERSpawnMemory::Add(TArray<FERSpawnEvent>& Events,FVector Location,double Time)
{
    if(Events.Num()>=32)Events.RemoveAt(0);
    Events.Add({Location,Time});
}
void FERSpawnMemory::Reset()
{
    for(int32 Team=0;Team<2;++Team){Deaths[Team].Reset();Starts[Team].Reset();}
}
bool ERSpawnRisk::Enabled(const UWorld* World){return ERServer::Get(World,"Health.SpawnScoring",1)>.5f;}
FERSpawnRiskTuning ERSpawnRisk::Tuning(const UWorld* World)
{
    FERSpawnRiskTuning T;
    // km/h to cm/s. A floor keeps proximity meaningful with protection disabled.
    const float Sprint=ERServer::Get(World,"Move.Sprint",27)/3.6f*100;
    T.NearDistance=Sprint*FMath::Max(ERServer::Get(World,"Health.Protection",4),1.f);
    return T;
}
void ERSpawnRisk::GatherThreats(UWorld* World,EERTeam Team,TArray<FERSpawnThreat>& Threats,TArray<FERSpawnHazard>& Hazards)
{
    for(TActorIterator<AERCharacter> It(World);It;++It)
    {
        const auto* State=It->GetPlayerState<AERPlayerState>();
        if(!State || State->Team==Team || State->Team==EERTeam::None || !It->Health || It->Health->bDead)continue;
        Threats.Add({It->Camera ? It->Camera->GetComponentLocation():It->GetActorLocation()+FVector(0,0,64)});
    }
    for(TActorIterator<AERClaymore> It(World);It;++It)
        if(It->PlacedTeam!=uint8(Team) && (It->State==EERClaymoreState::Arming || It->State==EERClaymoreState::Armed))
            Hazards.Add({It->GetActorLocation(),It->GetActorForwardVector(),It->DamageRadius,It->ConeHalfAngle});
}
int32 ERSpawnRisk::Score(UWorld* World,TArray<FERSpawnCandidate>& Candidates,const TArray<FERSpawnThreat>& Threats,const TArray<FERSpawnHazard>& Hazards,
    const TArray<FERSpawnEvent>& Deaths,const TArray<FERSpawnEvent>& Starts,double Now,const FERSpawnRiskTuning& T)
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERSpawnRisk),false);
    if(World)for(TActorIterator<APawn> Pawn(World);Pawn;++Pawn)Query.AddIgnoredActor(*Pawn);
    int32 Traces=0;
    const int32 PerCandidate=T.MaxTraces/FMath::Max(1,Candidates.Num());
    const FVector Samples[]={FVector(0,0,T.EyeOffset),FVector(0,0,10)};
    auto Recent=[&](const TArray<FERSpawnEvent>& Events,FVector Location,float Radius)
    {
        float Weight=0;
        for(const auto& Event:Events)if(FVector::DistSquared(Event.Location,Location)<=Radius*Radius)
            Weight=FMath::Max(Weight,1-float(Now-Event.Time)/T.MemorySeconds);
        return Weight;
    };
    for(auto& C:Candidates)
    {
        C.Risk=0;C.Seen=0;C.bInHazard=false;
        // Nearest enemies first: the budget ends on the least urgent traces.
        TArray<const FERSpawnThreat*> Nearest;for(const auto& Threat:Threats)Nearest.Add(&Threat);
        Nearest.Sort([&](const FERSpawnThreat& A,const FERSpawnThreat& B){return FVector::DistSquared(A.Eye,C.Location)<FVector::DistSquared(B.Eye,C.Location);});
        int32 Spent=0;float Proximity=0;
        for(const auto* Threat:Nearest)
        {
            // Straight distance ignores walls, so it can only overstate how close an enemy is.
            Proximity+=FMath::Square(FMath::Max(0.f,1-float(FVector::Dist(Threat->Eye,C.Location))/T.NearDistance));
            bool Open=false;
            for(const FVector& Sample:Samples)
            {
                if(!World || Open || Spent>=PerCandidate)break;
                ++Spent;FHitResult Hit;
                Open=!World->LineTraceSingleByChannel(Hit,Threat->Eye,C.Location+Sample,ECC_GameTraceChannel1,Query);
            }
            C.Seen+=Open;
        }
        Traces+=Spent;
        C.Risk+=C.Seen*T.SightPenalty+FMath::Min(Proximity,2.f)*T.ProximityPenalty;
        for(const auto& H:Hazards)
        {
            const FVector Delta=C.Location-H.Location;
            if(Delta.SizeSquared()<=H.Radius*H.Radius && FVector::DotProduct(Delta.GetSafeNormal(),H.Forward)>=FMath::Cos(FMath::DegreesToRadians(H.HalfAngle)))C.bInHazard=true;
        }
        C.Risk+=C.bInHazard ? T.HazardPenalty:0;
        C.Risk+=Recent(Deaths,C.Location,T.SectorRadius)*T.DeathPenalty+Recent(Starts,C.Location,10)*T.RepeatPenalty;
    }
    return Traces;
}
int32 ERSpawnRisk::Pick(const TArray<FERSpawnCandidate>& Candidates,const FERSpawnRiskTuning& T,TFunctionRef<int32(int32)> Random)
{
    if(Candidates.IsEmpty())return INDEX_NONE;
    float Best=MAX_flt;for(const auto& C:Candidates)Best=FMath::Min(Best,C.Risk);
    TArray<int32> Tied;
    for(int32 I=0;I<Candidates.Num();++I)if(Candidates[I].Risk<=Best+T.Comparable)Tied.Add(I);
    return Tied[Random(Tied.Num())];
}
