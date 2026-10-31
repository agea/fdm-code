#include "ERAlleyDestruction.h"
#include "ERProceduralMap.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

namespace
{
FCollisionQueryParams Query(UWorld* World)
{
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ERArchitecture),false);
    for(TActorIterator<APawn> P(World);P;++P)Q.AddIgnoredActor(*P);return Q;
}
}
bool ERAlleyDestruction::FindWall(AERProceduralMap* Map,FBreach& Out,TOptional<EERBlockMaterial> Only)
{
    auto* W=Map->GetWorld();auto Q=Query(W);const auto Capsule=FCollisionShape::MakeCapsule(34,92.5);
    for(const auto& H:Map->GetLayout().Houses)
    {
        const auto Size=H.Frame.GetScale3D();const FTransform Frame(H.Frame.GetRotation(),H.Frame.GetLocation());
        for(int32 Axis=0;Axis<2;++Axis)for(int32 Sign:{-1,1})for(float Fraction:{-.3f,0.f,.3f})
        {
            FVector A(0,0,94),B;A[Axis]=Sign*(Size[Axis]*.5+100);A[1-Axis]=Size[1-Axis]*Fraction;
            B=A;B[Axis]=Sign*(Size[Axis]*.5-150);A=Frame.TransformPosition(A);B=Frame.TransformPosition(B);
            auto Stand=[&](FVector& P)
            {
                FHitResult Floor;if(!W->LineTraceSingleByChannel(Floor,P+FVector(0,0,100),P-FVector(0,0,130),ECC_WorldStatic,Q))return false;
                P.Z=Floor.ImpactPoint.Z+94;return !W->OverlapBlockingTestByChannel(P,FQuat::Identity,ECC_Pawn,Capsule,Q);
            };
            if(!Stand(A) || !Stand(B) || FMath::Abs(A.Z-B.Z)>35)continue;
            A.Z=B.Z=FMath::Max(A.Z,B.Z)+3;FHitResult Hit;FERCellId Cell;
            if(!W->LineTraceSingleByChannel(Hit,A,B,ECC_GameTraceChannel1,Q) || Hit.GetActor()!=Map || !Hit.GetComponent()->ComponentHasTag(H.Mesh ? H.Mesh->GetFName():H.StructureName) || !Map->ResolveCell(Hit,Cell))continue;
            if(Only && Map->GetCellWorld().GetPanels()[Cell.Panel].Material!=*Only)continue;
            if(!W->SweepTestByChannel(A,B,FQuat::Identity,ECC_Pawn,Capsule,Q))continue;
            auto Without=Q;Without.AddIgnoredComponent(Hit.GetComponent());
            // The new hole belongs to one building. Reject routes where the
            // capsule head would also cross a different neighbouring house.
            if(W->SweepTestByChannel(A+FVector(0,0,30),B+FVector(0,0,30),FQuat::Identity,ECC_Pawn,Capsule,Without))continue;
            Out={A,B,Hit,Cell};return true;
        }
    }
    return false;
}
FBox ERAlleyDestruction::WallBounds(FVector A,FVector B,float Floor)
{
    FBox Box(ForceInit);Box+=A;Box+=B;Box=Box.ExpandBy(FVector(110,110,0));Box.Min.Z=Floor-1;Box.Max.Z=Floor+250;return Box;
}
int32 ERAlleyDestruction::Cut(AERProceduralMap* Map,int32 Panel,const FBox& Bounds)
{
    const auto& World=Map->GetCellWorld();if(!World.GetPanels().IsValidIndex(Panel))return 0;
    const auto& P=World.GetPanels()[Panel];const auto Local=Bounds.TransformBy(P.Transform.Inverse());
    FIntVector Lo,Hi;for(int32 A=0;A<3;++A){Lo[A]=FMath::Clamp(FMath::FloorToInt(Local.Min[A]/P.Cell[A]),0,P.Dims[A]-1);Hi[A]=FMath::Clamp(FMath::FloorToInt(Local.Max[A]/P.Cell[A]),0,P.Dims[A]-1);}
    TMap<FERCellId,float> Damage;
    for(int32 Z=Lo.Z;Z<=Hi.Z;++Z)for(int32 Y=Lo.Y;Y<=Hi.Y;++Y)for(int32 X=Lo.X;X<=Hi.X;++X)
    {const FERCellId Id{Panel,X,Y,Z};if(World.IsAlive(Id) && Bounds.IsInsideOrOn(World.GetCellCenter(Id)))Damage.Add(Id,FLT_MAX);}
    return Map->ApplyCellBatch(Damage).Destroyed;
}
int32 ERAlleyDestruction::SeverSupports(AERProceduralMap* Map,int32 Panel)
{
    // Cut the lower supports AND lateral contacts to neighbouring buildings.
    // A building still connected to a neighbour correctly remains supported.
    const auto& W=Map->GetCellWorld();
    const FString Role=W.GetPanels()[Panel].SkinName.ToString();
    if(Role.StartsWith(TEXT("AlleyModule_")) && (Role.EndsWith(TEXT("_Roof")) || Role.EndsWith(TEXT("_Floor"))))
    {
        const FName Structure(*Role.LeftChop(Role.EndsWith(TEXT("_Roof")) ? 5:6));
        const int32 Walls=W.GetPanels().IndexOfByPredicate([&](const auto& P){return P.SkinName==Structure;});
        if(Walls!=INDEX_NONE)Panel=Walls;
    }
    const auto& P=W.GetPanels()[Panel];TMap<FERCellId,float> Damage;
    for(int32 C=P.FirstChunk;C<P.FirstChunk+P.ChunksX*P.ChunksZ;++C)
    {
        const auto& K=W.GetChunks()[C];if(!K.AliveCount)continue;
        for(int32 Z=K.Z0;Z<K.Z0+K.SZ;++Z)
        {
            const double Height=(Z+.5)*P.Cell.Z;if(Height<80)continue;
            for(int32 Y=0;Y<P.Dims.Y;++Y)for(int32 X=K.X0;X<K.X0+K.SX;++X)
            {
                if(Height>160 && X>1 && X<P.Dims.X-2 && Y>1 && Y<P.Dims.Y-2)continue;
                const FERCellId Id{Panel,X,Y,Z};if(W.IsAlive(Id))Damage.Add(Id,FLT_MAX);
            }
        }
    }
    // The roof slab is its own panel spanning the whole footprint, so its edge
    // touches adjoining walls too. Its inner ring stays on the kept wall layers.
    const FName RoofName(*(P.SkinName.ToString()+TEXT("_Roof")));
    const int32 Roof=W.GetPanels().IndexOfByPredicate([&](const auto& D){return D.SkinName==RoofName;});
    if(Roof!=INDEX_NONE)
    {
        const auto& D=W.GetPanels()[Roof];
        for(int32 Z=0;Z<D.Dims.Z;++Z)for(int32 Y=0;Y<D.Dims.Y;++Y)for(int32 X=0;X<D.Dims.X;++X)
        {
            if(X>1 && X<D.Dims.X-2 && Y>1 && Y<D.Dims.Y-2)continue;
            const FERCellId Id{Roof,X,Y,Z};if(W.IsAlive(Id))Damage.Add(Id,FLT_MAX);
        }
    }
    return Map->ApplyCellBatch(Damage).Destroyed;
}
bool ERAlleyDestruction::FindRoof(AERProceduralMap* Map,FVector& Point,FERCellId& Cell,TOptional<EERBlockMaterial> Only)
{
    auto Q=Query(Map->GetWorld());
    for(const auto& H:Map->GetLayout().Houses)
    {
        const FTransform F(H.Frame.GetRotation(),H.Frame.GetLocation());const auto Size=H.Frame.GetScale3D();
        for(float X:{0.f,-.2f,.2f})for(float Y:{0.f,-.2f,.2f})
        {
            const auto P=F.TransformPosition({Size.X*X,Size.Y*Y,Size.Z+150});FHitResult Hit;
            if(!Map->GetWorld()->LineTraceSingleByChannel(Hit,P,P-FVector(0,0,Size.Z*.5+200),ECC_GameTraceChannel1,Q) || Hit.GetActor()!=Map || !Hit.GetComponent()->ComponentHasTag(H.Mesh ? H.Mesh->GetFName():H.StructureName) || Hit.ImpactNormal.Z<.85 || !Map->ResolveCell(Hit,Cell))continue;
            if(Only && Map->GetCellWorld().GetPanels()[Cell.Panel].Material!=*Only)continue;
            Point=Hit.ImpactPoint;
            if(Map->GetWorld()->OverlapBlockingTestByChannel(Point+FVector(0,0,98),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92.5),Q))continue;
            return true;
        }
    }
    return false;
}
