// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXMatchResultUI.generated.h"

class UButton;

/** 결과 화면의 버튼 클릭을 처리합니다. 배치와 외형은 파생 BP에서 설정합니다. */
UCLASS()
class CX_API UCXMatchResultUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 파생 BP에 같은 이름의 Button이 있어야 연결됩니다.
	UPROPERTY(BlueprintReadOnly, Category = "Match Result", meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(BlueprintReadOnly, Category = "Match Result", meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

private:
	UFUNCTION()
	void OnRestartButtonClicked();

	UFUNCTION()
	void OnQuitButtonClicked();
};
