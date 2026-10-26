// CXPlayerController.cpp


#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "PlayerState/CXPlayerState.h"

void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}
	
	// spawn the UI widget and add it to the viewport
	VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);

	check(VehicleUI);

	VehicleUI->AddToViewport();
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);

		// optionally add the steering wheel context
		if (bUseSteeringWheelControls && SteeringWheelInputMappingContext)
		{
			Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 1);
		}
	}
}

void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, VehiclePawn);
}

void ACXPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<ACXPawn>(InPawn);
}

// Server RPC의 실제 처리는 서버에서 실행됩니다.
void ACXPlayerController::CTS_ChatMessage_Implementation(const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("Server received chat: %s"), *Message);

	// 보낸 사람의 이름은 서버에 있는 PlayerState에서 가져옵니다.
	const APlayerState* SenderPlayerState = GetPlayerState<APlayerState>();
	if (IsValid(SenderPlayerState) == false)
	{
		return;
	}
	const FString Sender = SenderPlayerState->GetPlayerName();

	// 각 컨트롤러의 소유 클라이언트에 전달하므로 보낸 사람도 수신합니다.
	for (TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It)
	{
		It->STC_ChatMessage(Sender, Message);
	}
}

void ACXPlayerController::STC_ChatMessage_Implementation(const FString& Sender, const FString& Message)
{
	// 수신 내용을 로그로 확인하고, 이 플레이어의 HUD에도 전달합니다.
	UE_LOG(LogTemp, Log, TEXT("Client received chat: %s: %s"), *Sender, *Message);
	VehicleUI->OnNewChatMessage(Sender, Message);
}

// 임시 복제 테스트: 실행 확인 후 제거합니다.
void ACXPlayerController::TestAddCoins(int32 Amount)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (IsLocalController() && Amount > 0)
	{
		CTS_TestAddCoins(Amount);
	}
#endif
}

void ACXPlayerController::CTS_TestAddCoins_Implementation(int32 Amount)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	ACXPlayerState* CXPlayerState = GetPlayerState<ACXPlayerState>();
	if (!IsValid(CXPlayerState) || Amount <= 0)
	{
		return;
	}

	const int32 BeforeCoins = CXPlayerState->GetCoins();
	if (BeforeCoins > MAX_int32 - Amount)
	{
		return;
	}

	// 기존 서버 전용 함수를 호출하고 변경 전후를 기록합니다.
	CXPlayerState->AddCoins(Amount);
	UE_LOG(LogTemp, Log, TEXT("[TempCoinTest][Server] %s: %d -> %d"),
		*CXPlayerState->GetPlayerName(), BeforeCoins, CXPlayerState->GetCoins());
#endif
}
