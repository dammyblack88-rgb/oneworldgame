#pragma once

#include "CoreMinimal.h"
#include "Engine/PrimaryDataAsset.h"
#include "OneWorldRegionData.generated.h"

USTRUCT(BlueprintType)
struct FOneWorldDistrictEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    FName DistrictId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    FText Description;
};

/**
 * Data-driven region descriptor. Region assets describe content identity;
 * they do not imply that a playable map has already been authored.
 */
UCLASS(BlueprintType)
class PARADISEHEAT_API UOneWorldRegionData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    FName CountryId = TEXT("NG");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    FName StateOrTerritoryId = TEXT("LA");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    FName CurrencyId = TEXT("NGN");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    FName TimeZoneId = TEXT("Africa/Lagos");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Region")
    TArray<FOneWorldDistrictEntry> Districts;
};
