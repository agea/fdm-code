#include "Misc/AutomationTest.h"
#include "ERAudio.h"
#include "ERFootsteps.h"
#include "ERSmokeCloud.h"
#include "ERWeapon.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeModulator.h"
#include "Sound/SoundNodeSwitch.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundAttenuation.h"
#include "Engine/Blueprint.h"
#include "ERCharacter.h"
#include "ERBlockRoom.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/World.h"
#include "Components/AudioComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERAudioShuffleTest,"EclipseRecoil.Audio.NoRepeat",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERAudioShuffleTest::RunTest(const FString&)
{
    for (int32 Count:{1,2,3,4,6,8})
    {
        FERAudioShuffle Bag; int32 Last=INDEX_NONE; TSet<int32> Seen;
        for (int32 I=0;I<Count*100;++I)
        {
            int32 Pick=Bag.Choose(Count);
            TestTrue(TEXT("Valid sample"),Pick>=0 && Pick<Count);
            if (Count>1) TestNotEqual(TEXT("No adjacent repeat including bag boundary"),Pick,Last);
            if (I%Count==0) Seen.Reset();
            TestFalse(TEXT("Every recording is used once per bag"),Seen.Contains(Pick));Seen.Add(Pick);Last=Pick;
        }
    }
    FERAudioShuffle Empty;TestEqual(TEXT("Empty palette is silent"),Empty.Choose(0),INDEX_NONE);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERAudioCadenceTest,"EclipseRecoil.Audio.Cadence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERAudioCadenceTest::RunTest(const FString&)
{
    for (const float Hz:{30.f,60.f,144.f}) for (const float Speed:{138.8889f,277.7778f,500.f})
    {
        FERFootstepCadence C;int32 Count=0;FVector P=FVector::ZeroVector;
        for(int32 I=0;I<10*Hz;++I){P.X+=Speed/Hz;Count+=C.Advance(P,true,Speed,false,false,I/Hz);}
        const float Rate=Count/10.f;
        TestTrue(TEXT("Real distance cadence across frame rates"),Rate>=(Speed<200?1.3f:Speed<400?2.f:3.f) && Rate<=(Speed<200?1.6f:Speed<400?2.3f:3.6f));
        for(int32 I=0;I<200;++I) TestFalse(TEXT("WASD against a wall cannot create footsteps"),C.Advance(P,true,Speed,false,false,11+I/Hz));
        P.X+=5000;TestFalse(TEXT("Teleport has no step burst"),C.Advance(P,true,Speed,false,false,20));
        for(int32 I=0;I<200;++I){P.X+=Speed/Hz;TestFalse(TEXT("Air, death, traversal and menu reset phase"),C.Advance(P,false,Speed,false,false,21+I/Hz));}
        TestFalse(TEXT("Landing frame never doubles as an ordinary step"),C.Advance(P,true,Speed,false,false,30));
    }
    TestTrue(TEXT("Crouch is quiet"),FERFootstepCadence::Gain(100,true,false)<.45f);
    TestTrue(TEXT("Sprint louder than run"),FERFootstepCadence::Gain(500,false,false)>FERFootstepCadence::Gain(277,false,false));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERAudioAssetsTest,"EclipseRecoil.Audio.SavedAssets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERAudioAssetsTest::RunTest(const FString&)
{
    TSet<UEROneShotAudio*> Distinct;
    for(const TCHAR* Key:{TEXT("Pistol"),TEXT("SMG"),TEXT("Shotgun"),TEXT("Assault"),TEXT("Carbine"),TEXT("Sniper"),TEXT("Launcher")})
    {
        auto* Data=LoadObject<UERWeaponData>(nullptr,*FString::Printf(TEXT("/Game/EclipseRecoil/Weapons/Sketchfab/%s/DA_%s"),Key,Key));
        if(!TestNotNull(TEXT("Weapon saved"),Data))continue;
        auto* Audio=Data->ShotAudio.Get();if(!TestNotNull(Key,Audio))continue;
        TestFalse(TEXT("Separate palette per weapon"),Distinct.Contains(Audio));Distinct.Add(Audio);
        TestTrue(TEXT("Actual variants"),Audio->Variants.Num()>0);
        auto* Cue=Audio->Cue.Get();if(!TestNotNull(TEXT("Saved SoundCue"),Cue))continue;
        auto* Mod=Cast<USoundNodeModulator>(Cue->FirstNode);
        if(!TestNotNull(TEXT("Small modulator"),Mod))continue;
        TestTrue(TEXT("Weapon pitch range"),FMath::IsNearlyEqual(Mod->PitchMin,.98f) && FMath::IsNearlyEqual(Mod->PitchMax,1.02f));
        auto* Param=Cast<USoundNodeSwitch>(Mod->ChildNodes[0]);
        if(!TestNotNull(TEXT("Per-emitter finite switch"),Param))continue;
        auto* Random=Cast<USoundNodeRandom>(Param->ChildNodes[0]);
        if(TestNotNull(TEXT("Preview random fallback"),Random))TestTrue(TEXT("Without replacement"),Random->bRandomizeWithoutReplacement && Random->ChildNodes.Num()==Audio->Variants.Num());
        TestTrue(TEXT("Bounded concurrency"),Cue->ConcurrencySet.Num()==2);
        TestFalse(TEXT("One-shot cue has finite duration, never a virtual loop"),Cue->IsLooping());
    }
    auto* Steps=LoadObject<UERFootstepAudio>(nullptr,TEXT("/Game/EclipseRecoil/Audio/Footsteps/DA_ER_Footsteps"));
    if(TestNotNull(TEXT("Surface data asset"),Steps))
    {
        TestEqual(TEXT("Wood cells"),Steps->CellSurfaces[EERBlockMaterial::Wood],EERFootstepSurface::Wood);
        TestEqual(TEXT("Metal cells"),Steps->CellSurfaces[EERBlockMaterial::Metal],EERFootstepSurface::Metal);
        TestEqual(TEXT("Concrete cells"),Steps->CellSurfaces[EERBlockMaterial::Concrete],EERFootstepSurface::Hard);
        TestNotNull(TEXT("Safe hard fallback"),Steps->Find(EERFootstepSurface::Hard));
        for(const auto& Entry:Steps->Sounds) TestTrue(TEXT("Surface has real samples"),Entry.Value && Entry.Value->Variants.Num()>0);
        TestNotNull(TEXT("Step attenuation"),Steps->Attenuation.Get());
    }
    auto* CharacterBP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/EclipseRecoil/Core/BP_ERCharacter"));
    if(TestNotNull(TEXT("Character Blueprint"),CharacterBP))
    {
        const auto* Character=Cast<AERCharacter>(CharacterBP->GeneratedClass->GetDefaultObject());
        TestTrue(TEXT("Saved inherited footstep component binding"),Character->Footsteps && Character->Footsteps->Audio==Steps);
    }
    auto* SmokeBP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/EclipseRecoil/Effects/Smoke/BP_ERSmokeCloud"));
    if(TestNotNull(TEXT("Smoke Blueprint"),SmokeBP))
    {
        const auto* Smoke=Cast<AERSmokeCloud>(SmokeBP->GeneratedClass->GetDefaultObject());
        TestTrue(TEXT("Smoke only minus ten dB from actual .65 gain"),FMath::IsNearlyEqual(Smoke->ReleaseVolume,.65f*.31622777f,.000001f));
        TestEqual(TEXT("Cloud duration unchanged"),Smoke->Duration,12.f);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERAudioSurfaceTest,"EclipseRecoil.Audio.SurfaceQueries",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERAudioSurfaceTest::RunTest(const FString&)
{
    auto* Audio=LoadObject<UERFootstepAudio>(nullptr,TEXT("/Game/EclipseRecoil/Audio/Footsteps/DA_ER_Footsteps"));
    if(!TestNotNull(TEXT("Footstep data"),Audio))return false;
    FHitResult Empty;TestEqual(TEXT("Unknown floor falls back to hard"),Audio->Resolve(Empty),EERFootstepSurface::Hard);
    auto* Physical=NewObject<UPhysicalMaterial>();Physical->SurfaceType=SurfaceType2;
    FHitResult PhysicalHit;PhysicalHit.PhysMaterial=Physical;
    TestEqual(TEXT("Ordinary mesh physical material"),Audio->Resolve(PhysicalHit),EERFootstepSurface::Wood);
    UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);
    if(!World)return false;
    auto* Room=World->SpawnActor<AERBlockRoom>();
    if(!Room->Regenerate()){World->DestroyWorld(false);return false;}
    TSet<EERBlockMaterial> Queried;
    const auto& Panels=Room->GetCellWorld().GetPanels();
    for(const auto& Panel:Panels)
    {
        if(Queried.Contains(Panel.Material))continue;
        const FVector Local=Panel.Size*.5f;
        const FVector Center=Room->GetActorTransform().TransformPosition(Panel.Transform.TransformPosition(Local));
        const FVector Normal=Room->GetActorTransform().TransformVectorNoScale(Panel.Transform.TransformVectorNoScale(FVector(0,1,0)));
        FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(ERAudioSurfaceTest),false);
        if(World->LineTraceSingleByObjectType(Hit,Center+Normal*40,Center-Normal*40,FCollisionObjectQueryParams(ECC_WorldStatic),Q))
        {
            FERCellId Id;
            if(Room->ResolveCell(Hit,Id))
            {
                TestEqual(TEXT("Cell ID resolves without physical material or physics edits"),Audio->Resolve(Hit),Audio->CellSurfaces[Panel.Material]);
                Queried.Add(Panel.Material);
            }
        }
    }
    TestTrue(TEXT("Real procedural wood, steel and concrete collision queried"),Queried.Contains(EERBlockMaterial::Wood) && Queried.Contains(EERBlockMaterial::Metal) && Queried.Contains(EERBlockMaterial::Concrete));
    World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERAudioNoDeviceTest,"EclipseRecoil.Audio.NoDeviceCleanup",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERAudioNoDeviceTest::RunTest(const FString&)
{
    auto* Palette=LoadObject<UEROneShotAudio>(nullptr,TEXT("/Game/EclipseRecoil/Audio/Weapons/DA_ER_Shot_Pistol"));
    if(!TestNotNull(TEXT("Saved palette"),Palette))return false;
    UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);
    if(!World)return false;
    auto* Owner=World->SpawnActor<AActor>();FERAudioShuffle Bag;
    for(int32 I=0;I<100;++I)TestTrue(TEXT("Headless audit still selects a recording"),ERAudio::Play(Owner,Palette,Bag,FVector::ZeroVector,true,1,nullptr)>=0);
    TArray<UAudioComponent*> Components;Owner->GetComponents(Components);
    TestEqual(TEXT("No device cannot leak registered auto-destroy components"),Components.Num(),0);
    World->DestroyWorld(false);return true;
}

#endif
