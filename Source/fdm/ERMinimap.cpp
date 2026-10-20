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
    const FVector2D Center(Canvas->SizeX-Radius-30,Radius+90);
    constexpr float ViewSpan=8000.f; // Fixed 80m diameter, independent of map extent.
    const FVector ViewCenter=PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : Frame.Center;
    const FVector Offset=(ViewCenter-Frame.Center)/Frame.Span;
    const FVector2D MapOrigin(Offset.Y,-Offset.X);
    const float MapScale=Frame.Span/ViewSpan;
    auto ToView = [&](const FVector2D& P) { return (P-MapOrigin)*(2.f*MapScale); };
    auto ToUV = [&](const FVector2D& P) { return P/(2.f*MapScale)+MapOrigin+FVector2D(.5); };
    constexpr int32 DiscSegments=128;
    const auto Color = ERRecon::TeamColor(Data.Team);
    // Use the same local heading as the owner's marker, every rendered frame.
    const float ViewYaw=PC->GetControlRotation().Yaw;
    float Sin, Cos; FMath::SinCos(&Sin,&Cos,FMath::DegreesToRadians(ViewYaw));
    // Fog and the cached bird image retain their shared world-space grid.
    // Rotate every displayed position around the disc using the same basis.
    auto Orient = [&](const FVector2D& P) { return FVector2D(Cos*P.X+Sin*P.Y,-Sin*P.X+Cos*P.Y); };
    auto Circle = [&](FVector2D At,float R,FLinearColor Ink,float Feather=1.f)
    {
        TArray<FCanvasUVTri> Tris;
        auto Add = [&](FVector2D A,FLinearColor CA,FVector2D B,FLinearColor CB,FVector2D C,FLinearColor CC)
        {
            FCanvasUVTri Tri;
            Tri.V0_Pos=A; Tri.V1_Pos=B; Tri.V2_Pos=C;
            Tri.V0_UV=Tri.V1_UV=Tri.V2_UV=FVector2D::ZeroVector;
            Tri.V0_Color=CA; Tri.V1_Color=CB; Tri.V2_Color=CC; Tris.Add(Tri);
        };
        const auto Clear=Ink.CopyWithNewOpacity(0);
        for (int32 I=0; I<DiscSegments; ++I)
        {
            const float A=I*2*PI/DiscSegments, B=(I+1)*2*PI/DiscSegments;
            const FVector2D DA(FMath::Cos(A),FMath::Sin(A)), DB(FMath::Cos(B),FMath::Sin(B));
            const FVector2D IA=At+DA*(R-Feather), IB=At+DB*(R-Feather), OA=At+DA*R, OB=At+DB*R;
            Add(At,Ink,IA,Ink,IB,Ink);
            Add(IA,Ink,OA,Clear,OB,Clear); Add(IA,Ink,OB,Clear,IB,Ink);
        }
        FCanvasTriangleItem Item(Tris,GWhiteTexture); Item.BlendMode=SE_BLEND_Translucent; Canvas->DrawItem(Item);
    };
    // A feathered circular shadow, followed by an antialiased dark frame.
    Circle(Center+FVector2D(0,4),Radius+17,FLinearColor(0,0,0,.5),16);
    Circle(Center,Radius+5,FLinearColor(.005,.009,.014,.96),1.5);
    Circle(Center,Radius,FLinearColor(.015,.021,.028,1),1.25);
    const bool HasImage = CapturedEpoch == Frame.Epoch && BirdView && BirdView->GetResource();
    const FTexture* Texture=HasImage ? BirdView->GetResource() : GWhiteTexture;
    TArray<FCanvasUVTri> Tiles;
    // Transform the existing world fog grid into the local fixed-scale view,
    // then clip terrain and fog together. UVs still address the whole-map capture.
    for (int32 Cell=0; Cell<ERRecon::Grid*ERRecon::Grid; ++Cell)
    {
        if (!ERRecon::IsExplored(Data.Explored,Cell)) continue;
        const float X=float(Cell%ERRecon::Grid)/ERRecon::Grid-.5f;
        const float Y=float(Cell/ERRecon::Grid)/ERRecon::Grid-.5f;
        const float Step=1.f/ERRecon::Grid;
        TArray<FVector2D,TInlineAllocator<16>> Polygon;
        Polygon.Append({ToView(FVector2D(X,Y)),ToView(FVector2D(X+Step,Y)),ToView(FVector2D(X+Step,Y+Step)),ToView(FVector2D(X,Y+Step))});
        if (ToView(FVector2D(X+Step*.5,Y+Step*.5)).SizeSquared()>FMath::Square(1+Step*MapScale*1.415f)) continue;
        bool Inside=true; for (const auto& P:Polygon) Inside &= P.SizeSquared()<=.997;
        if (!Inside) for (int32 Edge=0; Edge<DiscSegments && Polygon.Num(); ++Edge)
        {
            const float Angle=(Edge+.5f)*2*PI/DiscSegments;
            const FVector2D Normal(FMath::Cos(Angle),FMath::Sin(Angle));
            const float Limit=FMath::Cos(PI/DiscSegments);
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
            Tri.V0_UV=ToUV(Polygon[0]); Tri.V1_UV=ToUV(Polygon[I]); Tri.V2_UV=ToUV(Polygon[I+1]);
            const float Shade=HasImage ? .85f : .22f;
            const auto EdgeInk = [&](const FVector2D& P)
            { return FLinearColor(Shade,Shade,Shade,FMath::Clamp((1.f-P.Size())*Radius/1.25f,0.f,1.f)); };
            Tri.V0_Color=EdgeInk(Polygon[0]); Tri.V1_Color=EdgeInk(Polygon[I]); Tri.V2_Color=EdgeInk(Polygon[I+1]);
            Tiles.Add(Tri);
        }
    }
    if (Tiles.Num()) { FCanvasTriangleItem Item(Tiles,Texture); Item.BlendMode=SE_BLEND_Translucent; Canvas->DrawItem(Item); }
    auto Project = [&](const FVector& Position)
    {
        const FVector P=(Position-ViewCenter)/ViewSpan;
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
    TArray<FCanvasUVTri> Rim;
    for (int32 I=0; I<DiscSegments; ++I)
    {
        const float A=I*2*PI/DiscSegments, B=(I+1)*2*PI/DiscSegments;
        const FVector2D DA(FMath::Cos(A),FMath::Sin(A)), DB(FMath::Cos(B),FMath::Sin(B));
        for (int32 Band=0; Band<2; ++Band)
        {
            const float Inner=Radius-1.5f+Band*1.5f, Outer=Inner+1.5f;
            const auto CI=Color.CopyWithNewOpacity(Band ? .75f : 0.f), CO=Color.CopyWithNewOpacity(Band ? 0.f : .75f);
            FCanvasUVTri T;
            T.V0_Pos=Center+DA*Inner; T.V1_Pos=Center+DA*Outer; T.V2_Pos=Center+DB*Outer;
            T.V0_Color=CI; T.V1_Color=T.V2_Color=CO; Rim.Add(T);
            T.V1_Pos=T.V2_Pos; T.V2_Pos=Center+DB*Inner; T.V2_Color=CI; Rim.Add(T);
        }
    }
    FCanvasTriangleItem RimItem(Rim,GWhiteTexture); RimItem.BlendMode=SE_BLEND_Translucent; Canvas->DrawItem(RimItem);
    const FVector2D North=Center+Orient(FVector2D(0,-1))*(Radius+18);
    const float TextScale=FMath::Clamp(Canvas->SizeY/720.f,.75f,2.f);
    // The disc is capped in pixels, so its labels must follow its size too.
    const float LabelScale=Radius/115.2f;
    HUD->DrawHUDText(TEXT("N"),North.X,North.Y-14*LabelScale,FLinearColor::White,.85f*LabelScale/TextScale,true);
    FString Status=FString::Printf(TEXT("RECON  %d / 5"),Data.Remaining);
    if (Data.Airborne) Status+=TEXT("  •  LIVE");
    else if (Data.NextLaunch>0) Status+=FString::Printf(TEXT("  •  %ds"),FMath::Max(0,FMath::CeilToInt(Data.NextLaunch-Now)));
    else Status+=TEXT("  •  OFFLINE");
    HUD->DrawHUDText(Status,Center.X,Center.Y+Radius+30+20*LabelScale,Color,.9f*LabelScale/TextScale,true);
}
