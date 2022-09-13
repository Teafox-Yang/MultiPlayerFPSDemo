// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KismetMultiFpsLibrary.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FDeathMatchPlayerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName PlayerName;
	UPROPERTY(BlueprintReadWrite)
	int PlayerScore;
	UPROPERTY(BlueprintReadWrite)
	int PlayerKill;
	UPROPERTY(BlueprintReadWrite)
	int PlayerDead;

	FDeathMatchPlayerData()
	{
		PlayerName = TEXT(" ");
		PlayerScore = 0;
		PlayerKill = 0;
		PlayerDead = 0;
	}
	
};
UCLASS()
class MULTIFPS_API UKismetMultiFpsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	UFUNCTION(BlueprintCallable, Category="Sort")
	static void SortScores(UPARAM(ref)TArray<FDeathMatchPlayerData>& Scores);

	static TArray<FDeathMatchPlayerData>& QuickSort(UPARAM(ref)TArray<FDeathMatchPlayerData>& Scores, int start, int end);
};
