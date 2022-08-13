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


private:
#pragma region Component
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UCameraComponent* PlayerCamera;

	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FPArmsMesh;

	UPROPERTY(Category=Character, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UAnimInstance* ClientArmsAnimBP;

	UPROPERTY(Category=Character, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UAnimInstance* ServerBodiesAnimBP;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	AMultiFpsPlayerController* FpsPlayerController;
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
	void EquipPrimary(AWeaponBaseServer* WeaponBaseServer);
	
private:
	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess = "true"))
	EWeaponType ActiveWeapon;
	
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerPrimaryWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClient* ClientPrimaryWeapon;

	void StartWithKindOfWeapon();

	void PurchaseWeapon(EWeaponType WeaponType);

	AWeaponBaseClient* GetCurrentClientFPArmsWeaponActor();
	
#pragma endregion

#pragma region Fire
public:
	void FireWeaponPrimary();
	void StopFirePrimary();
	void RifleLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving);
	void DamagePlayer(UPhysicalMaterial* PhysicalMaterial, AActor* DamagedActor, FVector& HitFromLocation, FHitResult& HitInfo);

	UFUNCTION()
	void OnHit(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);

	float Health;
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

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiShooting();
	void MultiShooting_Implementation();
	bool MultiShooting_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiSpawnBulletDecal(FVector Location, FRotator Rotation);
	void MultiSpawnBulletDecal_Implementation(FVector Location, FRotator Rotation);
	bool MultiSpawnBulletDecal_Validate(FVector Location, FRotator Rotation);

	UFUNCTION(Client, Reliable)
	void ClientEquipFPArmsPrimary();

	UFUNCTION(Client, Reliable)
	void ClientFire();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmoUI(int32 ClipCurrentAmmo, int32 GunCurrentAmmo);

	UFUNCTION(Client, Reliable)
	 void ClientUpdateHealthUI(float NewHealth);

#pragma endregion  

};
