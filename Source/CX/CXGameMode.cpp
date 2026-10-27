// CXGameMode.cpp

#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/CXPlayerState.h"
#include "GameState/CXGameState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCXCountdown, Log, All);

ACXGameMode::ACXGameMode()
{
	PlayerControllerClass = ACXPlayerController::StaticClass();

	// 플레이어별 자원을 관리할 PlayerState 클래스를 지정합니다.
	PlayerStateClass = ACXPlayerState::StaticClass();

	// 모든 플레이어가 공유할 경기 정보 클래스를 지정합니다.
	GameStateClass = ACXGameState::StaticClass();
}

void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 첫 번째 유효한 접속에서만 대기 시간을 시작합니다.
	if (IsValid(NewPlayer) && bWaitingCountdownStarted == false)
	{
		StartWaitingCountdown();
	}
}

void ACXGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(GameTimerHandle);
	GetWorldTimerManager().ClearTimer(RestartTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ACXGameMode::StartWaitingCountdown()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		UE_LOG(LogCXCountdown, Error, TEXT("CXGameState is not configured."));
		return;
	}

	bWaitingCountdownStarted = true;
	CXGameState->SetGameState(EGameState::Waiting);
	CXGameState->SetRemainingTime(5);
	// 같은 핸들을 사용해 대기 타이머와 경기 타이머를 차례로 실행합니다.
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ACXGameMode::HandleWaitingTimerTick, 1.0f, true);
	UE_LOG(LogCXCountdown, Log, TEXT("[Countdown] Waiting: 5"));
}

void ACXGameMode::HandleWaitingTimerTick()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		return;
	}

	const int32 NewRemainingTime = CXGameState->GetRemainingTime() - 1;
	CXGameState->SetRemainingTime(NewRemainingTime);
	UE_LOG(LogCXCountdown, Log, TEXT("[Countdown] Waiting: %d"), CXGameState->GetRemainingTime());
	if (NewRemainingTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		StartPlayingState();
	}
}

void ACXGameMode::StartPlayingState()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	CXGameState->SetGameState(EGameState::Playing);
	// 라이브 세션과 동일하게 우선 30초로 경기 흐름을 확인합니다.
	CXGameState->SetRemainingTime(30);
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ACXGameMode::HandlePlayingTimerTick, 1.0f, true);
	UE_LOG(LogCXCountdown, Log, TEXT("[Countdown] Playing: 30"));
}

void ACXGameMode::HandlePlayingTimerTick()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		return;
	}

	const int32 NewRemainingTime = CXGameState->GetRemainingTime() - 1;
	CXGameState->SetRemainingTime(NewRemainingTime);
	UE_LOG(LogCXCountdown, Log, TEXT("[Countdown] Playing: %d"), CXGameState->GetRemainingTime());
	if (NewRemainingTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		FinishGame();
	}
}

void ACXGameMode::FinishGame()
{
	GetWorldTimerManager().ClearTimer(GameTimerHandle);
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		UE_LOG(LogCXCountdown, Error, TEXT("Cannot determine winners: CXGameState is invalid."));
		return;
	}

	// 코인은 0 이상이므로 -1부터 비교하면 0코인 플레이어도 후보가 됩니다.
	int32 HighestCoins = -1;
	TArray<TObjectPtr<ACXPlayerState>> MatchWinners;
	for (APlayerState* PlayerState : CXGameState->PlayerArray)
	{
		ACXPlayerState* CXPlayerState = Cast<ACXPlayerState>(PlayerState);
		if (IsValid(CXPlayerState) == false)
		{
			continue;
		}

		const int32 PlayerCoins = CXPlayerState->GetCoins();
		if (PlayerCoins > HighestCoins)
		{
			// 더 높은 점수가 나오면 이전 후보들은 더 이상 최고점이 아닙니다.
			HighestCoins = PlayerCoins;
			MatchWinners.Empty();
			MatchWinners.Add(CXPlayerState);
		}
		else if (PlayerCoins == HighestCoins)
		{
			// 현재 최고점과 같으면 기존 후보를 유지하며 공동 우승자로 추가합니다.
			MatchWinners.Add(CXPlayerState);
		}
	}

	CXGameState->SetWinners(MatchWinners);
	CXGameState->SetRemainingTime(0);
	CXGameState->SetGameState(EGameState::Finished);

	// 서버 판정 확인용 로그입니다. 결과 UI는 다음 단계에서 연결합니다.
	UE_LOG(LogCXCountdown, Log, TEXT("[MatchResult] HighestCoins=%d, WinnerCount=%d"), HighestCoins, MatchWinners.Num());
	for (const TObjectPtr<ACXPlayerState>& Winner : MatchWinners)
	{
		UE_LOG(LogCXCountdown, Log, TEXT("[MatchResult] Winner=%s, Coins=%d"), *GetNameSafe(Winner.Get()), Winner->GetCoins());
	}
	UE_LOG(LogCXCountdown, Log, TEXT("[Countdown] Finished"));
}

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> AvailablePlayerStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (IsPlayerStartAvailable(PlayerStart))
		{
			AvailablePlayerStarts.Add(PlayerStart);
		}
	}

	checkf(AvailablePlayerStarts.IsEmpty() == false, TEXT("There is no available Vehicle PlayerStart."));

	const int32 RandomIndex = FMath::RandHelper(AvailablePlayerStarts.Num());
	return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* PlayerStart) const
{
	if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
	{
		return false;
	}

	const FTransform PlayerStartTransform = PlayerStart->GetActorTransform();
	const FVector CheckLocation = PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);
	const FQuat CheckRotation = PlayerStartTransform.GetRotation();
	const FCollisionShape CheckShape = FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false);

	const bool bVehicleExists =
		GetWorld()->OverlapAnyTestByObjectType(
			CheckLocation,
			CheckRotation,
			ObjectQueryParams,
			CheckShape,
			QueryParams
		);

	return !bVehicleExists;
}
void ACXGameMode::RequestMatchRestart()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (HasAuthority() == false || IsValid(CXGameState) == false)
	{
		return;
	}

	// 경기 도중 요청과 이미 시작된 카운트다운의 재설정을 막습니다.
	if (CXGameState->IsMatchFinished() == false || bRestartRequested)
	{
		return;
	}

	bRestartRequested = true;
	CXGameState->SetRestartRemainingTime(5);
	GetWorldTimerManager().SetTimer(RestartTimerHandle, this, &ACXGameMode::HandleRestartTimerTick, 1.0f, true);
	UE_LOG(LogCXCountdown, Log, TEXT("[Restart] Starts in: 5"));
}

void ACXGameMode::HandleRestartTimerTick()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false || CXGameState->IsMatchFinished() == false)
	{
		GetWorldTimerManager().ClearTimer(RestartTimerHandle);
		bRestartRequested = false;
		if (IsValid(CXGameState))
		{
			CXGameState->SetRestartRemainingTime(-1);
		}
		return;
	}

	const int32 NewRemainingTime = FMath::Max(0, CXGameState->GetRestartRemainingTime() - 1);
	CXGameState->SetRestartRemainingTime(NewRemainingTime);
	UE_LOG(LogCXCountdown, Log, TEXT("[Restart] Starts in: %d"), NewRemainingTime);
	if (NewRemainingTime > 0)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(RestartTimerHandle);
	// 현재 맵을 모두 함께 다시 불러옵니다. PlayerState도 새로 만들어 자원을 초기화합니다.
	// 새 GameMode의 PostLogin에서 기존 시작 대기 5초가 다시 진행됩니다.
	if (GetWorld()->ServerTravel(TEXT("?Restart?NoSeamlessTravel")) == false)
	{
		// 이동 요청이 거절되면 다시 버튼을 눌러 요청할 수 있도록 풀어줍니다.
		bRestartRequested = false;
		CXGameState->SetRestartRemainingTime(-1);
		UE_LOG(LogCXCountdown, Error, TEXT("[Restart] ServerTravel request was rejected."));
	}
}
