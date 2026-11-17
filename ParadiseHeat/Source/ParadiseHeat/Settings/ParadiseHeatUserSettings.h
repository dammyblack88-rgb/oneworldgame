#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "ParadiseHeatUserSettings.generated.h"

/** Per-user settings, persisted by Unreal in GameUserSettings.ini. */
UCLASS(Config=GameUserSettings)
class PARADISEHEAT_API UParadiseHeatUserSettings : public UGameUserSettings
{
 GENERATED_BODY()
public:
 UParadiseHeatUserSettings();
 virtual void SetToDefaults() override;
 virtual void ValidateSettings() override;
 static UParadiseHeatUserSettings* Get();
 bool IsPointerYInverted() const { return bInvertPointerY; }
 float GetPointerSensitivity() const { return PointerSensitivity; }
 void SetPointerYInverted(bool bValue) { bInvertPointerY = bValue; }
 void SetPointerSensitivity(float Value);
 void ApplyCityQuality(bool bDetailed);
 UPROPERTY(Config) float MasterVolume=1.f;
 UPROPERTY(Config) float MusicVolume=1.f;
 UPROPERTY(Config) float EngineVolume=1.f;
 UPROPERTY(Config) float ReactionVolume=1.f;
 UPROPERTY(Config) FString CustomSongPath;
private:
 UPROPERTY(Config) int32 CitySettingsVersion = 0;
 UPROPERTY(Config) bool bInvertPointerY = true;
 UPROPERTY(Config) float PointerSensitivity = 1.f;
};
