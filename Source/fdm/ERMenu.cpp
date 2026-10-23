#include "ERLoadout.h"
#include "ERPlayerController.h"
#include "ERPerformanceSettings.h"
#include "EROnline.h"
#include "ERMatch.h"
#include "ERCharacter.h"
#include "ERMenuPreview.h"
#include "ERInterfaceStyle.h"
#include "ERMapCatalog.h"
#include "ERWeapon.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Fonts/CompositeFont.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
FSlateFontInfo MenuFont(int32 Size)
{
    return ERInterface::Font(Size);
}
TSharedRef<STextBlock> Text(const FString& Value, int32 Size = 24)
{
    return SNew(STextBlock).Text(FText::FromString(Value)).Font(MenuFont(Size))
        .ColorAndOpacity(FLinearColor::White).ShadowOffset(FVector2D(2, 2)).ShadowColorAndOpacity(FLinearColor::Black).AutoWrapText(true);
}
TSharedRef<SWidget> Button(const FString& Label, TFunction<FReply()> Action, bool Main = false)
{
    if (!Main)
        return SNew(SButton).ContentPadding(FMargin(18, 10)).OnClicked_Lambda(MoveTemp(Action))[Text(Label, 25)];
    // Keep the background artwork visible in every state. Text inherits the
    // button foreground, so hover/press no longer get masked by fixed white.
    static const FButtonStyle MainStyle = FButtonStyle(FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
        .SetNormalForeground(FLinearColor::White)
        .SetHoveredForeground(FLinearColor(1.f, .68f, .22f))
        .SetPressedForeground(FLinearColor(1.f, .24f, .045f))
        .SetNormalPadding(FMargin(0))
        .SetPressedPadding(FMargin(4, 2, -4, -2));
    auto Entry = Text(Label, 54);
    Entry->SetColorAndOpacity(FSlateColor::UseForeground());
    return SNew(SButton).ButtonStyle(&MainStyle).Cursor(EMouseCursor::Hand)
        .ContentPadding(FMargin(12, 9)).OnClicked_Lambda(MoveTemp(Action))[Entry];
}
void Add(TSharedRef<SVerticalBox> Panel, TSharedRef<SWidget> Widget)
{
    Panel->AddSlot().AutoHeight().Padding(0, 5)[Widget];
}
// Reflow whole label/control groups, preserving keyboard order and keeping
// each field together. Cached local width includes Slate DPI scaling.
TFunction<TSharedRef<SVerticalBox>()> Columns(TSharedRef<SVerticalBox> Panel)
{
    auto Grid=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(24,8));
    Add(Panel,Grid);
    TWeakPtr<SVerticalBox> WeakPanel=Panel;
    return [Grid,WeakPanel]()
    {
        auto Cell=SNew(SVerticalBox);
        Grid->AddSlot().VAlign(VAlign_Top)
            [SNew(SBox).WidthOverride_Lambda([WeakPanel]()
            {
                const auto Owner=WeakPanel.Pin();
                const float Width=Owner.IsValid() ? Owner->GetCachedGeometry().GetLocalSize().X:0.f;
                return Width>0 ? (Width>=900 ? (Width-24)/2:Width):440.f;
            })[Cell]];
        return Cell;
    };
}
void Choice(TSharedRef<SVerticalBox> Panel, const FString& Title, TArray<FString> Labels,
    TFunction<int32()> Get, TFunction<void(int32)> Set)
{
    Add(Panel, Text(Title, 22));
    auto Items = SNew(SVerticalBox);
    for (int32 I = 0; I < Labels.Num(); ++I)
        Add(Items, Button(Labels[I], [I, Set]() { Set(I); FSlateApplication::Get().DismissAllMenus(); return FReply::Handled(); }));
    Add(Panel, SNew(SComboButton)
        .ButtonContent()[SNew(STextBlock).Font(MenuFont(25)).AutoWrapText(true).Text_Lambda([Labels, Get]() { return FText::FromString(Labels[FMath::Clamp(Get(), 0, Labels.Num()-1)]); })]
        .MenuContent()[SNew(SBox).MaxDesiredHeight(420)[SNew(SScrollBox) + SScrollBox::Slot()[Items]]]);
}
}
void AERPlayerController::ERMenu(int32 Page) { OpenMenuPage(FMath::Clamp(Page, 0, 8)); }
void AERPlayerController::OpenMenuPage(int32 Page)
{
    if ((Page == 5 || Page == 6 || Page == 7) && MenuPage != 5 && MenuPage != 6 && MenuPage != 7) SubmenuParent = MenuPage;
    if(Page==6 && MenuPage!=6)
    {
        const auto* GI=GetGameInstance<UERGameInstance>();const auto* PS=(!IsMenuLevel() || IsInLobby())?GetPlayerState<AERPlayerState>():nullptr;
        const auto Kit=ERLoadouts::Resolve(PS?PS->LoadoutPreset:GI->LoadoutPreset,PS?PS->LoadoutVariant:GI->LoadoutVariant);
        DraftPreset=Kit.Preset;DraftVariant=Kit.Variant;
    }
    MenuPage = Page;
    ShowMenu(true);
}
void AERPlayerController::ShowMenu(bool bShow)
{
    if (!IsLocalController() || !GetWorld()->GetGameViewport()) return;
    ResetSprintInput(); LastCrouchTap = -1;
    FireReleased(); WalkReleased(); CrouchReleased(); JumpReleased(); bShowScoreboard = false;
    if (auto* C = Cast<AERCharacter>(GetPawn()); C && C->Weapon) C->Weapon->ServerClearScope();
    bShowingLobby = IsInLobby();
    bMenuOpen = bShow; bShowMouseCursor = bShow;
    UpdateMenuMusic();
    SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
    if (MenuWidget.IsValid()) { GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef()); MenuWidget.Reset(); }
    if (MenuPreview && (!bShow || MenuPage!=6)) { MenuPreview->Destroy(); MenuPreview = nullptr; }
    PreviewBrush.Reset();
    MenuFocusWidget.Reset();
    bPendingWindowInput = true;
    if (!bShow) { ApplyWindowInputMode(); return; }
    auto* GI = GetGameInstance<UERGameInstance>(); if (!GI) return;
    const bool Main = IsMenuLevel();
    const bool Lobby = bShowingLobby;
    if (Lobby) GI->LastError.Empty();
    const bool Home = MenuPage == 0 && !Lobby;
    auto Panel = SNew(SVerticalBox);
    TSharedPtr<SWidget> Focus;
    auto Go = [this](int32 Page) { return [this, Page]() { OpenMenuPage(Page); return FReply::Handled(); }; };
    auto* Online = GI->GetSubsystem<UEROnline>();
    auto Back = [this, GI, Online]() { Online->Cancel(); GI->CancelLANSearch(); OpenMenuPage(MenuPage == 5 || MenuPage == 6 || MenuPage == 7 ? SubmenuParent : MenuPage == 8 ? 4 : 0); return FReply::Handled(); };
    auto SaveSupplies = [this, GI]() { GI->SaveProfile(); ServerSetSupplies(GI->SpareMagazines, GI->HECount, GI->SmokeCount, GI->StunCount); };
    TFunction<TSharedRef<SVerticalBox>()> Field=[Panel]() { return Panel; };
    auto IntChoice = [&Field](const FString& Label, TArray<FString> Labels, int32* Value)
    { Choice(Field(), Label, Labels, [Value]() { return *Value; }, [Value](int32 I) { *Value = I; }); };
    TArray<FString> Counts; for (int32 I = 0; I <= 20; ++I) Counts.Add(FString::FromInt(I));
    const auto* Catalog = SkinCatalog ? SkinCatalog->GetDefaultObject<AERCharacter>() : nullptr;
    if (Home)
    {
        if (Main)
        {
            Focus = Button(TEXT("Play Offline"), Go(1), true); Add(Panel, Focus.ToSharedRef());
            Add(Panel, Button(TEXT("Play Online"), Go(3), true));
            Add(Panel, Button(TEXT("Start Server"), [this, GI]() { GI->AlphaBots = GI->OmegaBots = 0; OpenMenuPage(2); return FReply::Handled(); }, true));
        }
        else
        {
            Focus = Button(TEXT("Resume"), [this]() { ShowMenu(false); return FReply::Handled(); }, true); Add(Panel, Focus.ToSharedRef());
            Add(Panel, Button(TEXT("Character"), Go(5), true));
            Add(Panel, Button(TEXT("Loadout"), Go(6), true));
            Add(Panel, Button(TEXT("Main Menu"), [GI]() { GI->ReturnToMenu(); return FReply::Handled(); }, true));
        }
        Add(Panel, Button(TEXT("Setup"), Go(4), true));
        Add(Panel, Button(TEXT("Credits"), Go(7), true));
        Add(Panel, Button(TEXT("Exit Game"), [GI]() { GI->QuitGame(); return FReply::Handled(); }, true));
    }
    else
    {
        const TCHAR* Titles[] = {TEXT("MULTIPLAYER LOBBY"), TEXT("PLAY OFFLINE"), TEXT("START SERVER"), TEXT("PLAY ONLINE"), TEXT("SETUP"), TEXT("CHARACTER"), TEXT("LOADOUT"), TEXT("CREDITS"), TEXT("PERFORMANCE")};
        Focus = Button(MenuPage == 0 ? TEXT("Leave Server") : TEXT("Back"), MenuPage == 0 ? TFunction<FReply()>([GI]() { GI->ReturnToMenu(); return FReply::Handled(); }) : TFunction<FReply()>(Back));
        if (MenuPage != 7)
        {
            Add(Panel, Text(Titles[FMath::Clamp(MenuPage, 0, 8)], 36));
            Add(Panel, Focus.ToSharedRef());
        }
        if (MenuPage == 0 && Lobby)
        {
            auto Status = SNew(STextBlock).Font(MenuFont(25)).AutoWrapText(true).Text_Lambda([this, GI]()
            {
                auto* GS = GetWorld()->GetGameState<AERGameState>();
                int32 Humans = 0, Ready = 0;
                if (GS) for (auto Item : GS->PlayerArray) if (const auto* PS = Cast<AERPlayerState>(Item); PS && !PS->IsABot() && !PS->IsInactive())
                { ++Humans; Ready += PS->bLobbyReady; }
                return FText::FromString(GS && GS->bLobbyStarting ? TEXT("Starting match · generating map...") : FString::Printf(TEXT("%s · %d / %d players    |    READY %d / %d needed"), GS && GS->bOnline ? TEXT("INTERNET") : TEXT("LAN"), Humans, GS ? GS->ServerMaxPlayers : GI->MaxPlayers, Ready, Humans / 2 + 1));
            });
            Add(Panel, Status);
            auto Body = SNew(SHorizontalBox);
            auto Teams = SNew(SVerticalBox);
            for (bool Alpha : {true, false})
            {
                Add(Teams, SNew(STextBlock).Font(MenuFont(28)).ColorAndOpacity(Alpha ? FLinearColor(.35f,.65f,1) : FLinearColor(1,.5f,.22f)).Text(FText::FromString(Alpha ? TEXT("ALPHA") : TEXT("OMEGA"))));
                Add(Teams, SNew(SBox).MaxDesiredHeight(175)[SNew(SScrollBox) + SScrollBox::Slot()[SNew(STextBlock).Font(MenuFont(20)).AutoWrapText(true).Text_Lambda([this, Alpha]()
                {
                    FString Roster;
                    const auto* GS = GetWorld()->GetGameState<AERGameState>();
                    if (GS) for (auto Item : GS->PlayerArray) if (const auto* PS = Cast<AERPlayerState>(Item); PS && !PS->IsInactive() && (PS->Team == EERTeam::Alpha) == Alpha)
                    {
                        Roster += PS->GetPlayerName() + (PS->bLobbyReady ? TEXT("   READY") : TEXT("   preparing")) + TEXT("\n");
                    }
                    const int32 Bots = GS ? (Alpha ? GS->LobbyAlphaBots : GS->LobbyOmegaBots) : 0;
                    for (int32 I = 0; I < Bots; ++I) Roster += FString::Printf(TEXT("Bot %d   [test]\n"), I + 1);
                    return FText::FromString(Roster.IsEmpty() ? TEXT("No players yet") : Roster.TrimEnd());
                })]]);
            }
            auto Actions = SNew(SVerticalBox);
            Add(Actions, Button(TEXT("Name & Team"), Go(5)));
            Add(Actions, Button(TEXT("Loadout"), Go(6)));
            Add(Actions, Text(TEXT("Vote for the first map"), 25));
            for (int32 I = 0; I < ERMaps::All().Num(); ++I)
            {
                if (!ERMaps::At(I).InRotation) continue;
                Add(Actions, SNew(SButton).ContentPadding(7).OnClicked_Lambda([this, I]() { ServerLobbyVote(I); return FReply::Handled(); })
                    [SNew(STextBlock).Font(MenuFont(25)).AutoWrapText(true).Text_Lambda([this, I]()
                    {
                        int32 Votes = 0;
                        if (const auto* GS = GetWorld()->GetGameState<AERGameState>()) for (auto Item : GS->PlayerArray)
                            if (const auto* PS = Cast<AERPlayerState>(Item); PS && !PS->IsABot() && !PS->IsInactive() && PS->MapVote == I) ++Votes;
                        const auto* PS = GetPlayerState<AERPlayerState>();
                        return FText::FromString(FString::Printf(TEXT("%s   (%d)%s"), ERMaps::At(I).Label, Votes, PS && PS->MapVote == I ? TEXT("  YOUR VOTE") : TEXT("")));
                    })]);
            }
            
            Add(Actions, Text(TEXT("Ties, including no votes: random choice among the leading maps."), 20));
            Add(Actions, SNew(SButton).ContentPadding(12).OnClicked_Lambda([this]() { const auto* PS = GetPlayerState<AERPlayerState>(); if (PS) ServerLobbyReady(!PS->bLobbyReady); return FReply::Handled(); })
                [SNew(STextBlock).Font(MenuFont(32)).Text_Lambda([this]() { const auto* PS = GetPlayerState<AERPlayerState>(); return FText::FromString(PS && PS->bLobbyReady ? TEXT("READY ✓ · click to cancel") : TEXT("READY")); })]);
            Body->AddSlot().FillWidth(1).Padding(0,0,28,0)[Teams];
            Body->AddSlot().FillWidth(1.1f)[Actions];
            Add(Panel, Body);

        }
        else if (MenuPage == 1)
        {
            GI->SessionMode = 0;
            Field=Columns(Panel);
            IntChoice(TEXT("Map"), ERMaps::Labels(), &GI->MapChoice);
            IntChoice(TEXT("Your team"), {TEXT("Alpha"), TEXT("Omega")}, &GI->PlayerTeam);
            TArray<FString> BotCounts; for (int32 I = 0; I <= 10; ++I) BotCounts.Add(FString::FromInt(I));
            IntChoice(TEXT("Alpha bots"), BotCounts, &GI->AlphaBots);
            IntChoice(TEXT("Omega bots"), BotCounts, &GI->OmegaBots);
            IntChoice(TEXT("Bot behavior"), {TEXT("Stationary targets"), TEXT("Movement only"), TEXT("Movement and combat")}, &GI->BotBehavior);
            Add(Panel, Button(TEXT("Character"), Go(5)));
            Add(Panel, Button(TEXT("Loadout"), Go(6)));
            Add(Panel, Button(TEXT("Start Match"), [GI]() { GI->StartSession(); return FReply::Handled(); }));
        }
        else if (MenuPage == 2)
        {
            Field=Columns(Panel);
            TArray<FString> BotCounts; for (int32 I = 0; I <= 10; ++I) BotCounts.Add(FString::FromInt(I));
            IntChoice(TEXT("Alpha bots (testing)"), BotCounts, &GI->AlphaBots);
            IntChoice(TEXT("Omega bots (testing)"), BotCounts, &GI->OmegaBots);
            auto Capacity=Field();
            Add(Capacity, Text(TEXT("Maximum human players (including you)")));
            Add(Capacity, SNew(SNumericEntryBox<int32>).Font(MenuFont(25)).MinValue(1)
                .MinSliderValue(1).MaxSliderValue(64).AllowSpin(true)
                .Value_Lambda([GI]() { return TOptional<int32>(GI->MaxPlayers); })
                .OnValueChanged_Lambda([GI](int32 Value) { GI->MaxPlayers = ERServer::ValidMaxPlayers(Value); })
                .OnValueCommitted_Lambda([GI](int32 Value, ETextCommit::Type) { GI->MaxPlayers = ERServer::ValidMaxPlayers(Value); GI->SaveServerSettings(); }));
            Choice(Field(), TEXT("Connection"), {TEXT("Internet (Epic Online Services)"), TEXT("LAN")},
                [GI]() { return GI->bInternetMenu ? 0 : 1; },
                [GI, Online](int32 I) { Online->Cancel(); GI->CancelLANSearch(); GI->bInternetMenu = I == 0; });
            Add(Panel, Text(TEXT("Internet: no account required. LAN: same local network. Test bots are additional."), 21));
            Add(Panel, SNew(STextBlock).Font(MenuFont(20)).AutoWrapText(true)
                .Visibility_Lambda([Online]() { return Online->Status.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
                .Text_Lambda([Online]() { return FText::FromString(Online->Status); }));
            Add(Panel, Button(TEXT("Start Server & Enter Lobby"), [GI, Online]() { if (GI->bInternetMenu) Online->Host(); else { Online->Cancel(); GI->StartLANServer(); } return FReply::Handled(); }));
        }
        else if (MenuPage == 3)
        {
            Choice(Panel, TEXT("Connection"), {TEXT("Internet (Epic Online Services)"), TEXT("LAN")},
                [GI]() { return GI->bInternetMenu ? 0 : 1; },
                [GI, Online](int32 I) { Online->Cancel(); GI->CancelLANSearch(); GI->bInternetMenu = I == 0; });
            Add(Panel, SNew(STextBlock).Font(MenuFont(28)).AutoWrapText(true).Text_Lambda([GI, Online]()
            { return FText::FromString(GI->bInternetMenu ? Online->Status : GI->IsSearchingLAN() ? FString::Printf(TEXT("Searching LAN... %ds"), GI->LANSecondsLeft()) : GI->LastError); }));
            Add(Panel, Text(TEXT("Internet: no account required; choose a server. LAN: we connect automatically to an available local server."), 21));
            auto Servers = SNew(SVerticalBox);
            for (int32 I = 0; I < 100; ++I)
                Add(Servers, SNew(SButton).ContentPadding(8)
                    .Visibility_Lambda([GI, Online, I]() { return GI->bInternetMenu && Online->Results.IsValidIndex(I) ? EVisibility::Visible : EVisibility::Collapsed; })
                    .IsEnabled_Lambda([Online]() { return !Online->IsBusy(); })
                    .OnClicked_Lambda([Online, I]() { Online->Join(I); return FReply::Handled(); })
                    [SNew(STextBlock).Font(MenuFont(25)).AutoWrapText(true).Text_Lambda([Online, I]()
                    {
                        if (!Online->Results.IsValidIndex(I)) return FText::GetEmpty();
                        const auto& Session = Online->Results[I].Session;
                        FString HostName;
                        Session.SessionSettings.Get(FName(TEXT("RR_HOST")), HostName);
                        HostName = AERGameMode::SanitizeName(HostName);
                        return FText::FromString(FString::Printf(TEXT("Join %s · %d / %d players"), *HostName,
                            Session.SessionSettings.NumPublicConnections - Session.NumOpenPublicConnections, Session.SessionSettings.NumPublicConnections));
                    })]);
            Add(Panel, SNew(SBox).MaxDesiredHeight(240)[SNew(SScrollBox) + SScrollBox::Slot()[Servers]]);
            Add(Panel, Button(TEXT("Search Servers"), [GI, Online]() { if (GI->bInternetMenu) Online->Search(); else { Online->Cancel(); GI->FindLANServer(); } return FReply::Handled(); }));
            Add(Panel, Button(TEXT("Cancel"), Back));
        }
        else if (MenuPage == 4)
        {
            Add(Panel, Button(TEXT("Performance"), Go(8)));
            Field=Columns(Panel);
            auto Display=Field();
            auto Displays = SNew(SVerticalBox);
            for (int32 Percent : {50,60,70,80,90,100})
                Add(Displays, Button(Percent == 100 ? TEXT("Fullscreen") : FString::Printf(TEXT("Windowed %d%%"), Percent), [this, Percent]() { ApplyDisplaySize(Percent); return FReply::Handled(); }));
            Add(Display, Text(TEXT("Display")));
            Add(Display, SAssignNew(DisplaySizeSelector, SComboButton)
                .ButtonContent()[SNew(STextBlock).Font(MenuFont(25)).Text_Lambda([this]() { return GetDisplaySizeText(); })].MenuContent()[Displays]);
            auto Slider = [Field](const FString& Label, TFunction<float()> Get, TFunction<void(float)> Set, TFunction<void()> Save)
            {
                auto Cell=Field();
                Add(Cell, SNew(STextBlock).Font(MenuFont(24)).Text_Lambda([Label, Get]() { return FText::FromString(FString::Printf(TEXT("%s · %.0f%%"), *Label, Get() * 100)); }));
                Add(Cell, SNew(SBox).HeightOverride(32)[SNew(SSlider).Value_Lambda([Get]() { return Get(); }).StepSize(.01f)
                    .OnValueChanged_Lambda([Set, Save](float V) { Set(V); Save(); })]);
            };
            Slider(TEXT("Sound volume"), [GI]() { return GI->GetSoundVolume(); }, [GI](float V) { GI->SetSoundVolume(V); }, [GI]() { GI->SaveMusicVolume(); });
            Slider(TEXT("Music"), [GI]() { return GI->GetMusicVolume(); }, [GI](float V) { GI->SetMusicVolume(V); }, [GI]() { GI->SaveMusicVolume(); });
            Slider(TEXT("Mouse sensitivity"), [GI]() { return GI->GetMouseSensitivity(); }, [GI](float V) { GI->SetMouseSensitivity(V); }, [GI]() { GI->SaveMouseSensitivity(); });
            Slider(TEXT("Mouse acceleration"), [GI]() { return GI->GetMouseAcceleration(); }, [GI](float V) { GI->SetMouseAcceleration(V); }, [GI]() { GI->SaveMouseSensitivity(); });
            Add(Panel, Text(TEXT("WASD Move · Shift Walk · double W / hold E Sprint\nSpace Jump / Climb · C / Ctrl Crouch · double C Prone\nR Reload · Tab Scores · Esc Menu"), 20));
            Add(Panel, Button(TEXT("Credits"), Go(7)));
        }
        else if (MenuPage == 8)
        {
            auto* Settings=&ERPerformance::Get();
            auto Apply=[this,Settings]() { Settings->Defaults=0; Settings->Apply(); Settings->Save(); ResetRenderResolution(); };
            auto Action=Columns(Panel);
            Add(Action(), Button(TEXT("Apply Performance preset"), [this,Settings]() { Settings->PerformancePreset(); ResetRenderResolution(); return FReply::Handled(); }));
            Add(Action(), Button(TEXT("Set default"), [this,Settings]() { Settings->SetDefaults(); ResetRenderResolution(); OpenMenuPage(8); return FReply::Handled(); }));
            Add(Panel,Text(TEXT("Default restores the original project graphics and automatic resolution."),20));
            Field=Columns(Panel);
            auto Option=[Field,Apply](const FString& Label,TArray<FString> Labels,int32* Value)
            { Choice(Field(),Label,Labels,[Value]() { return *Value; },[Value,Apply](int32 I) { *Value=I; Apply(); }); };
            Option(TEXT("Internal resolution"),{TEXT("Manual"),TEXT("Automatic · 60 FPS / max 1080p")},&Settings->Automatic);
            auto Scale=Field();
            Add(Scale,SNew(STextBlock).Font(MenuFont(24)).Text_Lambda([Settings]() { return FText::FromString(FString::Printf(TEXT("Manual internal scale · %d%%"),Settings->Scale)); }));
            Add(Scale,SNew(SSlider).MinValue(1).MaxValue(100).StepSize(1)
                .IsEnabled_Lambda([Settings]() { return !Settings->Automatic; })
                .Value_Lambda([Settings]() { return float(Settings->Scale); })
                .OnValueChanged_Lambda([Settings,Apply](float V) { Settings->Scale=FMath::Clamp(FMath::RoundToInt(V),1,100); Apply(); }));
            Add(Scale,Text(TEXT("Manual scale is relative to the window; HUD stays sharp. Very low scales lose detail."),20));
            Option(TEXT("Antialiasing / upscaling"),{TEXT("TSR"),TEXT("TAA · spatial upscale"),TEXT("TAAU · temporal upscale")},&Settings->AA);
            Option(TEXT("Global illumination"),{TEXT("Off · no Lumen"),TEXT("Lumen")},&Settings->GI);
            Option(TEXT("Hardware ray tracing (supported GPUs only)"),{TEXT("Off"),TEXT("Enabled when supported")},&Settings->HardwareRT);
            Option(TEXT("Shadows · resolution and distance"),{TEXT("Off"),TEXT("Low"),TEXT("Medium"),TEXT("High")},&Settings->Shadows);
            Option(TEXT("Reflections"),{TEXT("Off"),TEXT("Screen space · simple"),TEXT("Lumen")},&Settings->Reflections);
            Option(TEXT("Motion blur"),{TEXT("Off"),TEXT("On")},&Settings->MotionBlur);
            Option(TEXT("Depth of field"),{TEXT("Off"),TEXT("On")},&Settings->DepthOfField);
            Option(TEXT("Decorative effects"),{TEXT("Minimum"),TEXT("Low"),TEXT("Medium"),TEXT("High")},&Settings->Effects);
            Option(TEXT("Textures · choose according to GPU memory"),{TEXT("Low"),TEXT("Medium"),TEXT("High"),TEXT("Ultra")},&Settings->Textures);
            Add(Panel,Text(TEXT("Changes apply immediately and are saved. Texture streaming uses the hardware memory budget. Gameplay smoke remains unchanged."),20));
            Add(Panel,Button(TEXT("Reset Automatic Resolution"),[this]() { ResetRenderResolution(); return FReply::Handled(); }));
        }
        else if (MenuPage == 5)
        {
            auto CharacterPanel = SNew(SVerticalBox);
            auto CharacterBody = SNew(SHorizontalBox);
            Add(CharacterPanel, Text(TEXT("Player name")));
            Add(CharacterPanel, SAssignNew(NameBox, SEditableTextBox).Font(MenuFont(26)).Text(FText::FromString(GI->LocalName))
                .OnTextCommitted_Lambda([this, GI](const FText& Value, ETextCommit::Type) { GI->LocalName = Value.ToString(); GI->SaveProfile(); ServerSetName(GI->LocalName); }));
            Choice(CharacterPanel, TEXT("Team"), {TEXT("Alpha"), TEXT("Omega")}, [this, GI]() { const auto* PS = GetPlayerState<AERPlayerState>(); return IsInLobby() && PS ? int32(PS->Team == EERTeam::Omega) : GI->PlayerTeam; },
                [this, GI](int32 I) { GI->PlayerTeam = I; GI->SaveProfile(); if (IsInLobby()) ServerLobbyTeam(I); OpenMenuPage(5); });
            Add(Panel,CharacterPanel);
            Add(Panel,Text(TEXT("Your role determines your appearance. Choose a preset in Loadout."),20));
            Add(Panel,Button(TEXT("Loadout"),Go(6)));
        }
        else if (MenuPage == 6)
        {
            if(DraftPreset.IsNone()) { DraftPreset=GI->LoadoutPreset;DraftVariant=GI->LoadoutVariant; }
            const auto* PS=(!Main || Lobby)?GetPlayerState<AERPlayerState>():nullptr;
            const bool Omega=PS && PS->Team!=EERTeam::None ? PS->Team==EERTeam::Omega:GI->PlayerTeam==1;
            auto Body=SNew(SHorizontalBox);auto Roles=SNew(SVerticalBox);auto Variants=SNew(SVerticalBox);
            if(Catalog && !MenuPreview)MenuPreview=GetWorld()->SpawnActor<AERMenuPreview>(FVector(0,0,-100000),FRotator::ZeroRotator);
            PortraitBrushes.Empty();
            for(const FName Preset:ERLoadouts::Presets())
            {
                const auto Kit=ResolveMenuLoadout(Preset,DraftVariant);
                auto Card=SNew(SHorizontalBox);
                if(Catalog && MenuPreview)
                {
                    const FName Key(*(Preset.ToString()+(Omega?TEXT(".omega."):TEXT(".alpha."))+FString::FromInt(Kit.Skin)));
                    auto& Portrait=LoadoutPortraits.FindOrAdd(Key);
                    if(!Portrait)Portrait=MenuPreview->MakePortrait(ERSkins::Select(Omega?Catalog->OmegaSkins:Catalog->AlphaSkins,Omega?Catalog->OmegaAppearance:Catalog->AlphaAppearance,Kit.Skin));
                    auto Brush=MakeShared<FSlateBrush>();Brush->SetResourceObject(Portrait);Brush->ImageSize=FVector2D(48,60);PortraitBrushes.Add(Brush);
                    Card->AddSlot().AutoWidth().Padding(0,0,8,0)[SNew(SBox).WidthOverride(48).HeightOverride(60)[SNew(SImage).Image(&Brush.Get())]];
                }
                Card->AddSlot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Font(MenuFont(21)).Text_Lambda([this,Preset,Kit]() { return FText::FromString((DraftPreset==Preset?TEXT("[Selected] "):TEXT(""))+Kit.Role+TEXT(" · ")+ERLoadouts::WeaponName(Kit.Items[0].WeaponKey)); }).AutoWrapText(true)];
                auto RoleBorder=SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor_Lambda([this,Preset]() { return DraftPreset==Preset?FLinearColor(.9f,.55f,.15f):FLinearColor(.12f,.14f,.17f); }).Padding(2)
                    [SNew(SBorder).BorderBackgroundColor(FLinearColor(.06f,.07f,.09f)).Padding(4)[Card]];
                Add(Roles,SNew(SButton).ContentPadding(4).OnClicked_Lambda([this,Preset]() { DraftPreset=Preset;RefreshLoadoutPreview();return FReply::Handled(); })[RoleBorder]);
            }
            Body->AddSlot().FillWidth(.8f).Padding(0,0,14,0)[Roles];
            if(MenuPreview)
            {
                RefreshLoadoutPreview();
                PreviewBrush=MakeShared<FSlateBrush>();PreviewBrush->SetResourceObject(MenuPreview->Target);PreviewBrush->ImageSize=FVector2D(512,640);
                auto PreviewPanel=SNew(SVerticalBox);
                Add(PreviewPanel,SNew(SBox).HeightOverride(640)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(PreviewBrush.Get())]]);
                Add(PreviewPanel,SNew(STextBlock).Font(MenuFont(20)).Text(FText::FromString(TEXT("Preview unavailable")))
                    .Visibility_Lambda([this]() { return MenuPreview && MenuPreview->bPreviewAvailable?EVisibility::Collapsed:EVisibility::Visible; }));
                Body->AddSlot().FillWidth(1.2f)[SNew(SBorder)
                    .OnMouseMove_Lambda([this](const FGeometry&,const FPointerEvent& E) { if(MenuPreview && E.IsMouseButtonDown(EKeys::LeftMouseButton)) { MenuPreview->Rotate(E.GetCursorDelta().X*.5f);return FReply::Handled(); }return FReply::Unhandled(); })
                    [PreviewPanel]];
            }
            else Body->AddSlot().FillWidth(1.2f)[Text(TEXT("Preview unavailable"))];
            for(const FName Variant:ERLoadouts::Variants())
                Add(Variants,SNew(SButton).ContentPadding(10).OnClicked_Lambda([this,Variant]() { DraftVariant=Variant;RefreshLoadoutPreview();return FReply::Handled(); })
                    [SNew(STextBlock).Font(MenuFont(20)).AutoWrapText(true).Text_Lambda([this,Variant,Omega]()
                    {
                        const auto Kit=ResolveMenuLoadout(DraftPreset,Variant);
                        const FString Name=Variant==FName("gunner")?TEXT("Gunner"):Variant==FName("offense")?TEXT("Offense"):TEXT("Defense");
                        const FString Recipe=Variant==FName("gunner")?(DraftPreset==FName("grenadier")?TEXT("Utility: None"):TEXT("SMG · Utility: None")):Variant==FName("offense")?TEXT("HE x2 · Smoke x1"):TEXT("Smoke x2 · Claymore x1");
                        return FText::FromString((DraftVariant==Variant?TEXT("[Selected] "):TEXT(""))+Name+TEXT("\n")+Recipe+(DraftPreset==FName("grenadier")?FString::Printf(TEXT(" · SMG · Milkor loaded %d / reserve 0"),Kit.Items[0].Loaded):Variant!=FName("gunner")?TEXT(" · Pistol"):TEXT("")));
                    })]);
            Add(Variants,SNew(STextBlock).Font(MenuFont(20)).AutoWrapText(true).Text_Lambda([this,Omega]() { return FText::FromString(ResolveMenuLoadout(DraftPreset,DraftVariant).Summary()); }));
            Add(Variants,Text(TEXT("Placed mines last 60 seconds. Drag to rotate."),19));
            Body->AddSlot().FillWidth(1).Padding(14,0,0,0)[Variants];Add(Panel,Body);
            Add(Panel,SNew(STextBlock).Font(MenuFont(22)).AutoWrapText(true).Text_Lambda([this,GI]()
            {
                const auto* State=(!IsMenuLevel() || IsInLobby())?GetPlayerState<AERPlayerState>():nullptr;const auto* C=Cast<AERCharacter>(GetPawn());
                const auto Next=ERLoadouts::Resolve(State?State->LoadoutPreset:GI->LoadoutPreset,State?State->LoadoutVariant:GI->LoadoutVariant);
                FString Status=TEXT("Next spawn: ")+Next.Role+TEXT(" / ")+Next.Variant.ToString();
                if(C) { const auto Current=ERLoadouts::Resolve(C->ActivePreset,C->ActiveVariant);Status=TEXT("Current loadout: ")+Current.Role+TEXT(" / ")+Current.Variant.ToString()+TEXT(" · ")+Status; }
                if(bLoadoutPending)Status+=TEXT(" · Waiting for server");
                return FText::FromString(Status);
            }));
            Add(Panel,Button(Main && !Lobby?TEXT("Apply loadout"):TEXT("Apply for next spawn"),[this,GI,Main,Lobby]()
            {
                if(Main && !Lobby) { const auto Kit=ERLoadouts::Resolve(DraftPreset,DraftVariant);GI->LoadoutPreset=Kit.Preset;GI->LoadoutVariant=Kit.Variant;GI->SaveLoadout(); }
                else { bLoadoutPending=true;ServerSetLoadout(DraftPreset,DraftVariant); }
                return FReply::Handled();
            }));
        }
        else if (MenuPage == 7)
        {
            FString Credits;
            if (FFileHelper::LoadFileToString(Credits, *(FPaths::ProjectDir() / TEXT("Branding/Credits.txt"))))
            {
                Credits.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
                TArray<FString> Sections;
                Credits.ParseIntoArray(Sections, TEXT("\n\n"), true);
                for (const FString& Section : Sections)
                {
                    FString Title, Body;
                    Section.Split(TEXT("\n"), &Title, &Body);
                    auto Heading = Text(Title, 30);
                    Heading->SetColorAndOpacity(FLinearColor(1.f, .68f, .22f));
                    Panel->AddSlot().AutoHeight().Padding(0, 18, 0, 5)[Heading];
                    Add(Panel, Text(Body.TrimEnd(), 24));
                }
            }
            else
            {
                Add(Panel, Text(TEXT("Credits could not be loaded.")));
                UE_LOG(LogTemp, Warning, TEXT("Missing Branding/Credits.txt"));
            }
        }
    }
    static const FSlateDynamicImageBrush Background(FName(*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Branding/background.jpg"))), FVector2D(2560,1441));
    auto Overlay = SNew(SOverlay);
    if (Main) Overlay->AddSlot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFill).Clipping(EWidgetClipping::ClipToBounds).Visibility(EVisibility::HitTestInvisible)[SNew(SImage).Image(&Background)]];
    if (Home)
        Overlay->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(24,30,70,30)[SNew(SBox).WidthOverride(360)[Panel]];
    else
    {
        TSharedRef<SWidget> Content = SNew(SScrollBox) + SScrollBox::Slot()[Panel];
        if(MenuPage==6)
            Content=SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1200)[Panel]];
        if (MenuPage == 7)
            Content = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[Text(TEXT("CREDITS"), 36)]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[Focus.ToSharedRef()]
                + SVerticalBox::Slot().FillHeight(1)[Content];
        Overlay->AddSlot().HAlign(HAlign_Fill).VAlign(VAlign_Center).Padding(70,50)
            [SNew(SBox).MaxDesiredHeight(850)
                [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.025f,.03f,.04f,.94f)).ForegroundColor(FLinearColor::White).Padding(26)
                    [Content]]];
    }
    if (MenuPage != 3) Overlay->AddSlot().VAlign(VAlign_Bottom).Padding(30,8,200,10)[SNew(STextBlock).Font(MenuFont(22)).ColorAndOpacity(FLinearColor(1,.65f,.3f)).ShadowOffset(FVector2D(2,2)).ShadowColorAndOpacity(FLinearColor::Black).AutoWrapText(true).Text_Lambda([GI]() { return FText::FromString(GI->LastError); })];
    auto Version=Text(GI->BuildVersion,16);
    Version->SetAutoWrapText(false);
    Overlay->AddSlot()
        [SNew(SSafeZone).IsTitleSafe(true).HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(32)
            .Visibility(EVisibility::HitTestInvisible)[Version]];
    MenuWidget = Overlay;
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 20);
    MenuFocusWidget = Focus;
    ApplyWindowInputMode();
}

void AERPlayerController::RefreshLoadoutPreview()
{
    if(!MenuPreview || !SkinCatalog)return;
    const auto* Catalog=SkinCatalog->GetDefaultObject<AERCharacter>();
    const auto* PS=(!IsMenuLevel() || IsInLobby())?GetPlayerState<AERPlayerState>():nullptr;const auto* GI=GetGameInstance<UERGameInstance>();
    const bool Omega=PS && PS->Team!=EERTeam::None?PS->Team==EERTeam::Omega:GI->PlayerTeam==1;
    const auto Kit=ResolveMenuLoadout(DraftPreset,DraftVariant);
    MenuPreview->ShowLoadout(ERSkins::Select(Omega?Catalog->OmegaSkins:Catalog->AlphaSkins,Omega?Catalog->OmegaAppearance:Catalog->AlphaAppearance,Kit.Skin),Kit,Catalog->ReferenceArsenal);
}

FERResolvedLoadout AERPlayerController::ResolveMenuLoadout(FName Preset,FName Variant) const
{
    const auto* GI=GetGameInstance<UERGameInstance>();
    const auto* PS=(!IsMenuLevel() || IsInLobby())?GetPlayerState<AERPlayerState>():nullptr;
    const bool Omega=PS && PS->Team!=EERTeam::None?PS->Team==EERTeam::Omega:GI->PlayerTeam==1;
    const auto* Catalog=SkinCatalog?SkinCatalog->GetDefaultObject<AERCharacter>():nullptr;
    const auto Local=ERLoadouts::Resolve(Preset,Variant,Omega,Catalog);
    if(!IsMenuLevel() || IsInLobby())if(const auto* State=GetWorld()->GetGameState<AERGameState>())
        if(const auto* Server=State->LoadoutCatalog.FindByPredicate([&](const auto& Kit) { return Kit.Preset==Local.Preset && Kit.Variant==Local.Variant && Kit.bOmega==Omega; }))return *Server;
    return Local;
}
