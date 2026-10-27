#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXSpeedBuffItem.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UParticleSystem;
class USoundBase;

UCLASS()
class CX_API ACXSpeedBuffItem : public AActor
{
	GENERATED_BODY()

public:
	ACXSpeedBuffItem();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 라이브 세션의 최종 수정처럼 획득 연출은 Reliable로 전달합니다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPickupEffects();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpeedBuff")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpeedBuff")
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpeedBuff")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	// C++ 기본값을 사용하며, BP에서 획득 파티클과 소리를 교체할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpeedBuff")
	TObjectPtr<UParticleSystem> PickupParticle;

	// 획득 효과만 확대합니다. 아이템 메시와 충돌 범위에는 영향을 주지 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpeedBuff", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float PickupParticleScale = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpeedBuff")
	TObjectPtr<USoundBase> PickupSound;

private:
	// 서버에서만 사용하며, 같은 아이템의 중복 지급을 막습니다.
	bool bCollected = false;
};
