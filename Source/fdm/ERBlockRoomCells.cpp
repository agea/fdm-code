// 5cm cell mode of AERBlockRoom: logical cells per panel, one dynamic mesh
// (visible faces only) and one aggregate-box body per dirty chunk.
#include "ERBlockRoom.h"
#include "ERCellMesher.h"
#include "ERCellNetShard.h"
#include "ERPlayerController.h"
#include "ERProceduralMap.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Misc/Crc.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

using UE::Geometry::FDynamicMesh3;
using UE::Geometry::FIndex3i;

void AERBlockRoom::ClearCells()
{
    for (auto& Mesh : CellMeshes) if (Mesh)
    {
        RemoveInstanceComponent(Mesh); Mesh->DestroyComponent();
        // Free the deterministic, net-addressable name before a regeneration reuses it.
        Mesh->Rename(*MakeUniqueObjectName(this, UDynamicMeshComponent::StaticClass(), TEXT("DeadCells")).ToString(), nullptr, REN_DontCreateRedirectors | REN_NonTransactional);
    }
    CellMeshes.Reset(); CellComponentChunks.Reset(); ChunkQuads.Reset(); ChunkBoxes.Reset(); AppliedChunkRevisions.Reset();
}

void AERBlockRoom::BuildCellComponents()
{
    const auto& Chunks = CellWorld.GetChunks();
    CellMeshes.Reserve(Chunks.Num()); ChunkQuads.Init(0, Chunks.Num()); ChunkBoxes.Init(0, Chunks.Num());
    for (int32 Index = 0; Index < Chunks.Num(); ++Index)
    {
        const auto& Panel = CellWorld.GetPanels()[Chunks[Index].Panel];
        auto* Mesh = NewObject<UDynamicMeshComponent>(this, *FString::Printf(TEXT("Cells_P%d_C%d"), Panel.Id, Index));
        AddInstanceComponent(Mesh);
        Mesh->SetMobility(EComponentMobility::Movable);
        // Movable chunks become pawns' movement bases: corrections carry them by
        // name, identical on host and clients for the same layout.
        Mesh->SetNetAddressable();
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetRelativeTransform(Panel.Transform);
        // Mesh and collision are rebuilt together, explicitly, once per batch.
        Mesh->SetDeferredCollisionUpdatesEnabled(true, false);
        Mesh->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
        Mesh->CollisionType = CTF_UseSimpleAsComplex; Mesh->bEnableComplexCollision = false;
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        Mesh->SetCanEverAffectNavigation(false);
        if (auto* Surface = Panel.Surface ? Panel.Surface : MaterialCatalog->Find(Panel.Material)->Surface.Get()) Mesh->SetMaterial(0, Surface);
        Mesh->RegisterComponent();
        CellMeshes.Add(Mesh); CellComponentChunks.Add(Mesh, Index);
        RebuildCellChunk(Index);
    }
}

void AERBlockRoom::RebuildCellChunk(int32 Index)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERCell_RebuildChunk);
    auto* Component = CellMeshes.IsValidIndex(Index) ? CellMeshes[Index].Get() : nullptr;
    if (!Component) return;
    const auto& Panel = CellWorld.GetPanels()[CellWorld.GetChunks()[Index].Panel];
    double Start = FPlatformTime::Seconds();
    TArray<ERCellMesh::FQuad> Quads;
    ERCellMesh::BuildFaces(CellWorld, Index, Quads);
    FDynamicMesh3 Mesh;
    Mesh.EnableAttributes();
    auto* Normals = Mesh.Attributes()->PrimaryNormals();
    auto* UVs = Mesh.Attributes()->PrimaryUV();
    for (const auto& Q : Quads)
    {
        // Planar UVs in metres on the face plane; the PBR surfaces map by world position.
        // U is mirrored on negative faces so every tangent frame is right-handed:
        // mirrored UVs flip the auto-calculated shading normal of those faces.
        const int32 Axis = FMath::Abs(Q.Normal.X) > .5f ? 0 : FMath::Abs(Q.Normal.Y) > .5f ? 1 : 2;
        const int32 U = (Axis + 1) % 3, V = (Axis + 2) % 3;
        const float Mirror = Q.Normal[Axis] < 0 ? -1.f : 1.f;
        // Overlay elements belong to one parent vertex each: never share them across corners.
        int32 Vertex[4], UV[4], Normal[4];
        for (int32 I = 0; I < 4; ++I)
        {
            Vertex[I] = Mesh.AppendVertex(FVector3d(Q.Corner[I]));
            UV[I] = UVs->AppendElement(FVector2f(Mirror * Q.Corner[I][U], Q.Corner[I][V]) / 100);
            Normal[I] = Normals->AppendElement(Q.Normal);
        }
        const int32 A = Mesh.AppendTriangle(Vertex[0], Vertex[1], Vertex[2]), B = Mesh.AppendTriangle(Vertex[0], Vertex[2], Vertex[3]);
        ensure(A >= 0 && B >= 0);
        if (A >= 0) { Normals->SetTriangle(A, FIndex3i(Normal[0], Normal[1], Normal[2])); UVs->SetTriangle(A, FIndex3i(UV[0], UV[1], UV[2])); }
        if (B >= 0) { Normals->SetTriangle(B, FIndex3i(Normal[0], Normal[2], Normal[3])); UVs->SetTriangle(B, FIndex3i(UV[0], UV[2], UV[3])); }
    }
    Component->SetMesh(MoveTemp(Mesh));
    const double MeshEnd = FPlatformTime::Seconds();
    TArray<FBox> Boxes;
    ERCellMesh::BuildBoxes(CellWorld, Index, Boxes);
    FKAggregateGeom Geometry;
    for (const FBox& Box : Boxes)
    {
        FVector Size = Box.GetSize();
        for (int32 Axis = 0; Axis < 3; ++Axis) Size[Axis] = FMath::Max(Size[Axis], double(CellSettings.MinCollisionThickness));
        FKBoxElem& Element = Geometry.BoxElems.Emplace_GetRef(Size.X, Size.Y, Size.Z);
        Element.Center = Box.GetCenter();
    }
    Component->SetSimpleCollisionShapes(Geometry, true);
    if (Boxes.IsEmpty()) Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const double End = FPlatformTime::Seconds();
    RebuildProfile.MeshMs += (MeshEnd - Start) * 1000; RebuildProfile.CollisionMs += (End - MeshEnd) * 1000;
    RebuildProfile.Quads += Quads.Num(); RebuildProfile.Boxes += Boxes.Num(); ++RebuildProfile.Groups;
    ChunkQuads[Index] = Quads.Num(); ChunkBoxes[Index] = Boxes.Num();
}

bool AERBlockRoom::ResolveCell(const FHitResult& Hit, FERCellId& Id) const
{
    if (Hit.GetActor() != this) return false;
    const int32* Chunk = CellComponentChunks.Find(Hit.GetComponent());
    if (!Chunk || !CellWorld.GetChunks().IsValidIndex(*Chunk)) return false;
    const FTransform& Actor = GetActorTransform();
    return CellWorld.LocateCell(CellWorld.GetChunks()[*Chunk].Panel, Actor.InverseTransformPosition(Hit.ImpactPoint),
        Actor.InverseTransformVectorNoScale(Hit.ImpactNormal), Id);
}

bool AERBlockRoom::ApplyHitDamage(const FHitResult& Hit, float Amount)
{
    FERCellId Id;
    if (!ResolveCell(Hit, Id)) return false;
    TMap<FERCellId, float> Damage; Damage.Add(Id, Amount);
    return ApplyCellBatch(Damage).Damaged > 0;
}

FTransform AERBlockRoom::GetCellTransform(const FERCellId& Id) const
{
    if (!CellWorld.IsValid(Id)) return FTransform::Identity;
    const auto& P = CellWorld.GetPanels()[Id.Panel];
    const FBox Box = CellWorld.GetCellBox(Id);
    return FTransform(P.Transform.GetRotation(), P.Transform.TransformPosition(Box.GetCenter()), Box.GetSize() / 100) * GetActorTransform();
}

FString AERBlockRoom::DescribeCell(const FERCellId& Id) const
{
    if (!CellWorld.IsValid(Id)) return TEXT("none");
    const auto& P = CellWorld.GetPanels()[Id.Panel];
    return FString::Printf(TEXT("P%d:%d,%d,%d HP=%.1f material=%d"), P.Id, Id.X, Id.Y, Id.Z, CellWorld.GetHealth(Id), int32(P.Material));
}

FERBlockBatchResult AERBlockRoom::ApplyCellBatch(const TMap<FERCellId, float>& Damage)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERCell_ApplyBatch);
    FERBlockBatchResult Result;
    if (!CanMutateBlocks() || Damage.IsEmpty()) return Result;
    const double Start = FPlatformTime::Seconds();
    RebuildProfile = {}; UpdateProfile = {};
    const auto Batch = CellWorld.ApplyDamage(Damage);
    UpdateProfile.LogicalMs = (FPlatformTime::Seconds() - Start) * 1000;
    for (int32 Chunk : Batch.DirtyChunks) RebuildCellChunk(Chunk);
    UpdateProfile.MeshMs = RebuildProfile.MeshMs; UpdateProfile.CollisionMs = RebuildProfile.CollisionMs;
    UpdateProfile.Quads = RebuildProfile.Quads; UpdateProfile.Boxes = RebuildProfile.Boxes; UpdateProfile.Groups = RebuildProfile.Groups;
    Result.Damaged = Batch.Damaged; Result.Destroyed = Batch.Destroyed; Result.GroupsRebuilt = Batch.DirtyChunks.Num();
    LastGroupsRebuilt = Result.GroupsRebuilt;
    if (Batch.Damaged > 0) LastHit = DescribeCell(Batch.Last);
    PublishCellChanges(Batch.ChangedChunks);
    LastUpdateMs = float((FPlatformTime::Seconds() - Start) * 1000); UpdateProfile.TotalMs = LastUpdateMs;
    if (Batch.Destroyed > 0) { ++DamageRevision; RequestSupportCheck(); }
    return Result;
}

AERBlockRoom::FERCellShotStep AERBlockRoom::ShootCells(const FHitResult& Hit, const FVector& Cursor, const FVector& Ray, double Range,
    float Structural, const FERBlockShotSettings& Settings, float& Energy, int32 RunsLeft)
{
    FERCellShotStep Step; Step.Exit = Hit.ImpactPoint;
    FERCellId First;
    if (!ResolveCell(Hit, First) || !(Settings.Energy > 0)) return Step;
    const int32 Panel = First.Panel;
    const auto& P = CellWorld.GetPanels()[Panel];
    const auto* Material = MaterialCatalog->Find(P.Material);
    const FTransform& Actor = GetActorTransform();
    const double Scale = Actor.GetScale3D().GetAbs().GetMin();
    // Never restart behind the cursor: cells already crossed are not hit twice.
    FVector Start = Hit.ImpactPoint - Ray * .05;
    if (FVector::DotProduct(Start - Cursor, Ray) < 0) Start = Cursor;
    const double Length = FMath::Min(Range - FVector::DotProduct(Start - Cursor, Ray), 5000.);
    if (Length <= 0 || Scale < .0001) return Step;
    const bool bGrazing = !Hit.bStartPenetrating && FMath::Abs(FVector::DotProduct(Ray, Hit.ImpactNormal)) < FMath::Clamp(Settings.MinImpactCosine, .01f, 1.f);
    TMap<FERCellId, float> Damage;
    bool bInside = false, bStopped = false;
    double ExitT = 0;
    CellWorld.Traverse(Panel, Actor.InverseTransformPosition(Start), Actor.InverseTransformPosition(Start + Ray * Length), [&](const FIntVector& C, double T0, double T1)
    {
        ExitT = T1;
        if (!CellWorld.IsAlive(Panel, C.X, C.Y, C.Z)) { bInside = false; return true; }
        const bool bEnter = !bInside;
        if (bEnter && Step.Runs >= RunsLeft) { Step.bLimited = bStopped = true; ExitT = T0; return false; }
        if (bEnter) { bInside = true; ++Step.Runs; }
        // Residual energy at this cell's entry: never restart from full damage.
        Damage.FindOrAdd({Panel, C.X, C.Y, C.Z}) += Structural * Energy / Settings.Energy * Material->BulletDamageScale; ++Step.Cells;
        const double Cost = ((bEnter ? Material->EntryEnergyCost : 0) + (T1 - T0) * Length * Material->EnergyCostPerCm)*P.Strength;
        if (bGrazing || Cost >= Energy) { bStopped = true; ExitT = (T0 + T1) * .5; return false; }
        Energy = float(Energy - Cost);
        return true;
    });
    ApplyCellBatch(Damage);
    SpawnCellImpact(P.Material, Hit.ImpactPoint, Hit.ImpactNormal);
    Step.bStop = bStopped;
    Step.Exit = Start + Ray * (Length * ExitT);
    if (!bStopped)
    {
        // Thin sheets have thicker collision boxes than cells: resume past them.
        const double Inflate = FMath::Max(0., (CellSettings.MinCollisionThickness - P.Size.Y) * .5) * Scale;
        const FVector Depth = Actor.TransformVectorNoScale(P.Transform.GetRotation().GetAxisY());
        Step.Exit += Ray * FMath::Min(10., .1 + Inflate / FMath::Max(.2, FMath::Abs(FVector::DotProduct(Ray, Depth))));
        if (FVector::DotProduct(Step.Exit - Hit.ImpactPoint, Ray) < .5) Step.Exit = Hit.ImpactPoint + Ray * .5;
    }
    return Step;
}

void AERBlockRoom::CollectCellExplosion(const FVector& Origin, const FERBlockExplosionProfile& Profile, const FCollisionQueryParams& Query,
    TMap<FERCellId, float>& Out, FERBlockBatchResult& Result) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERCell_CollectExplosion);
    const FTransform& Actor = GetActorTransform();
    const double Scale = Actor.GetScale3D().GetAbs().GetMin();
    if (!MaterialCatalog || Scale < .0001 || !Profile.IsValid()) return;
    const double QueryStart = FPlatformTime::Seconds();
    const FVector Local = Actor.InverseTransformPosition(Origin);
    const double Radius = Profile.OuterRadius / Scale;
    const auto& Panels = CellWorld.GetPanels();
    const auto& Chunks = CellWorld.GetChunks();
    // Transmission is per reference length of intact material, applied per cm crossed.
    const double Reference = FMath::Max(1.f, MaterialCatalog->CellCalibration.TransmissionReferenceCm);
    double LogPerCm[7]; float Multiplier[7];
    for (int32 I = 0; I < 7; ++I)
    {
        const auto* M = MaterialCatalog->Find(EERBlockMaterial(I));
        LogPerCm[I] = M->ExplosionTransmission > 0 ? FMath::Loge(double(M->ExplosionTransmission)) / Reference : -1.e30;
        Multiplier[I] = M->ExplosionDamageScale;
    }
    TArray<int32> NearPanels, NearChunks;
    for (int32 I = 0; I < Panels.Num(); ++I)
        if (FMath::SphereAABBIntersection(FSphere(Local, Radius), Panels[I].Bounds)) NearPanels.Add(I);
    for (int32 PanelIndex : NearPanels)
        for (int32 I = 0; I < Panels[PanelIndex].ChunksX * Panels[PanelIndex].ChunksZ; ++I)
        {
            const int32 C = Panels[PanelIndex].FirstChunk + I;
            if (Chunks[C].AliveCount > 0 && FMath::SphereAABBIntersection(FSphere(Local, Radius), Chunks[C].Bounds)) NearChunks.Add(C);
        }
    Result.QueryMs += (FPlatformTime::Seconds() - QueryStart) * 1000;
    const double ExposureStart = FPlatformTime::Seconds();
    const double MinLogExposure = FMath::Loge(1.e-6);
    for (int32 ChunkIndex : NearChunks)
    {
        const auto& C = Chunks[ChunkIndex];
        const auto& P = Panels[C.Panel];
        // Non-destructible occlusion: nine samples on the chunk, not one trace per cell.
        const FVector Center = C.Bounds.GetCenter(), Extent = C.Bounds.GetExtent() * .8;
        int32 Clear = 0;
        for (int32 S = 0; S < 9; ++S)
        {
            const FVector Sample = S == 0 ? Center : Center + Extent * FVector(S & 1 ? 1 : -1, S & 2 ? 1 : -1, S & 4 ? 1 : -1);
            ++Result.CoverTraces;
            if (!GetWorld()->LineTraceTestByObjectType(Origin, Actor.TransformPosition(Sample), FCollisionObjectQueryParams(ECC_WorldStatic), Query)) ++Clear;
        }
        const FVector PanelOrigin = P.Transform.InverseTransformPosition(Local);
        for (int32 Z = 0; Z < C.SZ; ++Z)
            for (int32 Y = 0; Y < P.Dims.Y; ++Y)
                for (int32 X = 0; X < C.SX; ++X)
                {
                    const int32 Bit = FERCellWorld::CellIndex(C, X, Y, Z);
                    if (!((C.Alive[Bit >> 6] >> (Bit & 63)) & 1)) continue;
                    const FERCellId Id{C.Panel, C.X0 + X, Y, C.Z0 + Z};
                    // Distance to the cell centre, as the preset specifies.
                    const double Distance = FVector::Distance(CellWorld.GetCellCenter(Id), Local) * Scale;
                    if (Distance >= Profile.OuterRadius || P.bIndestructible) continue;
                    ++Result.Candidates;
                    if (Clear == 0) { ++Result.Occluded; continue; }
                    // Sample the face toward the origin, slightly inside this cell.
                    const FBox Box = CellWorld.GetCellBox(Id);
                    FVector Face;
                    for (int32 A = 0; A < 3; ++A)
                    {
                        const double Inset = (Box.Max[A] - Box.Min[A]) * .05;
                        Face[A] = FMath::Clamp(PanelOrigin[A], Box.Min[A] + Inset, Box.Max[A] - Inset);
                    }
                    const FVector Target = P.Transform.TransformPosition(Face);
                    const double Segment = FVector::Distance(Local, Target);
                    double LogExposure = 0, Crossed = 0;
                    bool bBlocked = false;
                    for (int32 Other : NearPanels)
                    {
                        if (!FMath::LineBoxIntersection(Panels[Other].Bounds, Local, Target, Target - Local)) continue;
                        const auto& OP = Panels[Other];
                        CellWorld.Traverse(Other, Local, Target, [&](const FIntVector& Cell, double T0, double T1)
                        {
                            if ((Other == Id.Panel && Cell == FIntVector(Id.X, Id.Y, Id.Z)) || !CellWorld.IsAlive(Other, Cell.X, Cell.Y, Cell.Z)) return true;
                            if (OP.bIndestructible) { bBlocked = true; return false; }
                            // Intact cells of the pre-event snapshot attenuate by path length.
                            const double Cm = (T1 - T0) * Segment * Scale;
                            Crossed += Cm; LogExposure += LogPerCm[uint8(OP.Material)] * Cm * OP.Strength;
                            if (Crossed > Profile.MaxShieldingCm || LogExposure < MinLogExposure) { bBlocked = true; return false; }
                            return true;
                        });
                        if (bBlocked) break;
                    }
                    if (bBlocked) { ++Result.Occluded; continue; }
                    const double Exposure = Clear / 9. * FMath::Exp(LogExposure);
                    if (Exposure < 1) ++Result.Attenuated;
                    const double CellDamage = ERBlockCombat::ExplosionCellDamage(Profile, Distance, Multiplier[uint8(P.Material)], Exposure);
                    if (CellDamage > 0) Out.FindOrAdd(Id) += float(FMath::Min(double(TNumericLimits<float>::Max()), CellDamage));
                }
    }
    Result.ExposureMs += (FPlatformTime::Seconds() - ExposureStart) * 1000;
}

void AERBlockRoom::GetCellComponents(TArray<UPrimitiveComponent*>& Out) const
{
    for (const auto& Mesh : CellMeshes) if (Mesh) Out.Add(Mesh);
}

bool AERBlockRoom::ValidateCells() const
{
    const auto& Chunks = CellWorld.GetChunks();
    if (CellMeshes.Num() != Chunks.Num() || CellComponentChunks.Num() != Chunks.Num()) return false;
    TArray<ERCellMesh::FQuad> Quads; TArray<FBox> Boxes;
    for (int32 I = 0; I < Chunks.Num(); ++I)
    {
        const auto* Mesh = CellMeshes[I].Get();
        const int32* Index = CellComponentChunks.Find(Mesh);
        if (!Mesh || !Mesh->IsRegistered() || !Index || *Index != I) return false;
        if (!Mesh->GetRelativeTransform().Equals(CellWorld.GetPanels()[Chunks[I].Panel].Transform, 1.e-3)) return false;
        ERCellMesh::BuildFaces(CellWorld, I, Quads); ERCellMesh::BuildBoxes(CellWorld, I, Boxes);
        int32 Triangles = 0;
        Mesh->ProcessMesh([&Triangles](const FDynamicMesh3& M) { Triangles = M.TriangleCount(); });
        if (Quads.Num() != ChunkQuads[I] || Boxes.Num() != ChunkBoxes[I] || Triangles != 2 * Quads.Num() ||
            Mesh->GetSimpleCollisionShapes().BoxElems.Num() != Boxes.Num()) return false;
        if (!Boxes.IsEmpty() && Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision && !Mesh->GetBodyInstance()->IsValidBodyInstance()) return false;
    }
    return true;
}

void AERBlockRoom::SpawnCellImpact(EERBlockMaterial Material, const FVector& Position, const FVector& Normal)
{
    if (HasAuthority() && GetNetMode() != NM_Standalone && !bInCosmeticMulticast)
    { MulticastCellImpact(Network.Epoch, uint8(Material), Position, Normal); return; }
    if (const auto* Profile = MaterialCatalog ? MaterialCatalog->Find(Material) : nullptr) SpawnMaterialImpact(*Profile, true, Position, Normal);
}
void AERBlockRoom::MulticastCellImpact_Implementation(uint32 Epoch, uint8 Material, FVector Position, FVector Normal)
{
    if (GetNetMode() == NM_DedicatedServer || Epoch != Network.Epoch || !IsMapReady() || Material > uint8(EERBlockMaterial::Glass)) return;
    bInCosmeticMulticast = true; SpawnCellImpact(EERBlockMaterial(Material), Position, Normal); bInCosmeticMulticast = false;
}

void AERBlockRoom::PublishCellGeneration()
{
    for (auto& S : AuthorityShards) if (S.IsValid()) S->Destroy();
    AuthorityShards.Empty();
    const uint32 Epoch = Network.Epoch + 1;
    Network = {}; Network.Epoch = Epoch ? Epoch : 1; Network.Protocol = 8; Network.Cells = true; Network.CellSettings = CellSettings;
    Network.Blocks = Config; Network.BlockCount = int32(CellWorld.GetCellCount()); Network.LayoutDigest = GetLayoutDigest(); Network.Sequence = 1;
    if (auto* Map = Cast<AERProceduralMap>(this)) { Network.Procedural = true; Network.Layout = Map->LayoutConfig; Network.Ceilings = Map->bCeilings; }
    // Untouched chunks never travel: shards start empty and only carry modified masks.
    const int32 Count = FMath::DivideAndRoundUp(CellWorld.GetChunks().Num(), AERCellNetShard::Capacity);
    Network.PageVersions.Init(1, Count); AuthorityShards.SetNum(Count);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        auto* S = GetWorld()->SpawnActor<AERCellNetShard>(); check(S);
        S->Room = this; S->Header.Epoch = Network.Epoch; S->Header.Index = Index; S->Header.Version = 1;
        AuthorityShards[Index] = S; S->ForceNetUpdate();
    }
    ForceNetUpdate();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It) if (auto* PC = Cast<AERPlayerController>(It->Get())) PC->HoldForBlockMap(this, true);
    UE_LOG(LogTemp, Display, TEXT("ER_BLOCK_NET_GENERATION epoch=%u cells=%d shards=%d layout=%u"), Network.Epoch, Network.BlockCount, Count, Network.LayoutDigest);
}

void AERBlockRoom::PublishCellChanges(const TArray<int32>& Changed)
{
    if (!CanMutateBlocks() || GetNetMode() == NM_Standalone || Network.Epoch == 0 || !Network.Cells || Changed.IsEmpty()) return;
    TSet<int32> Dirty;
    for (int32 Chunk : Changed)
    {
        const int32 Index = Chunk / AERCellNetShard::Capacity;
        auto* S = AuthorityShards.IsValidIndex(Index) ? AuthorityShards[Index].Get() : nullptr;
        if (!S) continue;
        TArray<uint8> Mask; CellWorld.EncodeChunk(Chunk, Mask);
        if (S->SetChunk(Chunk, CellWorld.GetChunks()[Chunk].Revision, MoveTemp(Mask))) Dirty.Add(Index);
    }
    for (int32 Index : Dirty)
    {
        auto* S = AuthorityShards[Index].Get();
        ++S->Header.Version; Network.PageVersions[Index] = S->Header.Version; S->ForceNetUpdate();
    }
    if (!Dirty.IsEmpty()) { ++Network.Sequence; ForceNetUpdate(); }
}

bool AERBlockRoom::OnRepCellDescriptor()
{
    auto* Map = Cast<AERProceduralMap>(this);
    if (Network.Protocol != 8 || !Network.Cells || Network.BlockCount <= 0 || Network.BlockCount > 8000000 || Network.Procedural != (Map != nullptr))
    { bNetworkError = true; UE_LOG(LogTemp, Error, TEXT("ER_BLOCK_NET_INCOMPATIBLE descriptor")); return false; }
    Config = Network.Blocks; CellSettings = Network.CellSettings;
    if (Map) { Map->LayoutConfig = Network.Layout; Map->bCeilings = Network.Ceilings; }
    bApplyingNetwork = true; const bool bBuilt = Regenerate(); bApplyingNetwork = false; SetMapCollision(false);
    if (!bBuilt || CellWorld.GetCellCount() != Network.BlockCount || GetLayoutDigest() != Network.LayoutDigest ||
        Network.PageVersions.Num() != FMath::DivideAndRoundUp(CellWorld.GetChunks().Num(), AERCellNetShard::Capacity))
    { bNetworkError = true; UE_LOG(LogTemp, Error, TEXT("ER_BLOCK_NET_INCOMPATIBLE layout/assets epoch=%u"), Network.Epoch); return false; }
    BuiltEpoch = Network.Epoch; AppliedSequence = 0;
    AppliedPageVersions.Init(0, Network.PageVersions.Num()); AppliedChunkRevisions.Init(0, CellWorld.GetChunks().Num());
    // Shards can arrive before their descriptor or their Room reference.
    ReceivedShards.Empty();
    for (TActorIterator<AERCellNetShard> S(GetWorld()); S; ++S) if (S->Room == this && S->Header.Epoch == BuiltEpoch) ReceivedShards.Add(S->Header.Index, *S);
    return true;
}

void AERBlockRoom::ReceiveCellShard(AERCellNetShard* Shard)
{
    if (HasAuthority() || !Shard || Shard->Room != this || Shard->Header.Epoch != Network.Epoch || !Network.PageVersions.IsValidIndex(Shard->Header.Index)) return;
    ReceivedShards.Add(Shard->Header.Index, Shard); TryApplyCellNetwork();
}

void AERBlockRoom::TryApplyCellNetwork()
{
    if (HasAuthority() || bNetworkError || BuiltEpoch == 0 || BuiltEpoch != Network.Epoch || AppliedSequence == Network.Sequence) return;
    // The entire manifest must match before logical state or collision changes.
    for (int32 I = 0; I < Network.PageVersions.Num(); ++I)
    {
        const auto* Weak = ReceivedShards.Find(I); const auto* S = Weak ? Weak->Get() : nullptr;
        if (!S || S->Header.Epoch != BuiltEpoch || S->Header.Version != Network.PageVersions[I] || !S->IsPayloadCurrent()) return;
    }
    TArray<TPair<int32, TArray<uint64>>> Pending;
    for (int32 I = 0; I < Network.PageVersions.Num(); ++I)
    {
        if (AppliedPageVersions[I] == Network.PageVersions[I]) continue;
        for (const auto& Item : ReceivedShards[I]->Chunks.Items)
        {
            if (!CellWorld.GetChunks().IsValidIndex(Item.Chunk) || Item.Chunk / AERCellNetShard::Capacity != I)
            { bNetworkError = true; SetMapCollision(false); UE_LOG(LogTemp, Error, TEXT("ER_BLOCK_NET_INCOMPATIBLE state")); return; }
            if (AppliedChunkRevisions[Item.Chunk] == Item.Revision) continue;
            auto& Entry = Pending.Emplace_GetRef(Item.Chunk, TArray<uint64>());
            if (!CellWorld.DecodeChunk(Item.Chunk, Item.Mask, Entry.Value))
            { bNetworkError = true; SetMapCollision(false); UE_LOG(LogTemp, Error, TEXT("ER_BLOCK_NET_INCOMPATIBLE state")); return; }
        }
    }
    TArray<int32> Dirty;
    for (const auto& Entry : Pending) CellWorld.SetChunkAlive(Entry.Key, Entry.Value, Dirty);
    for (const auto& Entry : Pending) for (const auto& Item : ReceivedShards[Entry.Key / AERCellNetShard::Capacity]->Chunks.Items)
        if (Item.Chunk == Entry.Key) AppliedChunkRevisions[Entry.Key] = Item.Revision;
    RebuildProfile = {};
    for (int32 Chunk : Dirty) RebuildCellChunk(Chunk);
    for (int32 I = 0; I < Network.PageVersions.Num(); ++I) AppliedPageVersions[I] = Network.PageVersions[I];
    AppliedSequence = Network.Sequence;
    if (!bMapReady) { bMapReady = true; SetMapCollision(true); }
    // Empty chunks keep no query collision after the gate opens.
    for (int32 I = 0; I < CellMeshes.Num(); ++I) if (CellMeshes[I] && ChunkBoxes[I] == 0) CellMeshes[I]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UE_LOG(LogTemp, Verbose, TEXT("ER_BLOCK_NET_APPLIED epoch=%u sequence=%u removed_cells=%lld"), BuiltEpoch, AppliedSequence, CellWorld.GetRemovedCount());
}
