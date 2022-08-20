#include "MultiFPS/Public/FpsBaseCharacter.h"

#include "Components/DecalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

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

	PlayerThirdCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerThirdCamera"));
	if(PlayerThirdCamera != nullptr)
	{
		PlayerThirdCamera -> SetupAttachment(RootComponent);
		PlayerThirdCamera -> SetActive(false);
		PlayerThirdCamera -> bUsePawnControlRotation = true;
	}
	CurrentCamera = PlayerCamera;
	FPArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmMesh"));
	if(FPArmsMesh != nullptr)
	{
		FPArmsMesh -> SetupAttachment(PlayerCamera);
		FPArmsMesh -> SetOnlyOwnerSee(true);
		FPArmsMesh -> SetCastShadow(true);
	}

	UMeshComponent* ThisMesh = GetMesh();
	ThisMesh -> SetOwnerNoSee(true);
	ThisMesh -> SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ThisMesh -> SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	ThisMesh -> SetCastHiddenShadow(true);
#pragma endregion
}

#pragma region Engine
void AFpsBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = 100;
	//绑定OnHit方法
	OnTakePointDamage.AddDynamic(this, &AFpsBaseCharacter::OnHit);
	
	StartWithKindOfWeapon();

	ClientArmsAnimBP = FPArmsMesh -> GetAnimInstance();
	ServerBodiesAnimBP = GetMesh() -> GetAnimInstance();

	FpsPlayerController = Cast<AMultiFpsPlayerController>(GetController());
	if(FpsPlayerController)
	{
		FpsPlayerController -> CreatePlayerUI();
	}
}

void AFpsBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void AFpsBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	//按shift静步
	InputComponent -> BindAction(TEXT("QuietWalk"), IE_Pressed, this, &AFpsBaseCharacter::QuietWalkAction);
	InputComponent -> BindAction(TEXT("QuietWalk"), IE_Released, this, &AFpsBaseCharacter::NormalWalkAction);
	
	//按空格跳跃
	InputComponent -> BindAction(TEXT("Jump"), IE_Pressed, this, &AFpsBaseCharacter::JumpAction);
	InputComponent -> BindAction(TEXT("Jump"), IE_Released, this, &AFpsBaseCharacter::StopJumpAction);
	
	//鼠标左键开火
	InputComponent -> BindAction(TEXT("Fire"), IE_Pressed, this, &AFpsBaseCharacter::InputFirePressed);
	InputComponent -> BindAction(TEXT("Fire"), IE_Released, this, &AFpsBaseCharacter::InputFireReleased);

	InputComponent -> BindAction(TEXT("Attack"), IE_Pressed, this, &AFpsBaseCharacter::InputAttackPressed);
	InputComponent -> BindAction(TEXT("Attack"), IE_Released, this, &AFpsBaseCharacter::InputAttackReleased); 
	//按TAB切换视角
	InputComponent -> BindAction(TEXT("SwitchView"), IE_Pressed, this, &AFpsBaseCharacter::SwitchView);

	//控制移动和视角
	InputComponent -> BindAxis(TEXT("MoveRight"), this, &AFpsBaseCharacter::MoveRight);
	InputComponent -> BindAxis(TEXT("MoveForward"), this, &AFpsBaseCharacter::MoveForward);
	InputComponent -> BindAxis(TEXT("Yaw"), this, &AFpsBaseCharacter::AddControllerYawInput);
	InputComponent -> BindAxis(TEXT("Pitch"), this, &AFpsBaseCharacter::AddControllerPitchInput);
}
#pragma endregion 

#pragma region Weapon
//装备武器（生成）
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

AWeaponBaseClient* AFpsBaseCharacter::GetCurrentClientFPArmsWeaponActor()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::AK47:
		{
			return ClientPrimaryWeapon;
		}
		default:break;
	}
	return nullptr;
}

#pragma endregion 

#pragma region Fire

void AFpsBaseCharacter::FireWeaponPrimary()
{
	//UE_LOG(LogTemp, Warning, TEXT("void AFpsBaseCharacter::FireWeaponPrimary()"));
	//判断子弹是否足够
	if(ServerPrimaryWeapon -> ClipCurrentAmmo > 0)
	{
		//服务端（枪口的闪光粒子效果(done)，播放射击声音(done)，减少弹药(done)，子弹的UI(done), 射线检测（三种步枪，手枪，狙击枪），伤害应用，弹孔生成等）
		ServerFireRifleWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), false);
		
		//客户端（枪体播放动画(done)，手臂播放动画（done)，播放射击声音（done)，应用屏幕抖动(done)，枪口闪光粒子效果(done)，后坐力）
		//客户端(十字线瞄准UI(done), 初始化UI(done), 播放准星扩散动画（done)）
		ClientFire();
		//连发枪的连击系统
		//UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("ServerPrimaryWeapon -> ClipCurrentAmmo : %d"),ServerPrimaryWeapon -> ClipCurrentAmmo));

	}
}

void AFpsBaseCharacter::StopFirePrimary()
{
}

//需要考虑非常多事情的方法
void AFpsBaseCharacter::RifleLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	//人物是否运动会导致不同的endlocation计算
	if(IsMoving)
	{
		
	}
	else
	{
		EndLocation = CameraLocation + CameraForwardVector * ServerPrimaryWeapon -> BulletDistance;
	}
	bool LineHitSuccess = UKismetSystemLibrary::LineTraceSingle(GetWorld(), CameraLocation, EndLocation, ETraceTypeQuery::TraceTypeQuery1, false,
		IgnoreArray, EDrawDebugTrace::None, HitResult, true, FLinearColor::Red,
		FLinearColor::Green, 3.f);
	if(LineHitSuccess)
	{
		//UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("Hit Actor Name : %s"), *HitResult.Actor -> GetName()));
		AFpsBaseCharacter* FpsCharacter = Cast<AFpsBaseCharacter>(HitResult.Actor);
		if(FpsCharacter)
		{
			//打到玩家，应用伤害
			DamagePlayer(HitResult.PhysMaterial.Get(), HitResult.Actor.Get(), CameraLocation, HitResult);
		}
		else
		{
			//打到墙壁，生成弹孔（广播）
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);
			MultiSpawnBulletDecal(HitResult.Location, XRotator);
		}
	}
}

void AFpsBaseCharacter::DamagePlayer(UPhysicalMaterial* PhysicalMaterial, AActor* DamagedActor, FVector& HitFromLocation, FHitResult& HitInfo)
{
	//观察者
	if(ServerPrimaryWeapon)
	{
		//角色身体的五个位置应用不同的伤害
		switch (PhysicalMaterial -> SurfaceType)
		{
		case EPhysicalSurface::SurfaceType1 :
			{
				//Head
				UGameplayStatics::ApplyPointDamage(DamagedActor, ServerPrimaryWeapon -> BaseDamage * 4, HitFromLocation, HitInfo,
		GetController(), this, UDamageType::StaticClass());
			}
			break;
		case EPhysicalSurface::SurfaceType2 :
			{
				//Body
				UGameplayStatics::ApplyPointDamage(DamagedActor, ServerPrimaryWeapon -> BaseDamage * 1, HitFromLocation, HitInfo,
		GetController(), this, UDamageType::StaticClass());
			}
			break;
		case EPhysicalSurface::SurfaceType3 :
			{
				//Arm
				UGameplayStatics::ApplyPointDamage(DamagedActor, ServerPrimaryWeapon -> BaseDamage * 0.8, HitFromLocation, HitInfo,
		GetController(), this, UDamageType::StaticClass());
			}
			break;
		case EPhysicalSurface::SurfaceType4 :
			{
				//Leg
				UGameplayStatics::ApplyPointDamage(DamagedActor, ServerPrimaryWeapon -> BaseDamage * 0.7, HitFromLocation, HitInfo,
		GetController(), this, UDamageType::StaticClass());
			}
			break;
		default:break;
		}
	}
}

void AFpsBaseCharacter::OnHit(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType,
	AActor* DamageCauser)
{
	Health -= Damage;
	//UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("Health : %f"), Health));
	
	//1.客户端RPC 2.调用客户端PlayerController（持有UI）方法，留给蓝图实现。 3.实现PlayerUI里的血量减少的接口
	ClientUpdateHealthUI(Health);
	if(Health <= 0)
	{
		//死亡逻辑
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

void AFpsBaseCharacter::ServerFireRifleWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,
	bool IsMoving)
{
	if(ServerPrimaryWeapon)
	{
		//多播(必须再服务器调用，谁调谁多播）
		ServerPrimaryWeapon -> MultiShootingEffect();
		ServerPrimaryWeapon -> ClipCurrentAmmo -= 1;
		//多播（播放身体射击动画蒙太奇）
		MultiShooting();
		//客户端更新UI
		ClientUpdateAmmoUI(ServerPrimaryWeapon -> ClipCurrentAmmo, ServerPrimaryWeapon -> GunCurrentAmmo);
		//射线检测
		RifleLineTrace(CameraLocation, CameraRotation, IsMoving);
	}
	//UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("ServerPrimaryWeapon -> ClipCurrentAmmo : %d"),ServerPrimaryWeapon -> ClipCurrentAmmo));
}

bool AFpsBaseCharacter::ServerFireRifleWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	return true;
}

void AFpsBaseCharacter::ServerAttackAction_Implementation()
{
	if(ServerPrimaryWeapon)
	{
		MultiAttack();
	}
}

bool AFpsBaseCharacter::ServerAttackAction_Validate()
{
	return true;
}

void AFpsBaseCharacter::MultiShooting_Implementation()
{
	if(ServerBodiesAnimBP)
	{
		ServerBodiesAnimBP -> Montage_Play(ServerPrimaryWeapon -> ServerTPBodiesShootAnimMontage);
	}
}

bool AFpsBaseCharacter::MultiShooting_Validate()
{
	return true;
}

void AFpsBaseCharacter::MultiAttack_Implementation()
{
	if(ServerBodiesAnimBP)
	{
		if(!GetCurrentMontage())
		{
			if(this->GetVelocity().Size() > 0)
			{
				ServerBodiesAnimBP -> Montage_Play(ServerPrimaryWeapon -> ServerTPBodiesAttackUpperAnimMontage);
			}
			else
			{
				ServerBodiesAnimBP -> Montage_Play(ServerPrimaryWeapon -> ServerTPBodiesAttackAnimMontage);
			}
		}
	}
}

bool AFpsBaseCharacter::MultiAttack_Validate()
{
	return true;
}

void AFpsBaseCharacter::MultiSpawnBulletDecal_Implementation(FVector Location, FRotator Rotation)
{
	UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), ServerPrimaryWeapon -> BulletDecalMaterial, FVector(8, 8, 8),
		Location, Rotation, 10);
	if(Decal)
	{
		Decal -> SetFadeScreenSize(0.001);
	}
}

bool AFpsBaseCharacter::MultiSpawnBulletDecal_Validate(FVector Location, FRotator Rotation)
{
	return true;
}

void AFpsBaseCharacter::ClientUpdateHealthUI_Implementation(float NewHealth)
{
	if(FpsPlayerController)
	{
		FpsPlayerController -> UpdateHealthUI(NewHealth);
	}
}

void AFpsBaseCharacter::ClientUpdateAmmoUI_Implementation(int32 ClipCurrentAmmo, int32 GunCurrentAmmo)
{
	if(FpsPlayerController)
	{
		FpsPlayerController -> UpdateAmmoUI(ClipCurrentAmmo, GunCurrentAmmo);
	}
}

void AFpsBaseCharacter::ClientFire_Implementation()
{
	AWeaponBaseClient* CurrentClientWeapon = GetCurrentClientFPArmsWeaponActor();
	if(CurrentClientWeapon)
	{
		//枪体播放动画
		CurrentClientWeapon -> PlayShootAnimation();
		
		//手臂播放动画蒙太奇
		UAnimMontage* ClientArmsFireMontage = CurrentClientWeapon -> ClientArmsFireAnimMontage;
		ClientArmsAnimBP -> Montage_SetPlayRate(ClientArmsFireMontage, 1);
		ClientArmsAnimBP -> Montage_Play(ClientArmsFireMontage);

		//枪体播放射击音效
		CurrentClientWeapon -> DisplayWeaponEffects();

		//应用屏幕抖动
		FpsPlayerController -> PlayerCameraShake(CurrentClientWeapon -> CameraShakeClass);

		//播放准星扩散动画
		FpsPlayerController -> DoCrosshairRecoil();
	}
	
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

			ClientUpdateAmmoUI(ServerPrimaryWeapon -> ClipCurrentAmmo, ServerPrimaryWeapon -> GunCurrentAmmo);
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

void AFpsBaseCharacter::InputFirePressed()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::AK47:
		{
			FireWeaponPrimary();
		}
		default:break;
	}
	
}

void AFpsBaseCharacter::InputFireReleased()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::AK47:
		{
			StopFirePrimary();
		}
		default:break;
	}
}

void AFpsBaseCharacter::InputAttackPressed()
{
	//服务器播放
	ServerAttackAction();
}

void AFpsBaseCharacter::InputAttackReleased()
{
}

void AFpsBaseCharacter::SwitchView()
{
	if(PlayerCamera -> IsActive())
	{
		PlayerCamera -> SetActive(false);
		PlayerThirdCamera -> SetActive(true);
		
		GetMesh() -> SetOwnerNoSee(false);
		FPArmsMesh -> SetVisibility(false);
		
		CurrentCamera = PlayerThirdCamera;
		if(ClientPrimaryWeapon)
		{
			ClientPrimaryWeapon -> WeaponMesh -> SetOwnerNoSee(true);
		}
		if(ServerPrimaryWeapon)
		{
			ServerPrimaryWeapon -> WeaponMesh -> SetOwnerNoSee(false);
		}
	}
	else
	{
		PlayerThirdCamera -> SetActive(false);
		PlayerCamera -> SetActive(true);
		
		GetMesh() -> SetOwnerNoSee(true);
		FPArmsMesh -> SetVisibility(true);
		
		CurrentCamera = PlayerCamera;
		if(ClientPrimaryWeapon)
		{
			ClientPrimaryWeapon -> WeaponMesh -> SetOwnerNoSee(false);
		}
		if(ServerPrimaryWeapon)
		{
			ServerPrimaryWeapon -> WeaponMesh -> SetOwnerNoSee(true);
		}
	}
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


