#include "ERBlockSupport.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
namespace
{
    constexpr double ContactTolerance=.01;
    bool AxisAligned(const FTransform& T)
    {
        const FQuat Q=T.GetRotation();
        for (const FVector Axis:{Q.GetAxisX(),Q.GetAxisY(),Q.GetAxisZ()})
            if (!FMath::IsNearlyEqual(Axis.GetAbs().GetMax(),1.,.00001)) return false;
        return true;
    }
    bool FaceContact(const FBox& A,const FBox& B)
    {
        // Exactly one touching axis and strictly positive overlap on the other
        // two. Edge/corner contact cannot transmit structural support.
        int32 Touch=0;
        for (int32 Axis=0;Axis<3;++Axis)
        {
            const double Overlap=FMath::Min(A.Max[Axis],B.Max[Axis])-FMath::Max(A.Min[Axis],B.Min[Axis]);
            if (Overlap<-ContactTolerance) return false;
            if (Overlap<=ContactTolerance) ++Touch;
        }
        return Touch==1;
    }
}
bool FERBlockSupportGraph::Build(const TArray<FERBlockRecord>& Blocks,bool GroundAnchors,float GroundZ)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_SupportBuild);
    const double Begin=FPlatformTime::Seconds();Reset();
    if (Blocks.Num()>50000 || !FMath::IsFinite(GroundZ)) return false;
    TArray<FBox> Bounds;Bounds.Reserve(Blocks.Num());
    double Cell=1;
    for (const auto& B:Blocks)
    {
        if (!AxisAligned(B.LocalTransform)) return false;
        const FBox Box=FBox(FVector(-50),FVector(50)).TransformBy(B.LocalTransform.ToMatrixWithScale());
        Bounds.Add(Box);Cell=FMath::Max(Cell,Box.GetSize().GetMax()+ContactTolerance*2);
    }
    TMap<FIntVector,TArray<int32>> Buckets;
    Neighbours.SetNum(Blocks.Num());Anchors.Init(false,Blocks.Num());
    for (int32 I=0;I<Blocks.Num();++I)
    {
        Anchors[I]=Blocks[I].bStructuralAnchor || Blocks[I].bIndestructible || (GroundAnchors && Bounds[I].Min.Z<=GroundZ+ContactTolerance);
        const FVector P=Bounds[I].GetCenter();
        Buckets.FindOrAdd(FIntVector(FMath::FloorToInt(P.X/Cell),FMath::FloorToInt(P.Y/Cell),FMath::FloorToInt(P.Z/Cell))).Add(I);
    }
    int32 Pairs=0;
    for (int32 I=0;I<Blocks.Num();++I)
    {
        const FVector P=Bounds[I].GetCenter();
        const FIntVector C(FMath::FloorToInt(P.X/Cell),FMath::FloorToInt(P.Y/Cell),FMath::FloorToInt(P.Z/Cell));
        for (int32 X=-1;X<=1;++X) for (int32 Y=-1;Y<=1;++Y) for (int32 Z=-1;Z<=1;++Z)
            if (const auto* Bucket=Buckets.Find(C+FIntVector(X,Y,Z)))
                for (int32 J:*Bucket) if (J>I)
                {
                    if (++Pairs>8000000) {Reset();return false;}
                    if (FaceContact(Bounds[I],Bounds[J])) {Neighbours[I].Add(J);Neighbours[J].Add(I);}
                }
    }
    for (auto& N:Neighbours) N.Sort();
    BuildMs=(FPlatformTime::Seconds()-Begin)*1000;return true;
}
void FERBlockSupportGraph::Start()
{
    Visited.Init(0,Neighbours.Num());Queue.Reset();Unsupported.Reset();
    Cursor=Head=Edge=Components=0;Phase=EPhase::Seed;
}
bool FERBlockSupportGraph::Step(const TArray<FERBlockRecord>& Blocks,int32 WorkBudget,double TimeBudgetMs)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_SupportStep);
    const double Begin=FPlatformTime::Seconds();LastWork=0;
    if (Blocks.Num()!=Neighbours.Num() || Phase==EPhase::Idle) return false;
    const int32 Limit=FMath::Clamp(WorkBudget,1,16384);
    const double Ms=FMath::Clamp(TimeBudgetMs,.01,4.);
    while (Phase!=EPhase::Done && LastWork<Limit)
    {
        if (LastWork%16==0 && (FPlatformTime::Seconds()-Begin)*1000>=Ms) break;
        ++LastWork;
        if (Phase==EPhase::Seed)
        {
            if (Cursor==Blocks.Num()) {Cursor=0;Phase=EPhase::AnchoredFlood;continue;}
            const int32 I=Cursor++;
            if (Anchors[I] && Blocks[I].IsAttached()) {Visited[I]=1;Queue.Add(I);}
        }
        else if (Phase==EPhase::AnchoredFlood || Phase==EPhase::UnsupportedFlood)
        {
            if (Head==Queue.Num())
            {
                Queue.Reset();Head=Edge=0;Phase=EPhase::FindComponent;continue;
            }
            const int32 I=Queue[Head];
            if (!Blocks[I].IsAttached() || Edge==Neighbours[I].Num()) {++Head;Edge=0;continue;}
            const int32 J=Neighbours[I][Edge++];
            if (!Visited[J] && Blocks[J].IsAttached())
            {
                Visited[J]=Phase==EPhase::AnchoredFlood ? 1 : 2;Queue.Add(J);
                if (Phase==EPhase::UnsupportedFlood) Unsupported.Add(J);
            }
        }
        else if (Phase==EPhase::FindComponent)
        {
            if (Cursor==Blocks.Num()) {Phase=EPhase::Done;continue;}
            const int32 I=Cursor++;
            if (!Visited[I] && Blocks[I].IsAttached())
            {
                ++Components;Visited[I]=2;Unsupported.Add(I);Queue.Add(I);Phase=EPhase::UnsupportedFlood;
            }
        }
    }
    LastStepMs=(FPlatformTime::Seconds()-Begin)*1000;
    return IsReady();
}
void FERBlockSupportGraph::Reset()
{
    Neighbours.Empty();Anchors.Empty();Visited.Empty();Queue.Empty();Unsupported.Empty();
    Cursor=Head=Edge=Components=0;Phase=EPhase::Idle;BuildMs=LastStepMs=0;LastWork=0;
}
SIZE_T FERBlockSupportGraph::GetAllocatedBytes() const
{
    SIZE_T Size=Neighbours.GetAllocatedSize()+Anchors.GetAllocatedSize()+Visited.GetAllocatedSize()+Queue.GetAllocatedSize()+Unsupported.GetAllocatedSize();
    for (const auto& N:Neighbours) Size+=N.GetAllocatedSize();return Size;
}
