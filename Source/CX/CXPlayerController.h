// CXPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CXPlayerController.generated.h"

class UInputMappingContext;
class ACXPawn;
class UCXUI;
class UUserWidget;

/**
 *  Vehicle Player Controller class
 *  Handles input mapping and user interface
 */
UCLASS(abstract)
class CX_API ACXPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACXPlayerController();
	void ShowMatchResult();

	// 게임 화면과 결과 위젯에서 함께 사용하는 채팅 진입점입니다.
	void BeginChatInput();

	// 채팅 전송/취소 후 현재 경기 상태에 맞는 화면으로 복귀합니다.
	void FinishChatInput();

	// 결과 위젯의 버튼이 자기 컨트롤러를 통해 서버에 재시작을 요청합니다.
	UFUNCTION(BlueprintCallable, Category = "Match")
	void RequestMatchRestart();

protected:
	UFUNCTION(Server, Reliable)
	void CTS_RequestMatchRestart();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> MatchResultWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MatchResultWidget;

	// 채팅 위젯에서 protected RPC를 호출할 수 있도록 허용합니다.
	friend class UChatUI;

protected:

	// 소유 클라이언트가 입력한 채팅을 서버로 전달합니다.
	UFUNCTION(Server, Reliable)
	void CTS_ChatMessage(const FString& Message);

	// 이 컨트롤러를 소유한 클라이언트에 채팅을 전달합니다.
	UFUNCTION(Client, Reliable)
	void STC_ChatMessage(const FString& Sender, const FString& Message);

	/** Input Mapping Context to be used for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext;

	/** If true, the optional steering wheel input mapping context will be registered */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	bool bUseSteeringWheelControls = false;

	/** Optional Input Mapping Context to be used for steering wheel input.
	 *  This is added alongside the default Input Mapping Context and does not block other forms of input.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta=(EditCondition="bUseSteeringWheelControls"))
	UInputMappingContext* SteeringWheelInputMappingContext;

	/** Pointer to the controlled vehicle pawn */
	UPROPERTY(Replicated)
	TObjectPtr<ACXPawn> VehiclePawn;

	/** Type of the UI to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UCXUI> VehicleUIClass;

	/** Pointer to the UI widget */
	UPROPERTY(Transient)
	TObjectPtr<UCXUI> VehicleUI;

	

	// Begin Actor interface
protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

public:

	virtual void Tick(float Delta) override;

	// End Actor interface

	// Begin PlayerController interface
protected:

	virtual void OnPossess(APawn* InPawn) override;

	// End PlayerController interface
};
