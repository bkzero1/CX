#include "GameState/CXGameState.h"
#include "PlayerState/CXPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "CXPlayerController.h"
#include "Engine/World.h"

void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 경기 정보는 소유자 한 명이 아니라 모든 클라이언트에 전달합니다.
	DOREPLIFETIME(ThisClass, CurrentGameState);
	DOREPLIFETIME(ThisClass, RemainingTime);
	DOREPLIFETIME(ThisClass, Winners);
	DOREPLIFETIME(ThisClass, RestartRemainingTime);
}

void ACXGameState::SetGameState(EGameState NewGameState)
{
	// 경기 상태의 변경 권한은 서버에만 있습니다.
	if (HasAuthority() == false)
	{
		return;
	}

	CurrentGameState = NewGameState;
	// C++에서 값을 바꾼 서버에서도 로컬 플레이어(리슨 서버)를 처리합니다.
	OnRep_CurrentGameState();
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

void ACXGameState::SetWinners(const TArray<TObjectPtr<ACXPlayerState>>& NewWinners)
{
	if (HasAuthority() == false)
	{
		return;
	}

	// Winners는 이미 복제 대상으로 등록되어 있습니다.
	Winners = NewWinners;
}

FString ACXGameState::GetMatchResultInfo(APlayerState* LocalPlayerState) const
{
	// 경기 종료 전이나 내 PlayerState가 준비되기 전에는 결과를 표시하지 않습니다.
	if (CurrentGameState != EGameState::Finished || IsValid(LocalPlayerState) == false)
	{
		return FString();
	}

	// 종료 상태를 먼저 받았더라도 승자 정보가 준비되기 전에는 패배로 단정하지 않습니다.
	if (Winners.IsEmpty())
	{
		return TEXT("Waiting for result...");
	}

	bool bIsWinner = false;
	for (const TObjectPtr<ACXPlayerState>& Winner : Winners)
	{
		if (IsValid(Winner.Get()) == false)
		{
			return TEXT("Waiting for result...");
		}

		if (Winner.Get() == LocalPlayerState)
		{
			bIsWinner = true;
		}
	}

	if (bIsWinner)
	{
		return Winners.Num() > 1 ? TEXT("Joint Victory!") : TEXT("Victory!");
	}

	return TEXT("Defeat");
}

void ACXGameState::OnRep_CurrentGameState()
{
	if (IsMatchFinished() == false)
	{
		return;
	}

	// 복제된 종료 상태를 받은 각 컴퓨터에서 자신의 결과 화면만 표시합니다.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ACXPlayerController* CXController = Cast<ACXPlayerController>(It->Get());
		if (IsValid(CXController) && CXController->IsLocalController() && CXController->HasActorBegunPlay())
		{
			CXController->ShowMatchResult();
		}
	}
}

void ACXGameState::SetRestartRemainingTime(int32 NewRemainingTime)
{
	if (HasAuthority() == false)
	{
		return;
	}

	RestartRemainingTime = FMath::Max(-1, NewRemainingTime);
	// 초 단위 변경을 다음 복제 갱신에서 보내도록 요청합니다.
	ForceNetUpdate();
}

FString ACXGameState::GetRestartInfo() const
{
	if (IsMatchFinished() == false || RestartRemainingTime < 0)
	{
		return FString();
	}
	if (RestartRemainingTime == 0)
	{
		return TEXT("경기를 다시 시작합니다...");
	}
	return FString::Printf(TEXT("%d초 후 경기를 다시 시작합니다"), RestartRemainingTime);
}
