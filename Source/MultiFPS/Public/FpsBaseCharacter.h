// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MultiFpsPlayerController.h"
#include "WeaponBaseServer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "FpsBaseCharacter.generated.h"

//告知UE4生成类的反射数据，类必须派生自UObject
UCLASS()
class MULTIFPS_API AFpsBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFpsBaseCharacter();

	UFUNCTION()
	void DelayBeginPlayCallBack();

private:
#pragma region Component
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UCameraComponent* PlayerCamera;

	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UCameraComponent* PlayerThirdCamera;

	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FPArmsMesh;

	UPROPERTY(Category=Character, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UAnimInstance* ClientArmsAnimBP;

	UPROPERTY(Category=Character, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UAnimInstance* ServerBodiesAnimBP;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	AMultiFpsPlayerController* FpsPlayerController;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UCameraComponent* CurrentCamera;

	UPROPERTY(EditAnywhere)
	EWeaponType TestStartWeapon;

#pragma endregion 
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#pragma region InputEvent
protected:
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void MoveRight(float AxisValue);
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void MoveForward(float AxisValue);
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void JumpAction();
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void StopJumpAction();
	
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void InputFirePressed();
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void InputFireReleased();
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void InputAimingPressed();
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void InputAimingReleased();
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void InputAttackPressed();
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void InputReloadPressed();
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void InputAttackReleased();
	
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void SwitchView();
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void QuietWalkAction();
	UFUNCTION(BlueprintCallable, Category="InputAction")
	void NormalWalkAction();
#pragma endregion

private:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#pragma region Weapon
public:
	
	UPROPERTY(EditAnywhere)
	bool BornWithWeapon;
	void EquipPrimary(AWeaponBaseServer* WeaponBaseServer);

	void EquipSecondary(AWeaponBaseServer* WeaponBaseServer);

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateFPArmsBlendPose(int NewIndex);

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateTPBodiesBlendPose(int NewIndex);
	
private:
	UPROPERTY(meta=(AllowPrivateAccess = "true"), Replicated)
	EWeaponType ActiveWeapon;
	
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerPrimaryWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerSecondaryWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClient* ClientPrimaryWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClient* ClientSecondaryWeapon;

	void StartWithKindOfWeapon();

	void PurchaseWeapon(EWeaponType WeaponType);
	
	UFUNCTION(BlueprintCallable, Category="WeaponFunction")
	AWeaponBaseClient* GetCurrentClientFPArmsWeaponActor();
	
	UFUNCTION(BlueprintCallable, Category="WeaponFunction")
	AWeaponBaseServer* GetCurrentServerTPBodiesWeaponActor();
	
#pragma endregion

#pragma region Fire
public:
	//计时器
	FTimerHandle AutomaticFireTimerHandle;
	void AutomaticFire();
	//后坐力
	float NewVerticalRecoilAmount;
	float OldVerticalRecoilAmount;
	float VerticalRecoilAmount;
	float RecoilXCoordPerShoot;

	float NewHorizontalRecoilAmount;
	float OldHorizontalRecoilAmount;
	float HorizontalRecoilAmount;
	void ResetRecoil();

	float PistolSpreadMin = 0;
	float PistolSpreadMax = 0;
	
	//换弹
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool IsFiring;
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool IsReloading;
	UFUNCTION()
	void DelayPlayArmReloadCallBack();
	
	//步枪射击相关
	void FireWeaponPrimary();
	void StopFirePrimary();
	void RifleLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	//狙击枪射击相关
	void FireWeaponSniper();
	void StopFireSniper();
	void SniperLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	
	UPROPERTY(Replicated);
	bool IsAiming;

	UPROPERTY(VisibleAnywhere, Category = "SniperUI")
	UUserWidget* WidgetScope;

	UPROPERTY(EditAnywhere, Category = "SniperUI")
	TSubclassOf<UUserWidget> SniperScopeBPClass;
	
	UFUNCTION()
	void DelaySniperShootCallBack();
	
	//手枪射击相关
	void FireWeaponSecondary();
	void StopFireSecondary();
	void PistolLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	UFUNCTION()
	void DelaySpreadWeaponShootCallBack();
	
	void DamagePlayer(UPhysicalMaterial* PhysicalMaterial, AActor* DamagedActor, FVector& HitFromLocation, FHitResult& HitInfo);

	UFUNCTION()
	void OnHit(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);

	UPROPERTY(BlueprintReadWrite, Category = "Attacker")
	float Health;

	UFUNCTION(BlueprintCallable)
	void DeathMatchDeath(AActor *DamageCauser);
#pragma endregion

#pragma region NetWorking
public:
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerQuietWalkAction();
	void ServerQuietWalkAction_Implementation();
	bool ServerQuietWalkAction_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerNormalWalkAction();
	void ServerNormalWalkAction_Implementation();
	bool ServerNormalWalkAction_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFireRifleWeapon(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	void ServerFireRifleWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	bool ServerFireRifleWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFireSniperWeapon(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	void ServerFireSniperWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	bool ServerFireSniperWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFirePistolWeapon(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	void ServerFirePistolWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	bool ServerFirePistolWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAttackAction();
	void ServerAttackAction_Implementation();
	bool ServerAttackAction_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReloadPrimary();
	void ServerReloadPrimary_Implementation();
	bool ServerReloadPrimary_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReloadSecondary();
	void ServerReloadSecondary_Implementation();
	bool ServerReloadSecondary_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopFiring();
	void ServerStopFiring_Implementation();
	bool ServerStopFiring_Validate();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetAiming(bool AimingState);
	void ServerSetAiming_Implementation(bool AimingState);
	bool ServerSetAiming_Validate(bool AimingState);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetActiveWeapon(EWeaponType WeaponType);
	void ServerSetActiveWeapon_Implementation(EWeaponType WeaponType);
	bool ServerSetActiveWeapon_Validate(EWeaponType WeaponType);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipPrimary(AWeaponBaseServer* ServerWeapon);
	void ServerEquipPrimary_Implementation(AWeaponBaseServer* ServerWeapon);
	bool ServerEquipPrimary_Validate(AWeaponBaseServer* ServerWeapon);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipSecondary(AWeaponBaseServer* ServerWeapon);
	void ServerEquipSecondary_Implementation(AWeaponBaseServer* ServerWeapon);
	bool ServerEquipSecondary_Validate(AWeaponBaseServer* ServerWeapon);
	
	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiShooting();
	void MultiShooting_Implementation();
	bool MultiShooting_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void SetBodyBlendPose(int NewIndex);
	void SetBodyBlendPose_Implementation(int NewIndex);
	bool SetBodyBlendPose_Validate(int NewIndex);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiReloading();
	void MultiReloading_Implementation();
	bool MultiReloading_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiAttack();
	void MultiAttack_Implementation();
	bool MultiAttack_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiSpawnBulletDecal(FVector Location, FRotator Rotation);
	void MultiSpawnBulletDecal_Implementation(FVector Location, FRotator Rotation);
	bool MultiSpawnBulletDecal_Validate(FVector Location, FRotator Rotation);

	UFUNCTION(Client, Reliable)
	void ClientEquipFPArmsPrimary();

	UFUNCTION(Client, Reliable)
	void ClientEquipFPArmsSecondary();

	UFUNCTION(Client, Reliable)
	void ClientFire();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmoUI(int32 ClipCurrentAmmo, int32 GunCurrentAmmo);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void ClientUpdateHealthUI(float NewHealth);

	UFUNCTION(Client, Reliable)
	void ClientRecoil();

	UFUNCTION(Client, Reliable)
	void ClientReload();

	UFUNCTION(Client, Reliable)
	void ClientAiming();

	UFUNCTION(Client, Reliable)
	void ClientStopAiming();

	UFUNCTION(Client, Reliable)
	void ClientDeathMatchDeath();

#pragma endregion  

};
