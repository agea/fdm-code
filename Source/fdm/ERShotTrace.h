#pragma once
#include "CoreMinimal.h"
namespace ERShotTrace
{
// Existing anatomical physics bodies, with the world queried first for cover.
bool Pellet(UWorld* World, FHitResult& Hit, const FVector& Start, const FVector& End, const FCollisionQueryParams& Query);
}
