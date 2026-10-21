#include "ERCellLabProbe.h"
#include "ERBlockRoom.h"
#include "ERBlockTarget.h"
#include "ERCharacter.h"
#include "ERMatch.h"
#include "ERPlayerController.h"
#include "ERWeapon.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AERCellLabProbe::AERCellLabProbe() { PrimaryActorTick.bCanEverTick = true; }
void AERCellLabProbe::Check(bool Value, const TCHAR* Label)
{
    Failed |= !Value;
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_PROBE_%s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), Label);
}
void AERCellLabProbe::Capture(AERCharacter* Character, AERPlayerController* PC, const TCHAR* Name, FVector At, float Yaw, int32 Next)
{
    Elapsed = 0;
    if (!FApp::CanEverRender()) { Phase = Next; return; }
    Character->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
    Character->GetCharacterMovement()->StopMovementImmediately();
    PC->SetControlRotation(FRotator(-8, Yaw, 0));
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Testing") / Name, true, false);
    Phase = 100; ResumePhase = Next;
}
void AERCellLabProbe::Metrics(const TCHAR* Label, const FERBlockBatchResult& R, AERBlockRoom* Room)
{
    const auto& U = Room->UpdateProfile;
    UE_LOG(LogTemp, Display, TEXT("ER_CELL_PROBE_METRIC %s candidates=%d occluded=%d attenuated=%d static_traces=%d damaged=%d removed=%d chunks=%d query_ms=%.3f exposure_ms=%.3f mutation_ms=%.3f total_ms=%.3f mesh_ms=%.3f collision_ms=%.3f quads=%d boxes=%d"),
        Label, R.Candidates, R.Occluded, R.Attenuated, R.CoverTraces, R.Damaged, R.Destroyed, R.GroupsRebuilt, R.QueryMs, R.ExposureMs, R.MutationMs, R.TotalMs, U.MeshMs, U.CollisionMs, U.Quads, U.Boxes);
}
void AERCellLabProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    TActorIterator<AERBlockRoom> RoomIterator(GetWorld());
    AERBlockRoom* Room = RoomIterator ? *RoomIterator : nullptr;
    auto* PC = Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
    auto* Character = PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
    AERBlockTarget* Drywall = nullptr;
    for (TActorIterator<AERBlockTarget> It(GetWorld()); It; ++It) if (It->ActorHasTag(TEXT("ERBlockDrywall"))) Drywall = *It;
    if (!Room || !Character || !Character->Weapon || !Drywall)
    {
        if (Elapsed > 30) { Check(false, TEXT("lab_initialized")); FPlatformMisc::RequestExitWithStatus(false, 1); }
        return;
    }
    const auto& Cells = Room->GetCellWorld();
    if (Phase == 100) { if (Elapsed > .6f) { Phase = ResumePhase; Elapsed = 0; } return; }
    auto MoveTo = [Character](FVector Position)
    {
        Character->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
        Character->GetCharacterMovement()->StopMovementImmediately();
    };
    auto Blast = [&](const TCHAR* Label, FVector Origin)
    {
        const auto R = AERBlockRoom::ApplyWorldExplosion(GetWorld(), Origin, FERBlockExplosionProfile(), Character);
        Metrics(Label, R, Room); ++Grenades;
        return R;
    };
    if (Phase == 0 && Elapsed > 2)
    {
        int64 Expected = 0;
        for (const auto& P : Cells.GetPanels()) Expected += int64(P.Dims.X) * P.Dims.Y * P.Dims.Z;
        Check(GetNetMode() == NM_Standalone && Cells.GetCellCount() > 0, TEXT("lab_runs_in_5cm_cells"));
        Check(Cells.GetCellCount() == Expected && Expected == 77580 && Cells.GetPanels().Num() == 8 && Room->ValidateCells(), TEXT("lab_cells_panels_chunks_and_components"));
        Check(Character->GetActorLocation().Z > 90 && Character->GetActorLocation().Z < 101, TEXT("pawn_supported_by_floor"));
        const auto& G = Room->GenerationProfile; const auto M = Room->GetMemoryProfile();
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_PROBE_METRIC generation total_ms=%.3f logical_ms=%.3f components_ms=%.3f mesh_ms=%.3f collision_ms=%.3f cells=%lld panels=%d chunks=%d quads=%d boxes=%d bodies=%d tracked_bytes=%llu logical_bytes=%llu mapping_bytes=%llu mesh_resource_bytes=%llu"),
            G.TotalMs, G.LogicalMs, G.AddMs, G.MeshMs, G.CollisionMs, Cells.GetCellCount(), Cells.GetPanels().Num(), Cells.GetChunks().Num(), M.Quads, M.CollisionBoxes, M.CollisionBodies,
            uint64(M.TrackedBytes()), uint64(M.LogicalBytes), uint64(M.MappingBytes), uint64(M.InstanceResourceBytes));
        MoveTo(FVector(25, 0, 100)); PC->SetControlRotation(FRotator(0, -90, 0));
        PC->ConsoleCommand(TEXT("ERBlockDamage 10"));
        Check(Cells.GetRemovedCount() == 0 && Room->LastHit.Contains(TEXT("HP=10.0")) && Room->LastHit.Contains(TEXT("material=0")), TEXT("console_aim_damages_one_drywall_cell"));
        PC->ConsoleCommand(TEXT("ERBlockDestroy"));
        Check(Cells.GetRemovedCount() == 1 && Room->ValidateCells(), TEXT("console_aim_destroys_exactly_one_cell"));
        // The real 36-damage cartridge exhausts itself against two 20HP boards.
        MoveTo(FVector(0, 0, 100));
        TargetBefore = Drywall->Health;
        const int32 Ammo = Character->Weapon->MagazineAmmo;
        const int64 Before = Cells.GetRemovedCount();
        Character->Weapon->StartFire(); Character->Weapon->StopFire();
        const int64 Removed = Cells.GetRemovedCount() - Before;
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_PROBE_WEAPON_SHOT removed=%lld target=%.1f->%.1f %s"), Removed, TargetBefore, Drywall->Health, *Room->LastCombat);
        Check(Character->Weapon->MagazineAmmo == Ammo - 1 && Removed == 1 && Drywall->Health == TargetBefore && Room->ValidateCells(),
            TEXT("36_damage_cartridge_stops_in_two_20hp_boards_before_rear_target"));
        Room->Regenerate();
        Capture(Character, PC, TEXT("Cells-Overview.png"), FVector(170, 170, 100), -135, 20);
    }
    // Both faces of the wood board: east (local -Y) lit by the fill light, west (local +Y) by the sun.
    else if (Phase == 20) Capture(Character, PC, TEXT("Cells-WoodEast.png"), FVector(-40, -155, 100), 180, 21);
    else if (Phase == 21) Capture(Character, PC, TEXT("Cells-WoodWest.png"), FVector(-205, -60, 100), -45, 5);
    else if (Phase == 5)
    {
        Blast(TEXT("grenade_drywall_partition"), FVector(-100, -240, 110));
        Check(Cells.GetRemovedCount() > 2000 && Room->ValidateCells(), TEXT("grenade_opens_drywall_breach"));
        Capture(Character, PC, TEXT("Cells-DrywallBreach.png"), FVector(150, -40, 100), -140, 6);
    }
    else if (Phase == 6)
    {
        PC->SetControlRotation(FRotator(0, -90, 0));
        MoveTo(FVector(-100, -180, 100));
        Phase = 1; Elapsed = 0;
    }
    else if (Phase == 1)
    {
        Character->AddMovementInput(FVector(0, -1, 0), 1);
        if (Elapsed > 1.5f)
        {
            Check(Character->GetActorLocation().Y < -290 && Character->GetActorLocation().Z > 90, TEXT("actual_character_walks_through_grenade_breach"));
            Room->Regenerate();
            Blast(TEXT("grenade_brick20_1"), FVector(240, 0, 110));
            MoveTo(FVector(180, 0, 100));
            Phase = 2; Elapsed = 0;
        }
    }
    else if (Phase == 2)
    {
        Character->AddMovementInput(FVector(1, 0, 0), 1);
        if (Elapsed > 1.5f)
        {
            Check(Character->GetActorLocation().X < 250, TEXT("one_grenade_does_not_breach_four_layer_brick"));
            Blast(TEXT("grenade_brick20_2"), FVector(240, 0, 110));
            Blast(TEXT("grenade_brick20_3"), FVector(240, 0, 110));
            // Equivalent transmission: three grenades open 20cm brick for a crouched pawn.
            MoveTo(FVector(180, 0, 100)); Character->Crouch();
            Phase = 3; Elapsed = 0;
        }
    }
    else if (Phase == 3)
    {
        Character->AddMovementInput(FVector(1, 0, 0), 1);
        if (Elapsed > 1.5f)
        {
            Check(Character->GetActorLocation().X > 290 && Room->ValidateCells(), TEXT("three_grenades_breach_four_layer_brick_for_crouched_pawn"));
            Character->UnCrouch();
            Capture(Character, PC, TEXT("Cells-BrickBreach.png"), FVector(90, -70, 100), 20, 7);
        }
    }
    else if (Phase == 7)
    {
        {
            Room->Regenerate();
            const auto Concrete = Blast(TEXT("grenade_concrete20"), FVector(0, 240, 110));
            Check(Concrete.Destroyed == 0 && Concrete.Damaged > 0, TEXT("grenade_damages_but_does_not_remove_concrete"));
            const auto Wood = Blast(TEXT("grenade_wood_board"), FVector(-130, -155, 110));
            const auto Sheet = Blast(TEXT("grenade_container_sheet"), FVector(-130, 155, 110));
            Check(Wood.Destroyed > 500 && Sheet.Destroyed > 300 && Room->ValidateCells(), TEXT("grenade_breaches_wood_board_and_container_sheet"));
            Room->Regenerate();
            MoveTo(FVector(0, 0, 100));
            // Real HE projectile fuse goes through the same structural profile.
            auto* Grenade = GetWorld()->SpawnActorDeferred<AERProjectile>(Character->Weapon->ProjectileClass, FTransform(FVector(100, -238, 110)), Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (Grenade)
            {
                Grenade->Payload = EERPayload::HE; Grenade->Damage = 100; Grenade->ExplosionRadius = 300; Grenade->FuseDuration = .2f;
                Grenade->BlockExplosion = FERBlockExplosionProfile();
                Grenade->Movement->InitialSpeed = 0; Grenade->Movement->MaxSpeed = 0; Grenade->Movement->Velocity = FVector::ZeroVector;
                UGameplayStatics::FinishSpawningActor(Grenade, Grenade->GetActorTransform());
                Grenade->Movement->ProjectileGravityScale = 0;
            }
            else Check(false, TEXT("real_grenade_spawn"));
            Phase = 4; Elapsed = 0;
        }
    }
    else if (Phase == 4 && Elapsed > 1)
    {
        // The structural profile is local: nothing changes at or beyond 200cm.
        bool bLocal = true;
        const FVector Origin = Room->GetActorTransform().InverseTransformPosition(FVector(100, -238, 110 + 10));
        for (int32 C = 0; C < Cells.GetChunks().Num(); ++C)
        {
            const auto& Chunk = Cells.GetChunks()[C];
            const auto& P = Cells.GetPanels()[Chunk.Panel];
            for (int32 Z = 0; Z < Chunk.SZ; ++Z) for (int32 Y = 0; Y < P.Dims.Y; ++Y) for (int32 X = 0; X < Chunk.SX; ++X)
            {
                const FERCellId Id{Chunk.Panel, Chunk.X0 + X, Y, Chunk.Z0 + Z};
                if (Cells.GetHealth(Id) < Cells.GetMaxHealth(Chunk.Panel)) bLocal &= FVector::Distance(Cells.GetCellCenter(Id), Origin) < FERBlockExplosionProfile().OuterRadius;
            }
        }
        Check(Cells.GetRemovedCount() > 500 && bLocal && Room->ValidateCells(), TEXT("real_he_fuse_breaches_cells_within_outer_radius"));
        Phase = 8; Elapsed = 0;
    }
    else if (Phase == 8)
    {
        const int32 Components = Room->GetInstanceComponents().Num();
        PC->ConsoleCommand(TEXT("ERBlockRegenerate")); PC->ConsoleCommand(TEXT("ERBlockRegenerate"));
        Check(Room->ValidateCells() && Room->GetInstanceComponents().Num() == Components && Cells.GetRemovedCount() == 0 && Character->GetActorLocation().Size2D() < 1,
            TEXT("cell_regeneration_reuses_no_stale_components_and_safe_spawn"));
        UE_LOG(LogTemp, Display, TEXT("ER_CELL_PROBE_COMPLETE result=%s grenades=%d"), Failed ? TEXT("FAIL") : TEXT("PASS"), Grenades);
        SetActorTickEnabled(false);
        FPlatformMisc::RequestExitWithStatus(false, Failed ? 1 : 0);
    }
}
