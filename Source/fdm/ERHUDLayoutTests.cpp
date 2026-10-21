#include "ERHUDLayout.h"
#include "ERPlayerController.h"
#include "ERRecon.h"
#include "Engine/Canvas.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// Opt-in rendered resize regression: exercise the same live viewport and DPI
// path used when changing window size, including the scoreboard and Slate FPS.
void AERHUD::TickHUDLayoutGallery()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(),TEXT("ERHUDGallery"))) return;
    auto* PC=Cast<AERPlayerController>(PlayerOwner);
    if (!PC || !PC->Recon || !PC->Recon->Snapshot.Epoch || GetWorld()->GetTimeSeconds()<HUDGalleryNextAt) return;
    const FIntPoint Sizes[]={{640,360},{1280,720},{1920,1080},{3840,2160},{2560,1080},{1024,768},{720,1280}};
    const int32 Index=HUDGalleryStep/3, Phase=HUDGalleryStep%3;
    if (Index>=UE_ARRAY_COUNT(Sizes))
    {
        UE_LOG(LogTemp,Display,TEXT("ER_HUD_GALLERY_COMPLETE"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    if (Phase==0)
    {
        PC->bShowScoreboard=false;
        PC->ConsoleCommand(FString::Printf(TEXT("r.SetRes %dx%dw"),Sizes[Index].X,Sizes[Index].Y));
    }
    else
    {
        const FERHUDLayout Layout(FVector2D(Canvas->SizeX,Canvas->SizeY));
        UE_LOG(LogTemp,Display,TEXT("ER_HUD_GALLERY index=%d scoreboard=%d viewport=%dx%d scale=%.3f radius=%.2f edge=%.2f,%.2f"),
            Index,Phase==2,Canvas->SizeX,Canvas->SizeY,Layout.Scale,Layout.MinimapRadius(),Layout.Edge().X,Layout.Edge().Y);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/HUD/%d-%s.png"),Index,Phase==2?TEXT("scoreboard"):TEXT("hud")),true,false);
        PC->bShowScoreboard=Phase==1;
    }
    ++HUDGalleryStep;
    HUDGalleryNextAt=GetWorld()->GetTimeSeconds()+2;
#endif
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERHUDLayoutTest,"EclipseRecoil.Player.HUDViewportLayout",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERHUDLayoutTest::RunTest(const FString&)
{
    const FERHUDLayout Base(FVector2D(1280,720));
    for (const float Factor : {.5f,1.f,1.5f,2.f,3.f,4.f})
    {
        const FERHUDLayout L(Base.Viewport*Factor);
        TestTrue(TEXT("Minimap grows proportionally below and above the old pixel caps"),FMath::IsNearlyEqual(L.MinimapRadius(),Base.MinimapRadius()*Factor));
        TestTrue(TEXT("Corner offsets stay proportional on both axes"),L.Edge().Equals(Base.Edge()*Factor));
        TestTrue(TEXT("Minimap keeps the same normalized placement"),L.MinimapCenter().Equals(Base.MinimapCenter()*Factor));
    }
    for (const FVector2D Size : {FVector2D(1024,768),FVector2D(2560,1080),FVector2D(720,1280)})
    {
        const FERHUDLayout L(Size);
        TestTrue(TEXT("Horizontal inset is a fraction of width"),FMath::IsNearlyEqual(L.Edge().X/Size.X,40./1280));
        TestTrue(TEXT("Vertical inset is a fraction of height"),FMath::IsNearlyEqual(L.Edge().Y/Size.Y,40./720));
        const auto C=L.MinimapCenter(); const float R=L.MinimapRadius()+28*L.Scale;
        TestTrue(TEXT("Minimap, north marker and shadow fit every tested aspect ratio"),C.X-R>0 && C.Y-R>0 && C.X+R<Size.X && C.Y+R+80*L.Scale<Size.Y);
        TestTrue(TEXT("Vitals fit beside bottom-right ammo"),L.Edge().X+206*L.Scale<Size.X*.5);
    }
    return true;
}
#endif
