#include "ERPlayerController.h"
#include "ERRecon.h"
#include "ERMapCatalog.h"
#include "Internationalization/Internationalization.h"
#include "ERMouseResponse.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SScaleBox.h"
#include "ERCharacter.h"
#include "ERMovement.h"
#include "ERMatch.h"
#include "ERWeapon.h"
#include "ERBlockRoom.h"
#include "ERBlockNetworkProbe.h"
#include "ERBlockTarget.h"
#include "ERProceduralMap.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "GenericPlatform/GenericApplication.h"
#include "UnrealClient.h"
#include "DynamicResolutionState.h"
#include "ERResolutionPolicy.h"
#include "ERLadder.h"
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

#if PLATFORM_MAC
void ERSetMacGameIcon(const FString& IconPath);
#endif

void UERGameInstance::Init()
{
    Super::Init();
    if (!FFileHelper::LoadFileToString(BuildVersion, *(FPaths::ProjectDir() / TEXT("Build/GameVersion.txt"))))
        BuildVersion = TEXT("unversioned");
    BuildVersion.TrimStartAndEndInline();
    UE_LOG(LogTemp, Display, TEXT("ER_BUILD_VERSION %s"), *BuildVersion);
    FInternationalization::Get().SetCurrentLanguage(TEXT("en"));
#if PLATFORM_MAC
    // Local -game runs use the Editor executable, so also set this process's Dock icon.
    if (!GIsEditor && !IsRunningDedicatedServer())
        ERSetMacGameIcon(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Branding/icon.png")));
#endif
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
void UERGameInstance::StartSession()
{
    if (SessionMode == 2) { Join(ServerAddress); return; }
    LastError.Empty();
    const auto& Map=ERMaps::At(MapChoice);
    const TCHAR* Behaviors[] = {TEXT("Target"), TEXT("Movement"), TEXT("Patrol")};
    FString Options = SessionMode == 1 ? TEXT("listen?Name=") : TEXT("Name=");
    Options += AERGameMode::SanitizeName(LocalName);
    Options += FString::Printf(TEXT("?Primary=%d?AlphaSkin=%d?OmegaSkin=%d?Team=%d?ERAlphaBots=%d?EROmegaBots=%d?ERBotMode=%s"),
        PrimaryWeapon, AlphaSkin, OmegaSkin, FMath::Clamp(PlayerTeam,0,1), FMath::Clamp(AlphaBots,0,10), FMath::Clamp(OmegaBots,0,10), Behaviors[FMath::Clamp(BotBehavior,0,2)]);
    if(Map.Procedural)Options+=FString::Printf(TEXT("?MapSeed=%d"),FMath::Rand());
    UGameplayStatics::OpenLevel(GetWorld(), FName(Map.Package), true, Options);
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
    if (!IsValidAddress(Address)) { LastError = TEXT("Enter a valid IPv4 address, e.g. 192.168.1.20:7777."); return false; }
    if (APlayerController* PC = GetFirstLocalPlayerController())
    {
        LastError = TEXT("Connecting...");
        PC->ClientTravel(Address + TEXT("?Name=") + AERGameMode::SanitizeName(LocalName) + FString::Printf(TEXT("?Primary=%d?AlphaSkin=%d?OmegaSkin=%d?Team=%d"), PrimaryWeapon, AlphaSkin, OmegaSkin, FMath::Clamp(PlayerTeam,0,1)), TRAVEL_Absolute); return true;
    }
    LastError = TEXT("Local player unavailable."); return false;
}
void UERGameInstance::ReturnToMenu() { UGameplayStatics::OpenLevel(GetWorld(), TEXT("/Game/EclipseRecoil/Maps/L_MainMenu")); }
void UERGameInstance::NetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error)
{
    if (World != GetWorld()) return;
    LastError = TEXT("Connection lost or failed: ") + Error.Left(180);
    ReturnToMenu();
}
void UERGameInstance::TravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error)
{
    if (World != GetWorld()) return;
    LastError = TEXT("Travel failed: ") + Error.Left(180);
    if (!UGameplayStatics::GetCurrentLevelName(World).Contains(TEXT("L_MainMenu"))) ReturnToMenu();
}
bool AERPlayerController::IsMenuLevel() const { return UGameplayStatics::GetCurrentLevelName(GetWorld()).Contains(TEXT("L_MainMenu")); }
AERPlayerController::AERPlayerController()
{
    Recon = CreateDefaultSubobject<UERReconComponent>(TEXT("Recon"));
}
void AERPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) return;
#if !UE_BUILD_SHIPPING
    bInputAudit = FParse::Param(FCommandLine::Get(), TEXT("ERInputAudit"));
    bPerformanceAudit = FParse::Param(FCommandLine::Get(), TEXT("ERPerformanceAudit"));
#endif
    if (auto* GI = GetGameInstance<UERGameInstance>())
    {
        ServerSetName(GI->LocalName);
        if (!GIsEditor && IsMenuLevel() && !GI->bInitialWindowSizeApplied &&
            !FParse::Param(FCommandLine::Get(), TEXT("nullrhi")))
        {
            GI->bInitialWindowSizeApplied = true;
            int32 Percent = 70;
            int32 ExplicitSize = 0;
            const bool bRequestedPercent = FParse::Value(FCommandLine::Get(), TEXT("ERInitialWindowPercent="), Percent);
            const bool bExplicitResolution = FParse::Value(FCommandLine::Get(), TEXT("ResX="), ExplicitSize) ||
                FParse::Value(FCommandLine::Get(), TEXT("ResY="), ExplicitSize);
            // Apply once after the native window exists, overriding stale saved sizes.
            // Explicit capture/probe resolutions still work without the launcher flag.
            if (bRequestedPercent || !bExplicitResolution) ApplyDisplaySize(FMath::Clamp(Percent, 50, 90));
        }
    }
    if (ULocalPlayer* LP = GetLocalPlayer()) if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) if (GameplayMapping) Subsystem->AddMappingContext(GameplayMapping, 0);
    ShowMenu(IsMenuLevel());
    ERInstallResolutionGovernor();
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
    UpdateBlockMapLoading();
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
    InputComponent->BindKey(EKeys::W, IE_Pressed, this, &AERPlayerController::ForwardPressed);
    InputComponent->BindKey(EKeys::W, IE_Released, this, &AERPlayerController::ForwardReleased);
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AERPlayerController::SelectPrimary);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AERPlayerController::SelectSidearm);
    InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AERPlayerController::SelectHE);
    InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AERPlayerController::SelectSmoke);
    InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AERPlayerController::SelectStun);
    InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AERPlayerController::SelectSix);
    InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AERPlayerController::SelectSeven);
    InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AERPlayerController::SelectEight);
    InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AERPlayerController::SelectReferenceHE);
    InputComponent->BindKey(EKeys::G, IE_Pressed, this, &AERPlayerController::SelectReferenceSmoke);
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
void AERPlayerController::ForwardPressed()
{
    auto* C = Cast<AERCharacter>(GetPawn());
    if (bMenuOpen || !C || C->Health->bDead) return;
    if (GesturePawn.Get() != C) { GesturePawn = C; LastForwardTap = LastCrouchTap = -1; }
    const double Now = GetWorld()->GetTimeSeconds();
    if (LastForwardTap >= 0 && Now - LastForwardTap <= .3)
    {
        Cast<UERMovement>(C->GetCharacterMovement())->bSprintHeld = true;
        LastForwardTap = -1;
    }
    else LastForwardTap = Now;
}
void AERPlayerController::ForwardReleased()
{
    if (auto* C = Cast<AERCharacter>(GetPawn())) Cast<UERMovement>(C->GetCharacterMovement())->bSprintHeld = false;
}
void AERPlayerController::WalkPressed() { if (!bMenuOpen) if (auto* C = Cast<AERCharacter>(GetPawn())) Cast<UERMovement>(C->GetCharacterMovement())->bWalkHeld = true; }
void AERPlayerController::WalkReleased() { if (auto* C = Cast<AERCharacter>(GetPawn())) Cast<UERMovement>(C->GetCharacterMovement())->bWalkHeld = false; }
void AERPlayerController::Move(const FInputActionValue& Value)
{
    auto* Character = Cast<AERCharacter>(GetPawn()); if (!Character || Character->Health->bDead || bMenuOpen) return;
    const FVector2D Axis = Value.Get<FVector2D>(); const FRotator Rotation(0, GetControlRotation().Yaw, 0);
    if(auto* M=Cast<UERMovement>(Character->GetCharacterMovement());M->IsOnLadder())
    {
        if(auto* L=M->FindLadder())Character->AddMovementInput(-L->GetActorForwardVector(),Axis.Y);
        return;
    }
    Character->AddMovementInput(Rotation.Vector(), Axis.Y);
    Character->AddMovementInput(FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y), Axis.X);
}
void AERPlayerController::ZoomPressed()
{
    if (!bMenuOpen) if (auto* C = Cast<AERCharacter>(GetPawn()); C && C->Weapon) C->Weapon->ServerCycleScope();
}
void AERPlayerController::Look(const FInputActionValue& Value)
{
    if (bMenuOpen) return;
    const UERGameInstance* GI = GetGameInstance<UERGameInstance>();
    const FVector2D Input = Value.Get<FVector2D>();
    if (!FMath::IsFinite(Input.X) || !FMath::IsFinite(Input.Y)) return;
    const float Gain = ERMouseResponse::Gain(Input, FApp::GetDeltaTime(), GI ? GI->GetMouseAcceleration() : .5f);
    const auto* C = Cast<AERCharacter>(GetPawn());
    const float ZoomGain = C && C->Weapon && !C->IsWeaponStowed() ? (C->Weapon->Data && C->Weapon->Data->ScopeMode==1 ? FMath::Lerp(90.f,C->Weapon->Data->ZoomFOV,C->GetIronAimBlend()) : C->Weapon->GetScopeFOV())/90.f : 1.f;
    const FVector2D Axis = Input * Gain * (GI ? GI->GetMouseLookMultiplier() : 0.16f / 0.18f) * ZoomGain;
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
    if (auto* Character = Cast<AERCharacter>(GetPawn())) { Character->SetGrenadeInputHeld(true);Character->ServerStartFire(); }
}
void AERPlayerController::FireReleased() { if (auto* Character = Cast<AERCharacter>(GetPawn())) { Character->SetGrenadeInputHeld(false);Character->ServerStopFire(); } }
void AERPlayerController::JumpPressed()
{
    if (bMenuOpen) return;
    if (bAwaitingRespawn) { ServerRequestRespawn(); return; }
    if (auto* Character = Cast<AERCharacter>(GetPawn()); Character && !Character->Health->bDead) { Cast<UERMovement>(Character->GetCharacterMovement())->bClimbHeld = true; Character->Jump(); }
}
void AERPlayerController::JumpReleased() { if (auto* Character = Cast<AERCharacter>(GetPawn())) { Cast<UERMovement>(Character->GetCharacterMovement())->bClimbHeld = false; Character->StopJumping(); } }
void AERPlayerController::CrouchPressed()
{
    auto* C = Cast<AERCharacter>(GetPawn());
    if (bMenuOpen || !C || C->Health->bDead) return;
    auto* M = Cast<UERMovement>(C->GetCharacterMovement());
    if (M->IsTraversing() || !M->IsMovingOnGround()) return;
    if (GesturePawn.Get() != C) { GesturePawn = C; LastForwardTap = LastCrouchTap = -1; }
    const double Now = GetWorld()->GetTimeSeconds();
    if (M->bProneWanted)
    {
        M->bProneWanted = false; C->Crouch(); LastCrouchTap = -1;
    }
    else if (LastCrouchTap >= 0 && Now - LastCrouchTap <= .3)
    {
        M->bProneWanted = true; C->UnCrouch(); LastCrouchTap = -1;
    }
    else { C->Crouch(); LastCrouchTap = Now; }
    M->bSprintHeld = false;
}
void AERPlayerController::CrouchReleased() { if (auto* C = Cast<AERCharacter>(GetPawn())) C->UnCrouch(); }
void AERPlayerController::ReloadPressed() { if (!bMenuOpen) if (auto* Character = Cast<AERCharacter>(GetPawn())) Character->ServerReload(); }
void AERPlayerController::ScoreboardPressed() { bShowScoreboard = true; }
void AERPlayerController::ScoreboardReleased() { bShowScoreboard = false; }
void AERPlayerController::MenuPressed() { if (!IsMenuLevel()) ShowMenu(!bMenuOpen); }
void AERPlayerController::SelectSlot(int32 Slot)
{
    if (bMenuOpen) return;
    if (auto* C = Cast<AERCharacter>(GetPawn()); C && C->Weapon) C->Weapon->ServerClearScope();
    if (auto* C = Cast<AERCharacter>(GetPawn())) C->ServerSelectWeapon(Slot);
}
void AERPlayerController::SelectNumber(int32 Index)
{
    if (const auto* C = Cast<AERCharacter>(GetPawn()); C && (C->UsesReferenceArsenal() || Index < 5)) SelectSlot(Index);
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
    // this scene output further when the GPU exceeds the 60 FPS budget.
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
        return FText::FromString(TEXT("Fullscreen"));
    const FIntPoint Resolution = Viewport->Viewport ? Viewport->Viewport->GetSizeXY() : GEngine->GetGameUserSettings()->GetScreenResolution();
    const FIntPoint Display = GetDisplayResolution();
    const int32 Percent = FMath::RoundToInt(100.f * FMath::Min(float(Resolution.X) / Display.X, float(Resolution.Y) / Display.Y));
    return FText::FromString(FString::Printf(TEXT("Windowed %d%%  ·  %d × %d"), Percent, Resolution.X, Resolution.Y));
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
    ForwardReleased(); LastForwardTap = LastCrouchTap = -1;
    FireReleased(); WalkReleased(); CrouchReleased(); JumpReleased(); bShowScoreboard = false;
    if (auto* C = Cast<AERCharacter>(GetPawn()); C && C->Weapon) C->Weapon->ServerClearScope();
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
    TSharedRef<SVerticalBox> GamePanel = SNew(SVerticalBox);
    TSharedRef<SVerticalBox> SetupPanel = SNew(SVerticalBox);
    auto Heading = [](const TCHAR* Text) { return SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Bold",22)); };
    GamePanel->AddSlot().AutoHeight().Padding(0,0,0,12)[Heading(TEXT("MATCH"))];
    Panel->AddSlot().AutoHeight().Padding(0,0,0,12)[Heading(TEXT("CHARACTER & WEAPONS"))];
    SetupPanel->AddSlot().AutoHeight().Padding(0,0,0,12)[Heading(TEXT("SETTINGS"))];
    TSharedPtr<SWidget> FocusWidget;
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
            return FString::Printf(TEXT("Variant %d"), Index + 1);
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
            .Text(FText::FromString(Alpha ? TEXT("Alpha character · blue / cool") : TEXT("Omega character · orange / warm")))];
        Panel->AddSlot().AutoHeight()[SNew(SComboButton)
            .ButtonContent()[SNew(STextBlock).Text_Lambda([GI, Alpha, Label]() { return FText::FromString(Label(Alpha ? GI->AlphaSkin : GI->OmegaSkin)); })]
            .MenuContent()[SNew(SBox).MaxDesiredHeight(400)[SNew(SScrollBox) + SScrollBox::Slot()[SkinChoices]]]];
    }
    Panel->AddSlot().AutoHeight().Padding(0,5)[SNew(STextBlock).AutoWrapText(true)
        .Text(FText::FromString(bMain ? TEXT("Choose a skin for each team and the team you want to play on.") : TEXT("Character changes apply on your next spawn.")))];
    if (bMain)
    {
        auto Choice = [Button](TSharedRef<SVerticalBox> Parent, const TCHAR* Title, TArray<FString> Labels, int32* Value, TFunction<bool()> Enabled)
        {
            Parent->AddSlot().AutoHeight().Padding(0,8,0,4)[SNew(STextBlock).Text(FText::FromString(Title))];
            TSharedRef<SVerticalBox> Items = SNew(SVerticalBox);
            for (int32 I=0; I<Labels.Num(); ++I)
                Items->AddSlot().AutoHeight()[Button(Labels[I], [Value,I]() { *Value=I; FSlateApplication::Get().DismissAllMenus(); return FReply::Handled(); })];
            Parent->AddSlot().AutoHeight()[SNew(SComboButton).IsEnabled_Lambda(MoveTemp(Enabled))
                .ButtonContent()[SNew(STextBlock).Text_Lambda([Labels,Value]() { return FText::FromString(Labels[FMath::Clamp(*Value,0,Labels.Num()-1)]); })]
                .MenuContent()[Items]];
        };
        auto Always = []() { return true; };
        auto OwnGame = [GI]() { return GI->SessionMode != 2; };
        GamePanel->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Player name")))];
        GamePanel->AddSlot().AutoHeight().Padding(0,4)[SAssignNew(NameBox, SEditableTextBox).Text(FText::FromString(GI->LocalName))];
        FocusWidget = NameBox;
        Choice(GamePanel,TEXT("Connection"),{TEXT("Local"),TEXT("Host and play"),TEXT("Join a server")},&GI->SessionMode,Always);
        Choice(GamePanel,TEXT("Map type"),ERMaps::Labels(),&GI->MapChoice,OwnGame);
        Choice(GamePanel,TEXT("Your team"),{TEXT("Alpha"),TEXT("Omega")},&GI->PlayerTeam,Always);
        TArray<FString> Counts; for (int32 I=0; I<=10; ++I) Counts.Add(FString::FromInt(I));
        Choice(GamePanel,TEXT("Alpha bots"),Counts,&GI->AlphaBots,OwnGame);
        Choice(GamePanel,TEXT("Omega bots"),Counts,&GI->OmegaBots,OwnGame);
        Choice(GamePanel,TEXT("Bot behavior"),{TEXT("Stationary"),TEXT("Movement only"),TEXT("Movement and combat")},&GI->BotBehavior,OwnGame);
        GamePanel->AddSlot().AutoHeight().Padding(0,10)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([GI]() {
            return FText::FromString(GI->SessionMode==2 ? TEXT("The server selects the map and bots.") : GI->SessionMode==1 ? TEXT("Hosting on port 7777. The match can start with one player.") : TEXT("Industrial maps vary each match. Depot is available as a reference environment."));
        })];
        GamePanel->AddSlot().AutoHeight().Padding(0,4)[SAssignNew(AddressBox, SEditableTextBox)
            .IsEnabled_Lambda([GI]() { return GI->SessionMode==2; })
            .Text(FText::FromString(GI->ServerAddress)).HintText(FText::FromString(TEXT("Server IP:port")))];
        GamePanel->AddSlot().AutoHeight().Padding(0,12)[SNew(SButton).ContentPadding(FMargin(16,12))
            .OnClicked_Lambda([this,GI]() { GI->LocalName=NameBox->GetText().ToString(); GI->ServerAddress=AddressBox->GetText().ToString(); GI->StartSession(); return FReply::Handled(); })
            [SNew(STextBlock).Text_Lambda([GI]() { return FText::FromString(GI->SessionMode==2 ? TEXT("CONNECT") : TEXT("START MATCH")); })]];
    }
    else
    {
        FocusWidget = Button(TEXT("RESUME"), [this]() { ShowMenu(false); return FReply::Handled(); });
        GamePanel->AddSlot().AutoHeight().Padding(0,10)[FocusWidget.ToSharedRef()];
        GamePanel->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("MAIN MENU"), [GI]() { GI->ReturnToMenu(); return FReply::Handled(); })];
    }
    const auto* MenuPawn=Cast<AERCharacter>(GetPawn());
    const bool bReferenceMenu=MenuPawn ? MenuPawn->UsesReferenceArsenal() : bReferenceArsenalMenu;
    if (!PrimaryOptions.IsEmpty() && !bReferenceMenu)
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
        Panel->AddSlot().AutoHeight().Padding(0,8)[SNew(STextBlock).Text(FText::FromString(bMain ? TEXT("Primary weapon") : TEXT("Primary weapon · next spawn")))];
        Panel->AddSlot().AutoHeight()[SNew(SComboButton)
            .ButtonContent()[SNew(STextBlock).Text_Lambda([this, GI]()
            { return FText::FromString(PrimaryOptions.IsValidIndex(GI->PrimaryWeapon) ? PrimaryOptions[GI->PrimaryWeapon]->DisplayName : TEXT("Assault Rifle")); })]
            .MenuContent()[Choices]];
        Panel->AddSlot().AutoHeight().Padding(0,5)[SNew(STextBlock).Text(FText::FromString(TEXT("1 Primary · 2 Pistol · 3 HE Grenade · 4 Smoke Grenade · 5 Stun Grenade\nMouse wheel: switch weapon · Right mouse: zoom"))).AutoWrapText(true)];
    }
    else Panel->AddSlot().AutoHeight().Padding(0,8)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(
        TEXT("1 Knife · 2 Pistol · 3 SMG · 4 Shotgun\n5 Assault Rifle · 6 Sniper Rifle\n7 Marksman Rifle · 8 Grenade Launcher\nF HE Grenade · G Smoke Grenade\nMouse wheel: switch weapon · Right mouse: zoom")))];
    TSharedRef<SVerticalBox> DisplayChoices = SNew(SVerticalBox);
    for (int32 Percent : {50, 60, 70, 80, 90, 100})
    {
        const FString Label = Percent == 100 ? TEXT("Fullscreen") : FString::Printf(TEXT("Windowed %d%%"), Percent);
        DisplayChoices->AddSlot().AutoHeight()[Button(Label, [this, Percent]() { ApplyDisplaySize(Percent); return FReply::Handled(); })];
    }
    SetupPanel->AddSlot().AutoHeight().Padding(0,10,0,5)[SNew(STextBlock).Text(FText::FromString(TEXT("Window / display size")))];
    SetupPanel->AddSlot().AutoHeight().Padding(0,5)[SAssignNew(DisplaySizeSelector, SComboButton)
        .ButtonContent()[SNew(STextBlock).Text_Lambda([this]() { return GetDisplaySizeText(); })]
        .MenuContent()[DisplayChoices]];
    SetupPanel->AddSlot().AutoHeight().Padding(0,8,0,3)[SNew(STextBlock)
        .Text(FText::FromString(TEXT("Automatic 3D resolution · target 60 FPS · tolerance and delayed adjustment"))).AutoWrapText(true)];
    SetupPanel->AddSlot().AutoHeight().Padding(0,5)[Button(TEXT("RESET AUTOMATIC RESOLUTION"), [this]() {
        ResetRenderResolution(); return FReply::Handled();
    })];
    SetupPanel->AddSlot().AutoHeight().Padding(0,12,0,5)[SNew(STextBlock).Text_Lambda([GI]() {
        return FText::FromString(FString::Printf(TEXT("Mouse sensitivity · %d%%"), FMath::RoundToInt(GI->GetMouseSensitivity() * 100.f)));
    })];
    SetupPanel->AddSlot().AutoHeight().Padding(0,5)[SNew(SSlider)
        .Value_Lambda([GI]() { return GI->GetMouseSensitivity(); })
        .StepSize(.01f)
        .OnValueChanged_Lambda([GI](float Value) { GI->SetMouseSensitivity(Value); })
        .OnMouseCaptureEnd_Lambda([GI]() { GI->SaveMouseSensitivity(); })
        .OnControllerCaptureEnd_Lambda([GI]() { GI->SaveMouseSensitivity(); })];
    SetupPanel->AddSlot().AutoHeight().Padding(0,12,0,5)[SNew(STextBlock).Text_Lambda([GI]() {
        return FText::FromString(FString::Printf(TEXT("Mouse acceleration · %d%%"), FMath::RoundToInt(GI->GetMouseAcceleration() * 100.f)));
    })];
    SetupPanel->AddSlot().AutoHeight().Padding(0,5)[SNew(SSlider)
        .Value_Lambda([GI]() { return GI->GetMouseAcceleration(); })
        .StepSize(.01f)
        .OnValueChanged_Lambda([GI](float Value) { GI->SetMouseAcceleration(Value); })
        .OnMouseCaptureEnd_Lambda([GI]() { GI->SaveMouseSensitivity(); })
        .OnControllerCaptureEnd_Lambda([GI]() { GI->SaveMouseSensitivity(); })];
    SetupPanel->AddSlot().AutoHeight().Padding(0,3)[SNew(STextBlock)
        .Text(FText::FromString(TEXT("Precise slow movements, faster turns with quick movements. 0% = uniform response."))).AutoWrapText(true)];
    SetupPanel->AddSlot().AutoHeight().Padding(0,15)[SNew(STextBlock).Text_Lambda([GI]() { return FText::FromString(GI->LastError); }).AutoWrapText(true).ColorAndOpacity(FLinearColor(1,.6f,.25f))];
    SetupPanel->AddSlot().AutoHeight().Padding(0,10)[SNew(STextBlock).Text(FText::FromString(TEXT("WASD  Move     Mouse  Aim / Fire\nShift  Walk     Space  Jump / Climb / Respawn     Ctrl / C  Crouch\nDouble-tap W  Sprint (max 5 s / double recovery time)\nDouble-tap Ctrl / C  Prone\nW / S  Climb ladder (automatic weapon stow)     Space  Detach\nR  Reload     Tab  Scoreboard     Esc  Menu"))).AutoWrapText(true)];
    SetupPanel->AddSlot().AutoHeight().Padding(0,12)[SNew(STextBlock)
        .Text(FText::FromString(TEXT("Recon models: S9 drone by blockv_lb; Pelican Case Game Ready by polbrainstorm. CC BY 4.0. Adapted for the game.")))
        .ToolTipText(FText::FromString(TEXT("S9 drone: https://skfb.ly/6SSyD\nPelican Case: https://skfb.ly/6xwPJ\nLicense: https://creativecommons.org/licenses/by/4.0/\nChanges: static conversion, scale, closed lid and material adaptation.")))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular",10)).AutoWrapText(true)];
    auto Column = [](TSharedRef<SVerticalBox> Content) -> TSharedRef<SWidget>
    {
        return SNew(SBorder).Padding(20).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0,0,0,.76f))
            .ForegroundColor(FLinearColor(.95f,.95f,.95f,1))
            [SNew(SScrollBox) + SScrollBox::Slot()[Content]];
    };
    static const FSlateDynamicImageBrush BackgroundBrush(FName(*FPaths::ConvertRelativePathToFull(
        FPaths::ProjectDir() / TEXT("Branding/background.jpg"))), FVector2D(2560, 1441));
    MenuWidget = SNew(SOverlay)
        + SOverlay::Slot()
        [SNew(SScaleBox).Stretch(EStretch::ScaleToFill)
            .Clipping(EWidgetClipping::ClipToBounds)
            .Visibility(EVisibility::HitTestInvisible)
            [SNew(SImage).Image(&BackgroundBrush)]]
        + SOverlay::Slot().Padding(24,24,24,42)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.1f).Padding(0,0,8,0)[Column(GamePanel)]
            + SHorizontalBox::Slot().FillWidth(1).Padding(8,0)[Column(Panel)]
            + SHorizontalBox::Slot().FillWidth(1).Padding(8,0,0,0)[Column(SetupPanel)]]
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(16,8)
        [SNew(STextBlock)
            .Text(FText::FromString(GI->BuildVersion))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
            .ColorAndOpacity(FLinearColor(.85f,.85f,.85f,.9f))
            .ShadowOffset(FVector2D(1,1)).ShadowColorAndOpacity(FLinearColor::Black)
            .Visibility(EVisibility::HitTestInvisible)];
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 20);
    // GameAndUI lets Esc reach Enhanced Input while the menu owns the mouse.
    SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(FocusWidget).SetHideCursorDuringCapture(false));
}
void AERPlayerController::ERBlockDamage(float Amount)
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetPawn()) return;
    FVector Eye; FRotator Aim;
    GetPawn()->GetActorEyesViewPoint(Eye, Aim);
    if (auto* Character = Cast<AERCharacter>(GetPawn())) Eye = Character->Camera->GetComponentLocation();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ERBlockDebug), false, GetPawn());
    if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + Aim.Vector() * 10000, ECC_GameTraceChannel1, Params))
        if (auto* Room = Cast<AERBlockRoom>(Hit.GetActor())) Room->ApplyHitDamage(Hit, Amount);
#endif
}
void AERPlayerController::ERBlockDestroy()
{
    ERBlockDamage(TNumericLimits<float>::Max());
}
void AERPlayerController::ERBlockStats()
{
#if !UE_BUILD_SHIPPING
    for (TActorIterator<AERBlockRoom> It(GetWorld()); It; ++It)
    {
        const auto M=It->GetMemoryProfile();const auto& G=It->GenerationProfile;const auto& U=It->UpdateProfile;
        UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_PROFILE %s logical=%.3fms grouping=%.3fms clear=%.3fms add=%.3fms mapping=%.3fms update_clear=%.3fms update_add=%.3fms uploaded=%d tracked=%.2fMiB resident=%.2fMiB bodies=%d decals=%d emitters=%d bursts=%d support_bytes=%llu support_build=%.3fms support_peak=%.3fms support_steps=%d unsupported=%d components=%d net_epoch=%u net_sequence=%u net_pages=%d net_ready=%d net_bytes=%llu"),
            *It->GetStatistics(),G.LogicalMs,G.GroupingMs,G.ClearMs,G.AddMs,G.MappingMs,U.ClearMs,U.AddMs,U.Uploaded,double(M.TrackedBytes())/1048576,double(M.ResidentBytes)/1048576,M.CollisionBodies,M.Decals,M.Emitters,M.Bursts,uint64(M.SupportBytes),It->GetSupportBuildMs(),It->PeakSupportMs,It->SupportSteps,It->LastUnsupported,It->LastUnsupportedComponents,It->GetMapEpoch(),It->GetMapSequence(),It->GetNetworkPageCount(),It->IsMapReady(),uint64(M.NetworkBytes));
    }
#endif
}
void AERPlayerController::ERBlockRegenerate()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority()) return;
    for (TActorIterator<AERBlockRoom> It(GetWorld()); It; ++It)
    {
        if (!It->Regenerate()) continue;
        if (GetPawn())
        {
            GetPawn()->SetActorLocation(It->GetSpawnLocation(), false, nullptr, ETeleportType::TeleportPhysics);
            GetPawn()->GetMovementComponent()->StopMovementImmediately();
        }
    }
    for (TActorIterator<AERBlockTarget> It(GetWorld()); It; ++It) It->ResetHealth();
#endif
}
void AERPlayerController::ERMapGenerate(int32 Seed, int32 Rooms, int32 Floors)
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority()) return;
    for (TActorIterator<AERProceduralMap> It(GetWorld()); It; ++It)
    {
        const FERProceduralConfig Previous = It->LayoutConfig;
        It->LayoutConfig.Seed = Seed; It->LayoutConfig.RoomCount = Rooms;
        if(Floors!=0)It->LayoutConfig.Floors=Floors;
        if (!It->Regenerate())
        {
            It->LayoutConfig = Previous;
            UE_LOG(LogTemp, Warning, TEXT("ER_MAP_REJECTED invalid config or block budget; previous layout preserved"));
            continue;
        }
        if (GetPawn())
        {
            const auto* State=GetPlayerState<AERPlayerState>();
            const auto& Starts=State && State->Team==EERTeam::Omega ? It->GetLayout().OmegaSpawns:It->GetLayout().AlphaSpawns;
            const FVector Spawn=Starts.IsEmpty() ? It->GetSpawnLocation():It->GetActorTransform().TransformPosition(Starts[0].GetLocation());
            GetPawn()->SetActorLocation(Spawn, false, nullptr, ETeleportType::TeleportPhysics);
            GetPawn()->GetMovementComponent()->StopMovementImmediately();
        }
    }
#endif
}
void AERPlayerController::ERBlockShot(float Energy, float Damage)
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetPawn()) return;
    FVector Eye; FRotator Aim;
    GetPawn()->GetActorEyesViewPoint(Eye, Aim);
    if (auto* Character = Cast<AERCharacter>(GetPawn())) Eye = Character->Camera->GetComponentLocation();
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERBlockDebugShot), false, GetPawn());
    if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + Aim.Vector() * 10000, ECC_GameTraceChannel1, Query))
    {
        FERBlockShotSettings Settings; Settings.Energy = Energy;
        ERBlockCombat::ContinueShot(GetWorld(), Hit, Eye, Aim.Vector(), 10000, Damage, Settings, Query, this, GetPawn());
    }
#endif
}
void AERPlayerController::ERBlockExplosion(float Radius, float Damage)
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetPawn()) return;
    FVector Eye; FRotator Aim;
    GetPawn()->GetActorEyesViewPoint(Eye, Aim);
    if (auto* Character = Cast<AERCharacter>(GetPawn())) Eye = Character->Camera->GetComponentLocation();
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERBlockDebugBlast), false, GetPawn());
    if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + Aim.Vector() * 10000, ECC_GameTraceChannel1, Query))
        if (auto* Room = Cast<AERBlockRoom>(Hit.GetActor()))
        {
            const FVector Origin = Hit.ImpactPoint + Hit.ImpactNormal * 10;
            const auto Result = AERBlockRoom::ApplyWorldExplosion(GetWorld(), Origin, Radius, Damage, GetPawn());
            if (Result.Damaged > 0) Room->SpawnExplosionEffect(Origin);
        }
#endif
}
void AERPlayerController::ERBlockGrenade()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetPawn()) return;
    FVector Eye; FRotator Aim;
    GetPawn()->GetActorEyesViewPoint(Eye, Aim);
    if (auto* Character = Cast<AERCharacter>(GetPawn())) Eye = Character->Camera->GetComponentLocation();
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERBlockDebugGrenade), false, GetPawn());
    if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + Aim.Vector() * 10000, ECC_GameTraceChannel1, Query))
        if (auto* Room = Cast<AERBlockRoom>(Hit.GetActor()))
        {
            const FVector Origin = Hit.ImpactPoint + Hit.ImpactNormal * 10;
            const auto Result = AERBlockRoom::ApplyWorldExplosion(GetWorld(), Origin, FERBlockExplosionProfile(), GetPawn());
            if (Result.Damaged > 0) Room->SpawnExplosionEffect(Origin);
            UE_LOG(LogTemp, Display, TEXT("ER_BLOCK_GRENADE candidates=%d occluded=%d damaged=%d removed=%d rebuilt=%d query=%.2fms exposure=%.2fms mutation=%.2fms total=%.2fms"),
                Result.Candidates, Result.Occluded, Result.Damaged, Result.Destroyed, Result.GroupsRebuilt, Result.QueryMs, Result.ExposureMs, Result.MutationMs, Result.TotalMs);
        }
#endif
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
    for (TActorIterator<AERBlockRoom> It(GetWorld()); It; ++It)
    {
        if(PC->IsBlockMapLoading())Text(TEXT("Synchronizing map..."),30,83,FLinearColor::Yellow,1);
        if(const auto* Map=Cast<AERProceduralMap>(*It))if(Map->LayoutConfig.TemplateAsset && !FParse::Param(FCommandLine::Get(),TEXT("ERBlockDebug")))continue;
        Text(TEXT("DESTRUCTION LAB 5cm CELLS | ERBlockGrenade / ERBlockShot 100 36 / ERBlockDamage 25 / ERBlockCollapseDemo"), 30, 105, FLinearColor::Yellow, 1);
        if (const auto* Map=Cast<AERProceduralMap>(*It)) Text(Map->LayoutConfig.TemplateAsset ? TEXT("ERMapGenerate 42 8 1 | template seed"):TEXT("ERMapGenerate 42 6 3 | seed / rooms 6..10 / floors 1..3"), 30, 185, FLinearColor::Yellow, 1);
        Text(It->GetStatistics(), 30, 125, FLinearColor::White, .9f);
        Text(TEXT("Last block: ") + It->LastHit, 30, 145, FLinearColor::White, 1);
        Text(It->LastCombat, 30, 165, FLinearColor::White, 1);
    }
    const bool bLab = GetWorld()->GetMapName().Contains(TEXT("L_MovementLab"));
    const auto* Character = Cast<AERCharacter>(PC->GetPawn());
    if (const auto* Weapon = Character && !Character->Health->bDead && !Character->IsWeaponStowed() ? Character->Weapon.Get() : nullptr; Weapon && Weapon->HasOpticalScope() && Weapon->ScopeLevel)
    {
        const float R = FMath::Min(W,H)*.48f, CX=W*.5f, CY=H*.5f;
        // Opaque outside a circular lens, including ultrawide side bars.
        for (int32 Y=0; Y<FMath::CeilToInt(H); ++Y)
        {
            const float DY=Y+.5f-CY;
            const float X=FMath::Abs(DY)<R ? FMath::Sqrt(R*R-DY*DY) : 0.f;
            DrawRect(FLinearColor::Black,0,Y,CX-X,1);
            DrawRect(FLinearColor::Black,CX+X,Y,W-CX-X,1);
        }
        const float Blur=Weapon->GetScopeBlur();
        for(int32 I=-4; I<=4; ++I)
        {
            if(I && Blur<=0) continue;
            const float Offset=I*Blur*2.f;
            const FLinearColor Ink(0,0,0,I==0?1.f-Blur*.7f:Blur*.12f);
            DrawRect(Ink,CX-R,CY+Offset-.75f,2.f*R,1.5f);
            DrawRect(Ink,CX+Offset-.75f,CY-R,1.5f,2.f*R);
        }
    }

    DrawRect(FLinearColor(0.01f,0.02f,0.04f,.8f), W/2-220, 15, 440, 70);
    Text(bLab ? FString(TEXT("MOVEMENT LAB  |  185 cm")) : FString::Printf(TEXT("ALPHA  %02d     /     %02d  OMEGA"), GS->AlphaScore, GS->OmegaScore), W/2-160, 25, FLinearColor::White, 1.5f);
    FString Status = TEXT("Waiting for players");
    if (GS->IsMatchInProgress())
    {
        const int32 Seconds = FMath::Max(0, FMath::CeilToInt(GS->MatchEndServerTime - GS->GetServerWorldTimeSeconds()));
        Status = FString::Printf(TEXT("%02d:%02d    •    First to 50"), Seconds/60, Seconds%60);
    }
    else if (GS->GetMatchState() == MatchState::WaitingPostMatch)
        Status = GS->Winner == EERTeam::None ? TEXT("DRAW • Next match in 10 seconds") : (GS->Winner == EERTeam::Alpha ? TEXT("ALPHA WINS") : TEXT("OMEGA WINS"));
    if (bLab && Character) Status = FString::Printf(TEXT("%.1f km/h   |   Feet height: %.0f cm"), Character->GetVelocity().Size2D()*.036f, Character->GetActorLocation().Z-Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-2.1f);
    Text(Status, W/2-150, 58);
    const auto* PS = PC->GetPlayerState<AERPlayerState>();
    Text(PS && PS->Team == EERTeam::Omega ? TEXT("OMEGA") : TEXT("ALPHA"), 30, 30, PS && PS->Team == EERTeam::Omega ? FLinearColor(1,.25f,.1f) : FLinearColor(.2f,.65f,1));
    if (TActorIterator<AERReconDirector> It(GetWorld()); It) It->DrawMinimap(this,PC);
    if (bLab)
    {
        Text(TEXT("SHIFT: walk    SPACE + forward: climb"),30,105);
        Text(TEXT("30 cm: step    80 cm: vault    150 cm: shoulder height"),30,128);
    }
    if (Character && !Character->Health->bDead)
    {
        const auto* Movement = Cast<UERMovement>(Character->GetCharacterMovement());
        const float Sprint = Movement->GetSprintRemaining(), Recovery = Movement->GetRecoveryRemaining();
        Text(Sprint > 0 ? FString::Printf(TEXT("SPRINT %.1f s"), Sprint) : (Recovery > 0 ? FString::Printf(TEXT("RECOVERY %.1f s"), Recovery) : TEXT("SPRINT READY  |  double-tap W")), 30, H-100, FLinearColor(.6f,.8f,1), 1.f);
        if (Movement->bProneWanted || Movement->IsProne()) Text(TEXT("PRONE  |  C / Ctrl to stand up"), 30, H-125, FLinearColor::White, 1.f);
        const bool bOptical = Character->Weapon && Character->Weapon->HasOpticalScope();
        if (!bOptical)
        {
            DrawLine(W/2-8,H/2,W/2-3,H/2,FLinearColor::White); DrawLine(W/2+3,H/2,W/2+8,H/2,FLinearColor::White);
            DrawLine(W/2,H/2-8,W/2,H/2-3,FLinearColor::White); DrawLine(W/2,H/2+3,W/2,H/2+8,FLinearColor::White);
        }
        Text(FString::Printf(TEXT("HP  %.0f   AP  %.0f"), Character->Health->Health, Character->Health->Armor), 30, H-65, FLinearColor(.4f,1,.7f), 2.f);
        if (const auto* Weapon = Character->Weapon.Get())
        {
            Text(Weapon->bReloading ? TEXT("RELOADING...") : FString::Printf(TEXT("%02d / %02d    %s"),Weapon->MagazineAmmo,Weapon->ReserveAmmo,
                Weapon->Data ? *Weapon->Data->DisplayName : TEXT("Assault Rifle")),W-440,H-65,FLinearColor::White,1.4f);
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
            const bool bUnavailable = Character->IsWeaponStowed() || Weapon->bReloading || Cast<UERMovement>(Character->GetCharacterMovement())->bHighClimb;
            const FLinearColor AmmoColor = bUnavailable ? FLinearColor(1.f,.7f,.25f,.8f) :
                (Remaining <= Capacity / 5 ? FLinearColor(1.f,.4f,.18f,.95f) : FLinearColor(.8f,.95f,1.f,.9f));
            // Keep ammunition and the real spread cone visible through every aiming mode.
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
        if (Character->Health->IsProtected()) Text(TEXT("SPAWN PROTECTION"), W/2-145,H-100,FLinearColor(.4f,.8f,1));
    }
    if (PC->bAwaitingRespawn)
    {
        const int32 Seconds = FMath::Max(0, FMath::CeilToInt(PC->RespawnAllowedAt-GS->GetServerWorldTimeSeconds()));
        Text(Seconds > 0 ? FString::Printf(TEXT("Respawn available in %d"),Seconds) : TEXT("Press fire or jump to respawn"),W/2-175,H/2+60,FLinearColor(1,.6f,.3f),1.6f);
    }
    Text(TEXT("TAB  Scoreboard     ESC  Menu"),30,H-25,FLinearColor(.6f,.65f,.7f),1.f);
    if (PC->bShowScoreboard || GS->GetMatchState() == MatchState::WaitingPostMatch)
    {
        DrawRect(FLinearColor(.015f,.025f,.04f,.92f),W/2-340,130,680,240);
        Text(TEXT("TEAM          PLAYER                     K     D     TK"), W/2-320,150);
        float Y = 190;
        for (APlayerState* Item : GS->PlayerArray) if (const auto* State = Cast<AERPlayerState>(Item))
        {
            Text(FString::Printf(TEXT("%-10s    %-24s   %d     %d     %d"), State->Team == EERTeam::Alpha ? TEXT("Alpha") : TEXT("Omega"),*State->GetPlayerName(), State->Kills, State->Deaths, State->TeamKills),W/2-320,Y); Y += 36;
        }
    }
}

void AERPlayerController::ERBlockCollapseDemo()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority()) return;
    TActorIterator<AERBlockRoom> It(GetWorld());
    if (!It) return;
    // Cut a ring through every layer of the lab's south wall (wall 0), 20cm deep
    // below: the 145x95cm inside loses all support, collapses and has room to fall.
    TMap<FERCellId, float> Ring;
    const auto& Cells = It->GetCellWorld();
    for (int32 P = 0; P < Cells.GetPanels().Num(); ++P)
    {
        if (Cells.GetPanels()[P].Id / 8 != 0) continue;
        for (int32 X = 30; X <= 60; ++X) for (int32 Z = 17; Z <= 40; ++Z) for (int32 Y = 0; Y < Cells.GetPanels()[P].Dims.Y; ++Y)
            if (X == 30 || X == 60 || Z <= 20 || Z == 40) Ring.Add({P, X, Y, Z}, TNumericLimits<float>::Max());
    }
    It->ApplyCellBatch(Ring);
#endif
}

void UERGameInstance::HostProcedural()
{
    LastError.Empty();
    MapChoice=0;SessionMode=1;StartSession();
}
void AERPlayerController::OnPossess(APawn* Pawn)
{
    Super::OnPossess(Pawn);
    TActorIterator<AERBlockRoom> Room(GetWorld());if(Room)HoldForBlockMap(*Room);
}
void AERPlayerController::HoldForBlockMap(AERBlockRoom* Room,bool Reposition)
{
    if(!HasAuthority() || !Room || GetNetMode()==NM_Standalone)return;
    auto* C=Cast<AERCharacter>(GetPawn());if(!C)return;
    if(Reposition)
    {
        FVector P=Room->GetSpawnLocation();
        const auto* PS=GetPlayerState<AERPlayerState>();
        if(auto* Map=Cast<AERProceduralMap>(Room))if(PS && PS->Team==EERTeam::Omega)P=Map->GetActorTransform().TransformPosition(Map->GetLayout().Routes.Last()[0]);
        P.X+=(PS && (PS->GetPlayerId()/2)%2 ? -80:80);C->TeleportTo(P,FRotator::ZeroRotator);
        C->GetCharacterMovement()->StopMovementImmediately();
        if(IsLocalController() && !C->Health->bDead)C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        C->GetCharacterMovement()->ForceClientAdjustment();
    }
    if(!IsLocalController() && AcknowledgedBlockEpoch!=Room->GetMapEpoch())C->GetCharacterMovement()->SetMovementMode(MOVE_None);
}
void AERPlayerController::ServerBlockMapReady_Implementation(AERBlockRoom* Room,uint32 Epoch)
{
    if(!Room || !Room->CanMutateBlocks() || Epoch==0 || Room->GetMapEpoch()!=Epoch)return;
    AcknowledgedBlockEpoch=Epoch;
    if(auto* C=Cast<AERCharacter>(GetPawn()))if(!C->Health->bDead){C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);C->GetCharacterMovement()->ForceClientAdjustment();}
    UE_LOG(LogTemp,Display,TEXT("ER_BLOCK_NET_PLAYER_READY name=%s epoch=%u"),*GetPlayerState<AERPlayerState>()->GetPlayerName(),Epoch);
}
void AERPlayerController::UpdateBlockMapLoading()
{
    if(!IsLocalController() || GetNetMode()!=NM_Client)return;
    TActorIterator<AERBlockRoom> Room(GetWorld());
    const bool Lab=GetWorld()->GetMapName().Contains(TEXT("L_CentralBuilding")) || GetWorld()->GetMapName().Contains(TEXT("L_ProceduralLab")) || GetWorld()->GetMapName().Contains(TEXT("L_DestructionLab")) || GetWorld()->GetMapName().Contains(TEXT("L_Killhouse")) || GetWorld()->GetMapName().Contains(TEXT("L_DustMirror"));
    const bool Loading=Room ? !Room->IsMapReady():Lab;
    if(Loading!=bBlockMapLoading)
    {
        bBlockMapLoading=Loading;SetIgnoreMoveInput(Loading);
        if(auto* C=Cast<AERCharacter>(GetPawn()))C->GetCharacterMovement()->SetComponentTickEnabled(!Loading);
    }
    // A pawn can arrive after the loading gate was already entered.
    if(Loading)if(auto* C=Cast<AERCharacter>(GetPawn()))C->GetCharacterMovement()->SetComponentTickEnabled(false);
    if(Room && !Loading && GetPawn() && AcknowledgedBlockEpoch!=Room->GetMapEpoch())
    {AcknowledgedBlockEpoch=Room->GetMapEpoch();if(auto* C=Cast<AERCharacter>(GetPawn()))C->GetCharacterMovement()->SetComponentTickEnabled(true);ServerBlockMapReady(*Room,AcknowledgedBlockEpoch);}
}

void AERPlayerController::ServerBlockProbeReport_Implementation(AERBlockNetworkProbe* Probe,int32 Stage,uint32 Epoch,uint32 Sequence,uint32 Digest,bool Passed)
{
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("ERBlockNetworkProbe")) && Probe && Probe->GetWorld()==GetWorld())Probe->ReceiveReport(this,Stage,Epoch,Sequence,Digest,Passed);
#endif
}
