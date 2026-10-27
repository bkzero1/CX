#include "Item/CXSpeedBuffItem.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "CXPawn.h"
#include "PlayerState/CXPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ACXSpeedBuffItem::ACXSpeedBuffItem()
{
	// 속도 버프 전용 파티클을 기본값으로 사용하며, BP에서 교체할 수 있습니다.
	static ConstructorHelpers::FObjectFinder<UParticleSystem> PickupParticleAsset(
		TEXT("/Game/StarterContent/Particles/P_SpeedBuffPickup.P_SpeedBuffPickup"));
	if (PickupParticleAsset.Succeeded())
	{
		PickupParticle = PickupParticleAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> PickupSoundAsset(
		TEXT("/Engine/VREditor/Sounds/UI/Camera_Shutter.Camera_Shutter"));
	if (PickupSoundAsset.Succeeded())
	{
		PickupSound = PickupSoundAsset.Object;
	}

	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(1.0f);
	SetMinNetUpdateFrequency(1.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 눈에 보이는 메시와 별개로, 반지름 100cm의 구가 차량 진입을 감지합니다.
	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(SceneRoot);
	OverlapSphere->InitSphereRadius(100.0f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionObjectType(ECC_WorldDynamic);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	OverlapSphere->SetGenerateOverlapEvents(true);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(SceneRoot);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 라이브 세션처럼 원뿔 메시를 사용하며, BP에서 교체할 수 있습니다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ItemMeshAsset(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ItemMeshAsset.Succeeded())
	{
		ItemMesh->SetStaticMesh(ItemMeshAsset.Object);
	}

	// 원본 메시 에셋 대신 이 컴포넌트의 첫 번째 머티리얼 슬롯에 적용합니다.
	static ConstructorHelpers::FObjectFinder<UMaterial> ItemMaterialAsset(
		TEXT("/Engine/EngineMaterials/M_Emissive.M_Emissive"));
	if (ItemMaterialAsset.Succeeded())
	{
		ItemMesh->SetMaterial(0, ItemMaterialAsset.Object);
	}
}

void ACXSpeedBuffItem::BeginPlay()
{
	Super::BeginPlay();

	// 획득 판정과 추가 최대 속도 변경은 서버에서만 실행합니다.
	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapSphereBeginOverlap);
	}
}

void ACXSpeedBuffItem::OnOverlapSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority() == false || bCollected)
	{
		return;
	}

	ACXPawn* VehiclePawn = Cast<ACXPawn>(OtherActor);
	if (IsValid(VehiclePawn) == false)
	{
		return;
	}

	ACXPlayerState* PlayerState = VehiclePawn->GetPlayerState<ACXPlayerState>();
	if (IsValid(PlayerState) == false)
	{
		return;
	}

	// 지급 대상을 확인한 뒤 획득을 확정하고, 제거 전에 연출을 전달합니다.
	bCollected = true;
	PlayerState->AddBonusMaxSpeed(10.0f);
	MulticastPickupEffects();
	Destroy();
}

void ACXSpeedBuffItem::MulticastPickupEffects_Implementation()
{
	UWorld* World = GetWorld();

	if (IsValid(PickupParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, PickupParticle, GetActorLocation(),
			FRotator::ZeroRotator, FVector(PickupParticleScale));
	}

	if (IsValid(PickupSound))
	{
		UGameplayStatics::PlaySoundAtLocation(World, PickupSound, GetActorLocation());
	}
}
