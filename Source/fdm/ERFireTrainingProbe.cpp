#include "ERFireTrainingProbe.h"
#include "ERProceduralMap.h"
#include "ERRecon.h"
#include "ERPlayerController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

AERFireTrainingProbe::AERFireTrainingProbe(){PrimaryActorTick.bCanEverTick=true;}
void AERFireTrainingProbe::Check(bool Good,const TCHAR* Label)
{
    UE_LOG(LogTemp,Display,TEXT("ER_FIRE_PROBE_CHECK %s=%s"),Label,Good ? TEXT("PASS"):TEXT("FAIL"));Failed|=!Good;
}
void AERFireTrainingProbe::Tick(float Delta)
{
    Super::Tick(Delta);Elapsed+=Delta;
    TActorIterator<AERProceduralMap> MapIt(GetWorld());AERProceduralMap* Map=MapIt ? *MapIt:nullptr;
    auto* PC=Cast<AERPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
    if(!Map || !PC || !PC->GetPawn()){if(Elapsed>60){Check(false,TEXT("startup"));FPlatformMisc::RequestExitWithStatus(false,1);Phase=99;}return;}
    const auto& L=Map->GetLayout();
    if(Phase==0)
    {
        if(Elapsed<12)return;
#if WITH_EDITOR
        FAssetCompilingManager::Get().FinishAllCompilation();
#endif
        // Indoor recon: the frame stays under the eave and both cases found ground.
        TActorIterator<AERReconDirector> ReconIt(GetWorld());AERReconDirector* Recon=ReconIt ? *ReconIt:nullptr;
        Check(Recon && FMath::IsNearlyEqual(Recon->Frame.Indoor,L.IndoorCeiling,1.f),TEXT("recon_indoor_frame"));
        int32 Bases=0;for(TActorIterator<AERReconBase> It(GetWorld());It;++It)++Bases;
        Check(Bases==2,TEXT("recon_cases_placed_indoors"));
        int32 Below=0,Drones=0;
        for(TActorIterator<AERReconDrone> It(GetWorld());It;++It){++Drones;Below+=It->Flight.Center.Z<L.IndoorCeiling+200;}
        Check(Drones>0 && Below==Drones,TEXT("drones_orbit_under_the_roof"));
        PC->GetPawn()->SetActorHiddenInGame(true);PC->ClientSetHUD(AHUD::StaticClass());
        Phase=1;StageTime=Elapsed;return;
    }
    // -ERFireTrainingSeeds=a,b,c picks the gallery seeds (default 42,73,7919).
    static TArray<int32> Seeds=[]{TArray<int32> Out;FString List;
        if(FParse::Value(FCommandLine::Get(),TEXT("ERFireTrainingSeeds="),List,false))
        {TArray<FString> Parts;List.ParseIntoArray(Parts,TEXT(","));for(const auto& P:Parts)Out.Add(FCString::Atoi(*P));}
        if(Out.IsEmpty())Out={42,73,7919};return Out;}();
    static const TCHAR* Names[]={TEXT("Plan"),TEXT("Spawn"),TEXT("Overview"),TEXT("Catwalk")};
    constexpr int32 PerSeed=UE_ARRAY_COUNT(Names);
    if(Phase==1)
    {
        if(Shot>=PerSeed*Seeds.Num())
        {
            UE_LOG(LogTemp,Display,TEXT("ER_FIRE_PROBE_COMPLETE result=%s"),Failed ? TEXT("FAIL"):TEXT("PASS"));
            FPlatformMisc::RequestExitWithStatus(false,Failed ? 1:0);Phase=99;return;
        }
        const int32 Wanted=Seeds[Shot/PerSeed];
        if(Seed!=Wanted)
        {
            Map->LayoutConfig.Seed=Wanted;Check(Map->Regenerate(),TEXT("gallery_seed"));Seed=Wanted;
            FString Features;for(const FName& F:L.FeatureNames)Features+=F.ToString()+TEXT(",");
            UE_LOG(LogTemp,Display,TEXT("ER_FIRE_GALLERY seed=%d width=%.0f bays=%d bands=%d attempt=%d elevated=%.3f overhead=%.3f open=%.3f lights=%s features=%s digest=%u"),
                Seed,L.WarehouseWidth,L.WarehouseBays,L.WarehouseBands,L.LayoutAttempt,L.ElevatedRatio,L.OverheadRatio,L.OpenRegionRatio,TEXT("procedural"),*Features,Map->GetStateDigest());
            StageTime=Elapsed;
        }
        if(Elapsed-StageTime<2)return;
        const int32 Kind=Shot%PerSeed;const float HL=210.f*L.WarehouseBays,W=L.WarehouseWidth;
        const bool Plan=Kind==0;
        TInlineComponentArray<UPrimitiveComponent*> Parts(Map);
        for(auto* Part:Parts)if(Part->ComponentHasTag(TEXT("ERTemplateRoof")))Part->SetVisibility(!Plan);
        FVector Eye,Target;
        if(Plan){Eye={0,0,4000};Target=FVector::ZeroVector;}
        else if(Kind==1){Eye={0,-HL+120,172};Target={0,HL*.4f,150};}
        else if(Kind==2){Eye={W*.5f-120,-HL+150,L.IndoorCeiling-60};Target={-W*.2f,HL*.2f,120};}
        else if(!L.CrossingRoutes.IsEmpty()){const auto& R=L.CrossingRoutes[0];Eye=R[0]+FVector(0,-40,70);Target=R.Last()+FVector(0,0,40);}
        else{Eye={-W*.5f+150,0,L.IndoorCeiling-80};Target={W*.3f,-HL*.5f,100};}
        auto* Camera=GetWorld()->SpawnActor<ACameraActor>(Eye,Plan ? FRotator(-90,0,0):(Target-Eye).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(Kind==3 ? 80:90);
        if(Plan)
        {
            Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
            Camera->GetCameraComponent()->SetAspectRatio(16.f/9);
            Camera->GetCameraComponent()->OrthoWidth=FMath::Max(L.ArenaSize.Y,L.ArenaSize.X*16.f/9)*1.04f;
        }
        PC->SetViewTarget(Camera);Phase=2;StageTime=Elapsed;
    }
    else if(Phase==2 && Elapsed-StageTime>3)
    {
        if(FApp::CanEverRender())FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/FireTraining-%d-%s.png"),Seed,Names[Shot%PerSeed]),false,false);
        Phase=3;StageTime=Elapsed;
    }
    else if(Phase==3 && Elapsed-StageTime>1){++Shot;Phase=1;}
}
