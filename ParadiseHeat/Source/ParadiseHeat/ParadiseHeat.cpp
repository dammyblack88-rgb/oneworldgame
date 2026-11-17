#include "ParadiseHeat.h"
#include "Modules/ModuleManager.h"
#include "MoviePlayer.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/StrongObjectPtr.h"
#include "Settings/ParadiseHeatUserSettings.h"
static TStrongObjectPtr<UAudioComponent> LoadingMusic;
UAudioComponent* TakeLoadingMusic(){auto* Result=LoadingMusic.Get();LoadingMusic.Reset();return Result;}
#include "UObject/UObjectGlobals.h"
#include "Misc/Paths.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Styling/CoreStyle.h"

class FParadiseHeatModule : public FDefaultGameModuleImpl
{
 FDelegateHandle LoadingHandle;
 TSharedPtr<FSlateDynamicImageBrush> WelcomeBrush;
 void LoadingScreen(const FString& Map)
 {
  if(IsRunningCommandlet()||!IsMoviePlayerEnabled())return;
  if(!LoadingMusic.IsValid()&&GEngine)
  if(auto* Device=GEngine->GetMainAudioDeviceRaw())
  if(auto* Sound=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/LocalPrototype/LocalPrototypeBGM.LocalPrototypeBGM")))
  {
   FAudioDevice::FCreateComponentParams Params(Device);Params.bAutoDestroy=false;Params.bPlay=false;Params.bStopWhenOwnerDestroyed=false;
   LoadingMusic.Reset(FAudioDevice::CreateComponent(Sound,Params));
   if(auto* Audio=LoadingMusic.Get())
   {
    Audio->bIsUISound=true;Audio->bAllowSpatialization=false;Audio->bIgnoreForFlushing=true;
    auto* Settings=UParadiseHeatUserSettings::Get();Audio->SetVolumeMultiplier(.18f*(Settings?Settings->MasterVolume*Settings->MusicVolume:1.f));Audio->Play();
    UE_LOG(LogParadiseHeat,Display,TEXT("LOADING_MUSIC_STARTED playing=%d"),Audio->IsPlaying());
   }
  }
  const FString ImagePath=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("UI/Loading/welcome-screen.png"));
  WelcomeBrush=MakeShared<FSlateDynamicImageBrush>(FName(*ImagePath),FVector2D(1672,941));
  FLoadingScreenAttributes Screen;
  Screen.bAutoCompleteWhenLoadingCompletes=true;
  Screen.MinimumLoadingScreenDisplayTime=3.f;
  Screen.WidgetLoadingScreen=SNew(SOverlay)
   +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor::Black)]
   +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(WelcomeBrush.Get())]]
   +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(55,0,0,55)
   [SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
     [SNew(STextBlock).Text(FText::FromString("LOADING THE CITY")).ColorAndOpacity(FLinearColor::White).Font(FCoreStyle::GetDefaultFontStyle("BoldItalic",22))]
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16,0,0,0)[SNew(SThrobber).NumPieces(3)]
   ];
  GetMoviePlayer()->SetupLoadingScreen(Screen);
  UE_LOG(LogParadiseHeat,Display,TEXT("LOADING_SCREEN_OPEN title=Paradise Heat image=%s exists=%d map=%s"),*ImagePath,FPaths::FileExists(ImagePath),*Map);
 }
public:
 virtual void StartupModule() override
 {LoadingHandle=FCoreUObjectDelegates::PreLoadMap.AddRaw(this,&FParadiseHeatModule::LoadingScreen);}
 virtual void ShutdownModule() override
 {FCoreUObjectDelegates::PreLoadMap.Remove(LoadingHandle);WelcomeBrush.Reset();}
};
IMPLEMENT_PRIMARY_GAME_MODULE(FParadiseHeatModule,ParadiseHeat,"Paradise Heat");
DEFINE_LOG_CATEGORY(LogParadiseHeat)
