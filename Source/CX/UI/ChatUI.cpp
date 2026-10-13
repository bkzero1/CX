// ChatUI.cpp

#include "UI/ChatUI.h"
#include "CXPlayerController.h"

void UChatUI::OnNewChatMessage(const FString Sender, const FString Message)
{
	// 채팅 기록 표시는 이후 단계에서 구현합니다.
}

void UChatUI::NativeConstruct()
{
	Super::NativeConstruct();
	// 입력이 확정되면 우리가 만든 처리 함수를 호출하도록 연결합니다.
	if (IsValid(EditableTextBox_ChatInput))
	{
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &UChatUI::OnChatInputTextCommitted);
	}
}

void UChatUI::NativeDestruct()
{
	// 부모의 정리 전에 이 위젯에서 연결한 입력 이벤트를 해제합니다.
	if (IsValid(EditableTextBox_ChatInput))
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &UChatUI::OnChatInputTextCommitted);
	}

	Super::NativeDestruct();
}

void UChatUI::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	// 포커스를 잃어서 확정된 입력은 제외하고 Enter 입력만 처리합니다.
	if (CommitMethod == ETextCommit::OnEnter)
	{
		// 입력창을 비우기 전에 전송할 문자열을 보관합니다.
		const FString Message = Text.ToString();

		if (IsValid(EditableTextBox_ChatInput))
		{
			EditableTextBox_ChatInput->SetText(FText::GetEmpty());
		}

		// 이 위젯을 소유한 컨트롤러를 통해 서버에 전달합니다.
		if (ACXPlayerController* PlayerController = GetOwningPlayer<ACXPlayerController>())
		{
			PlayerController->CTS_ChatMessage(Message);
		}
	}
}
