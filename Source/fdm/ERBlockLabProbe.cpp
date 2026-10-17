#include "ERBlockLabProbe.h"
#include "ERBlockRoom.h"
#include "ERBlockTarget.h"
#include "Camera/CameraComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ERCharacter.h"
#include "ERMatch.h"
#include "ERPlayerController.h"
#include "ERWeapon.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AERBlockLabProbe::AERBlockLabProbe() { PrimaryActorTick.bCanEverTick = true; }
void AERBlockLabProbe::Check(bool Value, const TCHAR* Label)
{
    Failed |= !Value;
    UE_LOG(LogTemp, Display, TEXT("ER_BLOCK_PROBE_%s %s"), Value ? TEXT("PASS") : TEXT("FAIL"), Label);
}
void AERBlockLabProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    TActorIterator<AERBlockRoom> RoomIterator(GetWorld());
    AERBlockRoom* Room = RoomIterator ? *RoomIterator : nullptr;
    auto* PC = Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
    auto* Character = PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
    if (!Room || !Character || !Character->Weapon)
    {
        if (Elapsed > 30) { Check(false, TEXT("lab_initialized")); FPlatformMisc::RequestExitWithStatus(false, 1); }
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("ERBlockCombatProbe")))
    {
        TickCombat(Character, PC, Room, DeltaSeconds);
        return;
    }
    auto MoveTo = [Character](FVector Position)
    {
        Character->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
        Character->GetCharacterMovement()->StopMovementImmediately();
    };
    if (Phase == 0 && Elapsed > 2)
    {
        Check(GetNetMode() == NM_Standalone && GetWorld()->GetAuthGameMode<AERGameMode>()->IsMatchInProgress(), TEXT("single_player_match_and_firing_enabled"));
        Check(Room->GetInstanceCount() == 504 && Room->ValidateMappings(), TEXT("one_generated_room"));
        Check(Character->GetActorLocation().Z > 90 && Character->GetActorLocation().Z < 101, TEXT("pawn_supported_by_floor"));
        MoveTo(FVector(25, 0, 100)); PC->SetControlRotation(FRotator(0,-90,0));
        PC->ConsoleCommand(TEXT("ERBlockDamage 10"));
        Check(Room->BlockWorld->GetDestroyedCount() == 0 && Room->LastHit.Contains(TEXT("HP=20")), TEXT("console_aim_damage"));
        PC->ConsoleCommand(TEXT("ERBlockDestroy"));
        Check(Room->BlockWorld->GetDestroyedCount() == 1, TEXT("console_aim_destroy"));
        MoveTo(FVector(75, 0, 100));
        PC->ConsoleCommand(TEXT("ERBlockDestroy"));
        Check(Room->BlockWorld->GetDestroyedCount() == 2 && Room->ValidateMappings(), TEXT("adjacent_instance_identity"));
        MoveTo(FVector(125, 0, 100));
        FHitResult Prepared;
        FCollisionQueryParams PrepareQuery(SCENE_QUERY_STAT(ERBlockPrepare), false, Character);
        FERBlockId PreparedId;
        if (GetWorld()->LineTraceSingleByChannel(Prepared, Character->Camera->GetComponentLocation(), Character->Camera->GetComponentLocation() + FVector(0,-1000,0), ECC_GameTraceChannel1, PrepareQuery) && Room->ResolveHit(Prepared, PreparedId))
            Room->ApplyBlockDamage(PreparedId, Room->BlockWorld->Find(PreparedId)->Health - 1);
        const int32 Ammo = Character->Weapon->MagazineAmmo;
        const uint32 Shots = Character->Weapon->AuthorizedShotCount;
        Character->Weapon->StartFire(); Character->Weapon->StopFire();
        Check(Character->Weapon->AuthorizedShotCount == Shots + 1 && Character->Weapon->MagazineAmmo == Ammo - 1 && Room->BlockWorld->GetDestroyedCount() == 3, TEXT("authorized_weapon_breaks_target_block"));
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Destruction-Blocks.png"), true, false);
        MoveTo(FVector(-25, 0, 100));
        Phase = 1; Elapsed = 0;
    }
    else if (Phase == 1)
    {
        Character->AddMovementInput(FVector(0,-1,0), 1);
        if (Elapsed > 1.5f)
        {
            Check(Character->GetActorLocation().Y < -100 && Character->GetActorLocation().Y > -240, TEXT("actual_character_moves_and_intact_wall_blocks"));
            for (int32 Column = 3; Column <= 5; ++Column)
                for (int32 Row = 0; Row < 8; ++Row)
                {
                    FERBlockId Id; Id.Wall = 0; Id.Column = Column; Id.Row = Row;
                    Room->ApplyBlockDamage(Id, 0, true);
                }
            Phase = 2; Elapsed = 0;
        }
    }
    else if (Phase == 2)
    {
        Character->AddMovementInput(FVector(0,-1,0), 1);
        if (Elapsed > 1)
        {
            Check(Character->GetActorLocation().Y < -300 && Character->GetActorLocation().Z > 90, TEXT("actual_character_walks_through_breach"));
            Character->GetCharacterMovement()->StopMovementImmediately();
            PC->SetControlRotation(FRotator(0,90,0));
            Phase = 3; Elapsed = 0;
        }
    }
    else if (Phase == 3 && Elapsed > .5f)
    {
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Destruction-Breach.png"), true, false);
        Phase = 4; Elapsed = 0;
    }
    else if (Phase == 4 && Elapsed > .5f)
    {
        PC->ConsoleCommand(TEXT("ERBlockRegenerate"));
        Check(Room->GetInstanceCount() == 504 && Room->ValidateMappings() && Character->GetActorLocation().Size2D() < 1, TEXT("console_regeneration_resets_world_and_safe_spawn"));
        UE_LOG(LogTemp, Display, TEXT("ER_BLOCK_PROBE_COMPLETE result=%s"), Failed ? TEXT("FAIL") : TEXT("PASS"));
        SetActorTickEnabled(false);
        FPlatformMisc::RequestExitWithStatus(false, Failed ? 1 : 0);
    }
}

void AERBlockLabProbe::TickCombat(AERCharacter* Character, AERPlayerController* PC, AERBlockRoom* Room, float DeltaSeconds)
{
    AERBlockTarget* Drywall = nullptr;
    AERBlockTarget* Brick = nullptr;
    AERBlockTarget* Concrete = nullptr;
    for (TActorIterator<AERBlockTarget> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(TEXT("ERBlockDrywall"))) Drywall = *It;
        if (It->ActorHasTag(TEXT("ERBlockBrick"))) Brick = *It;
        if (It->ActorHasTag(TEXT("ERBlockConcrete"))) Concrete = *It;
    }
    if (!Drywall || !Brick || !Concrete)
    {
        if (Elapsed > 10) { Check(false, TEXT("combat_targets_saved")); FPlatformMisc::RequestExitWithStatus(false, 1); }
        return;
    }
    if (Phase == 0 && Elapsed > 2)
    {
        Character->SetActorLocation(FVector(0,0,100), false, nullptr, ETeleportType::TeleportPhysics);
        PC->SetControlRotation(FRotator(0,-90,0));
        PC->ConsoleCommand(TEXT("ERBlockShot 100 10"));
        Check(Drywall->Health < 500 && Room->GetInstanceCount() == 504, TEXT("penetration_hits_rear_target_without_breach"));
        const float Previous = Drywall->Health;
        const int32 Ammo = Character->Weapon->MagazineAmmo;
        Character->Weapon->StartFire(); Character->Weapon->StopFire();
        // Preset: base Damage 36 structural removes the 20 HP drywall cell in one hit,
        // and only the cells it crosses (two when real spread meets a seam);
        // the bullet still reaches the rear target.
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_PROBE_WEAPON_SHOT target=%.1f->%.1f ammo=%d->%d instances=%d damage=%.1f %s"),Previous,Drywall->Health,Ammo,
            Character->Weapon->MagazineAmmo,Room->GetInstanceCount(),Character->Weapon->Data ? Character->Weapon->Data->Damage : -1.f,*Room->LastCombat);
        Check(Drywall->Health < Previous && Character->Weapon->MagazineAmmo == Ammo - 1 && Room->GetInstanceCount() >= 502 && Room->GetInstanceCount() <= 503, TEXT("authorized_weapon_removes_only_crossed_drywall_cells_with_one_cartridge"));
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Destruction-Penetration.png"), true, false);
        Phase = 1; Elapsed = 0;
    }
    else if (Phase == 1 && Elapsed > .5f)
    {
        PC->ConsoleCommand(TEXT("ERBlockRegenerate"));
        PC->SetControlRotation(FRotator(0,0,0)); PC->ConsoleCommand(TEXT("ERBlockShot 100 10"));
        Check(Brick->Health == 500, TEXT("brick_stops_default_energy"));
        PC->SetControlRotation(FRotator(0,90,0)); PC->ConsoleCommand(TEXT("ERBlockShot 100 10"));
        Check(Concrete->Health == 500, TEXT("concrete_stops_default_energy"));
        PC->ConsoleCommand(TEXT("ERBlockRegenerate"));
        PC->SetControlRotation(FRotator(0,-90,0)); PC->ConsoleCommand(TEXT("ERBlockExplosion 250 180"));
        Check(Room->BlockWorld->GetDestroyedCount() > 20 && Room->LastGroupsRebuilt < Room->BlockWorld->GetDestroyedCount() && Room->ValidateMappings(), TEXT("console_explosion_opens_batch_breach"));
        Character->SetActorLocation(FVector(0,-150,100), false, nullptr, ETeleportType::TeleportPhysics);
        Character->GetCharacterMovement()->StopMovementImmediately();
        Phase = 2; Elapsed = 0;
    }
    else if (Phase == 2)
    {
        Character->AddMovementInput(FVector(0,-1,0), 1);
        if (Elapsed > 1.5f)
        {
            Check(Character->GetActorLocation().Y < -300 && Character->GetActorLocation().Z > 90, TEXT("actual_pawn_crosses_explosion_breach"));
            Character->GetCharacterMovement()->StopMovementImmediately();
            PC->SetControlRotation(FRotator(0,90,0)); Phase = 3; Elapsed = 0;
        }
    }
    else if (Phase == 3 && Elapsed > .5f)
    {
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Destruction-Explosion.png"), true, false);
        Phase = 4; Elapsed = 0;
    }
    else if (Phase == 4 && Elapsed > .5f)
    {
        PC->ConsoleCommand(TEXT("ERBlockRegenerate"));
        BaselineBlastDestroyed=AERBlockRoom::ApplyWorldExplosion(GetWorld(),FVector(100,-230,110),300,100,nullptr).Destroyed;
        PC->ConsoleCommand(TEXT("ERBlockRegenerate"));
        auto* Grenade = GetWorld()->SpawnActorDeferred<AERProjectile>(Character->Weapon->ProjectileClass, FTransform(FVector(100,-230,100)), Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Grenade)
        {
            Grenade->Payload = EERPayload::HE; Grenade->Damage = 100; Grenade->ExplosionRadius = 300; Grenade->FuseDuration = .2f;
            Grenade->Movement->InitialSpeed=0;Grenade->Movement->MaxSpeed=0;Grenade->Movement->Velocity=FVector::ZeroVector;
            UGameplayStatics::FinishSpawningActor(Grenade, Grenade->GetActorTransform());
            Grenade->Movement->ProjectileGravityScale=0; // Compare both blasts at exactly the same origin.
        }
        else Check(false, TEXT("real_grenade_spawn"));
        Phase = 5; Elapsed = 0;
    }
    else if (Phase == 5 && Elapsed > 1)
    {
        Check(Room->BlockWorld->GetDestroyedCount() > 10 && Room->ValidateMappings(), TEXT("real_he_fuse_damages_block_world"));
        // The HE structural profile is local: nothing changes at or beyond its outer radius.
        const FVector Blast(100,-230,110); bool Local=true;
        for (const auto& Block : Room->BlockWorld->GetBlocks())
            if (Block.Health < Room->MaterialCatalog->Find(Block.Material)->MaxHealth || !Block.IsAttached())
                Local &= FVector::Distance(Blast, Room->GetBlockTransform(Block.Id).GetLocation()) < FERBlockExplosionProfile().OuterRadius;
        Check(Local,TEXT("grenade_structural_profile_stays_within_outer_radius"));
        UE_LOG(LogTemp,Display,TEXT("ER_GRENADE_STRUCTURAL_COMPARE legacy_linear=%d profile=%d"),BaselineBlastDestroyed,Room->BlockWorld->GetDestroyedCount());
        PC->ConsoleCommand(TEXT("ERBlockRegenerate"));
        Check(Room->GetInstanceCount() == 504 && Drywall->Health == 500 && Brick->Health == 500 && Concrete->Health == 500, TEXT("combat_regeneration_resets_targets_and_blocks"));
        UE_LOG(LogTemp, Display, TEXT("ER_BLOCK_PROBE_COMPLETE result=%s"), Failed ? TEXT("FAIL") : TEXT("PASS"));
        SetActorTickEnabled(false); FPlatformMisc::RequestExitWithStatus(false, Failed ? 1 : 0);
    }
}
