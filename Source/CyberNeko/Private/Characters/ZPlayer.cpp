// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/ZPlayer.h"

#include <GameFramework/CharacterMovementComponent.h>

#include <GameFramework/Controller.h>

#include <GameFramework/SpringArmComponent.h>
#include <Camera/CameraComponent.h>

// #include <Components/TimelineComponent.h>

#include "../Public/Components/AC_TimeRewind.h"

// Sets default values
AZPlayer::AZPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	FollowCam = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCam->SetupAttachment(CameraBoom);

	timeRewindComponent = CreateDefaultSubobject<UAC_TimeRewind>(TEXT("TimeRewindComponent"));

	wallRunComp = CreateDefaultSubobject<UC_WallRun>(TEXT("WallRunComponent"));

	timerTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TimelineComponent"));
}

// Called when the game starts or when spawned
void AZPlayer::BeginPlay()
{
	Super::BeginPlay();

	m_montages.Add(firstJump.montage, firstJump.playRate);
	m_montages.Add(secondJump.montage, secondJump.playRate);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->OnMontageEnded.AddDynamic(this, &AZPlayer::Event_OnMontageEnded);

	wallRunComp->Initialize(this);

	m_onTimelineFinishedCallback.BindUFunction(this, FName(TEXT("timerTimelineFinishedCallback")));
	m_onTimerTimelineUpdate.BindDynamic(this, &AZPlayer::timerTimelineCallback);

	if (timerFloatCurve)
	{
		timerTimeline->SetTimelineLength(timeRewindComponent->GetTimeLimit());
		timerTimeline->AddInterpFloat(timerFloatCurve, m_onTimerTimelineUpdate);
		timerTimeline->SetTimelineFinishedFunc(m_onTimelineFinishedCallback);
		timerTimeline->SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
		timerTimeline->Play();

		/*
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue,
			FString::Printf(TEXT("Time Limit: %f"), timerTimeline->GetTimelineLength()));
		*/
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red,
			FString::Printf(TEXT("No float curve!"), m_rewindTime));
	}	
}

// Called every frame
void AZPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AZPlayer::MoveFAB(float val)
{
	if (!Controller && val == 0) return;

	const FRotator rotation = Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	const FVector direction = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);

	AddMovementInput(direction, val);
}

void AZPlayer::MoveHoriz(float val)
{
	if (!Controller && val == 0) return;

	const FRotator rotation = Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	const FVector direction = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(direction, val);
}

void AZPlayer::CharJump()
{
	wallRunComp->JumpOffWall();

	const bool isFalling = GetCharacterMovement()->IsFalling();

	if (isFalling) return;

	// if ((m_jumpCount == 0 && isFalling) || m_jumpCount >= m_maxJumpCount) return;

	// ++m_jumpCount;
	
	Jump();

	/*
	if (isFalling && secondJump.montage)
		PlayAnimMontage(secondJump.montage, secondJump.playRate);
	else if (firstJump.montage)
	*/
	PlayAnimMontage(firstJump.montage, firstJump.playRate);	
}

void AZPlayer::ResetJump()
{
	m_jumpCount = 0;
	StopJumping();
}

void AZPlayer::Land(FHitResult hit)
{
	wallRunComp->Land();
}

void AZPlayer::Dive()
{
	if (wallRunComp->IsWallRunning())
		return;

	m_isDiving = true;

	bool isFalling = GetCharacterMovement()->IsFalling();

	float diveHeight = (isFalling) ? diveHeightBoost : diveHeightBoost * diveUpGroundMultiplier;

	FVector launchVec = GetActorForwardVector() * diveBoost;
	launchVec *= (isFalling) ? 1.f :
		diveForwardGroundMultiplier;

	// FVector firstLaunchVec = moveForwardOnFirst ?
	//	FVector(launchVec.X, launchVec.Y, diveHeight) : FVector(0.f, 0.f, diveHeight);

	bool overrideZ = isFalling && GetVelocity().Z < 0;

	ICharacterActions::Execute_CharDived(GetMesh()->GetAnimInstance(), false);
	LaunchCharacter(FVector(launchVec.X, launchVec.Y, diveHeight), false, overrideZ);

	/*
	FTimerHandle launchDelay;

	if (secondDive)
	{
		FTimerDelegate launchDelagate;
		launchDelagate.BindLambda([&]()
			{
				
				//FVector launchVec = GetActorForwardVector() * diveBoost;
				//launchVec *= (isFalling) ? 1.f :
				//	diveForwardGroundMultiplier;
				
				launchVec = GetActorForwardVector() * diveBoost;
				launchVec *= (isFalling) ? 1.f :
					diveForwardGroundMultiplier;

				LaunchCharacter(FVector(launchVec.X, launchVec.Y, 0.f), false, false);
			});

		GetWorld()->GetTimerManager().SetTimer(launchDelay, launchDelagate, 0.05f, false);
	}
	*/

	StopAnimMontage(GetCurrentMontage());
}

/* ---- Camera ----- */

void AZPlayer::SwapCamShoulder()
{
	CameraBoom->SetRelativeLocation(CameraBoom->GetRelativeLocation() * FVector(1.f, -1.f, 1.f));
}

void AZPlayer::CamZoom(bool isZoomingIn)
{
	float dir = isZoomingIn ? 1.f : -1.f;

	CameraBoom->TargetArmLength = std::clamp(CameraBoom->TargetArmLength - (dir * camZoomSpeed),
		maxCamZoomInDistance, maxCamZoomOutDistance);
}

/* ---- Time ---- */

void AZPlayer::StartRewind()
{
	// startTimer(true);
	startTimerTimeline(true);

	auto rewindComponents = UAC_TimeRewind::GetAllTimeRewindComponents();

	for (auto rewindComp : rewindComponents)
		if (IsValid(rewindComp.Get()))
			ICharacterActions::Execute_TimeRewind(rewindComp, true, m_rewindTime);

	//ICharacterActions::Execute_TimeRewind(timeRewindComponent, true, m_rewindTime);
	ICharacterActions::Execute_TimeRewind(GetMesh()->GetAnimInstance(), true, 0.f);
}

void AZPlayer::StopRewind()
{
	// endTimer();
	endTimerTimeline();
	//ICharacterActions::Execute_TimeRewind(timeRewindComponent, false, GetRewindTimeAsFloat());
	// ICharacterActions::Execute_TimeRewind(timeRewindComponent, false, m_rewindTime);

	for (auto rewindComp : UAC_TimeRewind::GetAllTimeRewindComponents())
		if (IsValid(rewindComp.Get()))
			ICharacterActions::Execute_TimeRewind(rewindComp, false, m_rewindTime);

	// startTimer(false);
	startTimerTimeline(false);
	m_isDiving = (GetCharacterMovement()->IsFalling())? m_isDiving : false;
	
	ICharacterActions::Execute_TimeRewind(GetMesh()->GetAnimInstance(), false, 0.f);
}

void AZPlayer::startTimer(bool canCountDown)
{
	if (GetWorld()->GetTimerManager().IsTimerActive(m_timerHandle))
		endTimer();
	
	m_timerCountDelegate.BindUFunction(this, "timerCount", canCountDown);

	GetWorld()->GetTimerManager().SetTimer(m_timerHandle, m_timerCountDelegate, 
		0.01f, true, 0.f);
}

void AZPlayer::endTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(m_timerHandle);
	m_timerCountDelegate.Unbind();
}

void AZPlayer::timerCount(bool canCountDown)
{
	if (canCountDown)
	{
		if (m_milliseconds <= 0)
		{
			--m_seconds;
			m_milliseconds = 100;
		}

		--m_milliseconds;

		if (m_seconds == 0 && m_milliseconds == 0)
			endTimer();

		return;
	}
	
	if (m_milliseconds > 99)
	{
		++m_seconds;
		m_milliseconds = 0;
	}

	float timeLimit = timeRewindComponent->GetTimeLimit();
	int secondsLimit = (int)timeLimit;

	timeLimit -= secondsLimit;

	if (m_seconds == secondsLimit && m_milliseconds == timeLimit * 100)
	{
		endTimer();
		return;
	}

	++m_milliseconds;
}

void AZPlayer::startTimerTimeline(bool canCountDown)
{
	endTimerTimeline();

	timerTimeline->SetTimelineLength(timeRewindComponent->GetTimeLimit());
	timerTimeline->SetNewTime(m_rewindTime);
	// timerTimeline->SetPlaybackPosition(m_rewindTime, true);

	if (canCountDown)
		timerTimeline->Reverse();
	else
		timerTimeline->Play();
}

void AZPlayer::endTimerTimeline()
{
	if(timerTimeline->IsPlaying())
		timerTimeline->Stop();
}

void AZPlayer::timerTimelineCallback(float val)
{
	m_rewindTime = timerTimeline->GetPlaybackPosition();

	m_seconds = (int)m_rewindTime;
	m_milliseconds = (int)((m_rewindTime - m_seconds) * 100);

	/*
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow,
			FString::Printf(TEXT("Rewind Time: %f"), m_rewindTime));
	*/
}

void AZPlayer::timerTimelineFinishedCallback()
{
	// ICharacterActions::Execute_TimerStopped(timeRewindComponent, m_rewindTime);

	for (auto rewindComp : UAC_TimeRewind::GetAllTimeRewindComponents())
		if(IsValid(rewindComp.Get()))
			ICharacterActions::Execute_TimerStopped(rewindComp, m_rewindTime);

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red,
	//		FString::Printf(TEXT("Rewind Timer: %f"), m_rewindTime));
}

float AZPlayer::GetRewindTimeAsFloat()
{
	float time = (float)m_seconds;

	time += ((float)m_milliseconds * 0.0001);

	return time;
}

void AZPlayer::Event_OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	float* pMontagePlayRate = m_montages.Find(Montage);
	float playRate = (pMontagePlayRate)? *pMontagePlayRate : 1.f;

	if (timeRewindComponent->GetClass()->ImplementsInterface(UCharacterActions::StaticClass()))
		ICharacterActions::Execute_MontageEnded(timeRewindComponent, Montage, playRate);
	// Execute_MontageEnded(Cast<ICharacterActions>(timeRewindComponent), Montage, Montage->RateScale);
}

void AZPlayer::CharDived_Implementation(bool isFinished)
{
	m_isDiving = !isFinished;
}

void AZPlayer::CharIsWallRunning_Implementation()
{
	StopAnimMontage(GetCurrentMontage());
}

void AZPlayer::PlayBackwardsMontage_Implementation(UAnimMontage* montage, float playRate)
{
	if (m_seconds != 0 && m_milliseconds != 0)
	{
		UAnimInstance* animInstance = GetMesh()->GetAnimInstance();

		PlayAnimMontage(montage, playRate);
		animInstance->Montage_SetPosition(montage, montage->GetPlayLength());
	}
}

// Called to bind functionality to input
void AZPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
