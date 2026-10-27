// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CXMatchResultUI.h"
#include "Components/Button.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "CXPlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

void UCXMatchResultUI::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);

	// 같은 위젯을 다시 표시해도 동일한 클릭 함수가 중복 등록되지 않게 합니다.
	if (IsValid(RestartButton))
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnRestartButtonClicked);
	}
	if (IsValid(QuitButton))
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnQuitButtonClicked);
	}
}

FReply UCXMatchResultUI::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 버튼에 포커스가 있어도 Enter는 버튼 실행보다 채팅 시작을 우선합니다.
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		if (InKeyEvent.IsRepeat() == false)
		{
			if (ACXPlayerController* PlayerController = GetOwningPlayer<ACXPlayerController>())
			{
				PlayerController->BeginChatInput();
			}
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UCXMatchResultUI::OnRestartButtonClicked()
{
	// 기존 컨트롤러의 서버 RPC 흐름을 재사용합니다. 5초 안내도 그대로 유지됩니다.
	ACXPlayerController* CXPlayerController = GetOwningPlayer<ACXPlayerController>();
	if (IsValid(CXPlayerController))
	{
		CXPlayerController->RequestMatchRestart();
	}
}

void UCXMatchResultUI::OnQuitButtonClicked()
{
	// 종료는 이 화면을 소유한 로컬 플레이어 쪽에서 요청합니다.
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (IsValid(OwningPlayer) == false || OwningPlayer->IsLocalController() == false)
	{
		return;
	}

	UKismetSystemLibrary::QuitGame(this, OwningPlayer, EQuitPreference::Quit, false);
}
