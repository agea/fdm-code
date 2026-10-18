#pragma once
#include "CoreMinimal.h"

// CPU wall time for game-thread calls. ISM clear/add include synchronous Chaos
// work; deferred physics, render-thread work and GPU time are not separated here.
struct FERBlockOperationProfile
{
    double TotalMs=0, LogicalMs=0, GroupingMs=0, ClearMs=0, AddMs=0, MappingMs=0;
    int32 Groups=0, Uploaded=0;
    // 5cm cells: greedy face mesh and box collision of rebuilt chunks.
    double MeshMs=0, CollisionMs=0;
    int32 Quads=0, Boxes=0;
};
struct FERBlockMemoryProfile
{
    SIZE_T LogicalBytes=0, MappingBytes=0, LayoutBytes=0, InstanceResourceBytes=0;
    SIZE_T SupportBytes=0,NetworkBytes=0;
    int32 Debris=0, SimulatingDebris=0;
    uint64 ResidentBytes=0;
    int32 CollisionBodies=0, Components=0, Decals=0, Emitters=0, Bursts=0;
    int64 Cells=0, RemovedCells=0;
    int32 Quads=0, CollisionBoxes=0;
    SIZE_T TrackedBytes() const { return LogicalBytes+MappingBytes+LayoutBytes+InstanceResourceBytes+SupportBytes+NetworkBytes; }
};
