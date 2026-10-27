#include "Item/CXItemSpawner.h"
#include "Item/CXCoinItem.h"
#include "Components/BillboardComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ACXItemSpawner::ACXItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 스포너는 레벨에 배치하고, 생성한 아이템이 자신의 복제 설정을 사용합니다.
	bReplicates = false;
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	SetRootComponent(Billboard);
	Billboard->SetHiddenInGame(true);

	// BP에서 조정한 아이템 설정도 사용하도록 코인 BP를 기본 클래스로 연결합니다.
	ItemClass = ACXCoinItem::StaticClass();
	static ConstructorHelpers::FClassFinder<ACXCoinItem> CoinItemClassAsset(
		TEXT("/Game/Item/BP_CoinItem"));
	if (CoinItemClassAsset.Succeeded())
	{
		ItemClass = CoinItemClassAsset.Class;
	}
}

void ACXItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnItem();
	}
}

void ACXItemSpawner::SpawnItem()
{
	// 서버만 생성하며, 기존 아이템이 남아 있으면 중복 생성하지 않습니다.
	if (HasAuthority() == false || IsValid(SpawnedItem))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || ItemClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemSpawner] Missing world or ItemClass: %s"), *GetName());
		return;
	}

	const FTransform SpawnTransform = GetActorTransform();
	SpawnedItem = World->SpawnActorDeferred<AActor>(ItemClass, SpawnTransform,
		this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(SpawnedItem) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemSpawner] Spawn failed: %s"), *GetName());
		return;
	}

	// 생성 완료 즉시 오버랩으로 파괴되어도 알림을 받도록 먼저 구독합니다.
	SpawnedItem->OnDestroyed.AddDynamic(this, &ThisClass::OnItemDestroyed);
	SpawnedItem->FinishSpawning(SpawnTransform);
}

void ACXItemSpawner::OnItemDestroyed(AActor* DestroyedActor)
{
	if (HasAuthority() == false || DestroyedActor != SpawnedItem)
	{
		return;
	}

	// 반복 타이머가 아니라, 매번 아이템이 사라질 때 한 번씩 예약합니다.
	SpawnedItem = nullptr;
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ThisClass::SpawnItem,
		FMath::Max(SpawnInterval, 0.01f), false);
}

void ACXItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 종료 후 재생성하지 않도록 예약과 구독을 해제한 뒤 남은 아이템을 정리합니다.
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	if (HasAuthority() && IsValid(SpawnedItem))
	{
		SpawnedItem->OnDestroyed.RemoveDynamic(this, &ThisClass::OnItemDestroyed);
		SpawnedItem->Destroy();
		SpawnedItem = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
