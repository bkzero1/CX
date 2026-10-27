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

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void StartWaitingCountdown();
	void HandleWaitingTimerTick();
	void StartPlayingState();
	void HandlePlayingTimerTick();
	void FinishGame();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;

private:
	// 추가 접속자가 있어도 진행 중인 카운트다운을 다시 시작하지 않습니다.
	bool bWaitingCountdownStarted = false;
	FTimerHandle GameTimerHandle;

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckHalfExtent = FVector(250.0, 120.0, 80.0);

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckOffset = FVector(0.0, 0.0, 80.0);

};



