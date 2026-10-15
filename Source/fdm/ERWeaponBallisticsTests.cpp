#include "ERWeaponBallistics.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERWeaponBallisticsTest, "EclipseRecoil.Weapons.ReferenceBallistics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FERWeaponBallisticsTest::RunTest(const FString&)
{
    TestEqual(TEXT("Source 500 units maps to 1270cm"), ERWeaponBallistics::AtRange(100,.85f,1270),85.f);
    TestEqual(TEXT("Zero range preserves source damage"), ERWeaponBallistics::AtRange(53,.85f,0),53.f);
    TestTrue(TEXT("Two range intervals compound"), FMath::IsNearlyEqual(ERWeaponBallistics::AtRange(100,.85f,2540),72.25f));
    float Armor = 100;
    TestTrue(TEXT("Desert Eagle source armor ratio"), FMath::IsNearlyEqual(ERWeaponBallistics::AbsorbArmor(53,1.864f,Armor),49.396f));
    TestTrue(TEXT("Armor consumes half prevented health damage"), FMath::IsNearlyEqual(Armor,98.198f));
    Armor=1;
    TestEqual(TEXT("Insufficient armor cannot absorb unpaid damage"),ERWeaponBallistics::AbsorbArmor(100,1,Armor),98.f);
    TestEqual(TEXT("Armor depleted"),Armor,0.f);
    TestEqual(TEXT("Unarmored target gets full damage"),ERWeaponBallistics::AbsorbArmor(26,1.2f,Armor),26.f);
    Armor=100;
    TestEqual(TEXT("Full penetration preserves armor"),ERWeaponBallistics::AbsorbArmor(36,2,Armor),36.f);
    TestEqual(TEXT("Full penetration armor is untouched"),Armor,100.f);
    return true;
}
#endif
