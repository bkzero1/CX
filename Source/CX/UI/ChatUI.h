// ChatUI.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "ChatUI.generated.h"

UCLASS()
class CX_API UChatUI : public UUserWidget
{
	GENERATED_BODY()

public:
	// 자기 플레이어의 입력창에 포커스를 주고 UI 전용 입력으로 전환합니다.
	void FocusChatInput();

	// 수신한 발신자와 본문을 채팅 기록에 추가합니다.
	void OnNewChatMessage(const FString& InSender, const FString& InMessage);

protected:
	// 위젯의 입력 이벤트를 연결하고 해제할 시점입니다.
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 입력 확정 이벤트에 연결할 함수입니다. Enter 여부는 CommitMethod로 구분합니다.
	UFUNCTION()
	void OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	// 위젯 블루프린트에서 같은 이름의 요소와 연결합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ChatScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMultiLineEditableTextBox> MultiLineEditableTextBox_ChatLog;
};
