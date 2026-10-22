#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERDustTemplateProbe.generated.h"

// Opt-in local validation only; ordinary sessions never create this actor.
UCLASS()
class FDM_API AERDustTemplateProbe : public AActor
{
    GENERATED_BODY()
public:
    AERDustTemplateProbe();
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    int32 Phase=0;
    int32 ArchitectureStage=0,WallPanel=INDEX_NONE,RoofPanel=INDEX_NONE;
    int64 BeforeArchitectureDetach=0;
    FVector WallPoint=FVector::ZeroVector,WallNormal=FVector::ZeroVector;
    float WallFloor=0;
    UPROPERTY(Replicated) FVector ArchitectureOutside=FVector::ZeroVector;
    UPROPERTY(Replicated) FVector ArchitectureInside=FVector::ZeroVector;
    UPROPERTY(Replicated) FVector ArchitectureRoof=FVector::ZeroVector;
    void TickArchitecture(class AERProceduralMap* Map,class AERCharacter* Character,class AERPlayerController* Controller);
    UPROPERTY(Replicated) int32 SnapshotPhase=0;
    UPROPERTY(Replicated) uint32 ExpectedDigest=0;
    int32 SeenPhase=0;
    float Elapsed=0;
    float StageTime=0;
    int32 GateIndex=0,Waypoint=0;
    bool ReachedUpper=false;
    bool CapturedGate=false;
    bool Failed=false;
    // Remote pawns on the Short A deck (a runtime FixedMeshes group).
    int32 PlacedPhase=0,BaseChecked=0;
    float BaseClock=-1;
    int32 TraversalIndex=0;
    bool TraversalStarted=false;
    bool SawTowerLadder=false;
    TArray<FVector> TraversalPath;
    void Check(bool Good,const TCHAR* Label);
    void Publish(int32 Next,class AERProceduralMap* Map);
    UPROPERTY(Replicated) TObjectPtr<class APlayerState> DeckPlayer;
    void PlaceRemote(bool Latest,const FVector& P);
};
