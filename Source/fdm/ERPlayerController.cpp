#include "ERPlayerController.h"
#include "ERRecon.h"
#include "ERInterfaceStyle.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#include "ERMenuPreview.h"
#include "Containers/Ticker.h"
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
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "Scalability.h"
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
#include "Engine/Font.h"
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
    // Apply the game's rendering profile in every executable. Previously these
    // groups only lived in play.command, leaving packaged games at Epic quality.
    auto Quality = Scalability::GetQualityLevels();
    Quality.AntiAliasingQuality = 2;
    Quality.GlobalIlluminationQuality = 0;
    Quality.ReflectionQuality = 0;
    Quality.ShadowQuality = 1;
    Scalability::SetQualityLevels(Quality);
    UE_LOG(LogTemp, Display, TEXT("ER_RENDER_PROFILE aa=2 gi=0 reflections=0 shadows=1"));
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
    GConfig->GetInt(TEXT("EclipseRecoil.Loadout"), TEXT("ReferenceStartingWeapon"), ReferenceStartingWeapon, GGameUserSettingsIni);
    ReferenceStartingWeapon = FMath::Clamp(ReferenceStartingWeapon, 0, 9);
    GConfig->GetInt(TEXT("EclipseRecoil.Appearance"), TEXT("AlphaSkin"), AlphaSkin, GGameUserSettingsIni);
    GConfig->GetInt(TEXT("EclipseRecoil.Appearance"), TEXT("OmegaSkin"), OmegaSkin, GGameUserSettingsIni);
    AlphaSkin = ERSkins::ValidIndex(AlphaSkin); OmegaSkin = ERSkins::ValidIndex(OmegaSkin);
    GConfig->GetFloat(TEXT("EclipseRecoil.Input"), TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
    MouseSensitivity = FMath::IsFinite(MouseSensitivity) ? FMath::Clamp(MouseSensitivity, 0.f, 1.f) : .5f;
    GConfig->GetFloat(TEXT("EclipseRecoil.Input"), TEXT("MouseAcceleration"), MouseAcceleration, GGameUserSettingsIni);
    MouseAcceleration = FMath::IsFinite(MouseAcceleration) ? FMath::Clamp(MouseAcceleration, 0.f, 1.f) : .5f;
    GConfig->GetFloat(TEXT("EclipseRecoil.Audio"), TEXT("SoundVolume"), SoundVolume, GGameUserSettingsIni);
    SetSoundVolume(SoundVolume);
    GConfig->GetFloat(TEXT("EclipseRecoil.Audio"), TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
    MusicVolume = FMath::IsFinite(MusicVolume) ? FMath::Clamp(MusicVolume, 0.f, 1.f) : 1.f;
    GConfig->GetString(TEXT("EclipseRecoil.Profile"), TEXT("Name"), LocalName, GGameUserSettingsIni);
    GConfig->GetInt(TEXT("EclipseRecoil.Profile"), TEXT("Team"), PlayerTeam, GGameUserSettingsIni);
    PlayerTeam = FMath::Clamp(PlayerTeam, 0, 1);
    for (auto Pair : {TPair<const TCHAR*, int32*>(TEXT("Magazines"), &SpareMagazines),
        {TEXT("HE"), &HECount}, {TEXT("Smoke"), &SmokeCount}, {TEXT("Stun"), &StunCount}})
    {
        GConfig->GetInt(TEXT("EclipseRecoil.Loadout"), Pair.Key, *Pair.Value, GGameUserSettingsIni);
        *Pair.Value = FMath::Clamp(*Pair.Value, 0, 20);
    }
    GConfig->GetInt(TEXT("EclipseRecoil.Server"), TEXT("MaxPlayers"), MaxPlayers, GGameUserSettingsIni);
    MaxPlayers = ERServer::ValidMaxPlayers(MaxPlayers);
    DiscoveryTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UERGameInstance::TickDiscovery), .1f);
    NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UERGameInstance::NetworkFailure);
    TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UERGameInstance::TravelFailure);
}
void UERGameInstance::SaveLoadout()
{
    GConfig->SetInt(TEXT("EclipseRecoil.Loadout"), TEXT("PrimaryWeapon"), PrimaryWeapon, GGameUserSettingsIni);
    GConfig->SetInt(TEXT("EclipseRecoil.Loadout"), TEXT("ReferenceStartingWeapon"), ReferenceStartingWeapon, GGameUserSettingsIni);
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
    FTSTicker::GetCoreTicker().RemoveTicker(DiscoveryTicker);
    CloseDiscoverySocket();
    SaveProfile();
    SaveServerSettings();
    SaveMouseSensitivity();
    SaveMusicVolume();
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
void UERGameInstance::SetSoundVolume(float Value)
{
    if (!FMath::IsFinite(Value)) return;
    SoundVolume = FMath::Clamp(Value, 0.f, 1.f);
    FApp::SetVolumeMultiplier(SoundVolume);
    GConfig->SetFloat(TEXT("EclipseRecoil.Audio"), TEXT("SoundVolume"), SoundVolume, GGameUserSettingsIni);
}
void UERGameInstance::SetMusicVolume(float Value)
{
    if (!FMath::IsFinite(Value)) return;
    MusicVolume = FMath::Clamp(Value, 0.f, 1.f);
    GConfig->SetFloat(TEXT("EclipseRecoil.Audio"), TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
    if (auto* PC = Cast<AERPlayerController>(GetFirstLocalPlayerController())) PC->UpdateMenuMusic();
}
void UERGameInstance::SaveMusicVolume()
{
    GConfig->SetFloat(TEXT("EclipseRecoil.Audio"), TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
void UERGameInstance::SaveServerSettings()
{
    MaxPlayers = ERServer::ValidMaxPlayers(MaxPlayers);
    GConfig->SetInt(TEXT("EclipseRecoil.Server"), TEXT("MaxPlayers"), MaxPlayers, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
void UERGameInstance::SaveProfile()
{
    LocalName = AERGameMode::SanitizeName(LocalName);
    GConfig->SetString(TEXT("EclipseRecoil.Profile"), TEXT("Name"), *LocalName, GGameUserSettingsIni);
    GConfig->SetInt(TEXT("EclipseRecoil.Profile"), TEXT("Team"), PlayerTeam, GGameUserSettingsIni);
    for (auto Pair : {TPair<const TCHAR*, int32*>(TEXT("Magazines"), &SpareMagazines),
        {TEXT("HE"), &HECount}, {TEXT("Smoke"), &SmokeCount}, {TEXT("Stun"), &StunCount}})
        GConfig->SetInt(TEXT("EclipseRecoil.Loadout"), Pair.Key, *Pair.Value, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
int32 UERGameInstance::GetStartingWeapon() const
{
    const auto* PC = Cast<AERPlayerController>(GetFirstLocalPlayerController());
    return PC && !PC->bReferenceArsenalMenu ? PrimaryWeapon : ReferenceStartingWeapon;
}
void UERGameInstance::StartSession()
{
    if (SessionMode == 2) { Join(ServerAddress); return; }
    SaveProfile();
    LastError.Empty();
    const auto& Map=ERMaps::At(MapChoice);
    const TCHAR* Behaviors[] = {TEXT("Target"), TEXT("Movement"), TEXT("Patrol")};
    FString Options = SessionMode == 1 ? TEXT("listen?Name=") : TEXT("Name=");
    Options += AERGameMode::SanitizeName(LocalName);
    Options += FString::Printf(TEXT("?Primary=%d?AlphaSkin=%d?OmegaSkin=%d?Team=%d?ERAlphaBots=%d?EROmegaBots=%d?ERBotMode=%s"),
        GetStartingWeapon(), AlphaSkin, OmegaSkin, FMath::Clamp(PlayerTeam,0,1), FMath::Clamp(AlphaBots,0,10), FMath::Clamp(OmegaBots,0,10), Behaviors[FMath::Clamp(BotBehavior,0,2)]);
    Options += FString::Printf(TEXT("?Magazines=%d?HE=%d?Smoke=%d?Stun=%d"), SpareMagazines, HECount, SmokeCount, StunCount);
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
        SaveProfile();
        CancelLANSearch();
        LastError = TEXT("Server found. Connecting to ") + Address + TEXT("...");
        PC->ClientTravel(Address + TEXT("?Name=") + AERGameMode::SanitizeName(LocalName) + FString::Printf(TEXT("?Primary=%d?AlphaSkin=%d?OmegaSkin=%d?Team=%d?Magazines=%d?HE=%d?Smoke=%d?Stun=%d"), GetStartingWeapon(), AlphaSkin, OmegaSkin, FMath::Clamp(PlayerTeam,0,1), SpareMagazines, HECount, SmokeCount, StunCount), TRAVEL_Absolute); return true;
    }
    LastError = TEXT("Local player unavailable."); return false;
}
void UERGameInstance::ReturnToMenu() { CancelLANSearch(); CloseDiscoverySocket(); UGameplayStatics::OpenLevel(GetWorld(), TEXT("/Game/EclipseRecoil/Maps/L_MainMenu")); }
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
        if (!GIsEditor && IsMenuLevel() && !GI->bInitialWindowSizeApplied && !FParse::Param(FCommandLine::Get(), TEXT("nullrhi")))
        {
            GI->bInitialWindowSizeApplied = true;
            int32 ExplicitSize = 0;
            const bool Explicit = FParse::Value(FCommandLine::Get(), TEXT("ResX="), ExplicitSize) ||
                FParse::Value(FCommandLine::Get(), TEXT("ResY="), ExplicitSize) ||
                FParse::Param(FCommandLine::Get(), TEXT("windowed")) || FParse::Param(FCommandLine::Get(), TEXT("fullscreen"));
            bool Saved = false;
            GConfig->GetBool(TEXT("EclipseRecoil.Display"), TEXT("Configured"), Saved, GGameUserSettingsIni);
            if (!Explicit)
            {
                if (Saved) GEngine->GetGameUserSettings()->ApplyResolutionSettings(false);
                else ApplyDisplaySize(100);
            }
        }
    }
    if (ULocalPlayer* LP = GetLocalPlayer()) if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) if (GameplayMapping) Subsystem->AddMappingContext(GameplayMapping, 0);
    ShowMenu(IsMenuLevel());
    ERInstallResolutionGovernor();
    UpdateRenderResolution();
    FrameRateWidget = SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(12)
        [SNew(SBox).Padding(4)
            [SNew(STextBlock).Font(ERInterface::Font(11)).ShadowOffset(FVector2D(1,1))
                .ShadowColorAndOpacity(FLinearColor::Black).Text_Lambda([this]() { return GetFrameRateText(); })]];
    FrameRateWidget->SetVisibility(EVisibility::HitTestInvisible);
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(FrameRateWidget.ToSharedRef(), 30);
}
void AERPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    UpdateBlockMapLoading();
    if (IsLocalController() && bShowingLobby && !IsMenuLevel()) { MenuPage = 0; ShowMenu(false); }
    if (IsLocalController() && IsMenuLevel() && bShowingLobby != IsInLobby())
    { MenuPage = 0; ShowMenu(true); }
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
    const auto* PlayerCharacter = Cast<AERCharacter>(GetPawn());
    if (!PlayerCharacter || !PlayerCharacter->Weapon) return;
    const FVector Position = PlayerCharacter->GetActorLocation();
    const FVector Velocity = PlayerCharacter->GetVelocity();
    const auto* Weapon = PlayerCharacter->Weapon.Get();
    const auto* State = GetPlayerState<AERPlayerState>();
    UE_LOG(LogTemp,Display,TEXT("ER_INPUT_AUDIT pawn=%s pos=%.1f,%.1f,%.1f vel=%.1f,%.1f crouch=%d half=%.1f camz=%.1f aim=%.1f,%.1f ammo=%d/%d reload=%d dead=%d protected=%d menu=%d scoreboard=%d kills=%d deaths=%d ctrl=%d can_crouch=%d"),
        *PlayerCharacter->GetName(),Position.X,Position.Y,Position.Z,Velocity.Size2D(),Velocity.Z,PlayerCharacter->bIsCrouched,
        PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),PlayerCharacter->Camera->GetComponentLocation().Z,
        GetControlRotation().Pitch,GetControlRotation().Yaw,Weapon->MagazineAmmo,Weapon->ReserveAmmo,Weapon->bReloading,
        PlayerCharacter->Health->bDead,PlayerCharacter->Health->IsProtected(),bMenuOpen,bShowScoreboard,State ? State->Kills : 0,State ? State->Deaths : 0,
        IsInputKeyDown(EKeys::LeftControl),PlayerCharacter->CanCrouch());
#endif
}
void AERPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (MenuMusicComponent)
    {
        MenuMusicComponent->Stop();
        MenuMusicComponent->DestroyComponent();
        MenuMusicComponent = nullptr;
        UE_LOG(LogTemp, Display, TEXT("ER_MENU_MUSIC stopped (end play)"));
    }
    if (MenuWidget.IsValid() && GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
    if (FrameRateWidget.IsValid() && GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FrameRateWidget.ToSharedRef());
    FrameRateWidget.Reset(); DisplaySizeSelector.Reset();
    if (MenuPreview) { MenuPreview->Destroy(); MenuPreview = nullptr; }
    PreviewBrush.Reset();
    if (IsLocalController() && GetLocalPlayer()) if (auto* Subsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) if (GameplayMapping) Subsystem->RemoveMappingContext(GameplayMapping);
    MenuWidget.Reset(); Super::EndPlay(Reason);
}
void AERPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AERPlayerController::SprintPressed);
    InputComponent->BindKey(EKeys::E, IE_Released, this, &AERPlayerController::SprintReleased);
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
void AERPlayerController::ResetSprintInput()
{
    bSprintKeyHeld = bForwardSprintHeld = bForwardHeld = false;
    LastForwardTap = -1;
    UpdateSprintInput();
}
void AERPlayerController::PrepareGesturePawn(AERCharacter* Character)
{
    if (GesturePawn.Get() == Character) return;
    GesturePawn = Character;
    ResetSprintInput();
    LastCrouchTap = -1;
}
void AERPlayerController::UpdateSprintInput()
{
    if (auto* C = Cast<AERCharacter>(GetPawn()))
        Cast<UERMovement>(C->GetCharacterMovement())->bSprintHeld = bForwardSprintHeld || (bSprintKeyHeld && bForwardHeld);
}
void AERPlayerController::SprintPressed()
{
    auto* C = Cast<AERCharacter>(GetPawn());
    if (bMenuOpen || !C || C->Health->bDead) return;
    PrepareGesturePawn(C);
    bSprintKeyHeld = true;
    UpdateSprintInput();
}
void AERPlayerController::SprintReleased()
{
    bSprintKeyHeld = false;
    UpdateSprintInput();
}
void AERPlayerController::ForwardPressed()
{
    auto* C = Cast<AERCharacter>(GetPawn());
    if (bMenuOpen || !C || C->Health->bDead) return;
    PrepareGesturePawn(C);
    bForwardHeld = true;
    const double Now = GetWorld()->GetTimeSeconds();
    if (LastForwardTap >= 0 && Now - LastForwardTap <= .3)
    {
        bForwardSprintHeld = true;
        LastForwardTap = -1;
    }
    else LastForwardTap = Now;
    UpdateSprintInput();
}
void AERPlayerController::ForwardReleased()
{
    bForwardHeld = bForwardSprintHeld = false;
    UpdateSprintInput();
}
void AERPlayerController::WalkPressed() { if (!bMenuOpen) if (auto* C = Cast<AERCharacter>(GetPawn())) Cast<UERMovement>(C->GetCharacterMovement())->bWalkHeld = true; }
void AERPlayerController::WalkReleased() { if (auto* C = Cast<AERCharacter>(GetPawn())) Cast<UERMovement>(C->GetCharacterMovement())->bWalkHeld = false; }
void AERPlayerController::Move(const FInputActionValue& Value)
{
    auto* PlayerCharacter = Cast<AERCharacter>(GetPawn()); if (!PlayerCharacter || PlayerCharacter->Health->bDead || bMenuOpen) return;
    const FVector2D Axis = Value.Get<FVector2D>(); const FRotator Rotation(0, GetControlRotation().Yaw, 0);
    if(auto* M=Cast<UERMovement>(PlayerCharacter->GetCharacterMovement());M->IsOnLadder())
    {
        if(auto* L=M->FindLadder())PlayerCharacter->AddMovementInput(-L->GetActorForwardVector(),Axis.Y);
        return;
    }
    PlayerCharacter->AddMovementInput(Rotation.Vector(), Axis.Y);
    PlayerCharacter->AddMovementInput(FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y), Axis.X);
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
    if (auto* PlayerCharacter = Cast<AERCharacter>(GetPawn())) { PlayerCharacter->SetGrenadeInputHeld(true);PlayerCharacter->ServerStartFire(); }
}
void AERPlayerController::FireReleased() { if (auto* PlayerCharacter = Cast<AERCharacter>(GetPawn())) { PlayerCharacter->SetGrenadeInputHeld(false);PlayerCharacter->ServerStopFire(); } }
void AERPlayerController::JumpPressed()
{
    if (bMenuOpen) return;
    if (bAwaitingRespawn) { ServerRequestRespawn(); return; }
    if (auto* PlayerCharacter = Cast<AERCharacter>(GetPawn()); PlayerCharacter && !PlayerCharacter->Health->bDead) { Cast<UERMovement>(PlayerCharacter->GetCharacterMovement())->bClimbHeld = true; PlayerCharacter->Jump(); }
}
void AERPlayerController::JumpReleased() { if (auto* PlayerCharacter = Cast<AERCharacter>(GetPawn())) { Cast<UERMovement>(PlayerCharacter->GetCharacterMovement())->bClimbHeld = false; PlayerCharacter->StopJumping(); } }
void AERPlayerController::CrouchPressed()
{
    auto* C = Cast<AERCharacter>(GetPawn());
    if (bMenuOpen || !C || C->Health->bDead) return;
    auto* M = Cast<UERMovement>(C->GetCharacterMovement());
    if (M->IsTraversing() || !M->IsMovingOnGround()) return;
    PrepareGesturePawn(C);
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
    bForwardSprintHeld = false;
    UpdateSprintInput();
}
void AERPlayerController::CrouchReleased() { if (auto* C = Cast<AERCharacter>(GetPawn())) C->UnCrouch(); }
void AERPlayerController::ReloadPressed() { if (!bMenuOpen) if (auto* PlayerCharacter = Cast<AERCharacter>(GetPawn())) PlayerCharacter->ServerReload(); }
void AERPlayerController::ScoreboardPressed() { bShowScoreboard = true; }
void AERPlayerController::ScoreboardReleased() { bShowScoreboard = false; }
void AERPlayerController::MenuPressed()
{
    if (IsMenuLevel()) { if (MenuPage != 0) { GetGameInstance<UERGameInstance>()->CancelLANSearch(); OpenMenuPage(0); } }
    else if (MenuPage != 0) OpenMenuPage(0);
    else ShowMenu(!bMenuOpen);
}
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
    if (const auto* GS = GetWorld()->GetGameState<AERGameState>(); GS && GS->bLobbyStarting) return;
    if (Alpha < 0 || Alpha >= ERSkins::Count || Omega < 0 || Omega >= ERSkins::Count) return;
    if (auto* PS = GetPlayerState<AERPlayerState>())
    { PS->bLobbyReady = false; PS->AlphaSkin = Alpha; PS->OmegaSkin = Omega; PS->ForceNetUpdate(); }
}
void AERPlayerController::ServerSetPrimary_Implementation(int32 Index)
{
    if (const auto* GS = GetWorld()->GetGameState<AERGameState>(); GS && GS->bLobbyStarting) return;
    const bool Reference = bReferenceArsenalMenu && !GetWorld()->URL.HasOption(TEXT("ERLegacyArsenal"));
    if (Index < 0 || Index > (Reference ? 9 : 6)) return;
    if (auto* PS = GetPlayerState<AERPlayerState>()) { PS->bLobbyReady = false; PS->PrimaryWeapon = Index; PS->ForceNetUpdate(); }
}
void AERPlayerController::ClientFlash_Implementation(float Strength) { FlashOpacity = FMath::Max(FlashOpacity, FMath::Clamp(Strength, 0.f, .9f)); }
void AERPlayerController::ServerRequestRespawn_Implementation() { if (auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>()) Mode->RequestRespawn(this); }
void AERPlayerController::ServerSetName_Implementation(const FString& Name)
{
    if (const auto* GS = GetWorld()->GetGameState<AERGameState>(); GS && GS->bLobbyStarting) return;
    if (PlayerState) PlayerState->SetPlayerName(AERGameMode::SanitizeName(Name));
}
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
    return FText::FromString(FString::Printf(TEXT("%.0f FPS   ·   %d × %d"),
        1000.f / FrameTimeMs, RenderSize.X, RenderSize.Y));
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
    GConfig->SetBool(TEXT("EclipseRecoil.Display"), TEXT("Configured"), true, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
    if (DisplaySizeSelector.IsValid()) DisplaySizeSelector->SetIsOpen(false);
    UE_LOG(LogTemp, Display, TEXT("ER_DISPLAY size=%d%% resolution=%dx%d fullscreen=%d"), Percent, Resolution.X, Resolution.Y, bFullscreen);
}

void AERPlayerController::UpdateMenuMusic()
{
    const auto* GI = GetGameInstance<UERGameInstance>();
    const float Volume = MenuMusicVolume * (GI ? GI->GetMusicVolume() : 1.f) * (IsMenuLevel() ? 1.f : .5f);
    const bool bShouldPlay = IsLocalController() && GetNetMode() != NM_DedicatedServer &&
        bMenuOpen && MenuMusic && Volume > 0.f;
    if (bShouldPlay && !MenuMusicComponent)
    {
        MenuMusicComponent = UGameplayStatics::SpawnSound2D(this, MenuMusic,
            Volume, 1.f, 0.f, nullptr, false, false);
        if (MenuMusicComponent)
            UE_LOG(LogTemp, Display, TEXT("ER_MENU_MUSIC started sound=%s volume=%.3f main=%d"), *MenuMusic->GetPathName(), Volume, IsMenuLevel());
    }
    else if (!bShouldPlay && MenuMusicComponent)
    {
        MenuMusicComponent->Stop();
        MenuMusicComponent->DestroyComponent();
        MenuMusicComponent = nullptr;
        UE_LOG(LogTemp, Display, TEXT("ER_MENU_MUSIC stopped (menu closed)"));
    }
    if (bShouldPlay && MenuMusicComponent) MenuMusicComponent->SetVolumeMultiplier(Volume);
}
void AERPlayerController::ERBlockDamage(float Amount)
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !GetPawn()) return;
    FVector Eye; FRotator Aim;
    GetPawn()->GetActorEyesViewPoint(Eye, Aim);
    if (auto* PlayerCharacter = Cast<AERCharacter>(GetPawn())) Eye = PlayerCharacter->Camera->GetComponentLocation();
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
    if (auto* PlayerCharacter = Cast<AERCharacter>(GetPawn())) Eye = PlayerCharacter->Camera->GetComponentLocation();
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
    if (auto* PlayerCharacter = Cast<AERCharacter>(GetPawn())) Eye = PlayerCharacter->Camera->GetComponentLocation();
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
    if (auto* PlayerCharacter = Cast<AERCharacter>(GetPawn())) Eye = PlayerCharacter->Camera->GetComponentLocation();
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
void AERHUD::DrawHUDText(const FString& Value, float X, float Y, FLinearColor Color, float Scale, bool Centered)
{
    DrawHUDTextAnchored(Value,FVector2D(X,Y),Color,Scale,FVector2D(Centered ? .5f : 0.f,0));
}

FVector2D AERHUD::DrawHUDTextAnchored(const FString& Value, FVector2D Position, FLinearColor Color, float Scale, FVector2D Anchor)
{
    if (!Canvas || !Canvas->Canvas) return FVector2D::ZeroVector;
    const float ViewScale = FMath::Clamp(Canvas->SizeY / 720.f, .75f, 2.f);
    if (!HUDTypeface)
    {
        // Canvas requires a UFont even when given a composite Slate typeface.
        HUDTypeface = NewObject<UFont>(this);
        HUDTypeface->FontCacheType = EFontCacheType::Runtime;
        HUDTypeface->GetMutableInternalCompositeFont() = *ERInterface::Font(24).GetCompositeFont();
    }
    const FSlateFontInfo Font(HUDTypeface,FMath::RoundToInt(24.f*Scale*ViewScale),TEXT("SemiBold"));
    const float DPI=Canvas->Canvas->GetDPIScale();
    const FVector2D Size=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value,Font,DPI)/DPI;
    Position-=Size*Anchor;
    FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(Position.X),FMath::RoundToFloat(Position.Y)),FText::FromString(Value),Font,Color);
    Item.EnableShadow(FLinearColor(0,0,0,.9f), FVector2D(2,2)*ViewScale);
    Canvas->DrawItem(Item);
    return Size;
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
    const float HUDScale = FMath::Clamp(H / 720.f, .75f, 2.f);
    const float EdgeMargin=40.f*HUDScale;
    auto Text = [this](const FString& Value, float X, float Y, FLinearColor Color = FLinearColor::White, float Scale = 1.3f) { DrawHUDText(Value, X, Y, Color, Scale); };
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

    const FLinearColor AlphaColor=ERRecon::TeamColor(EERTeam::Alpha), OmegaColor=ERRecon::TeamColor(EERTeam::Omega);
    DrawHUDText(FString::Printf(TEXT("%02d"),GS->AlphaScore),W/2-85*HUDScale,20*HUDScale,AlphaColor,1.45f,true);
    DrawHUDText(TEXT("50"),W/2,27*HUDScale,FLinearColor(.8f,.82f,.85f),1.f,true);
    DrawHUDText(FString::Printf(TEXT("%02d"),GS->OmegaScore),W/2+85*HUDScale,20*HUDScale,OmegaColor,1.45f,true);
    FString Status = TEXT("Waiting for players");
    if (GS->IsMatchInProgress())
    {
        const int32 Seconds = FMath::Max(0, FMath::CeilToInt(GS->MatchEndServerTime - GS->GetServerWorldTimeSeconds()));
        Status = FString::Printf(TEXT("%02d:%02d"), Seconds/60, Seconds%60);
    }
    else if (GS->GetMatchState() == MatchState::WaitingPostMatch)
        Status = GS->Winner == EERTeam::None ? TEXT("DRAW") : TEXT("ROUND OVER");
    if (bLab && Character) Status = FString::Printf(TEXT("%.1f km/h   |   Feet height: %.0f cm"), Character->GetVelocity().Size2D()*.036f, Character->GetActorLocation().Z-Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-2.1f);
    DrawHUDText(Status, W/2, 59*HUDScale, FLinearColor(.8f,.82f,.85f), .9f, true);
    if (TActorIterator<AERReconDirector> It(GetWorld()); It) It->DrawMinimap(this,PC);
    if (bLab)
    {
        Text(TEXT("SHIFT: walk    SPACE + forward: climb"),30,105);
        Text(TEXT("30 cm: step    80 cm: vault    150 cm: shoulder height"),30,128);
    }
    if (Character && !Character->Health->bDead)
    {
        const auto* Movement = Cast<UERMovement>(Character->GetCharacterMovement());
        const FVector2D HealthSize=DrawHUDTextAnchored(FString::Printf(TEXT("HP  %.0f   AP  %.0f"),Character->Health->Health,Character->Health->Armor),
            FVector2D(EdgeMargin,H-EdgeMargin),FLinearColor(.4f,1,.7f),1.35f,FVector2D(0,1));
        const float SprintBottom=H-EdgeMargin-HealthSize.Y-6.f*HUDScale;
        const float BarWidth=180.f*HUDScale, BarHeight=8.f*HUDScale;
        const float BarTop=SprintBottom-BarHeight;
        const FLinearColor StaminaColor=Movement->IsSprinting() ? FLinearColor(1,.65f,.2f) : FLinearColor(.6f,.8f,1);
        DrawRect(FLinearColor(0,0,0,.65f),EdgeMargin-HUDScale,BarTop-HUDScale,BarWidth+2*HUDScale,BarHeight+2*HUDScale);
        DrawRect(FLinearColor(.15f,.18f,.22f,.9f),EdgeMargin,BarTop,BarWidth,BarHeight);
        DrawRect(StaminaColor,EdgeMargin,BarTop,BarWidth*Movement->GetStaminaFraction(),BarHeight);
        const FVector2D SprintSize=DrawHUDTextAnchored(TEXT("STAMINA"),
            FVector2D(EdgeMargin,BarTop-3.f*HUDScale),StaminaColor,.85f,FVector2D(0,1));
        if (Movement->bProneWanted || Movement->IsProne()) DrawHUDTextAnchored(TEXT("PRONE  |  C / Ctrl to stand up"),
            FVector2D(EdgeMargin,BarTop-3.f*HUDScale-SprintSize.Y-6.f*HUDScale),FLinearColor::White,.85f,FVector2D(0,1));
        const bool bOptical = Character->Weapon && Character->Weapon->HasOpticalScope();
        if (!bOptical)
        {
            DrawLine(W/2-8,H/2,W/2-3,H/2,FLinearColor::White); DrawLine(W/2+3,H/2,W/2+8,H/2,FLinearColor::White);
            DrawLine(W/2,H/2-8,W/2,H/2-3,FLinearColor::White); DrawLine(W/2,H/2+3,W/2,H/2+8,FLinearColor::White);
        }
        if (const auto* Weapon = Character->Weapon.Get())
        {
            DrawHUDTextAnchored(Weapon->bReloading ? TEXT("RELOADING...") : FString::Printf(TEXT("%02d / %02d    %s"),Weapon->MagazineAmmo,Weapon->ReserveAmmo,
                Weapon->Data ? *Weapon->Data->DisplayName : TEXT("Assault Rifle")),FVector2D(W-EdgeMargin,H-EdgeMargin),FLinearColor::White,1.1f,FVector2D(1,1));
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
                const FLinearColor DotColor = I < Remaining ? AmmoColor : FLinearColor(.14f,.19f,.23f,.55f);
                constexpr int32 DotSegments = 16;
                const float DotRadius = 1.85f * Scale;
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
                const auto AddDisc = [&](FVector2D At, float Radius, float Feather, FLinearColor Ink)
                {
                    const FLinearColor Clear = Ink.CopyWithNewOpacity(0);
                    for (int32 Segment = 0; Segment < DotSegments; ++Segment)
                    {
                        const float A = Segment * 2.f * PI / DotSegments, B = (Segment+1) * 2.f * PI / DotSegments;
                        const FVector2D DA(FMath::Cos(A),FMath::Sin(A)), DB(FMath::Cos(B),FMath::Sin(B));
                        const FVector2D InnerA=At+DA*(Radius-Feather), InnerB=At+DB*(Radius-Feather);
                        const FVector2D OuterA=At+DA*Radius, OuterB=At+DB*Radius;
                        AddDotTriangle(At,Ink,InnerA,Ink,InnerB,Ink);
                        AddDotTriangle(InnerA,Ink,OuterA,Clear,OuterB,Clear);
                        AddDotTriangle(InnerA,Ink,OuterB,Clear,InnerB,Ink);
                    }
                };
                AddDisc(Center+FVector2D(0,.75f*Scale),DotRadius+1.5f*Scale,1.4f*Scale,FLinearColor(0,0,0,.8f));
                AddDisc(Center,DotRadius,.7f*Scale,DotColor);
                FCanvasTriangleItem Dot(DotTriangles, GWhiteTexture);
                Dot.BlendMode = SE_BLEND_Translucent;
                Canvas->DrawItem(Dot);
            }
#if !UE_BUILD_SHIPPING
            if (FParse::Param(FCommandLine::Get(), TEXT("ERAccuracyAudit")) && GetWorld()->GetTimeSeconds() >= NextReticleAuditAt)
            {
                NextReticleAuditAt = GetWorld()->GetTimeSeconds() + .1;
                UE_LOG(LogTemp,Display,TEXT("ER_RETICLE ammo=%d ticks=%d capacity=%d spread=%.3f bloom=%.3f radius=%.1f reload=%d"),
                    Weapon->MagazineAmmo,Remaining,Capacity,Spread,Weapon->GetBloomDegrees(),ReticleRadius,Weapon->bReloading);
            }
#endif
        }
        if (Character->Health->IsProtected()) DrawHUDText(TEXT("SPAWN PROTECTION"), W/2,H-119*HUDScale,FLinearColor(.4f,.8f,1),1.3f,true);
    }
    if (PC->bAwaitingRespawn)
    {
        const int32 Seconds = FMath::Max(0, FMath::CeilToInt(PC->RespawnAllowedAt-GS->GetServerWorldTimeSeconds()));
        DrawHUDText(Seconds > 0 ? FString::Printf(TEXT("Respawn available in %d"),Seconds) : TEXT("Press fire or jump to respawn"),W/2,H/2+60,FLinearColor(1,.6f,.3f),1.6f,true);
    }
    if (PC->bShowScoreboard || GS->GetMatchState() == MatchState::WaitingPostMatch)
    {
        const float Left=W/2-340*HUDScale, Top=130*HUDScale;
        DrawRect(FLinearColor(.015f,.025f,.04f,.92f),Left,Top,680*HUDScale,FMath::Min(H-Top-40*HUDScale,(65+GS->PlayerArray.Num()*36)*HUDScale));
        const auto Row = [&](const FString& Team, const FString& Name, const FString& K, const FString& D, const FString& TK, float Y)
        {
            const FString Values[]={Team,Name,K,D,TK};
            const float Columns[]={20,140,490,545,600};
            for(int32 I=0;I<5;++I) Text(Values[I],Left+Columns[I]*HUDScale,Y,FLinearColor::White,1.05f);
        };
        Row(TEXT("TEAM"),TEXT("PLAYER"),TEXT("K"),TEXT("D"),TEXT("TK"),Top+12*HUDScale);
        float Y = Top+54*HUDScale;
        for (APlayerState* Item : GS->PlayerArray) if (const auto* State = Cast<AERPlayerState>(Item))
        {
            if(Y+36*HUDScale>H-40*HUDScale) break;
            Row(State->Team == EERTeam::Alpha ? TEXT("Alpha") : TEXT("Omega"),State->GetPlayerName().Left(24),FString::FromInt(State->Kills),FString::FromInt(State->Deaths),FString::FromInt(State->TeamKills),Y);
            Y += 36*HUDScale;
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
void AERPlayerController::OnPossess(APawn* PossessedPawn)
{
    Super::OnPossess(PossessedPawn);
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
