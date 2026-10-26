// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/CXPlayerState.h"
#include "Net/UnrealNetwork.h"

void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// 부모 클래스의 복제 등록을 유지합니다.
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 이 플레이어를 소유한 클라이언트에게만 두 값을 복제합니다.
	DOREPLIFETIME_CONDITION(ThisClass, Coins, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, BonusMaxSpeed, COND_OwnerOnly);
}

void ACXPlayerState::AddCoins(int32 Amount)
{
	if (HasAuthority() == false || Amount <= 0)
	{
		return;
	}

	Coins += Amount;
}

bool ACXPlayerState::TryRemoveCoins(int32 Amount)
{
	if (HasAuthority() == false || Coins < Amount || Amount <= 0)
	{
		return false;
	}
	
	Coins -= Amount;

	return true;
}

void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
	if (HasAuthority() == false || Amount <= 0.0f)
	{
		return;
	}

	BonusMaxSpeed += Amount;
}

int32 ACXPlayerState::GetCoins() const
{
	return Coins;
}

float ACXPlayerState::GetBonusMaxSpeed() const
{
	return BonusMaxSpeed;
}
