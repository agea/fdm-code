#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERBlockProfile.h"
#include "ERBlockStressProbe.generated.h"

// Opt-in deterministic benchmark/stress runner. Never spawned in ordinary play.
UCLASS(NotBlueprintable)
class FDM_API AERBlockStressProbe : public AActor
{
    GENERATED_BODY()
public:
    AERBlockStressProbe();
    virtual void Tick(float DeltaSeconds) override;
private:
    bool Failed=false, Started=false, BenchmarkOnly=false;
    int32 Case=0, Sample=-1, Samples=8, Phase=0;
    float Elapsed=0;
    uint32 InitialShots=0;
    FString Label=TEXT("optimized"), Output;
    TArray<FString> Rows;
    void Check(bool Value,const TCHAR* Name);
    void Record(class AERProceduralMap* Map,const TCHAR* Operation,const FERBlockOperationProfile& Profile,int32 Removed=0,int32 Candidates=0,int32 Examined=0);
    void RunSample(class AERProceduralMap* Map,class AERCharacter* Character);
    void Finish();
};
