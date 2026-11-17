#include "ParadiseHeatPauseMenu.h"
#include "ParadiseHeatUserSettings.h"
#include "ParadiseHeatExperience.h"
#include "NativeSongPicker.h"
#include "HAL/IConsoleManager.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "ParadiseHeatPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/BackgroundBlur.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"

UTextBlock* UParadiseHeatPauseMenu::AddLabel(UVerticalBox* Box, const FString& Text, int32 Size)
{
 auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
 Label->SetText(FText::FromString(Text));
 auto Font = Label->GetFont(); Font.Size = Size; Label->SetFont(Font);
 Label->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.93f,.96f)));
 Label->SetAutoWrapText(true);if(Size<=20)Label->SetWrapTextAt(440.f);
 Box->AddChildToVerticalBox(Label)->SetPadding(FMargin(0,8));
 return Label;
}
UButton* UParadiseHeatPauseMenu::AddButton(UVerticalBox* Box, const FString& Text)
{
 auto* Button = WidgetTree->ConstructWidget<UButton>();
 auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Text));
 Label->SetColorAndOpacity(FSlateColor(FLinearColor(.95f,.95f,.95f)));
 auto Font=Label->GetFont();Font.Size=18;Label->SetFont(Font);Button->SetBackgroundColor(FLinearColor(.12f,.19f,.25f));
 Button->AddChild(Label); Box->AddChildToVerticalBox(Button)->SetPadding(FMargin(0,6));
 return Button;
}
TSharedRef<SWidget> UParadiseHeatPauseMenu::RebuildWidget()
{
 if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("SettingsTree"));
 auto* Layers=WidgetTree->ConstructWidget<UOverlay>();
 auto FillLayer=[&](UWidget* Widget){auto* Slot=Layers->AddChildToOverlay(Widget);Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Fill);};
 auto* Cover=WidgetTree->ConstructWidget<UImage>();
 if(auto* Texture=FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir()/TEXT("UI/Loading/welcome-screen.png")))Cover->SetBrushFromTexture(Texture,true);
 auto* CoverScale=WidgetTree->ConstructWidget<UScaleBox>();CoverScale->SetStretch(EStretch::ScaleToFill);CoverScale->SetClipping(EWidgetClipping::ClipToBounds);CoverScale->AddChild(Cover);FillLayer(CoverScale);
 auto* Blur=WidgetTree->ConstructWidget<UBackgroundBlur>();Blur->SetBlurStrength(10.f);FillLayer(Blur);
 auto* Background = WidgetTree->ConstructWidget<UBorder>();
 Background->SetBrushColor(FLinearColor(.005f,.009f,.016f,.78f));
 Background->SetHorizontalAlignment(HAlign_Center); Background->SetVerticalAlignment(VAlign_Center);
 auto* Width = WidgetTree->ConstructWidget<USizeBox>(); Width->SetWidthOverride(1040.f);Width->SetHeightOverride(900.f);
 auto* Box = WidgetTree->ConstructWidget<UVerticalBox>();
 Width->AddChild(Box); Background->AddChild(Width); FillLayer(Background); WidgetTree->RootWidget = Layers;
 AddLabel(Box,TEXT("PARADISE HEAT  /  PAUSED"),36);
 AddLabel(Box,TEXT("YOUR CITY. YOUR RULES."),16);
 auto* Columns=WidgetTree->ConstructWidget<UHorizontalBox>();
 auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Scroll->AddChild(Columns);
 Box->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 auto* Controls=WidgetTree->ConstructWidget<UVerticalBox>();auto* Audio=WidgetTree->ConstructWidget<UVerticalBox>();
 auto* Left=Columns->AddChildToHorizontalBox(Controls);Left->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Left->SetPadding(FMargin(0,0,32,0));
 auto* Right=Columns->AddChildToHorizontalBox(Audio);Right->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Right->SetPadding(FMargin(32,0,0,0));
 auto* Footer=Box;Box=Controls;
 AddLabel(Box,TEXT("Mouse & touchpad"),20);
 auto* Settings = UParadiseHeatUserSettings::Get();
 auto* Invert = WidgetTree->ConstructWidget<UCheckBox>();
 auto* InvertText = WidgetTree->ConstructWidget<UTextBlock>(); InvertText->SetText(FText::FromString(TEXT("Reverse vertical look")));
 Invert->AddChild(InvertText); Invert->SetIsChecked(!Settings || Settings->IsPointerYInverted());
 Box->AddChildToVerticalBox(Invert)->SetPadding(FMargin(0,8));
 Invert->OnCheckStateChanged.AddDynamic(this,&UParadiseHeatPauseMenu::InvertChanged);
 SensitivityLabel = AddLabel(Box,TEXT("Sensitivity"));
 auto* Slider = WidgetTree->ConstructWidget<USlider>(); Slider->SetMinValue(.25f); Slider->SetMaxValue(2.f);
 Slider->SetValue(Settings ? Settings->GetPointerSensitivity() : 1.f);
 Box->AddChildToVerticalBox(Slider)->SetPadding(FMargin(0,8));
 Slider->OnValueChanged.AddDynamic(this,&UParadiseHeatPauseMenu::SensitivityChanged);
 QualityLabel = AddLabel(Box,TEXT("Graphics"),20);
 AddButton(Box,TEXT("Balanced"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::Balanced);
 AddButton(Box,TEXT("Detailed"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::Detailed);
 AddLabel(Box,TEXT("Frame rate limit"),18);
 AddButton(Box,TEXT("30 FPS"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::Fps30);
 AddButton(Box,TEXT("60 FPS"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::Fps60);
 AddButton(Box,TEXT("120 FPS"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::Fps120);
 auto* VSync=WidgetTree->ConstructWidget<UCheckBox>();auto* VLabel=WidgetTree->ConstructWidget<UTextBlock>();VLabel->SetText(FText::FromString(TEXT("Vertical sync")));VSync->AddChild(VLabel);VSync->SetIsChecked(Settings&&Settings->IsVSyncEnabled());Box->AddChild(VSync);VSync->OnCheckStateChanged.AddDynamic(this,&UParadiseHeatPauseMenu::VSyncChanged);
 auto* Full=WidgetTree->ConstructWidget<UCheckBox>();auto* FLabel=WidgetTree->ConstructWidget<UTextBlock>();FLabel->SetText(FText::FromString(TEXT("Fullscreen")));Full->AddChild(FLabel);Full->SetIsChecked(Settings&&Settings->GetFullscreenMode()!=EWindowMode::Windowed);Box->AddChild(Full);Full->OnCheckStateChanged.AddDynamic(this,&UParadiseHeatPauseMenu::FullscreenChanged);
 AddLabel(Audio,TEXT("AUDIO"),22);
 auto AddVolume=[&](const TCHAR* Title,float Value){AddLabel(Audio,Title,18);auto* S=WidgetTree->ConstructWidget<USlider>();S->SetMinValue(0);S->SetMaxValue(1);S->SetValue(Value);S->SetSliderHandleColor(FLinearColor(1,.45f,.12f));Audio->AddChildToVerticalBox(S)->SetPadding(FMargin(0,4,0,14));return S;};
 AddVolume(TEXT("Master volume"),Settings?Settings->MasterVolume:1)->OnValueChanged.AddDynamic(this,&UParadiseHeatPauseMenu::MasterChanged);
 AddVolume(TEXT("Background music"),Settings?Settings->MusicVolume:1)->OnValueChanged.AddDynamic(this,&UParadiseHeatPauseMenu::MusicChanged);
 AddVolume(TEXT("Car & motorcycle engines"),Settings?Settings->EngineVolume:1)->OnValueChanged.AddDynamic(this,&UParadiseHeatPauseMenu::EngineChanged);
 AddVolume(TEXT("Shouts & human reactions"),Settings?Settings->ReactionVolume:1)->OnValueChanged.AddDynamic(this,&UParadiseHeatPauseMenu::ReactionChanged);
 AddLabel(Audio,TEXT("YOUR SOUNDTRACK"),22);
 AddLabel(Audio,TEXT("Choose an MP3, WAV, M4A or AAC file, or paste its path. Your song loops while playing and pauses with the game."),14);
 SongPath=WidgetTree->ConstructWidget<UEditableTextBox>();SongPath->SetForegroundColor(FLinearColor(.04f,.04f,.04f));SongPath->SetHintText(FText::FromString(TEXT("/Users/you/Music/song.mp3")));if(Settings)SongPath->SetText(FText::FromString(Settings->CustomSongPath));Audio->AddChildToVerticalBox(SongPath);
 AddButton(Audio,TEXT("Browse for music…"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::BrowseSong);
 AddButton(Audio,TEXT("Use this song"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::LoadSong);
 AddButton(Audio,TEXT("Restore built-in music"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::DefaultSong);
 SongStatus=AddLabel(Audio,TEXT(""),14);
 AddLabel(Audio,TEXT("CONTROLS"),22);
 AddLabel(Audio,TEXT("WASD / arrows — move or drive\nShift — run\nSpace — jump / vehicle brake\nE — enter or exit vehicle\nF — punch\nTouchpad — look around\nESC — pause / resume"),15);
 Box=Footer;
 AddButton(Box,TEXT("Resume / Esc"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::Resume);
 AddButton(Box,TEXT("Quit game"))->OnClicked.AddDynamic(this,&UParadiseHeatPauseMenu::Quit);
 RefreshLabels(); SetIsFocusable(true);
 return Super::RebuildWidget();
}
void UParadiseHeatPauseMenu::RefreshLabels()
{
 if (auto* Settings = UParadiseHeatUserSettings::Get())
 {
  if (SensitivityLabel) SensitivityLabel->SetText(FText::FromString(FString::Printf(TEXT("Sensitivity: %.2fx"),Settings->GetPointerSensitivity())));
  if (QualityLabel) QualityLabel->SetText(FText::FromString(Settings->GetShadowQuality()>=3 ? TEXT("Graphics: Detailed") : TEXT("Graphics: Balanced")));
 }
}
void UParadiseHeatPauseMenu::Resume()
{
 if (auto* Settings = UParadiseHeatUserSettings::Get()) Settings->SaveSettings();
 if (auto* PC = Cast<AParadiseHeatPlayerController>(GetOwningPlayer())) PC->ResumeGameplay();
}
void UParadiseHeatPauseMenu::InvertChanged(bool bChecked)
{
 if (auto* Settings = UParadiseHeatUserSettings::Get()) { Settings->SetPointerYInverted(bChecked); Settings->SaveSettings(); }
}
void UParadiseHeatPauseMenu::SensitivityChanged(float Value)
{
 if (auto* Settings = UParadiseHeatUserSettings::Get()) Settings->SetPointerSensitivity(Value);
 RefreshLabels(); // Persist once on Resume/Quit, not for every slider mouse event.
}
void UParadiseHeatPauseMenu::Balanced() { if (auto* S=UParadiseHeatUserSettings::Get()) S->ApplyCityQuality(false); RefreshLabels(); }
void UParadiseHeatPauseMenu::Detailed() { if (auto* S=UParadiseHeatUserSettings::Get()) S->ApplyCityQuality(true); RefreshLabels(); }
void UParadiseHeatPauseMenu::Quit()
{
 if (auto* Settings=UParadiseHeatUserSettings::Get()) Settings->SaveSettings();
 UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);
}
FReply UParadiseHeatPauseMenu::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
 if (Event.GetKey()==EKeys::Escape) { Resume(); return FReply::Handled(); }
 return Super::NativeOnKeyDown(Geometry,Event);
}


void UParadiseHeatPauseMenu::NativeTick(const FGeometry& G,float Dt)
{
 Super::NativeTick(G,Dt);
 if(auto* PC=GetOwningPlayer())if(auto* E=PC->FindComponentByClass<UParadiseHeatExperience>())if(SongStatus)SongStatus->SetText(FText::FromString(E->Status));
}
void UParadiseHeatPauseMenu::AudioChanged(){if(auto* PC=GetOwningPlayer())if(auto* E=PC->FindComponentByClass<UParadiseHeatExperience>())E->ApplyAudio();}
void UParadiseHeatPauseMenu::MasterChanged(float V){if(auto* S=UParadiseHeatUserSettings::Get())S->MasterVolume=V;AudioChanged();}
void UParadiseHeatPauseMenu::MusicChanged(float V){if(auto* S=UParadiseHeatUserSettings::Get())S->MusicVolume=V;AudioChanged();}
void UParadiseHeatPauseMenu::EngineChanged(float V){if(auto* S=UParadiseHeatUserSettings::Get())S->EngineVolume=V;AudioChanged();}
void UParadiseHeatPauseMenu::ReactionChanged(float V){if(auto* S=UParadiseHeatUserSettings::Get())S->ReactionVolume=V;AudioChanged();}
void UParadiseHeatPauseMenu::LoadSong(){if(auto* PC=GetOwningPlayer())if(auto* E=PC->FindComponentByClass<UParadiseHeatExperience>())E->LoadSong(SongPath->GetText().ToString());}
void UParadiseHeatPauseMenu::DefaultSong(){if(auto* PC=GetOwningPlayer())if(auto* E=PC->FindComponentByClass<UParadiseHeatExperience>())E->UseDefaultSong();SongPath->SetText(FText::GetEmpty());}
void UParadiseHeatPauseMenu::VSyncChanged(bool V){if(auto* S=UParadiseHeatUserSettings::Get()){S->SetVSyncEnabled(V);S->ApplyNonResolutionSettings();S->SaveSettings();}}
void UParadiseHeatPauseMenu::FullscreenChanged(bool V){if(auto* S=UParadiseHeatUserSettings::Get()){S->SetFullscreenMode(V?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);S->ApplyResolutionSettings(false);S->SaveSettings();}}
void UParadiseHeatPauseMenu::SetFps(float V){if(auto* S=UParadiseHeatUserSettings::Get()){S->SetFrameRateLimit(V);S->ApplyNonResolutionSettings();if(auto* C=IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))C->Set(V,ECVF_SetByConsole);S->SaveSettings();}}
void UParadiseHeatPauseMenu::Fps30(){SetFps(30);}
void UParadiseHeatPauseMenu::Fps60(){SetFps(60);}
void UParadiseHeatPauseMenu::Fps120(){SetFps(120);}

void UParadiseHeatPauseMenu::BrowseSong()
{
 TWeakObjectPtr<UParadiseHeatPauseMenu> Weak(this);
 OpenNativeSongPicker([Weak](FString Path)
 {
  if(Weak.IsValid()&&!Path.IsEmpty())
  {Weak->SongPath->SetText(FText::FromString(Path));Weak->LoadSong();}
 });
}
