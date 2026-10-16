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
    const float Inner=ERWeaponBallistics::GrenadeDamage(140,200,200,600);
    Armor=100;
    TestTrue(TEXT("Full armor cannot survive an exposed inner blast"),ERWeaponBallistics::AbsorbArmor(Inner,ERWeaponBallistics::GrenadeArmorRatio(1),Armor)>=100);
    TestTrue(TEXT("Blast armor is consumed without the bullet 50-percent reduction"),FMath::IsNearlyEqual(Armor,86.f));
    TestEqual(TEXT("Four metre fragmentation tail"),ERWeaponBallistics::GrenadeDamage(140,400,200,600),35.f);
    TestEqual(TEXT("Exactly six metres is zero damage"),ERWeaponBallistics::GrenadeDamage(140,600,200,600),0.f);
    TestEqual(TEXT("Beyond six metres is zero damage"),ERWeaponBallistics::GrenadeDamage(140,800,200,600),0.f);
    TestTrue(TEXT("Outer injury tapers without the previous minimum-damage step"),ERWeaponBallistics::GrenadeDamage(140,599,200,600)<.01f);
    float Total=0;Armor=100;
    for(int32 I=0;I<6;++I)Total+=ERWeaponBallistics::AbsorbArmor(20,1.6f,Armor);
    TestTrue(TEXT("Six armored chest pellets total 96 and spend 12 AP"),FMath::IsNearlyEqual(Total,96.f) && FMath::IsNearlyEqual(Armor,88.f));
    TestEqual(TEXT("Head pellet base"),20*ERWeaponBallistics::ZoneMultiplier(ERWeaponBallistics::EHitZone::Head),80.f);
    TestEqual(TEXT("Stomach pellet base"),20*ERWeaponBallistics::ZoneMultiplier(ERWeaponBallistics::EHitZone::Stomach),25.f);
    TestEqual(TEXT("Leg pellet base"),20*ERWeaponBallistics::ZoneMultiplier(ERWeaponBallistics::EHitZone::Legs),15.f);
    TestTrue(TEXT("XM 12.7m retains 70 percent"),FMath::IsNearlyEqual(ERWeaponBallistics::AtRange(20,.7f,1270),14.f));
    FRandomStream Pattern(1701),Repeat(1701),Other(1702);
    bool Identical=true,Changed=false,Bounded=true;
    for(int32 I=0;I<6;++I)
    {
        const FVector Ray=ERWeaponBallistics::PelletDirection(FVector::ForwardVector,I,6,3.2f,Pattern);
        Identical &= Ray.Equals(ERWeaponBallistics::PelletDirection(FVector::ForwardVector,I,6,3.2f,Repeat));
        Changed |= !Ray.Equals(ERWeaponBallistics::PelletDirection(FVector::ForwardVector,I,6,3.2f,Other));
        const float Angle=FMath::RadiansToDegrees(FMath::Acos(Ray.X));
        Bounded &= Angle>1.5f && Angle<3.2f;
    }
    TestTrue(TEXT("Server pattern is reproducible, jittered and bounded with no centre pellet"),Identical && Changed && Bounded);
    return true;
}
#endif
