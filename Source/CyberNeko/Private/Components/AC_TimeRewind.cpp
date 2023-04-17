// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AC_TimeRewind.h"

#include <Components/CapsuleComponent.h>

#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>

#include "../../Public/Characters/ZPlayer.h"

TArray<TObjectPtr<UAC_TimeRewind>> UAC_TimeRewind::m_rewindComponents;

// Sets default values for this component's properties
UAC_TimeRewind::UAC_TimeRewind()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

UAC_TimeRewind::~UAC_TimeRewind()
{
	
}

// Called when the game starts
void UAC_TimeRewind::BeginPlay()
{
	Super::BeginPlay();

	m_rewindComponents.Add(this);
}

void UAC_TimeRewind::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// Super::OnComponentDestroyed(bDestroyingHierarchy);
	m_rewindComponents.Remove(this);
}


// Called every frame
void UAC_TimeRewind::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TObjectPtr<AActor> objectToRewind = GetOwner();

	if (!objectToRewind || !m_isActive)
		return;

	// Add items to list when not rewinding
	if (!m_isRewinding)
	{
		FZRewindInfo rewindInfo;
		rewindInfo.ActorTransform = objectToRewind->GetTransform();

		if (canApplyVelocity)
		{
			UPrimitiveComponent* pPrimitive = dynamic_cast<UPrimitiveComponent*>(GetOwner());
			if (pPrimitive)
			{
				rewindInfo.ActorLinearVel = pPrimitive->GetPhysicsLinearVelocity();
				rewindInfo.ActorAngularVel = pPrimitive->GetPhysicsAngularVelocityInDegrees();
			}
		}

		if (m_pCurRewindMontage)
		{
			rewindInfo.MontageToRewind = m_pCurRewindMontage;
			rewindInfo.MontagePlayRate = m_montagePlayRate;
			m_pCurRewindMontage = nullptr;
			m_montagePlayRate = -1.f;
		}

		ACharacter* pActor = dynamic_cast<ACharacter*>(GetOwner());
		if (pActor)
		{
			rewindInfo.MoveSpeed = pActor->GetVelocity().Length();
			
			AZPlayer* pPlayer = dynamic_cast<AZPlayer*>(pActor);
			if (pPlayer)
			{
				rewindInfo.isDiving = pPlayer->IsDiving();
				rewindInfo.isOnRightWall = pPlayer->IsOnRightWall();
				rewindInfo.isOnLeftWall = pPlayer->IsOnLeftWall();
				rewindInfo.isSwingingForward = pPlayer->IsSwingingForward();
				rewindInfo.isSwingingBackward = pPlayer->IsSwingingBackward();
			}
		}

		rewindInfo.miscInfo = miscInfo;

		m_rewindInfo.PushLast(rewindInfo);

		if (m_hasReachedTimeLimit)
			m_rewindInfo.PopFirst();

		return;
	}

	// Rewind

	if (!m_rewindInfo.IsEmpty())
	{
		objectToRewind->SetActorTransform(m_rewindInfo.Last().ActorTransform);

		if (canApplyVelocity)
		{
			UPrimitiveComponent* pPrimitive = dynamic_cast<UPrimitiveComponent*>(GetOwner());
			if (pPrimitive)
			{
				pPrimitive->SetPhysicsLinearVelocity(m_rewindInfo.Last().ActorLinearVel);
				pPrimitive->SetPhysicsAngularVelocityInDegrees(m_rewindInfo.Last().ActorAngularVel);
			}
		}

		TObjectPtr<UAnimMontage> montage = m_rewindInfo.Last().MontageToRewind;

		if (montage && GetOwner()->GetClass()->ImplementsInterface(UCharacterActions::StaticClass()))
			Execute_PlayBackwardsMontage(GetOwner(), montage, m_rewindInfo.Last().MontagePlayRate);

		m_curMoveSpeed = m_rewindInfo.Last().MoveSpeed;
		m_hasPlayerDove = m_rewindInfo.Last().isDiving;

		m_curRewindInfo = m_rewindInfo.Last();

		m_rewindInfo.PopLast();
	}
}

void UAC_TimeRewind::TimeRewind_Implementation(bool isRewinding, float rewindTimeRemaning)
{
	m_isRewinding = isRewinding;

	m_hasReachedTimeLimit = isRewinding;

	if (rewindTimeRemaning == 0)
		m_rewindInfo.Empty();

	if (isRewinding)
		OnRewindStart.Broadcast();
	else
		OnRewindStop.Broadcast();
}

void UAC_TimeRewind::MontageEnded_Implementation(UAnimMontage* montage, float playRate)
{
	m_pCurRewindMontage = montage;
	m_montagePlayRate = playRate * -1.f;
}

void UAC_TimeRewind::TimerStopped_Implementation(float time)
{
	m_hasReachedTimeLimit = time >= m_timeLimit && !m_isRewinding;
}

void UAC_TimeRewind::reachTimeLimit()
{
	m_hasReachedTimeLimit = true;

	/*
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow,
			FString::Printf(TEXT("Rewind Size: %d"), m_rewindInfo.Num()));
	*/
}

void UAC_TimeRewind::SetTimeLimit(TArray<UAC_TimeRewind*> timeComponentsToSet,
	float timeLimit)
{
	for (auto timeComp : timeComponentsToSet)
		timeComp->m_timeLimit = timeLimit;
}