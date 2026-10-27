#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "CXItemSpawner.generated.h"

class UBillboardComponent;

UCLASS()
class CX_API ACXItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACXItemSpawner();

	void StartSpawning();
	void StopSpawning();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SpawnItem();

	UFUNCTION()
	void OnItemDestroyed(AActor* DestroyedActor);

	// 에디터에서 위치를 표시하고, 아이템을 생성할 기준 위치를 제공합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<UBillboardComponent> Billboard;

	// 기본은 코인입니다. BP나 배치한 스포너에서 속도 버프로 바꿀 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	TSubclassOf<AActor> ItemClass;

	// 아이템이 파괴된 시점부터 다음 생성까지 기다릴 시간(초)입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SpawnInterval = 2.0f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedItem;

	FTimerHandle SpawnTimerHandle;
};
