#include "ERWeaponAccuracy.h"
#include "ERWeapon.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERWorksheetAccuracyTest,"EclipseRecoil.Weapons.WorksheetAccuracy",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERWorksheetAccuracyTest::RunTest(const FString&)
{
    FString Text;TSharedPtr<FJsonObject> Root;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("docs/references/cs2-weapon-ballistics-2026-03-18.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root))return false;
    const auto& Profiles=Root->GetObjectField(TEXT("weapons"))->Values;
    for(const auto& Entry:Profiles)
    {
        const FString Key=*Entry.Key;
        const auto Spec=Entry.Value->AsObject()->GetObjectField(TEXT("accuracy"));
        const auto Checks=Spec->GetObjectField(TEXT("simple_checks"));
        auto* Data=LoadObject<UERWeaponData>(nullptr,*FString::Printf(TEXT("/Game/EclipseRecoil/Weapons/Sketchfab/%s/DA_%s.DA_%s"),*Key,*Key,*Key));
        if(!TestNotNull(*Key,Data))continue;
        TestTrue(*(Key+TEXT(" worksheet mode enabled")),Data->bUseWorksheetAccuracy);
        for(bool Scoped:{false,true})
        {
            const auto Expected=Spec->GetObjectField(Scoped?TEXT("scoped"):TEXT("hip"));
            const auto& P=Scoped?Data->ScopedAccuracy:Data->HipAccuracy;
            auto Same=[&](float Actual,const TCHAR* Field){TestTrue(*(Key+FString::Printf(TEXT(" %s %s"),Scoped?TEXT("scope"):TEXT("hip"),Field)),FMath::IsNearlyEqual(Actual,float(Expected->GetNumberField(Field)),.0001f));};
            Same(P.Spread,TEXT("spread"));Same(P.Stand,TEXT("stand"));Same(P.Crouch,TEXT("crouch"));Same(P.Fire,TEXT("fire"));
            Same(P.Move,TEXT("move"));Same(P.Ladder,TEXT("ladder"));Same(P.Jump,TEXT("jump"));Same(P.JumpInitial,TEXT("jump_initial"));
            Same(P.JumpApex,TEXT("jump_apex"));Same(P.Land,TEXT("land"));Same(P.MaxSpeed,TEXT("max_speed"));
            Same(P.RecoveryStand,TEXT("recovery_stand"));Same(P.RecoveryStandFinal,TEXT("recovery_stand_final"));
            Same(P.RecoveryCrouch,TEXT("recovery_crouch"));Same(P.RecoveryCrouchFinal,TEXT("recovery_crouch_final"));
            TestTrue(TEXT("scope retains nonzero minimum spread"),P.Spread+P.Crouch>0);
        }
        const auto& P=Data->HipAccuracy;
        auto Check=[&](const TCHAR* Name,float Actual){TestTrue(*(Key+TEXT(" cached Simple ")+Name),FMath::IsNearlyEqual(Actual,float(Checks->GetNumberField(Name)),.0001f));};
        Check(TEXT("standing"),P.Spread+ERWeaponAccuracy::Base(P,0,0,false,false,0,460));
        Check(TEXT("crouching"),P.Spread+ERWeaponAccuracy::Base(P,1,0,false,false,0,460));
        Check(TEXT("running"),P.Spread+ERWeaponAccuracy::Base(P,0,P.MaxSpeed*2.54f,false,false,0,460));
        Check(TEXT("ladder"),P.Spread+ERWeaponAccuracy::Base(P,0,0,true,false,144,460));
        Check(TEXT("jump_apex"),P.Spread+ERWeaponAccuracy::Base(P,0,0,false,true,0,460));
        Check(TEXT("landing_333ms"),P.Spread+P.Stand+ERWeaponAccuracy::Recover(P.Jump+P.Land*302,.333,P.RecoveryStand));
        const auto& AngleProfile=Spec->GetStringField(TEXT("standing_full_angle_mode"))==TEXT("scoped")?Data->ScopedAccuracy:P;
        TestTrue(TEXT("correct half-angle, not workbook full angle or raw values as degrees"),FMath::IsNearlyEqual(
            2*ERWeaponAccuracy::Degrees(AngleProfile.Spread+AngleProfile.Stand),float(Spec->GetNumberField(TEXT("standing_full_angle_degrees"))),.00001f));
        const float Cycle=Spec->GetNumberField(TEXT("cycle_time"));
        TestTrue(TEXT("firing cycle matches worksheet"),FMath::IsNearlyEqual(Data->ShotInterval,Cycle,.00001f));
        for(bool Crouched:{false,true})
        {
            const auto& Golden=Spec->GetObjectField(TEXT("burst_checks"))->GetArrayField(Crouched?TEXT("crouching"):TEXT("standing"));
            float Remaining=0;
            for(int32 I=0;I<Golden.Num();++I)
            {
                TestTrue(*(Key+FString::Printf(TEXT(" cached burst low=%d bullet=%d"),Crouched,I+1)),FMath::IsNearlyEqual(Remaining,float(Golden[I]->AsNumber()),.0001f));
                Remaining=ERWeaponAccuracy::Recover(Remaining+P.Fire,Cycle,ERWeaponAccuracy::Recovery(P,Crouched?1:0,I+1));
            }
        }
        float Tapping=0;const auto& Taps=Spec->GetArrayField(TEXT("tapping_checks"));
        for(int32 I=0;I<Taps.Num();++I)
        {
            TestTrue(*(Key+FString::Printf(TEXT(" cached tap bullet=%d"),I+1)),FMath::IsNearlyEqual(Tapping,float(Taps[I]->AsNumber()),.0001f));
            Tapping=ERWeaponAccuracy::Recover(Tapping+P.Fire,Spec->GetNumberField(TEXT("tap_interval")),P.RecoveryStand);
        }
    }
    FRandomStream Stream(1701),Repeat(1701),Different(1702);
    bool Same=true,Varies=false,Bounded=true;float Sum=0;
    const float Bound=ERWeaponAccuracy::Degrees(45);
    for(int32 I=0;I<4096;++I)
    {
        const FVector Ray=ERWeaponAccuracy::Sample(FVector::ForwardVector,38,7,Stream);
        Same&=Ray.Equals(ERWeaponAccuracy::Sample(FVector::ForwardVector,38,7,Repeat));
        Varies|=!Ray.Equals(ERWeaponAccuracy::Sample(FVector::ForwardVector,38,7,Different));
        const float Angle=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Ray.X,-1.,1.)));
        Bounded&=Angle<=Bound+.001f;Sum+=Angle;
    }
    TestTrue(TEXT("independent spread/inaccuracy sampling deterministic, varying and within HUD envelope"),Same&&Varies&&Bounded&&Sum>100);
    return true;
}
#endif
