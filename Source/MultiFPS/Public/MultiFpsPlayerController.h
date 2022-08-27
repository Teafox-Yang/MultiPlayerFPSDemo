// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MultiFpsPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MULTIFPS_API AMultiFpsPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void PlayerCameraShake(TSubclassOf<UCameraShakeBase> CameraShake);

	UFUNCTION(BlueprintImplementableEvent, Category="PlayerUI")
	void CreatePlayerUI();

	UFUNCTION(BlueprintImplementableEvent, Category="PlayerUI")
	void DoCrosshairRecoil();

	UFUNCTION(BlueprintImplementableEvent, Category="PlayerUI")
	void UpdateAmmoUI(int32 ClipCurrentAmmo, int32 GunCurrentAmmo);

	UFUNCTION(BlueprintImplementableEvent, Category="PlayerUI")
	void UpdateHealthUI(float NewHealth);

	UFUNCTION(BlueprintImplementableEvent, Category="PlayerUI")
	void HidePlayerUI();

	UFUNCTION(BlueprintImplementableEvent, Category="PlayerUI")
	void ShowPlayerUI();
};

