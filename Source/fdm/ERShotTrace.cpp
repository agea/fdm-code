#include "ERShotTrace.h"
#include "ERCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

bool ERShotTrace::Pellet(UWorld* World,FHitResult& Hit,const FVector& Start,const FVector& End,const FCollisionQueryParams& Query)
{
    FCollisionQueryParams WorldQuery=Query;
    TArray<AERCharacter*,TInlineAllocator<24>> Targets;
    for(TActorIterator<AERCharacter> It(World);It;++It)
    {
        if(!Query.GetIgnoredSourceObjects().Contains(It->GetUniqueID()) && !It->Health->bDead) Targets.Add(*It);
        // Movement capsules are broad locomotion volumes, not bullet anatomy.
        WorldQuery.AddIgnoredActor(*It);
    }
    bool Found=World->LineTraceSingleByChannel(Hit,Start,End,ECC_GameTraceChannel1,WorldQuery);
    FVector Limit=Found ? Hit.ImpactPoint : End;
    double Nearest=FVector::DistSquared(Start,Limit);
    for(auto* Target:Targets)
    {
        FHitResult BodyHit;
        if(Target->GetMesh()->LineTraceComponent(BodyHit,Start,Limit,Query) &&
            FVector::DistSquared(Start,BodyHit.ImpactPoint)<Nearest)
        {
            Hit=BodyHit;Found=true;Limit=Hit.ImpactPoint;Nearest=FVector::DistSquared(Start,Limit);
        }
    }
    return Found;
}
