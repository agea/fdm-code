#include "ERPlayerController.h"
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
void Choice(TSharedRef<SVerticalBox> Panel, const FString& Title, TArray<FString> Labels,
    TFunction<int32()> Get, TFunction<void(int32)> Set)
{
    Add(Panel, Text(Title, 22));
    auto Items = SNew(SVerticalBox);
    for (int32 I = 0; I < Labels.Num(); ++I)
        Add(Items, Button(Labels[I], [I, Set]() { Set(I); FSlateApplication::Get().DismissAllMenus(); return FReply::Handled(); }));
    Add(Panel, SNew(SComboButton)
        .ButtonContent()[SNew(STextBlock).Font(MenuFont(25)).Text_Lambda([Labels, Get]() { return FText::FromString(Labels[FMath::Clamp(Get(), 0, Labels.Num()-1)]); })]
        .MenuContent()[SNew(SBox).MaxDesiredHeight(420)[SNew(SScrollBox) + SScrollBox::Slot()[Items]]]);
}
}
void AERPlayerController::ERMenu(int32 Page) { OpenMenuPage(FMath::Clamp(Page, 0, 7)); }
void AERPlayerController::OpenMenuPage(int32 Page)
{
    if ((Page == 5 || Page == 6 || Page == 7) && MenuPage != 5 && MenuPage != 6 && MenuPage != 7) SubmenuParent = MenuPage;
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
    if (MenuPreview) { MenuPreview->Destroy(); MenuPreview = nullptr; }
    PreviewBrush.Reset();
    if (!bShow) { SetInputMode(FInputModeGameOnly()); return; }
    auto* GI = GetGameInstance<UERGameInstance>(); if (!GI) return;
    const bool Main = IsMenuLevel();
    const bool Lobby = bShowingLobby;
    if (Lobby) GI->LastError.Empty();
    const bool Home = MenuPage == 0 && !Lobby;
    auto Panel = SNew(SVerticalBox);
    TSharedPtr<SWidget> Focus;
    auto Go = [this](int32 Page) { return [this, Page]() { OpenMenuPage(Page); return FReply::Handled(); }; };
    auto* Online = GI->GetSubsystem<UEROnline>();
    auto Back = [this, GI, Online]() { Online->Cancel(); GI->CancelLANSearch(); OpenMenuPage(MenuPage == 5 || MenuPage == 6 || MenuPage == 7 ? SubmenuParent : 0); return FReply::Handled(); };
    auto SaveSupplies = [this, GI]() { GI->SaveProfile(); ServerSetSupplies(GI->SpareMagazines, GI->HECount, GI->SmokeCount, GI->StunCount); };
    auto IntChoice = [Panel](const FString& Label, TArray<FString> Labels, int32* Value)
    { Choice(Panel, Label, Labels, [Value]() { return *Value; }, [Value](int32 I) { *Value = I; }); };
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
            Add(Panel, Button(TEXT("Equipment"), Go(6), true));
            Add(Panel, Button(TEXT("Main Menu"), [GI]() { GI->ReturnToMenu(); return FReply::Handled(); }, true));
        }
        Add(Panel, Button(TEXT("Setup"), Go(4), true));
        Add(Panel, Button(TEXT("Credits"), Go(7), true));
        Add(Panel, Button(TEXT("Exit Game"), [GI]() { GI->QuitGame(); return FReply::Handled(); }, true));
    }
    else
    {
        const TCHAR* Titles[] = {TEXT("MULTIPLAYER LOBBY"), TEXT("PLAY OFFLINE"), TEXT("START SERVER"), TEXT("PLAY ONLINE"), TEXT("SETUP"), TEXT("CHARACTER"), TEXT("EQUIPMENT"), TEXT("CREDITS")};
        Focus = Button(MenuPage == 0 ? TEXT("Leave Server") : TEXT("Back"), MenuPage == 0 ? TFunction<FReply()>([GI]() { GI->ReturnToMenu(); return FReply::Handled(); }) : TFunction<FReply()>(Back));
        if (MenuPage != 7)
        {
            Add(Panel, Text(Titles[FMath::Clamp(MenuPage, 0, 7)], 36));
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
                Add(Teams, SNew(SBox).MaxDesiredHeight(175)[SNew(SScrollBox) + SScrollBox::Slot()[SNew(STextBlock).Font(MenuFont(23)).AutoWrapText(true).Text_Lambda([this, Alpha]()
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
            Add(Actions, Button(TEXT("Name, Team & Skin"), Go(5)));
            Add(Actions, Button(TEXT("Equipment"), Go(6)));
            Add(Actions, Text(TEXT("Vote for the first map"), 25));
            for (int32 I = 0; I < ERMaps::All().Num(); ++I)
            {
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
            IntChoice(TEXT("Map"), ERMaps::Labels(), &GI->MapChoice);
            IntChoice(TEXT("Your team"), {TEXT("Alpha"), TEXT("Omega")}, &GI->PlayerTeam);
            TArray<FString> BotCounts; for (int32 I = 0; I <= 10; ++I) BotCounts.Add(FString::FromInt(I));
            IntChoice(TEXT("Alpha bots"), BotCounts, &GI->AlphaBots);
            IntChoice(TEXT("Omega bots"), BotCounts, &GI->OmegaBots);
            IntChoice(TEXT("Bot behavior"), {TEXT("Stationary targets"), TEXT("Movement only"), TEXT("Movement and combat")}, &GI->BotBehavior);
            Add(Panel, Button(TEXT("Character"), Go(5)));
            Add(Panel, Button(TEXT("Equipment"), Go(6)));
            Add(Panel, Button(TEXT("Start Match"), [GI]() { GI->StartSession(); return FReply::Handled(); }));
        }
        else if (MenuPage == 2)
        {
            TArray<FString> BotCounts; for (int32 I = 0; I <= 10; ++I) BotCounts.Add(FString::FromInt(I));
            IntChoice(TEXT("Alpha bots (testing)"), BotCounts, &GI->AlphaBots);
            IntChoice(TEXT("Omega bots (testing)"), BotCounts, &GI->OmegaBots);
            Add(Panel, Text(TEXT("Maximum human players (including you)")));
            Add(Panel, SNew(SNumericEntryBox<int32>).Font(MenuFont(25)).MinValue(1)
                .MinSliderValue(1).MaxSliderValue(64).AllowSpin(true)
                .Value_Lambda([GI]() { return TOptional<int32>(GI->MaxPlayers); })
                .OnValueChanged_Lambda([GI](int32 Value) { GI->MaxPlayers = ERServer::ValidMaxPlayers(Value); })
                .OnValueCommitted_Lambda([GI](int32 Value, ETextCommit::Type) { GI->MaxPlayers = ERServer::ValidMaxPlayers(Value); GI->SaveServerSettings(); }));
            Choice(Panel, TEXT("Connection"), {TEXT("Internet (Epic Online Services)"), TEXT("LAN")},
                [GI]() { return GI->bInternetMenu ? 0 : 1; },
                [GI, Online](int32 I) { Online->Cancel(); GI->CancelLANSearch(); GI->bInternetMenu = I == 0; });
            Add(Panel, Text(TEXT("Internet: sign in to Epic Games. LAN: same local network. Test bots are additional."), 21));
            Add(Panel, SNew(STextBlock).Font(MenuFont(23)).AutoWrapText(true)
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
            Add(Panel, Text(TEXT("Internet: sign in to Epic Games and choose a server. LAN: we connect automatically to an available local server."), 21));
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
                        return FText::FromString(FString::Printf(TEXT("Join %s · %d / %d players"), *Session.OwningUserName,
                            Session.SessionSettings.NumPublicConnections - Session.NumOpenPublicConnections, Session.SessionSettings.NumPublicConnections));
                    })]);
            Add(Panel, SNew(SBox).MaxDesiredHeight(240)[SNew(SScrollBox) + SScrollBox::Slot()[Servers]]);
            Add(Panel, Button(TEXT("Search Servers"), [GI, Online]() { if (GI->bInternetMenu) Online->Search(); else { Online->Cancel(); GI->FindLANServer(); } return FReply::Handled(); }));
            Add(Panel, Button(TEXT("Cancel"), Back));
        }
        else if (MenuPage == 4)
        {
            auto Displays = SNew(SVerticalBox);
            for (int32 Percent : {50,60,70,80,90,100})
                Add(Displays, Button(Percent == 100 ? TEXT("Fullscreen") : FString::Printf(TEXT("Windowed %d%%"), Percent), [this, Percent]() { ApplyDisplaySize(Percent); return FReply::Handled(); }));
            Add(Panel, Text(TEXT("Display")));
            Add(Panel, SAssignNew(DisplaySizeSelector, SComboButton)
                .ButtonContent()[SNew(STextBlock).Font(MenuFont(25)).Text_Lambda([this]() { return GetDisplaySizeText(); })].MenuContent()[Displays]);
            Add(Panel, Text(TEXT("3D resolution adapts automatically to target 60 FPS. Display settings are saved."), 21));
            Add(Panel, Button(TEXT("Reset Automatic Resolution"), [this]() { ResetRenderResolution(); return FReply::Handled(); }));
            auto Slider = [Panel](const FString& Label, TFunction<float()> Get, TFunction<void(float)> Set, TFunction<void()> Save)
            {
                Add(Panel, SNew(STextBlock).Font(MenuFont(24)).Text_Lambda([Label, Get]() { return FText::FromString(FString::Printf(TEXT("%s · %.0f%%"), *Label, Get() * 100)); }));
                Add(Panel, SNew(SBox).HeightOverride(32)[SNew(SSlider).Value_Lambda([Get]() { return Get(); }).StepSize(.01f)
                    .OnValueChanged_Lambda([Set, Save](float V) { Set(V); Save(); })]);
            };
            Slider(TEXT("Sound volume"), [GI]() { return GI->GetSoundVolume(); }, [GI](float V) { GI->SetSoundVolume(V); }, [GI]() { GI->SaveMusicVolume(); });
            Slider(TEXT("Music"), [GI]() { return GI->GetMusicVolume(); }, [GI](float V) { GI->SetMusicVolume(V); }, [GI]() { GI->SaveMusicVolume(); });
            Slider(TEXT("Mouse sensitivity"), [GI]() { return GI->GetMouseSensitivity(); }, [GI](float V) { GI->SetMouseSensitivity(V); }, [GI]() { GI->SaveMouseSensitivity(); });
            Slider(TEXT("Mouse acceleration"), [GI]() { return GI->GetMouseAcceleration(); }, [GI](float V) { GI->SetMouseAcceleration(V); }, [GI]() { GI->SaveMouseSensitivity(); });
            Add(Panel, Text(TEXT("WASD Move · Shift Walk · double W / hold E Sprint\nSpace Jump / Climb · C / Ctrl Crouch · double C Prone\nR Reload · Tab Scores · Esc Menu"), 20));
            Add(Panel, Button(TEXT("Credits"), Go(7)));
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
            const bool Alpha = GI->PlayerTeam == 0;
            TArray<FString> Labels;
            for (int32 I = 0; I < ERSkins::Count; ++I)
            {
                const auto* Options = Catalog ? (Alpha ? &Catalog->AlphaSkins : &Catalog->OmegaSkins) : nullptr;
                Labels.Add(Options && Options->IsValidIndex(I) ? (*Options)[I].DisplayName : FString::Printf(TEXT("Skin %d"), I + 1));
            }
            Choice(CharacterPanel, TEXT("Skin"), Labels, [GI, Alpha]() { return Alpha ? GI->AlphaSkin : GI->OmegaSkin; },
                [this, GI, Alpha](int32 I) { (Alpha ? GI->AlphaSkin : GI->OmegaSkin) = I; GI->SaveSkins(); ServerSetSkins(GI->AlphaSkin, GI->OmegaSkin); OpenMenuPage(5); });
            if (Catalog)
            {
                const auto& Appearance = ERSkins::Select(Alpha ? Catalog->AlphaSkins : Catalog->OmegaSkins, Alpha ? Catalog->AlphaAppearance : Catalog->OmegaAppearance, Alpha ? GI->AlphaSkin : GI->OmegaSkin);
                MenuPreview = GetWorld()->SpawnActor<AERMenuPreview>(FVector(0,0,-100000), FRotator::ZeroRotator);
                MenuPreview->ShowAppearance(Appearance);
                PreviewBrush = MakeShared<FSlateBrush>(); PreviewBrush->SetResourceObject(MenuPreview->Target);
                PreviewBrush->ImageSize = FVector2D(256,320);
                CharacterBody->AddSlot().FillWidth(1).Padding(20,0,0,0)
                    [SNew(SBox).HeightOverride(440)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(PreviewBrush.Get())]]];
            }
            CharacterBody->InsertSlot(0).FillWidth(1.1f)[CharacterPanel];
            Add(Panel, CharacterBody);
            Add(Panel, Text(Lobby ? TEXT("Changing team or skin clears your READY vote.") : TEXT("In a match, appearance changes apply at your next spawn."), 20));
        }
        else if (MenuPage == 6)
        {
            const auto* PawnCatalog = Catalog;
            const bool Reference = PawnCatalog && PawnCatalog->ReferenceArsenal.Num() == 10;
            TArray<FString> Weapons;
            if (Reference) for (int32 I = 0; I < 10; ++I) Weapons.Add(PawnCatalog->ReferenceArsenal[I]->DisplayName);
            else for (auto Weapon : PrimaryOptions) if (Weapon) Weapons.Add(Weapon->DisplayName);
            if (Weapons.Num()) Choice(Panel, Reference ? TEXT("Starting weapon (all weapons available)") : TEXT("Primary weapon"), Weapons,
                [GI, Reference]() { return Reference ? GI->ReferenceStartingWeapon : GI->PrimaryWeapon; }, [this, GI, Reference](int32 I) { (Reference ? GI->ReferenceStartingWeapon : GI->PrimaryWeapon) = I; GI->SaveLoadout(); ServerSetPrimary(I); });
            for (auto Pair : {TPair<FString, int32*>(TEXT("Spare magazines per weapon"), &GI->SpareMagazines),
                {TEXT("HE grenades"), &GI->HECount}, {TEXT("Smoke grenades"), &GI->SmokeCount}, {TEXT("Stun grenades"), &GI->StunCount}})
                if (!Reference || Pair.Value != &GI->StunCount) Choice(Panel, Pair.Key, Counts, [Value=Pair.Value]() { return *Value; }, [Value=Pair.Value, SaveSupplies](int32 I) { *Value = I; SaveSupplies(); });
            Add(Panel, Text(TEXT("Test loadout: 0–20 supplies per type, with no weight or point budget. Applied on spawn."), 20));
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
        if (MenuPage == 7)
            Content = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[Text(TEXT("CREDITS"), 36)]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[Focus.ToSharedRef()]
                + SVerticalBox::Slot().FillHeight(1)[Content];
        Overlay->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(30,50,50,50)
            [SNew(SBox).WidthOverride((Lobby && MenuPage == 0) || MenuPage == 5 || MenuPage == 7 ? 900 : 540).MaxDesiredHeight(850)
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
    SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(Focus).SetHideCursorDuringCapture(false));
}
