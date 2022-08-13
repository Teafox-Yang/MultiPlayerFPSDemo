#include "WeaponBaseClient.h"

#include "Kismet/GameplayStatics.h"

AWeaponBaseClient::AWeaponBaseClient()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh -> SetOnlyOwnerSee(true);
}

void AWeaponBaseClient::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeaponBaseClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeaponBaseClient::DisplayWeaponEffects()
{
	UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, WeaponMesh, TEXT("Fire_FX_Slot"),
		FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
		EAttachLocation::KeepRelativeOffset, true, EPSCPoolMethod::None,
		true);
	UGameplayStatics::PlaySound2D(GetWorld(), FireSound);
}

