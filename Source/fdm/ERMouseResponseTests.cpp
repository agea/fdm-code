#include "ERMouseResponse.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERMouseResponseTest, "FDM.Input.MouseResponse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FERMouseResponseTest::RunTest(const FString& Parameters)
{
    const auto AtSpeed = [](double Speed, double FPS, float Strength = .5f)
    { return ERMouseResponse::Gain(FVector2D(Speed * .18 / FPS, 0), 1.0 / FPS, Strength); };
    TestTrue(TEXT("Slow corrections are precise"), FMath::IsNearlyEqual(AtSpeed(100, 60), .75f));
    TestTrue(TEXT("Fast turns have bounded gain"), FMath::IsNearlyEqual(AtSpeed(3000, 60), 1.8f));
    float Previous = 0;
    for (int32 Speed = 0; Speed <= 4000; Speed += 10)
    {
        const float Gain = AtSpeed(Speed, 60);
        if (Gain < Previous || Gain < .75f || Gain > 1.801f) AddError(TEXT("Curve is not monotone/bounded"));
        Previous = Gain;
        for (double FPS : {30.0, 45.0, 90.0, 144.0})
            if (!FMath::IsNearlyEqual(Gain, AtSpeed(Speed, FPS))) AddError(TEXT("Gain depends on frame rate"));
        if (!FMath::IsNearlyEqual(AtSpeed(Speed, 60, 0), 1.f)) AddError(TEXT("Zero strength is not linear"));
    }
    const float Horizontal = ERMouseResponse::Gain(FVector2D(3, 0), .01, .5f);
    TestTrue(TEXT("Direction does not change gain"), FMath::IsNearlyEqual(Horizontal, ERMouseResponse::Gain(FVector2D(-1.8, 2.4), .01, .5f)));
    TestTrue(TEXT("Zero delta stays zero"), (FVector2D::ZeroVector * ERMouseResponse::Gain(FVector2D::ZeroVector, .01, .5f)).IsZero());
    TestEqual(TEXT("Invalid time is safe"), ERMouseResponse::Gain(FVector2D(1, 0), 0, .5f), 1.f);
    return true;
}
#endif
