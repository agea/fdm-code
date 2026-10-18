#include "ERBlockCombat.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
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

#endif
