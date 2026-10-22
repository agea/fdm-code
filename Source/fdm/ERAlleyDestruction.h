#pragma once
#include "CoreMinimal.h"
#include "ERCellWorld.h"

// Geometry queries shared by opt-in native/game probes. Ordinary matches use
// the standard weapon/explosion paths and never invoke these controlled cuts.
class AERProceduralMap;
namespace ERAlleyDestruction
{
    struct FBreach { FVector Outside,Inside; FHitResult Hit; FERCellId Cell; };
    bool FindWall(AERProceduralMap* Map,FBreach& Out);
    FBox WallBounds(FVector Outside,FVector Inside,float Floor);
    int32 Cut(AERProceduralMap* Map,int32 Panel,const FBox& RoomBounds);
    int32 SeverSupports(AERProceduralMap* Map,int32 Panel);
    bool FindRoof(AERProceduralMap* Map,FVector& Point,FERCellId& Cell);
}
