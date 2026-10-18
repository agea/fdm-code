#include "ERCellNetworkProbe.h"
#include "ERBlockRoom.h"
#include "ERCellNetShard.h"
#include "ERCharacter.h"
#include "ERMatch.h"
#include "ERPlayerController.h"
#include "ERProceduralMap.h"
#include "Components/CapsuleComponent.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/DelayedAutoRegister.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

namespace
{
    const FVector DrywallGrenade(-100, -240, 110), BrickGrenade(240, 0, 110), WoodGrenade(-130, -155, 110);
    constexpr int32 Final = 13;
    int64 RemovedOf(const AERBlockRoom* Room) { return Room->GetCellWorld().GetRemovedCount(); }
    AERBlockRoom* FindLabRoom(UWorld* World)
    {
        for (TActorIterator<AERBlockRoom> It(World); It; ++It) if (!Cast<AERProceduralMap>(*It)) return *It;
        return nullptr;
    }
    // Client process state, observed from map load on: the probe actor itself
    // usually replicates after the first snapshot has already been applied.
    struct FERCellClientMonitor
    {
        FString Role;
        int32 GatedTicks = 0, GateViolations = 0, FirstReadyInBytes = -1;
        uint32 AppliedEpoch = 0, AppliedSequence = 0;
    } GClient;
    void MonitorClient(UWorld* World)
    {
        if (!World->IsGameWorld() || World->GetNetMode() != NM_Client || !World->GetMapName().Contains(TEXT("L_DestructionLab"))) return;
        auto* Room = FindLabRoom(World);
        if (!Room || !Room->HasActorBegunPlay()) return;
        // Before a complete snapshot, no room component may collide or be queried.
        if (!Room->IsMapReady())
        {
            ++GClient.GatedTicks;
            TInlineComponentArray<UPrimitiveComponent*> Components(Room);
            for (auto* C : Components)
                if (C->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
                {
                    if (GClient.GateViolations++ == 0) UE_LOG(LogTemp, Error, TEXT("ER_CELL_NET_GATE_VIOLATION role=%s component=%s epoch=%u"), *GClient.Role, *C->GetName(), Room->GetMapEpoch());
                    break;
                }
            return;
        }
        if (GClient.FirstReadyInBytes < 0)
        {
            const UNetDriver* Driver = World->GetNetDriver();
            GClient.FirstReadyInBytes = Driver && Driver->ServerConnection ? Driver->ServerConnection->InTotalBytes : 0;
        }
        // Every state a client ever applies must be one the host published (no torn manifests).
        if (Room->GetMapEpoch() != GClient.AppliedEpoch || Room->GetAppliedMapSequence() != GClient.AppliedSequence)
        {
            GClient.AppliedEpoch = Room->GetMapEpoch(); GClient.AppliedSequence = Room->GetAppliedMapSequence();
            UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_APPLIED role=%s epoch=%u sequence=%u digest=%u cells=%d removed=%lld gated_ticks=%d"),
                *GClient.Role, GClient.AppliedEpoch, GClient.AppliedSequence, Room->GetStateDigest(), 1, RemovedOf(Room), GClient.GatedTicks);
        }
    }
}

#if !UE_BUILD_SHIPPING
// Self-registering, so the probe needs no game-mode or controller wiring: only a
// listen host started with -ERCellNetworkProbe on L_DestructionLab spawns it, and
// clients started with -ERCellNetRole= monitor their room from map load. Registered
// before the startup map loads (EndOfEngineInit would run after it).
static FDelayedAutoRegisterHelper GERCellNetworkProbeHook(EDelayedRegisterRunPhase::ObjectSystemReady, []
{
    if (FParse::Value(FCommandLine::Get(), TEXT("ERCellNetRole="), GClient.Role))
        FWorldDelegates::OnWorldPostActorTick.AddLambda([](UWorld* World, ELevelTick, float) { if (World) MonitorClient(World); });
    if (!FParse::Param(FCommandLine::Get(), TEXT("ERCellNetworkProbe"))) return;
    FWorldDelegates::OnPostWorldInitialization.AddLambda([](UWorld* World, const UWorld::InitializationValues)
    {
        if (!World || !World->IsGameWorld()) return;
        World->OnWorldBeginPlay.AddLambda([World]
        {
            if (World->GetNetMode() == NM_ListenServer && World->GetMapName().Contains(TEXT("L_DestructionLab"))) World->SpawnActor<AERCellNetworkProbe>();
        });
    });
});
#endif

AERCellNetworkReporter::AERCellNetworkReporter()
{
    bReplicates = true; bOnlyRelevantToOwner = true; PrimaryActorTick.bCanEverTick = false;
}
void AERCellNetworkReporter::ServerHello_Implementation(const FString& InRole)
{
    Role = InRole;
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_HELLO role=%s"), *Role);
}
void AERCellNetworkReporter::ServerReport_Implementation(int32 Stage, uint32 Epoch, uint32 Sequence, uint32 Digest, bool bPassed)
{
    if (Probe.IsValid()) Probe->ReceiveReport(this, Stage, Epoch, Sequence, Digest, bPassed);
}

AERCellNetworkProbe::AERCellNetworkProbe()
{
    bReplicates = true; bAlwaysRelevant = true; PrimaryActorTick.bCanEverTick = true; SetNetUpdateFrequency(20);
}
void AERCellNetworkProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AERCellNetworkProbe, Phase); DOREPLIFETIME(AERCellNetworkProbe, ExpectedEpoch); DOREPLIFETIME(AERCellNetworkProbe, ExpectedSequence);
    DOREPLIFETIME(AERCellNetworkProbe, ExpectedDigest); DOREPLIFETIME(AERCellNetworkProbe, ExpectedChunks); DOREPLIFETIME(AERCellNetworkProbe, ExpectedChunkHash);
    DOREPLIFETIME(AERCellNetworkProbe, ExpectedRemoved); DOREPLIFETIME(AERCellNetworkProbe, WalkFrom); DOREPLIFETIME(AERCellNetworkProbe, WalkTo);
    DOREPLIFETIME(AERCellNetworkProbe, bWalkCrouched);
}
void AERCellNetworkProbe::Check(bool bValue, const TCHAR* Label)
{
    bFailed |= !bValue;
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_PROBE_%s %s phase=%d"), bValue ? TEXT("PASS") : TEXT("FAIL"), Label, Phase);
}
AERBlockRoom* AERCellNetworkProbe::FindRoom() const { return FindLabRoom(GetWorld()); }
AERPlayerController* AERCellNetworkProbe::RolePlayer(const TCHAR* Name) const
{
    for (const auto& Pair : Reporters) if (Pair.Value.IsValid() && Pair.Value->Role == Name) return Pair.Key.Get();
    return nullptr;
}
bool AERCellNetworkProbe::ShardChunks(AERBlockRoom* Room, int32& Count, uint32& Hash, int32* MaskBytes, bool* bOnlyModified) const
{
    // What this epoch's shards actually carry; intact chunks must never appear.
    TArray<int32> Chunks; int32 Bytes = 0; bool bModified = true;
    const auto& Cells = Room->GetCellWorld();
    for (TActorIterator<AERCellNetShard> S(GetWorld()); S; ++S)
    {
        if (S->Room != Room || S->Header.Epoch != Room->GetMapEpoch()) continue;
        for (const auto& Item : S->Chunks.Items)
        {
            Chunks.Add(Item.Chunk); Bytes += Item.Mask.Num() + 8;
            if (!Cells.GetChunks().IsValidIndex(Item.Chunk)) { bModified = false; continue; }
            const auto& C = Cells.GetChunks()[Item.Chunk];
            bModified &= C.AliveCount < C.SX * C.SZ * Cells.GetPanels()[C.Panel].Dims.Y;
        }
    }
    Chunks.Sort();
    Count = Chunks.Num(); Hash = 0;
    for (int32 C : Chunks) Hash = HashCombine(Hash, ::GetTypeHash(C));
    if (MaskBytes) *MaskBytes = Bytes;
    if (bOnlyModified) *bOnlyModified = bModified;
    return true;
}
int32 AERCellNetworkProbe::CellParity(AERBlockRoom* Room, int32& Samples) const
{
    // One trace per cell column against that chunk's own component only: real
    // query collision must exist exactly where the logical alive bits say.
    TArray<UPrimitiveComponent*> Components; Room->GetCellComponents(Components);
    const auto& Cells = Room->GetCellWorld();
    Samples = 0;
    if (Components.Num() != Cells.GetChunks().Num()) return MAX_int32;
    int32 Mismatches = 0, SceneMismatches = 0;
    const FTransform& Actor = Room->GetActorTransform();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERCellNetworkParity), false);
    for (int32 Index = 0; Index < Cells.GetChunks().Num(); ++Index)
    {
        const auto& C = Cells.GetChunks()[Index];
        const auto& P = Cells.GetPanels()[C.Panel];
        for (int32 Z = C.Z0; Z < C.Z0 + C.SZ; ++Z)
            for (int32 X = C.X0; X < C.X0 + C.SX; ++X)
            {
                bool bAlive = false;
                for (int32 Y = 0; Y < P.Dims.Y && !bAlive; ++Y) bAlive = Cells.IsAlive(C.Panel, X, Y, Z);
                const FVector Center = Cells.GetCellBox({C.Panel, X, 0, Z}).GetCenter();
                const FVector A = Actor.TransformPosition(P.Transform.TransformPosition(FVector(Center.X, -3, Center.Z)));
                const FVector B = Actor.TransformPosition(P.Transform.TransformPosition(FVector(Center.X, P.Size.Y + 3, Center.Z)));
                FHitResult Hit;
                const bool bHit = Components[Index]->IsQueryCollisionEnabled() && Components[Index]->LineTraceComponent(Hit, A, B, Query);
                if (bHit != bAlive) ++Mismatches;
                // The same column through the scene query structure that pawn sweeps use.
                TArray<FHitResult> Hits;
                GetWorld()->LineTraceMultiByObjectType(Hits, A, B, FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), Query);
                bool bScene = false;
                for (const auto& H : Hits) bScene |= H.GetComponent() == Components[Index];
                if (bScene != bAlive) { if (SceneMismatches++ < 3) UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_SCENE_MISMATCH chunk=%d x=%d z=%d alive=%d scene=%d component=%d"), Index, X, Z, bAlive, bScene, bHit); }
                ++Samples;
            }
    }
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_PARITY authority=%d samples=%d component_mismatches=%d scene_mismatches=%d"), HasAuthority(), Samples, Mismatches, SceneMismatches);
    return Mismatches + SceneMismatches;
}
void AERCellNetworkProbe::SweepPath(AERCharacter* C, const FVector& From, const FVector& To) const
{
    // Diagnostic: the walking capsule's first blocking hit along the breach path.
    const auto* M = C->GetCharacterMovement();
    const float Half = bWalkCrouched ? M->GetCrouchedHalfHeight() : C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    const float Radius = C->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERCellNetworkSweep), false);
    for (TActorIterator<APawn> It(GetWorld()); It; ++It) Query.AddIgnoredActor(*It);
    for (float Lift : {0.f, 25.f, 45.f})
    {
        const FVector A(From.X, From.Y, Half + 1 + Lift), B(To.X, To.Y, Half + 1 + Lift);
        FHitResult Hit;
        const bool bHit = GetWorld()->SweepSingleByChannel(Hit, A, B, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, Half), Query);
        FERCellId Id; auto* Room = Cast<AERBlockRoom>(Hit.GetActor());
        const FString Cell = Room && Room->ResolveCell(Hit, Id) ? Room->DescribeCell(Id) : TEXT("none");
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_SWEEP authority=%d lift=%.0f half=%.2f radius=%.2f blocked=%d at=%s normal=%s component=%s cell=%s"), HasAuthority(), Lift, Half, Radius, bHit,
            *Hit.Location.ToString(), *Hit.ImpactNormal.ToString(), Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("none"), *Cell);
    }
}
void AERCellNetworkProbe::Place(AERPlayerController* PC, const FVector& P, const FRotator& Aim)
{
    if (!PC || !PC->GetPawn()) return;
    PC->GetPawn()->TeleportTo(P, Aim);
    auto* C = CastChecked<AERCharacter>(PC->GetPawn());
    C->GetCharacterMovement()->StopMovementImmediately(); C->GetCharacterMovement()->SetMovementMode(MOVE_Walking); C->GetCharacterMovement()->ForceClientAdjustment();
    PC->SetControlRotation(Aim); PC->ClientSetRotation(Aim);
}
bool AERCellNetworkProbe::WalkArrived(AERPlayerController* PC, const TCHAR* Label)
{
    // The report is reliable but the last moves are not: wait for the authoritative pawn.
    if (WalkReportedAt == 0) WalkReportedAt = GetWorld()->GetTimeSeconds();
    const FVector P = PC && PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector(1.e6);
    const bool bArrived = FVector::Dist2D(P, WalkTo) < 40 && P.Z > 40;
    const double Waited = GetWorld()->GetTimeSeconds() - WalkReportedAt;
    if (!bArrived && Waited < 3) return false;
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_WALK %s server_pos=%s waited=%.2f"), Label, *P.ToString(), Waited);
    if (auto* C = PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr) SweepPath(C, WalkFrom, WalkTo);
    Check(bArrived, Label); WalkReportedAt = 0;
    return true;
}
void AERCellNetworkProbe::Advance(int32 Next, float Seconds)
{
    auto* Room = Map.Get();
    Phase = Next; ExpectedEpoch = Room->GetMapEpoch(); ExpectedSequence = Room->GetMapSequence(); ExpectedDigest = Room->GetStateDigest();
    ExpectedRemoved = RemovedOf(Room);
    int32 MaskBytes = 0; bool bOnlyModified = true;
    ShardChunks(Room, ExpectedChunks, ExpectedChunkHash, &MaskBytes, &bOnlyModified);
    Check(bOnlyModified, TEXT("authority_shards_carry_only_modified_chunks"));
    {
        int32 Samples = 0; const int32 Mismatches = CellParity(Room, Samples);
        Check(Mismatches == 0 && Samples > 0 && Room->ValidateCells(), TEXT("authority_cell_collision_parity"));
    }
    Deadline = GetWorld()->GetTimeSeconds() + Seconds; ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_PHASE %d epoch=%u sequence=%u digest=%u cells=%d removed=%lld shard_chunks=%d/%d mask_bytes=%d"), Phase, ExpectedEpoch, ExpectedSequence, ExpectedDigest,
        1, ExpectedRemoved, ExpectedChunks, Room->GetCellWorld().GetChunks().Num(), MaskBytes);
}
void AERCellNetworkProbe::ReceiveReport(AERCellNetworkReporter* Reporter, int32 Stage, uint32 Epoch, uint32 Sequence, uint32 Digest, bool bPassed)
{
    if (!HasAuthority() || !Reporter || Stage != Phase || Reporter->Role.IsEmpty()) return;
    const bool bValid = bPassed && Epoch == ExpectedEpoch && Sequence == ExpectedSequence && Digest == ExpectedDigest;
    Check(bValid, TEXT("remote_snapshot_collision_report"));
    Reports.Add(Reporter->Role + FString::FromInt(Stage));
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_REPORT role=%s phase=%d valid=%d epoch=%u sequence=%u digest=%u"), *Reporter->Role, Stage, bValid, Epoch, Sequence, Digest);
}
void AERCellNetworkProbe::Complete()
{
    auto* Room = Map.Get();
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_PROBE_COMPLETE result=%s epoch=%u grenades=%d cells=%lld chunks=%d"), bFailed ? TEXT("FAIL") : TEXT("PASS"),
        Room ? Room->GetMapEpoch() : 0, Grenades, Room ? Room->GetCellWorld().GetCellCount() : 0, Room ? Room->GetCellWorld().GetChunks().Num() : 0);
    Phase = Final + 1; SetActorTickEnabled(false);
    FPlatformMisc::RequestExitWithStatus(false, bFailed ? 1 : 0);
}
void AERCellNetworkProbe::LogBytes(AERBlockRoom* Room, const TCHAR* Label) const
{
    const UNetDriver* Driver = GetWorld()->GetNetDriver();
    const UNetConnection* Connection = Driver ? Driver->ServerConnection.Get() : nullptr;
    int32 Chunks = 0, MaskBytes = 0; uint32 Hash = 0; ShardChunks(Room, Chunks, Hash, &MaskBytes);
    // What one raw alive bitmask of the whole lab would cost, for comparison.
    int64 FullBytes = 0;
    for (const auto& C : Room->GetCellWorld().GetChunks()) FullBytes += FMath::DivideAndRoundUp(C.SX * C.SZ * Room->GetCellWorld().GetPanels()[C.Panel].Dims.Y, 8);
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_BYTES %s role=%s first_ready_in_bytes=%d in_bytes=%d packets_lost=%d shard_chunks=%d/%d mask_bytes=%d full_bitmask_bytes=%lld removed=%lld"),
        Label, *Role, GClient.FirstReadyInBytes, Connection ? Connection->InTotalBytes : -1, Connection ? Connection->InTotalPacketsLost : -1, Chunks, Room->GetCellWorld().GetChunks().Num(), MaskBytes, FullBytes, RemovedOf(Room));
}
void AERCellNetworkProbe::ClientTick(float DeltaSeconds)
{
    Role = GClient.Role;
    // The snapshot can complete this very frame, before the post-tick monitor runs.
    MonitorClient(GetWorld());
    auto* Room = FindRoom();
    auto* PC = Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
    auto* C = PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
    AERCellNetworkReporter* Reporter = nullptr;
    for (TActorIterator<AERCellNetworkReporter> It(GetWorld()); It; ++It) if (PC && It->GetOwner() == PC) Reporter = *It;
    if (Reporter && !bHelloSent && !Role.IsEmpty()) { Reporter->ServerHello(Role); bHelloSent = true; }
    if (Room && Phase > 0 && GetWorld()->GetTimeSeconds() > NextDiagnostic)
    {
        NextDiagnostic = GetWorld()->GetTimeSeconds() + 2;
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_WAIT phase=%d role=%s ready=%d current=%d epoch=%u/%u sequence=%u applied=%u expected=%u digest=%u/%u pawn=%s"), Phase, *Role,
            Room->IsMapReady(), Room->IsSnapshotCurrent(), Room->GetMapEpoch(), ExpectedEpoch, Room->GetMapSequence(), Room->GetAppliedMapSequence(), ExpectedSequence,
            Room->GetStateDigest(), ExpectedDigest, C ? *C->GetActorLocation().ToString() : TEXT("none"));
    }
    if (C && (Phase == 6 || Phase == 7) && GetWorld()->GetTimeSeconds() > NextTrace)
    {
        NextTrace = GetWorld()->GetTimeSeconds() + .25;
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_TRACE client phase=%d pos=%s crouched=%d wants=%d mode=%d vel=%s"), Phase, *C->GetActorLocation().ToString(), C->bIsCrouched,
            C->GetCharacterMovement()->bWantsToCrouch, int32(C->GetCharacterMovement()->MovementMode), *C->GetVelocity().ToString());
    }
    if (!Reporter || !C || !Room || Phase <= 0 || Phase > Final || !Room->IsSnapshotCurrent() || Room->GetMapEpoch() != ExpectedEpoch ||
        Room->GetMapSequence() != ExpectedSequence || Room->GetStateDigest() != ExpectedDigest) return;
    if (ClientPhase != Phase) { ClientPhase = Phase; PhaseElapsed = 0; bWalking = false; }
    PhaseElapsed += DeltaSeconds;
    const bool bInitial = Role == TEXT("initial");
    if ((!bInitial && Phase < 8) || Reports.Contains(FString::FromInt(Phase))) return;
    FString Detail;
    bool bPassed = true;
    if (Phase == 6 || Phase == 7)
    {
        // The client's own replicated character crosses the breach under real movement.
        if (!bWalking)
        {
            if (PhaseElapsed < .5f || FVector::Dist2D(C->GetActorLocation(), WalkFrom) > 20) return;
            bWalking = true; PhaseElapsed = 0;
            if (bWalkCrouched) C->Crouch();
        }
        const FVector D = WalkTo - C->GetActorLocation();
        if (FVector2D(D.X, D.Y).Size() > 15 && PhaseElapsed < 8) { C->AddMovementInput(FVector(D.X, D.Y, 0).GetSafeNormal(), 1); return; }
        C->GetCharacterMovement()->StopMovementImmediately();
        bPassed = FVector::Dist2D(C->GetActorLocation(), WalkTo) < 40 && C->GetActorLocation().Z > 40;
        if (bWalkCrouched) C->UnCrouch();
        SweepPath(C, WalkFrom, WalkTo);
        Detail = FString::Printf(TEXT(" walk=%s crouched=%d pos=%s"), Phase == 6 ? TEXT("drywall_breach") : TEXT("brick_breach"), bWalkCrouched, *C->GetActorLocation().ToString());
    }
    int32 Chunks = 0, Samples = 0, Mismatches = 0; uint32 Hash = 0; bool bOnlyModified = true;
    ShardChunks(Room, Chunks, Hash, nullptr, &bOnlyModified);
    Mismatches = CellParity(Room, Samples);
    bPassed &= Mismatches == 0 && Samples > 0 && Room->ValidateCells();
    bPassed &= GClient.GateViolations == 0 && Chunks == ExpectedChunks && Hash == ExpectedChunkHash && bOnlyModified && RemovedOf(Room) == ExpectedRemoved;
    // Gating is enforced by GateViolations on every gated tick. Zero gated ticks is
    // legal: descriptor and shards can arrive in the same frame, before any tick.
    if (Phase == 1 && bInitial)
    {
        // Clients never mutate: neither grenades nor direct cell batches.
        const uint32 Before = Room->GetStateDigest();
        TMap<FERCellId, float> Damage; Damage.Add({0, 0, 0, 0}, TNumericLimits<float>::Max());
        bPassed &= AERBlockRoom::ApplyWorldExplosion(GetWorld(), DrywallGrenade, FERBlockExplosionProfile()).Damaged == 0 && Room->ApplyCellBatch(Damage).Damaged == 0 &&
            Before == Room->GetStateDigest() && Room->GetCellWorld().GetRemovedCount() == 0;
        Detail += TEXT(" authority_guard");
    }
    if ((Phase == 1 && bInitial) || (Phase == 8 && !bInitial) || Phase == 5) LogBytes(Room, Phase == 8 ? TEXT("late_join") : Phase == 1 ? TEXT("initial_join") : TEXT("after_brick"));
    Reports.Add(FString::FromInt(Phase));
    Reporter->ServerReport(Phase, Room->GetMapEpoch(), Room->GetMapSequence(), Room->GetStateDigest(), bPassed);
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_CLIENT_%s phase=%d role=%s epoch=%u sequence=%u digest=%u cells=%d removed=%lld shard_chunks=%d parity_samples=%d mismatches=%d gated_ticks=%d gate_violations=%d%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"), Phase, *Role, Room->GetMapEpoch(), Room->GetMapSequence(), Room->GetStateDigest(), 1, RemovedOf(Room), Chunks, Samples, Mismatches,
        GClient.GatedTicks, GClient.GateViolations, *Detail);
}
void AERCellNetworkProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    if (!HasAuthority()) { ClientTick(DeltaSeconds); return; }
    if (!Map.IsValid()) Map = FindRoom();
    auto* Room = Map.Get();
    if (!Room) return;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        auto* PC = Cast<AERPlayerController>(It->Get());
        if (!PC) continue;
        if (PC->IsLocalController()) { Host = PC; continue; }
        if (Reporters.Contains(PC)) continue;
        FActorSpawnParameters Spawn; Spawn.Owner = PC;
        auto* Reporter = GetWorld()->SpawnActor<AERCellNetworkReporter>(Spawn);
        if (Reporter) Reporter->Probe = this;
        Reporters.Add(PC, Reporter);
    }
    auto LogPublished = [this, Room]
    {
        if (Room->GetMapEpoch() == PublishedEpoch && Room->GetMapSequence() == PublishedSequence) return;
        PublishedEpoch = Room->GetMapEpoch(); PublishedSequence = Room->GetMapSequence();
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_PUBLISHED epoch=%u sequence=%u digest=%u cells=%d removed=%lld"), PublishedEpoch, PublishedSequence, Room->GetStateDigest(), 1, RemovedOf(Room));
    };
    LogPublished();
    auto Blast = [this, Room](const TCHAR* Label, const FVector& Origin)
    {
        const auto R = AERBlockRoom::ApplyWorldExplosion(GetWorld(), Origin, FERBlockExplosionProfile());
        ++Grenades;
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_GRENADE %s damaged=%d removed=%d chunks=%d total_ms=%.3f epoch=%u sequence=%u"), Label, R.Damaged, R.Destroyed, R.GroupsRebuilt, R.TotalMs, Room->GetMapEpoch(), Room->GetMapSequence());
        return R;
    };
    auto* Initial = RolePlayer(TEXT("initial"));
    auto* Late = RolePlayer(TEXT("late"));
    if (auto* C = Initial ? Cast<AERCharacter>(Initial->GetPawn()) : nullptr; C && (Phase == 6 || Phase == 7) && GetWorld()->GetTimeSeconds() > NextTrace)
    {
        NextTrace = GetWorld()->GetTimeSeconds() + .25;
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_TRACE server phase=%d pos=%s crouched=%d wants=%d mode=%d vel=%s"), Phase, *C->GetActorLocation().ToString(), C->bIsCrouched,
            C->GetCharacterMovement()->bWantsToCrouch, int32(C->GetCharacterMovement()->MovementMode), *C->GetVelocity().ToString());
    }
    if (Phase == 0)
    {
        const auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
        if (!Host.IsValid() || !Initial || !Initial->GetPawn() || !Mode || !Mode->IsMatchInProgress()) return;
        const auto& Cells = Room->GetCellWorld();
        Check(Cells.GetCellCount() == 77580 && Room->ValidateCells(), TEXT("listen_host_runs_5cm_cell_lab"));
        Check(Room->GetNetworkPageCount() == FMath::DivideAndRoundUp(Cells.GetChunks().Num(), AERCellNetShard::Capacity) && Room->GetMapEpoch() > 0, TEXT("protocol7_shard_manifest"));
        Advance(1, 60);
    }
    else if (GetWorld()->GetTimeSeconds() > Deadline && Phase <= Final)
    {
        Check(false, TEXT("phase_timeout"));
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_PROBE_COMPLETE result=FAIL"));
        FPlatformMisc::RequestExitWithStatus(false, 1); Phase = Final + 1;
        return;
    }
    else if (Phase == 1 && Reported(TEXT("initial"), 1))
    {
        Check(ExpectedChunks == 0, TEXT("intact_lab_sends_no_chunks"));
        const auto R = Blast(TEXT("drywall_partition"), DrywallGrenade);
        Check(R.Destroyed > 2000 && Room->ValidateCells(), TEXT("authority_grenade_opens_drywall_breach"));
        Advance(2);
    }
    else if (Phase == 2 && Reported(TEXT("initial"), 2))
    {
        const auto R = Blast(TEXT("brick20_1"), BrickGrenade);
        Check(R.Damaged > 0, TEXT("authority_grenade_damages_brick"));
        Advance(3);
    }
    else if (Phase == 3 && Reported(TEXT("initial"), 3)) { Blast(TEXT("brick20_2"), BrickGrenade); Advance(4); }
    else if (Phase == 4 && Reported(TEXT("initial"), 4)) { Blast(TEXT("brick20_3"), BrickGrenade); Advance(5); }
    else if (Phase == 5 && Reported(TEXT("initial"), 5))
    {
        WalkFrom = FVector(-100, -180, 100); WalkTo = FVector(-100, -330, 100); bWalkCrouched = false;
        Place(Initial, WalkFrom, FRotator(0, -90, 0));
        Advance(6);
    }
    else if (Phase == 6 && Reported(TEXT("initial"), 6) && WalkArrived(Initial, TEXT("authoritative_remote_crosses_drywall_breach")))
    {
        WalkFrom = FVector(180, 0, 100); WalkTo = FVector(330, 0, 100); bWalkCrouched = true;
        Place(Initial, WalkFrom, FRotator::ZeroRotator);
        Advance(7);
    }
    else if (Phase == 7 && Reported(TEXT("initial"), 7) && WalkArrived(Initial, TEXT("authoritative_crouched_remote_crosses_brick_breach")))
    {
        // Clear of both team starts (0,+-100), or the late player cannot spawn.
        Place(Initial, FVector(100, -150, 100), FRotator::ZeroRotator);
        Advance(8, 90);
        Check(ExpectedChunks > 0 && ExpectedChunks < Room->GetCellWorld().GetChunks().Num(), TEXT("late_join_snapshot_is_modified_chunks_only"));
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_NET_READY_FOR_LATE_JOIN"));
    }
    else if (Phase == 8 && Reported(TEXT("initial"), 8) && Reported(TEXT("late"), 8))
    {
        Check(Late != nullptr, TEXT("late_join_matches_destroyed_cell_snapshot"));
        Host->ConsoleCommand(TEXT("ERBlockRegenerate"));
        Check(Room->GetCellWorld().GetRemovedCount() == 0 && Room->ValidateCells() && Room->GetMapEpoch() == ExpectedEpoch + 1, TEXT("console_regenerate_publishes_cell_epoch"));
        Advance(9);
    }
    else if (Phase == 9 && Reported(TEXT("initial"), 9) && Reported(TEXT("late"), 9))
    {
        // Authority collapse: detached cells replicate as ordinary chunk masks.
        if (!bCollapseStarted) { bCollapseStarted = true; Host->ConsoleCommand(TEXT("ERBlockCollapseDemo")); return; }
        if (Room->IsSupportPending()) return;
        Check(Room->GetCellWorld().GetDetachedCount() == 2 * 29 * 19 && Room->ValidateCells(), TEXT("authority_collapse_detaches_isolated_patch"));
        Advance(10);
    }
    else if (Phase == 10 && Reported(TEXT("initial"), 10) && Reported(TEXT("late"), 10))
    {
        Check(Blast(TEXT("drywall_partition_epoch2"), DrywallGrenade).Destroyed > 2000, TEXT("new_cell_epoch_accepts_grenade"));
        Advance(11);
    }
    else if (Phase == 11 && Reported(TEXT("initial"), 11) && Reported(TEXT("late"), 11))
    {
        Host->ConsoleCommand(TEXT("ERBlockRegenerate"));
        Check(Room->GetCellWorld().GetRemovedCount() == 0 && Room->GetMapEpoch() == ExpectedEpoch + 1, TEXT("console_regenerate_publishes_clean_epoch"));
        Advance(12);
    }
    else if (Phase == 12 && Reported(TEXT("initial"), 12) && Reported(TEXT("late"), 12))
    {
        Check(Blast(TEXT("wood_board_epoch3"), WoodGrenade).Destroyed > 500, TEXT("regenerated_epoch_accepts_grenade"));
        Advance(13);
    }
    else if (Phase == Final && Reported(TEXT("initial"), Final) && Reported(TEXT("late"), Final)) Complete();
    LogPublished();
#endif
}
