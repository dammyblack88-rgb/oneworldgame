// Copyright Epic Games, Inc. All Rights Reserved.


#include "ParadiseHeatPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "ParadiseHeat.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Settings/ParadiseHeatPauseMenu.h"
#include "Settings/ParadiseHeatExperience.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Settings/ParadiseHeatUserSettings.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Vehicles/Motorcycle/ParadiseHeatMotorcycle.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Environment/ParadiseHeatCityRoute.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"

void AParadiseHeatPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalPlayerController())
	{
        if (GetWorld()->GetGameViewport()) if (auto Window=GetWorld()->GetGameViewport()->GetWindow()) Window->SetTitle(FText::FromString(TEXT("Paradise Heat")));
        auto* Experience=NewObject<UParadiseHeatExperience>(this);
        AddInstanceComponent(Experience);Experience->RegisterComponent();
        if(FParse::Param(FCommandLine::Get(),TEXT("ExperienceAudit")))
        {
         FTimerHandle Handle;
         GetWorld()->GetTimerManager().SetTimer(Handle,[this]()
         {
          ToggleCursorCapture();
          FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/paradiseheat-settings.png"),true,false);
          UE_LOG(LogParadiseHeat,Display,TEXT("SETTINGS_AUDIT open=1 paused=%d spawn=%s"),IsPaused(),*GetPawn()->GetActorLocation().ToString());
         },FParse::Param(FCommandLine::Get(),TEXT("CityImpactAudit"))?50.f:12.f,false);
        }
	}
	// Permit a low street-level view, but never orbit underneath the character.
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -65.f;
		PlayerCameraManager->ViewPitchMax = 8.f;
		FRotator View = GetControlRotation();
		View.Pitch = FMath::Clamp(FRotator::NormalizeAxis(View.Pitch), -65.f, 8.f);
		SetControlRotation(View);
		UE_LOG(LogParadiseHeat, Display, TEXT("Camera pitch limits active: -65 to +8 degrees"));
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogParadiseHeat, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AParadiseHeatPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this,
			&AParadiseHeatPlayerController::ToggleCursorCapture).bExecuteWhenPaused = true;
		InputComponent->BindKey(EKeys::E, IE_Pressed, this,&AParadiseHeatPlayerController::Interact);

		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void AParadiseHeatPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	TickVehicleInteraction(DeltaTime);
}

void AParadiseHeatPlayerController::TryAutoMountMotorcycle()
{
	if (!IsLocalPlayerController() || bCursorReleased || Cast<AParadiseHeatMotorcycle>(GetPawn())) return;
	if (!GetWorld() || GetWorld()->GetTimeSeconds() < AutoMountBlockedUntil) return;

	APawn* Character = GetPawn();
	if (!Character) return;
	AParadiseHeatMotorcycle* Best = nullptr;
	float BestDistance = FMath::Square(165.f);
	for (TActorIterator<AParadiseHeatMotorcycle> It(GetWorld()); It; ++It)
	{
		const float Distance = FVector::DistSquared(Character->GetActorLocation(), It->GetActorLocation());
		if (Distance < BestDistance && It->IsAvailableFor(Character))
		{
			Best = *It;
			BestDistance = Distance;
		}
	}
	if (Best) Best->Mount(this, Character);
}

void AParadiseHeatPlayerController::Interact()
{
	if(bCursorReleased || PendingVehicle || (GetPawn()&&GetPawn()->ActorHasTag(TEXT("ImpactRagdoll")))) return;
	if(DrivenCar){ExitCar();return;}
	if(auto* Bike=Cast<AParadiseHeatMotorcycle>(GetPawn()))
	{
		Bike->Dismount(this);
		if (GetWorld()) AutoMountBlockedUntil = GetWorld()->GetTimeSeconds() + 3.f;
		return;
	}
	APawn* Character=GetPawn(); if(!Cast<ACharacter>(Character))return;
	AActor* Best=nullptr;float BestDistance=FMath::Square(380.f);
	for(TActorIterator<AParadiseHeatMotorcycle> It(GetWorld());It;++It)
	{
		const float Distance=FVector::DistSquared(Character->GetActorLocation(),It->GetActorLocation());
		if(Distance<BestDistance&&It->IsAvailableFor(Character)){Best=*It;BestDistance=Distance;}
	}
	for(TActorIterator<AActor> It(GetWorld());It;++It)
	{
		if((!It->ActorHasTag(TEXT("CityTraffic"))&&!It->ActorHasTag(TEXT("PlayerUsableCar"))) || It->ActorHasTag(TEXT("VehicleEntryPending")))continue;
		const float Distance=FVector::DistSquared2D(Character->GetActorLocation(),It->GetActorLocation());
		if(Distance<BestDistance){Best=*It;BestDistance=Distance;}
	}
	if(auto* Bike=Cast<AParadiseHeatMotorcycle>(Best)) Bike->Mount(this,Character);
	else if(Best) BeginVehicleEntry(Best);
}

#include "VehicleInteraction.inl"

void AParadiseHeatPlayerController::ToggleCursorCapture()
{
	bCursorReleased = !bCursorReleased;
	bShowMouseCursor = bCursorReleased;
	SetIgnoreMoveInput(bCursorReleased);
	SetIgnoreLookInput(bCursorReleased);

	if (bCursorReleased)
	{
		if (!PauseMenu) PauseMenu = CreateWidget<UParadiseHeatPauseMenu>(this);
		if (PauseMenu) PauseMenu->AddToViewport(20);
		SetPause(true);
		if(auto* E=FindComponentByClass<UParadiseHeatExperience>())E->PauseMusic(true);
		FInputModeGameAndUI InputMode;
		if (PauseMenu) InputMode.SetWidgetToFocus(PauseMenu->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		SetPause(false);
		if(auto* E=FindComponentByClass<UParadiseHeatExperience>())E->PauseMusic(false);
		if (PauseMenu) PauseMenu->RemoveFromParent();
		if (auto* Settings = UParadiseHeatUserSettings::Get()) Settings->SaveSettings();
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		SetInputMode(InputMode);
	}

	UE_LOG(LogParadiseHeat, Display, TEXT("Cursor %s"),
		bCursorReleased ? TEXT("released; Escape resumes gameplay") : TEXT("captured; gameplay resumed"));
}

bool AParadiseHeatPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AParadiseHeatPlayerController::ResumeGameplay()
{
	if (bCursorReleased) ToggleCursorCapture();
}
