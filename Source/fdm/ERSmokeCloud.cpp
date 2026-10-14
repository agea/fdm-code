#include "ERSmokeCloud.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "UObject/UnrealType.h"
#include "ERCharacter.h"
#include "ERMatch.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraDataInterfaceRenderTargetVolume.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Misc/App.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
int32 ActiveFogClouds=0, PreviousFogSetting=0;
}

AERSmokeCloud::AERSmokeCloud()
{
    bReplicates = true; bAlwaysRelevant = true; SetNetUpdateFrequency(2);
    PrimaryActorTick.bCanEverTick = true;
    Fumes = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Fumes"));
    SetRootComponent(Fumes);
    Fumes->SetAutoActivate(false);
    Fumes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Fumes->SetCastShadow(false);
}

void AERSmokeCloud::Prewarm(UWorld* World, TSubclassOf<AERSmokeCloud> CloudClass, const FVector& ViewPosition)
{
    if(!World || !CloudClass || World->GetNetMode()==NM_DedicatedServer || !FApp::CanEverRender()) return;
    static TSet<TWeakObjectPtr<UWorld>> PreparedWorlds;
    if(PreparedWorlds.Contains(World)) return;
    PreparedWorlds.Add(World);
    TWeakObjectPtr<UWorld> WeakWorld=World;
    // Run after initial scalability commands. Empty density warms the same
    // simulation/fog pipelines during map loading without a visible smoke puff.
    World->GetTimerManager().SetTimerForNextTick([WeakWorld,CloudClass,ViewPosition]()
    {
        if(!WeakWorld.IsValid()) return;
        const FTransform Transform(FRotator::ZeroRotator,ViewPosition+FVector(350,0,-100));
        auto* Warmup=WeakWorld->SpawnActorDeferred<AERSmokeCloud>(CloudClass,Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!Warmup) return;
        Warmup->bPrewarm=true; Warmup->Duration=.8f; Warmup->SetReplicates(false);
        UGameplayStatics::FinishSpawningActor(Warmup,Transform);
    });
}

double AERSmokeCloud::ServerNow() const
{
    const auto* State=GetWorld()->GetGameState<AERGameState>();
    return State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

void AERSmokeCloud::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) { StartedAtServerTime=ServerNow(); SetLifeSpan(Duration); }
    SetActorEnableCollision(false);
    Fumes->SetAsset(System);
    Fumes->SetVariableInt(TEXT("User.ResolutionMaxAxis"),FMath::Clamp(GridResolution,24,64));
    Fumes->SetVariableVec3(TEXT("User.WorldSpaceSize"),SimSize);
    Fumes->SetVariableVec3(TEXT("User.SourceOffset"),FVector(0,0,80));
    Fumes->SetVariableFloat(TEXT("User.Smoke_Radius"),1);
    Fumes->SetVariableFloat(TEXT("User.SmokeReponseRate"),1.5f);
    Fumes->SetVariableFloat(TEXT("User.AppearFogParam"),bPrewarm ? 0 : 1);
    Fumes->SetVariableLinearColor(TEXT("User.SRend_SmokeColor"),SmokeColor);
    Fumes->SetSystemFixedBounds(FBox(FVector(-SimSize.X*.5f,-SimSize.Y*.5f,-20),FVector(SimSize.X*.5f,SimSize.Y*.5f,SimSize.Z+20)));
    if (GetNetMode()!=NM_DedicatedServer && FApp::CanEverRender())
    {
        if (auto* Setting=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog")))
        {
            if(ActiveFogClouds++==0) PreviousFogSetting=Setting->GetInt();
            Setting->SetWithCurrentPriority(1); bUsesFogPass=true;
        }
        for(TActorIterator<AExponentialHeightFog> It(GetWorld());It;++It)
            if(It->GetComponent()->bEnableVolumetricFog) { FogSupport=*It; break; }
        if(!FogSupport)
        {
            FogSupport=GetWorld()->SpawnActor<AExponentialHeightFog>();
            FogSupport->Tags.Add(TEXT("ERSmokeFogSupport"));
            FogSupport->SetReplicates(false);
            auto* Fog=FogSupport->GetComponent();
            // A near-zero background density enables particle volume injection
            // without adding visible haze to the rest of the map.
            Fog->SetFogDensity(.000001f);
            Fog->SetVolumetricFog(true);
            Fog->SetVolumetricFogDistance(3500);
            Fog->SetVolumetricFogExtinctionScale(1);
        }
        Fumes->Activate(true);
    }
    else Fumes->SetComponentTickEnabled(false);
    UE_LOG(LogTemp,Display,TEXT("ER_SMOKE_%s system=%s grid=%d duration=%.1f replicated=%d collision=none"),
        bPrewarm ? TEXT("PREWARM") : TEXT("CLOUD"),*GetNameSafe(System),GridResolution,Duration,GetIsReplicated());
}

void AERSmokeCloud::EndPlay(const EEndPlayReason::Type Reason)
{
    if(bUsesFogPass && --ActiveFogClouds==0)
        if(auto* Setting=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog"))) Setting->SetWithCurrentPriority(PreviousFogSetting);
    if(FogSupport && FogSupport->ActorHasTag(TEXT("ERSmokeFogSupport")))
    {
        bool Used=false;
        for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It)
            if(*It!=this && !It->IsActorBeingDestroyed() && It->FogSupport==FogSupport) { Used=true; break; }
        if(!Used) FogSupport->Destroy();
    }
    Super::EndPlay(Reason);
}

void AERSmokeCloud::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AERSmokeCloud,StartedAtServerTime);
}

void AERSmokeCloud::AddShot(const FVector& Start, const FVector& End)
{
    if (ServerNow()-StartedAtServerTime >= Duration || (End-Start).IsNearlyZero()) return;
    const FVector Center=GetActorLocation()+FVector(0,0,SimSize.Z*.5f);
    const FBox Bounds(Center-SimSize*.5f,Center+SimSize*.5f);
    if (!FMath::LineBoxIntersection(Bounds,Start,End,End-Start)) return;
    const FVector Direction=(End-Start).GetSafeNormal();
    const FVector Closest=FMath::ClosestPointOnSegment(Center,Start,End);
    const float HalfSpan=SimSize.Size()*.5f;
    const double Until=GetWorld()->GetTimeSeconds()+.3;
    const int32 Before=ShotCuts.Num();
    // Submitted positions are visual carve spheres, never projectile collision
    // or client-selected damage. Clip to the actual server-reported shot segment.
    for (float Offset=-HalfSpan;Offset<=HalfSpan;Offset+=40.f)
    {
        const FVector Point=Closest+Direction*Offset;
        const float Along=FVector::DotProduct(Point-Start,Direction);
        if(Bounds.IsInside(Point) && Along>=0 && Along<=FVector::Dist(Start,End)) ShotCuts.Add({Point,Until});
    }
    if(ShotCuts.Num()>Before) ++ShotInteractionCount;
    if(ShotCuts.Num()>128) ShotCuts.RemoveAt(0,ShotCuts.Num()-128);
    UE_LOG(LogTemp,Verbose,TEXT("ER_SMOKE_SHOT_CUT points=%d"),ShotCuts.Num());
}

void AERSmokeCloud::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GetNetMode()==NM_DedicatedServer || !System) return;
    const float Age=FMath::Max(0.f,float(ServerNow()-StartedAtServerTime));
    const float Fade=FMath::Clamp((Duration-Age)/2.f,0.f,1.f);
    Fumes->SetVariableFloat(TEXT("User.AppearFogParam"),bPrewarm ? 0 : Fade);
    if(bPrewarm) return;
    TArray<FVector4> Colliders;
    const FVector Center=GetActorLocation()+FVector(0,0,SimSize.Z*.5f);
    const FBox Bounds(Center-SimSize*.5f,Center+SimSize*.5f);
    for(TActorIterator<AERCharacter> It(GetWorld());It;++It)
    {
        if(It->Health->bDead || !Bounds.IsInside(It->GetActorLocation())) continue;
        const FVector Position=It->GetActorLocation();
        // The asset's W field is a scale volume (X*Y*Z), not centimeters.
        const float Scale=It->GetCapsuleComponent()->GetScaledCapsuleRadius()/50.f;
        Colliders.Add(FVector4(Position,Scale*Scale*Scale));
    }
    const double Now=GetWorld()->GetTimeSeconds();
    ShotCuts.RemoveAll([Now](const FShotCut& Cut){return Cut.Until<Now;});
    for(const auto& Cut:ShotCuts) Colliders.Add(FVector4(Cut.Position,.125f));
    LastColliderCount=Colliders.Num();
    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(Fumes,TEXT("User.ActorPosSize"),Colliders);
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
