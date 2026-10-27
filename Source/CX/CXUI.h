// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXUI.generated.h"


// 포인터 선언에 필요한 클래스 이름만 먼저 알립니다.
class UChatUI;

/**
 *  Simple Vehicle HUD class
 *  Displays the current speed and gear.
 *  Widget setup is handled in a Blueprint subclass.
 */
UCLASS(abstract)
class CX_API UCXUI : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// 디자이너에 배치한 같은 이름의 채팅 위젯을 연결합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UChatUI> UI_Chat;

	/** Controls the display of speed in Km/h or MPH */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Vehicle)
	bool bIsMPH = false;

public:
	// 내부 채팅 위젯에 키보드 입력을 넘깁니다.
	void FocusChatInput();

	// 경기 종료 후에는 채팅만 남겨 결과창 위에 표시합니다.
	void ShowChatOverResult();

	// 수신한 메시지를 내부 채팅 위젯으로 전달합니다.
	void OnNewChatMessage(const FString& InSender, const FString& InMessage);

	/** Called to update the speed display */
	void UpdateSpeed(float NewSpeed);

	/** Called to update the gear display */
	void UpdateGear(int32 NewGear);

protected:

	/** Implemented in Blueprint to display the new speed */
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnSpeedUpdate(float NewSpeed);

	/** Implemented in Blueprint to display the new gear */
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnGearUpdate(int32 NewGear);
};
