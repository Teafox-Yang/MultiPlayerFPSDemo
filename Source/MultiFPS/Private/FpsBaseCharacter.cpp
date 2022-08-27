#include "MultiFPS/Public/FpsBaseCharacter.h"

#include "Blueprint/UserWidget.h"
#include "Components/DecalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
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
	IsFiring = false;
	IsReloading = false;
	IsAiming = false;
	//绑定OnHit方法
	OnTakePointDamage.AddDynamic(this, &AFpsBaseCharacter::OnHit);
	if(BornWithWeapon)
	{
		StartWithKindOfWeapon();
	}
	ClientArmsAnimBP = FPArmsMesh -> GetAnimInstance();
	ServerBodiesAnimBP = GetMesh() -> GetAnimInstance();

	FpsPlayerController = Cast<AMultiFpsPlayerController>(GetController());
	if(FpsPlayerController)
	{
		FpsPlayerController -> CreatePlayerUI();
	}
}

void AFpsBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AFpsBaseCharacter, IsFiring, COND_None);
	DOREPLIFETIME_CONDITION(AFpsBaseCharacter, IsReloading, COND_None);
	DOREPLIFETIME_CONDITION(AFpsBaseCharacter, IsAiming, COND_None);
	DOREPLIFETIME_CONDITION(AFpsBaseCharacter, ActiveWeapon, COND_None);
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

	//鼠标右键开镜
	InputComponent -> BindAction(TEXT("Aiming"), IE_Pressed, this, &AFpsBaseCharacter::InputAimingPressed);
	InputComponent -> BindAction(TEXT("Aiming"), IE_Released, this, &AFpsBaseCharacter::InputAimingReleased);
	
	//按V键近战攻击
	InputComponent -> BindAction(TEXT("Attack"), IE_Pressed, this, &AFpsBaseCharacter::InputAttackPressed);
	InputComponent -> BindAction(TEXT("Attack"), IE_Released, this, &AFpsBaseCharacter::InputAttackReleased);

	//按R换弹
	InputComponent -> BindAction(TEXT("Reload"), IE_Pressed, this, &AFpsBaseCharacter::InputReloadPressed);
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

void AFpsBaseCharacter::EquipSecondary(AWeaponBaseServer* WeaponBaseServer)
{
	if(ServerSecondaryWeapon)
	{
	 	
	}
	else
	{
		ServerSecondaryWeapon = WeaponBaseServer;
		ServerSecondaryWeapon -> SetOwner(this);
		ServerSecondaryWeapon -> K2_AttachToComponent(GetMesh(),TEXT("Weapon_Rifle"), EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
			true);
		ClientEquipFPArmsSecondary();
	}
}

void AFpsBaseCharacter::StartWithKindOfWeapon()
{
	if(HasAuthority())
	{
		PurchaseWeapon(TestStartWeapon);
	}
}

void AFpsBaseCharacter::PurchaseWeapon(EWeaponType WeaponType)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	switch(WeaponType)
	{
	case EWeaponType::Ak47:
		{
			//动态获取AK47 Server类
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/AK47/ServerBP_AK47.ServerBP_AK47_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			ActiveWeapon = EWeaponType::Ak47;
			UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("EquipPrimary(ServerWeapon)")));
			EquipPrimary(ServerWeapon);
		}
		break;
	case EWeaponType::M4A1:
		{
			//动态获取M4A1 Server类
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/M4A1/ServerBP_M4A1.ServerBP_M4A1_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			ActiveWeapon = EWeaponType::M4A1;
			EquipPrimary(ServerWeapon);
		}
		break;
	case EWeaponType::MP7:
		{
			//动态获取MP7 Server类
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/MP7/ServerBP_MP7.ServerBP_MP7_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			ActiveWeapon = EWeaponType::MP7;
			EquipPrimary(ServerWeapon);
		}
		break;
	case EWeaponType::DesertEagle:
		{
			//动态获取DesertEagle Server类
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/DesertEagle/ServerBP_DesertEagle.ServerBP_DesertEagle_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			ActiveWeapon = EWeaponType::DesertEagle;
			EquipSecondary(ServerWeapon);
		}
		break;
	case EWeaponType::Sniper:
		{
			//动态获取Sniper Server类
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/Sniper/ServerBP_Sniper.ServerBP_Sniper_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			ActiveWeapon = EWeaponType::Sniper;
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
	case EWeaponType::Ak47:
		{
			return ClientPrimaryWeapon;
		}
	case EWeaponType::M4A1:
		{
			return ClientPrimaryWeapon;
		}
	case EWeaponType::MP7:
		{
			return ClientPrimaryWeapon;
		}
	case EWeaponType::DesertEagle:
		{
			return ClientSecondaryWeapon;
		}
	case EWeaponType::Sniper:
		{
			return ClientPrimaryWeapon;
		}
	default:break;
	}
	return nullptr;
}

AWeaponBaseServer* AFpsBaseCharacter::GetCurrentServerTPBodiesWeaponActor()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::Ak47:
		{
			return ServerPrimaryWeapon;
		}
	case EWeaponType::M4A1:
		{
			return ServerPrimaryWeapon;
		}
	case EWeaponType::MP7:
		{
			return ServerPrimaryWeapon;
		}
	case EWeaponType::DesertEagle:
		{
			return ServerSecondaryWeapon;
		}
	case EWeaponType::Sniper:
		{
			return ServerPrimaryWeapon;
		}
	default:break;
	}
	return nullptr;
}

#pragma endregion 

#pragma region Fire

void AFpsBaseCharacter::AutomaticFire()
{
	//判断子弹是否足够
	if(ServerPrimaryWeapon -> ClipCurrentAmmo > 0)
	{
		//服务端（枪口的闪光粒子效果(done)，播放射击声音(done)，减少弹药(done)，子弹的UI(done), 射线检测（三种步枪，手枪，狙击枪），伤害应用，弹孔生成等）
		if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
		{
			ServerFireRifleWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), true);
		}
		else
		{
			ServerFireRifleWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), false);
		}
		
		//客户端（枪体播放动画(done)，手臂播放动画（done)，播放射击声音（done)，应用屏幕抖动(done)，枪口闪光粒子效果(done)，后坐力）
		//客户端(十字线瞄准UI(done), 初始化UI(done), 播放准星扩散动画（done)）
		ClientFire();
		ClientRecoil();
	}
	else
	{
		StopFirePrimary();
	}
}

void AFpsBaseCharacter::ResetRecoil()
{
	RecoilXCoordPerShoot = 0;
	NewVerticalRecoilAmount = 0;
	OldVerticalRecoilAmount = 0;
	VerticalRecoilAmount = 0;

}

void AFpsBaseCharacter::DelayPlayArmReloadCallBack()
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	if(CurrentServerWeapon)
	{
		int32 GunCurrentAmmo = CurrentServerWeapon -> GunCurrentAmmo;
		int32 ClipCurrentAmmo = CurrentServerWeapon -> ClipCurrentAmmo;
		int32 MaxClipAmmo = CurrentServerWeapon -> MaxClipAmmo;

		//是否装填全部剩余子弹
		if(MaxClipAmmo - ClipCurrentAmmo >= GunCurrentAmmo)
		{
			ClipCurrentAmmo += GunCurrentAmmo;
			GunCurrentAmmo = 0;
		}
		else
		{
			GunCurrentAmmo -= MaxClipAmmo - ClipCurrentAmmo;
			ClipCurrentAmmo = MaxClipAmmo;
		}
		CurrentServerWeapon -> GunCurrentAmmo = GunCurrentAmmo;
		CurrentServerWeapon -> ClipCurrentAmmo = ClipCurrentAmmo;

		ClientUpdateAmmoUI(ClipCurrentAmmo, GunCurrentAmmo);
		IsReloading = false;
	}
}

void AFpsBaseCharacter::FireWeaponPrimary()
{
	//UE_LOG(LogTemp, Warning, TEXT("void AFpsBaseCharacter::FireWeaponPrimary()"));
	//判断子弹是否足够
	if(ServerPrimaryWeapon)
	{
		if(ServerPrimaryWeapon -> ClipCurrentAmmo > 0 && !IsReloading)
		{
			//服务端（枪口的闪光粒子效果(done)，播放射击声音(done)，减少弹药(done)，子弹的UI(done), 射线检测（三种步枪，手枪，狙击枪），伤害应用，弹孔生成等）
			if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
			{
				ServerFireRifleWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), true);
			}
			else
			{
				ServerFireRifleWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), false);
			}
		
			//客户端（枪体播放动画(done)，手臂播放动画（done)，播放射击声音（done)，应用屏幕抖动(done)，枪口闪光粒子效果(done)，后坐力）
			//客户端(十字线瞄准UI(done), 初始化UI(done), 播放准星扩散动画（done)）
			ClientFire();
			ClientRecoil();
			//开启计时器，每隔固定时间重新射击(射速)
			if(ServerPrimaryWeapon -> IsAutomatic)
			{
				GetWorldTimerManager().SetTimer(AutomaticFireTimerHandle, this, &AFpsBaseCharacter::AutomaticFire, ServerPrimaryWeapon -> AutomaticFireRate, true);
			}
			//连发枪的连击系统
			//UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("ServerPrimaryWeapon -> ClipCurrentAmmo : %d"),ServerPrimaryWeapon -> ClipCurrentAmmo));
		}
	}
}

void AFpsBaseCharacter::StopFirePrimary()
{
	//更改IsFire变量
	ServerStopFiring();
	//关闭计时器
	GetWorldTimerManager().ClearTimer(AutomaticFireTimerHandle);
	//重置后坐力相关变量
	ResetRecoil();
	
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
		//x,y,z加上一个随机的偏移
		FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon -> BulletDistance;
		float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon -> MovingFireRandomRange, ServerPrimaryWeapon -> MovingFireRandomRange);
		float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon -> MovingFireRandomRange, ServerPrimaryWeapon -> MovingFireRandomRange);
		float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon -> MovingFireRandomRange, ServerPrimaryWeapon -> MovingFireRandomRange);
		EndLocation = FVector(Vector.X + RandomX, Vector.Y + RandomY, Vector.Z + RandomZ);
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

void AFpsBaseCharacter::FireWeaponSniper()
{
	//UE_LOG(LogTemp, Warning, TEXT("void AFpsBaseCharacter::FireWeaponPrimary()"));
	//判断子弹是否足够
	if(ServerPrimaryWeapon)
	{
		if(ServerPrimaryWeapon -> ClipCurrentAmmo > 0 && !IsReloading && !IsFiring)
		{
			//服务端（枪口的闪光粒子效果(done)，播放射击声音(done)，减少弹药(done)，子弹的UI(done), 射线检测（三种步枪，手枪，狙击枪），伤害应用，弹孔生成等）
			if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
			{
				ServerFireSniperWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), true);
			}
			else
			{
				ServerFireSniperWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), false);
			}
		
			//客户端（枪体播放动画(done)，手臂播放动画（done)，播放射击声音（done)，应用屏幕抖动(done)，枪口闪光粒子效果(done)，后坐力）
			//客户端(十字线瞄准UI(done), 初始化UI(done), 播放准星扩散动画（done)）
			ClientFire();
		}
	}
}

void AFpsBaseCharacter::StopFireSniper()
{
}

void AFpsBaseCharacter::SniperLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	//人物是否运动会导致不同的endlocation计算
	//是否开镜导致不同的射线检测计算
	if(IsAiming)
	{
		if(IsMoving)
		{
			//x,y,z加上一个随机的偏移
			FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon -> BulletDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon -> MovingFireRandomRange, ServerPrimaryWeapon -> MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon -> MovingFireRandomRange, ServerPrimaryWeapon -> MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon -> MovingFireRandomRange, ServerPrimaryWeapon -> MovingFireRandomRange);
			EndLocation = FVector(Vector.X + RandomX, Vector.Y + RandomY, Vector.Z + RandomZ);
		}
		else
		{
			EndLocation = CameraLocation + CameraForwardVector * ServerPrimaryWeapon -> BulletDistance;
		}
		ClientStopAiming();
	}
	else
	{
		if(IsMoving)
		{
			//x,y,z加上一个随机的偏移
			FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon -> BulletDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon -> MovingFireRandomRange, ServerPrimaryWeapon -> MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon -> MovingFireRandomRange, ServerPrimaryWeapon -> MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon -> MovingFireRandomRange, ServerPrimaryWeapon -> MovingFireRandomRange);
			EndLocation = FVector(Vector.X + RandomX, Vector.Y + RandomY, Vector.Z + RandomZ);
		}
		else
		{
			EndLocation = CameraLocation + CameraForwardVector * ServerPrimaryWeapon -> BulletDistance;
		}
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

void AFpsBaseCharacter::DelaySniperShootCallBack()
{
	IsFiring = false;
}

void AFpsBaseCharacter::FireWeaponSecondary()
{
	//判断子弹是否足够
	if(ServerSecondaryWeapon)
	{
		if(ServerSecondaryWeapon -> ClipCurrentAmmo > 0 && !IsReloading)
		{
			//服务端（枪口的闪光粒子效果(done)，播放射击声音(done)，减少弹药(done)，子弹的UI(done), 射线检测（三种步枪，手枪，狙击枪），伤害应用，弹孔生成等）
			if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
			{
				ServerFirePistolWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), true);
			}
			else
			{
				ServerFirePistolWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), false);
			}
		
			//客户端（枪体播放动画(done)，手臂播放动画（done)，播放射击声音（done)，应用屏幕抖动(done)，枪口闪光粒子效果(done)，后坐力）
			//客户端(十字线瞄准UI(done), 初始化UI(done), 播放准星扩散动画（done)）
			ClientFire();
		}
	}
}

void AFpsBaseCharacter::StopFireSecondary()
{
	//更改IsFire变量
	ServerStopFiring();
}

void AFpsBaseCharacter::PistolLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector;
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	//人物是否运动会导致不同的endlocation计算
	if(IsMoving)
	{
		//旋转加上一个随机偏移，根据射击的快慢决定，连续射击得越快，子弹偏移越大。
		FRotator Rotator;
		Rotator.Roll = CameraRotation.Roll;
		Rotator.Pitch = CameraRotation.Pitch + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin, PistolSpreadMax);
		Rotator.Yaw = CameraRotation.Yaw + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin, PistolSpreadMax);
		CameraForwardVector = UKismetMathLibrary::GetForwardVector(Rotator);
		EndLocation = CameraLocation + CameraForwardVector * ServerSecondaryWeapon -> BulletDistance;
		//x,y,z加上一个随机的偏移
		FVector Vector = CameraLocation + CameraForwardVector * ServerSecondaryWeapon -> BulletDistance;
		float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon -> MovingFireRandomRange, ServerSecondaryWeapon -> MovingFireRandomRange);
		float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon -> MovingFireRandomRange, ServerSecondaryWeapon -> MovingFireRandomRange);
		float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon -> MovingFireRandomRange, ServerSecondaryWeapon -> MovingFireRandomRange);
		EndLocation = FVector(Vector.X + RandomX, Vector.Y + RandomY, Vector.Z + RandomZ);
	}
	else
	{
		//旋转加上一个随机偏移，根据射击的快慢决定，连续射击得越快，子弹偏移越大。
		FRotator Rotator;
		Rotator.Roll = CameraRotation.Roll;
		Rotator.Pitch = CameraRotation.Pitch + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin, PistolSpreadMax);
		Rotator.Yaw = CameraRotation.Yaw + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin, PistolSpreadMax);

		CameraForwardVector = UKismetMathLibrary::GetForwardVector(Rotator);
		EndLocation = CameraLocation + CameraForwardVector * ServerSecondaryWeapon -> BulletDistance;
	}
	bool LineHitSuccess = UKismetSystemLibrary::LineTraceSingle(GetWorld(), CameraLocation, EndLocation, ETraceTypeQuery::TraceTypeQuery1, false,
		IgnoreArray, EDrawDebugTrace::None, HitResult, true, FLinearColor::Red,
		FLinearColor::Green, 3.f);

	PistolSpreadMax += ServerSecondaryWeapon -> SpreadWeaponMaxIndex;
	PistolSpreadMin -= ServerSecondaryWeapon -> SpreadWeaponMinIndex;
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

void AFpsBaseCharacter::DelaySpreadWeaponShootCallBack()
{
	PistolSpreadMin = 0;
	PistolSpreadMax = 0;
}

void AFpsBaseCharacter::DamagePlayer(UPhysicalMaterial* PhysicalMaterial, AActor* DamagedActor, FVector& HitFromLocation, FHitResult& HitInfo)
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	//观察者
	if(CurrentServerWeapon)
	{
		//角色身体的五个位置应用不同的伤害
		switch (PhysicalMaterial -> SurfaceType)
		{
		case EPhysicalSurface::SurfaceType1 :
			{
				//Head
				UGameplayStatics::ApplyPointDamage(DamagedActor, CurrentServerWeapon -> BaseDamage * 4, HitFromLocation, HitInfo,
		GetController(), this, UDamageType::StaticClass());
			}
			break;
		case EPhysicalSurface::SurfaceType2 :
			{
				//Body
				UGameplayStatics::ApplyPointDamage(DamagedActor, CurrentServerWeapon -> BaseDamage * 1, HitFromLocation, HitInfo,
		GetController(), this, UDamageType::StaticClass());
			}
			break;
		case EPhysicalSurface::SurfaceType3 :
			{
				//Arm
				UGameplayStatics::ApplyPointDamage(DamagedActor, CurrentServerWeapon -> BaseDamage * 0.8, HitFromLocation, HitInfo,
		GetController(), this, UDamageType::StaticClass());
			}
			break;
		case EPhysicalSurface::SurfaceType4 :
			{
				//Leg
				UGameplayStatics::ApplyPointDamage(DamagedActor, CurrentServerWeapon -> BaseDamage * 0.7, HitFromLocation, HitInfo,
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
		IsFiring = true;
	}
	//UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("ServerPrimaryWeapon -> ClipCurrentAmmo : %d"),ServerPrimaryWeapon -> ClipCurrentAmmo));
}

bool AFpsBaseCharacter::ServerFireRifleWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	return true;
}

void AFpsBaseCharacter::ServerFireSniperWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,
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
		SniperLineTrace(CameraLocation, CameraRotation, IsMoving);
		IsFiring = true;
	}
	if(ClientPrimaryWeapon)
	{
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;
		ActionInfo.ExecutionFunction = TEXT("DelaySniperShootCallBack");
		ActionInfo.UUID = FMath::Rand();
		ActionInfo.Linkage = 0;
		UKismetSystemLibrary::Delay(this, ClientPrimaryWeapon -> ClientArmsFireAnimMontage -> GetPlayLength(), ActionInfo);
	}
}

bool AFpsBaseCharacter::ServerFireSniperWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	return true;
}

void AFpsBaseCharacter::ServerFirePistolWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,
                                                              bool IsMoving)
{
	FLatentActionInfo ActionInfo;
	ActionInfo.CallbackTarget = this;
	ActionInfo.ExecutionFunction = TEXT("DelaySpreadWeaponShootCallBack");
	ActionInfo.UUID = FMath::Rand();
	ActionInfo.Linkage = 0;
	UKismetSystemLibrary::Delay(this, ServerSecondaryWeapon -> SpreadWeaponCallBackRate, ActionInfo);
	if(ServerSecondaryWeapon)
	{
		//多播(必须再服务器调用，谁调谁多播）
		ServerSecondaryWeapon -> MultiShootingEffect();
		ServerSecondaryWeapon -> ClipCurrentAmmo -= 1;
		//多播（播放身体射击动画蒙太奇）
		MultiShooting();
		//客户端更新UI
		ClientUpdateAmmoUI(ServerSecondaryWeapon -> ClipCurrentAmmo, ServerSecondaryWeapon -> GunCurrentAmmo);
		//射线检测
		PistolLineTrace(CameraLocation, CameraRotation, IsMoving);
		IsFiring = true;
	}
}

bool AFpsBaseCharacter::ServerFirePistolWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
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


void AFpsBaseCharacter::ServerReloadPrimary_Implementation()
{
	//客户端手臂播放动画，服务器身体多播动画，子弹数据更新，UI更改
	if(ServerPrimaryWeapon)
	{
		//枪剩余子弹数大于0且弹夹不满
		if(ServerPrimaryWeapon -> GunCurrentAmmo > 0 && ServerPrimaryWeapon -> ClipCurrentAmmo < ServerPrimaryWeapon -> MaxClipAmmo)
		{
			ClientReload();
			MultiReloading();
			IsReloading = true;
			if(ClientPrimaryWeapon)
			{
				FLatentActionInfo ActionInfo;
				ActionInfo.CallbackTarget = this;
				ActionInfo.ExecutionFunction = TEXT("DelayPlayArmReloadCallBack");
				ActionInfo.UUID = FMath::Rand();
				ActionInfo.Linkage = 0;
				UKismetSystemLibrary::Delay(this, ClientPrimaryWeapon -> ClientArmsReloadAnimMontage -> GetPlayLength(), ActionInfo);
			}
		}
	}
}

bool AFpsBaseCharacter::ServerReloadPrimary_Validate()
{
	return true;
}

void AFpsBaseCharacter::ServerReloadSecondary_Implementation()
{
	if(ServerSecondaryWeapon)
	{
		//枪剩余子弹数大于0且弹夹不满
		if(ServerSecondaryWeapon -> GunCurrentAmmo > 0 && ServerSecondaryWeapon -> ClipCurrentAmmo < ServerSecondaryWeapon -> MaxClipAmmo)
		{
			ClientReload();
			MultiReloading();
			IsReloading = true;
			if(ClientSecondaryWeapon)
			{
				FLatentActionInfo ActionInfo;
				ActionInfo.CallbackTarget = this;
				ActionInfo.ExecutionFunction = TEXT("DelayPlayArmReloadCallBack");
				ActionInfo.UUID = FMath::Rand();
				ActionInfo.Linkage = 0;
				UKismetSystemLibrary::Delay(this, ClientSecondaryWeapon -> ClientArmsReloadAnimMontage -> GetPlayLength(), ActionInfo);
			}
		}
	}
}

bool AFpsBaseCharacter::ServerReloadSecondary_Validate()
{
	return true;
}

void AFpsBaseCharacter::ServerStopFiring_Implementation()
{
	IsFiring = false;
}

bool AFpsBaseCharacter::ServerStopFiring_Validate()
{
	return true;
}

void AFpsBaseCharacter::ServerSetAiming_Implementation(bool AimingState)
{
	IsAiming = AimingState;
}

bool AFpsBaseCharacter::ServerSetAiming_Validate(bool AimingState)
{
	return true;
}

void AFpsBaseCharacter::ServerSetActiveWeapon_Implementation(EWeaponType WeaponType)
{
	ActiveWeapon = WeaponType;
}

bool AFpsBaseCharacter::ServerSetActiveWeapon_Validate(EWeaponType WeaponType)
{
	return true;
}

void AFpsBaseCharacter::ServerEquipPrimary_Implementation(AWeaponBaseServer* ServerWeapon)
{
	if(ServerPrimaryWeapon)
	{
	 	
	}
	else
	{
		ServerPrimaryWeapon = ServerWeapon;
		ServerPrimaryWeapon -> SetOwner(this);
		ServerPrimaryWeapon -> K2_AttachToComponent(GetMesh(),TEXT("Weapon_Rifle"), EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
			true);
	}
	if(!ClientPrimaryWeapon)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = this;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ClientPrimaryWeapon = GetWorld() -> SpawnActor<AWeaponBaseClient>(ServerPrimaryWeapon->ClientWeaponBaseBPClass,
					GetActorTransform(),
					SpawnInfo);
	
		FName WeaponSocketName = TEXT("WeaponSocket");
		if(ActiveWeapon == EWeaponType::M4A1)
		{
			WeaponSocketName = TEXT("M4A1_Socket");
		}
		if(ActiveWeapon == EWeaponType::Sniper)
		{
			WeaponSocketName = TEXT("AWP_Socket");
		}
		ClientPrimaryWeapon -> K2_AttachToComponent(FPArmsMesh, WeaponSocketName, EAttachmentRule::SnapToTarget,
			 EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
			 true);
		//手臂动画
		if(ClientPrimaryWeapon)
		{
			UpdateFPArmsBlendPose(ClientPrimaryWeapon -> FPArmsBlendPoseIndex);
		}
	}
}

bool AFpsBaseCharacter::ServerEquipPrimary_Validate(AWeaponBaseServer* ServerWeapon)
{
	return true;
}

void AFpsBaseCharacter::ServerEquipSecondary_Implementation(AWeaponBaseServer* ServerWeapon)
{
	if(ServerSecondaryWeapon)
	{
	 	
	}
	else
	{
		ServerSecondaryWeapon = ServerWeapon;
		ServerSecondaryWeapon -> SetOwner(this);
		ServerSecondaryWeapon -> K2_AttachToComponent(GetMesh(),TEXT("Weapon_Rifle"), EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
			true);
	}
	if(!ClientSecondaryWeapon)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = this;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ClientSecondaryWeapon = GetWorld() -> SpawnActor<AWeaponBaseClient>(ServerWeapon->ClientWeaponBaseBPClass,
			GetActorTransform(),
			SpawnInfo);
		//不同武器的插槽
		FName WeaponSocketName = TEXT("WeaponSocket");
		ClientSecondaryWeapon -> K2_AttachToComponent(FPArmsMesh, WeaponSocketName, EAttachmentRule::SnapToTarget,
			 EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
			 true);
	}
	if(ClientSecondaryWeapon)
	{
		UpdateFPArmsBlendPose(ClientSecondaryWeapon -> FPArmsBlendPoseIndex);
	}
}

bool AFpsBaseCharacter::ServerEquipSecondary_Validate(AWeaponBaseServer* ServerWeapon)
{
	return true;
}

void AFpsBaseCharacter::MultiShooting_Implementation()
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	if(ServerBodiesAnimBP)
	{
		if(CurrentServerWeapon)
		{
			ServerBodiesAnimBP -> Montage_Play(CurrentServerWeapon -> ServerTPBodiesShootAnimMontage);
		}
	}
}

bool AFpsBaseCharacter::MultiShooting_Validate()
{
	return true;
}

void AFpsBaseCharacter::SetBodyBlendPose_Implementation(int NewIndex)
{
	UpdateTPBodiesBlendPose(NewIndex);
}

bool AFpsBaseCharacter::SetBodyBlendPose_Validate(int NewIndex)
{
	return true;
}

void AFpsBaseCharacter::MultiReloading_Implementation()
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	if(ServerBodiesAnimBP)
	{
		if(CurrentServerWeapon)
		{
			ServerBodiesAnimBP -> Montage_Play(CurrentServerWeapon -> ServerTPBodiesReloadAnimMontage);
		}
	}
}

bool AFpsBaseCharacter::MultiReloading_Validate()
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
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	if(CurrentServerWeapon)
	{
		UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CurrentServerWeapon -> BulletDecalMaterial, FVector(8, 8, 8),
			Location, Rotation, 10);
		if(Decal)
		{
			Decal -> SetFadeScreenSize(0.001);
		}
	}
}

bool AFpsBaseCharacter::MultiSpawnBulletDecal_Validate(FVector Location, FRotator Rotation)
{
	return true;
}

void AFpsBaseCharacter::ClientStopAiming_Implementation()
{
	if(FPArmsMesh)
	{
		FPArmsMesh -> SetHiddenInGame(false);
	}
	if(ClientPrimaryWeapon)
	{
		ClientPrimaryWeapon -> SetActorHiddenInGame(false);
		if(PlayerCamera)
		{
			PlayerCamera -> SetFieldOfView(90);
		}
	}
	if(WidgetScope)
	{
		WidgetScope -> RemoveFromParent();
	}
	FpsPlayerController -> ShowPlayerUI();
}

void AFpsBaseCharacter::ClientAiming_Implementation()
{
	if(FPArmsMesh)
	{
		FPArmsMesh -> SetHiddenInGame(true);
	}
	if(ClientPrimaryWeapon)
	{
		ClientPrimaryWeapon -> SetActorHiddenInGame(true);
		if(PlayerCamera)
		{
			PlayerCamera -> SetFieldOfView(ClientPrimaryWeapon -> FieldOfAimingView);
		}
	}
	WidgetScope = CreateWidget<UUserWidget>(GetWorld(), SniperScopeBPClass);
	WidgetScope -> AddToViewport();
	FpsPlayerController -> HidePlayerUI();
	
}

void AFpsBaseCharacter::ClientEquipFPArmsSecondary_Implementation()
{
	if(ServerSecondaryWeapon)
	{
		if(ClientSecondaryWeapon)
		{
			
		}
		else
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner = this;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ClientSecondaryWeapon = GetWorld() -> SpawnActor<AWeaponBaseClient>(ServerSecondaryWeapon->ClientWeaponBaseBPClass,
				GetActorTransform(),
				SpawnInfo);
			//不同武器的插槽
			FName WeaponSocketName = TEXT("WeaponSocket");
			ClientSecondaryWeapon -> K2_AttachToComponent(FPArmsMesh, WeaponSocketName, EAttachmentRule::SnapToTarget,
				 EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
				 true);

			ClientUpdateAmmoUI(ServerSecondaryWeapon -> ClipCurrentAmmo, ServerSecondaryWeapon -> GunCurrentAmmo);
			//手臂动画
			if(ClientSecondaryWeapon)
			{
				UpdateFPArmsBlendPose(ClientSecondaryWeapon -> FPArmsBlendPoseIndex);
			}
		}
	}
}

void AFpsBaseCharacter::ClientReload_Implementation()
{
	//手臂播放动画蒙太奇
	AWeaponBaseClient* CurrentClientWeapon = GetCurrentClientFPArmsWeaponActor();
	if(CurrentClientWeapon)
	{
		UAnimMontage* ClientArmsReloadMontage = CurrentClientWeapon -> ClientArmsReloadAnimMontage;
		ClientArmsAnimBP -> Montage_Play(ClientArmsReloadMontage);
		CurrentClientWeapon -> PlayReloadAnimation();
	}
}

void AFpsBaseCharacter::ClientRecoil_Implementation()
{
	UCurveFloat* VerticalRecoilCurve = nullptr;
	UCurveFloat* HorizontalRocoilCurve = nullptr;
	if(ServerPrimaryWeapon)
	{
		VerticalRecoilCurve = ServerPrimaryWeapon -> VerticalRecoilCurve;
		HorizontalRocoilCurve = ServerPrimaryWeapon -> HorizontalRecoilCurve;
	}
	RecoilXCoordPerShoot += 0.1;
	
	if(VerticalRecoilCurve)
	{
		NewVerticalRecoilAmount = VerticalRecoilCurve->GetFloatValue(RecoilXCoordPerShoot);
	}
	if(HorizontalRocoilCurve)
	{
		NewHorizontalRecoilAmount = HorizontalRocoilCurve->GetFloatValue(RecoilXCoordPerShoot);
	}
	
	VerticalRecoilAmount = NewVerticalRecoilAmount - OldVerticalRecoilAmount;
	HorizontalRecoilAmount = NewHorizontalRecoilAmount - OldHorizontalRecoilAmount;
	
	if(FpsPlayerController)
	{
		FRotator ControllerRotator = FpsPlayerController -> GetControlRotation();
		FpsPlayerController -> SetControlRotation(FRotator(ControllerRotator.Pitch + VerticalRecoilAmount,
			ControllerRotator.Yaw + HorizontalRecoilAmount,
			ControllerRotator.Roll));
	}
	
	OldVerticalRecoilAmount = NewVerticalRecoilAmount;
	OldHorizontalRecoilAmount = NewHorizontalRecoilAmount;
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
		//枪体播放动画w
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
			//不同武器的插槽
			FName WeaponSocketName = TEXT("WeaponSocket");
			if(ActiveWeapon == EWeaponType::M4A1)
			{
				WeaponSocketName = TEXT("M4A1_Socket");
			}
			if(ActiveWeapon == EWeaponType::Sniper)
			{
				WeaponSocketName = TEXT("AWP_Socket");
			}
			ClientPrimaryWeapon -> K2_AttachToComponent(FPArmsMesh, WeaponSocketName, EAttachmentRule::SnapToTarget,
				 EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
				 true);

			ClientUpdateAmmoUI(ServerPrimaryWeapon -> ClipCurrentAmmo, ServerPrimaryWeapon -> GunCurrentAmmo);
			//手臂动画
			if(ClientPrimaryWeapon)
			{
				UpdateFPArmsBlendPose(ClientPrimaryWeapon -> FPArmsBlendPoseIndex);
			}
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
	case EWeaponType::Ak47:
		{
			FireWeaponPrimary();
		}
		break;
	case EWeaponType::M4A1:
		{
			FireWeaponPrimary();
		}
		break;
	case EWeaponType::MP7:
		{
			FireWeaponPrimary();
		}
		break;
	case EWeaponType::DesertEagle:
		{
			FireWeaponSecondary();
		}
		break;
	case EWeaponType::Sniper:
		{
			FireWeaponSniper();
		}
		break;
		default:break;
	}
	
}

void AFpsBaseCharacter::InputFireReleased()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::Ak47:
		{
			StopFirePrimary();
		}
		break;
	case EWeaponType::M4A1:
		{
			StopFirePrimary();
		}
		break;
	case EWeaponType::MP7:
		{
			StopFirePrimary();
		}
		break;
	case EWeaponType::DesertEagle:
		{
			StopFireSecondary();
		}
		break;
	case EWeaponType::Sniper:
		{
			StopFireSniper();
		}
		break;
		default:break;
	}
}

void AFpsBaseCharacter::InputAimingPressed()
{
	//设置瞄准镜的UI，关闭枪体可见性，摄像机距离拉远
	//更改IsAiming属性，服务器RPC
	if(ActiveWeapon == EWeaponType::Sniper)
	{
		ServerSetAiming(true);
		ClientAiming();
	}
}

void AFpsBaseCharacter::InputAimingReleased()
{
	//取消瞄准镜的UI，开启枪体可见，摄像机距离恢复
	//更改IsAiming属性，服务器RPC
	if(ActiveWeapon == EWeaponType::Sniper)
	{
		ServerSetAiming(false);
		ClientStopAiming();
	}
}

void AFpsBaseCharacter::InputAttackPressed()
{
	//服务器播放
	ServerAttackAction();
}

void AFpsBaseCharacter::InputReloadPressed()
{
	if(!IsReloading)
	{
		if(!IsFiring)
		{
			switch(ActiveWeapon)
			{
			case EWeaponType::Ak47:
				{
					ServerReloadPrimary();
				}
				break;
			case EWeaponType::M4A1:
				{
					ServerReloadPrimary();
				}
				break;
			case EWeaponType::MP7:
				{
					ServerReloadPrimary();
				}
				break;
			case EWeaponType::DesertEagle:
				{
					ServerReloadSecondary();
				}
				break;
			case EWeaponType::Sniper:
				{
					ServerReloadPrimary();
				}
				break;
				default:break;
			}
			
		}
	}
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


