#include "ERPlayerController.h"
#include "ERMouseResponse.h"
#include "Misc/App.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "ERMatch.h"
#include "ERWeapon.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "GenericPlatform/GenericApplication.h"
#include "UnrealClient.h"
#include "DynamicResolutionState.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "GlobalRenderResources.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"

void UERGameInstance::Init()
{
    Super::Init();
    GConfig->GetInt(TEXT("EclipseRecoil.Loadout"), TEXT("PrimaryWeapon"), PrimaryWeapon, GGameUserSettingsIni);
    PrimaryWeapon = FMath::Clamp(PrimaryWeapon, 0, 6);
    GConfig->GetInt(TEXT("EclipseRecoil.Appearance"), TEXT("AlphaSkin"), AlphaSkin, GGameUserSettingsIni);
    GConfig->GetInt(TEXT("EclipseRecoil.Appearance"), TEXT("OmegaSkin"), OmegaSkin, GGameUserSettingsIni);
    AlphaSkin = ERSkins::ValidIndex(AlphaSkin); OmegaSkin = ERSkins::ValidIndex(OmegaSkin);
    GConfig->GetFloat(TEXT("EclipseRecoil.Input"), TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
    MouseSensitivity = FMath::IsFinite(MouseSensitivity) ? FMath::Clamp(MouseSensitivity, 0.f, 1.f) : .5f;
    GConfig->GetFloat(TEXT("EclipseRecoil.Input"), TEXT("MouseAcceleration"), MouseAcceleration, GGameUserSettingsIni);
    MouseAcceleration = FMath::IsFinite(MouseAcceleration) ? FMath::Clamp(MouseAcceleration, 0.f, 1.f) : .5f;
    NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UERGameInstance::NetworkFailure);
    TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UERGameInstance::TravelFailure);
}
void UERGameInstance::SaveLoadout()
{
    GConfig->SetInt(TEXT("EclipseRecoil.Loadout"), TEXT("PrimaryWeapon"), PrimaryWeapon, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
void UERGameInstance::SaveSkins()
{
    GConfig->SetInt(TEXT("EclipseRecoil.Appearance"), TEXT("AlphaSkin"), AlphaSkin, GGameUserSettingsIni);
    GConfig->SetInt(TEXT("EclipseRecoil.Appearance"), TEXT("OmegaSkin"), OmegaSkin, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
void UERGameInstance::Shutdown()
{
    SaveMouseSensitivity();
    GEngine->OnNetworkFailure().Remove(NetworkFailureHandle); GEngine->OnTravelFailure().Remove(TravelFailureHandle); Super::Shutdown();
}
float UERGameInstance::GetMouseLookMultiplier() const
{
    // IA_Look already scales by 0.18. The slider midpoint lowers that to 0.16;
    // logarithmic travel gives equal proportional adjustment on either side.
    return (0.16f / 0.18f) * FMath::Pow(4.f, 2.f * MouseSensitivity - 1.f);
}
void UERGameInstance::SetMouseSensitivity(float Value)
{
    if (!FMath::IsFinite(Value)) return;
    MouseSensitivity = FMath::Clamp(Value, 0.f, 1.f);
    GConfig->SetFloat(TEXT("EclipseRecoil.Input"), TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
}
void UERGameInstance::SaveMouseSensitivity()
{
    GConfig->SetFloat(TEXT("EclipseRecoil.Input"), TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("EclipseRecoil.Input"), TEXT("MouseAcceleration"), MouseAcceleration, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
    UE_LOG(LogTemp, Display, TEXT("ER_MOUSE_SENSITIVITY slider=%.0f%% multiplier=%.3f"), MouseSensitivity * 100.f, GetMouseLookMultiplier());
    UE_LOG(LogTemp, Display, TEXT("ER_MOUSE_ACCELERATION slider=%.0f%%"), MouseAcceleration * 100.f);
}
void UERGameInstance::SetMouseAcceleration(float Value)
{
    if (!FMath::IsFinite(Value)) return;
    MouseAcceleration = FMath::Clamp(Value, 0.f, 1.f);
    GConfig->SetFloat(TEXT("EclipseRecoil.Input"), TEXT("MouseAcceleration"), MouseAcceleration, GGameUserSettingsIni);
}
void UERGameInstance::Host(int32 BotCount, bool bStationaryTarget)
{
    LastError.Empty();
    FString Options = TEXT("listen?Name=") + AERGameMode::SanitizeName(LocalName);
    if (BotCount > 0) Options += FString::Printf(TEXT("?ERBots=%d?ERBotMode=%s"), FMath::Clamp(BotCount,1,bStationaryTarget ? 2 * ERSkins::Count : 3), bStationaryTarget ? TEXT("Target") : TEXT("Patrol"));
    Options += FString::Printf(TEXT("?Primary=%d?AlphaSkin=%d?OmegaSkin=%d"), PrimaryWeapon, AlphaSkin, OmegaSkin);
    UGameplayStatics::OpenLevel(GetWorld(), TEXT("/Game/EclipseRecoil/Maps/L_Depot"), true, Options);
}
bool UERGameInstance::IsValidAddress(const FString& Address)
{
    FString Host, Port; TArray<FString> Segments;
    if (!Address.Split(TEXT(":"), &Host, &Port)) Host = Address;
    else
    {
        if (Port.IsEmpty() || !Port.IsNumeric() || Port.Contains(TEXT(".")) || Port.StartsWith(TEXT("-")) || Port.StartsWith(TEXT("+")) || Port.Len() > 5) return false;
        const int32 Number = FCString::Atoi(*Port); if (Number < 1 || Number > 65535) return false;
    }
    Host.ParseIntoArray(Segments, TEXT("."), false);
    if (Segments.Num() != 4) return false;
    for (const FString& Segment : Segments)
    {
        if (Segment.IsEmpty() || Segment.Len() > 3) return false;
        for (TCHAR C : Segment) if (!FChar::IsDigit(C)) return false;
        if (FCString::Atoi(*Segment) > 255) return false;
    }
    return true;
}
bool UERGameInstance::Join(const FString& RawAddress)
{
    const FString Address = RawAddress.TrimStartAndEnd();
    if (!IsValidAddress(Address)) { LastError = TEXT("Inserisci un IPv4 valido, per esempio 192.168.1.20:7777."); return false; }
    if (APlayerController* PC = GetFirstLocalPlayerController())
    {
        LastError = TEXT("Connessione in corso...");
        PC->ClientTravel(Address + TEXT("?Name=") + AERGameMode::SanitizeName(LocalName) + FString::Printf(TEXT("?Primary=%d?AlphaSkin=%d?OmegaSkin=%d"), PrimaryWeapon, AlphaSkin, OmegaSkin), TRAVEL_Absolute); return true;
    }
    LastError = TEXT("Giocatore locale non disponibile."); return false;
}
void UERGameInstance::ReturnToMenu() { UGameplayStatics::OpenLevel(GetWorld(), TEXT("/Game/EclipseRecoil/Maps/L_MainMenu")); }
void UERGameInstance::NetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error)
{
    if (World != GetWorld()) return;
    LastError = TEXT("Connessione interrotta o non riuscita: ") + Error.Left(180);
    ReturnToMenu();
}
void UERGameInstance::TravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error)
{
    if (World != GetWorld()) return;
    LastError = TEXT("Caricamento non riuscito: ") + Error.Left(180);
    if (!UGameplayStatics::GetCurrentLevelName(World).Contains(TEXT("L_MainMenu"))) ReturnToMenu();
}
bool AERPlayerController::IsMenuLevel() const { return UGameplayStatics::GetCurrentLevelName(GetWorld()).Contains(TEXT("L_MainMenu")); }
void AERPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
#if !UE_BUILD_SHIPPING
    bInputAudit = FParse::Param(FCommandLine::Get(), TEXT("ERInputAudit"));
    bPerformanceAudit = FParse::Param(FCommandLine::Get(), TEXT("ERPerformanceAudit"));
#endif
    if (auto* GI = GetGameInstance<UERGameInstance>()) ServerSetName(GI->LocalName);
    if (ULocalPlayer* LP = GetLocalPlayer()) if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) if (GameplayMapping) Subsystem->AddMappingContext(GameplayMapping, 0);
    ShowMenu(IsMenuLevel());
    UpdateRenderResolution();
    FrameRateWidget = SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(12)
        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0,0,0,.55f)).Padding(8)
            [SNew(STextBlock).Text_Lambda([this]() { return GetFrameRateText(); })]];
    FrameRateWidget->SetVisibility(EVisibility::HitTestInvisible);
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(FrameRateWidget.ToSharedRef(), 30);
}
void AERPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    FlashOpacity = FMath::Max(0.f, FlashOpacity - DeltaTime * .35f);
    if (IsLocalController()) UpdateRenderResolution();
    if (IsLocalController() && DeltaTime > 0)
        FrameTimeMs = FrameTimeMs > 0 ? FMath::Lerp(FrameTimeMs, DeltaTime * 1000.f, 1.f - FMath::Exp(-DeltaTime * 3.f)) : DeltaTime * 1000.f;
#if !UE_BUILD_SHIPPING
    if (bPerformanceAudit && IsLocalController() && GetWorld()->GetTimeSeconds() >= NextPerformanceAuditAt)
    {
        NextPerformanceAuditAt = GetWorld()->GetTimeSeconds() + 1;
        UE_LOG(LogTemp, Display, TEXT("ER_PERFORMANCE %s viewport=%dx%d secondary=%.1f%%"),
            *GetFrameRateText().ToString(), LastRenderViewport.X, LastRenderViewport.Y, SceneOutputFraction * 100.f);
    }
    if (!bInputAudit || !IsLocalController() || GetWorld()->GetTimeSeconds() < NextInputAuditAt) return;
    NextInputAuditAt = GetWorld()->GetTimeSeconds()+.1;
    const auto* Character = Cast<AERCharacter>(GetPawn());
    if (!Character || !Character->Weapon) return;
    const FVector Position = Character->GetActorLocation();
    const FVector Velocity = Character->GetVelocity();
    const auto* Weapon = Character->Weapon.Get();
    const auto* State = GetPlayerState<AERPlayerState>();
    UE_LOG(LogTemp,Display,TEXT("ER_INPUT_AUDIT pawn=%s pos=%.1f,%.1f,%.1f vel=%.1f,%.1f crouch=%d half=%.1f camz=%.1f aim=%.1f,%.1f ammo=%d/%d reload=%d dead=%d protected=%d menu=%d scoreboard=%d kills=%d deaths=%d ctrl=%d can_crouch=%d"),
        *Character->GetName(),Position.X,Position.Y,Position.Z,Velocity.Size2D(),Velocity.Z,Character->bIsCrouched,
        Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),Character->Camera->GetComponentLocation().Z,
        GetControlRotation().Pitch,GetControlRotation().Yaw,Weapon->MagazineAmmo,Weapon->ReserveAmmo,Weapon->bReloading,
        Character->Health->bDead,Character->Health->IsProtected(),bMenuOpen,bShowScoreboard,State ? State->Kills : 0,State ? State->Deaths : 0,
        IsInputKeyDown(EKeys::LeftControl),Character->CanCrouch());
#endif
}
void AERPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (MenuWidget.IsValid() && GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
    if (FrameRateWidget.IsValid() && GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FrameRateWidget.ToSharedRef());
    FrameRateWidget.Reset(); DisplaySizeSelector.Reset();
    if (IsLocalController() && GetLocalPlayer()) if (auto* Subsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) if (GameplayMapping) Subsystem->RemoveMappingContext(GameplayMapping);
    MenuWidget.Reset(); Super::EndPlay(Reason);
}
void AERPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AERPlayerController::SelectPrimary);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AERPlayerController::SelectSidearm);
    InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AERPlayerController::SelectHE);
    InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AERPlayerController::SelectSmoke);
    InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AERPlayerController::SelectStun);
    InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AERPlayerController::NextWeapon);
    InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AERPlayerController::PreviousWeapon);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AERPlayerController::ZoomPressed);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AERPlayerController::ZoomReleased);
    if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (WalkAction)
        {
            Input->BindAction(WalkAction, ETriggerEvent::Started, this, &AERPlayerController::WalkPressed);
            Input->BindAction(WalkAction, ETriggerEvent::Completed, this, &AERPlayerController::WalkReleased);
            Input->BindAction(WalkAction, ETriggerEvent::Canceled, this, &AERPlayerController::WalkReleased);
        }
        Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AERPlayerController::Move);
        Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AERPlayerController::Look);
        Input->BindAction(FireAction, ETriggerEvent::Started, this, &AERPlayerController::FirePressed);
        Input->BindAction(FireAction, ETriggerEvent::Completed, this, &AERPlayerController::FireReleased);
        Input->BindAction(FireAction, ETriggerEvent::Canceled, this, &AERPlayerController::FireReleased);
        Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AERPlayerController::JumpPressed);
        Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AERPlayerController::JumpReleased);
        Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &AERPlayerController::JumpReleased);
        Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AERPlayerController::CrouchPressed);
        Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AERPlayerController::CrouchReleased);
        Input->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &AERPlayerController::CrouchReleased);
        Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &AERPlayerController::ReloadPressed);
        Input->BindAction(ScoreboardAction, ETriggerEvent::Started, this, &AERPlayerController::ScoreboardPressed);
        Input->BindAction(ScoreboardAction, ETriggerEvent::Completed, this, &AERPlayerController::ScoreboardReleased);
        Input->BindAction(ScoreboardAction, ETriggerEvent::Canceled, this, &AERPlayerController::ScoreboardReleased);
        Input->BindAction(MenuAction, ETriggerEvent::Started, this, &AERPlayerController::MenuPressed);
    }
}
void AERPlayerController::WalkPressed() { if (!bMenuOpen) if (auto* C = Cast<AERCharacter>(GetPawn())) Cast<UERMovement>(C->GetCharacterMovement())->bWalkHeld = true; }
void AERPlayerController::WalkReleased() { if (auto* C = Cast<AERCharacter>(GetPawn())) Cast<UERMovement>(C->GetCharacterMovement())->bWalkHeld = false; }
void AERPlayerController::Move(const FInputActionValue& Value)
{
    auto* Character = Cast<AERCharacter>(GetPawn()); if (!Character || Character->Health->bDead || bMenuOpen) return;
    const FVector2D Axis = Value.Get<FVector2D>(); const FRotator Rotation(0, GetControlRotation().Yaw, 0);
    Character->AddMovementInput(Rotation.Vector(), Axis.Y);
    Character->AddMovementInput(FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y), Axis.X);
}
void AERPlayerController::Look(const FInputActionValue& Value)
{
    if (bMenuOpen) return;
    const UERGameInstance* GI = GetGameInstance<UERGameInstance>();
    const FVector2D Input = Value.Get<FVector2D>();
    if (!FMath::IsFinite(Input.X) || !FMath::IsFinite(Input.Y)) return;
    const float Gain = ERMouseResponse::Gain(Input, FApp::GetDeltaTime(), GI ? GI->GetMouseAcceleration() : .5f);
    const FVector2D Axis = Input * Gain * (GI ? GI->GetMouseLookMultiplier() : 0.16f / 0.18f);
    AddYawInput(Axis.X); AddPitchInput(-Axis.Y);
#if !UE_BUILD_SHIPPING
    if (bInputAudit && FApp::GetCurrentTime() >= NextMouseAuditAt)
    {
        NextMouseAuditAt = FApp::GetCurrentTime() + .1;
        UE_LOG(LogTemp, Display, TEXT("ER_MOUSE_LOOK input=%.3f,%.3f dt=%.5f gain=%.3f output=%.3f,%.3f"),
            Input.X, Input.Y, FApp::GetDeltaTime(), Gain, Axis.X, Axis.Y);
    }
#endif
}
void AERPlayerController::FirePressed()
{
    if (bMenuOpen) return;
    // Started fires once per physical press. Returning here consumes the respawn click.
    if (bAwaitingRespawn) { ServerRequestRespawn(); return; }
    if (auto* Character = Cast<AERCharacter>(GetPawn())) Character->ServerStartFire();
}
void AERPlayerController::FireReleased() { if (auto* Character = Cast<AERCharacter>(GetPawn())) Character->ServerStopFire(); }
void AERPlayerController::JumpPressed()
{
    if (bMenuOpen) return;
    if (bAwaitingRespawn) { ServerRequestRespawn(); return; }
    if (auto* Character = Cast<AERCharacter>(GetPawn()); Character && !Character->Health->bDead) { Cast<UERMovement>(Character->GetCharacterMovement())->bClimbHeld = true; Character->Jump(); }
}
void AERPlayerController::JumpReleased() { if (auto* Character = Cast<AERCharacter>(GetPawn())) { Cast<UERMovement>(Character->GetCharacterMovement())->bClimbHeld = false; Character->StopJumping(); } }
void AERPlayerController::CrouchPressed() { if (!bMenuOpen) if (auto* Character = Cast<AERCharacter>(GetPawn()); Character && !Character->Health->bDead && !Cast<UERMovement>(Character->GetCharacterMovement())->IsTraversing()) Character->Crouch(); }
void AERPlayerController::CrouchReleased() { if (auto* Character = Cast<AERCharacter>(GetPawn())) Character->UnCrouch(); }
void AERPlayerController::ReloadPressed() { if (!bMenuOpen) if (auto* Character = Cast<AERCharacter>(GetPawn())) Character->ServerReload(); }
void AERPlayerController::ScoreboardPressed() { bShowScoreboard = true; }
void AERPlayerController::ScoreboardReleased() { bShowScoreboard = false; }
void AERPlayerController::MenuPressed() { if (!IsMenuLevel()) ShowMenu(!bMenuOpen); }
void AERPlayerController::SelectSlot(int32 Slot)
{
    if (bMenuOpen) return;
    bZoomHeld = false;
    if (auto* C = Cast<AERCharacter>(GetPawn())) C->ServerSelectWeapon(Slot);
}
void AERPlayerController::NextWeapon()
{
    ScrollWeapon(1);
}
void AERPlayerController::PreviousWeapon()
{
    ScrollWeapon(-1);
}
void AERPlayerController::ScrollWeapon(int32 Direction)
{
    auto* C = Cast<AERCharacter>(GetPawn());
    if (bMenuOpen || !C || C->Health->bDead || !C->Weapon || C->Inventory.IsEmpty() ||
        (Direction != 1 && Direction != -1)) return;
    const int32 Current = C->Inventory.IndexOfByKey(C->Weapon);
    if (Current == INDEX_NONE) return;
    // One immediate step per gesture. Every momentum pulse extends the quiet
    // period, so a continuous wheel/trackpad burst never queues more switches.
    const double Now = FPlatformTime::Seconds();
    const bool bFreshGesture = Now >= WheelQuietUntil;
    WheelQuietUntil = Now + .25;
    if (!bFreshGesture) return;
    SelectSlot((Current + Direction + C->Inventory.Num()) % C->Inventory.Num());
}
void AERPlayerController::ServerSetSkins_Implementation(int32 Alpha, int32 Omega)
{
    if (Alpha < 0 || Alpha >= ERSkins::Count || Omega < 0 || Omega >= ERSkins::Count) return;
    if (auto* PS = GetPlayerState<AERPlayerState>())
    { PS->AlphaSkin = Alpha; PS->OmegaSkin = Omega; PS->ForceNetUpdate(); }
}
void AERPlayerController::ServerSetPrimary_Implementation(int32 Index)
{
    if (Index < 0 || Index > 6 || !PrimaryOptions.IsValidIndex(Index)) return;
    if (auto* PS = GetPlayerState<AERPlayerState>()) { PS->PrimaryWeapon = Index; PS->ForceNetUpdate(); }
}
void AERPlayerController::ClientFlash_Implementation(float Strength) { FlashOpacity = FMath::Max(FlashOpacity, FMath::Clamp(Strength, 0.f, .9f)); }
void AERPlayerController::ServerRequestRespawn_Implementation() { if (auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>()) Mode->RequestRespawn(this); }
void AERPlayerController::ServerSetName_Implementation(const FString& Name) { if (PlayerState) PlayerState->SetPlayerName(AERGameMode::SanitizeName(Name)); }
void AERPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME_CONDITION(AERPlayerController, bAwaitingRespawn, COND_OwnerOnly); DOREPLIFETIME_CONDITION(AERPlayerController, RespawnAllowedAt, COND_OwnerOnly);
}
void AERPlayerController::UpdateRenderResolution()
{
    const UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
    if (!Viewport || !Viewport->Viewport) return;
    const FIntPoint Size = Viewport->Viewport->GetSizeXY();
    if (Size.X <= 0 || Size.Y <= 0 || Size == LastRenderViewport) return;
    LastRenderViewport = Size;
    // Cap the scene AND temporal history, then spatially upscale to the window.
    // UI keeps the actual viewport resolution. Primary dynamic scaling can reduce
    // this scene output further when the GPU exceeds the 72 FPS budget.
    SceneOutputFraction = FMath::Min(1.f, FMath::Min(1920.f / Size.X, 1080.f / Size.Y));
    if (IConsoleVariable* Secondary = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SecondaryScreenPercentage.GameViewport")))
        Secondary->Set(SceneOutputFraction * 100.f, ECVF_SetByGameSetting);
    UE_LOG(LogTemp, Display, TEXT("ER_RENDER_OUTPUT viewport=%dx%d scene_max=%dx%d secondary=%.1f%%"),
        Size.X, Size.Y, FMath::RoundToInt(Size.X * SceneOutputFraction), FMath::RoundToInt(Size.Y * SceneOutputFraction), SceneOutputFraction * 100.f);
}

FText AERPlayerController::GetFrameRateText() const
{
    if (FrameTimeMs <= 0) return FText::FromString(TEXT("FPS —"));
    FDynamicResolutionStateInfos Info;
    GEngine->GetDynamicResolutionCurrentStateInfos(Info);
    const bool bDynamic = Info.Status == EDynamicResolutionStatus::Enabled;
    const float Primary = bDynamic ? Info.ResolutionFractionApproximations[GDynamicPrimaryResolutionFraction] : 1.f;
    const FIntPoint RenderSize(FMath::RoundToInt(LastRenderViewport.X * SceneOutputFraction * Primary),
        FMath::RoundToInt(LastRenderViewport.Y * SceneOutputFraction * Primary));
    return FText::FromString(FString::Printf(TEXT("%.0f FPS   %.1f ms   ·   3D %d × %d%s"),
        1000.f / FrameTimeMs, FrameTimeMs, RenderSize.X, RenderSize.Y, bDynamic ? TEXT(" auto") : TEXT("")));
}

void AERPlayerController::ResetRenderResolution()
{
    // Keep a single governor: Unreal already samples GPU timings and lowers
    // the primary resolution under load. Reset its history, not the window.
    if (GEngine->GetDynamicResolutionState())
        GEngine->GetDynamicResolutionState()->ResetHistory();
    LastRenderViewport = FIntPoint::ZeroValue;
    UpdateRenderResolution();
    UE_LOG(LogTemp, Display, TEXT("ER_RENDER_RESET dynamic resolution history reset; window unchanged"));
}

FIntPoint AERPlayerController::GetDisplayResolution() const
{
    FDisplayMetrics Metrics;
    FDisplayMetrics::RebuildDisplayMetrics(Metrics);
    if (GetWorld() && GetWorld()->GetGameViewport())
    {
        const TSharedPtr<SWindow> Window = GetWorld()->GetGameViewport()->GetWindow();
        if (Window.IsValid())
        {
            const FVector2D Center = Window->GetPositionInScreen() + Window->GetSizeInScreen() * .5f;
            for (const FMonitorInfo& Monitor : Metrics.MonitorInfo)
            {
                const FPlatformRect& Rect = Monitor.DisplayRect;
                if (Center.X >= Rect.Left && Center.X < Rect.Right && Center.Y >= Rect.Top && Center.Y < Rect.Bottom)
                    return FIntPoint(Rect.Right - Rect.Left, Rect.Bottom - Rect.Top);
            }
        }
    }
    return FIntPoint(FMath::Max(1, Metrics.PrimaryDisplayWidth), FMath::Max(1, Metrics.PrimaryDisplayHeight));
}

FText AERPlayerController::GetDisplaySizeText() const
{
    const UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
    const TSharedPtr<SWindow> Window = Viewport->GetWindow();
    if (Window.IsValid() && Window->GetWindowMode() != EWindowMode::Windowed)
        return FText::FromString(TEXT("Schermo intero"));
    const FIntPoint Resolution = Viewport->Viewport ? Viewport->Viewport->GetSizeXY() : GEngine->GetGameUserSettings()->GetScreenResolution();
    const FIntPoint Display = GetDisplayResolution();
    const int32 Percent = FMath::RoundToInt(100.f * FMath::Min(float(Resolution.X) / Display.X, float(Resolution.Y) / Display.Y));
    return FText::FromString(FString::Printf(TEXT("Finestra %d%%  ·  %d × %d"), Percent, Resolution.X, Resolution.Y));
}

void AERPlayerController::ApplyDisplaySize(int32 Percent)
{
    UGameUserSettings* Settings = GEngine->GetGameUserSettings();
    if (!Settings) return;
    const FIntPoint Display = GetDisplayResolution();
    const bool bFullscreen = Percent == 100;
    const int32 WindowPercent = FMath::Clamp(Percent, 50, 90);
    const FIntPoint Resolution = bFullscreen ? Display : FIntPoint(
        FMath::RoundToInt(Display.X * WindowPercent / 100.f), FMath::RoundToInt(Display.Y * WindowPercent / 100.f));
    Settings->SetFullscreenMode(bFullscreen ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
    Settings->SetScreenResolution(Resolution);
    // The local launcher supplies an initial size; menu choices must override it.
    Settings->ApplyResolutionSettings(false);
    Settings->ConfirmVideoMode();
    Settings->SaveSettings();
    if (DisplaySizeSelector.IsValid()) DisplaySizeSelector->SetIsOpen(false);
    UE_LOG(LogTemp, Display, TEXT("ER_DISPLAY size=%d%% resolution=%dx%d fullscreen=%d"), Percent, Resolution.X, Resolution.Y, bFullscreen);
}

void AERPlayerController::ShowMenu(bool bShow)
{
    if (!IsLocalController() || !GetWorld()->GetGameViewport()) return;
    FireReleased(); WalkReleased(); CrouchReleased(); JumpReleased(); bShowScoreboard = false;
    bZoomHeld = false;
    bMenuOpen = bShow; bShowMouseCursor = bShow;
    SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
    if (MenuWidget.IsValid()) { GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef()); MenuWidget.Reset(); }
    if (!bShow) { SetInputMode(FInputModeGameOnly()); return; }
    auto* GI = GetGameInstance<UERGameInstance>(); if (!GI) return;
    const bool bMain = IsMenuLevel();
    auto Button = [](const FString& Label, TFunction<FReply()> Action) -> TSharedRef<SWidget>
    {
        return SNew(SButton).ContentPadding(FMargin(16,12)).OnClicked_Lambda(MoveTemp(Action))[SNew(STextBlock).Text(FText::FromString(Label)).Justification(ETextJustify::Center)];
    };
    TSharedRef<SVerticalBox> Panel = SNew(SVerticalBox);
    TSharedPtr<SWidget> FocusWidget;
    Panel->AddSlot().AutoHeight().Padding(0,0,0,10)[SNew(STextBlock).Text(FText::FromString(TEXT("ECLIPSE RECOIL"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",30))];
    Panel->AddSlot().AutoHeight().Padding(0,0,0,20)[SNew(STextBlock).Text(FText::FromString(TEXT("ALPHA / OMEGA     •     TEAM DEATHMATCH")))];
    const auto* Catalog = SkinCatalog ? SkinCatalog->GetDefaultObject<AERCharacter>() : nullptr;
    for (bool Alpha : {true, false})
    {
        auto Label = [Catalog, Alpha](int32 Index)
        {
            if (Catalog)
            {
                const auto& Options = Alpha ? Catalog->AlphaSkins : Catalog->OmegaSkins;
                if (Options.IsValidIndex(Index)) return Options[Index].DisplayName;
            }
            return FString::Printf(TEXT("Variante %d"), Index + 1);
        };
        TSharedRef<SVerticalBox> SkinChoices = SNew(SVerticalBox);
        for (int32 Index = 0; Index < ERSkins::Count; ++Index)
            SkinChoices->AddSlot().AutoHeight()[Button(Label(Index), [this, GI, Alpha, Index]()
            {
                (Alpha ? GI->AlphaSkin : GI->OmegaSkin) = Index;
                GI->SaveSkins(); ServerSetSkins(GI->AlphaSkin, GI->OmegaSkin);
                FSlateApplication::Get().DismissAllMenus();
                return FReply::Handled();
            })];
        Panel->AddSlot().AutoHeight().Padding(0,8)[SNew(STextBlock)
            .ColorAndOpacity(Alpha ? FLinearColor(.4f,.7f,1) : FLinearColor(1,.55f,.25f))
            .Text(FText::FromString(Alpha ? TEXT("Personaggio Alpha · blu / freddo") : TEXT("Personaggio Omega · arancio / caldo")))];
        Panel->AddSlot().AutoHeight()[SNew(SComboButton)
            .ButtonContent()[SNew(STextBlock).Text_Lambda([GI, Alpha, Label]() { return FText::FromString(Label(Alpha ? GI->AlphaSkin : GI->OmegaSkin)); })]
            .MenuContent()[SkinChoices]];
    }
    Panel->AddSlot().AutoHeight().Padding(0,5)[SNew(STextBlock).AutoWrapText(true)
        .Text(FText::FromString(bMain ? TEXT("Scegli un aspetto per ogni squadra. Il team viene assegnato automaticamente.") : TEXT("Le modifiche al personaggio si applicano dalla prossima nascita.")))];
    if (bMain)
    {
        Panel->AddSlot().AutoHeight().Padding(0,5)[SNew(STextBlock).Text(FText::FromString(TEXT("Nome giocatore")))];
        Panel->AddSlot().AutoHeight().Padding(0,5)[SAssignNew(NameBox, SEditableTextBox).Text(FText::FromString(GI->LocalName))];
        FocusWidget = NameBox;
        Panel->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("OSPITA PARTITA LAN"), [this, GI]() { GI->LocalName = NameBox->GetText().ToString(); GI->Host(); return FReply::Handled(); })];
        Panel->AddSlot().AutoHeight().Padding(0,5)[Button(TEXT("ALLENAMENTO CONTRO BOT"), [this, GI]() { GI->LocalName = NameBox->GetText().ToString(); GI->Host(1); return FReply::Handled(); })];
        Panel->AddSlot().AutoHeight().Padding(0,5)[Button(TEXT("BERSAGLI FERMI · 5 ALPHA / 5 OMEGA"), [this, GI]() { GI->LocalName = NameBox->GetText().ToString(); GI->Host(2 * ERSkins::Count,true); return FReply::Handled(); })];
        Panel->AddSlot().AutoHeight().Padding(0,5)[Button(TEXT("LABORATORIO MOVIMENTO"), [this, GI]() { GI->LocalName = NameBox->GetText().ToString(); GI->GetWorld()->ServerTravel(TEXT("/Game/EclipseRecoil/Maps/L_MovementLab?listen?Name=") + AERGameMode::SanitizeName(GI->LocalName) + FString::Printf(TEXT("?Primary=%d?AlphaSkin=%d?OmegaSkin=%d"), GI->PrimaryWeapon, GI->AlphaSkin, GI->OmegaSkin)); return FReply::Handled(); })];
        Panel->AddSlot().AutoHeight().Padding(0,5)[SAssignNew(AddressBox, SEditableTextBox).Text(FText::FromString(TEXT("127.0.0.1:7777"))).HintText(FText::FromString(TEXT("IP dell'host")))];
        Panel->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("CONNETTI"), [this, GI]() { GI->LocalName = NameBox->GetText().ToString(); GI->Join(AddressBox->GetText().ToString()); return FReply::Handled(); })];
    }
    else
    {
        FocusWidget = Button(TEXT("RIPRENDI"), [this]() { ShowMenu(false); return FReply::Handled(); });
        Panel->AddSlot().AutoHeight().Padding(0,10)[FocusWidget.ToSharedRef()];
        Panel->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("TORNA AL MENU"), [GI]() { GI->ReturnToMenu(); return FReply::Handled(); })];
    }
    if (!PrimaryOptions.IsEmpty())
    {
        TSharedRef<SVerticalBox> Choices = SNew(SVerticalBox);
        for (int32 Index = 0; Index < FMath::Min(7, PrimaryOptions.Num()); ++Index)
        {
            const FString Label = PrimaryOptions[Index]->DisplayName;
            Choices->AddSlot().AutoHeight()[Button(Label, [this, GI, Index]()
            {
                GI->PrimaryWeapon = Index; GI->SaveLoadout(); ServerSetPrimary(Index);
                FSlateApplication::Get().DismissAllMenus();
                return FReply::Handled();
            })];
        }
        Panel->AddSlot().AutoHeight().Padding(0,8)[SNew(STextBlock).Text(FText::FromString(bMain ? TEXT("Arma principale") : TEXT("Arma principale · dalla prossima nascita")))];
        Panel->AddSlot().AutoHeight()[SNew(SComboButton)
            .ButtonContent()[SNew(STextBlock).Text_Lambda([this, GI]()
            { return FText::FromString(PrimaryOptions.IsValidIndex(GI->PrimaryWeapon) ? PrimaryOptions[GI->PrimaryWeapon]->DisplayName : TEXT("AK74")); })]
            .MenuContent()[Choices]];
        Panel->AddSlot().AutoHeight().Padding(0,5)[SNew(STextBlock).Text(FText::FromString(TEXT("1 Principale · 2 M1911 · 3 HE · 4 Fumo · 5 Stun\nRotella cambia arma · Tasto destro zoom"))).AutoWrapText(true)];
    }
    TSharedRef<SVerticalBox> DisplayChoices = SNew(SVerticalBox);
    for (int32 Percent : {50, 60, 70, 80, 90, 100})
    {
        const FString Label = Percent == 100 ? TEXT("Schermo intero") : FString::Printf(TEXT("Finestra %d%%"), Percent);
        DisplayChoices->AddSlot().AutoHeight()[Button(Label, [this, Percent]() { ApplyDisplaySize(Percent); return FReply::Handled(); })];
    }
    Panel->AddSlot().AutoHeight().Padding(0,10,0,5)[SNew(STextBlock).Text(FText::FromString(TEXT("Dimensione finestra / schermo")))];
    Panel->AddSlot().AutoHeight().Padding(0,5)[SAssignNew(DisplaySizeSelector, SComboButton)
        .ButtonContent()[SNew(STextBlock).Text_Lambda([this]() { return GetDisplaySizeText(); })]
        .MenuContent()[DisplayChoices]];
    Panel->AddSlot().AutoHeight().Padding(0,8,0,3)[SNew(STextBlock)
        .Text(FText::FromString(TEXT("Risoluzione 3D automatica · obiettivo 60+ FPS"))).AutoWrapText(true)];
    Panel->AddSlot().AutoHeight().Padding(0,5)[Button(TEXT("REIMPOSTA RISOLUZIONE AUTOMATICA"), [this]() {
        ResetRenderResolution(); return FReply::Handled();
    })];
    Panel->AddSlot().AutoHeight().Padding(0,12,0,5)[SNew(STextBlock).Text_Lambda([GI]() {
        return FText::FromString(FString::Printf(TEXT("Sensibilità mouse · %d%%"), FMath::RoundToInt(GI->GetMouseSensitivity() * 100.f)));
    })];
    Panel->AddSlot().AutoHeight().Padding(0,5)[SNew(SSlider)
        .Value_Lambda([GI]() { return GI->GetMouseSensitivity(); })
        .StepSize(.01f)
        .OnValueChanged_Lambda([GI](float Value) { GI->SetMouseSensitivity(Value); })
        .OnMouseCaptureEnd_Lambda([GI]() { GI->SaveMouseSensitivity(); })
        .OnControllerCaptureEnd_Lambda([GI]() { GI->SaveMouseSensitivity(); })];
    Panel->AddSlot().AutoHeight().Padding(0,12,0,5)[SNew(STextBlock).Text_Lambda([GI]() {
        return FText::FromString(FString::Printf(TEXT("Accelerazione mouse · %d%%"), FMath::RoundToInt(GI->GetMouseAcceleration() * 100.f)));
    })];
    Panel->AddSlot().AutoHeight().Padding(0,5)[SNew(SSlider)
        .Value_Lambda([GI]() { return GI->GetMouseAcceleration(); })
        .StepSize(.01f)
        .OnValueChanged_Lambda([GI](float Value) { GI->SetMouseAcceleration(Value); })
        .OnMouseCaptureEnd_Lambda([GI]() { GI->SaveMouseSensitivity(); })
        .OnControllerCaptureEnd_Lambda([GI]() { GI->SaveMouseSensitivity(); })];
    Panel->AddSlot().AutoHeight().Padding(0,3)[SNew(STextBlock)
        .Text(FText::FromString(TEXT("Movimenti lenti precisi, movimenti rapidi più ampi. 0% = risposta uniforme."))).AutoWrapText(true)];
    Panel->AddSlot().AutoHeight().Padding(0,15)[SNew(STextBlock).Text_Lambda([GI]() { return FText::FromString(GI->LastError); }).AutoWrapText(true).ColorAndOpacity(FLinearColor(1,.6f,.25f))];
    Panel->AddSlot().AutoHeight().Padding(0,10)[SNew(STextBlock).Text(FText::FromString(TEXT("WASD  Movimento     Mouse  Mira / Fuoco\nShift  Cammina     Spazio  Salto / Arrampicata / Respawn     Ctrl / C  Accovacciati\nR  Ricarica     Tab  Classifica     Esc  Menu"))).AutoWrapText(true)];
    MenuWidget = SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(.015f,.025f,.045f,.96f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(470).MaxDesiredHeight_Lambda([this]() {
            const auto* Viewport = GetWorld()->GetGameViewport()->Viewport;
            return Viewport ? FMath::Max(200.f, float(Viewport->GetSizeXY().Y) - 48.f) : 600.f;
        })[SNew(SScrollBox) + SScrollBox::Slot()[Panel]]];
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 20);
    // GameAndUI lets Esc reach Enhanced Input while the menu owns the mouse.
    SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(FocusWidget).SetHideCursorDuringCapture(false));
}
void AERHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* PC = Cast<AERPlayerController>(PlayerOwner); if (!PC || PC->bMenuOpen || !Canvas) return;
    if (PC->FlashOpacity > 0)
    {
        FCanvasTileItem Flash(FVector2D::ZeroVector, GWhiteTexture, FVector2D(Canvas->ClipX, Canvas->ClipY), FLinearColor(1,1,1,PC->FlashOpacity));
        Flash.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Flash);
    }
    auto* GS = GetWorld()->GetGameState<AERGameState>(); if (!GS) return;
    const float W = Canvas->SizeX, H = Canvas->SizeY;
    auto Text = [this](const FString& Value, float X, float Y, FLinearColor Color = FLinearColor::White, float Scale = 1.3f) { DrawText(Value, Color, X, Y, nullptr, Scale); };
    const bool bLab = GetWorld()->GetMapName().Contains(TEXT("L_MovementLab"));
    const auto* Character = Cast<AERCharacter>(PC->GetPawn());
    DrawRect(FLinearColor(0.01f,0.02f,0.04f,.8f), W/2-220, 15, 440, 70);
    Text(bLab ? FString(TEXT("LAB MOVIMENTO  |  185 cm")) : FString::Printf(TEXT("ALPHA  %02d     /     %02d  OMEGA"), GS->AlphaScore, GS->OmegaScore), W/2-160, 25, FLinearColor::White, 1.5f);
    FString Status = TEXT("In attesa di giocatori");
    if (GS->IsMatchInProgress())
    {
        const int32 Seconds = FMath::Max(0, FMath::CeilToInt(GS->MatchEndServerTime - GS->GetServerWorldTimeSeconds()));
        Status = FString::Printf(TEXT("%02d:%02d    •    Primo a 50"), Seconds/60, Seconds%60);
    }
    else if (GS->GetMatchState() == MatchState::WaitingPostMatch)
        Status = GS->Winner == EERTeam::None ? TEXT("PAREGGIO • Nuova partita tra 10 secondi") : (GS->Winner == EERTeam::Alpha ? TEXT("ALPHA VINCE") : TEXT("OMEGA VINCE"));
    if (bLab && Character) Status = FString::Printf(TEXT("%.1f km/h   |   Quota piedi: %.0f cm"), Character->GetVelocity().Size2D()*.036f, Character->GetActorLocation().Z-Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-2.1f);
    Text(Status, W/2-150, 58);
    const auto* PS = PC->GetPlayerState<AERPlayerState>();
    Text(PS && PS->Team == EERTeam::Omega ? TEXT("OMEGA") : TEXT("ALPHA"), 30, 30, PS && PS->Team == EERTeam::Omega ? FLinearColor(1,.25f,.1f) : FLinearColor(.2f,.65f,1));
    if (bLab)
    {
        Text(TEXT("SHIFT: cammina    SPAZIO + avanti: arrampica"),30,105);
        Text(TEXT("30 cm: passo    80 cm: salita    150 cm: spalle"),30,128);
    }
    if (Character && !Character->Health->bDead)
    {
        DrawLine(W/2-8,H/2,W/2-3,H/2,FLinearColor::White); DrawLine(W/2+3,H/2,W/2+8,H/2,FLinearColor::White);
        DrawLine(W/2,H/2-8,W/2,H/2-3,FLinearColor::White); DrawLine(W/2,H/2+3,W/2,H/2+8,FLinearColor::White);
        Text(FString::Printf(TEXT("HP  %.0f"), Character->Health->Health), 30, H-65, FLinearColor(.4f,1,.7f), 2.f);
        if (const auto* Weapon = Character->Weapon.Get())
        {
            Text(Weapon->bReloading ? TEXT("RICARICA...") : FString::Printf(TEXT("%02d / %02d    %s"),Weapon->MagazineAmmo,Weapon->ReserveAmmo,
                Weapon->Data ? *Weapon->Data->DisplayName : TEXT("Rifle A")),W-280,H-65,FLinearColor::White,1.7f);
            const int32 Capacity = Weapon->Data ? FMath::Clamp(Weapon->Data->MagazineSize, 1, 120) : 30;
            const int32 Remaining = FMath::Clamp(Weapon->MagazineAmmo, 0, Capacity);
            const float Scale = FMath::Clamp(H / 720.f, .65f, 2.f);
            const float HalfFov = FMath::DegreesToRadians(PC->PlayerCameraManager ? PC->PlayerCameraManager->GetFOVAngle() * .5f : 45.f);
            const float Spread = Weapon->GetSpreadDegrees();
            // Keep dots separated even with the M249's larger magazine.
            // Beyond that margin the radius follows the same cone used by the server.
            const float TargetRadius = FMath::Max(26.f, Capacity*.55f) * Scale + (W * .5f / FMath::Tan(HalfFov)) * FMath::Tan(FMath::DegreesToRadians(Spread));
            if (ReticleWeapon.Get() != Weapon) { ReticleWeapon = const_cast<AERWeapon*>(Weapon); ReticleRadius = TargetRadius; }
            ReticleRadius = FMath::FInterpTo(ReticleRadius, TargetRadius, GetWorld()->GetDeltaSeconds(), 18.f);
            const bool bUnavailable = Weapon->bReloading || Cast<UERMovement>(Character->GetCharacterMovement())->bHighClimb;
            const FLinearColor AmmoColor = bUnavailable ? FLinearColor(1.f,.7f,.25f,.8f) :
                (Remaining <= Capacity / 5 ? FLinearColor(1.f,.4f,.18f,.95f) : FLinearColor(.8f,.95f,1.f,.9f));
            for (int32 I = 0; I < Capacity; ++I)
            {
                const float Angle = -PI * .5f + (2.f * PI * I) / Capacity;
                const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
                const FVector2D Center = FVector2D(W*.5f,H*.5f) + Direction * ReticleRadius;
                const FLinearColor DotColor = I < Remaining ? FLinearColor(1,1,1,.70f) : FLinearColor(0,0,0,.10f);
                constexpr int32 DotSegments = 12;
                const float DotRadius = 1.5f * Scale;
                const float SolidRadius = DotRadius - .5f * Scale;
                TArray<FCanvasUVTri> DotTriangles;
                DotTriangles.Reserve(DotSegments * 3);
                const auto AddDotTriangle = [&DotTriangles](const FVector2D& A, const FLinearColor& ColorA,
                    const FVector2D& B, const FLinearColor& ColorB,
                    const FVector2D& C, const FLinearColor& ColorC)
                {
                    FCanvasUVTri Triangle;
                    Triangle.V0_Pos = A; Triangle.V0_Color = ColorA;
                    Triangle.V1_Pos = B; Triangle.V1_Color = ColorB;
                    Triangle.V2_Pos = C; Triangle.V2_Color = ColorC;
                    DotTriangles.Add(Triangle);
                };
                const FLinearColor TransparentDotColor(DotColor.R, DotColor.G, DotColor.B, 0.f);
                for (int32 Segment = 0; Segment < DotSegments; ++Segment)
                {
                    const float A = Segment * 2.f * PI / DotSegments, B = (Segment+1) * 2.f * PI / DotSegments;
                    const FVector2D InnerA = Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * SolidRadius;
                    const FVector2D InnerB = Center + FVector2D(FMath::Cos(B), FMath::Sin(B)) * SolidRadius;
                    const FVector2D OuterA = Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * DotRadius;
                    const FVector2D OuterB = Center + FVector2D(FMath::Cos(B), FMath::Sin(B)) * DotRadius;

                    AddDotTriangle(Center, DotColor, InnerA, DotColor, InnerB, DotColor);
                    AddDotTriangle(InnerA, DotColor, OuterA, TransparentDotColor, OuterB, TransparentDotColor);
                    AddDotTriangle(InnerA, DotColor, OuterB, TransparentDotColor, InnerB, DotColor);
                }
                FCanvasTriangleItem Dot(DotTriangles, GWhiteTexture);
                Dot.BlendMode = SE_BLEND_Translucent;
                Canvas->DrawItem(Dot);
            }
            const FString CenterAmmo = Weapon->bReloading ? FString(TEXT("R")) : FString::FromInt(Remaining);
            float TextW = 0, TextH = 0;
            GetTextSize(CenterAmmo, TextW, TextH, nullptr, .9f*Scale);
            Text(CenterAmmo,W*.5f-TextW*.5f,H*.5f+ReticleRadius+10.f*Scale,AmmoColor,.9f*Scale);
#if !UE_BUILD_SHIPPING
            if (FParse::Param(FCommandLine::Get(), TEXT("ERAccuracyAudit")) && GetWorld()->GetTimeSeconds() >= NextReticleAuditAt)
            {
                NextReticleAuditAt = GetWorld()->GetTimeSeconds() + .1;
                UE_LOG(LogTemp,Display,TEXT("ER_RETICLE ammo=%d ticks=%d capacity=%d spread=%.3f bloom=%.3f radius=%.1f reload=%d"),
                    Weapon->MagazineAmmo,Remaining,Capacity,Spread,Weapon->GetBloomDegrees(),ReticleRadius,Weapon->bReloading);
            }
#endif
        }
        if (Character->Health->IsProtected()) Text(TEXT("PROTEZIONE ALLA NASCITA"), W/2-145,H-100,FLinearColor(.4f,.8f,1));
    }
    if (PC->bAwaitingRespawn)
    {
        const int32 Seconds = FMath::Max(0, FMath::CeilToInt(PC->RespawnAllowedAt-GS->GetServerWorldTimeSeconds()));
        Text(Seconds > 0 ? FString::Printf(TEXT("Rientro disponibile tra %d"),Seconds) : TEXT("Premi sparo o salto per rientrare"),W/2-175,H/2+60,FLinearColor(1,.6f,.3f),1.6f);
    }
    Text(TEXT("TAB  Classifica     ESC  Menu"),30,H-25,FLinearColor(.6f,.65f,.7f),1.f);
    if (PC->bShowScoreboard || GS->GetMatchState() == MatchState::WaitingPostMatch)
    {
        DrawRect(FLinearColor(.015f,.025f,.04f,.92f),W/2-340,130,680,240);
        Text(TEXT("SQUADRA       GIOCATORE                   K     D     TK"), W/2-320,150);
        float Y = 190;
        for (APlayerState* Item : GS->PlayerArray) if (const auto* State = Cast<AERPlayerState>(Item))
        {
            Text(FString::Printf(TEXT("%-10s    %-24s   %d     %d     %d"), State->Team == EERTeam::Alpha ? TEXT("Alpha") : TEXT("Omega"),*State->GetPlayerName(), State->Kills, State->Deaths, State->TeamKills),W/2-320,Y); Y += 36;
        }
    }
}
