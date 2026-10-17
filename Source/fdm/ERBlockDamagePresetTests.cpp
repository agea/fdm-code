#include "ERBlockCombat.h"
#include "ERBlockRoom.h"
#include "ERBlockTarget.h"
#include "ERProceduralMap.h"
#include "Engine/World.h"
#include "Algo/Reverse.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
UWorld* CreatePresetWorld()
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    return UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
}
FERBlockShotResult FireAt(UWorld* World, FVector Start, FVector Direction, float CharacterDamage, float Structural, float Energy = 100)
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERBlockPresetTest), false);
    FHitResult Hit;
    World->LineTraceSingleByChannel(Hit, Start, Start + Direction * 1000, ECC_GameTraceChannel1, Query);
    FERBlockShotSettings Settings; Settings.Energy = Energy;
    Settings.bOverrideStructuralDamage = true; Settings.StructuralDamage = Structural;
    return ERBlockCombat::ContinueShot(World, Hit, Start, Direction, 1000, CharacterDamage, Settings, Query, nullptr, nullptr);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockDamagePresetTest, "EclipseRecoil.Destruction.DamagePreset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FERBlockDamagePresetTest::RunTest(const FString& Parameters)
{
    // 1. Preset: IDs 0-6 keep their types and adopt the table values.
    const float HP[] = {20,100,600,40,1000,100,8};
    const float Bullet[] = {1,.75f,.1f,1,.05f,.5f,1};
    const float Blast[] = {2,1.5f,.5f,1.5f,.2f,1,3};
    const auto* Native = GetDefault<UERBlockMaterialCatalog>();
    TestTrue(TEXT("native preset valid"), Native->IsValidCatalog());
    const auto* Saved = LoadObject<UERBlockMaterialCatalog>(nullptr, TEXT("/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.DA_ER_BlockMaterials"));
    TestNotNull(TEXT("saved project catalog"), Saved);
    const TCHAR* Surfaces[] = {TEXT("M_ER_PBR_Plaster001"),TEXT("M_ER_PBR_red_brick_03"),TEXT("M_ER_PBR_concrete_wall_006"),TEXT("M_ER_PBR_wood_planks"),
        TEXT("M_ER_PBR_stone_wall_02"),TEXT("M_ER_PBR_rusty_metal_04"),TEXT("M_ER_Glass")};
    const TCHAR* Decals[] = {TEXT("M_ER_BlockImpact_Drywall"),TEXT("M_ER_BlockImpact_Brick"),TEXT("M_ER_BlockImpact_Concrete")};
    for (int32 I = 0; I < 7; ++I)
        for (const auto* Catalog : {Native, Saved})
        {
            if (!Catalog) continue;
            const auto* P = Catalog->Find(static_cast<EERBlockMaterial>(I));
            if (!TestNotNull(TEXT("profile for every ID"), P)) continue;
            TestTrue(FString::Printf(TEXT("preset values for ID %d"), I), P->MaxHealth == HP[I] && P->BulletDamageScale == Bullet[I] && P->ExplosionDamageScale == Blast[I]);
            if (Catalog != Saved) continue;
            TestTrue(FString::Printf(TEXT("surface kept for ID %d"), I), P->Surface && P->Surface->GetName() == Surfaces[I]);
            if (I < 3) TestTrue(FString::Printf(TEXT("decal kept for ID %d"), I), P->ImpactDecal && P->ImpactDecal->GetName() == Decals[I]);
        }

    // 3. Bullets: 36 structural points per hit on an intact cell, no truncation.
    const int32 Shots[] = {1,4,0,2,0,6,1};
    for (int32 I = 0; I < 7; ++I)
    {
        const auto* P = Native->Find(static_cast<EERBlockMaterial>(I));
        int32 Count = 0;
        for (float Health = P->MaxHealth; Health > 0 && Count < 1000; ++Count) Health = FMath::Max(0.f, Health - 36 * P->BulletDamageScale);
        if (Shots[I]) TestEqual(FString::Printf(TEXT("hits to remove ID %d"), I), Count, Shots[I]);
    }
    TestTrue(TEXT("concrete 3.6 / stone 1.8 per impact"), FMath::IsNearlyEqual(36 * Native->Find(EERBlockMaterial::Concrete)->BulletDamageScale, 3.6f, 1.e-5f) &&
        FMath::IsNearlyEqual(36 * Native->Find(EERBlockMaterial::Stone)->BulletDamageScale, 1.8f, 1.e-5f));

    // 4. Grenade falloff and full-exposure maxima.
    const FERBlockExplosionProfile Grenade;
    TestTrue(TEXT("grenade profile 400/0/25/200/2"), Grenade.BaseDamage == 400 && Grenade.MinimumDamage == 0 && Grenade.InnerRadius == 25 && Grenade.OuterRadius == 200 && Grenade.FalloffExponent == 2);
    TestEqual(TEXT("F(0)"), ERBlockCombat::ExplosionFalloff(Grenade, 0), 1.);
    TestEqual(TEXT("F(25)"), ERBlockCombat::ExplosionFalloff(Grenade, 25), 1.);
    TestTrue(TEXT("F(112.5)"), FMath::IsNearlyEqual(ERBlockCombat::ExplosionFalloff(Grenade, 112.5), .25, 1.e-9));
    TestEqual(TEXT("F(200)"), ERBlockCombat::ExplosionFalloff(Grenade, 200), 0.);
    TestEqual(TEXT("F(250)"), ERBlockCombat::ExplosionFalloff(Grenade, 250), 0.);
    const double Maxima[] = {800,600,200,600,80,400,1200};
    for (int32 I = 0; I < 7; ++I)
    {
        const auto* P = Native->Find(static_cast<EERBlockMaterial>(I));
        TestTrue(FString::Printf(TEXT("max blast ID %d"), I), FMath::IsNearlyEqual(ERBlockCombat::ExplosionCellDamage(Grenade, 10, P->ExplosionDamageScale, 1), Maxima[I], 1.e-3));
        TestEqual(FString::Printf(TEXT("no blast at 200cm ID %d"), I), ERBlockCombat::ExplosionCellDamage(Grenade, 200, P->ExplosionDamageScale, 1), 0.);
    }
    TestTrue(TEXT("exposure scales damage"), FMath::IsNearlyEqual(ERBlockCombat::ExplosionCellDamage(Grenade, 10, 2, .25), 200., 1.e-6));
    TestEqual(TEXT("no exposure, no damage"), ERBlockCombat::ExplosionCellDamage(Grenade, 10, 2, 0), 0.);
    const auto Legacy = FERBlockExplosionProfile::Linear(220, 180);
    TestTrue(TEXT("legacy linear envelope"), FMath::IsNearlyEqual(ERBlockCombat::ExplosionCellDamage(Legacy, 110, 1, 1), 90., 1.e-6));
    FERBlockExplosionProfile Invalid; Invalid.InnerRadius = 300;
    TestEqual(TEXT("invalid profile rejected"), ERBlockCombat::ExplosionCellDamage(Invalid, 10, 1, 1), 0.);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockDamageLocalTest, "EclipseRecoil.Destruction.DamageLocalAndShielding", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FERBlockDamageLocalTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreatePresetWorld();
    if (!TestNotNull(TEXT("Physics world"), World)) return false;
    auto* Room = World->SpawnActor<AERBlockRoom>();
    auto* Target = World->SpawnActor<AERBlockTarget>();
    Room->Regenerate();
    const FERBlockId Drywall{0,5,6}, Brick{1,5,6}, Concrete{2,5,6};
    auto Health = [&](AERBlockRoom* R, const FERBlockId& Id) { return R->BlockWorld->Find(Id)->Health; };

    // 2. Local damage: one cell only, never a shared wall pool or neighbours.
    TestTrue(TEXT("overkill on one cell"), Room->ApplyBlockDamage(Drywall, 1000));
    TestEqual(TEXT("only that cell removed"), Room->BlockWorld->GetDestroyedCount(), 1);
    for (const FERBlockId& Neighbour : {FERBlockId{0,4,6}, FERBlockId{0,6,6}, FERBlockId{0,5,5}, FERBlockId{0,5,7}})
        TestEqual(TEXT("neighbour keeps full HP"), Health(Room, Neighbour), 20.f);

    // 3/6. Real shots: structural damage is separate from character damage.
    Room->Regenerate(); Target->ResetHealth(); Target->SetActorLocation(FVector(25,-350,162.5));
    auto Shot = FireAt(World, FVector(25,0,162.5), FVector(0,-1,0), 10, 36);
    TestTrue(TEXT("36 structural removes drywall in one hit"), Room->BlockWorld->Find(Drywall)->State == EERBlockState::Destroyed);
    TestTrue(TEXT("rear target uses character damage, not structural"), Shot.TerminalHit.GetActor() == Target && FMath::IsNearlyEqual(Target->Health, 491.5f, .01f));
    for (int32 I = 1; I <= 4; ++I)
    {
        FireAt(World, FVector(0,25,162.5), FVector(1,0,0), 10, 36);
        if (I < 4) TestTrue(TEXT("brick loses 27 per hit"), FMath::IsNearlyEqual(Health(Room, Brick), 100.f - 27 * I, .001f));
    }
    TestTrue(TEXT("brick removed on the fourth hit"), Room->BlockWorld->Find(Brick)->State == EERBlockState::Destroyed);
    FireAt(World, FVector(25,0,162.5), FVector(0,1,0), 10, 36);
    TestTrue(TEXT("concrete keeps fractional damage"), FMath::IsNearlyEqual(Health(Room, Concrete), 596.4f, .001f));
    FERBlockShotSettings Default;
    TestEqual(TEXT("structural starts from weapon damage"), Default.GetStructuralDamage(36), 36.f);

    // 6. Processing order does not change the result.
    Room->Regenerate();
    TArray<FERBlockDamageRequest> Requests;
    for (const auto& Block : Room->BlockWorld->GetBlocks()) Requests.Add({Block.Id, float(Block.Id.Column % 7) * 4.5f + Block.Id.Row, false});
    Room->ApplyDamageBatch(Requests);
    const uint32 Forward = Room->GetStateDigest();
    Room->Regenerate();
    Algo::Reverse(Requests);
    Room->ApplyDamageBatch(Requests);
    TestEqual(TEXT("reversed order, same state"), Room->GetStateDigest(), Forward);
    Room->Destroy(); Target->Destroy();

    // 4/5. Grenade on a single wall versus a four-layer wall. Rooms offset by
    // 20cm stack their drywall wall 0 into consecutive layers; large HP make
    // every delivered point measurable.
    const FERBlockExplosionProfile Grenade;
    const FVector Origin(25,-290,162.5);
    TArray<AERBlockRoom*> Layers;
    for (int32 L = 0; L < 4; ++L)
    {
        auto* Layer = World->SpawnActor<AERBlockRoom>(FVector(0, 20 * L, 0), FRotator::ZeroRotator);
        Layer->MaterialCatalog->Materials[0].MaxHealth = 1.e6f;
        Layer->Regenerate(); Layers.Add(Layer);
    }
    const auto Batch = AERBlockRoom::ApplyWorldExplosion(World, Origin, Grenade);
    const float Transmission = Layers[0]->MaterialCatalog->Find(EERBlockMaterial::Drywall)->ExplosionTransmission;
    for (int32 L = 0; L < 4; ++L)
    {
        const double Distance = FVector::Distance(Origin, Layers[L]->GetBlockTransform(Drywall).GetLocation());
        const double Expected = ERBlockCombat::ExplosionCellDamage(Grenade, Distance, 2, FMath::Pow(double(Transmission), L));
        TestTrue(FString::Printf(TEXT("layer %d receives attenuated exposure"), L), FMath::IsNearlyEqual(1.e6 - Health(Layers[L], Drywall), Expected, .05));
    }
    TestTrue(TEXT("front layer at 30cm close to full"), FMath::IsNearlyEqual(1.e6 - Health(Layers[0], Drywall), 800 * FMath::Square(170. / 175), .05));
    TestTrue(TEXT("rear cells shielded, not equal to exposed"), Batch.Attenuated > 0 && 1.e6 - Health(Layers[3], Drywall) < .2 * (1.e6 - Health(Layers[0], Drywall)));
    for (auto* Layer : Layers)
        for (const auto& Block : Layer->BlockWorld->GetBlocks())
            if (FVector::Distance(Origin, Layer->GetBlockTransform(Block.Id).GetLocation()) >= 200 && Block.Health != Layer->MaterialCatalog->Find(Block.Material)->MaxHealth)
                AddError(TEXT("damage at or beyond 200cm"));
    AddInfo(FString::Printf(TEXT("ER_BLOCK_PRESET_MULTILAYER candidates=%d covered=%d attenuated=%d ms=%.2f"), Batch.Candidates, Batch.Occluded, Batch.Attenuated, Batch.TotalMs));

    // Real preset HP: one layer is breached; four layers keep their rear.
    for (auto* Layer : Layers) { Layer->MaterialCatalog->Materials[0].MaxHealth = 20; Layer->Regenerate(); }
    for (int32 L = 1; L < 4; ++L) Layers[L]->SetActorLocation(FVector(0, 5000 + 1000 * L, 0));
    const int32 Single = AERBlockRoom::ApplyWorldExplosion(World, Origin, Grenade).Destroyed;
    for (int32 L = 0; L < 4; ++L) { Layers[L]->SetActorLocation(FVector(0, 20 * L, 0)); Layers[L]->Regenerate(); }
    AERBlockRoom::ApplyWorldExplosion(World, Origin, Grenade);
    const int32 Front = Layers[0]->BlockWorld->GetDestroyedCount(), Rear = Layers[3]->BlockWorld->GetDestroyedCount();
    TestTrue(TEXT("single layer breached, front layer of the stack matches it"), Single > 0 && Front == Single);
    TestTrue(TEXT("four layers are not a single layer"), Rear < Front);
    for (auto* Layer : Layers) TestTrue(TEXT("layer mappings"), Layer->ValidateMappings());

    // 2/4. Concrete keeps 400 HP after a full-exposure grenade at <=25cm.
    for (auto* Layer : Layers) Layer->Destroy();
    Room = World->SpawnActor<AERBlockRoom>(); Room->Regenerate();
    AERBlockRoom::ApplyWorldExplosion(World, FVector(25,275,162.5), Grenade);
    TestTrue(TEXT("concrete cell at 15cm: 600 - 200"), FMath::IsNearlyEqual(Health(Room, Concrete), 400.f, .01f));
    TestTrue(TEXT("no automatic whole-wall demolition"), Room->BlockWorld->GetDestroyedCount() == 0);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockGlassPaneTest, "EclipseRecoil.Destruction.GlassPane", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FERBlockGlassPaneTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreatePresetWorld();
    if (!TestNotNull(TEXT("Physics world"), World)) return false;
    auto* Map = World->SpawnActor<AERProceduralMap>();
    Map->LayoutConfig.RoomCount = 6; Map->LayoutConfig.WindowGlass = true; Map->LayoutConfig.AdditionalMaterials = true;
    TestTrue(TEXT("map with window panes"), Map->Regenerate());
    const auto* First = Map->BlockWorld->GetBlocks().FindByPredicate([](const auto& R) { return R.Material == EERBlockMaterial::Glass; });
    if (!TestNotNull(TEXT("glass section"), First)) { World->DestroyWorld(false); return false; }
    const FERBlockId Seed = First->Id;
    const TArray<FERBlockId> Pane = Map->BlockWorld->CollectGlassPane(Seed);
    int32 Glass = 0, OtherAttached = 0;
    for (const auto& R : Map->BlockWorld->GetBlocks()) { Glass += R.Material == EERBlockMaterial::Glass; OtherAttached += R.Material != EERBlockMaterial::Glass; }
    TestTrue(TEXT("pane has several sections, not every window"), Pane.Num() > 1 && Pane.Num() < Glass);
    TestTrue(TEXT("sub-threshold hit keeps the pane"), Map->ApplyBlockDamage(Seed, 7) && Map->BlockWorld->GetDestroyedCount() == 0);
    Map->ApplyBlockDamage(Seed, 1);
    for (const auto& Id : Pane) TestTrue(TEXT("whole pane breaks"), Map->BlockWorld->Find(Id)->State == EERBlockState::Destroyed);
    TestEqual(TEXT("only that pane removed"), Map->BlockWorld->GetDestroyedCount(), Pane.Num());
    int32 Remaining = 0;
    for (const auto& R : Map->BlockWorld->GetBlocks()) Remaining += R.Material != EERBlockMaterial::Glass && R.IsAttached();
    TestEqual(TEXT("pane logic never spreads to other materials"), Remaining, OtherAttached);
    const auto* Brick = Map->BlockWorld->GetBlocks().FindByPredicate([](const auto& R) { return R.Material == EERBlockMaterial::Brick; });
    if (Brick) TestEqual(TEXT("non-glass has no pane"), Map->BlockWorld->CollectGlassPane(Brick->Id).Num(), 0);
    TestTrue(TEXT("pane mappings"), Map->ValidateMappings());
    World->DestroyWorld(false);
    return true;
}
#endif
