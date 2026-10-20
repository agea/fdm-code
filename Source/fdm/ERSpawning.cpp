#include "ERMatch.h"
#include "ERCharacter.h"
#include "ERBotController.h"
#include "ERProceduralMap.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"

namespace
{
const FName AdditionalStartTag(TEXT("ERAdditionalStart"));
}

void AERGameMode::PrepareTeamStarts()
{
    TActorIterator<AERProceduralMap> Map(GetWorld());
    if (Map)
    {
        if (PreparedSpawnMap == *Map && PreparedSpawnEpoch == Map->GetMapEpoch() && PreparedSpawnSeed == Map->LayoutConfig.Seed) return;
        FERProceduralLayout InitialLayout;
        const FERProceduralLayout* Layout = &Map->GetLayout();
        // HandleStartingNewPlayer may run before the map actor's BeginPlay.
        // Build at most once per seed/epoch, then reuse the real layout metadata.
        if (Map->GetMapEpoch() == 0)
        {
            if (!ERProcedural::Build(Map->LayoutConfig, Map->Config, InitialLayout, Map->CellSettings)) return;
            Layout = &InitialLayout;
        }
        if (Layout->AlphaSpawns.IsEmpty() || Layout->OmegaSpawns.IsEmpty()) return;
        for (const EERTeam Team : {EERTeam::Alpha, EERTeam::Omega})
        {
            TArray<AERTeamPlayerStart*> Starts;
            for (TActorIterator<AERTeamPlayerStart> Start(GetWorld()); Start; ++Start)
            {
                if (Start->Team != Team || Start->ActorHasTag(AdditionalStartTag)) continue;
                bool PrivateBotStart = false;
                for (const auto& Bot : TestBots) if (IsValid(Bot.Get()) && Bot->TrainingStart == *Start) PrivateBotStart = true;
                if (!PrivateBotStart) Starts.Add(*Start);
            }
            const auto& Frames = Team == EERTeam::Alpha ? Layout->AlphaSpawns : Layout->OmegaSpawns;
            while (Starts.Num() < Frames.Num())
            {
                auto* Start = GetWorld()->SpawnActor<AERTeamPlayerStart>();
                if (!Start) break;
                Start->Team = Team;
                Starts.Add(Start);
            }
            for (int32 I = 0; I < Starts.Num(); ++I)
            {
                Starts[I]->GetRootComponent()->SetMobility(EComponentMobility::Movable);
                Starts[I]->SetActorTransform(Frames[I % Frames.Num()] * Map->GetActorTransform());
            }
        }
        // Overflow positions belonged to the previous arena, even if its new
        // seed happened to preserve an old floor. Recompute them on demand.
        for (TActorIterator<AERTeamPlayerStart> Start(GetWorld()); Start; ++Start)
            if (Start->ActorHasTag(AdditionalStartTag))
            {
                bool PrivateBotStart = false;
                for (const auto& Bot : TestBots) if (IsValid(Bot.Get()) && Bot->TrainingStart == *Start) PrivateBotStart = true;
                if (!PrivateBotStart) Start->Destroy();
            }
        PreparedSpawnMap = *Map;
        PreparedSpawnEpoch = Map->GetMapEpoch();
        PreparedSpawnSeed = Map->LayoutConfig.Seed;
        return;
    }
}

AERTeamPlayerStart* AERGameMode::FindAdditionalStart(AController* Player, TConstArrayView<AERTeamPlayerStart*> Anchors)
{
    const auto* Capsule = DefaultPawnClass->GetDefaultObject<ACharacter>()->GetCapsuleComponent();
    const float Radius = Capsule->GetScaledCapsuleRadius(), HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const auto Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
    FCollisionQueryParams SpaceQuery(SCENE_QUERY_STAT(ERAdditionalSpawn), false);
    if (Player->GetPawn()) SpaceQuery.AddIgnoredActor(Player->GetPawn());
    FCollisionQueryParams GeometryQuery(SpaceQuery);
    for (TActorIterator<APawn> Pawn(GetWorld()); Pawn; ++Pawn) GeometryQuery.AddIgnoredActor(*Pawn);
    // Search is bounded spatially, not by a player count. Never escape through
    // walls or spawn in mid-air; if the base is full, normal spawn retries wait.
    for (int32 Ring = 1; Ring <= 8; ++Ring) for (auto* Anchor : Anchors)
    {
        if (Anchor->ActorHasTag(AdditionalStartTag)) continue; // no creeping away from the team base
        for (int32 Side = 0; Side < Ring * 8; ++Side)
        {
            const float Angle = Side * 2.f * PI / (Ring * 8);
            FVector Position = Anchor->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * (Ring * 150.f);
            FHitResult Floor;
            if (!GetWorld()->LineTraceSingleByObjectType(Floor, Position + FVector(0,0,150), Position - FVector(0,0,300), FCollisionObjectQueryParams(ECC_WorldStatic), GeometryQuery) || Floor.ImpactNormal.Z < .7f) continue;
            Position.Z = Floor.ImpactPoint.Z + HalfHeight + 2;
            if (FMath::Abs(Position.Z - Anchor->GetActorLocation().Z) > 40) continue;
            if (GetWorld()->OverlapBlockingTestByChannel(Position, FQuat::Identity, ECC_Pawn, Shape, SpaceQuery)) continue;
            FHitResult Route;
            if (GetWorld()->SweepSingleByChannel(Route, Anchor->GetActorLocation(), Position, FQuat::Identity, ECC_Pawn, Shape, GeometryQuery)) continue;
            bool Supported = true;
            for (const FVector Edge : {FVector(Radius*.8f,0,0), FVector(-Radius*.8f,0,0), FVector(0,Radius*.8f,0), FVector(0,-Radius*.8f,0)})
            {
                FHitResult Support;
                const FVector Feet = Position - FVector(0,0,HalfHeight);
                if (!GetWorld()->LineTraceSingleByObjectType(Support, Feet + Edge + FVector(0,0,10), Feet + Edge - FVector(0,0,12), FCollisionObjectQueryParams(ECC_WorldStatic), GeometryQuery) || Support.ImpactNormal.Z < .7f)
                { Supported = false; break; }
            }
            if (!Supported) continue;
            auto* Start = GetWorld()->SpawnActor<AERTeamPlayerStart>(Position, Anchor->GetActorRotation());
            if (!Start) return nullptr;
            Start->Team = Player->GetPlayerState<AERPlayerState>()->Team;
            Start->Tags.Add(AdditionalStartTag);
            UE_LOG(LogTemp, Display, TEXT("ER_SPAWN_ADDITIONAL team=%d location=%s"), int32(Start->Team), *Position.ToCompactString());
            return Start;
        }
    }
    return nullptr;
}

AActor* AERGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    auto* State = Player ? Player->GetPlayerState<AERPlayerState>() : nullptr;
    if (!State) return nullptr;
    PrepareTeamStarts();
    const auto* Capsule = DefaultPawnClass->GetDefaultObject<ACharacter>()->GetCapsuleComponent();
    const auto Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
    auto* Bot = Cast<AERBotController>(Player);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERSpawn), false);
    if (Player->GetPawn()) Query.AddIgnoredActor(Player->GetPawn());
    if (Bot && Bot->TrainingStart)
        return GetWorld()->OverlapBlockingTestByChannel(Bot->TrainingStart->GetActorLocation(), FQuat::Identity, ECC_Pawn, Shape, Query) ? nullptr : Bot->TrainingStart.Get();

    TArray<AERTeamPlayerStart*> Anchors, Clear;
    for (TActorIterator<AERTeamPlayerStart> Start(GetWorld()); Start; ++Start)
    {
        if (Start->Team != State->Team) continue;
        bool PrivateBotStart = false;
        for (const auto& Other : TestBots) if (IsValid(Other.Get()) && Other->TrainingStart == *Start) PrivateBotStart = true;
        if (PrivateBotStart) continue;
        Anchors.Add(*Start);
        if (!GetWorld()->OverlapBlockingTestByChannel(Start->GetActorLocation(), FQuat::Identity, ECC_Pawn, Shape, Query)) Clear.Add(*Start);
    }
    if (Bot && bConfiguredBots)
    {
        Bot->TrainingStart = FindAdditionalStart(Player, Anchors);
        if (Bot->TrainingStart) return Bot->TrainingStart;
    }
    if (!Clear.IsEmpty()) return Clear[FMath::RandHelper(Clear.Num())];
    return FindAdditionalStart(Player, Anchors);
}
