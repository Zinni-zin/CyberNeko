// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include <Containers/Deque.h>

#include "../Interfaces/CharacterActions.h"

#include "AC_TimeRewind.generated.h"

class UCapsuleComponent;

USTRUCT(BlueprintType)
struct FZRewindInfo
{
	GENERATED_BODY()

	FTransform ActorTransform;
	FVector ActorLinearVel;
	FVector ActorAngularVel;
	TObjectPtr<UAnimMontage> MontageToRewind = nullptr;
	float MontagePlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float MoveSpeed = 0.f; // Used for entity animations
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool isDiving = false; // Used for player

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool isOnRightWall = false; // Wall run animation
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool isOnLeftWall = false; // Wall run animation

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool isSwingingForward = false; // Rope swing animation

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool isSwingingBackward = false; // Rope swing animation

	// Any additional info the user wants to record
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float miscInfo = 0.f;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CYBERNEKO_API UAC_TimeRewind : public UActorComponent, public ICharacterActions
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAC_TimeRewind();
	~UAC_TimeRewind();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	void TimeRewind_Implementation(bool isRewinding, float rewindTimeRemaning) override;
	void MontageEnded_Implementation(UAnimMontage* montage, float playRate);
	void TimerStopped_Implementation(float time);

	inline float GetTimeLimit() const { return m_timeLimit; }

	static TArray<TObjectPtr<UAC_TimeRewind>> GetAllTimeRewindComponents()
	{ 
		return m_rewindComponents; 
	}

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
		inline bool IsRewinding() const { return m_isRewinding; }

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly)
		inline FZRewindInfo GetCurrentRewindInfo() const { return m_curRewindInfo; }

	UFUNCTION(BlueprintCallable)
		static void SetTimeLimit(TArray<UAC_TimeRewind*> timeComponentsToSet, 
								 float timeLimit);

	static void resetTimeRewind(TArray<UAC_TimeRewind*> timeComponentsToSet);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimeRewindStart);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimeRewindStop);

private:
	UFUNCTION()
	void reachTimeLimit();

	/* ---- Variables ---- */

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewind Control")
		bool m_isActive = true;

	// Applies recorded velocity
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewind Control")
		bool canApplyVelocity = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timer")
		float m_timeLimit = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timer")
		float m_timeLimitBuffer = 0.3f;

	// Any additional info the user wants to record
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float miscInfo = 0.f;

protected:

	UPROPERTY(BlueprintAssignable)
		FOnTimeRewindStart OnRewindStart;

	UPROPERTY(BlueprintAssignable)
		FOnTimeRewindStop OnRewindStop;

private:
	TDeque<FZRewindInfo> m_rewindInfo;

	TObjectPtr<UAnimMontage> m_pCurRewindMontage = nullptr;
	float m_montagePlayRate = 1.f;

	FTimerHandle m_timerHandle;

	static TArray<TObjectPtr<UAC_TimeRewind>> m_rewindComponents;

	UPROPERTY(BlueprintGetter = IsRewinding)
		bool m_isRewinding = false;

	UPROPERTY(BlueprintGetter = GetCurrentRewindInfo)
		FZRewindInfo m_curRewindInfo;

	bool m_hasReachedTimeLimit = false;
};
