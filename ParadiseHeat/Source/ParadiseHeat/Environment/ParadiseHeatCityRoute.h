#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParadiseHeatCityRoute.generated.h"
class USplineComponent;
class UStaticMesh;
class ACharacter;
class UBlueprint;
class USkeletalMesh;
class UAnimSequence;

/** Local ambient life. Cars follow lanes; these are not player-drivable physics vehicles. */
UCLASS()
class PARADISEHEAT_API AParadiseHeatCityRoute : public AActor
{
 GENERATED_BODY()
public:
 AParadiseHeatCityRoute();
 UFUNCTION(BlueprintCallable, CallInEditor) int32 ConfigureLegacyTraffic(UBlueprint* Blueprint);
 void ReleaseTraffic(AActor* Vehicle);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USplineComponent> Path;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPedestrians=false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Population=3;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float TravelSpeed=600.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<USkeletalMesh>> PedestrianMeshes;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UAnimSequence> PedestrianWalk;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UAnimSequence> PedestrianIdle;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<USkeletalMesh> TrafficSkeletalMesh;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<USkeletalMesh>> TrafficSkeletalMeshes;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<UStaticMesh>> TrafficParts;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FTransform> TrafficPartTransforms;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UStaticMesh> CarBody;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UStaticMesh> CarGlass;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UStaticMesh> CarWheel;
protected:
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Agents;
 TArray<float> Distances;
 TMap<TWeakObjectPtr<ACharacter>,float> FleeUntil;
 TMap<TWeakObjectPtr<ACharacter>,float> HitCooldown;
 TMap<TWeakObjectPtr<ACharacter>,float> HurtUntil;
 float Age=0.f;
 float NextSample=5.f;
 bool bReported=false;
 AActor* SpawnAgent(int32 Slot, const FVector& Position, const FRotator& Rotation);
};
