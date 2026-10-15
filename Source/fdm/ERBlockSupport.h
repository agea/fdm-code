#pragma once
#include "CoreMinimal.h"
#include "ERBlockWorld.h"

// Geometric face contacts in room-local space. No instance indices, physics
// queries or frame-dependent order enter structural decisions.
class FERBlockSupportGraph
{
public:
    bool Build(const TArray<FERBlockRecord>& Blocks,bool GroundAnchors=true,float GroundZ=0);
    void Start();
    // Work and time budgets include anchor seeding, flood edges and components.
    // During a job geometry only loses blocks: supported reachability can be an
    // overestimate after new damage, but an unsupported result stays safe.
    bool Step(const TArray<FERBlockRecord>& Blocks,int32 WorkBudget,double TimeBudgetMs);
    const TArray<int32>& GetUnsupported() const { return Unsupported; }
    int32 GetComponentCount() const { return Components; }
    bool IsReady() const { return Phase==EPhase::Done; }
    SIZE_T GetAllocatedBytes() const;
    void Reset();
    double BuildMs=0, LastStepMs=0;
    int32 LastWork=0;
private:
    enum class EPhase:uint8 { Idle, Seed, AnchoredFlood, FindComponent, UnsupportedFlood, Done };
    EPhase Phase=EPhase::Idle;
    TArray<TArray<int32>> Neighbours;
    TBitArray<> Anchors;
    TArray<uint8> Visited;
    TArray<int32> Queue,Unsupported;
    int32 Cursor=0,Head=0,Edge=0,Components=0;
};
