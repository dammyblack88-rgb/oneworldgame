#include "DistrictGeometry.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CollisionShape.h"
#include "ParadiseHeat.h"

ADistrictGeometry::ADistrictGeometry()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DistrictRoot"));
	RootComponent->SetMobility(EComponentMobility::Static);
	PrimaryActorTick.bCanEverTick = true;
}

void ADistrictGeometry::AddDistrictInstance(UStaticMesh* Mesh, UMaterialInterface* Material,
	const FTransform& Transform, bool bCollision)
{
	if (!Mesh || !Material) return;
	const FString Key = Mesh->GetPathName() + TEXT("|") + Material->GetPathName()
		+ (bCollision ? TEXT("|solid") : TEXT("|visual"));
	TObjectPtr<UInstancedStaticMeshComponent>& Batch = Batches.FindOrAdd(Key);
	if (!Batch)
	{
		Batch = NewObject<UInstancedStaticMeshComponent>(this,
			*FString::Printf(TEXT("DistrictBatch_%d"), Batches.Num()), RF_Transactional);
		AddInstanceComponent(Batch);
		Batch->SetupAttachment(RootComponent);
		Batch->SetMobility(EComponentMobility::Static);
		Batch->SetStaticMesh(Mesh);
		Batch->SetMaterial(0, Material);
		Batch->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Batch->SetCollisionResponseToAllChannels(ECR_Block);
		Batch->SetGenerateOverlapEvents(false);
		Batch->SetCastShadow(bCollision);
		Batch->RegisterComponent();
	}
	Batch->AddInstance(Transform);
}

void ADistrictGeometry::AddDistrictMesh(UStaticMesh* Mesh, const TArray<UMaterialInterface*>& Materials, const FTransform& Transform, bool bCollision)
{
 if (!Mesh) return;
 FString Key=Mesh->GetPathName()+(bCollision?TEXT("|solid"):TEXT("|visual"));
 for(auto* Material:Materials) Key+=TEXT("|")+GetPathNameSafe(Material);
 auto& Batch=Batches.FindOrAdd(Key);
 if(!Batch)
 {
  Batch=NewObject<UInstancedStaticMeshComponent>(this,*FString::Printf(TEXT("MeshBatch_%d"),Batches.Num()),RF_Transactional);
  AddInstanceComponent(Batch); Batch->SetupAttachment(RootComponent); Batch->SetMobility(EComponentMobility::Static);
  Batch->SetStaticMesh(Mesh);
  for(int32 i=0;i<Materials.Num();++i) if(Materials[i]) Batch->SetMaterial(i,Materials[i]);
  Batch->SetCollisionEnabled(bCollision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
  Batch->SetCollisionResponseToAllChannels(ECR_Block); Batch->SetGenerateOverlapEvents(false);
  Batch->SetCullDistances(0,45000); Batch->RegisterComponent();
 }
 Batch->AddInstance(Transform);
}

void ADistrictGeometry::BeginPlay()
{
	Super::BeginPlay();
	int32 Instances = 0;
	for (const auto& Pair : Batches) Instances += Pair.Value->GetInstanceCount();
	UE_LOG(LogParadiseHeat, Display, TEXT("District loaded: %d instances in %d mesh/material/collision batches"), Instances, Batches.Num());
	bAudit = FParse::Param(FCommandLine::Get(), TEXT("DistrictAudit"));
	bPerfCapture = FParse::Param(FCommandLine::Get(), TEXT("CityPerfCapture"));
	SetActorTickEnabled(bAudit || bPerfCapture);
	if (!bAudit || GetWorld()->GetMapName().Contains(TEXT("CityDistrict10x"))) return;
	for (const auto& Pair : Batches)
	{
		if (Pair.Key.Contains(TEXT("M_Asphalt")))
		{
			FTransform T;
			Pair.Value->GetInstanceTransform(0, T, true);
			UE_LOG(LogParadiseHeat, Display, TEXT("District road transform %s, collision=%d pawnResponse=%d"),
				*T.ToHumanReadableString(), int32(Pair.Value->GetCollisionEnabled()), int32(Pair.Value->GetCollisionResponseToChannel(ECC_Pawn)));
		}
	}

	int32 Passed = 0, Total = 0;
	const bool bCityBlock = GetWorld()->GetMapName().Contains(TEXT("CityBlock01"));
	const bool bNeon = GetWorld()->GetMapName().Contains(TEXT("NeonCorner"));
	const TArray<float> SampleX = bCityBlock ? TArray<float>{-6000.f, -3000.f, -1200.f, 0.f, 1200.f, 3000.f, 6000.f} : bNeon ? TArray<float>{-3500.f, -2800.f, -1200.f, 0.f, 1200.f, 2800.f, 3500.f} : TArray<float>{-10500.f, -7500.f, -3500.f, 0.f, 3500.f, 7500.f, 10500.f};
	const TArray<float> SampleY = bCityBlock ? TArray<float>{-1200.f, -500.f, 500.f, 1200.f} : bNeon ? TArray<float>{-1300.f, -500.f, 500.f, 1300.f} : TArray<float>{-1800.f, 0.f, 1100.f, 6500.f};
	for (const float X : SampleX)
	{
		for (const float Y : SampleY)
		{
			FHitResult Hit;
			const bool bGround = GetWorld()->LineTraceSingleByChannel(Hit,
				FVector(X, Y, 600.f), FVector(X, Y, -100.f), ECC_Visibility);
			++Total;
			if (bGround)
			{
				++Passed;
				if (Y == -1800.f) UE_LOG(LogParadiseHeat, Display, TEXT("District ground at %.0f %.0f: z=%.1f %s"), X, Y, Hit.ImpactPoint.Z, *GetNameSafe(Hit.GetComponent()));
			}
			else UE_LOG(LogParadiseHeat, Error, TEXT("District ground audit MISS x=%.0f y=%.0f"), X, Y);
		}
	}
	UE_LOG(LogParadiseHeat, Display, TEXT("District ground collision samples: %d/%d hit"), Passed, Total);
	auto CheckBarrier = [this](const TCHAR* Label, FVector Start, FVector End)
	{
		FHitResult Hit;
		const bool bBlocked = GetWorld()->SweepSingleByChannel(Hit, Start, End,
			FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42.f, 90.f));
		UE_LOG(LogParadiseHeat, Display, TEXT("District barrier %s: %s at %s"),
			Label, bBlocked ? TEXT("BLOCKED") : TEXT("MISS"), *Hit.ImpactPoint.ToString());
	};
	if (bCityBlock)
	{
		CheckBarrier(TEXT("block north boundary"), FVector(0, 11700, 112), FVector(0, 12300, 112));
		CheckBarrier(TEXT("block east boundary"), FVector(11700, 0, 112), FVector(12300, 0, 112));
		CheckBarrier(TEXT("block building facade"), FVector(-3750, 1400, 112), FVector(-3750, 2300, 112));
		return;
	}
	if (bNeon)
	{
		CheckBarrier(TEXT("neon storefront"), FVector(-3000, 1300, 112), FVector(-3000, 2200, 112));
		CheckBarrier(TEXT("neon east boundary"), FVector(3600, 0, 112), FVector(4200, 0, 112));
		CheckBarrier(TEXT("neon rear boundary"), FVector(0, 3150, 112), FVector(0, 3550, 112));
		return;
	}
	CheckBarrier(TEXT("storefront"), FVector(-4000, 1100, 112), FVector(-4000, 2000, 112));
	CheckBarrier(TEXT("waterfront rail"), FVector(2000, -2500, 112), FVector(2000, -3300, 112));
	CheckBarrier(TEXT("east boundary"), FVector(12000, 600, 112), FVector(13000, 600, 112));
}

void ADistrictGeometry::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// A short frame-time observation, not a full performance/handling certification.
	AuditTime += DeltaSeconds;
	++AuditFrames;
	AuditMaxFrame = FMath::Max(AuditMaxFrame, DeltaSeconds);
	if (AuditFrames == 60)
	{
		if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
		{
			UE_LOG(LogParadiseHeat, Display, TEXT("District player after 60 frames: %s velocity %s movement=%d floor=%d"),
				*Player->GetActorLocation().ToString(), *Player->GetVelocity().ToString(),
				int32(Player->GetCharacterMovement()->MovementMode), Player->GetCharacterMovement()->CurrentFloor.bBlockingHit);
		}
	}
	if (!bInitialAuditReported && AuditTime >= 15.f)
	{
		UE_LOG(LogParadiseHeat, Display, TEXT("District initial frame sample: %d frames / %.2fs, avg %.2fms (%.1f FPS), worst %.2fms; includes warm-up"),
			AuditFrames, AuditTime, AuditTime * 1000.f / AuditFrames, AuditFrames / AuditTime, AuditMaxFrame * 1000.f);
		bInitialAuditReported = true;
		if (!bPerfCapture) SetActorTickEnabled(false);
	}
	if (bPerfCapture && !bPerfStarted && AuditTime >= 30.f)
	{
		bPerfStarted = true;
		GEngine->Exec(GetWorld(), TEXT("StartFPSChart"));
	}
	if (bPerfCapture && AuditTime >= 60.f)
	{
		GEngine->Exec(GetWorld(), TEXT("StopFPSChart"));
		FScreenshotRequest::RequestScreenshot(false, true);
		SetActorTickEnabled(false);
	}
}
