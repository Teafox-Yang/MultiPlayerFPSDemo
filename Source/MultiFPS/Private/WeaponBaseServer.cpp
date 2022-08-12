#include "WeaponBaseServer.h"

#include "FpsBaseCharacter.h"

AWeaponBaseServer::AWeaponBaseServer()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision -> SetupAttachment(WeaponMesh);

	WeaponMesh -> SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh -> SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);

	SphereCollision -> SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision -> SetCollisionObjectType((ECollisionChannel::ECC_WorldDynamic));
	
	WeaponMesh -> SetOwnerNoSee(true);
	WeaponMesh -> SetEnableGravity(true);
	WeaponMesh -> SetSimulatePhysics(true);

	SphereCollision -> OnComponentBeginOverlap.AddDynamic(this, &AWeaponBaseServer::OnOtherBeginOverlap);

	SetReplicates(true);
}

void AWeaponBaseServer::OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AFpsBaseCharacter* FpsCharacter = Cast<AFpsBaseCharacter>(OtherActor);
	if(FpsCharacter)
	{
		EquipWeapon();
		//玩家逻辑
		FpsCharacter -> EquipPrimary(this);
	}
}

//被拾取时自动设置碰撞相关属性
void AWeaponBaseServer::EquipWeapon()
{
	WeaponMesh -> SetEnableGravity(false);
	WeaponMesh -> SetSimulatePhysics(false);
	SphereCollision -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeaponBaseServer::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeaponBaseServer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

