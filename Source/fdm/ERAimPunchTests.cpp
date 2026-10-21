#include "ERAimPunch.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERAimPunchTest,"EclipseRecoil.Weapons.AimPunch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERAimPunchTest::RunTest(const FString&)
{
    FERAimPunchTuning T;
    const auto Hit=ERAimPunch::Impulse(T,20,false,false,1,1,true);
    TestTrue(TEXT("20HP gives .64 pitch and .224 yaw"),Hit.Equals(FVector2D(.64,.224),1.e-6));
    TestTrue(TEXT("armor uses real applied-damage multiplier"),ERAimPunch::Impulse(T,20,false,true,1,1,true).Equals(Hit*.6,1.e-6));
    TestTrue(TEXT("head uses authoritative bone multiplier"),ERAimPunch::Impulse(T,20,true,false,1,1,true).Equals(Hit*1.5,1.e-6));
    TestTrue(TEXT("left source gives negative yaw"),ERAimPunch::Impulse(T,20,false,false,-1,1,true).Y<0);
    TestTrue(TEXT("zero damage cannot disturb aim"),ERAimPunch::Impulse(T,0,false,false,1,1,true).IsZero());
    TestTrue(TEXT("pellet base is counted once"),ERAimPunch::Impulse(T,20,false,false,1,1,false).Equals(FVector2D(.24,.084),1.e-6));
    TestTrue(TEXT("duplicates and reordered snapshots rejected"),!ERAimPunch::Newer(3,3)&&!ERAimPunch::Newer(2,3)&&ERAimPunch::Newer(4,3));
    TestTrue(TEXT("event sequence wraps safely"),ERAimPunch::Newer(1,MAX_uint32));
    for(int32 Hz:{30,60,120})
    {
        FERAimPunchState State;ERAimPunch::Add(State,T,Hit,0);
        for(int32 I=1;I<=Hz;++I){State.Angle=ERAimPunch::At(State,double(I)/Hz);State.ServerTime=double(I)/Hz;}
        TestTrue(TEXT("exact frame-independent half-life at 30/60/120Hz"),State.Angle.Equals(Hit/32.,1.e-7));
    }
    FERAimPunchState State;ERAimPunch::Add(State,T,Hit,0);
    TestTrue(TEXT("half at .2s"),ERAimPunch::At(State,.2).Equals(Hit*.5,1.e-7));
    TestTrue(TEXT("94 percent recovered at .8s"),ERAimPunch::At(State,.8).Equals(Hit/16.,1.e-7));
    for(int32 I=0;I<100;++I)ERAimPunch::Add(State,T,Hit,0);
    TestTrue(TEXT("stacking bounded on both axes"),State.Angle.Equals(FVector2D(2.5,1.5),1.e-6));
    TestTrue(TEXT("no residual drift"),ERAimPunch::At(State,20).IsZero());
    return true;
}
#endif
