#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ParadiseHeatExperience.generated.h"
class UAudioComponent;
class USoundClass;
class USoundMix;
class UMediaPlayer;
class UMediaSoundComponent;
UCLASS()
class PARADISEHEAT_API UParadiseHeatExperience : public UActorComponent
{
 GENERATED_BODY()
public:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 void ApplyAudio();
 bool LoadSong(const FString& Path);
 void UseDefaultSong();
 void PauseMusic(bool Paused);
 FString Status=TEXT("Built-in soundtrack");
private:
 UPROPERTY() TObjectPtr<UAudioComponent> Music;
 UPROPERTY() TObjectPtr<USoundMix> Mix;
 UPROPERTY() TArray<TObjectPtr<USoundClass>> Classes;
 UPROPERTY() TObjectPtr<UMediaPlayer> Media;
 UPROPERTY() TObjectPtr<UMediaSoundComponent> MediaSound;
 bool bPaused=false,bCustomPlaying=false;
 FString PendingPath;
 UFUNCTION() void SongOpened(FString Url);
 UFUNCTION() void SongFailed(FString Url);
};
