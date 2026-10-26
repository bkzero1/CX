// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 저장된 자원을 읽습니다. 조회는 객체의 상태를 변경하지 않습니다.
	int32 GetCoins() const;
	float GetBonusMaxSpeed() const;

	void AddCoins(int32 Amount);

	bool TryRemoveCoins(int32 Amount);

	void AddBonusMaxSpeed(float Amount);

protected:
	// 플레이어가 보유한 원화와 추가 최대 속도입니다.
	UPROPERTY(Replicated)
	int32 Coins = 0;

	UPROPERTY(Replicated)
	float BonusMaxSpeed = 0.f;
};
