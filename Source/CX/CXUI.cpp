// Copyright Epic Games, Inc. All Rights Reserved.


#include "CXUI.h"
#include "UI/ChatUI.h"
#include "Components/PanelWidget.h"
#include "Blueprint/GameViewportSubsystem.h"

void UCXUI::FocusChatInput()
{
	if (IsValid(UI_Chat))
	{
		UI_Chat->FocusChatInput();
	}
}

void UCXUI::ShowChatOverResult()
{
	UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld());
	if (IsValid(UI_Chat) == false || ViewportSubsystem == nullptr)
	{
		return;
	}

	// 채팅의 부모 레이아웃은 유지하고, 다른 HUD 영역만 숨깁니다.
	// Hidden은 공간을 유지하므로 기존 채팅 위치와 크기가 바뀌지 않습니다.
	UWidget* ChatBranch = UI_Chat;
	while (UPanelWidget* ParentPanel = ChatBranch->GetParent())
	{
		for (UWidget* Child : ParentPanel->GetAllChildren())
		{
			if (Child != ChatBranch)
			{
				Child->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		// 빈 HUD 영역이 아래 결과창 버튼의 마우스 입력을 가로채지 않게 합니다.
		ParentPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ChatBranch = ParentPanel;
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// 위젯을 제거/재생성하지 않고 표시 순서만 올려 채팅 기록을 보존합니다.
	FGameViewportWidgetSlot ViewportSlot = ViewportSubsystem->GetWidgetSlot(this);
	ViewportSlot.ZOrder = 101;
	ViewportSubsystem->SetWidgetSlot(this, ViewportSlot);
}

void UCXUI::OnNewChatMessage(const FString& InSender, const FString& InMessage)
{
	// HUD가 받은 발신자와 본문을 채팅 위젯에 그대로 넘깁니다.
	UI_Chat->OnNewChatMessage(InSender, InMessage);
}

void UCXUI::UpdateSpeed(float NewSpeed)
{
	// format the speed to KPH or MPH
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	// call the Blueprint handler
	OnSpeedUpdate(FormattedSpeed);
}

void UCXUI::UpdateGear(int32 NewGear)
{
	// call the Blueprint handler
	OnGearUpdate(NewGear);
}