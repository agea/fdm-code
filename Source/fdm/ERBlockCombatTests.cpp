#include "ERBlockCombat.h"
#include "ERBlockRoom.h"
#include "ERBlockTarget.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockCombatTest, "EclipseRecoil.Destruction.BallisticsAndExplosion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FERBlockCombatTest::RunTest(const FString& Parameters)
{
    double Entry = 0, Exit = 0;
    const FTransform Box(FRotator::ZeroRotator, FVector(25,-260,162.5), FVector(.5,.2,.25));
    TestTrue(TEXT("Normal block path"), ERBlockCombat::IntersectBlock(Box, FVector(25,0,162.5), FVector(0,-1,0), 1000, Entry, Exit));
    TestTrue(TEXT("20cm normal thickness"), FMath::IsNearlyEqual(Exit - Entry, 20., .001));
    const FVector Oblique = FVector(.5,-FMath::Sqrt(.75),0);
    TestTrue(TEXT("Oblique block path"), ERBlockCombat::IntersectBlock(Box, Box.GetLocation() - Oblique * 200, Oblique, 1000, Entry, Exit));
    TestTrue(TEXT("Actual oblique thickness"), FMath::IsNearlyEqual(Exit - Entry, 20 / FMath::Sqrt(.75), .001));
    TestTrue(TEXT("Starting inside has finite exit"), ERBlockCombat::IntersectBlock(Box, Box.GetLocation(), FVector(0,-1,0), 1000, Entry, Exit) && Entry == 0 && FMath::IsNearlyEqual(Exit, 10., .001));
    FTransform Scaled(FRotator(0,90,0), FVector(1000,1000,100), FVector(.5,.4,.25));
    const FVector ScaledRay = Scaled.GetRotation().RotateVector(FVector(0,1,0));
    TestTrue(TEXT("Rotated/scaled volume"), ERBlockCombat::IntersectBlock(Scaled, Scaled.GetLocation() - ScaledRay * 100, ScaledRay, 1000, Entry, Exit) && FMath::IsNearlyEqual(Exit - Entry, 40., .001));
    TestFalse(TEXT("Parallel outside ray"), ERBlockCombat::IntersectBlock(Box, FVector(100,0,162.5), FVector(0,-1,0), 1000, Entry, Exit));
    TestFalse(TEXT("Zero direction"), ERBlockCombat::IntersectBlock(Box, FVector::ZeroVector, FVector::ZeroVector, 1000, Entry, Exit));

    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Physics world"), World)) return false;
    auto* Room = World->SpawnActor<AERBlockRoom>();
    auto* Target = World->SpawnActor<AERBlockTarget>();
    if (!TestNotNull(TEXT("Room"), Room) || !TestNotNull(TEXT("Target"), Target)) { World->DestroyWorld(false); return false; }
    Room->Regenerate();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERBlockCombatTest), false);
    auto Fire = [&](FVector Start, FVector Direction, float Energy = 100, float Range = 1000, int32 Limit = 8)
    {
        FHitResult Hit;
        World->LineTraceSingleByChannel(Hit, Start, Start + Direction * Range, ECC_GameTraceChannel1, Query);
        FERBlockShotSettings Settings; Settings.Energy = Energy; Settings.MaxPenetrations = Limit;
        return ERBlockCombat::ContinueShot(World, Hit, Start, Direction, Range, 10, Settings, Query, nullptr, nullptr);
    };
    auto Reset = [&]() { Room->Regenerate(); Target->ResetHealth(); };
    FERBlockId Drywall; Drywall.Wall = 0; Drywall.Column = 5; Drywall.Row = 6;
    Target->SetActorLocation(FVector(25,-350,162.5));
    auto Shot = Fire(FVector(25,0,162.5), FVector(0,-1,0));
    TestEqual(TEXT("Cross one drywall block"), Shot.Penetrations, 1);
    TestTrue(TEXT("Exit used once, target reached"), Shot.ExtraTraces == 1 && Shot.TerminalHit.GetActor() == Target);
    TestTrue(TEXT("Residual energy and actual damage to rear target"), FMath::IsNearlyEqual(Shot.RemainingEnergy, 85.f, .01f) && FMath::IsNearlyEqual(Target->Health, 491.5f, .01f));
    TestEqual(TEXT("Penetration preserves damaged wall"), Room->BlockWorld->Find(Drywall)->Health, 25.f);
    FHitResult Hit;
    TestTrue(TEXT("Intact collision still stops an ordinary trace"), World->LineTraceSingleByChannel(Hit, FVector(25,0,162.5), FVector(25,-350,162.5), ECC_GameTraceChannel1, Query) && Hit.GetActor() == Room);
    TestEqual(TEXT("No physical breach from low damage penetration"), Room->GetInstanceCount(), 504);
    Reset();
    Shot = Fire(FVector(25,0,162.5), FVector(0,-1,0), 100, 265);
    TestTrue(TEXT("Range ending inside the wall never reaches target"), Shot.ExtraTraces == 0 && Target->Health == 500);
    Reset();
    Shot = Fire(FVector(25,-260,162.5), FVector(0,-1,0));
    TestTrue(TEXT("Start inside terminates and hits rear target once"), Shot.ExtraTraces <= 1 && Shot.BlockHits <= 1 && Target->Health < 500);
    Reset();
    Shot = Fire(FVector(20,-249.9,162.5), FVector(1,-.01,0).GetSafeNormal());
    TestTrue(TEXT("Grazing ray is bounded"), Shot.Penetrations == 0 && Shot.ExtraTraces == 0);
    Reset(); Target->SetActorLocation(FVector(350,25,162.5)); Target->SetActorRotation(FRotator(0,90,0));
    Shot = Fire(FVector(0,25,162.5), FVector(1,0,0));
    TestTrue(TEXT("Brick stops default energy"), Shot.Penetrations == 0 && Target->Health == 500);
    Reset(); Shot = Fire(FVector(0,25,162.5), FVector(1,0,0), 200);
    TestTrue(TEXT("Higher energy penetrates brick with reduced rear damage"), Shot.Penetrations == 1 && FMath::IsNearlyEqual(Target->Health, 496.f, .01f));
    Reset(); Target->SetActorLocation(FVector(25,350,162.5)); Target->SetActorRotation(FRotator::ZeroRotator);
    Shot = Fire(FVector(25,0,162.5), FVector(0,1,0));
    TestTrue(TEXT("Concrete stops default energy"), Shot.Penetrations == 0 && Target->Health == 500);

    Reset();
    FERBlockId Surface = Drywall;
    TestTrue(TEXT("Spatial query includes a near-face block even across centre cells"), Room->BlockWorld->QuerySphere(FVector(25,-250,162.5), 1).Contains(Surface));
    auto Batch = Room->ApplyDamageBatch({{Surface, 3, false}, {Surface, 4, false}});
    TestTrue(TEXT("Duplicate events aggregated with no unnecessary rebuild"), Batch.Damaged == 1 && Batch.GroupsRebuilt == 0 && Room->BlockWorld->Find(Surface)->Health == 23);
    TestEqual(TEXT("Reject invalid explosion radius"), AERBlockRoom::ApplyWorldExplosion(World, FVector(25,-240,162.5), -1, 100).Damaged, 0);
    Reset();
    Batch = AERBlockRoom::ApplyWorldExplosion(World, FVector(25,-240,100), 220, 180);
    TestTrue(TEXT("Blast creates a multi-block breach"), Batch.Destroyed > 20 && Batch.Damaged >= Batch.Destroyed);
    TestTrue(TEXT("One update for each of two affected spatial/material groups"), Batch.GroupsRebuilt == 2 && Room->LastGroupsRebuilt == 2);
    TestTrue(TEXT("Batch mapping matches collision"), Room->ValidateMappings());
    TestFalse(TEXT("Capsule passes blast breach"), World->SweepSingleByChannel(Hit, FVector(25,0,100), FVector(25,-320,100), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(34,92.5f), Query));
    TestTrue(TEXT("Floor survives explosion"), World->LineTraceSingleByChannel(Hit, FVector(0,0,50), FVector(0,0,-50), ECC_GameTraceChannel1, Query) && Hit.GetComponent() == Room->Floor);

    Reset();
    auto* Shielded = World->SpawnActor<AERBlockRoom>();
    Shielded->SetActorLocation(FVector(0,600,0)); Shielded->Regenerate();
    Batch = AERBlockRoom::ApplyWorldExplosion(World, FVector(25,200,162.5), 250, 2000);
    TestTrue(TEXT("Blast destroys exposed concrete"), Room->BlockWorld->GetDestroyedCount() > 0);
    TestTrue(TEXT("Concrete shields rear drywall against the same explosion, even if removed"), Batch.Occluded > 0 && Shielded->BlockWorld->Find(Surface)->Health == 30);
    AERBlockRoom::ApplyWorldExplosion(World, FVector(25,200,162.5), 250, 2000);
    TestTrue(TEXT("A later blast can use the newly opened breach"), Shielded->BlockWorld->Find(Surface)->Health == 0);
    Shielded->Destroy();

    Reset();
    auto* Second = World->SpawnActor<AERBlockRoom>();
    Second->SetActorLocation(FVector(0,-600,0)); Second->Regenerate();
    Target->SetActorLocation(FVector(25,-500,162.5));
    Shot = Fire(FVector(25,0,162.5), FVector(0,-1,0), 1000, 1000, 1);
    TestTrue(TEXT("Crossing cap stops before the next block; room components are never ignored"), Shot.bLimited && Shot.Penetrations == 1 && Shot.BlockHits == 1 && Target->Health == 500);
    TestTrue(TEXT("Second block untouched at crossing cap"), Second->BlockWorld->GetBlocks().ContainsByPredicate([](const auto& Block) { return Block.Id.Wall == 2 && Block.Health == 300; }));
    TestTrue(TEXT("Final mappings valid"), Room->ValidateMappings() && Second->ValidateMappings());
    World->DestroyWorld(false);
    return true;
}
#endif
