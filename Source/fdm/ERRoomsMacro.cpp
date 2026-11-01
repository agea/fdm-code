#include "ERRoomsMacro.h"
#include "ERProceduralMap.h"
#include "Algo/Sort.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformProcess.h"
#include "Metadata/PCGMetadata.h"
#include "PCGComponent.h"
#include "PCGData.h"
#include "PCGDataAsset.h"
#include "PCGGraph.h"
#include "Data/PCGPointData.h"
#include "Subsystems/PCGSubsystem.h"
#include "Async/TaskGraphInterfaces.h"
#include "Components/BoxComponent.h"
#include "Data/PCGBasePointData.h"

namespace
{
const FName StableId(TEXT("StableId")),BakeKey(TEXT("BakeKey")),ZoneType(TEXT("ZoneType")),TeamSide(TEXT("TeamSide")),Elevation(TEXT("Elevation"));
// PCGEx cluster contract 0.79.2: both packed attributes hold two 32-bit halves.
const FName EdgeEndpoints(TEXT("PCGEx/EData")),VertexIds(TEXT("PCGEx/VData"));
FIntPoint Pair(int32 A,int32 B){return {FMath::Min(A,B),FMath::Max(A,B)};}
}

int32 ERRoomsMacro::Mirror(int32 Slot,int32 N)
{
    if(Slot==AlphaBase || Slot==OmegaBase)return 1-Slot;
    if(Slot==LeftMid || Slot==RightMid)return LeftMid+RightMid-Slot;
    if(Slot==CentreLeft || Slot==CentreRight)return CentreLeft+CentreRight-Slot;
    return Slot<RightRow ? RightRow+(N-1-(Slot-LeftRow)):LeftRow+(N-1-(Slot-RightRow));
}
int32 ERRoomsMacro::LaneMiddle(bool Left,int32 N)
{
    if(N%2==0)return Left ? LeftMid:RightMid;
    return (Left ? LeftRow:RightRow)+N/2;
}
bool ERRoomsMacro::IsRealizable(FIntPoint Link,int32 N)
{
    const FIntPoint P=Pair(Link.X,Link.Y);
    if(P==FIntPoint(CentreLeft,CentreRight))return true;
    for(int32 Base:{AlphaBase,OmegaBase})for(int32 Centre:{CentreLeft,CentreRight})if(P==Pair(Base,Centre))return true;
    return P==Pair(CentreLeft,LaneMiddle(true,N)) || P==Pair(CentreRight,LaneMiddle(false,N));
}
float ERRoomsMacro::CentreOffset(float LaneX){return LaneX*.4f;}
TArray<const FERRoomsMacroVariant*> ERRoomsMacro::Candidates(const UERRoomsMacroTable* Table,int32 N,int32 Seed)
{
    TArray<const FERRoomsMacroVariant*> Out;if(!Table)return Out;
    for(const auto& V:Table->Variants)if(V.LaneRooms==N)Out.Add(&V);
    if(Out.Num()<2)return Out;
    // Rotate, so each seed still ends on the same fallback order on every peer.
    const int32 First=int32(HashCombine(uint32(Seed),0x4d41434fu)%uint32(Out.Num()));
    TArray<const FERRoomsMacroVariant*> Turned;
    for(int32 I=0;I<Out.Num();++I)Turned.Add(Out[(First+I)%Out.Num()]);
    return Turned;
}

UPCGDataAsset* UERRoomsMacroBake::FindOrCreateInput(const FString& PackageName)
{
#if WITH_EDITOR
    if(!PackageName.StartsWith(TEXT("/Game/EclipseRecoil/Art/MapTemplates/RoomsPCG/")) || !FPackageName::IsValidLongPackageName(PackageName))return nullptr;
    const FString Name=FPackageName::GetLongPackageAssetName(PackageName);
    if(FPackageName::DoesPackageExist(PackageName))return LoadObject<UPCGDataAsset>(nullptr,*(PackageName+TEXT(".")+Name));
    auto* Package=CreatePackage(*PackageName);
    auto* Asset=NewObject<UPCGDataAsset>(Package,*Name,RF_Public|RF_Standalone);
    FAssetRegistryModule::AssetCreated(Asset);Package->MarkPackageDirty();
    return Asset;
#else
    return nullptr;
#endif
}

bool UERRoomsMacroBake::FillInput(UERMapTemplate* T,UPCGDataAsset* Asset,int32 Samples)
{
    if(!T || !Asset || T->Archetype!=EERMapArchetype::Rooms || Samples<1)return false;
    Asset->Modify();Asset->Data.TaggedData.Reset();
    for(int32 N=FMath::Max(2,T->Rooms.MinLaneRooms);N<=T->Rooms.MaxLaneRooms;++N)for(int32 Sample=0;Sample<Samples;++Sample)
    {
        // A real plan of this size: slots come from its nodes, not from a sketch.
        FERProceduralLayout L;FERProceduralConfig C;C.TemplateAsset=T;bool Found=false;
        for(int32 Seed=Sample*131+N;Seed<Sample*131+N+4096 && !Found;++Seed)
        {C.Seed=Seed;Found=ERProcedural::Build(C,FERBlockRoomConfig(),L) && L.Rooms.Num()==2+2*N;}
        if(!Found)continue;
        struct FSlot{int32 Id;FVector Position;int32 Zone;int32 Team;float Height;};
        TArray<FSlot> Slots;
        auto Add=[&](int32 Id,int32 Node,int32 Zone,int32 Team){const FVector P=L.RoomNodes[Node].Center;Slots.Add({Id,FVector(P.X,P.Y,0),Zone,Team,float(P.Z)});};
        Add(ERRoomsMacro::AlphaBase,0,0,0);Add(ERRoomsMacro::OmegaBase,1,0,1);
        for(int32 Row=0;Row<N;++Row){Add(ERRoomsMacro::LeftRow+Row,2+Row,1,-1);Add(ERRoomsMacro::RightRow+Row,2+N+Row,1,-1);}
        if(N%2==0){Add(ERRoomsMacro::LeftMid,2+2*N,2,-1);Add(ERRoomsMacro::RightMid,3+2*N,2,-1);}
        const float Lane=FMath::Abs(L.RoomNodes[ERRoomsMacro::LeftRow-10+2].Center.X);
        const float Centre=ERRoomsMacro::CentreOffset(Lane);
        Slots.Add({ERRoomsMacro::CentreLeft,FVector(-Centre,0,0),3,-1,0});Slots.Add({ERRoomsMacro::CentreRight,FVector(Centre,0,0),3,-1,0});
        // Half-turn pairs jitter together, so a symmetric input gives a symmetric graph.
        FRandomStream Jitter(int32(HashCombine(uint32(N),uint32(Sample)*0x9e3779b9u)));
        TMap<int32,FVector> Offsets;Slots.Sort([](const FSlot& A,const FSlot& B){return A.Id<B.Id;});
        for(const auto& S:Slots)if(!Offsets.Contains(S.Id))
        {
            const FVector Offset(Jitter.FRandRange(-1.,1.)*Lane*.35,Jitter.FRandRange(-1.,1.)*Lane*.35,0);
            Offsets.Add(S.Id,Offset);Offsets.Add(ERRoomsMacro::Mirror(S.Id,N),-Offset);
        }
        auto* Data=NewObject<UPCGPointData>(Asset);
        auto* IdAttribute=Data->Metadata->CreateAttribute<int32>(StableId,0,false,true);
        auto* KeyAttribute=Data->Metadata->CreateAttribute<int32>(BakeKey,0,false,true);
        auto* ZoneAttribute=Data->Metadata->CreateAttribute<int32>(ZoneType,0,false,true);
        auto* TeamAttribute=Data->Metadata->CreateAttribute<int32>(TeamSide,-1,false,true);
        auto* HeightAttribute=Data->Metadata->CreateAttribute<float>(Elevation,0,false,true);
        auto& Points=Data->GetMutablePoints();
        for(const auto& S:Slots)
        {
            auto& Point=Points.AddDefaulted_GetRef();Point.Transform=FTransform(S.Position+Offsets[S.Id]);Point.Seed=int32(HashCombine(uint32(S.Id),uint32(Sample)));
            Point.MetadataEntry=Data->Metadata->AddEntry();
            IdAttribute->SetValue(Point.MetadataEntry,S.Id);KeyAttribute->SetValue(Point.MetadataEntry,N*1000+Sample);
            ZoneAttribute->SetValue(Point.MetadataEntry,S.Zone);TeamAttribute->SetValue(Point.MetadataEntry,S.Team);HeightAttribute->SetValue(Point.MetadataEntry,S.Height);
        }
        auto& Tagged=Asset->Data.TaggedData.AddDefaulted_GetRef();Tagged.Data=Data;Tagged.Pin=TEXT("Slots");Tagged.Tags.Add(FString::Printf(TEXT("Plan%d"),N*1000+Sample));
    }
    return !Asset->Data.TaggedData.IsEmpty();
}

int32 UERRoomsMacroBake::Bake(UPCGGraphInterface* Graph,UERRoomsMacroTable* Table)
{
    if(!Graph || !Table || !GEngine)return -1;
    UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(false);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);if(!World)return -1;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());World->BeginPlay();
    // PCG refuses a component without bounds; these only need to enclose the plan.
    auto* Actor=World->SpawnActor<AActor>();auto* Volume=NewObject<UBoxComponent>(Actor);
    Volume->SetBoxExtent(FVector(50000));Actor->SetRootComponent(Volume);Volume->RegisterComponent();
    auto* Component=NewObject<UPCGComponent>(Actor);
    Component->RegisterComponent();Component->SetGraph(Graph);Component->GenerateLocal(true);
    auto* Subsystem=UPCGSubsystem::GetInstance(World);
    // PCG advances over ticks: this bounded pump is why it stays out of ERProcedural::Build.
    const double Deadline=FPlatformTime::Seconds()+120;
    while(Component->IsGenerating() && FPlatformTime::Seconds()<Deadline)
    {
        FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
        if(Subsystem)Subsystem->Tick(.016f);
        FPlatformProcess::Sleep(.001f);
    }
    const bool Finished=!Component->IsGenerating();
    TMap<FString,const UPCGBasePointData*> Vertices,Edges;
    if(Finished)for(const auto& Tagged:Component->GetGeneratedGraphOutput().TaggedData)
    {
        const auto* Points=Cast<UPCGBasePointData>(Tagged.Data);if(!Points)continue;
        FString Cluster;bool IsEdges=false;
        for(const auto& Tag:Tagged.Tags)
        {
            if(Tag.StartsWith(TEXT("PCGEx/Cluster:")))Cluster=Tag;
            IsEdges=IsEdges || Tag==TEXT("PCGEx/Edges");
        }
        if(!Cluster.IsEmpty())(IsEdges ? Edges:Vertices).Add(Cluster,Points);
    }
    TMap<int32,TArray<FIntPoint>> Plans;bool Complete=Finished && !Vertices.IsEmpty();
    for(const auto& Cluster:Vertices)
    {
        const auto* Vtx=Cluster.Value;const auto* const* EdgeData=Edges.Find(Cluster.Key);
        const auto* Ids=Vtx->ConstMetadata()->GetConstTypedAttribute<int32>(StableId);
        const auto* Keys=Vtx->ConstMetadata()->GetConstTypedAttribute<int32>(BakeKey);
        const auto* Packed=Vtx->ConstMetadata()->GetConstTypedAttribute<int64>(VertexIds);
        if(!EdgeData || !Ids || !Keys || !Packed){Complete=false;continue;}
        const auto* Ends=(*EdgeData)->ConstMetadata()->GetConstTypedAttribute<int64>(EdgeEndpoints);
        if(!Ends){Complete=false;continue;}
        const auto VtxEntries=Vtx->GetConstMetadataEntryValueRange();
        TMap<uint32,int32> SlotOf;int32 Key=0;
        for(int32 I=0;I<VtxEntries.Num();++I)
        {
            SlotOf.Add(uint32(uint64(Packed->GetValueFromItemKey(VtxEntries[I]))>>32),Ids->GetValueFromItemKey(VtxEntries[I]));
            Key=Keys->GetValueFromItemKey(VtxEntries[I]);
        }
        auto& Links=Plans.FindOrAdd(Key);
        const auto EdgeEntries=(*EdgeData)->GetConstMetadataEntryValueRange();
        for(int32 I=0;I<EdgeEntries.Num();++I)
        {
            const uint64 Value=uint64(Ends->GetValueFromItemKey(EdgeEntries[I]));
            const int32* A=SlotOf.Find(uint32(Value>>32));const int32* B=SlotOf.Find(uint32(Value));
            if(!A || !B){Complete=false;continue;}
            Links.AddUnique(Pair(*A,*B));
        }
    }
    World->DestroyWorld(false);GEngine->DestroyWorldContext(World);
    if(!Complete)return -1;
    Table->Modify();Table->Variants.Reset();
    TArray<int32> Keys;Plans.GetKeys(Keys);Keys.Sort();
    for(int32 Key:Keys)
    {
        const int32 N=Key/1000;TArray<FIntPoint> Links;
        for(const auto& Link:Plans[Key])if(ERRoomsMacro::IsRealizable(Link,N))Links.Add(Link);
        Algo::Sort(Links,[](FIntPoint A,FIntPoint B){return A.X!=B.X ? A.X<B.X:A.Y<B.Y;});
        // The graph must keep half-turn pairing, and must reach both centre slots.
        const bool Paired=!Links.ContainsByPredicate([&](FIntPoint L){return !Links.Contains(Pair(ERRoomsMacro::Mirror(L.X,N),ERRoomsMacro::Mirror(L.Y,N)));});
        const bool Reached=Links.ContainsByPredicate([&](FIntPoint L){return L.X<=ERRoomsMacro::OmegaBase;});
        if(!Paired || !Reached || Table->Variants.ContainsByPredicate([&](const FERRoomsMacroVariant& V){return V.LaneRooms==N && V.Links==Links;}))continue;
        auto& Variant=Table->Variants.AddDefaulted_GetRef();Variant.LaneRooms=N;Variant.BakeSeed=Key%1000;Variant.Links=MoveTemp(Links);
    }
    return Table->Variants.Num();
}
