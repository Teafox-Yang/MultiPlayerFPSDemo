#include "MultiFPS/Public/FpsBaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AFpsBaseCharacter::AFpsBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

#pragma region Componet
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	if(PlayerCamera != nullptr)
	{
		PlayerCamera -> SetupAttachment(RootComponent);
		PlayerCamera -> bUsePawnControlRotation = true;
	}

	FPArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmMesh"));
	if(FPArmsMesh != nullptr)
	{
		FPArmsMesh -> SetupAttachment(PlayerCamera);
		FPArmsMesh -> SetOnlyOwnerSee(true);
	}

	UMeshComponent* ThisMesh = GetMesh();
	ThisMesh -> SetOwnerNoSee(true);
	ThisMesh -> SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ThisMesh -> SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
#pragma endregion
}

void AFpsBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	StartWithKindOfWeapon();
}

void AFpsBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void AFpsBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent -> BindAction(TEXT("QuietWalk"), IE_Pressed, this, &AFpsBaseCharacter::QuietWalkAction);
	InputComponent -> BindAction(TEXT("QuietWalk"), IE_Released, this, &AFpsBaseCharacter::NormalWalkAction);
	InputComponent -> BindAction(TEXT("Jump"), IE_Pressed, this, &AFpsBaseCharacter::JumpAction);
	InputComponent -> BindAction(TEXT("Jump"), IE_Released, this, &AFpsBaseCharacter::StopJumpAction);
	
	InputComponent -> BindAxis(TEXT("MoveRight"), this, &AFpsBaseCharacter::MoveRight);
	InputComponent -> BindAxis(TEXT("MoveForward"), this, &AFpsBaseCharacter::MoveForward);

	InputComponent -> BindAxis(TEXT("Yaw"), this, &AFpsBaseCharacter::AddControllerYawInput);
	InputComponent -> BindAxis(TEXT("Pitch"), this, &AFpsBaseCharacter::AddControllerPitchInput);
}

#pragma region Weapon
void AFpsBaseCharacter::EquipPrimary(AWeaponBaseServer* WeaponBaseServer)
{
	 if(ServerPrimaryWeapon)
	 {
	 	
	 }
	 else
	 {
	 	ServerPrimaryWeapon = WeaponBaseServer;
	 	ServerPrimaryWeapon -> SetOwner(this);
	 	ServerPrimaryWeapon -> K2_AttachToComponent(GetMesh(),TEXT("Weapon_Rifle"), EAttachmentRule::SnapToTarget,
	 		EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
	 		true);
		ClientEquipFPArmsPrimary();
	 	
	 }
}

void AFpsBaseCharacter::StartWithKindOfWeapon()
{
	if(HasAuthority())
	{
		PurchaseWeapon(EWeaponType::AK47);
	}
}

void AFpsBaseCharacter::PurchaseWeapon(EWeaponType WeaponType)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	switch(WeaponType)
	{
	case EWeaponType::AK47:
		{
			//动态获取AK47_Server类
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/AK47/ServerBP_AK47.ServerBP_AK47_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			EquipPrimary(ServerWeapon);
		}
		break;
	default:
		{
				
		}
	}
}
#pragma endregion 

#pragma region NetWorking
void AFpsBaseCharacter::ServerQuietWalkAction_Implementation()
{
	GetCharacterMovement() -> MaxWalkSpeed = 300;
}

bool AFpsBaseCharacter::ServerQuietWalkAction_Validate()
{
	return true;
}

void AFpsBaseCharacter::ServerNormalWalkAction_Implementation()
{
	GetCharacterMovement() -> MaxWalkSpeed = 600;
}

bool AFpsBaseCharacter::ServerNormalWalkAction_Validate()
{
	return true;
}

void AFpsBaseCharacter::ClientEquipFPArmsPrimary_Implementation()
{
	if(ServerPrimaryWeapon)
	{
		if(ClientPrimaryWeapon)
		{
			
		}
		else
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner = this;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ClientPrimaryWeapon = GetWorld() -> SpawnActor<AWeaponBaseClient>(ServerPrimaryWeapon->ClientWeaponBaseBPClass,
				GetActorTransform(),
				SpawnInfo);
		
			ClientPrimaryWeapon -> K2_AttachToComponent(FPArmsMesh, TEXT("WeaponSocket"), EAttachmentRule::SnapToTarget,
				 EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
				 true);
			//手臂动画
		}
	}
}
#pragma endregion 

#pragma region InputEvent
void AFpsBaseCharacter::MoveRight(float AxisValue)
{
	AddMovementInput(GetActorRightVector(), AxisValue, false);
}

void AFpsBaseCharacter::MoveForward(float AxisValue)
{
	AddMovementInput(GetActorForwardVector(), AxisValue, false);
}

void AFpsBaseCharacter::JumpAction()
{
	Jump();
}

void AFpsBaseCharacter::StopJumpAction()
{
	StopJumping();
}

void AFpsBaseCharacter::QuietWalkAction()
{
	GetCharacterMovement() -> MaxWalkSpeed = 300;
	ServerQuietWalkAction();
}

void AFpsBaseCharacter::NormalWalkAction()
{
	GetCharacterMovement() -> MaxWalkSpeed = 600;
	ServerNormalWalkAction();
}
#pragma endregion

