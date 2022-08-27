#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBaseClient.generated.h"

UCLASS()
class MULTIFPS_API AWeaponBaseClient : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBaseClient();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsFireAnimMontage;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsReloadAnimMontage;

	UPROPERTY(EditAnywhere)
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	UPROPERTY(EditAnywhere)
	int32 FPArmsBlendPoseIndex;

	UPROPERTY(EditAnywhere)
	float FieldOfAimingView;
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "FPGunAnimation")
	void PlayShootAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "FPGunAnimation")
	void PlayReloadAnimation();

	void DisplayWeaponEffects();
};
