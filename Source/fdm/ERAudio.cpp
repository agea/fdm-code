#include "ERAudio.h"
#include "ERBlockRoom.h"
#include "Engine/World.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundCue.h"
#if WITH_EDITOR
#include "EdGraph/EdGraphNode.h"
#endif
#include "Sound/SoundWave.h"
#include "Sound/SoundNodeModulator.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeSwitch.h"
#include "Sound/SoundNodeWavePlayer.h"

int32 FERAudioShuffle::Choose(int32 Count)
{
    if (Count <= 0) return INDEX_NONE;
    if (Count == 1) { Remaining.Reset(); Last = 0; return Last; }
    if (Remaining.IsEmpty() || Remaining.ContainsByPredicate([Count](int32 I){ return I >= Count; }))
    {
        Remaining.Reset();
        for (int32 I=0; I<Count; ++I) Remaining.Add(I);
        // The first draw of a new bag cannot repeat the previous bag's last draw.
        int32 Pick=Random.RandRange(0, Count-2);
        if (Pick >= Last && Last != INDEX_NONE && Last < Count) ++Pick;
        else if (Last == INDEX_NONE || Last >= Count) Pick=Random.RandRange(0,Count-1);
        Last=Remaining[Pick]; Remaining.RemoveAtSwap(Pick); return Last;
    }
    const int32 Pick=Random.RandRange(0,Remaining.Num()-1);
    Last=Remaining[Pick]; Remaining.RemoveAtSwap(Pick); return Last;
}

UEROneShotAudio* UERFootstepAudio::Find(EERFootstepSurface Surface) const
{
    auto* Result=Sounds.Find(Surface);
    if (!Result || !*Result) Result=Sounds.Find(EERFootstepSurface::Hard);
    return Result ? Result->Get() : nullptr;
}
EERFootstepSurface UERFootstepAudio::Resolve(const FHitResult& Hit) const
{
    if (const auto* Room=Cast<AERBlockRoom>(Hit.GetActor()))
    {
        FERCellId Id;
        if (Room->ResolveCell(Hit,Id))
            if (const auto* Surface=CellSurfaces.Find(Room->GetCellWorld().GetPanels()[Id.Panel].Material)) return *Surface;
    }
    if (Hit.PhysMaterial.IsValid())
        if (const auto* Surface=PhysicalSurfaces.Find(UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()))) return *Surface;
    // Authored floor/terrain palettes can lack physical materials. Match asset
    // identity in the data asset, never string guesses or collision changes.
    if (auto* Component=Hit.GetComponent())
        for (int32 I=0; I<Component->GetNumMaterials(); ++I)
            if (const auto* Surface=MaterialSurfaces.Find(Component->GetMaterial(I))) return *Surface;
    return EERFootstepSurface::Hard;
}
int32 ERAudio::Play(AActor* Owner, const UEROneShotAudio* Palette, FERAudioShuffle& Shuffle,
    const FVector& Position, bool Local, float Volume, USoundAttenuation* Attenuation)
{
    if (!Owner || Owner->GetNetMode()==NM_DedicatedServer || !Palette || !Palette->Cue || Palette->Variants.IsEmpty()) return INDEX_NONE;
    const int32 Index=Shuffle.Choose(Palette->Variants.Num());
    if (!Palette->Variants[Index]) return INDEX_NONE;
    // UAudioComponent::Play returns early without auto-destroy when no device
    // exists. Keep headless selection/audits, without registering silent objects.
    if (!Owner->GetWorld() || !Owner->GetWorld()->bAllowAudioPlayback || !Owner->GetWorld()->GetAudioDeviceRaw()) return Index;
    auto* Audio=NewObject<UAudioComponent>(Owner);
    Audio->SetAutoActivate(false); // Registration must not play the preview/fallback.
    Audio->bAutoDestroy=true; Audio->bStopWhenOwnerDestroyed=true;
    Audio->bAllowSpatialization=!Local;
    Audio->SetSound(Palette->Cue);
    Audio->AttenuationSettings=Local ? nullptr : Attenuation;
    Audio->SetVolumeMultiplier(Volume);
    Audio->RegisterComponentWithWorld(Owner->GetWorld());
    Audio->SetWorldLocation(Position);
    const bool AutoStarted=Audio->IsPlaying();
    // Set before Play: one source voice, without briefly playing the fallback.
    Audio->SetIntParameter(TEXT("ERVariant"),Index);
    Audio->Play();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(),TEXT("ERAudioAudit")))
        UE_LOG(LogTemp,Display,TEXT("ER_AUDIO_VOICE palette=%s sample=%s local=%d started=%d autostart=%d"),
            *Palette->GetName(),*GetNameSafe(Palette->Variants[Index]),Local,Audio->IsPlaying(),AutoStarted);
#endif
    return Index;
}

bool UERAudioEditorLibrary::BuildOneShotCue(USoundCue* Cue, const TArray<USoundWave*>& Waves, float PitchMin, float PitchMax)
{
#if WITH_EDITOR
    if (!Cue || Waves.IsEmpty() || !Cue->GetPathName().StartsWith(TEXT("/Game/EclipseRecoil/Audio/"))) return false;
    for (auto* Wave:Waves) if (!Wave) return false;
    Cue->Modify();
    Cue->FirstNode=nullptr; Cue->AllNodes.Reset();
    // Recreate the graph as well as its serialized sound nodes on reruns.
    Cue->CreateGraph(); Cue->ClearGraph();
    auto* Mod=Cue->ConstructSoundNode<USoundNodeModulator>();
    Mod->PitchMin=PitchMin; Mod->PitchMax=PitchMax;
    Mod->VolumeMin=.891251f; Mod->VolumeMax=1.122018f;
    auto* Param=Cue->ConstructSoundNode<USoundNodeSwitch>(); Param->IntParameterName=TEXT("ERVariant");
    auto* Random=Cue->ConstructSoundNode<USoundNodeRandom>();
    Random->bRandomizeWithoutReplacement=true; Random->bShouldExcludeFromBranchCulling=true;
    TArray<USoundNode*> Children;
    for (auto* Wave:Waves)
    {
        auto* Player=Cue->ConstructSoundNode<USoundNodeWavePlayer>();
        Player->SetSoundWave(Wave); Player->bLooping=false; Children.Add(Player);
    }
    Random->SetChildNodes(Children);
    Children.Insert(Random,0); Param->SetChildNodes(Children);
    Children={Param}; Mod->SetChildNodes(Children);
    Cue->FirstNode=Mod; Cue->VolumeMultiplier=1.f; Cue->VirtualizationMode=EVirtualizationMode::Disabled; Cue->bExcludeFromRandomNodeBranchCulling=true;
    for (const auto& Node:Cue->AllNodes) if (Node && Node->GetGraphNode()) Node->GetGraphNode()->ReconstructNode();
    Cue->LinkGraphNodesFromSoundNodes(); Cue->CacheAggregateValues(); Cue->PostEditChange(); Cue->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
