#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OneWorldEconomyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOneWorldBalanceChanged, int64, NewBalance, int64, Delta);

/** Simple player wallet. Values are whole currency units (NGN for the initial Lagos region). */
UCLASS(ClassGroup=(ONEWORLD), meta=(BlueprintSpawnableComponent))
class PARADISEHEAT_API UOneWorldEconomyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UOneWorldEconomyComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ONE WORLD|Economy", meta=(ClampMin="0"))
    int64 StartingBalance = 5000;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ONE WORLD|Economy")
    int64 Balance = 0;

    UPROPERTY(BlueprintAssignable, Category="ONE WORLD|Economy")
    FOneWorldBalanceChanged OnBalanceChanged;

    UFUNCTION(BlueprintCallable, Category="ONE WORLD|Economy")
    void InitializeWallet();

    UFUNCTION(BlueprintCallable, Category="ONE WORLD|Economy")
    void AddFunds(int64 Amount);

    UFUNCTION(BlueprintCallable, Category="ONE WORLD|Economy")
    bool TrySpendFunds(int64 Amount);

    UFUNCTION(BlueprintPure, Category="ONE WORLD|Economy")
    bool CanAfford(int64 Amount) const;
};
