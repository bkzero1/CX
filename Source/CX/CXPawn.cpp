// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "Components/ArrowComponent.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "Particles/ParticleSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "PlayerState/CXPlayerState.h"
#include "ChaosWheeledVehicleMovementComponent.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	// 차체를 따라가는 공격 기준점. 화살표는 에디터에서만 보입니다.
	AttackOrigin = CreateDefaultSubobject<UArrowComponent>(TEXT("AttackOrigin"));
	AttackOrigin->SetupAttachment(GetMesh());
	AttackOrigin->SetRelativeLocation(FVector(250.0f, 0.0f, 50.0f));
	AttackOrigin->SetRelativeRotation(FRotator::ZeroRotator);
	AttackOrigin->SetHiddenInGame(true);
	AttackOrigin->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 모든 차량이 같은 공격 액션을 사용하도록 코드에서 연결합니다.
	static ConstructorHelpers::FObjectFinder<UInputAction> AttackActionAsset(
		TEXT("/Game/VehicleTemplate/Input/Actions/IA_Attack.IA_Attack"));
	if (AttackActionAsset.Succeeded())
	{
		AttackAction = AttackActionAsset.Object;
	}

	// 기본 명중 효과를 제공하며, 차량 BP에서 다른 에셋으로 변경할 수 있습니다.
	static ConstructorHelpers::FObjectFinder<UParticleSystem> HitParticleAsset(
		TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
	if (HitParticleAsset.Succeeded())
	{
		HitParticle = HitParticleAsset.Object;
	}

	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);

		if (IsValid(AttackAction))
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACXPawn::InputAttack);
		}
		else
		{
			UE_LOG(LogTemplateVehicle, Warning, TEXT("AttackAction is not assigned: %s"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

float ACXPawn::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 이 함수의 this는 맞은 차량입니다. 코인은 서버에서만 변경합니다.
	ACXPawn* Attacker = Cast<ACXPawn>(DamageCauser);
	if (HasAuthority() == false || IsValid(Attacker) == false || Attacker == this)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	// 양쪽 정보를 먼저 확인하여, 받을 사람이 없는 상태에서 차감하지 않습니다.
	ACXPlayerState* VictimPlayerState = GetPlayerState<ACXPlayerState>();
	ACXPlayerState* AttackerPlayerState = Attacker->GetPlayerState<ACXPlayerState>();
	if (IsValid(VictimPlayerState) == false || IsValid(AttackerPlayerState) == false)
	{
		return 0.0f;
	}

	// 라이브 세션처럼 실제 데미지를 코인 수로 사용합니다. 공격에서는 5.0f를 전달합니다.
	const int32 TransferCoinAmount = static_cast<int32>(ActualDamage);
	if (VictimPlayerState->TryRemoveCoins(TransferCoinAmount) == false)
	{
		UE_LOG(LogTemplateVehicle, Log, TEXT("[CoinTransfer][Server] Not enough coins: %s (Coins=%d, Required=%d)"),
			*GetNameSafe(VictimPlayerState), VictimPlayerState->GetCoins(), TransferCoinAmount);
		return 0.0f;
	}

	// 차감에 성공했을 때만 같은 수량을 공격자에게 지급합니다.
	AttackerPlayerState->AddCoins(TransferCoinAmount);
	UE_LOG(LogTemplateVehicle, Log, TEXT("[CoinTransfer][Server] %s -> %s : %d (Victim=%d, Attacker=%d)"),
		*GetNameSafe(VictimPlayerState), *GetNameSafe(AttackerPlayerState), TransferCoinAmount,
		VictimPlayerState->GetCoins(), AttackerPlayerState->GetCoins());

	return ActualDamage;
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	// get the input magnitude for steering
	float SteeringValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	// get the input magnitude for the throttle
	float ThrottleValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	// get the input magnitude for the brakes
	float BreakValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input 
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;
	
	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle"));
}

void ACXPawn::InputAttack(const FInputActionValue& Value)
{
	CTSAttack();
}

void ACXPawn::CTSAttack_Implementation()
{
	// 서버에 있는 기준점의 위치와 앞 방향으로 공격을 판정합니다.
	const FVector Start = AttackOrigin->GetComponentLocation();
	const FVector End = Start + AttackOrigin->GetForwardVector() * 700.0f;

	// 자기 차량을 맞히지 않도록 검사 대상에서 제외합니다.
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// 벽 명중과 차량 명중을 구분합니다. 코인 이전 성공 여부와는 별개입니다.
	bool bHitVehicle = false;
	FHitResult HitResult;
	const bool bBlockingHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);
	if (bBlockingHit)
	{
		// 명중 로그는 벽에도 남기되, 데미지는 차량에만 전달합니다.
		UE_LOG(LogTemplateVehicle, Log, TEXT("[AttackHit][Server] %s -> %s"),
			*GetName(), *GetNameSafe(HitResult.GetActor()));

		if (ACXPawn* Victim = Cast<ACXPawn>(HitResult.GetActor()))
		{
			bHitVehicle = true;
			// 피해 차량, 코인 5개에 해당하는 데미지, 공격자 컨트롤러, 공격 차량.
			UGameplayStatics::ApplyDamage(Victim, 5.0f, GetController(), this, nullptr);
		}
	}

	// 빗나갔을 때도 공격 경로를 확인할 수 있도록 명중 조건 밖에서 호출합니다.
	MulticastHitEffect(Start, End, bBlockingHit, bHitVehicle, HitResult.ImpactPoint, HitResult.ImpactNormal);
}

void ACXPawn::MulticastHitEffect_Implementation(const FVector& TraceStart, const FVector& TraceEnd, bool bBlockingHit, bool bHitVehicle, const FVector& ImpactPoint, const FVector& ImpactNormal)
{
	// 서버는 표시하지 않습니다. 각 클라이언트는 서버가 보낸 판정 결과로만 그립니다.
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	// 명중하면 충돌점까지, 빗나가면 사거리 끝까지 표시합니다. 실제 판정은 여전히 직선입니다.
	const FVector VisualEnd = bBlockingHit ? ImpactPoint : TraceEnd;
	constexpr int32 SegmentCount = 20;
	constexpr float DisplayTime = 1.0f;
	FVector PreviousPoint = TraceStart;
	PreviousPoint.Z += AttackArcHeight;

	// 시작점은 높게, 끝점은 실제 충돌점에 둡니다. 끝으로 갈수록 높이 변화가 완만해집니다.
	for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / SegmentCount;
		FVector CurrentPoint = FMath::Lerp(TraceStart, VisualEnd, Alpha);
		CurrentPoint.Z += AttackArcHeight * (1.0f - Alpha) * (1.0f - Alpha);

		DrawDebugLine(GetWorld(), PreviousPoint, CurrentPoint, FColor::Red, false, DisplayTime, 0, 3.0f);
		PreviousPoint = CurrentPoint;
	}

	// 벽과 차량 모두 실제 충돌점에 반지름 8cm의 구를 표시합니다. 빗나가면 생략합니다.
	if (bBlockingHit)
	{
		DrawDebugSphere(GetWorld(), ImpactPoint, 8.0f, 12, FColor::Yellow, false, DisplayTime, 0, 2.0f);
	}

	// 선은 항상 표시하지만 파티클은 차량을 맞혔을 때만 재생합니다.
	if (bHitVehicle == false)
	{
		return;
	}

	// 충돌 위치에 효과를 만들고, 충돌 표면의 수직 방향으로 회전시킵니다.
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticle, ImpactPoint, ImpactNormal.Rotation(), FVector(2.0f));
}

#undef LOCTEXT_NAMESPACE