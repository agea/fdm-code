#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "GameFramework/HUD.h"
#include "InputActionValue.h"
#include "ERPlayerController.generated.h"

UCLASS()
class FDM_API UERGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite) FString LocalName = TEXT("Recluta");
    UPROPERTY(BlueprintReadOnly) FString LastError;
    virtual void Init() override;
    virtual void Shutdown() override;
    void Host(int32 BotCount = 0, bool bStationaryTarget = false);
    bool Join(const FString& Address);
    void ReturnToMenu();
    static bool IsValidAddress(const FString& Address);
    float GetMouseSensitivity() const { return MouseSensitivity; }
    float GetMouseAcceleration() const { return MouseAcceleration; }
    void SetMouseAcceleration(float Value);
    float GetMouseLookMultiplier() const;
    void SetMouseSensitivity(float Value);
    void SaveMouseSensitivity();
    int32 PrimaryWeapon = 0;
    int32 AlphaSkin = 0;
    int32 OmegaSkin = 0;
    void SaveSkins();
    void SaveLoadout();
private:
    float MouseSensitivity = .5f;
    float MouseAcceleration = .5f;
    FDelegateHandle NetworkFailureHandle, TravelFailureHandle;
    void NetworkFailure(UWorld* World, class UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
    void TravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error);
};

UCLASS()
class FDM_API AERPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UPROPERTY(Replicated, BlueprintReadOnly) bool bAwaitingRespawn = false;
    UPROPERTY(Replicated, BlueprintReadOnly) double RespawnAllowedAt = 0;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputMappingContext> GameplayMapping;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputAction> MoveAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputAction> LookAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputAction> JumpAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputAction> CrouchAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputAction> FireAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputAction> ReloadAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputAction> ScoreboardAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputAction> MenuAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite) TObjectPtr<class UInputAction> WalkAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Arsenal") TArray<TObjectPtr<class UERWeaponData>> PrimaryOptions;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Team appearance") TSubclassOf<class AERCharacter> SkinCatalog;
    bool bZoomHeld = false;
    float FlashOpacity = 0;
    bool bShowScoreboard = false;
    bool bMenuOpen = false;
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(Server, Reliable) void ServerRequestRespawn();
    UFUNCTION(Server, Reliable) void ServerSetName(const FString& Name);
    UFUNCTION(Server, Reliable) void ServerSetPrimary(int32 Index);
    UFUNCTION(Server, Reliable) void ServerSetSkins(int32 Alpha, int32 Omega);
    UFUNCTION(Client, Reliable) void ClientFlash(float Strength);
    bool IsMenuLevel() const;
    void ShowMenu(bool bShow);
    void ScrollWeapon(int32 Direction);
private:
    double LastForwardTap = -1;
    double LastCrouchTap = -1;
    TWeakObjectPtr<APawn> GesturePawn;
    void ForwardPressed();
    void ForwardReleased();
    double WheelQuietUntil = 0;
    TSharedPtr<class SWidget> MenuWidget;
    TSharedPtr<class SWidget> FrameRateWidget;
    TSharedPtr<class SComboButton> DisplaySizeSelector;
    float FrameTimeMs = 0;
    FIntPoint LastRenderViewport = FIntPoint::ZeroValue;
    float SceneOutputFraction = 1;
    bool bPerformanceAudit = false;
    double NextPerformanceAuditAt = 0;
    void UpdateRenderResolution();
    void ResetRenderResolution();
    FText GetFrameRateText() const;
    FIntPoint GetDisplayResolution() const;
    FText GetDisplaySizeText() const;
    void ApplyDisplaySize(int32 Percent);
    TSharedPtr<class SEditableTextBox> NameBox, AddressBox;
    bool bInputAudit = false;
    double NextInputAuditAt = 0;
    double NextMouseAuditAt = 0;
    void WalkPressed();
    void WalkReleased();
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void FirePressed();
    void FireReleased();
    void JumpPressed();
    void JumpReleased();
    void CrouchPressed();
    void CrouchReleased();
    void ReloadPressed();
    void ScoreboardPressed();
    void ScoreboardReleased();
    void MenuPressed();
    void SelectSlot(int32 Slot);
    void SelectPrimary() { SelectSlot(0); }
    void SelectSidearm() { SelectSlot(1); }
    void SelectHE() { SelectSlot(2); }
    void SelectSmoke() { SelectSlot(3); }
    void SelectStun() { SelectSlot(4); }
    void NextWeapon();
    void PreviousWeapon();
    void ZoomPressed() { if (!bMenuOpen) bZoomHeld = true; }
    void ZoomReleased() { bZoomHeld = false; }
};

UCLASS()
class FDM_API AERHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    TWeakObjectPtr<class AERWeapon> ReticleWeapon;
    float ReticleRadius = 0;
    double NextReticleAuditAt = 0;
};
