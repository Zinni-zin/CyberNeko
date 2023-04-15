// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Components/TimelineComponent.h"

#include "../Interfaces/CharacterActions.h"

#include "../Public/Components/C_WallRun.h"

#include <stdint.h>

#include "ZPlayer.generated.h"


// To start a build, press CTRL + ALT + F11 

class USpringArmComponent;
class UCameraComponent;
class UAnimMontage;
class UTimelineComponent;
class UAC_TimeRewind;

USTRUCT(BlueprintType)
struct FMontagePair
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UAnimMontage* montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float playRate = 1.f;
};

UCLASS()
class CYBERNEKO_API AZPlayer : public ACharacter, public ICharacterActions
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
		inline bool IsDiving() const { return m_isDiving; }

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
		inline int GetSeconds() const { return m_seconds; }

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
		inline int GetMilliseconds() const { return m_milliseconds; }

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
		inline float GetRewindTime() const { return m_rewindTime; }

	//UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
	inline bool IsSwingingForward() const {	return isSwingingForward; }
	
	//UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
	inline bool IsSwingingBackward() const { return isSwingingBackward; }

	inline bool IsOnRightWall() const { return wallRunComp->IsOnRightWall(); }
	inline bool IsOnLeftWall() const { return wallRunComp->IsOnLeftWall(); }

protected:
	/* ---- Movement ---- */
	UFUNCTION(BlueprintCallable)
		void MoveFAB(float val); // FAB = Forward and Backwords

	UFUNCTION(BlueprintCallable)
		void MoveHoriz(float val);

	UFUNCTION(BlueprintCallable)
		void CharJump();

	UFUNCTION(BlueprintCallable)
		void ResetJump();

	UFUNCTION(BlueprintCallable)
		void Land(FHitResult hit);

	UFUNCTION(BlueprintCallable)
		void Dive();

	/* ---- Camera ---- */

	UFUNCTION(BlueprintCallable)
		void SwapCamShoulder();

	UFUNCTION(BlueprintCallable)
		void CamZoom(bool isZoomingIn = true); // Will zoom out if zooming in is false

	/* ---- Time ---- */

	UFUNCTION(BlueprintCallable)
		void StartRewind();

	UFUNCTION(BlueprintCallable)
		void StopRewind();


	UFUNCTION(BlueprintCallable)
		void startTimer(bool canCountDown);
	
	UFUNCTION(BlueprintCallable)
		void endTimer();

	UFUNCTION(BlueprintCallable)
		void startTimerTimeline(bool canCountDown);
	
	UFUNCTION(BlueprintCallable)
		void endTimerTimeline();

	UFUNCTION(BlueprintCallable)
		float GetRewindTimeAsFloat();

	UFUNCTION()
		void Event_OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/* Interface Implemnatations */

	void CharDived_Implementation(bool isFinished);

	void CharIsWallRunning_Implementation();

	void PlayBackwardsMontage_Implementation(UAnimMontage* montage, float playRate);

private:

	UFUNCTION()
		void timerCount(bool canCountDown);

	UFUNCTION()
		void timerTimelineCallback(float val);

	UFUNCTION()
		void timerTimelineFinishedCallback();

// Variables
protected:
	/* Movement */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dive")
		float diveBoost = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dive")
		float diveHeightBoost = 350.f;

	// Boost the distance of the dive when starting from the ground
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dive")
		float diveForwardGroundMultiplier = 1.5f;

	// Boost the height of the dive when starting from the ground
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dive")
		float diveUpGroundMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swinging")
		bool isSwingingForward = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swinging")
		bool isSwingingBackward = false;

	/*
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dive")
		bool diveOverrideZ = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dive")
		bool secondDive = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dive")
		bool moveForwardOnFirst = false;
	*/

	/* Camera */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		UCameraComponent* FollowCam;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
		float maxCamZoomInDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
		float maxCamZoomOutDistance = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
		float camZoomSpeed = 100.f;

	/* Montages */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montages")
		FMontagePair firstJump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montages")
		FMontagePair secondJump;

	/* Time */

	UPROPERTY(EditAnywhere, Category = "Time")
		UCurveFloat* timerFloatCurve;

	/* Components */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
		UAC_TimeRewind* timeRewindComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
		UTimelineComponent* timerTimeline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		UC_WallRun* wallRunComp;


private:
	TMap<UAnimMontage*, float> m_montages;

	int m_jumpCount = 0;
	int m_maxJumpCount = 2;

	float m_curMontageRate = 1.f;

	UPROPERTY(BlueprintGetter = GetRewindTime)
	float m_rewindTime = 0.f;

	UPROPERTY(BlueprintGetter = GetSeconds)
		int m_seconds = 0;

	UPROPERTY(BlueprintGetter = GetMilliseconds)
		int m_milliseconds = 00;

	UPROPERTY(BlueprintGetter = IsDiving)
		bool m_isDiving = false;

	FTimerHandle m_timerHandle;
	FTimerDelegate m_timerCountDelegate;

	FOnTimelineFloat m_onTimerTimelineUpdate;
	FOnTimelineEventStatic m_onTimelineFinishedCallback;
};
