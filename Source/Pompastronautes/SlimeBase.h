// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SlimeBase.generated.h"

class UNiagaraSystem;


UENUM(BlueprintType)
enum class ESlimeType : uint8
{
	Water   UMETA(DisplayName = "Water"),
	Electric UMETA(DisplayName = "Electric")	
};

UENUM(BlueprintType)
enum class EZoneEffectType : uint8
{
	WaterElectricExplosion   UMETA(DisplayName = "WaterElectricExplosion"),
	FireElectricExplosion  UMETA(DisplayName = "FireElectricExplosion")	
};

UCLASS()
class POMPASTRONAUTES_API ASlimeBase : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASlimeBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float minExplosionDelay = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float maxExplosionDelay = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime")
	ESlimeType SlimeType;

	UPROPERTY(EditAnywhere, Category = "FX Slime")
	UNiagaraSystem* WaterElectricExplosionFX;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called when another slime collides with this one
	UFUNCTION(BlueprintCallable, Category = "Slime")
	virtual void OnHitBySlime(ESlimeType OtherSlimeType);

	// Called when a zone effect (like an electric explosion) reaches this slime
	UFUNCTION(BlueprintCallable, Category = "Slime")
	virtual void OnAffectedByZoneEffect(EZoneEffectType ZoneEffectType, float DistFromSource = 0.0f);

private:
	bool isExploding = false;

	FTimerHandle ExplosionTimerHandle;
	
	void WaterOnHitBySlime(ESlimeType OtherSlimeType);
	void ElectricOnHitBySlime(ESlimeType OtherSlimeType);

	void WaterOnAffectedByZoneEffect(EZoneEffectType ZoneEffectType, float DistFromSource = 0.0f);
	void ElectricOnAffectedByZoneEffect(EZoneEffectType ZoneEffectType, float DistFromSource = 0.0f);

	void WaterElecticityExplosion();

	//FX
	void PlayWaterElectricExplosionFX(float Delay, bool PlayAtLocation = false);
};




