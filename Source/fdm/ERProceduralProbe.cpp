#include "ERProceduralProbe.h"
#include "ERProceduralMap.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERPlayerController.h"
#include "ERMatch.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
AERProceduralProbe::AERProceduralProbe() { PrimaryActorTick.bCanEverTick=true; }
void AERProceduralProbe::Check(bool Value,const TCHAR* Label)
{
    Failed|=!Value;UE_LOG(LogTemp,Display,TEXT("ER_PROCEDURAL_PROBE_%s %s"),Value ? TEXT("PASS") : TEXT("FAIL"),Label);
}
void AERProceduralProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);Elapsed+=DeltaSeconds;
    TActorIterator<AERProceduralMap> Iterator(GetWorld());auto* Map=Iterator ? *Iterator : nullptr;
    auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    auto* C=PC ? Cast<AERCharacter>(PC->GetPawn()) : nullptr;
    if (!Map || !C || !C->Weapon) { if (Elapsed>30) {Check(false,TEXT("initialized"));FPlatformMisc::RequestExitWithStatus(false,1);} return; }
    auto Move=[&](FVector P) { C->SetActorLocation(P,false,nullptr,ETeleportType::TeleportPhysics);C->GetCharacterMovement()->StopMovementImmediately(); };
    if (Phase==0 && Elapsed>3)
    {
        Check(GetWorld()->GetAuthGameMode<AERGameMode>()->IsMatchInProgress(),TEXT("standalone_firing_enabled"));
        Check(Map->GetLayout().Rooms.Num()==8 && Map->ValidateMappings(),TEXT("eight_connected_rooms_initialized"));
        UE_LOG(LogTemp,Display,TEXT("ER_PROCEDURAL_SPAWN pawn=%s expected=%s"),*C->GetActorLocation().ToString(),*Map->GetSpawnLocation().ToString());
        Check(C->GetActorLocation().Z>90 && C->GetActorLocation().Z<101 && FVector::Dist2D(C->GetActorLocation(),Map->GetSpawnLocation())<5,TEXT("pawn_free_spawn_and_floor"));
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Procedural-Room.png"),true,false);
        const auto& Route=Map->GetLayout().Routes[0];Move(Route[0]);Destination=Route.Last();
        PC->SetControlRotation(FRotator(0,-90,0));Phase=1;Elapsed=0;
    }
    else if (Phase==1)
    {
        if (C->GetActorLocation().Y>Destination.Y+20) C->AddMovementInput(FVector(0,-1,0),1);
        if (Elapsed>6)
        {
            Check(FMath::Abs(C->GetActorLocation().Y-Destination.Y)<80 && C->GetActorLocation().Z>90,TEXT("actual_pawn_crosses_room_and_hall_doors"));
            C->GetCharacterMovement()->StopMovementImmediately();PC->SetControlRotation(FRotator(0,0,0));
            if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Procedural-Corridor.png"),true,false);
            Phase=2;Elapsed=0;
        }
    }
    else if (Phase==2 && Elapsed>1)
    {
        PC->ConsoleCommand(TEXT("ERMapGenerate 7919 10"));
        Check(Map->LayoutConfig.Seed==7919 && Map->GetLayout().Rooms.Num()==10 && Map->ValidateMappings(),TEXT("console_seed_and_room_count"));
        Check(FVector::Dist(C->GetActorLocation(),Map->GetSpawnLocation())<5,TEXT("regeneration_relocates_to_safe_spawn"));
        const auto& Room=Map->GetLayout().Rooms[0];
        // Aim at the side wall away from doors/windows; exercise the real weapon.
        Move(FVector((Room.Min.X+Room.Max.X)*.5f,(Room.Min.Y+Room.Max.Y)*.5f,100));PC->SetControlRotation(FRotator(0,0,0));
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(ERProceduralProbe),false,C);FERBlockId Id;
        const FVector Eye=C->Camera->GetComponentLocation();
        const bool Found=GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+FVector(1000,0,0),ECC_GameTraceChannel1,Query) && Map->ResolveHit(Hit,Id);
        Check(Found,TEXT("procedural_wall_hit_identity"));
        if (Found) Map->ApplyBlockDamage(Id,Map->BlockWorld->Find(Id)->Health-1);
        const uint32 Shots=C->Weapon->AuthorizedShotCount;
        C->Weapon->StartFire();C->Weapon->StopFire();
        Check(C->Weapon->AuthorizedShotCount==Shots+1 && Map->BlockWorld->GetDestroyedCount()>0,TEXT("existing_weapon_destroys_procedural_block"));
        PC->SetControlRotation(FRotator(-10,0,0));
        PC->ConsoleCommand(TEXT("ERBlockExplosion 250 2400"));
        Check(Map->BlockWorld->GetDestroyedCount()>1 && Map->ValidateMappings(),TEXT("shared_explosion_batch_on_procedural_wall"));
        if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/Procedural-Breach.png"),true,false);
        Phase=3;Elapsed=0;
    }
    else if (Phase==3)
    {
        C->AddMovementInput(FVector(1,0,0),1);
        if (Elapsed<=2) return;
        const auto& Room=Map->GetLayout().Rooms[0];
        Check(C->GetActorLocation().X>Room.Max.X+35 && C->GetActorLocation().Z>65,TEXT("actual_pawn_crosses_explosion_breach_onto_asphalt"));
        const int32 Instances=Map->GetInstanceCount();PC->ConsoleCommand(TEXT("ERMapGenerate 1 11"));
        Check(Map->GetInstanceCount()==Instances && Map->LayoutConfig.Seed==7919,TEXT("invalid_request_preserves_live_map"));
        PC->ConsoleCommand(TEXT("ERBlockRegenerate"));
        Check(Map->BlockWorld->GetDestroyedCount()==0 && Map->ValidateMappings(),TEXT("regeneration_resets_damage"));
        UE_LOG(LogTemp,Display,TEXT("ER_PROCEDURAL_PROBE_COMPLETE result=%s %s"),Failed ? TEXT("FAIL") : TEXT("PASS"),*Map->GetStatistics());
        FPlatformMisc::RequestExitWithStatus(false,Failed ? 1 : 0);Phase=4;
    }
}
