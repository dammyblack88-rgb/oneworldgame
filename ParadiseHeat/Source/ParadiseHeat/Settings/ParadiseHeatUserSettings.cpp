#include "ParadiseHeatUserSettings.h"
#include "Engine/Engine.h"
UParadiseHeatUserSettings::UParadiseHeatUserSettings() { bInvertPointerY = true; PointerSensitivity = 1.f; }
UParadiseHeatUserSettings* UParadiseHeatUserSettings::Get()
{
 return GEngine ? Cast<UParadiseHeatUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}
void UParadiseHeatUserSettings::SetToDefaults()
{
 Super::SetToDefaults(); MasterVolume=MusicVolume=EngineVolume=ReactionVolume=1.f;CustomSongPath.Empty(); bInvertPointerY = true; PointerSensitivity = 1.f; CitySettingsVersion = 0;
 SetShadowQuality(2); SetGlobalIlluminationQuality(2); SetReflectionQuality(2); SetVisualEffectQuality(2); SetAntiAliasingQuality(2); SetFrameRateLimit(60.f);
}
void UParadiseHeatUserSettings::ValidateSettings()
{
 Super::ValidateSettings();
 MasterVolume=FMath::Clamp(MasterVolume,0.f,1.f);MusicVolume=FMath::Clamp(MusicVolume,0.f,1.f);EngineVolume=FMath::Clamp(EngineVolume,0.f,1.f);ReactionVolume=FMath::Clamp(ReactionVolume,0.f,1.f); SetPointerSensitivity(PointerSensitivity);
 if (CitySettingsVersion < 1)
 {
  SetShadowQuality(2); SetGlobalIlluminationQuality(2); SetReflectionQuality(2); SetVisualEffectQuality(2); SetAntiAliasingQuality(2); SetFrameRateLimit(60.f); CitySettingsVersion = 1;
 }
}
void UParadiseHeatUserSettings::SetPointerSensitivity(float Value)
{
 PointerSensitivity = FMath::IsFinite(Value) ? FMath::Clamp(Value, .25f, 2.f) : 1.f;
}
void UParadiseHeatUserSettings::ApplyCityQuality(bool bDetailed)
{
 const int32 Quality = bDetailed ? 3 : 2;
 SetShadowQuality(Quality); SetGlobalIlluminationQuality(Quality);
 SetReflectionQuality(Quality); SetVisualEffectQuality(Quality); SetAntiAliasingQuality(Quality);
 SetFrameRateLimit(60.f);
 ApplyNonResolutionSettings(); SaveSettings();
}
