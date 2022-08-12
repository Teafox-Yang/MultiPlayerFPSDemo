// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	UCameraComponent* PlayerCamera;

	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FPArmsMesh;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#pragma region InputEvent
	void MoveRight(float AxisValue);
	void MoveForward(float AxisValue);
	
	void JumpAction();
	void StopJumpAction();

	void QuietWalkAction();
	void NormalWalkAction();
#pragma endregion

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#pragma region Weapon
public:
	void EquipPrimary(AWeaponBaseServer* WeaponBaseServer);
private:
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerPrimaryWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClient* ClientPrimaryWeapon;

	void StartWithKindOfWeapon();

	void PurchaseWeapon(EWeaponType WeaponType);
	
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

	UFUNCTION(Client, Reliable)
	void ClientEquipFPArmsPrimary();
#pragma endregion  

};
