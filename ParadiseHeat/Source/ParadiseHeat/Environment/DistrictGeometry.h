#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DistrictGeometry.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/** Authored district geometry, batched by mesh/material/collision and saved in the map. */
UCLASS()
class PARADISEHEAT_API ADistrictGeometry : public AActor
{
	GENERATED_BODY()
public:
	ADistrictGeometry();

	UFUNCTION(BlueprintCallable, Category="District Authoring")
	void AddDistrictInstance(UStaticMesh* Mesh, UMaterialInterface* Material,
		const FTransform& Transform, bool bCollision);

UFUNCTION(BlueprintCallable, Category="District Authoring")
 void AddDistrictMesh(UStaticMesh* Mesh, const TArray<UMaterialInterface*>& Materials, const FTransform& Transform, bool bCollision);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> Batches;

private:
	bool bAudit = false;
	bool bPerfCapture = false;
	bool bPerfStarted = false;
	bool bInitialAuditReported = false;
	float AuditTime = 0.f;
	int32 AuditFrames = 0;
	float AuditMaxFrame = 0.f;
};
