#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

class ACXPlayerState;
class APlayerState;

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

	bool IsMatchFinished() const { return CurrentGameState == EGameState::Finished; }

	int32 GetRemainingTime() const { return RemainingTime; }

	// 각 클라이언트 HUD가 복제받은 경기 정보를 읽어 표시합니다.
	UFUNCTION(BlueprintPure, Category = "Match")
	FString GetGameStateInfo() const;

	// 호출한 위젯을 소유한 플레이어의 승패를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Match")
	FString GetMatchResultInfo(APlayerState* LocalPlayerState) const;

	// 결과 화면 상단에 표시할 공용 재시작 안내입니다.
	UFUNCTION(BlueprintPure, Category = "Match")
	FString GetRestartInfo() const;

	int32 GetRestartRemainingTime() const { return RestartRemainingTime; }
	void SetRestartRemainingTime(int32 NewRemainingTime);

	void SetGameState(EGameState NewGameState);
	void SetRemainingTime(int32 NewRemainingTime);

	// 서버가 판정한 공동 우승자 목록을 저장합니다.
	void SetWinners(const TArray<TObjectPtr<ACXPlayerState>>& NewWinners);

protected:
	UFUNCTION()
	void OnRep_CurrentGameState();

	UPROPERTY(ReplicatedUsing = OnRep_CurrentGameState)
	EGameState CurrentGameState = EGameState::Waiting;

	// 현재 단계의 남은 시간이며, 단위는 초입니다.
	UPROPERTY(Replicated)
	int32 RemainingTime = 0;

	// -1은 재시작 요청 없음, 0 이상은 재시작까지 남은 초입니다.
	UPROPERTY(Replicated)
	int32 RestartRemainingTime = -1;

	// 공동 우승이 가능하므로 플레이어 상태를 배열로 보관합니다.
	UPROPERTY(Replicated)
	TArray<TObjectPtr<ACXPlayerState>> Winners;
};
