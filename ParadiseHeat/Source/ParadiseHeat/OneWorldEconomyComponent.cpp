#include "OneWorldEconomyComponent.h"

UOneWorldEconomyComponent::UOneWorldEconomyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UOneWorldEconomyComponent::InitializeWallet()
{
    const int64 Previous = Balance;
    Balance = FMath::Max<int64>(0, StartingBalance);
    OnBalanceChanged.Broadcast(Balance, Balance - Previous);
}

void UOneWorldEconomyComponent::AddFunds(const int64 Amount)
{
    if (Amount <= 0) return;
    const int64 Previous = Balance;
    Balance += Amount;
    OnBalanceChanged.Broadcast(Balance, Balance - Previous);
}

bool UOneWorldEconomyComponent::CanAfford(const int64 Amount) const
{
    return Amount >= 0 && Balance >= Amount;
}

bool UOneWorldEconomyComponent::TrySpendFunds(const int64 Amount)
{
    if (Amount <= 0 || !CanAfford(Amount)) return false;
    Balance -= Amount;
    OnBalanceChanged.Broadcast(Balance, -Amount);
    return true;
}
