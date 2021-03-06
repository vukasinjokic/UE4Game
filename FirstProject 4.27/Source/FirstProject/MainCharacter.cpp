// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"	
#include "Math/RotationMatrix.h"
#include "GameFramework/Controller.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Enemy.h"
#include "MainPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "FirstSaveGame.h"
#include "ItemStorage.h"
// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 3.0f;
	// camera boom should rotate when the pawn controller rotates
	CameraBoom->bUsePawnControlRotation = true;
	CameraFollow = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraFollow"));
	CameraFollow->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// since camera is attached to camera boom we dont need it to rotate with the controller
	CameraFollow->bUsePawnControlRotation = false;

	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	//dont rotate when controller rotates
	//let it just affect the camera
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationPitch = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 340.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 650.f;
	GetCharacterMovement()->AirControl = 0.7f;

	MaxHealth = 100.f;
	Health = 65.f;
	MaxStamina = 150.f;
	Stamina = 120.f;
	Coins = 0;

	RunningSpeed = 650.f;
	SprintingSpeed = 950.f;

	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	StaminaDrainRate = 25.f;
	MinSprintStamina = 50.f;

	InterpSpeed = 100.f;
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();
	MainPlayerController = Cast<AMainPlayerController>(GetController());
	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	if (MapName != "SunTemple")
	{
		LoadGameNoSwitch();
		if (MainPlayerController) MainPlayerController->GameModeOnly();
	}	
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Alive()) return;

	float DeltaStamina = StaminaDrainRate * DeltaTime;
	switch(StaminaStatus)
	{
	case EStaminaStatus::ESS_Normal:
		if(bShiftKeyDown)
		{
			if (bMovingForward || bMovingRight)
			{
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
				Stamina -= DeltaStamina;
				if (Stamina <= MinSprintStamina)
				{
					SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
				}
			}
			else
			{
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
		}
		else
		{
			SetMovementStatus(EMovementStatus::EMS_Normal);
			Stamina += DeltaStamina;
			if(Stamina >= MaxStamina)
			{
				Stamina = MaxStamina;
			}
		}
		break;

	case EStaminaStatus::ESS_BelowMinimum:
		if(bShiftKeyDown)
		{
			Stamina -= DeltaStamina;
			if(Stamina <= 0)
			{
				Stamina = 0;
				SetMovementStatus(EMovementStatus::EMS_Normal);
				SetStaminaStatus(EStaminaStatus::ESS_Exhuasted);
			}
			else
			{
				if (bMovingForward || bMovingRight) 
				{
					SetMovementStatus(EMovementStatus::EMS_Sprinting);
				}
				else 
				{
					SetMovementStatus(EMovementStatus::EMS_Normal);
				}
			}
		}
		else
		{
			SetMovementStatus(EMovementStatus::EMS_Normal);
			Stamina += DeltaStamina;
			if(Stamina >= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
			}
		}
		break;

	case EStaminaStatus::ESS_Exhuasted:
		SetMovementStatus(EMovementStatus::EMS_Normal);
		if(bShiftKeyDown)
		{
			Stamina = 0;
		}
		else
		{
			Stamina += DeltaStamina;
			SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
		}
		break;

	case EStaminaStatus::ESS_ExhaustedRecovering:
		SetMovementStatus(EMovementStatus::EMS_Normal);
		Stamina += DeltaStamina;
		if(Stamina >= MinSprintStamina)
		{
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
		}
		break;

	default: 
		;
	}

	if (bInterpToEnemy && CombatTarget)
	{
		FRotator LookAtTargetYaw = GetLookAtTargetYaw(CombatTarget->GetActorLocation());
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtTargetYaw, DeltaTime, InterpSpeed);
		SetActorRotation(InterpRotation);
	}

	if (CombatTarget && MainPlayerController)
	{
		MainPlayerController->EnemyLocation = CombatTarget->GetActorLocation();

	}

}

FRotator AMainCharacter::GetLookAtTargetYaw(FVector Target)
{
	FRotator LookAtTarget = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtTargetYaw(0.f, LookAtTarget.Yaw, 0.f);
	return LookAtTargetYaw;
}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMainCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMainCharacter::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMainCharacter::ShiftKeyUp);

	PlayerInputComponent->BindAction("ESC", IE_Pressed, this, &AMainCharacter::ESCDown).bExecuteWhenPaused = true;
	PlayerInputComponent->BindAction("ESC", IE_Released, this, &AMainCharacter::ESCUp).bExecuteWhenPaused = true;

	PlayerInputComponent->BindAxis("MoveForward", this, &AMainCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainCharacter::MoveRight);

	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMainCharacter::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMainCharacter::LMBUp);

	PlayerInputComponent->BindAxis("Turn", this, &AMainCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AMainCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("TurnRate", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &APawn::AddControllerPitchInput);

}


void AMainCharacter::MoveForward(float input){
	bMovingForward = false;
	if(Alive() && Controller != nullptr && input != 0.0f && !bAttacking){
		bMovingForward = true;
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator RotationYaw(0.f, Rotation.Yaw, 0.f);
		const FVector Direction = FRotationMatrix(RotationYaw).GetUnitAxis(EAxis::X);

		AddMovementInput(Direction, input);
	}
}

void AMainCharacter::MoveRight(float input){

	bMovingRight = false;
	if(Alive() && Controller != nullptr && input != 0.0f && !bAttacking){
		bMovingRight = true;
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator RotationYaw(0.f, Rotation.Yaw, 0.f);
		const FVector Direction = FRotationMatrix(RotationYaw).GetUnitAxis(EAxis::Y);

		AddMovementInput(Direction, input);
	}
}


void AMainCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMainCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMainCharacter::DecrementHealth(float Amount)
{
	Health -= Amount;
	if(Health <= 0) Die();
}

float AMainCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	DecrementHealth(DamageAmount);
	return DamageAmount;
}

void AMainCharacter::Die()
{
	if (MovementStatus == EMovementStatus::EMS_Dead) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CombatMontage)
	{
		AnimInstance->Montage_Play(CombatMontage);
		AnimInstance->Montage_JumpToSection(FName("Death"));
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMainCharacter::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;
}

void AMainCharacter::IncrementCoins()
{
	Coins++;
}

void AMainCharacter::IncrementHealth(float Amount)
{
	Health += Amount;
	if (Health > MaxHealth) Health = MaxHealth;
}

void AMainCharacter::SetMovementStatus(EMovementStatus NewStatus)
{
	MovementStatus = NewStatus;
	if(MovementStatus == EMovementStatus::EMS_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AMainCharacter::ShiftKeyDown()
{
	bShiftKeyDown = true;
}

void AMainCharacter::ShiftKeyUp()
{
	bShiftKeyDown = false;
}


void AMainCharacter::LMBDown()
{
	bLMBDown = true;
	if (!Alive()) return;
	AWeapon* Weapon = Cast<AWeapon>(OverlappingItem);
	if(Weapon)
	{
		Weapon->Equip(this);
		SetOverlappingItem(nullptr);
	}
	else if (EquippedWeapon) {
		Attack();
	}
}

void AMainCharacter::LMBUp()
{
	bLMBDown = false;
}

void AMainCharacter::ESCDown()
{
	bESCDown = true;
	if (MainPlayerController)
	{
		MainPlayerController->TogglePauseMenu();
	}
}

void AMainCharacter::ESCUp()
{
	bESCDown = false;
}

void AMainCharacter::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	if(EquippedWeapon) EquippedWeapon->Destroy();
	EquippedWeapon = WeaponToSet;
	
}

void AMainCharacter::Attack()
{
	if (!bAttacking && Alive())
	{
		bAttacking = true;
		bInterpToEnemy = true;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage)
		{
			int32 Section = FMath::RandRange(0, 1);
			switch (Section) {
			case 0:
				AnimInstance->Montage_Play(CombatMontage, 1.8f);
				AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
				break;
			case 1:
				AnimInstance->Montage_Play(CombatMontage, 2.0f);
				AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
				break;
			default:
				;
			}
		}
		
	}
	
}

void AMainCharacter::EndAttack()
{
	bAttacking = false;
	bInterpToEnemy = false;
	if (bLMBDown)
	{
		Attack();
	}
}

void AMainCharacter::PlaySwingSound()
{
	if (EquippedWeapon)
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);
	}
}


bool AMainCharacter::Alive()
{
	return MovementStatus != EMovementStatus::EMS_Dead;
}

void AMainCharacter::Jump()
{
	if (Alive())
	{
		Super::Jump();
	}
}

void AMainCharacter::UpdateCombatTarget()
{
	TSet<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, EnemyFilter);
	
	float MinDistance = 1000.f;
	AEnemy* ClosestEnemy = nullptr;
	if (OverlappingActors.Num() == 0)
	{
		if (MainPlayerController)
		{
			MainPlayerController->HideEnemyHealthBar();
		}
		return;
	}
	for (AActor* Actor : OverlappingActors)
	{
		AEnemy* Enemy = Cast<AEnemy>(Actor);
		if (Enemy)
		{
			float Distance = (Enemy->GetActorLocation() - GetActorLocation()).Size();
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestEnemy = Enemy;
			}
		}
	}

	SetCombatTarget(ClosestEnemy);
	if (MainPlayerController)
	{
		MainPlayerController->DisplayEnemyHealthBar();
	}
}

void AMainCharacter::SwitchLevel(FName LevelName)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FString CurrentLevel = World->GetMapName();
		if (LevelName != *CurrentLevel)
		{
			UGameplayStatics::OpenLevel(World, LevelName);
		}
	}
}

void AMainCharacter::SaveGame()
{
	UFirstSaveGame* SaveGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	SaveGameInstance->CharacterStats.Coins = Coins;
	SaveGameInstance->CharacterStats.Health = Health;
	SaveGameInstance->CharacterStats.Location = GetActorLocation();
	SaveGameInstance->CharacterStats.MaxHealth = MaxHealth;
	SaveGameInstance->CharacterStats.MaxStamina = MaxStamina;
	SaveGameInstance->CharacterStats.Rotation = GetActorRotation();
	SaveGameInstance->CharacterStats.Stamina = Stamina;

	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	SaveGameInstance->CharacterStats.MapName = MapName;
	if (EquippedWeapon)
	{
		SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->Name;
	}

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->PlayerName, SaveGameInstance->UserIndex);
}

void AMainCharacter::LoadGame(bool SetPosition)
{
	UFirstSaveGame* LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));
	LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

	Coins = LoadGameInstance->CharacterStats.Coins;
	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Stamina = LoadGameInstance->CharacterStats.Stamina;

	

	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;
			if (Weapons->WeaponMap.Contains(WeaponName))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				WeaponToEquip->Equip(this);
			}
			
		}
	}

	if (SetPosition)
	{
		SetActorLocation(LoadGameInstance->CharacterStats.Location);
		SetActorRotation(LoadGameInstance->CharacterStats.Rotation);
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (LoadGameInstance->CharacterStats.MapName != TEXT(""))
	{
		FName MapName = *LoadGameInstance->CharacterStats.MapName;
		SwitchLevel(MapName);
	}

	if (MainPlayerController) MainPlayerController->GameModeOnly();
}

void AMainCharacter::LoadGameNoSwitch()
{
	UFirstSaveGame* LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));
	LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

	Coins = LoadGameInstance->CharacterStats.Coins;
	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Stamina = LoadGameInstance->CharacterStats.Stamina;

	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;
			if (Weapons->WeaponMap.Contains(WeaponName))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				WeaponToEquip->Equip(this);
			}

		}
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}