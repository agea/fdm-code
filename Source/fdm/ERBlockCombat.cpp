#include "ERBlockCombat.h"
#include "ERShotTrace.h"
#include "ERWeapon.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ERBlockRoom.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool FERBlockExplosionProfile::IsValid() const
{
    for (float Value : {BaseDamage, MinimumDamage, InnerRadius, OuterRadius, FalloffExponent})
        if (!FMath::IsFinite(Value) || Value < 0) return false;
    return OuterRadius > 0 && OuterRadius <= 5000 && InnerRadius < OuterRadius && MinimumDamage <= BaseDamage &&
        FalloffExponent > 0 && MaxShieldingLayers >= 0;
}
double ERBlockCombat::ExplosionFalloff(const FERBlockExplosionProfile& Profile, double Distance)
{
    if (!Profile.IsValid() || !FMath::IsFinite(Distance) || Distance >= Profile.OuterRadius) return 0;
    if (Distance <= Profile.InnerRadius) return 1;
    return FMath::Pow((Profile.OuterRadius - Distance) / (Profile.OuterRadius - Profile.InnerRadius), double(Profile.FalloffExponent));
}
double ERBlockCombat::ExplosionCellDamage(const FERBlockExplosionProfile& Profile, double Distance, float Multiplier, double Exposure)
{
    if (!Profile.IsValid() || !FMath::IsFinite(Distance) || Distance >= Profile.OuterRadius ||
        !FMath::IsFinite(Multiplier) || Multiplier <= 0 || !FMath::IsFinite(Exposure) || Exposure <= 0) return 0;
    const double Falloff = ExplosionFalloff(Profile, Distance);
    // Each cell receives the local value; the base is not a total to divide.
    const double Local = Profile.MinimumDamage + (double(Profile.BaseDamage) - Profile.MinimumDamage) * Falloff;
    return Local * Multiplier * FMath::Min(1., Exposure);
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
        !FMath::IsFinite(Settings.Energy) || Settings.Energy <= 0 || !FMath::IsFinite(Settings.MinImpactCosine) ||
        !FMath::IsFinite(Settings.GetStructuralDamage(Damage)) || Settings.GetStructuralDamage(Damage) < 0) return Result;
    const FVector Ray = Direction.GetSafeNormal();
    const float Structural = Settings.GetStructuralDamage(Damage);
    const int32 Limit = FMath::Clamp(Settings.MaxPenetrations, 1, 16);
    Result.RemainingEnergy = Settings.Energy;
    FHitResult Hit = InitialHit;
    FVector Cursor = Start;
    double RemainingRange = Range;
    TArray<AERBlockRoom*> Rooms;
    int32 CellSteps = 0;
    while (Hit.bBlockingHit)
    {
        Result.End = Hit.ImpactPoint; Result.TerminalHit = Hit;
        auto* Room = Cast<AERBlockRoom>(Hit.GetActor());
        if (Room && Room->CanMutateBlocks())
        {
            // 5cm cells: one voxel traversal per panel, entry cost once per material run.
            if (Result.Penetrations >= Limit || ++CellSteps > 4 * Limit) { Result.bLimited = true; break; }
            Rooms.AddUnique(Room);
            const auto Step = Room->ShootCells(Hit, Cursor, Ray, RemainingRange, Structural, Settings, Result.RemainingEnergy, Limit - Result.Penetrations);
            Result.BlockHits += Step.Cells; Result.bLimited |= Step.bLimited;
            if (Step.bStop) { Result.End = Step.Exit; break; }
            Result.Penetrations += Step.Runs;
            RemainingRange -= FVector::DotProduct(Step.Exit - Cursor, Ray); Cursor = Step.Exit;
            if (RemainingRange <= .1) break;
        }
        else
        {
            Result.TerminalDamage = UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage * Result.RemainingEnergy / Settings.Energy, Ray, Hit, Instigator, Causer, UDamageType::StaticClass());
            break;
        }
        Result.End = Cursor + Ray * RemainingRange;
        Result.TerminalHit = FHitResult();
        ++Result.ExtraTraces;
        const auto* Gun=Cast<AERWeapon>(Causer);
        const bool Anatomical=Gun && Gun->Data && Gun->Data->bUseBodyHitZones;
        if (!(Anatomical ? ERShotTrace::Pellet(World,Hit,Cursor,Result.End,Query) :
            World->LineTraceSingleByChannel(Hit, Cursor, Result.End, ECC_GameTraceChannel1, Query))) break;
    }
    for (auto* Room : Rooms)
        Room->LastCombat = FString::Printf(TEXT("shot hits=%d penetrations=%d energy=%.1f limited=%d target=%s damage=%.1f"), Result.BlockHits, Result.Penetrations, Result.RemainingEnergy, Result.bLimited, Result.TerminalHit.GetActor() && !Result.TerminalHit.GetActor()->IsA<AERBlockRoom>() ? *Result.TerminalHit.GetActor()->GetName() : TEXT("none"), Result.TerminalDamage);
    Result.TotalMs=(FPlatformTime::Seconds()-StartTime)*1000;
    return Result;
}
