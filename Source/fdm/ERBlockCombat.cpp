#include "ERBlockCombat.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ERBlockRoom.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool ERBlockCombat::IntersectBlock(const FTransform& Transform, const FVector& Start, const FVector& UnitDirection,
    double MaxDistance, double& EntryDistance, double& ExitDistance)
{
    EntryDistance = 0; ExitDistance = 0;
    if (Transform.ContainsNaN() || Start.ContainsNaN() || UnitDirection.ContainsNaN() ||
        !FMath::IsFinite(MaxDistance) || MaxDistance <= 0 || UnitDirection.IsNearlyZero() ||
        Transform.GetScale3D().GetAbs().GetMin() < .0001) return false;
    const FVector Origin = Transform.InverseTransformPosition(Start);
    const FVector Ray = Transform.InverseTransformVector(UnitDirection);
    double Near = -TNumericLimits<double>::Max(), Far = TNumericLimits<double>::Max();
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        if (FMath::Abs(Ray[Axis]) < 1.e-10)
        {
            if (Origin[Axis] < -50 || Origin[Axis] > 50) return false;
            continue;
        }
        double A = (-50 - Origin[Axis]) / Ray[Axis], B = (50 - Origin[Axis]) / Ray[Axis];
        if (A > B) Swap(A, B);
        Near = FMath::Max(Near, A); Far = FMath::Min(Far, B);
        if (Far < Near) return false;
    }
    if (Far <= 0 || Near > MaxDistance || !FMath::IsFinite(Far)) return false;
    EntryDistance = FMath::Max(0., Near); ExitDistance = Far;
    return true;
}
FERBlockShotResult ERBlockCombat::ContinueShot(UWorld* World, const FHitResult& InitialHit, const FVector& Start,
    const FVector& Direction, float Range, float Damage, const FERBlockShotSettings& Settings,
    const FCollisionQueryParams& Query, AController* Instigator, AActor* Causer)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_ContinueShot);
    const double StartTime=FPlatformTime::Seconds();
    FERBlockShotResult Result;
    Result.End = InitialHit.ImpactPoint;
    Result.TerminalHit = InitialHit;
    if (!World || World->GetNetMode() == NM_Client || Start.ContainsNaN() || Direction.ContainsNaN() || Direction.IsNearlyZero() ||
        !FMath::IsFinite(Range) || Range <= 0 || !FMath::IsFinite(Damage) || Damage <= 0 ||
        !FMath::IsFinite(Settings.Energy) || Settings.Energy <= 0 || !FMath::IsFinite(Settings.MinImpactCosine)) return Result;
    const FVector Ray = Direction.GetSafeNormal();
    const int32 Limit = FMath::Clamp(Settings.MaxPenetrations, 1, 16);
    const float MinCos = FMath::Clamp(Settings.MinImpactCosine, .01f, 1.f);
    Result.RemainingEnergy = Settings.Energy;
    FHitResult Hit = InitialHit;
    FVector Cursor = Start;
    double RemainingRange = Range;
    TArray<TPair<AERBlockRoom*, FERBlockId>> Visited;
    TArray<AERBlockRoom*> Rooms;
    while (Hit.bBlockingHit)
    {
        Result.End = Hit.ImpactPoint; Result.TerminalHit = Hit;
        auto* Room = Cast<AERBlockRoom>(Hit.GetActor());
        FERBlockId Id;
        if (!Room || !Room->CanMutateBlocks() || !Room->ResolveHit(Hit, Id))
        {
            Result.TerminalDamage = UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage * Result.RemainingEnergy / Settings.Energy, Ray, Hit, Instigator, Causer, UDamageType::StaticClass());
            break;
        }
        if (Result.Penetrations >= Limit) { Result.bLimited = true; break; }
        if (Visited.ContainsByPredicate([&](const auto& Pair) { return Pair.Key == Room && Pair.Value == Id; }))
        { Result.bLimited = true; break; }
        Visited.Emplace(Room, Id); Rooms.AddUnique(Room); ++Result.BlockHits;
        const auto* Block = Room->BlockWorld->Find(Id);
        const auto* Profile = Room->MaterialCatalog ? Room->MaterialCatalog->Find(Block->Material) : nullptr;
        if (!Profile) break;
        const float HitDamage = Damage * (Result.RemainingEnergy / Settings.Energy) * Profile->BulletDamageScale;
        double Entry = 0, Exit = 0;
        const bool bVolume = IntersectBlock(Room->GetBlockTransform(Id), Cursor, Ray, RemainingRange, Entry, Exit);
        // Resolve ID before any rebuild: FHitResult.Item becomes stale on removal.
        Room->ApplyBlockDamage(Id, HitDamage);
        Room->SpawnBlockImpact(Id, Hit.ImpactPoint, Hit.ImpactNormal);
        const double Cost = bVolume ? Profile->EntryEnergyCost + (Exit - Entry) * Profile->EnergyCostPerCm : TNumericLimits<double>::Max();
        const bool bGrazing = !Hit.bStartPenetrating && FMath::Abs(FVector::DotProduct(Ray, Hit.ImpactNormal)) < MinCos;
        if (!bVolume || bGrazing || !FMath::IsFinite(Cost) || Cost >= Result.RemainingEnergy || Exit + .1 >= RemainingRange) break;
        // The actual volume exit is used even if the block survived. Never ignore
        // a whole room/component: subsequent blocks still stop the same bullet.
        Result.RemainingEnergy = float(Result.RemainingEnergy - Cost);
        ++Result.Penetrations;
        Cursor += Ray * (Exit + .1); RemainingRange -= Exit + .1;
        Result.End = Cursor + Ray * RemainingRange;
        Result.TerminalHit = FHitResult();
        ++Result.ExtraTraces;
        if (!World->LineTraceSingleByChannel(Hit, Cursor, Result.End, ECC_GameTraceChannel1, Query)) break;
    }
    for (auto* Room : Rooms)
        Room->LastCombat = FString::Printf(TEXT("shot hits=%d penetrations=%d energy=%.1f limited=%d target=%s damage=%.1f"), Result.BlockHits, Result.Penetrations, Result.RemainingEnergy, Result.bLimited, Result.TerminalHit.GetActor() && !Result.TerminalHit.GetActor()->IsA<AERBlockRoom>() ? *Result.TerminalHit.GetActor()->GetName() : TEXT("none"), Result.TerminalDamage);
    Result.TotalMs=(FPlatformTime::Seconds()-StartTime)*1000;
    return Result;
}
