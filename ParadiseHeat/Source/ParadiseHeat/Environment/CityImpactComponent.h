#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CityImpactComponent.generated.h"
class UPrimitiveComponent;
class ACharacter;
UCLASS()
class PARADISEHEAT_API UCityImpactComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UCityImpactComponent();
 static UCityImpactComponent* Install(AActor* Actor);
 static bool KnockDown(ACharacter* Human,FVector Velocity);
 void Contact(AActor* Other,UPrimitiveComponent* OtherBody,const FHitResult& Hit,FVector Velocity);
 virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick) override;
private:
 UFUNCTION() void OnHit(UPrimitiveComponent* Body,AActor* Other,UPrimitiveComponent* OtherBody,FVector Impulse,const FHitResult& Hit);
 void ActivateWreck(FVector Velocity,FVector ImpactVelocity=FVector::ZeroVector);
 FVector PreviousVelocity=FVector::ZeroVector;
 FVector RecoveryOrigin=FVector::ZeroVector;
 FTransform SavedMesh;
 float Cooldown=0,RecoverAt=0,FleeUntil=0;
 FVector FleeDirection=FVector::ForwardVector;
 bool bFallen=false;
};
