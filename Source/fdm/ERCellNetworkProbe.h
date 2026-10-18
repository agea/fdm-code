#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERCellNetworkProbe.generated.h"

// One per remote player, owned by its controller: the client-to-server report
// channel of the cell network probe, without touching the player controller.
UCLASS(NotBlueprintable)
class AERCellNetworkReporter : public AActor
{
    GENERATED_BODY()
public:
    AERCellNetworkReporter();
    UFUNCTION(Server, Reliable) void ServerHello(const FString& InRole);
    UFUNCTION(Server, Reliable) void ServerReport(int32 Stage, uint32 Epoch, uint32 Sequence, uint32 Digest, bool bPassed);
    FString Role; // authority only
    TWeakObjectPtr<class AERCellNetworkProbe> Probe;
};

// Explicitly enabled (-ERCellNetworkProbe) listen-server regression of the 5cm
// cell protocol (7) on L_DestructionLab ?Cells=1, never normal play.
UCLASS(NotBlueprintable)
class AERCellNetworkProbe : public AActor
{
    GENERATED_BODY()
public:
    AERCellNetworkProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void ReceiveReport(AERCellNetworkReporter* Reporter, int32 Stage, uint32 Epoch, uint32 Sequence, uint32 Digest, bool bPassed);
private:
    UPROPERTY(Replicated) int32 Phase = 0;
    UPROPERTY(Replicated) uint32 ExpectedEpoch = 0;
    UPROPERTY(Replicated) uint32 ExpectedSequence = 0;
    UPROPERTY(Replicated) uint32 ExpectedDigest = 0;
    // Chunks carried by the authority shards of this epoch, and a hash of their indices.
    UPROPERTY(Replicated) int32 ExpectedChunks = 0;
    UPROPERTY(Replicated) uint32 ExpectedChunkHash = 0;
    UPROPERTY(Replicated) int64 ExpectedRemoved = 0;
    UPROPERTY(Replicated) FVector WalkFrom = FVector::ZeroVector;
    UPROPERTY(Replicated) FVector WalkTo = FVector::ZeroVector;
    UPROPERTY(Replicated) bool bWalkCrouched = false;
    TWeakObjectPtr<class AERBlockRoom> Map;
    TWeakObjectPtr<class AERPlayerController> Host;
    TMap<TWeakObjectPtr<class AERPlayerController>, TWeakObjectPtr<AERCellNetworkReporter>> Reporters;
    TSet<FString> Reports;
    double Deadline = 0, NextDiagnostic = 0, NextTrace = 0;
    uint32 PublishedEpoch = 0, PublishedSequence = 0;
    int32 Grenades = 0;
    bool bFailed = false;
    // Client side.
    FString Role;
    bool bHelloSent = false, bWalking = false, bCollapseStarted = false;
    int32 ClientPhase = -1;
    float PhaseElapsed = 0;
    double WalkReportedAt = 0;
    void Advance(int32 Next, float Seconds = 30);
    void Check(bool bValue, const TCHAR* Label);
    void Complete();
    void ClientTick(float DeltaSeconds);
    void LogBytes(class AERBlockRoom* Room, const TCHAR* Label) const;
    bool ShardChunks(class AERBlockRoom* Room, int32& Count, uint32& Hash, int32* MaskBytes = nullptr, bool* bOnlyModified = nullptr) const;
    int32 CellParity(class AERBlockRoom* Room, int32& Samples) const;
    void SweepPath(class AERCharacter* C, const FVector& From, const FVector& To) const;
    class AERPlayerController* RolePlayer(const TCHAR* Name) const;
    bool Reported(const TCHAR* Name, int32 Stage) const { return Reports.Contains(FString(Name) + FString::FromInt(Stage)); }
    void Place(class AERPlayerController* PC, const FVector& P, const FRotator& Aim);
    bool WalkArrived(class AERPlayerController* PC, const TCHAR* Label);
    class AERBlockRoom* FindRoom() const;
};
