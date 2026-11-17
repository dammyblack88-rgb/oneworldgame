#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ParadiseHeatMotorcycle.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UAudioComponent;
class UPoseableMeshComponent;
class USoundBase;
class ACharacter;

/** Simulated chassis with two wheel suspension probes and assisted upright balance. */
UCLASS()
class PARADISEHEAT_API AParadiseHeatMotorcycle : public APawn
{
 GENERATED_BODY()
public:
 AParadiseHeatMotorcycle();
 virtual void Tick(float DeltaSeconds) override;
 virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
 bool IsAvailableFor(const APawn* Candidate) const;
 void Mount(APlayerController* Controller, APawn* Character);
 void Dismount(APlayerController* Controller);
 bool IsMounted() const { return Rider.IsValid(); }
protected:
 virtual void BeginPlay() override;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> PhysicsBody;
 UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> VisualRoot;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyMesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Kickstand;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FrontWheel;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RearWheel;
 UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> RideCamera;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> EngineAudio;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UPoseableMeshComponent> RiderVisual;
 UPROPERTY(EditAnywhere,Category="Motorcycle|Physics") float DriveForce=155000.f;
 UPROPERTY(EditAnywhere,Category="Motorcycle|Physics") float BrakeForce=230000.f;
 UPROPERTY(EditAnywhere,Category="Motorcycle|Physics") float BalanceTorque=8500000.f;
 UPROPERTY(EditAnywhere,Category="Motorcycle|Physics") float SteeringTorque=2600000.f;
 UPROPERTY(EditAnywhere,Category="Motorcycle|Physics") float MaxLeanDegrees=24.f;
private:
 UFUNCTION() void OnCrashHit(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
 TWeakObjectPtr<ACharacter> FallenRider;
 FTransform SavedMeshTransform;
 FVector PreviousVelocity=FVector::ZeroVector;
 FVector CrashOrigin=FVector::ZeroVector;
 float RecoverAt=0.f,MountAllowedAt=0.f;
 bool bCrashed=false;
 TWeakObjectPtr<APawn> Rider;
 float ForwardInput=0.f,ReverseInput=0.f,LeftInput=0.f,RightInput=0.f,BrakeInput=0.f;
 float WheelAngle=0.f;
 float FeetDownBlend=1.f,LastPoseBlend=-1.f;
 float CurrentDriveSpeed=0.f;
 float LastCameraLookTime=-10.f;
 bool BuildRiderPose(ACharacter* Character);
 void TickAudit(float Dt);
 float AuditTime=0.f;
 int32 AuditStage=0;
 bool bAudit=false;
 FVector AuditStart=FVector::ZeroVector;
 TWeakObjectPtr<APlayerController> AuditController;
 void InputForward(float Value); void InputReverse(float Value); void InputLeft(float Value); void InputRight(float Value); void InputBrake(float Value);
 void ArrowForwardPressed(); void ArrowForwardReleased();
 void ArrowReversePressed(); void ArrowReverseReleased();
 void ArrowLeftPressed(); void ArrowLeftReleased();
 void ArrowRightPressed(); void ArrowRightReleased();
 void BrakePressed(); void BrakeReleased();
 void LookYaw(float Value); void LookPitch(float Value);
};
