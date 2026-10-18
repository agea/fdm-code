#include "ERSmokeCloud.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "UObject/UnrealType.h"
#include "ERCharacter.h"
#include "ERMatch.h"
#include "ERBlockRoom.h"
#include "NiagaraDataInterfaceRenderTargetVolume.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Misc/App.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr int32 NX=16, NY=16, NZ=8;
constexpr int32 Count=NX*NY*NZ;
int32 Index(int32 X,int32 Y,int32 Z) { return X+NX*(Y+NY*Z); }
}

AERSmokeCloud::AERSmokeCloud()
{
    bReplicates=true; bAlwaysRelevant=true; SetNetUpdateFrequency(2);
    PrimaryActorTick.bCanEverTick=true;
    Fumes=CreateDefaultSubobject<UNiagaraComponent>(TEXT("Fumes"));
    SetRootComponent(Fumes);
    Fumes->SetAutoActivate(false);
    Fumes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Fumes->SetCastShadow(false);
    Volume=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Volume"));
    Volume->SetupAttachment(Fumes);
    Volume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Volume->SetCastShadow(false);
    Volume->bReceivesDecals=false;
    SimSize=FVector(800,800,400);
}

FLinearColor AERSmokeCloud::ColorForTeam(EERTeam Team)
{
    if(Team==EERTeam::Alpha) return FLinearColor(.08f,.26f,.4f,1.f);
    if(Team==EERTeam::Omega) return FLinearColor(.4f,.1f,.04f,1.f);
    return FLinearColor(.30f,.31f,.32f,1.f);
}

void AERSmokeCloud::OnRep_SmokeColor()
{
    if(VolumeInstance) VolumeInstance->SetVectorParameterValue(TEXT("SmokeColor"),SmokeColor);
}

void AERSmokeCloud::Prewarm(UWorld* World,TSubclassOf<AERSmokeCloud> CloudClass,const FVector& ViewPosition)
{
    if(!World || !CloudClass || !FApp::CanEverRender() || World->GetNetMode()==NM_DedicatedServer) return;
    // The fixed volume has no fluid simulation or global fog setting to warm.
    CloudClass->GetDefaultObject<AERSmokeCloud>()->Volume->PrecachePSOs();
}

double AERSmokeCloud::ServerNow() const
{
    const auto* State=GetWorld()->GetGameState<AERGameState>();
    return State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

void AERSmokeCloud::BeginPlay()
{
    Super::BeginPlay();
    if(HasAuthority())
    {
        StartedAtServerTime=ServerNow(); SetLifeSpan(Duration);
        ShapePhase=FVector(FMath::FRandRange(0.f,100.f),FMath::FRandRange(0.f,100.f),FMath::FRandRange(0.f,100.f));
    }
    if(GetNetMode()!=NM_DedicatedServer && ReleaseSound)
    {
        const float Age=FMath::Max(0.f,float(ServerNow()-StartedAtServerTime));
        if(Age<ReleaseSound->GetDuration())
            ReleaseAudio=UGameplayStatics::SpawnSoundAtLocation(this,ReleaseSound,GetActorLocation(),FRotator::ZeroRotator,
                .65f,1.f,Age,ReleaseAttenuation,ReleaseConcurrency,true);
        UE_LOG(LogTemp,Display,TEXT("ER_SMOKE_AUDIO sound=%s started=%d spatial=%d concurrency=%d age=%.2f"),
            *GetNameSafe(ReleaseSound),ReleaseAudio && ReleaseAudio->IsPlaying(),ReleaseAttenuation!=nullptr,ReleaseConcurrency!=nullptr,Age);
    }
    SetActorEnableCollision(false);
    Fumes->Deactivate(); Fumes->SetComponentTickEnabled(false);
    SimSize=SimSize.GetAbs().BoundToBox(FVector(200,200,100),FVector(800,800,400));
    VolumeOrigin=GetActorLocation();
    FHitResult Floor;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERSmokeFloor),false,this);
    if(GetWorld()->LineTraceSingleByObjectType(Floor,VolumeOrigin+FVector(0,0,10),VolumeOrigin-FVector(0,0,150),
        FCollisionObjectQueryParams(ECC_WorldStatic),Query) && Floor.ImpactNormal.Z>.5f)
        VolumeOrigin.Z=Floor.ImpactPoint.Z+1.f;
    Volume->SetStaticMesh(VolumeMesh);
    Volume->SetWorldLocation(VolumeOrigin+FVector(0,0,SimSize.Z*.5f));
    Volume->SetWorldScale3D(SimSize/100.f);
    Volume->SetVisibility(true);
    Volume->SetHiddenInGame(false);
    RebuildContainment();
    if(GetNetMode()!=NM_DedicatedServer && FApp::CanEverRender() && VolumeMaterial)
    {
        VolumeInstance=Volume->CreateDynamicMaterialInstance(0,VolumeMaterial);
        VolumeInstance->SetVectorParameterValue(TEXT("VolumeMin"),FLinearColor(VolumeOrigin-FVector(SimSize.X*.5f,SimSize.Y*.5f,0)));
        VolumeInstance->SetVectorParameterValue(TEXT("VolumeSize"),FLinearColor(SimSize));
        VolumeInstance->SetVectorParameterValue(TEXT("SmokeColor"),SmokeColor);
        UploadDensity();
    }
    else Volume->SetVisibility(false);
    UE_LOG(LogTemp,Display,TEXT("ER_SMOKE_CLOUD shape=hemisphere cells=%d grid=16x16x8 duration=%.1f replicated=%d collision=none"),FilledCellCount,Duration,GetIsReplicated());
}

void AERSmokeCloud::RebuildContainment()
{
    const FVector Cell=SimSize/FVector(NX,NY,NZ);
    const FVector Min=VolumeOrigin-FVector(SimSize.X*.5f,SimSize.Y*.5f,0);
    TArray<uint8> Free; Free.Init(0,Count); Occupancy.Init(0,Count);
    TArray<FVector> Centers; Centers.SetNum(Count);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERSmokeContainment),false,this);
    const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
    // Conservative whole-cell overlap prevents a cell spanning a thin wall.
    // Edge sweeps also catch walls falling precisely between neighbouring cells.
    const auto Shape=FCollisionShape::MakeBox(Cell*.48f);
    int32 Seed=INDEX_NONE; float Nearest=MAX_flt;
    const FVector Source=GetActorLocation()+FVector(0,0,Cell.Z*.5f);
    for(int32 Z=0;Z<NZ;++Z) for(int32 Y=0;Y<NY;++Y) for(int32 X=0;X<NX;++X)
    {
        const int32 I=Index(X,Y,Z);
        Centers[I]=Min+Cell*FVector(X+.5f,Y+.5f,Z+.5f);
        const FVector Local=(Centers[I]-VolumeOrigin)/FVector(SimSize.X*.5f,SimSize.Y*.5f,SimSize.Z);
        if(Local.SizeSquared()>1.f || GetWorld()->OverlapAnyTestByObjectType(Centers[I],FQuat::Identity,Objects,Shape,Query)) continue;
        Free[I]=1;
        const float Distance=FVector::DistSquared(Centers[I],Source);
        FHitResult Hit;
        if(Distance<Nearest && !GetWorld()->LineTraceSingleByObjectType(Hit,Source,Centers[I],Objects,Query))
        { Seed=I; Nearest=Distance; }
    }
    TArray<int32> Queue;
    // Never teleport smoke into a remote pocket when the source is enclosed.
    if(Seed!=INDEX_NONE && Nearest<FMath::Square(Cell.Size()*1.5f)) { Queue.Add(Seed); Occupancy[Seed]=255; }
    constexpr int32 DX[]={1,-1,0,0,0,0}, DY[]={0,0,1,-1,0,0}, DZ[]={0,0,0,0,1,-1};
    for(int32 Head=0;Head<Queue.Num();++Head)
    {
        const int32 I=Queue[Head], X=I%NX, Y=(I/NX)%NY, Z=I/(NX*NY);
        for(int32 D=0;D<6;++D)
        {
            const int32 XX=X+DX[D],YY=Y+DY[D],ZZ=Z+DZ[D];
            if(XX<0 || XX>=NX || YY<0 || YY>=NY || ZZ<0 || ZZ>=NZ) continue;
            const int32 J=Index(XX,YY,ZZ);
            if(!Free[J] || Occupancy[J]) continue;
            FHitResult Hit;
            if(GetWorld()->SweepSingleByObjectType(Hit,Centers[I],Centers[J],FQuat::Identity,Objects,Shape,Query)) continue;
            Occupancy[J]=255; Queue.Add(J);
        }
    }
    FilledCellCount=Queue.Num();
    if(VolumeInstance) UploadDensity();
}

bool AERSmokeCloud::ContainsSmokeAt(const FVector& Position) const
{
    const FVector Unit=(Position-VolumeOrigin+FVector(SimSize.X*.5f,SimSize.Y*.5f,0))/SimSize;
    if(Unit.X<0 || Unit.X>=1 || Unit.Y<0 || Unit.Y>=1 || Unit.Z<0 || Unit.Z>=1) return false;
    const int32 I=Index(int32(Unit.X*NX),int32(Unit.Y*NY),int32(Unit.Z*NZ));
    return Occupancy.IsValidIndex(I) && Occupancy[I]!=0;
}

void AERSmokeCloud::UploadDensity()
{
    if(!VolumeInstance) return;
    constexpr int32 W=NX*NZ,H=NY;
    if(!DensityAtlas)
    {
        DensityAtlas=UTexture2D::CreateTransient(W,H,PF_B8G8R8A8,TEXT("ERSmokeContainment"));
        DensityAtlas->SRGB=false; DensityAtlas->Filter=TF_Bilinear;
        DensityAtlas->AddressX=TA_Clamp; DensityAtlas->AddressY=TA_Clamp;
        DensityAtlas->NeverStream=true;
        DensityAtlas->UpdateResource();
        VolumeInstance->SetTextureParameterValue(TEXT("DensityAtlas"),DensityAtlas);
    }
    auto* Pixels=new uint8[W*H*4];
    for(int32 Z=0;Z<NZ;++Z) for(int32 Y=0;Y<NY;++Y) for(int32 X=0;X<NX;++X)
    {
        const int32 P=(X+Z*NX+Y*W)*4;
        Pixels[P]=Pixels[P+1]=Pixels[P+2]=Occupancy[Index(X,Y,Z)]; Pixels[P+3]=255;
    }
    auto* Region=new FUpdateTextureRegion2D(0,0,0,0,W,H);
    DensityAtlas->UpdateTextureRegions(0,1,Region,W*4,4,Pixels,
        [](uint8* Data,const FUpdateTextureRegion2D* R){ delete[] Data; delete R; });
}

void AERSmokeCloud::AddShot(const FVector& Start,const FVector& End,float Radius,float Lifetime)
{
    if(ServerNow()-StartedAtServerTime>=Duration || (End-Start).IsNearlyZero()) return;
    const FVector Center=VolumeOrigin+FVector(0,0,SimSize.Z*.5f);
    Radius=FMath::Clamp(Radius,1.f,400.f);
    Lifetime=Radius>=100.f ? FMath::Max(.1f,Duration-float(ServerNow()-StartedAtServerTime)) : FMath::Clamp(Lifetime,.1f,4.f);
    if(!FMath::LineBoxIntersection(FBox(Center-SimSize*.5f,Center+SimSize*.5f).ExpandBy(Radius),Start,End,End-Start)) return;
    if(ShotCuts.Num()>=4)
    {
        // Short projectile wakes must not immediately erase a large blast pocket.
        int32 Replace=ShotCuts.IndexOfByPredicate([](const FShotCut& Cut){return Cut.Radius<100.f;});
        if(Replace==INDEX_NONE) { if(Radius<100.f) return;Replace=0; }
        ShotCuts.RemoveAt(Replace);
    }
    ShotCuts.Add({Start,End,GetWorld()->GetTimeSeconds()+Lifetime,Radius,Lifetime});
    if(ShotCuts.Num()>4) ShotCuts.RemoveAt(0,ShotCuts.Num()-4);
    ++ShotInteractionCount;
    if(Radius>=100.f) { ++BlastInteractionCount;LastBlastRadius=Radius;LastBlastLifetime=Lifetime; }
    else if(Radius>12.f) ++ProjectileInteractionCount;
}

float AERSmokeCloud::RecoveredRadius(const FShotCut& Cut,double Now) const
{
    if(Now>=Cut.Until) return 0;
    const float Remaining=FMath::Clamp(float(Cut.Until-Now)/Cut.Lifetime,0.f,1.f);
    // Blast dispersal barely refills before cloud expiry; bullet wakes close slowly.
    return Cut.Radius*(Cut.Radius>=100.f ? .95f+.05f*Remaining : FMath::Sqrt(Remaining));
}
float AERSmokeCloud::GetActiveBlastRadius() const
{
    float Radius=0;for(const auto& Cut:ShotCuts) if(Cut.Radius>=100.f) Radius=FMath::Max(Radius,RecoveredRadius(Cut,GetWorld()->GetTimeSeconds()));
    return Radius;
}
float AERSmokeCloud::GetActiveShotRadius() const
{
    float Radius=0;for(const auto& Cut:ShotCuts) if(Cut.Radius<=12.f) Radius=FMath::Max(Radius,RecoveredRadius(Cut,GetWorld()->GetTimeSeconds()));
    return Radius;
}

void AERSmokeCloud::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const double Now=GetWorld()->GetTimeSeconds();
    if(Now>=NextGeometryCheck)
    {
        uint64 Revision=0;
        for(TActorIterator<AERBlockRoom> It(GetWorld());It;++It)
            {
                Revision=HashCombineFast(Revision,It->GetMapEpoch());
                Revision=HashCombineFast(Revision,It->HasAuthority()?It->GetMapSequence():It->GetAppliedMapSequence());
                Revision=HashCombineFast(Revision,uint64(It->GetCellWorld().GetCellCount()));
                Revision=HashCombineFast(Revision,uint64(It->GetCellWorld().GetRemovedCount()));
            }
        if(GeometryRevision!=Revision) { GeometryRevision=Revision; RebuildContainment(); }
        NextGeometryCheck=Now+.75;
    }
    ShotCuts.RemoveAll([Now](const FShotCut& Cut){ return Cut.Until<Now; });
    LastColliderCount=ShotCuts.Num();
    if(!VolumeInstance) return;
    const float Age=FMath::Max(0.f,float(ServerNow()-StartedAtServerTime));
    if(Age>=1.f && Age-DeltaSeconds<1.f)
        UE_LOG(LogTemp,Display,TEXT("ER_SMOKE_RENDER mesh=%s material=%s visible=%d location=%s scale=%s bounds=%s"),*GetNameSafe(Volume->GetStaticMesh()),*GetNameSafe(Volume->GetMaterial(0)),Volume->IsVisible(),*Volume->GetComponentLocation().ToString(),*Volume->GetComponentScale().ToString(),*Volume->Bounds.GetBox().ToString());
    VolumeInstance->SetScalarParameterValue(TEXT("Growth"),FMath::Clamp(Age/2.2f,0.f,1.f));
    VolumeInstance->SetScalarParameterValue(TEXT("Fade"),FMath::Clamp((Duration-Age)/4.f,0.f,1.f));
    VolumeInstance->SetScalarParameterValue(TEXT("SmokeAge"),Age);
    VolumeInstance->SetVectorParameterValue(TEXT("ShapePhase"),FLinearColor(ShapePhase));
    for(int32 I=0;I<4;++I)
    {
        const auto* Cut=ShotCuts.IsValidIndex(I) ? &ShotCuts[I] : nullptr;
        VolumeInstance->SetVectorParameterValue(*FString::Printf(TEXT("CutStart%d"),I),FLinearColor(Cut ? Cut->Start : FVector::ZeroVector));
        VolumeInstance->SetVectorParameterValue(*FString::Printf(TEXT("CutEnd%d"),I),FLinearColor(Cut ? Cut->End : FVector::ZeroVector));
        VolumeInstance->SetScalarParameterValue(*FString::Printf(TEXT("CutRadius%d"),I),Cut ? RecoveredRadius(*Cut,Now) : 0.f);
    }
}

void AERSmokeCloud::EndPlay(const EEndPlayReason::Type Reason)
{
    if(ReleaseAudio) ReleaseAudio->Stop();
    Super::EndPlay(Reason);
}
void AERSmokeCloud::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERSmokeCloud,StartedAtServerTime);
    DOREPLIFETIME(AERSmokeCloud,ShapePhase);
    DOREPLIFETIME(AERSmokeCloud,SmokeColor);
}

bool AERSmokeCloud::PrepareOwnedSystem(UNiagaraSystem* SmokeSystem)
{
#if WITH_EDITOR
    if(!SmokeSystem || !SmokeSystem->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Effects/Smoke/"))) return false;
    SmokeSystem->Modify();
    auto& Store=SmokeSystem->GetExposedParameters();
    for(const auto& Variable:Store.ReadParameterVariables())
        if(Variable.GetName()==TEXT("User.outVolumeTX"))
            if(auto* Interface=Cast<UNiagaraDataInterfaceRenderTargetVolume>(Store.GetDataInterface(Variable)))
            {
                Interface->Modify();
                Interface->RenderTargetUserParameter.Parameter=FNiagaraVariable();
                Interface->bInheritUserParameterSettings=false;
                Interface->bOverrideFormat=true;
                Interface->OverrideRenderTargetFormat=RTF_RGBA16f;
                Interface->Size=FIntVector(40,40,28);
            }
    SmokeSystem->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

FString AERSmokeCloud::DescribeSmokeSource(UBlueprint* Blueprint, UNiagaraSystem* SmokeSystem)
{
    FString Text;
#if WITH_EDITOR
    if (SmokeSystem)
    {
        const auto& Store = SmokeSystem->GetExposedParameters();
        for (const auto& Variable : Store.ReadParameterVariables())
        {
            Text += FString::Printf(TEXT("PARAM %s type=%s"),*Variable.GetName().ToString(),*Variable.GetType().GetName());
            if (const auto* Struct = Variable.GetType().GetScriptStruct())
                if (const uint8* Data=Store.GetParameterData(Variable))
                {
                    FString Value; Struct->ExportText(Value,Data,nullptr,nullptr,PPF_None,nullptr);
                    Text += TEXT(" value=")+Value;
                }
            Text += TEXT("\n");
        }
    }
    if (Blueprint && Blueprint->GeneratedClass)
    {
        UObject* Defaults=Blueprint->GeneratedClass->GetDefaultObject();
        for (TFieldIterator<FProperty> It(Blueprint->GeneratedClass,EFieldIterationFlags::None);It;++It)
        {
            FString Value; It->ExportText_InContainer(0,Value,Defaults,nullptr,Defaults,PPF_None);
            Text += FString::Printf(TEXT("PROPERTY %s = %s\n"),*It->GetName(),*Value);
        }
        TArray<UEdGraph*> Graphs=Blueprint->UbergraphPages; Graphs.Append(Blueprint->FunctionGraphs);
        for (auto* Graph : Graphs) for (const auto& Node : Graph->Nodes)
        {
            Text += FString::Printf(TEXT("NODE %s: %s\n"),*Node->GetName(),*Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
            for (auto* Pin : Node->Pins)
            {
                Text += FString::Printf(TEXT("  PIN %s %s default=%s object=%s"),*Pin->PinName.ToString(),Pin->Direction==EGPD_Input ? TEXT("in") : TEXT("out"),*Pin->DefaultValue,*GetPathNameSafe(Pin->DefaultObject));
                for(auto* Link : Pin->LinkedTo) Text += FString::Printf(TEXT(" -> %s.%s"),*Link->GetOwningNode()->GetName(),*Link->PinName.ToString());
                Text += TEXT("\n");
            }
        }
    }
#endif
    return Text;
}
