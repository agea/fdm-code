#include "ERRecon.h"
#include "ERPlayerController.h"
#include "ERBlockRoom.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "CanvasItem.h"
#include "GlobalRenderResources.h"
#include "Misc/CommandLine.h"

void AERReconDirector::DrawMinimap(AERHUD* HUD, AERPlayerController* PC)
{
    if (!HUD || !HUD->Canvas || !PC->Recon || PC->IsBlockMapLoading()) return;
    for (TActorIterator<AERBlockRoom> It(GetWorld()); It; ++It)
        if (!It->IsMapReady() || It->GetMapEpoch()!=Frame.GeometryEpoch) return;
    const auto& Data = PC->Recon->Snapshot;
    const auto* PS = PC->GetPlayerState<AERPlayerState>();
    if (!PS || Data.Team != PS->Team || Data.Team == EERTeam::None || Data.Epoch != Frame.Epoch) return;
    UCanvas* Canvas = HUD->Canvas;
    const float Radius=FMath::Clamp(Canvas->SizeY*.16f,82.f,145.f);
    const FVector2D Center(Canvas->SizeX-Radius-22,Radius+77);
    const auto Color = ERRecon::TeamColor(Data.Team);
    // Use the same local heading as the owner's marker, every rendered frame.
    const float ViewYaw=PC->GetControlRotation().Yaw;
    float Sin, Cos; FMath::SinCos(&Sin,&Cos,FMath::DegreesToRadians(ViewYaw));
    // Fog and the cached bird image retain their shared world-space grid.
    // Rotate every displayed position around the disc using the same basis.
    auto Orient = [&](const FVector2D& P) { return FVector2D(Cos*P.X+Sin*P.Y,-Sin*P.X+Cos*P.Y); };
    auto Circle = [&](FVector2D At,float R,FLinearColor Ink)
    {
        TArray<FCanvasUVTri> Tris;
        for (int32 I=0; I<64; ++I)
        {
            const float A=I*2*PI/64, B=(I+1)*2*PI/64;
            FCanvasUVTri Tri;
            Tri.V0_Pos=At; Tri.V1_Pos=At+FVector2D(FMath::Cos(A),FMath::Sin(A))*R; Tri.V2_Pos=At+FVector2D(FMath::Cos(B),FMath::Sin(B))*R;
            Tri.V0_UV=Tri.V1_UV=Tri.V2_UV=FVector2D::ZeroVector; Tri.V0_Color=Tri.V1_Color=Tri.V2_Color=Ink;
            Tris.Add(Tri);
        }
        FCanvasTriangleItem Item(Tris,GWhiteTexture); Item.BlendMode=SE_BLEND_Translucent; Canvas->DrawItem(Item);
    };
    Circle(Center,Radius+5,FLinearColor(.005,.009,.014,.96));
    Circle(Center,Radius,FLinearColor(.015,.021,.028,1));
    const bool HasImage = CapturedEpoch == Frame.Epoch && BirdView && BirdView->GetResource();
    const FTexture* Texture=HasImage ? BirdView->GetResource() : GWhiteTexture;
    TArray<FCanvasUVTri> Tiles;
    // Tile polygons are clipped against a 64-sided disc; no fog or bird-view
    // rectangle leaks outside the circular frame, even at a partially seen edge.
    for (int32 Cell=0; Cell<ERRecon::Grid*ERRecon::Grid; ++Cell)
    {
        if (!ERRecon::IsExplored(Data.Explored,Cell)) continue;
        const float X=float(Cell%ERRecon::Grid)/ERRecon::Grid*2-1;
        const float Y=float(Cell/ERRecon::Grid)/ERRecon::Grid*2-1;
        const float Step=2.f/ERRecon::Grid;
        TArray<FVector2D,TInlineAllocator<16>> Polygon;
        Polygon.Append({FVector2D(X,Y),FVector2D(X+Step,Y),FVector2D(X+Step,Y+Step),FVector2D(X,Y+Step)});
        if (FVector2D(X+Step*.5,Y+Step*.5).SizeSquared()>FMath::Square(1+Step)) continue;
        bool Inside=true; for (const auto& P:Polygon) Inside &= P.SizeSquared()<=.997;
        if (!Inside) for (int32 Edge=0; Edge<64 && Polygon.Num(); ++Edge)
        {
            const float Angle=(Edge+.5f)*2*PI/64;
            const FVector2D Normal(FMath::Cos(Angle),FMath::Sin(Angle));
            const float Limit=FMath::Cos(PI/64);
            TArray<FVector2D,TInlineAllocator<16>> Clipped;
            for (int32 I=0; I<Polygon.Num(); ++I)
            {
                const FVector2D A=Polygon[I], B=Polygon[(I+1)%Polygon.Num()];
                const float DA=FVector2D::DotProduct(A,Normal)-Limit, DB=FVector2D::DotProduct(B,Normal)-Limit;
                if (DA<=0) Clipped.Add(A);
                if ((DA<0 && DB>0) || (DA>0 && DB<0)) Clipped.Add(FMath::Lerp(A,B,DA/(DA-DB)));
            }
            Polygon=MoveTemp(Clipped);
        }
        for (int32 I=1; I+1<Polygon.Num(); ++I)
        {
            FCanvasUVTri Tri;
            Tri.V0_Pos=Center+Orient(Polygon[0])*Radius; Tri.V1_Pos=Center+Orient(Polygon[I])*Radius; Tri.V2_Pos=Center+Orient(Polygon[I+1])*Radius;
            Tri.V0_UV=(Polygon[0]+FVector2D(1))*.5; Tri.V1_UV=(Polygon[I]+FVector2D(1))*.5; Tri.V2_UV=(Polygon[I+1]+FVector2D(1))*.5;
            const float Shade=HasImage ? .85f : .22f;
            Tri.V0_Color=Tri.V1_Color=Tri.V2_Color=FLinearColor(Shade,Shade,Shade,1);
            Tiles.Add(Tri);
        }
    }
    if (Tiles.Num()) { FCanvasTriangleItem Item(Tiles,Texture); Item.BlendMode=SE_BLEND_Opaque; Canvas->DrawItem(Item); }
    auto Project = [&](const FVector& Position)
    {
        const FVector P=(Position-Frame.Center)/Frame.Span;
        return Center+Orient(FVector2D(P.Y,-P.X))*(2*Radius);
    };
    const auto* GS=GetWorld()->GetGameState();
    const double Now=GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    // Packet loss must never turn the last enemy sample into a lasting tracker.
    LastDrawnEnemies=0;
    if (Now-Data.SampleTime<1) for (const auto& Contact:Data.Contacts)
    {
        FVector Position=Contact.Position;
        float Yaw=Contact.Yaw;
        if (Contact.Self && PC->GetPawn()) { Position=PC->GetPawn()->GetActorLocation(); Yaw=PC->GetControlRotation().Yaw; }
        FVector2D P=Project(Position);
        const float Distance=(P-Center).Size();
        if (Distance>Radius) continue;
        // Keep a known contact near the rim legible instead of discarding it
        // merely because its outlined marker would overlap the circular edge.
        if (Distance>Radius-9) P=Center+(P-Center)*((Radius-9)/Distance);
        const auto Ink=ERRecon::TeamColor(Contact.Team);
        if (Contact.Drone)
        {
            for (const FVector2D D : {FVector2D(1,1),FVector2D(1,-1)}) HUD->DrawLine(P.X-D.X*4,P.Y-D.Y*4,P.X+D.X*4,P.Y+D.Y*4,Ink,1.7);
            continue;
        }
        const bool Enemy=Contact.Team!=Data.Team;
        if (Enemy)
        {
            ++LastDrawnEnemies;
            Circle(P,8,FLinearColor(.95,.95,.95,1)); Circle(P,6.7,FLinearColor::Black);
            TArray<FCanvasUVTri> Diamond;
            for (int32 I=0;I<2;++I)
            {
                FCanvasUVTri Tri;
                Tri.V0_Pos=P+FVector2D(0,I?-5.2:5.2); Tri.V1_Pos=P+FVector2D(I?5.2:-5.2,0); Tri.V2_Pos=P+FVector2D(I?-5.2:5.2,0);
                Tri.V0_UV=Tri.V1_UV=Tri.V2_UV=FVector2D::ZeroVector; Tri.V0_Color=Tri.V1_Color=Tri.V2_Color=Ink;
                Diamond.Add(Tri);
            }
            FCanvasTriangleItem Item(Diamond,GWhiteTexture); Item.BlendMode=SE_BLEND_Translucent; Canvas->DrawItem(Item);
        }
        else { Circle(P,Contact.Self?7:5.5,FLinearColor::Black); Circle(P,Contact.Self?5.2:4.2,Ink); }
        const float Angle=FMath::DegreesToRadians(Yaw);
        const FVector2D Forward=Orient(FVector2D(FMath::Sin(Angle),-FMath::Cos(Angle)));
        HUD->DrawLine(P.X,P.Y,P.X+Forward.X*10,P.Y+Forward.Y*10,Contact.Self?FLinearColor::White:Ink,1.5);
    }
    if (LastHUDSample!=Data.SampleTime && FParse::Param(FCommandLine::Get(),TEXT("ERReconAudit")))
    {
        LastHUDSample=Data.SampleTime;
        UE_LOG(LogTemp,Display,TEXT("ER_RECON_HUD epoch=%u team=%d enemies_drawn=%d age=%.2f view_yaw=%.1f"),Frame.Epoch,int32(Data.Team),LastDrawnEnemies,Now-Data.SampleTime,ViewYaw);
    }
    for (int32 I=0; I<96; ++I)
    {
        const float A=I*2*PI/96,B=(I+1)*2*PI/96;
        HUD->DrawLine(Center.X+FMath::Cos(A)*Radius,Center.Y+FMath::Sin(A)*Radius,Center.X+FMath::Cos(B)*Radius,Center.Y+FMath::Sin(B)*Radius,Color.CopyWithNewOpacity(.7),1.5);
    }
    const FVector2D North=Center+Orient(FVector2D(0,-1))*(Radius+18);
    HUD->DrawText(TEXT("N"),FLinearColor::White,North.X-4,North.Y-5,nullptr,.85);
    FString Status=FString::Printf(TEXT("RECON  %d / 5"),Data.Remaining);
    if (Data.Airborne) Status+=TEXT("  •  LIVE");
    else if (Data.NextLaunch>0) Status+=FString::Printf(TEXT("  •  %ds"),FMath::Max(0,FMath::CeilToInt(Data.NextLaunch-Now)));
    else Status+=TEXT("  •  OFFLINE");
    HUD->DrawText(Status,Color,Center.X-Radius+8,Center.Y+Radius+30,nullptr,.85);
}
