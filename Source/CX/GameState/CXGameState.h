#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

class ACXPlayerState;

// 모든 플레이어가 공유하는 경기 진행 단계입니다.
UENUM(BlueprintType)
enum class EGameState : uint8
{
	None,
	Waiting,
	Playing,
	Finished,
	End
};

UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetRemainingTime() const { return RemainingTime; }

	// 각 클라이언트 HUD가 복제받은 경기 정보를 읽어 표시합니다.
	UFUNCTION(BlueprintPure, Category = "Match")
	FString GetGameStateInfo() const;

	void SetGameState(EGameState NewGameState);
	void SetRemainingTime(int32 NewRemainingTime);

protected:
	UPROPERTY(Replicated)
	EGameState CurrentGameState = EGameState::Waiting;

	// 현재 단계의 남은 시간이며, 단위는 초입니다.
	UPROPERTY(Replicated)
	int32 RemainingTime = 0;

	// 공동 우승이 가능하므로 플레이어 상태를 배열로 보관합니다.
	UPROPERTY(Replicated)
	TArray<TObjectPtr<ACXPlayerState>> Winners;
};
