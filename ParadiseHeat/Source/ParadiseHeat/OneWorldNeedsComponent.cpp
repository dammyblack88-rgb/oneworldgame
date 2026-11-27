#include "OneWorldNeedsComponent.h"

UOneWorldNeedsComponent::UOneWorldNeedsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UOneWorldNeedsComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UOneWorldNeedsComponent::Drain(float& Value, const float RatePerMinute, const float DeltaTime)
{
    Value = FMath::Clamp(Value - (FMath::Max(0.0f, RatePerMinute) * DeltaTime / 60.0f), 0.0f, 100.0f);
}

void UOneWorldNeedsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    Drain(Hunger, HungerLossPerMinute, DeltaTime);
    Drain(Energy, EnergyLossPerMinute, DeltaTime);
    Drain(Hygiene, HygieneLossPerMinute, DeltaTime);
    Drain(Social, SocialLossPerMinute, DeltaTime);
    Drain(Fun, FunLossPerMinute, DeltaTime);
    Drain(Bladder, BladderLossPerMinute, DeltaTime);
}

void UOneWorldNeedsComponent::RestoreNeed(const FName NeedId, const float Amount)
{
    const float Increase = FMath::Max(0.0f, Amount);
    if (NeedId == TEXT("Hunger")) Hunger = FMath::Clamp(Hunger + Increase, 0.0f, 100.0f);
    else if (NeedId == TEXT("Energy")) Energy = FMath::Clamp(Energy + Increase, 0.0f, 100.0f);
    else if (NeedId == TEXT("Hygiene")) Hygiene = FMath::Clamp(Hygiene + Increase, 0.0f, 100.0f);
    else if (NeedId == TEXT("Social")) Social = FMath::Clamp(Social + Increase, 0.0f, 100.0f);
    else if (NeedId == TEXT("Fun")) Fun = FMath::Clamp(Fun + Increase, 0.0f, 100.0f);
    else if (NeedId == TEXT("Bladder")) Bladder = FMath::Clamp(Bladder + Increase, 0.0f, 100.0f);
}

void UOneWorldNeedsComponent::SetNeed(const FName NeedId, const float Value)
{
    const float Clamped = FMath::Clamp(Value, 0.0f, 100.0f);
    if (NeedId == TEXT("Hunger")) Hunger = Clamped;
    else if (NeedId == TEXT("Energy")) Energy = Clamped;
    else if (NeedId == TEXT("Hygiene")) Hygiene = Clamped;
    else if (NeedId == TEXT("Social")) Social = Clamped;
    else if (NeedId == TEXT("Fun")) Fun = Clamped;
    else if (NeedId == TEXT("Bladder")) Bladder = Clamped;
}

float UOneWorldNeedsComponent::GetLowestNeed() const
{
    return FMath::Min(FMath::Min(FMath::Min(Hunger, Energy), FMath::Min(Hygiene, Social)), FMath::Min(Fun, Bladder));
}
