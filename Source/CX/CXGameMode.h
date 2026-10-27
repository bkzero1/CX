// CXGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

UCLASS(MinimalAPI)
class ACXGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACXGameMode();
	void RequestMatchRestart();

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void StartWaitingCountdown();
	void HandleWaitingTimerTick();
	void StartPlayingState();
	void HandlePlayingTimerTick();
	void FinishGame();
	void HandleRestartTimerTick();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;

private:
	// 추가 접속자가 있어도 진행 중인 카운트다운을 다시 시작하지 않습니다.
	bool bWaitingCountdownStarted = false;
	FTimerHandle GameTimerHandle;

	// 여러 플레이어가 눌러도 첫 요청의 카운트다운만 유지합니다.
	bool bRestartRequested = false;
	FTimerHandle RestartTimerHandle;

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckHalfExtent = FVector(250.0, 120.0, 80.0);

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckOffset = FVector(0.0, 0.0, 80.0);

};



