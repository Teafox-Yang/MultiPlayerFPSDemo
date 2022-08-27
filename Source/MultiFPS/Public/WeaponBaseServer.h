#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseClient.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "WeaponBaseServer.generated.h"

UENUM()
enum class EWeaponType : uint8
{
	Ak47 UMETA(DisplayName = "AK47"),
	M4A1 UMETA(DisplayName = "M4A1"),
	MP7 UMETA(DisplayName = "MP7"),
	DesertEagle UMETA(DisplayName = "DesertEagle"),
	Sniper UMETA(DisplayName = "Sniper")
};

UCLASS()
class MULTIFPS_API AWeaponBaseServer : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBaseServer();
	
	UPROPERTY(EditAnywhere)
	EWeaponType KindOfWeapon;
	

	UPROPERTY(EditAnywhere)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditAnywhere)
	USphereComponent* SphereCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AWeaponBaseClient> ClientWeaponBaseBPClass;
	

	UFUNCTION()
	void OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	
	UFUNCTION()
	void EquipWeapon();
	

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere)
	int32 GunCurrentAmmo; //枪体当前剩余子弹

	UPROPERTY(EditAnywhere, Replicated) //如果服务器改变了，客户端也随之改变
	int32 ClipCurrentAmmo; //弹匣当前剩余子弹

	UPROPERTY(EditAnywhere)
	int32 MaxClipAmmo; //弹匣容量

	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTPBodiesShootAnimMontage;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTPBodiesReloadAnimMontage;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTPBodiesAttackAnimMontage;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTPBodiesAttackUpperAnimMontage;

	UPROPERTY(EditAnywhere)
	float BulletDistance;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* BulletDecalMaterial;
	
	UPROPERTY(EditAnywhere)
	float BaseDamage;

	UPROPERTY(EditAnywhere)
	bool IsAutomatic;

	UPROPERTY(EditAnywhere)
	float AutomaticFireRate;

	UPROPERTY(EditAnywhere)
	UCurveFloat* VerticalRecoilCurve;

	UPROPERTY(EditAnywhere)
	UCurveFloat* HorizontalRecoilCurve;

	UPROPERTY(EditAnywhere)
	float MovingFireRandomRange;

	UPROPERTY(EditAnywhere, Category = "PistolSpreadData")
	float SpreadWeaponCallBackRate;

	UPROPERTY(EditAnywhere, Category = "PistolSpreadData")
	float SpreadWeaponMinIndex;

	UPROPERTY(EditAnywhere, Category = "PistolSpreadData")
	float SpreadWeaponMaxIndex;
	
	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiShootingEffect();
	void MultiShootingEffect_Implementation();
	bool MultiShootingEffect_Validate();
};
