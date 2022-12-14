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

void AFpsBaseCharacter::DelayBeginPlayCallBack()
{
	FpsPlayerController = Cast<AMultiFpsPlayerController>(GetController());
	if(FpsPlayerController)
	{
		FpsPlayerController -> CreatePlayerUI();
	}
	else
	{
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;
		ActionInfo.ExecutionFunction = TEXT("DelayBeginPlayCallBack");
		ActionInfo.UUID = FMath::Rand();
		ActionInfo.Linkage = 0;
		UKismetSystemLibrary::Delay(this, 0.5, ActionInfo);
	}
}

#pragma region Engine
void AFpsBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = 100;
	IsFiring = false;
	IsReloading = false;
	IsAiming = false;
	//??????OnHit??????
	OnTakePointDamage.AddDynamic(this, &AFpsBaseCharacter::OnHit);
	ClientArmsAnimBP = FPArmsMesh -> GetAnimInstance();
	ServerBodiesAnimBP = GetMesh() -> GetAnimInstance();

	FpsPlayerController = Cast<AMultiFpsPlayerController>(GetController());
	if(FpsPlayerController)
	{
		FpsPlayerController -> CreatePlayerUI();
	}
	else
	{
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;
		ActionInfo.ExecutionFunction = TEXT("DelayBeginPlayCallBack");
		ActionInfo.UUID = FMath::Rand();
		ActionInfo.Linkage = 0;
		UKismetSystemLibrary::Delay(this, 0.5, ActionInfo);
	}
	if(BornWithWeapon)
	{
		StartWithKindOfWeapon();
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
	
	//???shift??????
	InputComponent -> BindAction(TEXT("QuietWalk"), IE_Pressed, this, &AFpsBaseCharacter::QuietWalkAction);
	InputComponent -> BindAction(TEXT("QuietWalk"), IE_Released, this, &AFpsBaseCharacter::NormalWalkAction);
	
	//???????????????
	InputComponent -> BindAction(TEXT("Jump"), IE_Pressed, this, &AFpsBaseCharacter::JumpAction);
	InputComponent -> BindAction(TEXT("Jump"), IE_Released, this, &AFpsBaseCharacter::StopJumpAction);
	
	//??????????????????
	InputComponent -> BindAction(TEXT("Fire"), IE_Pressed, this, &AFpsBaseCharacter::InputFirePressed);
	InputComponent -> BindAction(TEXT("Fire"), IE_Released, this, &AFpsBaseCharacter::InputFireReleased);

	//??????????????????
	InputComponent -> BindAction(TEXT("Aiming"), IE_Pressed, this, &AFpsBaseCharacter::InputAimingPressed);
	InputComponent -> BindAction(TEXT("Aiming"), IE_Released, this, &AFpsBaseCharacter::InputAimingReleased);
	
	//???V???????????????
	InputComponent -> BindAction(TEXT("Attack"), IE_Pressed, this, &AFpsBaseCharacter::InputAttackPressed);
	InputComponent -> BindAction(TEXT("Attack"), IE_Released, this, &AFpsBaseCharacter::InputAttackReleased);

	//???R??????
	InputComponent -> BindAction(TEXT("Reload"), IE_Pressed, this, &AFpsBaseCharacter::InputReloadPressed);
	//???TAB????????????
	//InputComponent -> BindAction(TEXT("SwitchView"), IE_Pressed, this, &AFpsBaseCharacter::SwitchView);

	//?????????????????????
	InputComponent -> BindAxis(TEXT("MoveRight"), this, &AFpsBaseCharacter::MoveRight);
	InputComponent -> BindAxis(TEXT("MoveForward"), this, &AFpsBaseCharacter::MoveForward);
	InputComponent -> BindAxis(TEXT("Yaw"), this, &AFpsBaseCharacter::AddControllerYawInput);
	InputComponent -> BindAxis(TEXT("Pitch"), this, &AFpsBaseCharacter::AddControllerPitchInput);
}
#pragma endregion 

#pragma region Weapon
//????????????????????????
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
	 	ActiveWeapon = ServerPrimaryWeapon -> KindOfWeapon;
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
		ActiveWeapon = ServerSecondaryWeapon -> KindOfWeapon;
		ClientEquipFPArmsSecondary();
	}
}

void AFpsBaseCharacter::StartWithKindOfWeapon()
{
	if(HasAuthority())
	{
		PurchaseWeapon(static_cast<EWeaponType>(UKismetMathLibrary::RandomIntegerInRange(0,static_cast<int8>(EWeaponType::EEND)-1)));
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
			//????????????AK47 Server???
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/AK47/ServerBP_AK47.ServerBP_AK47_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("EquipPrimary(ServerWeapon)")));
			EquipPrimary(ServerWeapon);
		}
		break;
	case EWeaponType::M4A1:
		{
			//????????????M4A1 Server???
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/M4A1/ServerBP_M4A1.ServerBP_M4A1_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			EquipPrimary(ServerWeapon);
		}
		break;
	case EWeaponType::MP7:
		{
			//????????????MP7 Server???
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/MP7/ServerBP_MP7.ServerBP_MP7_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			EquipPrimary(ServerWeapon);
		}
		break;
	case EWeaponType::DesertEagle:
		{
			//????????????DesertEagle Server???
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/DesertEagle/ServerBP_DesertEagle.ServerBP_DesertEagle_C'"));
			AWeaponBaseServer* ServerWeapon = GetWorld() -> SpawnActor<AWeaponBaseServer>(BlueprintVar,
				GetActorTransform(),
				SpawnInfo);
			ServerWeapon -> EquipWeapon();
			EquipSecondary(ServerWeapon);
		}
		break;
	case EWeaponType::Sniper:
		{
			//????????????Sniper Server???
			UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(), nullptr, TEXT("Blueprint'/Game/BluePrint/Weapon/Sniper/ServerBP_Sniper.ServerBP_Sniper_C'"));
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
	//????????????????????????
	if(ServerPrimaryWeapon -> ClipCurrentAmmo > 0)
	{
		//???????????????????????????????????????(done)?????????????????????(done)???????????????(done)????????????UI(done), ???????????????????????????????????????????????????????????????????????????????????????
		if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
		{
			ServerFireRifleWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), true);
		}
		else
		{
			ServerFireRifleWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), false);
		}
		
		//??????????????????????????????(done)????????????????????????done)????????????????????????done)?????????????????????(done)???????????????????????????(done)???????????????
		//?????????(???????????????UI(done), ?????????UI(done), ???????????????????????????done)???
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

		//??????????????????????????????
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
	//????????????????????????
	if(ServerPrimaryWeapon)
	{
		if(ServerPrimaryWeapon -> ClipCurrentAmmo > 0 && !IsReloading)
		{
			//???????????????????????????????????????(done)?????????????????????(done)???????????????(done)????????????UI(done), ???????????????????????????????????????????????????????????????????????????????????????
			if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
			{
				ServerFireRifleWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), true);
			}
			else
			{
				ServerFireRifleWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), false);
			}
		
			//??????????????????????????????(done)????????????????????????done)????????????????????????done)?????????????????????(done)???????????????????????????(done)???????????????
			//?????????(???????????????UI(done), ?????????UI(done), ???????????????????????????done)???
			ClientFire();
			ClientRecoil();
			//????????????????????????????????????????????????(??????)
			if(ServerPrimaryWeapon -> IsAutomatic)
			{
				GetWorldTimerManager().SetTimer(AutomaticFireTimerHandle, this, &AFpsBaseCharacter::AutomaticFire, ServerPrimaryWeapon -> AutomaticFireRate, true);
			}
			//????????????????????????
			//UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("ServerPrimaryWeapon -> ClipCurrentAmmo : %d"),ServerPrimaryWeapon -> ClipCurrentAmmo));
		}
	}
}

void AFpsBaseCharacter::StopFirePrimary()
{
	//??????IsFire??????
	ServerStopFiring();
	//???????????????
	GetWorldTimerManager().ClearTimer(AutomaticFireTimerHandle);
	//???????????????????????????
	ResetRecoil();
	
}

//????????????????????????????????????
void AFpsBaseCharacter::RifleLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	//????????????????????????????????????endlocation??????
	if(IsMoving)
	{
		//x,y,z???????????????????????????
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
			//???????????????????????????
			DamagePlayer(HitResult.PhysMaterial.Get(), HitResult.Actor.Get(), CameraLocation, HitResult);
		}
		else
		{
			//???????????????????????????????????????
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);
			MultiSpawnBulletDecal(HitResult.Location, XRotator);
		}
	}
}

void AFpsBaseCharacter::FireWeaponSniper()
{
	//UE_LOG(LogTemp, Warning, TEXT("void AFpsBaseCharacter::FireWeaponPrimary()"));
	//????????????????????????
	if(ServerPrimaryWeapon)
	{
		if(ServerPrimaryWeapon -> ClipCurrentAmmo > 0 && !IsReloading && !IsFiring)
		{
			//???????????????????????????????????????(done)?????????????????????(done)???????????????(done)????????????UI(done), ???????????????????????????????????????????????????????????????????????????????????????
			if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
			{
				ServerFireSniperWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), true);
			}
			else
			{
				ServerFireSniperWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), false);
			}
		
			//??????????????????????????????(done)????????????????????????done)????????????????????????done)?????????????????????(done)???????????????????????????(done)???????????????
			//?????????(???????????????UI(done), ?????????UI(done), ???????????????????????????done)???
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
	//????????????????????????????????????endlocation??????
	//?????????????????????????????????????????????
	if(IsAiming)
	{
		if(IsMoving)
		{
			//x,y,z???????????????????????????
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
			//x,y,z???????????????????????????
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
			//???????????????????????????
			DamagePlayer(HitResult.PhysMaterial.Get(), HitResult.Actor.Get(), CameraLocation, HitResult);
		}
		else
		{
			//???????????????????????????????????????
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
	//????????????????????????
	if(ServerSecondaryWeapon)
	{
		if(ServerSecondaryWeapon -> ClipCurrentAmmo > 0 && !IsReloading)
		{
			//???????????????????????????????????????(done)?????????????????????(done)???????????????(done)????????????UI(done), ???????????????????????????????????????????????????????????????????????????????????????
			if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
			{
				ServerFirePistolWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), true);
			}
			else
			{
				ServerFirePistolWeapon(CurrentCamera -> GetComponentLocation(), CurrentCamera -> GetComponentRotation(), false);
			}
		
			//??????????????????????????????(done)????????????????????????done)????????????????????????done)?????????????????????(done)???????????????????????????(done)???????????????
			//?????????(???????????????UI(done), ?????????UI(done), ???????????????????????????done)???
			ClientFire();
		}
	}
}

void AFpsBaseCharacter::StopFireSecondary()
{
	//??????IsFire??????
	ServerStopFiring();
}

void AFpsBaseCharacter::PistolLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector;
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	//????????????????????????????????????endlocation??????
	if(IsMoving)
	{
		//????????????????????????????????????????????????????????????????????????????????????????????????????????????
		FRotator Rotator;
		Rotator.Roll = CameraRotation.Roll;
		Rotator.Pitch = CameraRotation.Pitch + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin, PistolSpreadMax);
		Rotator.Yaw = CameraRotation.Yaw + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin, PistolSpreadMax);
		CameraForwardVector = UKismetMathLibrary::GetForwardVector(Rotator);
		EndLocation = CameraLocation + CameraForwardVector * ServerSecondaryWeapon -> BulletDistance;
		//x,y,z???????????????????????????
		FVector Vector = CameraLocation + CameraForwardVector * ServerSecondaryWeapon -> BulletDistance;
		float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon -> MovingFireRandomRange, ServerSecondaryWeapon -> MovingFireRandomRange);
		float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon -> MovingFireRandomRange, ServerSecondaryWeapon -> MovingFireRandomRange);
		float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon -> MovingFireRandomRange, ServerSecondaryWeapon -> MovingFireRandomRange);
		EndLocation = FVector(Vector.X + RandomX, Vector.Y + RandomY, Vector.Z + RandomZ);
	}
	else
	{
		//????????????????????????????????????????????????????????????????????????????????????????????????????????????
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
			//???????????????????????????
			DamagePlayer(HitResult.PhysMaterial.Get(), HitResult.Actor.Get(), CameraLocation, HitResult);
		}
		else
		{
			//???????????????????????????????????????
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
	//?????????
	if(CurrentServerWeapon)
	{
		//????????????????????????????????????????????????
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
	
	//1.?????????RPC 2.???????????????PlayerController?????????UI????????????????????????????????? 3.??????PlayerUI???????????????????????????
	ClientUpdateHealthUI(Health);
	if(Health <= 0)
	{
		DeathMatchDeath(DamageCauser);
		//????????????
	}
}

void AFpsBaseCharacter::DeathMatchDeath(AActor* DamageCauser)
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodiesWeaponActor();
	AWeaponBaseClient* CurrentClientWeapon = GetCurrentClientFPArmsWeaponActor();
	if(CurrentClientWeapon)
	{
		CurrentClientWeapon -> Destroy();
	}
	if(CurrentServerWeapon)
	{
		CurrentServerWeapon -> Destroy();
	}
	ClientDeathMatchDeath();
	AMultiFpsPlayerController* MultiFpsPlayerController = Cast<AMultiFpsPlayerController>(GetController());
	if(MultiFpsPlayerController)
	{
		MultiFpsPlayerController -> DeathMatchDeath(DamageCauser);
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
		//??????(?????????????????????????????????????????????
		ServerPrimaryWeapon -> MultiShootingEffect();
		ServerPrimaryWeapon -> ClipCurrentAmmo -= 1;
		//?????????????????????????????????????????????
		MultiShooting();
		//???????????????UI
		ClientUpdateAmmoUI(ServerPrimaryWeapon -> ClipCurrentAmmo, ServerPrimaryWeapon -> GunCurrentAmmo);
		//????????????
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
		//??????(?????????????????????????????????????????????
		ServerPrimaryWeapon -> MultiShootingEffect();
		ServerPrimaryWeapon -> ClipCurrentAmmo -= 1;
		//?????????????????????????????????????????????
		MultiShooting();
		//???????????????UI
		ClientUpdateAmmoUI(ServerPrimaryWeapon -> ClipCurrentAmmo, ServerPrimaryWeapon -> GunCurrentAmmo);
		//????????????
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
		//??????(?????????????????????????????????????????????
		ServerSecondaryWeapon -> MultiShootingEffect();
		ServerSecondaryWeapon -> ClipCurrentAmmo -= 1;
		//?????????????????????????????????????????????
		MultiShooting();
		//???????????????UI
		ClientUpdateAmmoUI(ServerSecondaryWeapon -> ClipCurrentAmmo, ServerSecondaryWeapon -> GunCurrentAmmo);
		//????????????
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
	//?????????????????????????????????????????????????????????????????????????????????UI??????
	if(ServerPrimaryWeapon)
	{
		//????????????????????????0???????????????
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
		//????????????????????????0???????????????
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
		//????????????
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
		//?????????????????????
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

void AFpsBaseCharacter::ClientDeathMatchDeath_Implementation()
{
	AWeaponBaseClient* CurrentClientWeapon = GetCurrentClientFPArmsWeaponActor();
	if(CurrentClientWeapon)
	{
		CurrentClientWeapon -> Destroy();
	}
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
			//?????????????????????
			FName WeaponSocketName = TEXT("WeaponSocket");
			ClientSecondaryWeapon -> K2_AttachToComponent(FPArmsMesh, WeaponSocketName, EAttachmentRule::SnapToTarget,
				 EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
				 true);

			ClientUpdateAmmoUI(ServerSecondaryWeapon -> ClipCurrentAmmo, ServerSecondaryWeapon -> GunCurrentAmmo);
			//????????????
			if(ClientSecondaryWeapon)
			{
				UpdateFPArmsBlendPose(ClientSecondaryWeapon -> FPArmsBlendPoseIndex);
			}
		}
	}
}

void AFpsBaseCharacter::ClientReload_Implementation()
{
	//???????????????????????????
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
		//??????????????????w
		CurrentClientWeapon -> PlayShootAnimation();
		
		//???????????????????????????
		UAnimMontage* ClientArmsFireMontage = CurrentClientWeapon -> ClientArmsFireAnimMontage;
		ClientArmsAnimBP -> Montage_SetPlayRate(ClientArmsFireMontage, 1);
		ClientArmsAnimBP -> Montage_Play(ClientArmsFireMontage);

		//????????????????????????
		CurrentClientWeapon -> DisplayWeaponEffects();

		//??????????????????
		AMultiFpsPlayerController* MultiFpsPlayerController = Cast<AMultiFpsPlayerController>(GetController());
		if(MultiFpsPlayerController)
		{
			MultiFpsPlayerController -> PlayerCameraShake(CurrentClientWeapon -> CameraShakeClass);

			//????????????????????????
			MultiFpsPlayerController -> DoCrosshairRecoil();
		}
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
			//?????????????????????
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
			//????????????
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
	//??????????????????UI????????????????????????????????????????????????
	//??????IsAiming??????????????????RPC
	if(ActiveWeapon == EWeaponType::Sniper)
	{
		ServerSetAiming(true);
		ClientAiming();
	}
}

void AFpsBaseCharacter::InputAimingReleased()
{
	//??????????????????UI?????????????????????????????????????????????
	//??????IsAiming??????????????????RPC
	if(ActiveWeapon == EWeaponType::Sniper)
	{
		ServerSetAiming(false);
		ClientStopAiming();
	}
}

void AFpsBaseCharacter::InputAttackPressed()
{
	//???????????????
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


