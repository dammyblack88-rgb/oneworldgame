// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ParadiseHeatPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UParadiseHeatPauseMenu;
class UAudioComponent;
class ACharacter;
class UPoseableMeshComponent;
class UStaticMeshComponent;
class UMeshComponent;
class UMaterialInterface;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AParadiseHeatPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void ResumeGameplay();
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Escape releases desktop mouse capture; press again to resume gameplay. */
	void ToggleCursorCapture();
	void Interact();
	void TryAutoMountMotorcycle();
	void TickVehicleInteraction(float DeltaTime);
	void BeginVehicleEntry(AActor* Vehicle);
	void FinishVehicleEntry();
	void ExitCar();
	void EjectTrafficDriver(AActor* Vehicle);
	bool FindVehicleExit(AActor* Vehicle, FVector& Location) const;
	UPROPERTY(Transient) TObjectPtr<APawn> DrivenCar;
	UPROPERTY(Transient) TObjectPtr<ACharacter> CarHuman;
	UPROPERTY(Transient) TObjectPtr<UPoseableMeshComponent> CarDriverVisual;
	UPROPERTY(Transient) TObjectPtr<UPoseableMeshComponent> CockpitBody;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> CarMirrorCaptures;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> CarMirrorSurfaces;
	float NextMirrorUpdate=0.f;
	int32 MirrorUpdateIndex=0;
	void CreateCarMirrors();
	void UpdateCarMirrors();
	void RemoveCarMirrors();
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> CarSteeringWheel;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> CarEngineAudio;
	float CarEnginePitch=0.8f;
	UPROPERTY(Transient) TObjectPtr<AActor> PendingVehicle;
	UPROPERTY(Transient) TObjectPtr<UClass> EntryAnimationClass;
	UPROPERTY(Transient) TObjectPtr<APawn> PreparedVehicle;
	UPROPERTY(Transient) TObjectPtr<UPoseableMeshComponent> EntryVisual;
	UPROPERTY(Transient) TObjectPtr<AActor> EntryCamera;
	TArray<FTransform> EntryStandingPose;
	TArray<FTransform> EntrySeatedPose;
	TArray<TWeakObjectPtr<ACharacter>> EscapingDrivers;
	TArray<float> DriverHitTimes;
	float EntryStarted=0.f;
	bool bEntryApproaching=false;
 TArray<FVector> EntryApproachWaypoints;
 UPROPERTY(Transient) TArray<TObjectPtr<UMeshComponent>> SavedGlassMeshes;
 UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> SavedGlassMaterials;
 TArray<int32> SavedGlassSlots;
	float LastCarLookTime=-10.f;
	FVector CarWheelTarget=FVector(44,0,50);
	FVector CarWheelAxis=FVector(.889f,0,-.458f);
	float LastDriverSteering=0.f;
	bool bDriverEjected=false;
	FVector EntryLocation=FVector::ZeroVector;
	void TickVehicleEntryAudit();
	int32 VehicleAuditStage=0;
	float VehicleAuditTime=0;
	FVector VehicleAuditPosition=FVector::ZeroVector;
	TWeakObjectPtr<AActor> AuditCar;
	float AutoMountBlockedUntil = 0.f;
	bool bCursorReleased = false;
	UPROPERTY() TObjectPtr<UParadiseHeatPauseMenu> PauseMenu;
	UPROPERTY() TObjectPtr<UAudioComponent> BackgroundMusic;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};
