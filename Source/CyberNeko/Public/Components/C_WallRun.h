// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "../Interfaces/CharacterActions.h"

#include "C_WallRun.generated.h"

class UCharacterMovementComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CYBERNEKO_API UC_WallRun : public UActorComponent, public ICharacterActions
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UC_WallRun();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Initialize(ACharacter* character);

	UFUNCTION(BlueprintCallable)
		void JumpOffWall();

	UFUNCTION(BlueprintCallable)
		void Land();

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
		inline bool IsWallRunning() const { return m_isWallRunning; }

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
		inline bool IsOnRightWall() const { return m_isOnRightWall; }
	
	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
		inline bool IsOnLeftWall() const { return m_isOnLeftWall; }

private:
	UFUNCTION()
		void updateWallRun();

	bool wallRunMovement(FVector start, FVector end, float direction);
	
	void endWallRun(float resetTime);

	void suppressWallRun(float delay);
	
	UFUNCTION()
		void resetWallRunSuppression();

	// 1 = right | -1 = left
	FVector getEndVector(int direction = 1);

	void setGravityScale();

public:
	/* ---- Movement ---- */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		float m_wallRunJumpHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		float m_wallRunJumpForce = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		float m_wallRunSpeed = 500.f;

	/* ---- Gravity ---- */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
		bool m_hasGravity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
		float m_wallRunTargetGravity = 0.35f;

	/* ---- Raycast ---- */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Raycast")
		float m_sideRayLength = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Raycast")
		float m_forwardRayAngle = 0.f;

private:
	ACharacter* m_character;
	UCharacterMovementComponent* m_movementComp;

	FTimerHandle m_updateTimer;
	FTimerHandle m_suppressTimer;

	FVector m_wallRunNormal;

	UPROPERTY(BlueprintGetter = IsWallRunning)
		bool m_isWallRunning = false;

	UPROPERTY(BlueprintGetter = IsOnRightWall)
		bool m_isOnRightWall = false;

	UPROPERTY(BlueprintGetter = IsOnLeftWall)
		bool m_isOnLeftWall = false;

	float m_defaultGravityScale = 1.f;;

	bool m_isWallRunSuppressed = false;
};
