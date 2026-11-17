#include "ParadiseHeatExperience.h"
#include "Environment/CityImpactComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "AudioDevice.h"
#include "ParadiseHeatUserSettings.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "ParadiseHeat.h"

void UParadiseHeatExperience::BeginPlay()
{
 Super::BeginPlay();
 Mix=NewObject<USoundMix>(this);
 const TCHAR* Paths[]={
 TEXT("/Game/Audio/LocalPrototype/LocalPrototypeBGM.LocalPrototypeBGM"),
 TEXT("/Game/Audio/Vehicles/PrototypeCarEngine.PrototypeCarEngine"),
 TEXT("/Game/Audio/Vehicles/PrototypeMotorcycleEngine.PrototypeMotorcycleEngine"),
 TEXT("/Game/Audio/Pedestrians/PedestrianHurt.PedestrianHurt")};
 for(int32 I=0;I<4;++I)
 {
  auto* Class=NewObject<USoundClass>(this);Classes.Add(Class);
  if(auto Device=GetWorld()->GetAudioDevice())Device->RegisterSoundClass(Class);
  if(auto* Sound=LoadObject<USoundBase>(nullptr,Paths[I]))
  {
   Sound->SoundClassObject=Class;
   if(I==3)
   {
    auto* Attenuation=NewObject<USoundAttenuation>(Sound);
    Attenuation->Attenuation.bAttenuate=true;
    Attenuation->Attenuation.bSpatialize=true;
    Attenuation->Attenuation.AttenuationShapeExtents=FVector(150.f,0,0);
    Attenuation->Attenuation.FalloffDistance=1800.f;
    Sound->AttenuationSettings=Attenuation;
   }
  }
 }
 UGameplayStatics::PushSoundMixModifier(this,Mix);
 ApplyAudio();
 Music=TakeLoadingMusic();
 UE_LOG(LogParadiseHeat,Display,TEXT("LOADING_MUSIC_HANDOFF reused=%d"),Music!=nullptr);
 if(!Music)if(auto* Sound=LoadObject<USoundBase>(nullptr,Paths[0]))Music=UGameplayStatics::SpawnSound2D(this,Sound,.18f,1,0,nullptr,false,false);
 Media=NewObject<UMediaPlayer>(this);Media->SetLooping(true);Media->PlayOnOpen=false;
 Media->OnMediaOpened.AddDynamic(this,&UParadiseHeatExperience::SongOpened);
 Media->OnMediaOpenFailed.AddDynamic(this,&UParadiseHeatExperience::SongFailed);
 MediaSound=NewObject<UMediaSoundComponent>(GetOwner());GetOwner()->AddInstanceComponent(MediaSound);MediaSound->SetMediaPlayer(Media);MediaSound->RegisterComponent();
 if(auto* S=UParadiseHeatUserSettings::Get())if(!S->CustomSongPath.IsEmpty())LoadSong(S->CustomSongPath);
 ApplyAudio();
 if(FParse::Param(FCommandLine::Get(),TEXT("ExperienceAudit")))
 {
  if(auto* Human=UGameplayStatics::GetPlayerCharacter(this,0))
  {
   auto* Parked=GetWorld()->SpawnActor<AActor>();
   auto* Box=NewObject<UBoxComponent>(Parked);Parked->AddInstanceComponent(Box);Parked->SetRootComponent(Box);Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);Box->RegisterComponent();
   auto* Response=UCityImpactComponent::Install(Parked);
   const FVector Saved=Human->GetCapsuleComponent()->ComponentVelocity;Human->GetCapsuleComponent()->ComponentVelocity=FVector(300,0,0);
   FHitResult Hit;Hit.ImpactNormal=FVector(1,0,0);
   Response->Contact(Human,Human->GetCapsuleComponent(),Hit,FVector::ZeroVector);
   UE_LOG(LogParadiseHeat,Display,TEXT("PARKED_CONTACT_AUDIT harmless=%d"),!Human->GetMesh()->IsSimulatingPhysics());
   Human->GetCapsuleComponent()->ComponentVelocity=Saved;Parked->Destroy();
  }
  FString TestPath;
  FParse::Value(FCommandLine::Get(),TEXT("TestSong="),TestPath);
  if(!TestPath.IsEmpty())LoadSong(TestPath);
  FTimerHandle Test;
  GetWorld()->GetTimerManager().SetTimer(Test,[this]()
  {
   PauseMusic(true);
   UE_LOG(LogParadiseHeat,Display,TEXT("AUDIO_AUDIT ready=%d paused=%d status=%s"),Media->IsReady(),!Media->IsPlaying(),*Status);
   PauseMusic(false);
   UE_LOG(LogParadiseHeat,Display,TEXT("AUDIO_AUDIT resumed=%d"),Media->IsPlaying());
   UseDefaultSong();
  },9.f,false);
 }
}
void UParadiseHeatExperience::ApplyAudio()
{
 auto* S=UParadiseHeatUserSettings::Get();if(!S)return;
 for(int32 I=0;I<Classes.Num();++I)
 {
  const float Category=I==0?S->MusicVolume:I==3?S->ReactionVolume:S->EngineVolume;
  UGameplayStatics::SetSoundMixClassOverride(this,Mix,Classes[I],I==0?1.f:S->MasterVolume*Category,1,.1f,true);
 }
 if(Music)Music->SetVolumeMultiplier(.18f*S->MasterVolume*S->MusicVolume);
 if(MediaSound)MediaSound->SetVolumeMultiplier(.6f*S->MasterVolume*S->MusicVolume);
}
bool UParadiseHeatExperience::LoadSong(const FString& Input)
{
 FString Path=Input.TrimStartAndEnd();Path.RemoveFromStart(TEXT("\""));Path.RemoveFromEnd(TEXT("\""));
 if(Path.StartsWith(TEXT("~/")))Path=FPaths::Combine(FPlatformProcess::UserHomeDir(),Path.Mid(2));
 if(!IFileManager::Get().FileExists(*Path)){Status=TEXT("File not found. Paste the full path to your song.");return false;}
 const FString Ext=FPaths::GetExtension(Path).ToLower();
 if(Ext!=TEXT("mp3")&&Ext!=TEXT("wav")&&Ext!=TEXT("m4a")&&Ext!=TEXT("aac")){Status=TEXT("Choose an MP3, WAV, M4A or AAC file.");return false;}
 PendingPath=Path;Status=TEXT("Opening song...");
 if(!Media->OpenFile(Path)){SongFailed(Path);return false;}return true;
}
void UParadiseHeatExperience::SongOpened(FString Url)
{
 UE_LOG(LogParadiseHeat,Display,TEXT("CUSTOM_SONG_OPENED file=%s"),*PendingPath);
 bCustomPlaying=true;Media->SetLooping(true);if(Music)Music->SetPaused(true);
 Status=FPaths::GetCleanFilename(PendingPath);
 if(auto* S=UParadiseHeatUserSettings::Get()){S->CustomSongPath=PendingPath;S->SaveSettings();}
 if(!bPaused)Media->Play();
}
void UParadiseHeatExperience::SongFailed(FString Url)
{
 bCustomPlaying=false;Status=TEXT("Could not play this file. Built-in music restored.");
 if(Music)Music->SetPaused(bPaused);
}
void UParadiseHeatExperience::UseDefaultSong()
{
 Media->Close();bCustomPlaying=false;if(Music)Music->SetPaused(bPaused);Status=TEXT("Built-in soundtrack");
 if(auto* S=UParadiseHeatUserSettings::Get()){S->CustomSongPath.Empty();S->SaveSettings();}
}
void UParadiseHeatExperience::PauseMusic(bool Paused)
{
 bPaused=Paused;if(Music)Music->SetPaused(Paused||bCustomPlaying);
 if(Media){if(Paused)Media->Pause();else if(bCustomPlaying)Media->Play();}
}
void UParadiseHeatExperience::EndPlay(const EEndPlayReason::Type Reason)
{
 if(Media)Media->Close();if(Music)Music->Stop();
 if(Mix)UGameplayStatics::PopSoundMixModifier(this,Mix);
 Super::EndPlay(Reason);
}
