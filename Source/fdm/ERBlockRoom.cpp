#include "ERBlockRoom.h"
#include "ERBlockDebris.h"
#include "ERBlockNetPage.h"
#include "ERProceduralMap.h"
#include "ERPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Misc/Crc.h"
#include "TimerManager.h"
#include "ERGrenadeExplosion.h"
#include "Components/DecalComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMemory.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProfilingDebugging/ResourceSize.h"
#include "UObject/ConstructorHelpers.h"

AERBlockRoom::AERBlockRoom()
{
    bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(true);SetNetUpdateFrequency(20);
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    BlockWorld = CreateDefaultSubobject<UERBlockWorld>(TEXT("BlockWorld"));
    MaterialCatalog = CreateDefaultSubobject<UERBlockMaterialCatalog>(TEXT("DefaultMaterials"));
    Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IndestructibleFloor"));
    Floor->SetupAttachment(RootComponent);
    Floor->SetCollisionProfileName(TEXT("BlockAll"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    BlockMesh = Cube.Object;
    Floor->SetStaticMesh(Cube.Object);
}
bool AERBlockRoom::IsStandaloneLab() const
{
    return CanMutateBlocks() && GetNetMode()==NM_Standalone;
}
void AERBlockRoom::BeginPlay()
{
    Super::BeginPlay();
    bNetworkBegunPlay=true;
    if (HasAuthority()) Regenerate();
    else {SetMapCollision(false);OnRep_Network();}
}
bool AERBlockRoom::Regenerate()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_Regenerate);
    const double Start = FPlatformTime::Seconds();
    if ((!CanMutateBlocks() && !bApplyingNetwork) || !BlockMesh || !GenerateLogicalWorld()) return false;
    ClearCollapse();
    bGraphValid=HasAuthority() && Collapse.Enabled && SupportGraph.Build(BlockWorld->GetBlocks(),Collapse.GroundAnchors,Collapse.GroundZ);
    if (HasAuthority() && Collapse.Enabled && !bGraphValid) UE_LOG(LogTemp,Warning,TEXT("ER_BLOCK_SUPPORT_UNAVAILABLE unsupported geometry or graph budget"));
    RebuildProfile={}; GenerationProfile={};
    GenerationProfile.LogicalMs=(FPlatformTime::Seconds()-Start)*1000;
    const double GroupStart=FPlatformTime::Seconds();
    for (auto& Group : Groups)
    {
        RemoveInstanceComponent(Group.Mesh);
        Group.Mesh->DestroyComponent();
    }
    Groups.Reset(); Instances.Reset(); ComponentGroups.Reset(); CachedChunks=0;
    Instances.Reserve(BlockWorld->GetBlocks().Num());
    LastHit = TEXT("none"); LastCombat = TEXT("none"); LastUpdateMs = 0; LastGroupsRebuilt = 0;
    ClearOwnedEffects();
    TMap<FERBlockGroupKey,int32> GroupLookup;
    TSet<FIntVector> Chunks;
    ConfigureFloors();
    for (const auto& Block : BlockWorld->GetBlocks())
    {
        const FERBlockGroupKey Key{Block.Chunk,Block.Material};
        const int32* Existing=GroupLookup.Find(Key);
        int32 Index=Existing ? *Existing : INDEX_NONE;
        if (Index == INDEX_NONE)
        {
            Index = Groups.AddDefaulted(); GroupLookup.Add(Key,Index); Chunks.Add(Block.Chunk);
            auto& Group = Groups[Index];
            Group.Chunk = Block.Chunk; Group.Material = Block.Material;
            Group.Mesh = NewObject<UInstancedStaticMeshComponent>(this);
            AddInstanceComponent(Group.Mesh);
            Group.Mesh->SetMobility(EComponentMobility::Movable);
            Group.Mesh->SetupAttachment(RootComponent);
            Group.Mesh->SetStaticMesh(BlockMesh);
            Group.Mesh->SetCollisionProfileName(TEXT("BlockAll"));
            Group.Mesh->SetCanEverAffectNavigation(false);
            if (auto* Surface = MaterialCatalog->Find(Block.Material)->Surface.Get()) Group.Mesh->SetMaterial(0, Surface);
            Group.Mesh->RegisterComponent(); ComponentGroups.Add(Group.Mesh,Index);
        }
        Groups[Index].Members.Add(Block.Id);
    }
    CachedChunks=Chunks.Num();
    GenerationProfile.GroupingMs=(FPlatformTime::Seconds()-GroupStart)*1000;
    for (int32 Index = 0; Index < Groups.Num(); ++Index) RebuildGroup(Index);
    GenerationProfile.ClearMs=RebuildProfile.ClearMs; GenerationProfile.AddMs=RebuildProfile.AddMs; GenerationProfile.MappingMs=RebuildProfile.MappingMs; GenerationProfile.Groups=RebuildProfile.Groups; GenerationProfile.Uploaded=RebuildProfile.Uploaded;
    GenerationMs = float((FPlatformTime::Seconds() - Start) * 1000);
    GenerationProfile.TotalMs=GenerationMs;
    if (HasAuthority() && GetNetMode()!=NM_Standalone) PublishNetworkGeneration();
    UE_LOG(LogTemp, Display, TEXT("ER_BLOCK_GENERATED %s"), *GetStatistics());
    return true;
}
bool AERBlockRoom::GenerateLogicalWorld() { return BlockWorld->GenerateRoom(Config, MaterialCatalog); }
void AERBlockRoom::ConfigureFloors()
{
    Floor->SetRelativeLocation(FVector(0, 0, -10));
    Floor->SetRelativeScale3D(FVector(Config.InteriorSize + 2 * Config.BlockSize.Y + 800, Config.InteriorSize + 2 * Config.BlockSize.Y + 800, 20) / 100);
    if (FloorSurface) Floor->SetMaterial(0, FloorSurface);
}
FString AERBlockRoom::GetLayoutStatistics() const { return FString::Printf(TEXT("seed=%d rooms=1 openings=0"), Config.Seed); }
void AERBlockRoom::RebuildGroup(int32 Index)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_RebuildGroup);
    auto& Group = Groups[Index];
    ++RebuildProfile.Groups;
    double MappingStart=FPlatformTime::Seconds();
    for (const auto& Id : Group.InstanceIds) Instances.Remove(Id);
    RebuildProfile.MappingMs+=(FPlatformTime::Seconds()-MappingStart)*1000;
    const double ClearStart=FPlatformTime::Seconds();
    Group.Mesh->ClearInstances();
    RebuildProfile.ClearMs+=(FPlatformTime::Seconds()-ClearStart)*1000;
    Group.InstanceIds.Reset();
    // Prepare stable survivor order once, then bulk upload. Clear/AddInstances
    // release/recreate collision synchronously; indices come from Unreal.
    TArray<FTransform> Transforms;
    Transforms.Reserve(Group.Members.Num()); Group.InstanceIds.Reserve(Group.Members.Num());
    MappingStart=FPlatformTime::Seconds();
    for (const auto& Id:Group.Members)
    {
        const auto* Block=BlockWorld->Find(Id);
        if (!Block || !Block->IsAttached()) continue;
        Transforms.Add(Block->LocalTransform); Group.InstanceIds.Add(Id);
    }
    RebuildProfile.MappingMs+=(FPlatformTime::Seconds()-MappingStart)*1000;
    const double AddStart=FPlatformTime::Seconds();
    const TArray<FPrimitiveInstanceId> Handles=Group.Mesh->AddInstancesById(Transforms,false,false);
    RebuildProfile.AddMs+=(FPlatformTime::Seconds()-AddStart)*1000; RebuildProfile.Uploaded+=Transforms.Num();
    check(Handles.Num()==Group.InstanceIds.Num());
    MappingStart=FPlatformTime::Seconds();
    for (int32 I=0;I<Handles.Num();++I)
    {
        const int32 Instance=Group.Mesh->GetInstanceIndexForId(Handles[I]);
        check(Instance==I);
        Instances.Add(Group.InstanceIds[I],{Index,Instance,Handles[I]});
    }
    RebuildProfile.MappingMs+=(FPlatformTime::Seconds()-MappingStart)*1000;
}
void AERBlockRoom::UpdateGroup(int32 Index)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_RemoveInstancesById);
    auto& Group=Groups[Index];
    const double PrepareStart=FPlatformTime::Seconds();
    TArray<FPrimitiveInstanceId> Removed;
    TArray<TPair<FERBlockId,FPrimitiveInstanceId>> Survivors;
    Survivors.Reserve(Group.InstanceIds.Num());
    for (const auto& Id:Group.InstanceIds)
    {
        const auto* Location=Instances.Find(Id); const auto* Block=BlockWorld->Find(Id);
        // Generated runtime ISMs never receive Editor/HISM edits. Recover via
        // a full rebuild if an external manipulation invalidated native IDs.
        if (!Location || !Block || !Group.Mesh->IsValidId(Location->Handle))
        {
            UE_LOG(LogTemp,Warning,TEXT("ER_BLOCK_HANDLE_RECOVERY group=%d"),Index);
            RebuildGroup(Index);return;
        }
        if (!Block->IsAttached()) Removed.Add(Location->Handle);
        else Survivors.Emplace(Id,Location->Handle);
    }
    RebuildProfile.MappingMs+=(FPlatformTime::Seconds()-PrepareStart)*1000;
    const double RemoveStart=FPlatformTime::Seconds();
    if (Survivors.IsEmpty()) Group.Mesh->ClearInstances();
    else Group.Mesh->RemoveInstancesById(Removed,false);
    RebuildProfile.ClearMs+=(FPlatformTime::Seconds()-RemoveStart)*1000; ++RebuildProfile.Groups;
    const double MappingStart=FPlatformTime::Seconds();
    for (const auto& Id:Group.InstanceIds) Instances.Remove(Id);
    Group.InstanceIds.SetNum(Survivors.Num());
    for (const auto& Pair:Survivors)
    {
        const int32 Instance=Group.Mesh->GetInstanceIndexForId(Pair.Value);
        check(Group.InstanceIds.IsValidIndex(Instance));
        Group.InstanceIds[Instance]=Pair.Key;
        Instances.Add(Pair.Key,{Index,Instance,Pair.Value});
    }
    RebuildProfile.MappingMs+=(FPlatformTime::Seconds()-MappingStart)*1000;
}
bool AERBlockRoom::ResolveHit(const FHitResult& Hit, FERBlockId& Id) const
{
    if (Hit.GetActor() != this || Hit.Item < 0) return false;
    const int32* Index=ComponentGroups.Find(Hit.GetComponent());
    if (!Index || !Groups.IsValidIndex(*Index)) return false;
    const auto& Group=Groups[*Index];
    if (!Group.InstanceIds.IsValidIndex(Hit.Item)) return false;
    Id=Group.InstanceIds[Hit.Item];
    const auto* Block=BlockWorld->Find(Id);
    return Block && Block->IsAttached();
}
bool AERBlockRoom::ApplyBlockDamage(const FERBlockId& Id, float Amount, bool bDestroy)
{
    return ApplyDamageBatch({{Id, Amount, bDestroy}}).Damaged > 0;
}
FERBlockBatchResult AERBlockRoom::ApplyDamageBatch(const TArray<FERBlockDamageRequest>& Requests)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_ApplyBatch);
    FERBlockBatchResult Result;
    if (!CanMutateBlocks()) return Result;
    const double Start = FPlatformTime::Seconds();
    RebuildProfile={}; UpdateProfile={};
    TMap<FERBlockId, FERBlockDamageRequest> Aggregated;
    Aggregated.Reserve(FMath::Min(Requests.Num(),Instances.Num()));
    for (const auto& Request : Requests)
    {
        if (!Request.bDestroy && (!FMath::IsFinite(Request.Damage) || Request.Damage <= 0)) continue;
        auto& Combined = Aggregated.FindOrAdd(Request.Id);
        Combined.Id = Request.Id; Combined.bDestroy |= Request.bDestroy;
        if (FMath::IsFinite(Request.Damage) && Request.Damage > 0)
            Combined.Damage = float(FMath::Min(double(TNumericLimits<float>::Max()), double(Combined.Damage) + Request.Damage));
    }
    TArray<FERBlockId> Order; Aggregated.GetKeys(Order);
    Order.Sort([](const auto& A, const auto& B) { return A.Wall != B.Wall ? A.Wall < B.Wall : A.Row != B.Row ? A.Row < B.Row : A.Column < B.Column; });
    TSet<int32> Dirty;
    for (const auto& Id : Order)
    {
        const auto* Location = Instances.Find(Id);
        if (!Location) continue;
        const int32 Group = Location->Group;
        const auto& Request = Aggregated[Id];
        if (!(Request.bDestroy ? BlockWorld->DestroyBlock(Id) : BlockWorld->Damage(Id, Request.Damage))) continue;
        ++Result.Damaged;
        const auto* Block = BlockWorld->Find(Id);
        if (Block->State == EERBlockState::Destroyed) { Dirty.Add(Group); ++Result.Destroyed; }
        LastHit = FString::Printf(TEXT("%s HP=%.0f material=%d"), *Id.ToString(), Block->Health, int32(Block->Material));
    }
    TArray<int32> GroupsToUpdate = Dirty.Array(); GroupsToUpdate.Sort();
    UpdateProfile.LogicalMs=(FPlatformTime::Seconds()-Start)*1000;
    for (int32 Group : GroupsToUpdate) UpdateGroup(Group);
    UpdateProfile.ClearMs=RebuildProfile.ClearMs; UpdateProfile.AddMs=RebuildProfile.AddMs; UpdateProfile.MappingMs=RebuildProfile.MappingMs; UpdateProfile.Groups=RebuildProfile.Groups; UpdateProfile.Uploaded=RebuildProfile.Uploaded;
    Result.GroupsRebuilt = GroupsToUpdate.Num(); LastGroupsRebuilt = Result.GroupsRebuilt;
    LastUpdateMs = float((FPlatformTime::Seconds() - Start) * 1000);
    UpdateProfile.TotalMs=LastUpdateMs;
    if (Result.Damaged>0) PublishNetworkChanges(Order);
    if (Result.Destroyed > 0) RequestSupportCheck();
    if (Result.Damaged > 0) UE_LOG(LogTemp, Verbose, TEXT("ER_BLOCK_HIT %s damaged=%d removed=%d rebuilt=%d %s"), *LastHit, Result.Damaged, Result.Destroyed, Result.GroupsRebuilt, *GetStatistics());
    return Result;
}
FTransform AERBlockRoom::GetBlockTransform(const FERBlockId& Id) const
{
    const auto* Block = BlockWorld->Find(Id);
    return Block ? Block->LocalTransform * GetActorTransform() : FTransform::Identity;
}
FERBlockBatchResult AERBlockRoom::ApplyWorldExplosion(UWorld* World, const FVector& Origin, float Radius, float Damage, AActor* IgnoredActor)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_Explosion);
    const double Start=FPlatformTime::Seconds();
    FERBlockBatchResult Result;
    if (!World || World->GetNetMode() == NM_Client || Origin.ContainsNaN() ||
        !FMath::IsFinite(Radius) || Radius <= 0 || Radius > 5000 || !FMath::IsFinite(Damage) || Damage <= 0) return Result;
    struct FPending { AERBlockRoom* Room; TArray<FERBlockDamageRequest> Damage; };
    TArray<FPending> Pending;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERBlockExplosion), false, IgnoredActor);
    // All rooms are sampled against the unchanged collision snapshot first.
    // Destruction of the cover during this blast never exposes another room.
    for (TActorIterator<AERBlockRoom> It(World); It; ++It)
    {
        auto* Room = *It;
        if (!Room->CanMutateBlocks() || !Room->MaterialCatalog) continue;
        const double Scale = Room->GetActorScale3D().GetAbs().GetMin();
        if (Scale < .0001) continue;
        auto& Event = Pending.AddDefaulted_GetRef(); Event.Room = Room;
        const double QueryStart=FPlatformTime::Seconds();
        const auto Candidates = Room->BlockWorld->QuerySphere(Room->GetActorTransform().InverseTransformPosition(Origin), float(Radius / Scale));
        Result.QueryMs+=(FPlatformTime::Seconds()-QueryStart)*1000;
        const double ExposureStart=FPlatformTime::Seconds();
        for (const auto& Id : Candidates)
        {
            const auto* Block = Room->BlockWorld->Find(Id);
            if (Block->bIndestructible) continue;
            const FTransform Transform = Room->GetBlockTransform(Id);
            const FVector Local = Transform.InverseTransformPosition(Origin);
            const FVector Closest = Transform.TransformPosition(FVector(FMath::Clamp(Local.X, -50., 50.), FMath::Clamp(Local.Y, -50., 50.), FMath::Clamp(Local.Z, -50., 50.)));
            const double Distance = FVector::Distance(Origin, Closest);
            if (Distance >= Radius) continue;
            ++Result.Candidates;
            FHitResult Cover; FERBlockId CoverId;
            // Sample the exposed face, not the centre deep in the wall: a ray
            // to the centre would meet adjacent coplanar bricks first. Keep the
            // other coordinates inside the face to avoid shared-edge ambiguity.
            FVector Sample(FMath::Clamp(Local.X, -49., 49.), FMath::Clamp(Local.Y, -49., 49.), FMath::Clamp(Local.Z, -49., 49.));
            const int32 Face = FMath::Abs(Local.Y) > 50 ? 1 : FMath::Abs(Local.X) > 50 ? 0 : FMath::Abs(Local.Z) > 50 ? 2 : INDEX_NONE;
            if (Face != INDEX_NONE) Sample[Face] = Local[Face] > 0 ? 49.9999 : -49.9999;
            ++Result.CoverTraces;
            const bool bCover = World->LineTraceSingleByObjectType(Cover, Origin, Transform.TransformPosition(Sample), FCollisionObjectQueryParams(ECC_WorldStatic), Query);
            if (bCover && !(Room->ResolveHit(Cover, CoverId) && CoverId == Id)) { ++Result.Occluded; continue; }
            const auto* Profile = Room->MaterialCatalog->Find(Block->Material);
            if (Profile) Event.Damage.Add({Id, float(FMath::Min(double(TNumericLimits<float>::Max()), Damage * (1 - Distance / Radius) * Profile->ExplosionDamageScale)), false});
        }
        Result.ExposureMs+=(FPlatformTime::Seconds()-ExposureStart)*1000;
    }
    const double MutationStart=FPlatformTime::Seconds();
    for (auto& Event : Pending)
    {
        const auto Batch = Event.Room->ApplyDamageBatch(Event.Damage);
        Result.Damaged += Batch.Damaged; Result.Destroyed += Batch.Destroyed; Result.GroupsRebuilt += Batch.GroupsRebuilt;
    }
    Result.MutationMs=(FPlatformTime::Seconds()-MutationStart)*1000;
    Result.TotalMs=(FPlatformTime::Seconds()-Start)*1000;
    for (auto& Event : Pending)
        Event.Room->LastCombat = FString::Printf(TEXT("blast candidates=%d covered=%d damaged=%d removed=%d rebuilt=%d"), Result.Candidates, Result.Occluded, Result.Damaged, Result.Destroyed, Result.GroupsRebuilt);
    UE_LOG(LogTemp, Display, TEXT("ER_BLOCK_EXPLOSION candidates=%d covered=%d damaged=%d removed=%d rebuilt=%d"), Result.Candidates, Result.Occluded, Result.Damaged, Result.Destroyed, Result.GroupsRebuilt);
    return Result;
}
bool AERBlockRoom::CanShowEffects(const FVector& Position) const
{
    if (Position.ContainsNaN() || !FMath::IsFinite(EffectDistance) || EffectDistance<0 || !FApp::CanEverRender() || GetNetMode() == NM_DedicatedServer) return false;
    const auto* Camera = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
    return Camera && FVector::DistSquared(Camera->GetCameraLocation(), Position) <= double(EffectDistance) * EffectDistance;
}
void AERBlockRoom::SpawnBlockImpact(const FERBlockId& Id, const FVector& Position, const FVector& Normal)
{
    if(HasAuthority() && GetNetMode()!=NM_Standalone && !bInCosmeticMulticast)
    {MulticastBlockImpact(Network.Epoch,Id,Position,Normal);return;}
    if (!CanShowEffects(Position) || MaxImpactEffects <= 0 || Normal.ContainsNaN() || Normal.IsNearlyZero()) return;
    const auto* Block = BlockWorld->Find(Id);
    const auto* Profile = Block && MaterialCatalog ? MaterialCatalog->Find(Block->Material) : nullptr;
    if (!Profile) return;
    const int32 Budget = FMath::Clamp(MaxImpactEffects, 0, 64);
    ImpactDecals.RemoveAll([](const auto& Item) { return !Item.IsValid(); });
    ImpactEmitters.RemoveAll([](const auto& Item) { return !Item.IsValid() || !Item->IsActive(); });
    if (Block->IsAttached() && Profile->ImpactDecal)
    {
        while (ImpactDecals.Num() >= Budget) { ImpactDecals[0]->DestroyComponent(); ImpactDecals.RemoveAt(0); }
        if (auto* Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), Profile->ImpactDecal, FVector(.4f, 1.5f, 1.5f), Position + Normal * .05f, Normal.Rotation(), 8))
        { ImpactDecals.Add(Decal); Decal->SetFadeOut(6, 2); }
    }
    if (Profile->ImpactSystem)
    {
        while (ImpactEmitters.Num() >= Budget) { if (ImpactEmitters[0].IsValid()) ImpactEmitters[0]->DestroyComponent(); ImpactEmitters.RemoveAt(0); }
        if (auto* Emitter = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Profile->ImpactSystem, Position, Normal.Rotation())) ImpactEmitters.Add(Emitter);
    }
}
void AERBlockRoom::SpawnExplosionEffect(const FVector& Origin)
{
    if (!ExplosionEffectClass || !CanShowEffects(Origin)) return;
    TArray<AERGrenadeExplosion*> Effects;
    for (TActorIterator<AERGrenadeExplosion> It(GetWorld()); It; ++It) Effects.Add(*It);
    if (Effects.Num() >= 8)
    {
        Effects.Sort([](const auto& A, const auto& B) { return A.GetGameTimeSinceCreation() > B.GetGameTimeSinceCreation(); });
        Effects[0]->Destroy();
    }
    OwnedBursts.RemoveAll([](const auto& Item) { return !Item.IsValid(); });
    if (auto* Burst=GetWorld()->SpawnActor<AERGrenadeExplosion>(ExplosionEffectClass,Origin,FRotator::ZeroRotator)) OwnedBursts.Add(Burst);
}
float AERBlockRoom::TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer)
{
    if (!Event.IsOfType(FPointDamageEvent::ClassID)) return 0;
    const auto& Point = static_cast<const FPointDamageEvent&>(Event);
    FERBlockId Id;
    if (!ResolveHit(Point.HitInfo, Id)) return 0;
    const float Previous = BlockWorld->Find(Id)->Health;
    return ApplyBlockDamage(Id, Amount) ? Previous - BlockWorld->Find(Id)->Health : 0;
}
bool AERBlockRoom::ValidateMappings() const
{
    int32 Count = 0;
    if (ComponentGroups.Num()!=Groups.Num()) return false;
    for (int32 Index = 0; Index < Groups.Num(); ++Index)
    {
        const auto& Group = Groups[Index];
        const int32* ComponentIndex=ComponentGroups.Find(Group.Mesh);
        if (!ComponentIndex || *ComponentIndex!=Index || !IsValid(Group.Mesh) || Group.Mesh->GetInstanceCount() != Group.InstanceIds.Num()) return false;
        for (int32 Instance = 0; Instance < Group.InstanceIds.Num(); ++Instance)
        {
            const auto& Id = Group.InstanceIds[Instance];
            const auto* Location = Instances.Find(Id);
            const auto* Block = BlockWorld->Find(Id);
            FTransform Transform;
            if (!Location || Location->Group != Index || Location->Instance != Instance || !Block ||
                !Block->IsAttached() || !Group.Mesh->GetInstanceTransform(Instance, Transform) ||
                !Group.Mesh->IsValidId(Location->Handle) || Group.Mesh->GetInstanceIndexForId(Location->Handle)!=Instance ||
                !Transform.Equals(Block->LocalTransform)) return false;
            ++Count;
        }
    }
    return Count == Instances.Num() && Count == BlockWorld->GetBlocks().Num() - BlockWorld->GetRemovedCount();
}
int32 AERBlockRoom::GetChunkCount() const
{
    return CachedChunks;
}
FString AERBlockRoom::GetStatistics() const
{
    int32 Debris=0,Physics=0;GetDebrisCounts(Debris,Physics);
    return FString::Printf(TEXT("%s blocks=%d alive=%d destroyed=%d detached=%d chunks=%d groups=%d instances=%d generation=%.2fms update=%.2fms groups_updated=%d debris=%d physics=%d support=%.2fms pending=%d"),
        *GetLayoutStatistics(), BlockWorld->GetBlocks().Num(), Instances.Num(), BlockWorld->GetDestroyedCount(),BlockWorld->GetDetachedCount(), GetChunkCount(), Groups.Num(), Instances.Num(), GenerationMs, LastUpdateMs, LastGroupsRebuilt,Debris,Physics,LastSupportMs,bSupportPending);
}

FERBlockMemoryProfile AERBlockRoom::GetMemoryProfile() const
{
    FERBlockMemoryProfile Result;
    Result.NetworkBytes=Network.PageVersions.GetAllocatedSize()+AuthorityPages.GetAllocatedSize()+ReceivedPages.GetAllocatedSize()+AppliedPageVersions.GetAllocatedSize();
    auto PageBytes=[](const AERBlockNetPage* P){return P ? P->State.Health.GetAllocatedSize()+P->State.States.GetAllocatedSize():SIZE_T(0);};
    if(HasAuthority()){for(const auto& P:AuthorityPages)Result.NetworkBytes+=PageBytes(P.Get());}
    else {for(const auto& P:ReceivedPages)Result.NetworkBytes+=PageBytes(P.Value.Get());}
    Result.SupportBytes=SupportGraph.GetAllocatedBytes()+OwnedDebris.GetAllocatedSize();
    GetDebrisCounts(Result.Debris,Result.SimulatingDebris);
    Result.LogicalBytes=BlockWorld->GetStorageBytes(); Result.LayoutBytes=GetLayoutBytes();
    Result.MappingBytes=Groups.GetAllocatedSize()+Instances.GetAllocatedSize()+ComponentGroups.GetAllocatedSize()+ImpactDecals.GetAllocatedSize()+ImpactEmitters.GetAllocatedSize()+OwnedBursts.GetAllocatedSize();
    for (const auto& Group:Groups) Result.MappingBytes+=Group.Members.GetAllocatedSize()+Group.InstanceIds.GetAllocatedSize();
    TInlineComponentArray<UInstancedStaticMeshComponent*> Meshes(this);
    for (auto* Mesh:Meshes)
    {
        FResourceSizeEx Size(EResourceSizeMode::Exclusive); Mesh->GetResourceSizeEx(Size);
        Result.InstanceResourceBytes+=Size.GetTotalMemoryBytes();
        for (auto* Body:Mesh->GetInstanceBodies()) if (Body && Body->IsValidBodyInstance()) ++Result.CollisionBodies;
    }
    Result.Components=GetInstanceComponents().Num();
    GetEffectCounts(Result.Decals,Result.Emitters,Result.Bursts);
    Result.ResidentBytes=FPlatformMemory::GetStats().UsedPhysical;
    return Result;
}
void AERBlockRoom::GetEffectCounts(int32& Decals,int32& Emitters,int32& Bursts) const
{
    Decals=Emitters=Bursts=0;
    for (const auto& Item:ImpactDecals) if (Item.IsValid()) ++Decals;
    for (const auto& Item:ImpactEmitters) if (Item.IsValid() && Item->IsActive()) ++Emitters;
    for (TActorIterator<AERGrenadeExplosion> It(GetWorld()); It; ++It) ++Bursts;
}
void AERBlockRoom::ClearOwnedEffects()
{
    for (auto& Decal:ImpactDecals) if (Decal.IsValid()) Decal->DestroyComponent();
    for (auto& Emitter:ImpactEmitters) if (Emitter.IsValid()) Emitter->DestroyComponent();
    for (auto& Burst:OwnedBursts) if (Burst.IsValid()) Burst->Destroy();
    ImpactDecals.Reset(); ImpactEmitters.Reset(); OwnedBursts.Reset();
}
void AERBlockRoom::EndPlay(const EEndPlayReason::Type Reason)
{
    for(auto& Page:AuthorityPages)if(Page.IsValid())Page->Destroy();
    AuthorityPages.Empty();ReceivedPages.Empty();
    ClearCollapse();
    ClearOwnedEffects();
    Super::EndPlay(Reason);
}

void AERBlockRoom::ClearCollapse()
{
    GetWorldTimerManager().ClearTimer(SupportTimer);
    for (auto& Item:OwnedDebris) if (Item.IsValid()) Item->Destroy();
    OwnedDebris.Empty();SupportGraph.Reset();bGraphValid=bSupportPending=bSupportRunning=false;
    DetachCursor=SupportSteps=LastUnsupported=LastUnsupportedComponents=LastSupportWork=LastDetached=LastSpawned=0;
    LastSupportMs=PeakSupportMs=0;
}
void AERBlockRoom::RequestSupportCheck()
{
    if (!CanMutateBlocks() || !Collapse.Enabled || !bGraphValid) return;
    bSupportPending=true;
    if (!GetWorldTimerManager().IsTimerActive(SupportTimer))
    {
        FTimerManagerTimerParameters Parameters;Parameters.bLoop=true;Parameters.bMaxOncePerFrame=true;
        GetWorldTimerManager().SetTimer(SupportTimer,this,&AERBlockRoom::StepSupport,.05f,Parameters);
    }
}
void AERBlockRoom::GetDebrisCounts(int32& Alive,int32& Simulating) const
{
    Alive=Simulating=0;
    for (const auto& Item:OwnedDebris) if (Item.IsValid()) {++Alive;if (Item->IsSimulating()) ++Simulating;}
}
void AERBlockRoom::SpawnDebris(const FERBlockRecord& Block)
{
    if(GetNetMode()==NM_DedicatedServer)return;
    const FTransform T=GetBlockTransform(Block.Id);
    auto* Debris=GetWorld()->SpawnActorDeferred<AERBlockDebris>(AERBlockDebris::StaticClass(),T,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Debris) return;
    const auto* Profile=MaterialCatalog->Find(Block.Material);
    Debris->Initialize(BlockMesh,Profile ? Profile->Surface.Get() : nullptr,Block.Id,T,Collapse.DebrisLifetime,Collapse.DebrisSettleAfter);
    Debris->FinishSpawning(Debris->GetActorTransform());OwnedDebris.Add(Debris);++LastSpawned;
}
void AERBlockRoom::StepSupport()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ERBlock_CollapseStep);
    if (!bSupportPending || !CanMutateBlocks() || !Collapse.Enabled || !bGraphValid)
    { bSupportPending=false;GetWorldTimerManager().ClearTimer(SupportTimer);return; }
    const double Begin=FPlatformTime::Seconds();++SupportSteps;
    LastSupportWork=LastDetached=LastSpawned=0;
    if (!bSupportRunning)
    {
        SupportGraph.Start();SupportRevision=BlockWorld->GetDamageRevision();DetachCursor=0;bSupportRunning=true;
    }
    if (!SupportGraph.IsReady())
    {
        SupportGraph.Step(BlockWorld->GetBlocks(),Collapse.WorkPerStep,Collapse.GraphTimeMs);
        LastSupportWork=SupportGraph.LastWork;
        if (SupportGraph.IsReady())
        {
            LastUnsupported=SupportGraph.GetUnsupported().Num();LastUnsupportedComponents=SupportGraph.GetComponentCount();
            UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_SUPPORT unsupported=%d components=%d revision=%llu"),LastUnsupported,LastUnsupportedComponents,SupportRevision);
        }
    }
    if (SupportGraph.IsReady())
    {
        OwnedDebris.RemoveAll([](const auto& Item){return !Item.IsValid();});
        int32 GlobalDebris=0;
        for (TActorIterator<AERBlockDebris> It(GetWorld());It;++It) ++GlobalDebris;
        const int32 GlobalLimit=FMath::Clamp(Collapse.MaxDebris,0,64);
        const auto& Unsupported=SupportGraph.GetUnsupported();
        TSet<int32> Dirty;
        TArray<int32> Selected;
        TArray<FERBlockId> DetachedIds;
        const int32 Limit=FMath::Clamp(Collapse.RemovalsPerStep,1,256);
        RebuildProfile={};
        for (int32 Processed=0;DetachCursor<Unsupported.Num() && Processed<Limit;++Processed)
        {
            const int32 I=Unsupported[DetachCursor++];const auto& Block=BlockWorld->GetBlocks()[I];
            const auto* Location=Instances.Find(Block.Id);
            if (!Location) continue;
            const int32 Group=Location->Group;
            if (!BlockWorld->DetachBlock(Block.Id)) continue;
            Dirty.Add(Group);++LastDetached;DetachedIds.Add(Block.Id);
            if (GlobalDebris<GlobalLimit && Selected.Num()<FMath::Clamp(Collapse.DebrisPerStep,0,8))
            { Selected.Add(I);++GlobalDebris; }
        }
        TArray<int32> Order=Dirty.Array();Order.Sort();
        for (int32 Group:Order) UpdateGroup(Group);
        // Remove static collision for the entire batch before waking Chaos.
        for (int32 I:Selected) SpawnDebris(BlockWorld->GetBlocks()[I]);
        PublishNetworkChanges(DetachedIds);
        if (DetachCursor==Unsupported.Num())
        {
            bSupportRunning=false;
            bSupportPending=BlockWorld->GetDamageRevision()!=SupportRevision;
            if (!bSupportPending) GetWorldTimerManager().ClearTimer(SupportTimer);
        }
    }
    LastSupportMs=(FPlatformTime::Seconds()-Begin)*1000;
    PeakSupportMs=FMath::Max(PeakSupportMs,LastSupportMs);
}

bool AERBlockRoom::CanMutateBlocks() const {return GetWorld() && HasAuthority() && GetNetMode()!=NM_Client;}
void AERBlockRoom::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AERBlockRoom,Network);
}
uint32 AERBlockRoom::GetLayoutDigest() const
{
    uint32 H=6;
    if(const auto* Map=Cast<AERProceduralMap>(this))if(Map->LayoutConfig.TemplateAsset) H=HashCombine(H,Map->GetTemplateGeometryDigest());
    auto Mix=[&](int32 V){H=HashCombine(H,::GetTypeHash(V));};
    Mix(BlockWorld->GetBlocks().Num());
    for(const auto& B:BlockWorld->GetBlocks())
    {
        Mix(B.Id.Wall);Mix(B.Id.Column);Mix(B.Id.Row);Mix(uint8(B.Material));Mix(B.bIndestructible);Mix(B.bStructuralAnchor);
        const auto& T=B.LocalTransform;
        for(FVector V:{T.GetLocation(),T.GetScale3D()*100,T.GetRotation().GetAxisX(),T.GetRotation().GetAxisY(),T.GetRotation().GetAxisZ()})
            for(int32 A=0;A<3;++A)Mix(FMath::RoundToInt(V[A]*100));
    }
    for(int32 I=0;I<7;++I)if(const auto* P=MaterialCatalog->Find(static_cast<EERBlockMaterial>(I)))
    {
        for(float V:{P->MaxHealth,P->BulletDamageScale,P->EntryEnergyCost,P->EnergyCostPerCm,P->ExplosionDamageScale})
            {uint32 Bits;FMemory::Memcpy(&Bits,&V,4);H=HashCombine(H,Bits);}
        H=HashCombine(H,FCrc::StrCrc32(P->Surface ? *P->Surface->GetPathName():TEXT("")));
    }
    H=HashCombine(H,FCrc::StrCrc32(BlockMesh ? *BlockMesh->GetPathName():TEXT("")));
    if(BlockMesh && BlockMesh->GetBodySetup())H=HashCombine(H,GetTypeHash(BlockMesh->GetBodySetup()->BodySetupGuid));
    return H;
}
uint32 AERBlockRoom::GetStateDigest() const
{
    uint32 H=GetLayoutDigest();
    for(const auto& B:BlockWorld->GetBlocks())
    {uint32 Bits;FMemory::Memcpy(&Bits,&B.Health,4);H=HashCombine(H,Bits);H=HashCombine(H,uint8(B.State));}
    return H;
}
void AERBlockRoom::PublishNetworkGeneration()
{
    for(auto& P:AuthorityPages)if(P.IsValid())P->Destroy();AuthorityPages.Empty();
    const uint32 Epoch=Network.Epoch+1;
    Network={};Network.Epoch=Epoch ? Epoch:1;Network.Blocks=Config;Network.BlockCount=BlockWorld->GetBlocks().Num();
    Network.LayoutDigest=GetLayoutDigest();Network.Sequence=1;
    if(auto* Map=Cast<AERProceduralMap>(this)){Network.Procedural=true;Network.Layout=Map->LayoutConfig;Network.Ceilings=Map->bCeilings;}
    const int32 Count=FMath::DivideAndRoundUp(Network.BlockCount,AERBlockNetPage::Capacity);
    Network.PageVersions.Init(1,Count);AuthorityPages.SetNum(Count);
    for(int32 Index=0;Index<Count;++Index)
    {
        auto* P=GetWorld()->SpawnActor<AERBlockNetPage>();check(P);AuthorityPages[Index]=P;P->Room=this;
        P->State.Epoch=Network.Epoch;P->State.Index=Index;P->State.Version=1;
        const int32 End=FMath::Min((Index+1)*AERBlockNetPage::Capacity,Network.BlockCount);
        for(int32 I=Index*AERBlockNetPage::Capacity;I<End;++I)
        {const auto& B=BlockWorld->GetBlocks()[I];P->State.Health.Add(B.Health);P->State.States.Add(uint8(B.State));}
        P->ForceNetUpdate();
    }
    ForceNetUpdate();
    for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)if(auto* PC=Cast<AERPlayerController>(It->Get()))PC->HoldForBlockMap(this,true);
    UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_GENERATION epoch=%u blocks=%d pages=%d layout=%u"),Network.Epoch,Network.BlockCount,Count,Network.LayoutDigest);
}
void AERBlockRoom::PublishNetworkChanges(const TArray<FERBlockId>& Ids)
{
    if(!CanMutateBlocks() || GetNetMode()==NM_Standalone || Network.Epoch==0)return;
    TSet<int32> Dirty;
    for(const auto& Id:Ids){const int32 I=BlockWorld->GetBlockIndex(Id);if(I!=INDEX_NONE)Dirty.Add(I/AERBlockNetPage::Capacity);}
    bool Changed=false;
    for(int32 Index:Dirty)
    {
        auto* P=AuthorityPages.IsValidIndex(Index) ? AuthorityPages[Index].Get():nullptr;if(!P)continue;
        bool PageChanged=false;
        for(int32 Offset=0;Offset<P->State.Health.Num();++Offset)
        {
            const auto& B=BlockWorld->GetBlocks()[Index*AERBlockNetPage::Capacity+Offset];
            if(P->State.Health[Offset]!=B.Health || P->State.States[Offset]!=uint8(B.State))PageChanged=true;
            P->State.Health[Offset]=B.Health;P->State.States[Offset]=uint8(B.State);
        }
        if(PageChanged)
        {++P->State.Version;Network.PageVersions[Index]=P->State.Version;P->ForceNetUpdate();Changed=true;}
    }
    if(Changed){++Network.Sequence;ForceNetUpdate();}
}
void AERBlockRoom::SetMapCollision(bool Enabled)
{
    TInlineComponentArray<UPrimitiveComponent*> Components(this);
    for(auto* C:Components)
    {C->SetCollisionEnabled(Enabled && !C->ComponentHasTag(TEXT("ERTemplateVisual")) ? ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);C->SetVisibility(Enabled);}
}
void AERBlockRoom::OnRep_Network()
{
    if(HasAuthority() || !bNetworkBegunPlay || Network.Epoch==0)return;
    if(BuiltEpoch!=Network.Epoch)
    {
        bMapReady=false;bNetworkError=false;SetMapCollision(false);
        if(Network.Protocol!=6 || Network.BlockCount<=0 || Network.BlockCount>50000 || Network.PageVersions.Num()!=FMath::DivideAndRoundUp(Network.BlockCount,AERBlockNetPage::Capacity))
        {bNetworkError=true;UE_LOG(LogTemp,Error,TEXT("ER_BLOCK_NET_INCOMPATIBLE descriptor"));return;}
        Config=Network.Blocks;
        auto* Map=Cast<AERProceduralMap>(this);
        if(Network.Procedural!=(Map!=nullptr)) {bNetworkError=true;return;}
        if(Map){Map->LayoutConfig=Network.Layout;Map->bCeilings=Network.Ceilings;}
        bApplyingNetwork=true;const bool Built=Regenerate();bApplyingNetwork=false;SetMapCollision(false);
        if(!Built || BlockWorld->GetBlocks().Num()!=Network.BlockCount || GetLayoutDigest()!=Network.LayoutDigest)
        {bNetworkError=true;UE_LOG(LogTemp,Error,TEXT("ER_BLOCK_NET_INCOMPATIBLE layout/assets epoch=%u"),Network.Epoch);return;}
        BuiltEpoch=Network.Epoch;AppliedSequence=0;AppliedPageVersions.Init(0,Network.PageVersions.Num());
        // Page actors can arrive before their map descriptor or mapped Room ref.
        ReceivedPages.Empty();
        for(TActorIterator<AERBlockNetPage> P(GetWorld());P;++P)if(P->Room==this && P->State.Epoch==BuiltEpoch)ReceivedPages.Add(P->State.Index,*P);
    }
    TryApplyNetwork();
}
void AERBlockRoom::ReceiveNetworkPage(AERBlockNetPage* Page)
{
    if(HasAuthority() || !Page || Page->Room!=this || Page->State.Epoch!=Network.Epoch || Page->State.Index<0 || !Network.PageVersions.IsValidIndex(Page->State.Index))return;
    ReceivedPages.Add(Page->State.Index,Page);TryApplyNetwork();
}
void AERBlockRoom::TryApplyNetwork()
{
    if(HasAuthority() || bNetworkError || BuiltEpoch==0 || BuiltEpoch!=Network.Epoch || AppliedSequence==Network.Sequence)return;
    // Match the entire manifest before touching logical state or any collision.
    // Newer page snapshots wait for their corresponding newer manifest.
    for(int32 I=0;I<Network.PageVersions.Num();++I)
    {
        const auto* Weak=ReceivedPages.Find(I);auto* P=Weak ? Weak->Get():nullptr;
        if(!P || P->State.Epoch!=BuiltEpoch || P->State.Version!=Network.PageVersions[I])return;
        const int32 Expected=FMath::Min(AERBlockNetPage::Capacity,Network.BlockCount-I*AERBlockNetPage::Capacity);
        if(P->State.Health.Num()!=Expected || P->State.States.Num()!=Expected){bNetworkError=true;SetMapCollision(false);return;}
    }
    for(int32 I=0;I<Network.PageVersions.Num();++I)
    {
        if(AppliedPageVersions[I]==Network.PageVersions[I])continue;
        auto* P=ReceivedPages[I].Get();
        for(int32 J=0;J<P->State.Health.Num();++J)
            if(!BlockWorld->ValidateNetworkState(I*AERBlockNetPage::Capacity+J,P->State.States[J],P->State.Health[J],MaterialCatalog))
            {bNetworkError=true;SetMapCollision(false);UE_LOG(LogTemp,Error,TEXT("ER_BLOCK_NET_INCOMPATIBLE state"));return;}
    }
    TSet<int32> Dirty;TArray<int32> Cosmetic;
    for(int32 I=0;I<Network.PageVersions.Num();++I)
    {
        if(AppliedPageVersions[I]==Network.PageVersions[I])continue;
        auto* P=ReceivedPages[I].Get();
        for(int32 J=0;J<P->State.Health.Num();++J)
        {
            const int32 Index=I*AERBlockNetPage::Capacity+J;const auto& B=BlockWorld->GetBlocks()[Index];
            const bool WasAttached=B.IsAttached();
            const bool IsAttached=P->State.States[J]<=uint8(EERBlockState::Damaged);
            if(WasAttached && !IsAttached)
            {
                if(const auto* Location=Instances.Find(B.Id))Dirty.Add(Location->Group);
                if(bMapReady && P->State.States[J]==uint8(EERBlockState::Detached))Cosmetic.Add(Index);
            }
            BlockWorld->ApplyNetworkState(Index,P->State.States[J],P->State.Health[J]);
        }
        AppliedPageVersions[I]=Network.PageVersions[I];
    }
    RebuildProfile={};TArray<int32> Order=Dirty.Array();Order.Sort();for(int32 I:Order)UpdateGroup(I);
    OwnedDebris.RemoveAll([](const auto& Item){return !Item.IsValid();});
    int32 Global=0;for(TActorIterator<AERBlockDebris> D(GetWorld());D;++D)++Global;
    LastSpawned=0;
    for(int32 I:Cosmetic)
    {if(Global>=FMath::Clamp(Collapse.MaxDebris,0,64) || LastSpawned>=FMath::Clamp(Collapse.DebrisPerStep,0,8))break;SpawnDebris(BlockWorld->GetBlocks()[I]);++Global;}
    AppliedSequence=Network.Sequence;
    if(!bMapReady){bMapReady=true;SetMapCollision(true);}
    UE_LOG(LogTemp,Verbose,TEXT("ER_BLOCK_NET_APPLIED epoch=%u sequence=%u removed=%d"),BuiltEpoch,AppliedSequence,BlockWorld->GetRemovedCount());
}
void AERBlockRoom::MulticastBlockImpact_Implementation(uint32 Epoch,FERBlockId Id,FVector Position,FVector Normal)
{
    if(GetNetMode()==NM_DedicatedServer || Epoch!=Network.Epoch || !IsMapReady())return;
    bInCosmeticMulticast=true;SpawnBlockImpact(Id,Position,Normal);bInCosmeticMulticast=false;
}
