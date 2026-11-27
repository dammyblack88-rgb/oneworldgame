#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OneWorldNeedsComponent.generated.h"

/**
 * Lightweight life-simulation needs model.
 * Values are normalized to 0..100; 100 is fully satisfied.
 */
UCLASS(ClassGroup=(ONEWORLD), meta=(BlueprintSpawnableComponent))
class PARADISEHEAT_API UOneWorldNeedsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UOneWorldNeedsComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs", meta=(ClampMin="0", ClampMax="100"))
    float Hunger = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs", meta=(ClampMin="0", ClampMax="100"))
    float Energy = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs", meta=(ClampMin="0", ClampMax="100"))
    float Hygiene = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs", meta=(ClampMin="0", ClampMax="100"))
    float Social = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs", meta=(ClampMin="0", ClampMax="100"))
    float Fun = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs", meta=(ClampMin="0", ClampMax="100"))
    float Bladder = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs|Rates")
    float HungerLossPerMinute = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs|Rates")
    float EnergyLossPerMinute = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs|Rates")
    float HygieneLossPerMinute = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs|Rates")
    float SocialLossPerMinute = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs|Rates")
    float FunLossPerMinute = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ONE WORLD|Needs|Rates")
    float BladderLossPerMinute = 1.0f;

    UFUNCTION(BlueprintCallable, Category="ONE WORLD|Needs")
    void RestoreNeed(FName NeedId, float Amount);

    UFUNCTION(BlueprintCallable, Category="ONE WORLD|Needs")
    void SetNeed(FName NeedId, float Value);

    UFUNCTION(BlueprintPure, Category="ONE WORLD|Needs")
    float GetLowestNeed() const;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void Drain(float& Value, float RatePerMinute, float DeltaTime);
};
