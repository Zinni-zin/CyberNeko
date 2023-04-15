// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CharacterActions.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCharacterActions : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CYBERNEKO_API ICharacterActions
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/* Movement */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movement")
		void CharJumped(int jumpCount, int maxJumpAmount);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movement")
		void CharLanded();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movement")
		void CharDived(bool isFinished = false);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movement")
		void CharIsWallRunning();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Movement")
		void CharGrabbedRope(UObject* rope);

	/* Time */

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Time")
		void TimeRewind(bool isRewinding, float rewindTimeRemaning);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Time")
		void TimerStopped(float time);

	/* Montages */

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Time")
		void PlayBackwardsMontage(UAnimMontage* montage, float playRate);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Montages")
		void MontageEnded(UAnimMontage* montage, float playRate);

};
