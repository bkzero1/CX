// ChatUI.cpp

#include "UI/ChatUI.h"
#include "CXPlayerController.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"

void UChatUI::FocusChatInput()
{
	ACXPlayerController* PlayerController = GetOwningPlayer<ACXPlayerController>();
	if (IsValid(PlayerController) == false || IsValid(EditableTextBox_ChatInput) == false)
	{
		return;
	}

	// 입력 중 W/Space 등의 키가 차량 조작으로 전달되지 않게 합니다.
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(EditableTextBox_ChatInput->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	EditableTextBox_ChatInput->SetUserFocus(PlayerController);
}

FReply UChatUI::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Esc는 입력창보다 먼저 받아, 메시지를 보내지 않고 채팅을 끝냅니다.
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		EditableTextBox_ChatInput->SetText(FText::GetEmpty());
		if (ACXPlayerController* PlayerController = GetOwningPlayer<ACXPlayerController>())
		{
			PlayerController->FinishChatInput();
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UChatUI::OnNewChatMessage(const FString& InSender, const FString& InMessage)
{
	// 이번에 받은 메시지를 '발신자: 본문' 형태로 만듭니다.
	FString NewMessage = InSender + TEXT(": ") + InMessage;

	// 기존 기록에 줄바꿈을 붙여 새 메시지가 다음 줄에 오게 합니다.
	FString OldMessage = MultiLineEditableTextBox_ChatLog->GetText().ToString();
	OldMessage += TEXT("\n");

	// 합친 기록을 화면 표시용 FText로 변환하여 반영합니다.
	FText Message = FText::FromString(OldMessage + NewMessage);
	MultiLineEditableTextBox_ChatLog->SetText(Message);
}

void UChatUI::NativeConstruct()
{
	Super::NativeConstruct();
	// 입력이 확정되면 우리가 만든 처리 함수를 호출하도록 연결합니다.
	if (IsValid(EditableTextBox_ChatInput))
	{
		// 전송 처리에서 게임으로 돌려준 포커스를 Slate가 뒤늦게 지우지 않게 합니다.
		EditableTextBox_ChatInput->SetClearKeyboardFocusOnCommit(false);
		EditableTextBox_ChatInput->OnTextCommitted.AddUniqueDynamic(this, &UChatUI::OnChatInputTextCommitted);
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
		const FString Message = Text.ToString().TrimStartAndEnd();

		if (IsValid(EditableTextBox_ChatInput))
		{
			EditableTextBox_ChatInput->SetText(FText::GetEmpty());
		}

		// 이 위젯을 소유한 컨트롤러를 통해 서버에 전달합니다.
		if (ACXPlayerController* PlayerController = GetOwningPlayer<ACXPlayerController>())
		{
			// 빈 입력은 전송하지 않지만, Enter로 채팅을 끝낼 수는 있습니다.
			if (Message.IsEmpty() == false)
			{
				PlayerController->CTS_ChatMessage(Message);
			}
			PlayerController->FinishChatInput();
		}
	}
}
