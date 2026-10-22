#include "ERRecon.h"
#include "ERProceduralMap.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Kismet/KismetMathLibrary.h"

void AERReconDirector::DecorateBases()
{
    for(UDecalComponent* Mark:Graffiti) if(IsValid(Mark)) Mark->DestroyComponent();
    Graffiti.Reset(); GraffitiEpoch=Frame.Epoch;
    FVector Starts[2]; bool Found[2]={false,false};
    FVector Size(Frame.Span*.7,Frame.Span*.7,0), Center=Frame.Center;
    int32 Seed=17413;
    if(TActorIterator<AERProceduralMap> It(GetWorld());It)
    {
        const auto& L=It->GetLayout(); Size=L.ArenaSize; Center=It->GetActorLocation();
        Seed=It->LayoutConfig.Seed;
        for(int32 T=0;T<2;++T)
        {
            const auto& Points=T ? L.OmegaSpawns : L.AlphaSpawns;
            if(!Points.IsEmpty()) { Starts[T]=It->GetActorTransform().TransformPosition(Points[0].GetLocation()); Found[T]=true; }
        }
    }
    else for(TActorIterator<AERTeamPlayerStart> StartIt(GetWorld());StartIt;++StartIt)
    {
        const int32 T=StartIt->Team==EERTeam::Omega ? 1 : 0;
        if(StartIt->Team!=EERTeam::None && !Found[T]) { Starts[T]=StartIt->GetActorLocation(); Found[T]=true; }
    }
    if(!Found[0] || !Found[1]) return;
    FVector Axis=Starts[1]-Starts[0]; Axis.Z=0;
    const double Length=Axis.Size(); if(Length<100) return; Axis/=Length;
    FRandomStream Random(Seed ^ 0x47af);
    auto Query=SightQuery(); TArray<FVector> Positions;
    // A sparse 5.5m grid: probability falls toward the unmarked central 30%.
    for(int32 T=0;T<2;++T)
    {
        auto* Material=T ? OmegaGraffiti.Get() : AlphaGraffiti.Get(); if(!Material) continue;
        int32 Count=0;
        for(double X=-Size.X*.5+275; X<Size.X*.5 && Count<36; X+=550)
        for(double Y=-Size.Y*.5+275; Y<Size.Y*.5 && Count<36; Y+=550)
        {
            FVector P=Center+FVector(X,Y,0);
            const double Along=FVector::DotProduct(P-Starts[0],Axis)/Length;
            const double Depth=T ? 1-Along : Along;
            if(Depth>.35 || Depth<-.25 || Random.FRand()>.62f*(1-FMath::Clamp(Depth/ .35,0.,1.))) continue;
            FHitResult Ground;
            if(!GetWorld()->LineTraceSingleByChannel(Ground,P+FVector(0,0,Frame.Top+300-Center.Z),P-FVector(0,0,600),ECC_Visibility,Query)) continue;
            P.Z=Ground.ImpactPoint.Z+145;
            for(int32 D=0;D<8;++D)
            {
                const FVector Ray=FRotator(0,D*45,0).Vector(); FHitResult Hit;
                if(!GetWorld()->LineTraceSingleByChannel(Hit,P,P+Ray*600,ECC_Visibility,Query) || FMath::Abs(Hit.ImpactNormal.Z)>.12) continue;
                const float Half=Random.FRandRange(48,72);
                const FVector Tangent=FVector::CrossProduct(FVector::UpVector,Hit.ImpactNormal).GetSafeNormal();
                const double HitAlong=FVector::DotProduct(Hit.ImpactPoint-Starts[0],Axis)/Length;
                // Include the whole image footprint in the central exclusion.
                if((T ? 1-HitAlong : HitAlong)+(Half+15)/Length>.35) continue;
                if(Positions.ContainsByPredicate([&](const FVector& V){return FVector::DistSquared(V,Hit.ImpactPoint)<FMath::Square(500.f);})) continue;
                bool Fits=true;
                for(int32 A:{-1,1}) for(int32 B:{-1,1})
                {
                    const FVector Corner=Hit.ImpactPoint+Tangent*Half*A+FVector::UpVector*Half*B;
                    FHitResult Edge;
                    Fits &= GetWorld()->LineTraceSingleByChannel(Edge,Corner+Hit.ImpactNormal*20,Corner-Hit.ImpactNormal*20,ECC_Visibility,Query)
                        && FVector::DotProduct(Edge.ImpactNormal,Hit.ImpactNormal)>.98;
                }
                if(!Fits) continue;
                auto* Mark=NewObject<UDecalComponent>(this);
                Mark->SetDecalMaterial(Material); Mark->DecalSize=FVector(4,Half,Half);
                Mark->SetFadeScreenSize(.001f);
                Mark->SetWorldLocation(Hit.ImpactPoint+Hit.ImpactNormal);
                Mark->SetWorldRotation(UKismetMathLibrary::MakeRotFromXY(-Hit.ImpactNormal,-FVector::UpVector));
                Mark->RegisterComponent(); Graffiti.Add(Mark); Positions.Add(Hit.ImpactPoint); ++Count; break;
            }
        }
        UE_LOG(LogTemp,Display,TEXT("ER_BASE_GRAFFITI team=%d count=%d epoch=%u"),T,Count,Frame.Epoch);
    }
}

void AERReconDirector::TickGraffitiGallery()
{
#if !UE_BUILD_SHIPPING
    if(!FParse::Param(FCommandLine::Get(),TEXT("ERGraffitiGallery")) || !HasAuthority()) return;
    auto* PC=GetWorld()->GetFirstPlayerController(); auto* Base=Teams[0].Base.Get();
    if(!PC || !Base || GetWorld()->GetTimeSeconds()<8) return;
    if(GraffitiGalleryStart<0)
    {
        GraffitiGalleryStart=GetWorld()->GetTimeSeconds();
        GraffitiCamera=GetWorld()->SpawnActor<ACameraActor>();
        GraffitiCamera->GetCameraComponent()->SetFieldOfView(60);
        const FVector Focus=Base->GetActorLocation()+FVector(0,0,25);
        GraffitiCamera->SetActorLocation(Focus+Base->GetActorRotation().RotateVector(FVector(115,125,105)));
        GraffitiCamera->SetActorRotation((Focus-GraffitiCamera->GetActorLocation()).Rotation()); PC->SetViewTarget(GraffitiCamera);
        Base->LaunchAt=GetWorld()->GetGameState()->GetServerWorldTimeSeconds()+2;
    }
    const double Age=GetWorld()->GetTimeSeconds()-GraffitiGalleryStart;
    const double Times[]={1.5,3.6,4.1,6.5,7,9,11};
    if(GraffitiGalleryStep>=7 || Age<Times[GraffitiGalleryStep]) return;
    if(GraffitiGalleryStep<4)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/GraffitiCase%d.png"),GraffitiGalleryStep),false,false);
        UE_LOG(LogTemp,Display,TEXT("ER_GRAFFITI_CASE_CAPTURE step=%d roll=%.1f"),GraffitiGalleryStep,Base->Lid->GetRelativeRotation().Roll);
    }
    else if(GraffitiGalleryStep==4)
    {
        if(!Graffiti.IsEmpty())
        {
            auto* Mark=Graffiti[0].Get(); const FVector Focus=Mark->GetComponentLocation();
            GraffitiCamera->SetActorLocation(Focus-Mark->GetForwardVector()*300);
            GraffitiCamera->SetActorRotation((Focus-GraffitiCamera->GetActorLocation()).Rotation());
        }
    }
    else if(GraffitiGalleryStep==5) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Testing/GraffitiWall.png"),false,false);
    else
    {
        UE_LOG(LogTemp,Display,TEXT("ER_GRAFFITI_GALLERY_PASS marks=%d"),Graffiti.Num());
        FPlatformMisc::RequestExitWithStatus(false,Graffiti.IsEmpty()?1:0);
    }
    ++GraffitiGalleryStep;
#endif
}
