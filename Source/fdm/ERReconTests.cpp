#include "ERRecon.h"
#include "Misc/AutomationTest.h"
#include "ERSmokeCloud.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERReconFogTest,"EclipseRecoil.Recon.FogAndSight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERReconFogTest::RunTest(const FString& Parameters)
{
    TArray<uint8> Fog; Fog.Init(0,ERRecon::FogBytes);
    for (int32 I : {0,7,8,63,64,4095})
    {
        TestFalse(TEXT("Unseen initially"),ERRecon::IsExplored(Fog,I));
        ERRecon::Explore(Fog,I); TestTrue(TEXT("Seen persists"),ERRecon::IsExplored(Fog,I));
    }
    ERRecon::Explore(Fog,-1); ERRecon::Explore(Fog,4096);
    TestFalse(TEXT("No adjacent reveal"),ERRecon::IsExplored(Fog,9));
    TestFalse(TEXT("Invalid cell rejected"),ERRecon::IsExplored(Fog,4096));
    TArray<uint8> Other; Other.Init(0,ERRecon::FogBytes);
    TestFalse(TEXT("Other team stays hidden"),ERRecon::IsExplored(Other,0));
    TestTrue(TEXT("Forward sight"),ERRecon::InSightCone(FVector::ZeroVector,FVector::ForwardVector,FVector(1000,0,0),3000,false));
    TestFalse(TEXT("Behind hidden"),ERRecon::InSightCone(FVector::ZeroVector,FVector::ForwardVector,FVector(-1000,0,0),3000,false));
    TestTrue(TEXT("Drone sees behind"),ERRecon::InSightCone(FVector::ZeroVector,FVector::ForwardVector,FVector(-1000,0,0),4200,true));
    TestFalse(TEXT("Drone range bounded"),ERRecon::InSightCone(FVector::ZeroVector,FVector::ForwardVector,FVector(4201,0,0),4200,true));
    TestTrue(TEXT("Drone ground footprint ignores altitude"),ERRecon::InSightCone(FVector(0,0,2500),FVector::ForwardVector,FVector(3600,0,170),4200,true));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERReconFlightTest,"EclipseRecoil.Recon.Flight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERReconFlightTest::RunTest(const FString& Parameters)
{
    FERDroneFlight F; F.Launch=FVector(-2000,0,70); F.Center=FVector(0,0,2200); F.Radius=FVector2D(1800); F.StartedAt=10; F.Team=EERTeam::Alpha;
    TestTrue(TEXT("Launch at base"),AERReconDrone::FlightPosition(F,10).Equals(F.Launch,.01));
    TestTrue(TEXT("Vertical ascent"),AERReconDrone::FlightPosition(F,12).Equals(FVector(-2000,0,1135),.01));
    TestTrue(TEXT("Clear roofs before orbit"),FMath::IsNearlyEqual(AERReconDrone::FlightPosition(F,14).Z,2200.,.01));
    const FVector Entry=AERReconDrone::FlightPosition(F,18);
    TestTrue(TEXT("Full lap returns"),Entry.Equals(AERReconDrone::FlightPosition(F,18+2*PI/.16),.01));
    TestTrue(TEXT("No teleport at orbit join"),Entry.Equals(AERReconDrone::FlightPosition(F,18-.0001),1));
    F.Team=EERTeam::Omega;
    TestTrue(TEXT("Opposite teams opposite entry"),AERReconDrone::FlightPosition(F,18).Equals(FVector(-1800,0,2200),.01));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERReconSmokeTest,"EclipseRecoil.Recon.SmokeOcclusion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERReconSmokeTest::RunTest(const FString& Parameters)
{
    UWorld::InitializationValues Values; Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    if (!TestNotNull(TEXT("Smoke test world"),World)) return false;
    auto* Smoke=World->SpawnActor<AERSmokeCloud>(); Smoke->RebuildContainment();
    Smoke->ShapePhase=FVector::ZeroVector; Smoke->StartedAtServerTime=-3;
    const FVector Left(-1000,0,170), Right(1000,0,170);
    TestTrue(TEXT("Dense occupied smoke blocks player ray"),Smoke->ReconOpticalDepth(Left,Right)>=1);
    TestTrue(TEXT("Dense smoke blocks overhead drone ray"),Smoke->ReconOpticalDepth(FVector(0,0,2500),FVector(0,0,170))>=1);
    TestEqual(TEXT("Ray misses cloud"),Smoke->ReconOpticalDepth(FVector(-1000,600,170),FVector(1000,600,170)),0.f);
    Smoke->StartedAtServerTime=0;
    TestEqual(TEXT("No instant opaque box before growth"),Smoke->ReconOpticalDepth(Left,Right),0.f);
    Smoke->StartedAtServerTime=-11.99;
    TestTrue(TEXT("Faded cloud no longer blocks"),Smoke->ReconOpticalDepth(Left,Right)<1);
    Smoke->StartedAtServerTime=-12.1;
    TestEqual(TEXT("Expired cloud is transparent"),Smoke->ReconOpticalDepth(Left,Right),0.f);
    Smoke->StartedAtServerTime=-3;
    Smoke->AddShot(FVector(0,0,40),FVector(0,0,160),400,12);
    TestTrue(TEXT("Blast pocket restores player sight"),Smoke->ReconOpticalDepth(Left,Right)<1);
    World->DestroyWorld(false);
    return true;
}
#endif
