#include "GameState/CXGameState.h"
#include "PlayerState/CXPlayerState.h"
#include "Net/UnrealNetwork.h"

void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 경기 정보는 소유자 한 명이 아니라 모든 클라이언트에 전달합니다.
	DOREPLIFETIME(ThisClass, CurrentGameState);
	DOREPLIFETIME(ThisClass, RemainingTime);
	DOREPLIFETIME(ThisClass, Winners);
}

void ACXGameState::SetGameState(EGameState NewGameState)
{
	// 경기 상태의 변경 권한은 서버에만 있습니다.
	if (HasAuthority() == false)
	{
		return;
	}

	CurrentGameState = NewGameState;
}

void ACXGameState::SetRemainingTime(int32 NewRemainingTime)
{
	if (HasAuthority() == false)
	{
		return;
	}

	// 시간이 음수가 되지 않도록 0을 하한으로 둡니다.
	RemainingTime = FMath::Max(0, NewRemainingTime);
}

FString ACXGameState::GetGameStateInfo() const
{
	// 서버에서 시간을 계산하고, UI는 현재 복제된 값을 읽기만 합니다.
	switch (CurrentGameState)
	{
	case EGameState::Waiting:
		return FString::Printf(TEXT("Waiting | Starts in: %d s"), RemainingTime);
	case EGameState::Playing:
		return FString::Printf(TEXT("Playing | Remaining: %d s"), RemainingTime);
	case EGameState::Finished:
		return TEXT("Finished | Remaining: 0 s");
	default:
		// None, End 등 실제 경기 단계가 아닌 값은 표시하지 않습니다.
		return FString();
	}
}
