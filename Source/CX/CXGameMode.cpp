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
	if (IsValid(CXGameState))
	{
		// 이번 단계는 종료 상태까지만 연결하고, 승자 계산은 다음에 추가합니다.
		CXGameState->SetRemainingTime(0);
		CXGameState->SetGameState(EGameState::Finished);
		UE_LOG(LogCXCountdown, Log, TEXT("[Countdown] Finished"));
	}
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