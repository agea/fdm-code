#include "ERWeapon.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERWorksheetRecoilTest,"EclipseRecoil.Weapons.WorksheetRecoil",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERWorksheetRecoilTest::RunTest(const FString&)
{
    FString Text;TSharedPtr<FJsonObject> Root;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("docs/references/cs2-weapon-ballistics-2026-03-18.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root))return false;
    for(const auto& Entry:Root->GetObjectField(TEXT("weapons"))->Values)
    {
        const FString Key=*Entry.Key;
        const auto Spec=Entry.Value->AsObject()->GetObjectField(TEXT("recoil"));
        auto* Data=LoadObject<UERWeaponData>(nullptr,*FString::Printf(TEXT("/Game/EclipseRecoil/Weapons/Sketchfab/%s/DA_%s.DA_%s"),*Key,*Key,*Key));
        if(!TestNotNull(*Key,Data))continue;
        TestTrue(*(Key+TEXT(" worksheet recoil enabled")),Data->bUseWorksheetRecoil);
        TestEqual(*(Key+TEXT(" pattern class")),Data->bRandomRecoilPattern,Spec->GetStringField(TEXT("pattern"))==TEXT("Random"));
        TestEqual(*(Key+TEXT(" stable project seed")),Data->RecoilPatternSeed,int32(Spec->GetNumberField(TEXT("project_pattern_seed"))));
        for(bool Scoped:{false,true})
        {
            const auto Expected=Spec->GetObjectField(Scoped?TEXT("scoped"):TEXT("hip"));
            const auto& P=Scoped?Data->ScopedRecoil:Data->HipRecoil;
            auto Same=[&](float Actual,const TCHAR* Field){TestTrue(*(Key+FString::Printf(TEXT(" %s %s"),Scoped?TEXT("scope"):TEXT("hip"),Field)),FMath::IsNearlyEqual(Actual,float(Expected->GetNumberField(Field)),.0001f));};
            Same(P.AngleVariance,TEXT("angle_variance"));Same(P.Magnitude,TEXT("magnitude"));Same(P.MagnitudeVariance,TEXT("magnitude_variance"));
            const auto Vertical=ERWeaponRecoil::Impulse(P,.5f,.5f);
            TestTrue(TEXT("central impulse is vertical at exact sheet magnitude"),Vertical.Equals(FVector2D(P.Magnitude,0),.0001));
            FRandomStream R(123),Other(456);bool Bounded=true,Repeated=true,Differs=false;
            for(int32 I=0;I<512;++I)
            {
                const auto A=ERWeaponRecoil::Sample(P,Data->bRandomRecoilPattern,Data->RecoilPatternSeed,I,R);
                const auto B=ERWeaponRecoil::Sample(P,Data->bRandomRecoilPattern,Data->RecoilPatternSeed,I,Other);
                Repeated&=A.Equals(B,.0001);Differs|=!A.Equals(B,.0001);
                const double Angle=FMath::Abs(FMath::RadiansToDegrees(FMath::Atan2(A.Y,A.X)));
                Bounded&=A.X>=0 && Angle<=P.AngleVariance+.001 &&
                    A.Size()>=P.Magnitude-P.MagnitudeVariance-.001 && A.Size()<=P.Magnitude+P.MagnitudeVariance+.001;
            }
            TestTrue(TEXT("all recoil samples respect both worksheet variances"),Bounded);
            TestTrue(TEXT("fixed patterns repeat; Random patterns vary independently"),Data->bRandomRecoilPattern?Differs:Repeated);
        }
    }
    FERWeaponRecoilState Initial;Initial.ServerTime=0;Initial.Velocity=FVector2D(30,10);
    const auto Peak=ERWeaponRecoil::At(Initial,.1);
    TestTrue(TEXT("nonzero real angular deflection follows impulse"),Peak.Angle.X>1 && Peak.Angle.Y>0);
    for(int32 Hz:{30,60,144})
    {
        auto Stepped=Initial;
        for(int32 I=1;I<=Hz;++I)Stepped=ERWeaponRecoil::At(Stepped,double(I)/Hz);
        const auto Exact=ERWeaponRecoil::At(Initial,1);
        TestTrue(TEXT("response independent of frame rate"),Stepped.Angle.Equals(Exact.Angle,1.e-7)&&Stepped.Velocity.Equals(Exact.Velocity,1.e-7));
    }
    const auto Rest=ERWeaponRecoil::At(Initial,4);
    TestTrue(TEXT("recovers without drift or overshoot"),Rest.Angle.Size()<.00001&&Rest.Velocity.Size()<.00001);
    return true;
}
#endif
