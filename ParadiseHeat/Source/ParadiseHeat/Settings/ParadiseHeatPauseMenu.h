#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseHeatPauseMenu.generated.h"
class UTextBlock;
class UVerticalBox;
class UButton;
class UEditableTextBox;
UCLASS()
class PARADISEHEAT_API UParadiseHeatPauseMenu : public UUserWidget
{
 GENERATED_BODY()
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual void NativeTick(const FGeometry& Geometry,float Dt) override;
 virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
 UPROPERTY() TObjectPtr<UTextBlock> SensitivityLabel;
 UPROPERTY() TObjectPtr<UTextBlock> QualityLabel;
 UFUNCTION() void Resume();
 UFUNCTION() void InvertChanged(bool bChecked);
 UFUNCTION() void SensitivityChanged(float Value);
 UFUNCTION() void Balanced();
 UFUNCTION() void Detailed();
 UFUNCTION() void Quit();
 UPROPERTY() TObjectPtr<UEditableTextBox> SongPath;
 UPROPERTY() TObjectPtr<UTextBlock> SongStatus;
 UFUNCTION() void MasterChanged(float Value);
 UFUNCTION() void MusicChanged(float Value);
 UFUNCTION() void EngineChanged(float Value);
 UFUNCTION() void ReactionChanged(float Value);
 UFUNCTION() void LoadSong();
 UFUNCTION() void BrowseSong();
 UFUNCTION() void DefaultSong();
 UFUNCTION() void VSyncChanged(bool Value);
 UFUNCTION() void FullscreenChanged(bool Value);
 UFUNCTION() void Fps30();
 UFUNCTION() void Fps60();
 UFUNCTION() void Fps120();
 void AudioChanged();
 void SetFps(float Value);
 void RefreshLabels();
 UTextBlock* AddLabel(UVerticalBox* Box, const FString& Text, int32 Size = 18);
 UButton* AddButton(UVerticalBox* Box, const FString& Text);
};
