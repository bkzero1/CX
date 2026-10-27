// CXPlayerController.cpp


#include "CXPlayerController.h"
#include "CXGameMode.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameState/CXGameState.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"

ACXPlayerController::ACXPlayerController()
{
	// 기본 결과 화면을 연결하며, 파생 BP에서 다른 위젯으로 바꿀 수 있습니다.
	static ConstructorHelpers::FClassFinder<UUserWidget> ResultWidgetAsset(TEXT("/Game/UI/WBP_MatchResult"));
	if (ResultWidgetAsset.Succeeded())
	{
		MatchResultWidgetClass = ResultWidgetAsset.Class;
	}
}

void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}
	
	// 이전 경기의 UI 전용 입력을 새 경기의 게임 입력으로 되돌립니다.
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;

	// spawn the UI widget and add it to the viewport
	VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);

	check(VehicleUI);

	VehicleUI->AddToViewport();

	// 종료 상태를 먼저 받은 뒤 이 컨트롤러가 시작된 경우도 처리합니다.
	const ACXGameState* CXGameState = GetWorld()->GetGameState<ACXGameState>();
	if (IsValid(CXGameState) && CXGameState->IsMatchFinished())
	{
		ShowMatchResult();
	}
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 채팅 시작 단축키는 컨트롤러가 받습니다. 입력창 안의 Enter는 위젯이 처리합니다.
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ThisClass::BeginChatInput);
	
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

void ACXPlayerController::BeginChatInput()
{
	if (IsLocalController() == false || IsValid(VehicleUI) == false)
	{
		return;
	}

	// 누른 채로 채팅을 열었던 이동 키가 계속 눌린 상태로 남지 않게 해제합니다.
	FlushPressedKeys();
	VehicleUI->FocusChatInput();
}

void ACXPlayerController::FinishChatInput()
{
	if (IsLocalController() == false)
	{
		return;
	}

	if (IsValid(MatchResultWidget))
	{
		// 결과창으로 복귀하고, 다음 Enter도 결과 위젯이 받도록 포커스를 줍니다.
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MatchResultWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		return;
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
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

void ACXPlayerController::ShowMatchResult()
{
	// 서버의 원격 컨트롤러에서는 만들지 않고, 중복 생성도 막습니다.
	if (IsLocalController() == false || IsValid(MatchResultWidget))
	{
		return;
	}
	if (MatchResultWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("MatchResultWidgetClass is not configured."));
		return;
	}

	// this를 Owning Player로 전달하여 위젯이 자기 PlayerState를 찾도록 합니다.
	MatchResultWidget = CreateWidget<UUserWidget>(this, MatchResultWidgetClass);
	if (IsValid(MatchResultWidget) == false)
	{
		return;
	}
	MatchResultWidget->AddToViewport(100);
	if (IsValid(VehicleUI))
	{
		// 결과창은 유지하고, 기존 채팅 영역만 그 위에 표시합니다.
		VehicleUI->ShowChatOverResult();
	}

	// 최초 표시와 채팅 종료가 같은 결과창 포커스 설정을 사용합니다.
	FinishChatInput();
}

void ACXPlayerController::RequestMatchRestart()
{
	if (IsLocalController())
	{
		CTS_RequestMatchRestart();
	}
}

void ACXPlayerController::CTS_RequestMatchRestart_Implementation()
{
	// GameMode는 서버에만 있으므로 RPC로 서버에 도착한 뒤 찾습니다.
	ACXGameMode* CXGameMode = GetWorld()->GetAuthGameMode<ACXGameMode>();
	if (IsValid(CXGameMode))
	{
		CXGameMode->RequestMatchRestart();
	}
}

void ACXPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 맵을 떠날 때 이 컨트롤러가 만든 화면을 정리합니다.
	if (IsValid(MatchResultWidget))
	{
		MatchResultWidget->RemoveFromParent();
		MatchResultWidget = nullptr;
	}
	if (IsValid(VehicleUI))
	{
		VehicleUI->RemoveFromParent();
		VehicleUI = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
