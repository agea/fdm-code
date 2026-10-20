#pragma once
#include "CoreMinimal.h"
#include "ERServerSettings.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "ERMatch.generated.h"

UENUM(BlueprintType)
enum class EERTeam : uint8 { None, Alpha, Omega };

UCLASS()
class FDM_API AERPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    UPROPERTY(Replicated, BlueprintReadOnly) EERTeam Team = EERTeam::None;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Kills = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Deaths = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 TeamKills = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 PrimaryWeapon = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 AlphaSkin = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 OmegaSkin = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ActiveSkin = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bLobbyReady = false;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 MapVote = -1;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 SpareMagazines = -1;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 HECount = -1;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 SmokeCount = -1;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 StunCount = -1;
    virtual void CopyProperties(APlayerState* Target) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

UCLASS()
class FDM_API AERGameState : public AGameState
{
    GENERATED_BODY()
public:
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ServerMaxPlayers = ERServer::DefaultMaxPlayers;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bLobby = false;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bLobbyStarting = false;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 LobbyAlphaBots = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 LobbyOmegaBots = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 AlphaScore = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 OmegaScore = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) double MatchEndServerTime = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) double RestartServerTime = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) EERTeam Winner = EERTeam::None;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

UCLASS()
class FDM_API AERTeamPlayerStart : public APlayerStart
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EERTeam Team = EERTeam::Alpha;
};

UCLASS()
class FDM_API AERGameMode : public AGameMode
{
    GENERATED_BODY()
public:
    AERGameMode();
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Recon") TSubclassOf<class AERReconDirector> ReconClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) int32 ScoreLimit = 50;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) float MatchDuration = 600;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Training") TSubclassOf<class AERCharacter> TrainingTargetPawnClass;
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void BeginPlay() override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void Logout(AController* Player) override;
    virtual FString InitNewPlayer(APlayerController* Player, const FUniqueNetIdRepl& Id, const FString& Options, const FString& Portal) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* Player) override;
    virtual bool ReadyToStartMatch_Implementation() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual void RestartPlayer(AController* Player) override;
    virtual void HandleMatchHasStarted() override;
    void RecordDeath(class AERCharacter* Victim, AController* Killer);
    void RequestRespawn(AController* Player);
    int32 GetNumTestBots() const { return TestBots.Num(); }
    int32 GetMaxPlayers() const { return MaxHumanPlayers; }
    bool HasOpenPlayerSlot();
    bool IsLobby() const { return bLobby; }
    void CheckLobby();
    static FString SanitizeName(const FString& Name);
private:
    friend class AERIndustrialMapProbe;
    int32 MaxHumanPlayers = ERServer::DefaultMaxPlayers;
    TWeakObjectPtr<class AERProceduralMap> PreparedSpawnMap;
    uint32 PreparedSpawnEpoch = MAX_uint32;
    int32 PreparedSpawnSeed = 0;
    void PrepareTeamStarts();
    class AERTeamPlayerStart* FindAdditionalStart(AController* Player, TConstArrayView<AERTeamPlayerStart*> Anchors);
    bool bLobby = false;
    bool bLobbyTravelPending = false;
    EERTeam NextTieTeam = EERTeam::Alpha;
    int32 DesiredBots = 0;
    bool bStationaryBots = false;
    bool bMovementBots = false;
    bool bConfiguredBots = false;
    int32 AlphaBotCount = 0;
    int32 OmegaBotCount = 0;
    TArray<double> PendingHumanSlots;
    UPROPERTY() TArray<TObjectPtr<class AERBotController>> TestBots;
    FTimerHandle MatchTimer;
    void FillBots();
    void RemoveBot();
    void PrunePendingSlots();
    void CheckMatch();
    void FinishRound();
    void ResetRound();
    void AssignTeam(AERPlayerState* Player);
};
