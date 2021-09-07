// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnVolume.generated.h"

UCLASS()
class FIRSTPROJECT_API ASpawnVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawnVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Spawning)
	class UBoxComponent* SpawningBox;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Spawning)
	TSubclassOf<AActor> Actor_1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Spawning)
	TSubclassOf<AActor> Actor_2;

	TArray<TSubclassOf<AActor>> SpawnArray;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = Spawning)
	FVector GetSpawningPoint();

	UFUNCTION(BlueprintPure, Category = Spawning)
	TSubclassOf<AActor> GetSpawningActor();


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Spawning)
	void SpawnOurActor(UClass* ToSpawn, const FVector Location);

};
