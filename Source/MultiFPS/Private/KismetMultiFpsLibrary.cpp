// Fill out your copyright notice in the Description page of Project Settings.


#include "KismetMultiFpsLibrary.h"

#include "EnvironmentQuery/EnvQueryTypes.h"

void UKismetMultiFpsLibrary::SortScores(TArray<FDeathMatchPlayerData>& Scores)
{
	//Scores.Sort([](const FDeathMatchPlayerData& a, const FDeathMatchPlayerData& b){return a.PlayerScore > b.PlayerScore;});
	QuickSort(Scores, 0, Scores.Num()-1);
}

TArray<FDeathMatchPlayerData>& UKismetMultiFpsLibrary::QuickSort(TArray<FDeathMatchPlayerData>& Scores, int start,
	int end)
{
	if(start >= end)
	{
		return Scores;
	}
	int i = start, j = end;
	FDeathMatchPlayerData Temp = Scores[start];
	while(i != j)
	{
		while(j > i && Scores[j].PlayerScore <= Temp.PlayerScore)
		{
			j--;
		}
		Scores[i] = Scores[j];
		while(j > i && Scores[i].PlayerScore >= Temp.PlayerScore)
		{
			i++;
		}
		Scores[j] = Scores[i];
	}
	Scores[i] = Temp;
	QuickSort(Scores, start, i-1);
	QuickSort(Scores,i+1, end);

	return Scores;
}
