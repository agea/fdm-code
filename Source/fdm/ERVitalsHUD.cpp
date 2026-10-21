#include "ERPlayerController.h"
#include "ERHUDLayout.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "GlobalRenderResources.h"

void AERHUD::DrawVitals(const AERCharacter* Character)
{
    const auto* Movement=Cast<UERMovement>(Character->GetCharacterMovement());
    const FERHUDLayout Layout(FVector2D(Canvas->SizeX,Canvas->SizeY));
    const float Scale=Layout.Scale;
    const FVector2D Edge=Layout.Edge();
    const float IconSize=18.f*Scale, RowStep=24.f*Scale;
    const float Top=Canvas->SizeY-Edge.Y-IconSize-2.f*RowStep;
    const float BarX=Edge.X+IconSize+8.f*Scale, BarWidth=180.f*Scale, BarHeight=8.f*Scale;
    // Muted sRGB colors shared by each bar and its icon.
    const FLinearColor Colors[]={FLinearColor(FColor(154,84,84)),FLinearColor(FColor(86,132,100)),FLinearColor(FColor(82,130,154))};
    const float Fractions[]={Character->Health->Health/100.f,Character->Health->Armor/100.f,Movement->GetStaminaFraction()};
    TArray<FCanvasUVTri> Icons;
    for(int32 Row=0;Row<3;++Row)
    {
        const float Y=Top+Row*RowStep, BarY=Y+(IconSize-BarHeight)*.5f;
        DrawRect(FLinearColor(0,0,0,.65f),BarX-Scale,BarY-Scale,BarWidth+2.f*Scale,BarHeight+2.f*Scale);
        DrawRect(FLinearColor(.055f,.065f,.08f,.9f),BarX,BarY,BarWidth,BarHeight);
        DrawRect(Colors[Row],BarX,BarY,BarWidth*FMath::Clamp(Fractions[Row],0.f,1.f),BarHeight);
        // Normalized, code-drawn silhouettes stay crisp without fonts or texture assets.
        for(int32 Pass=0;Pass<2;++Pass)
        {
            const FLinearColor Ink=Pass ? Colors[Row] : FLinearColor(0,0,0,.9f);
            const float Size=IconSize+(Pass ? 0.f : 2.f*Scale);
            const FVector2D Origin(Edge.X-(Pass ? 0.f : Scale),Y-(Pass ? 0.f : Scale));
            auto Tri=[&](FVector2D A,FVector2D B,FVector2D C)
            {
                FCanvasUVTri T;
                T.V0_Pos=Origin+A*Size;T.V1_Pos=Origin+B*Size;T.V2_Pos=Origin+C*Size;
                T.V0_UV=T.V1_UV=T.V2_UV=FVector2D::ZeroVector;
                T.V0_Color=T.V1_Color=T.V2_Color=Ink;Icons.Add(T);
            };
            auto Quad=[&](FVector2D A,FVector2D B,FVector2D C,FVector2D D){Tri(A,B,C);Tri(A,C,D);};
            if(Row==0) // Medical cross.
            {
                Quad({.34,0},{.66,0},{.66,1},{.34,1});
                Quad({0,.34},{.34,.34},{.34,.66},{0,.66});
                Quad({.66,.34},{1,.34},{1,.66},{.66,.66});
            }
            else if(Row==1) // Shield, with a shallow shoulder and pointed base.
            {
                const FVector2D P[]={{.5,0},{.94,.17},{.87,.65},{.5,1},{.13,.65},{.06,.17}};
                for(int32 I=1;I<5;++I)Tri(P[0],P[I],P[I+1]);
            }
            else // Forward-leaning runner with bent arms and separated strides.
            {
                auto Disc=[&](FVector2D Center,float Radius)
                {
                    for(int32 I=0;I<16;++I)
                    {
                        const float A=2.f*PI*I/16.f,B=2.f*PI*(I+1)/16.f;
                        Tri(Center,Center+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius,
                            Center+FVector2D(FMath::Cos(B),FMath::Sin(B))*Radius);
                    }
                };
                const float Stroke=Pass ? .105f : .15f;
                auto Limb=[&](FVector2D A,FVector2D B,float Width)
                {
                    const FVector2D D=(B-A).GetSafeNormal(),N(-D.Y*Width*.5f,D.X*Width*.5f);
                    Quad(A+N,B+N,B-N,A-N);
                    Disc(A,Width*.5f);Disc(B,Width*.5f);
                };
                Disc({.64,.12},Pass ? .105f : .125f);
                Limb({.56,.32},{.43,.59},Stroke*1.35f);
                Limb({.56,.34},{.75,.46},Stroke);Limb({.75,.46},{.92,.46},Stroke);
                Limb({.55,.32},{.31,.33},Stroke);Limb({.31,.33},{.2,.51},Stroke);
                Limb({.43,.59},{.67,.71},Stroke);Limb({.67,.71},{.67,.94},Stroke);
                Limb({.43,.59},{.26,.8},Stroke);Limb({.26,.8},{.06,.8},Stroke);
            }
        }
    }
    FCanvasTriangleItem Item(Icons,GWhiteTexture);Item.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Item);
    if(Movement->bProneWanted || Movement->IsProne())
        DrawHUDTextAnchored(TEXT("PRONE  |  C / Ctrl to stand up"),FVector2D(Edge.X,Top-6.f*Scale),FLinearColor::White,.85f,FVector2D(0,1));
}
