#include "ERMapTemplate.h"
#include "ERProceduralMap.h"
#include "Engine/StaticMesh.h"

namespace ERProcedural
{
bool BuildTemplate(const FERProceduralConfig& C,const FERBlockRoomConfig& B,FERProceduralLayout& Out,const FERCellSettings& Cells)
{
    const auto* T=C.TemplateAsset.Get();
    if(!T || T->TemplateId.IsEmpty() || T->Revision<1 || C.Floors!=1 || B.BlockSize!=FVector(50,20,25) ||
        !FMath::IsFinite(B.ChunkSize) || B.ChunkSize<100 || T->Pieces.Num()>256 || T->Pieces.IsEmpty() ||
        T->ArenaSize.ContainsNaN() || T->ArenaSize.X<2000 || T->ArenaSize.Y<3000 || T->ArenaSize.Z<400 ||
        T->ArenaSize.X>10000 || T->ArenaSize.Y>16000 || T->ArenaSize.Z>1600 || uint8(T->Symmetry)>1 ||
        T->AlphaSpawns.IsEmpty() || T->AlphaSpawns.Num()>32 || T->OmegaSpawns.Num()>32 ||
        T->PatrolRoute.Num()<2 || T->PatrolRoute.Num()>128) return false;
    const bool Symmetric=T->Symmetry==EERTemplateSymmetry::HalfTurn;
    auto ValidMesh=[](UStaticMesh* M){return !M || (!M->GetBoundingBox().GetSize().ContainsNaN() && M->GetBoundingBox().GetSize().GetMin()>.1);};
    if(!ValidMesh(T->ShellWallMesh) || !ValidMesh(T->RoofMesh))return false;
    FERProceduralLayout L;L.ArenaSize=T->ArenaSize;L.AlphaSpawns=T->AlphaSpawns;L.OmegaSpawns=T->OmegaSpawns;
    TArray<FERMapTemplatePiece> Pieces=T->Pieces;TArray<FVector> Patrol=T->PatrolRoute;
    if(T->Town.Enabled && (T->Variation.Enabled || !BuildDustTown(*T,C.Seed,Pieces,L,Patrol)))return false;
    if(T->Variation.Enabled && !BuildTemplateVariants(*T,C.Seed,Pieces,L,Patrol))return false;
    const FVector Size=L.ArenaSize,Half=Size*.5;
    L.Corridors.Add({FVector2D(-Half.X,-Half.Y),FVector2D(Half.X,Half.Y)});
    TArray<FBox> Volumes;TArray<FTransform> VolumeFrames;int32 Wall=0;
    for(const auto& Piece:Pieces)
    {
        if(Piece.Size.GetMin()<1)UE_LOG(LogTemp,Warning,TEXT("ER_TEMPLATE_PIECE_REJECT seed=%d size=%s"),C.Seed,*Piece.Size.ToString());
        if(Piece.Center.ContainsNaN() || Piece.Size.ContainsNaN() || Piece.Jitter.ContainsNaN() || Piece.MeshRotation.ContainsNaN() || !FMath::IsFinite(Piece.RotationYaw) ||
            Piece.Size.GetMin()<1 || Piece.Size.GetMax()>16000 || Piece.Jitter.X<0 || Piece.Jitter.Y<0 ||
            Piece.Jitter.X>200 || Piece.Jitter.Y>200 || uint8(Piece.Material)>6 || (Piece.Destructible && Piece.FixedMesh) || !ValidMesh(Piece.FixedMesh) ||
            (Piece.WalkableSurface && (Piece.Destructible || Piece.FixedMesh || Piece.VisualOnly)) ||
            (Piece.Destructible && (Piece.VisualOnly || Piece.FixedSurface || Piece.RotationYaw!=0))) return false;
        if(Symmetric && !Piece.Mirrored && (!FMath::IsNearlyZero(Piece.Center.X) || !FMath::IsNearlyZero(Piece.Center.Y) || !Piece.Jitter.IsNearlyZero()))return false;
        FRandomStream Random(int32(HashCombine(uint32(C.Seed),uint32(Piece.Group))));
        const float OffsetX=Random.RandRange(-int32(Piece.Jitter.X/50),int32(Piece.Jitter.X/50))*50;
        const float OffsetY=Random.RandRange(-int32(Piece.Jitter.Y/50),int32(Piece.Jitter.Y/50))*50;
        const FVector Original=Piece.Center+FVector(OffsetX,OffsetY,0);
        for(int32 Copy=0;Copy<(Symmetric && Piece.Mirrored ? 2:1);++Copy)
        {
            const FVector Center=Copy ? FVector(-Original.X,-Original.Y,Original.Z):Original;
            const FQuat PhysicalRotation=FRotator(0,Piece.RotationYaw+(Copy ? 180:0),0).Quaternion();
            const FBox Bounds=FBox(-Piece.Size*.5,Piece.Size*.5).TransformBy(FTransform(Piece.RotationYaw!=0 ? PhysicalRotation:FQuat::Identity,Center));
            const FVector Min=Bounds.Min,Max=Bounds.Max;
            if(Min.X < -Half.X || Max.X > Half.X || Min.Y < -Half.Y || Max.Y > Half.Y || Min.Z<-.001 || Max.Z>Size.Z+.001) return false;
            const FBox Box(Min,Max);
            const FTransform Frame(Piece.RotationYaw!=0 ? PhysicalRotation:FQuat::Identity,Center,Piece.Size);
            if(!Piece.VisualOnly)for(int32 VI=0;VI<Volumes.Num();++VI)if(Box.Intersect(Volumes[VI]) && Box.Overlap(Volumes[VI]).GetSize().GetMin()>.01)
            {
                const auto& Other=VolumeFrames[VI];bool Separated=false;
                // Exact planar SAT for the angled door planks. Their
                // conservative world AABBs overlap even when faces just touch.
                for(const FVector Axis:{Frame.GetRotation().GetAxisX(),Frame.GetRotation().GetAxisY(),Other.GetRotation().GetAxisX(),Other.GetRotation().GetAxisY()})
                {
                    auto Radius=[&](const FTransform& F){return .5*(F.GetScale3D().X*FMath::Abs(FVector::DotProduct(F.GetRotation().GetAxisX(),Axis))+F.GetScale3D().Y*FMath::Abs(FVector::DotProduct(F.GetRotation().GetAxisY(),Axis)));};
                    if(FMath::Abs(FVector::DotProduct(Center-Other.GetLocation(),Axis))>=Radius(Frame)+Radius(Other)-.01){Separated=true;break;}
                }
                if(!Separated){UE_LOG(LogTemp,Warning,TEXT("ER_TEMPLATE_REJECT seed=%d overlapping volume %s with %s"),C.Seed,*Box.ToString(),*Volumes[VI].ToString());return false;}
            }
            if(!Piece.VisualOnly){Volumes.Add(Box);VolumeFrames.Add(Frame);}
            if(!Piece.Destructible)
            {
                FRotator Rotation=Piece.MeshRotation;Rotation.Yaw+=Copy ? 180:0;
                // Legacy axis-aligned paired pieces keep their old physical
                // transform; only explicit town leaves rotate their boxes.
                const FQuat R=Piece.RotationYaw!=0 ? PhysicalRotation:FQuat::Identity;
                L.FixedPieces.Add({FTransform(R,Center,Piece.Size/100),Piece.Material,Piece.FixedMesh,Rotation,false,Piece.VisualOnly,Piece.FixedSurface});continue;
            }
            // Mirror the exact original volume: equal-cell division keeps paired cells symmetric.
            FBox Cell(Original-Piece.Size*.5,Original+Piece.Size*.5);
            if(Copy)Cell=FBox(FVector(-Cell.Max.X,-Cell.Max.Y,Cell.Min.Z),FVector(-Cell.Min.X,-Cell.Min.Y,Cell.Max.Z));
            // Slabs/steps are fixed support; upper parapets anchor to the base roof.
            AddPieceCells(Wall,Piece.Material,Cell,Piece.Anchored,Cells,L.Panels);
            ++Wall;
        }
    }
    // Fixed shed panels tile at <=5m to retain vendor detail and texel scale.
    auto Fixed=[&](FVector Center,FVector Size,FRotator Rotation=FRotator::ZeroRotator){L.FixedPieces.Add({FTransform(FQuat::Identity,Center,Size/100),EERBlockMaterial::Metal,T->ShellWallMesh,Rotation});};
    if(T->ShellWallMesh)
    {
        const int32 Rows=FMath::CeilToInt(Size.Z/525);
        for(int32 Row=0;Row<Rows;++Row)
        {
            const float H=Size.Z/Rows,Z=(Row+.5)*H;
            for(int32 Axis=0;Axis<2;++Axis)
            {
                const float Length=Axis ? Size.X:Size.Y;
                const int32 Count=FMath::CeilToInt(Length/500);const float Width=Length/Count;
                for(int32 I=0;I<Count;++I)for(float Sign:{-1.f,1.f})
                {
                    const float Along=(I-(Count-1)*.5)*Width;
                    Fixed(Axis ? FVector(Along,Sign*(Half.Y+10),Z):FVector(Sign*(Half.X+10),Along,Z),Axis ? FVector(Width,20,H):FVector(20,Width,H),FRotator(0,(T->ShellWallMesh->GetBoundingBox().GetSize().X<T->ShellWallMesh->GetBoundingBox().GetSize().Y) ? (Axis ? 90:0):(Axis ? 0:90),0));
                }
            }
        }
    }
    else
    {
        Fixed(FVector(-Half.X-10,0,Half.Z),FVector(20,Size.Y+40,Size.Z));
        Fixed(FVector(Half.X+10,0,Half.Z),FVector(20,Size.Y+40,Size.Z));
        for(float Sign:{-1.f,1.f})Fixed(FVector(0,Sign*(Half.Y+10),Half.Z),FVector(Size.X,20,Size.Z));
    }
    if(T->RoofMesh)
    {
        const int32 NX=FMath::CeilToInt(Size.X/500),NY=FMath::CeilToInt(Size.Y/500);
        // Tiles reach the outer faces of the 20cm shell walls, like the collision ceiling:
        // a roof that only touches the wall top along an edge leaks a sunlight line.
        const FVector Cell((Size.X+40)/NX,(Size.Y+40)/NY,20);
        for(int32 X=0;X<NX;++X)for(int32 Y=0;Y<NY;++Y)L.FixedPieces.Add({FTransform(FQuat::Identity,FVector((X-(NX-1)*.5)*Cell.X,(Y-(NY-1)*.5)*Cell.Y,Size.Z+10),Cell/100),EERBlockMaterial::Metal,T->RoofMesh,(T->RoofMesh->GetBoundingBox().GetSize().X<T->RoofMesh->GetBoundingBox().GetSize().Y ? FRotator(90,0,0):FRotator(0,0,90)),true,true});
    }
    auto Clear=[&](FVector P)
    {
        if(P.ContainsNaN() || FMath::Abs(P.X)>Half.X-40 || FMath::Abs(P.Y)>Half.Y-40 || P.Z<94 || P.Z>Size.Z-94)return false;
        for(const auto& Frame:VolumeFrames)
        {
            const FVector HalfSize=Frame.GetScale3D()*.5;
            if(FBox(-HalfSize,HalfSize).ExpandBy(FVector(35,35,93)).IsInsideOrOn(Frame.InverseTransformPositionNoScale(P)))return false;
        }
        return true;
    };
    if(Symmetric){L.OmegaSpawns.Reset();for(const auto& S:L.AlphaSpawns){auto P=S.GetLocation();P.X=-P.X;P.Y=-P.Y;L.OmegaSpawns.Add(FTransform(FRotator(0,S.Rotator().Yaw+180,0),P));}}
    if(L.OmegaSpawns.IsEmpty())return false;
    for(const auto* Starts:{&L.AlphaSpawns,&L.OmegaSpawns})for(const auto& Start:*Starts)if(Start.ContainsNaN() || !Clear(Start.GetLocation()))
    {UE_LOG(LogTemp,Warning,TEXT("ER_TEMPLATE_SPAWN_REJECT seed=%d point=%s"),C.Seed,*Start.GetLocation().ToString());return false;}
    for(const auto& P:Patrol)if(!Clear(P)){UE_LOG(LogTemp,Warning,TEXT("ER_TEMPLATE_PATROL_REJECT seed=%d point=%s"),C.Seed,*P.ToString());return false;}
    // Validate complete circulation/room paths, not just their endpoints.
    auto ClearPath=[&](const TArray<FVector>& Path,bool Closed)
    {
        for(int32 I=0;I<(Closed ? Path.Num():Path.Num()-1);++I)
        {
            const FVector A=Path[I],B=Path[(I+1)%Path.Num()],Delta=B-A;
            // Test in each solid's unscaled local frame. World AABBs would
            // incorrectly seal the diagonal slot between opposite-swing leaves.
            for(const auto& Frame:VolumeFrames)
            {
                const FVector HalfSize=Frame.GetScale3D()*.5;
                const FBox Box=FBox(-HalfSize,HalfSize).ExpandBy(FVector(35,35,93));
                const FVector LocalA=Frame.InverseTransformPositionNoScale(A),LocalB=Frame.InverseTransformPositionNoScale(B);
                if(FMath::LineBoxIntersection(Box,LocalA,LocalB,LocalB-LocalA))
                {UE_LOG(LogTemp,Warning,TEXT("ER_TEMPLATE_ROUTE_REJECT seed=%d from=%s to=%s volume=%s"),C.Seed,*A.ToString(),*B.ToString(),*Frame.GetLocation().ToString());return false;}
            }
        }
        return true;
    };
    if(!ClearPath(Patrol,true))return false;
    for(const auto& Route:L.RoomAccessRoutes)if(!ClearPath(Route,false))return false;
    L.Spawn=L.AlphaSpawns[0].GetLocation();L.Routes.Add({L.Spawn,Patrol[0]});L.Routes.Add(Patrol);L.Routes.Add({L.OmegaSpawns[0].GetLocation(),Patrol.Last()});
    if(L.Panels.IsEmpty() || L.FixedPieces.Num()>2048)return false;
    Out=MoveTemp(L);return true;
}
}

bool UERMapTemplate::ValidateLayout(int32 Seed) const
{
    FERProceduralConfig C;C.Seed=Seed;C.TemplateAsset=const_cast<UERMapTemplate*>(this);
    FERBlockRoomConfig B;FERProceduralLayout L;return ERProcedural::Build(C,B,L);
}
