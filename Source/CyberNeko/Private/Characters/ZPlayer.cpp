// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/ZPlayer.h"

#include <GameFramework/CharacterMovementComponent.h>

#include <GameFramework/Controller.h>

#include <GameFramework/SpringArmComponent.h>
#include <Camera/CameraComponent.h>

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
		timerTimeline->SetTimelineLength(m_timeLimit);
		timerTimeline->AddInterpFloat(timerFloatCurve, m_onTimerTimelineUpdate);
		timerTimeline->SetTimelineFinishedFunc(m_onTimelineFinishedCallback);
		timerTimeline->SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
		timerTimeline->Play();
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red,
			FString::Printf(TEXT("No float curve!")));
	}	
}

// Called every frame
void AZPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AZPlayer::MoveFAB(float val)
{
	if (!Controller && val == 0)
		return;

	const FRotator rotation = Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	const FVector direction = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);

	AddMovementInput(direction, val);
}

void AZPlayer::MoveHoriz(float val)
{
	if (!Controller && val == 0)
		return;

	const FRotator rotation = Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	const FVector direction = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(direction, val);
}

void AZPlayer::CharJump()
{
	wallRunComp->JumpOffWall();

	const bool isFalling = GetCharacterMovement()->IsFalling();

	if (isFalling)
		return;

	Jump();

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

	bool overrideZ = isFalling && GetVelocity().Z < 0;

	ICharacterActions::Execute_CharDived(GetMesh()->GetAnimInstance(), false);
	LaunchCharacter(FVector(launchVec.X, launchVec.Y, diveHeight), false, overrideZ);

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
	startTimerTimeline(true);

	auto rewindComponents = UAC_TimeRewind::GetAllTimeRewindComponents();

	for (auto rewindComp : rewindComponents)
		if (IsValid(rewindComp.Get()))
			ICharacterActions::Execute_TimeRewind(rewindComp, true, m_rewindTime);

	ICharacterActions::Execute_TimeRewind(GetMesh()->GetAnimInstance(), true, 0.f);
}

void AZPlayer::StopRewind()
{
	endTimerTimeline();
	
	for (auto rewindComp : UAC_TimeRewind::GetAllTimeRewindComponents())
		if (IsValid(rewindComp.Get()))
			ICharacterActions::Execute_TimeRewind(rewindComp, false, m_rewindTime);

	startTimerTimeline(false);
	m_isDiving = (GetCharacterMovement()->IsFalling())? m_isDiving : false;
	
	ICharacterActions::Execute_TimeRewind(GetMesh()->GetAnimInstance(), false, 0.f);
}

void AZPlayer::startTimerTimeline(bool canCountDown)
{
	endTimerTimeline();

	timerTimeline->SetTimelineLength(m_timeLimit);
	timerTimeline->SetNewTime(m_rewindTime);

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
}

void AZPlayer::timerTimelineFinishedCallback()
{
	for (auto rewindComp : UAC_TimeRewind::GetAllTimeRewindComponents())
		if(IsValid(rewindComp.Get()))
			ICharacterActions::Execute_TimerStopped(rewindComp, m_rewindTime);

}

void AZPlayer::ChangeTimerLimit(float newTimeLimit)
{
	endTimerTimeline();

	m_timeLimit = newTimeLimit;
	m_rewindTime = 0.f;
	timerTimeline->SetNewTime(0.f);

	if (timeRewindComponent->IsRewinding())
	{
		for (auto rewindComp : UAC_TimeRewind::GetAllTimeRewindComponents())
			if (IsValid(rewindComp.Get()))
				ICharacterActions::Execute_TimeRewind(rewindComp, false, m_rewindTime);

		m_isDiving = (GetCharacterMovement()->IsFalling()) ? m_isDiving : false;

		ICharacterActions::Execute_TimeRewind(GetMesh()->GetAnimInstance(), false, 0.f);
	}


	UAC_TimeRewind::resetTimeRewind(UAC_TimeRewind::GetAllTimeRewindComponents());
	
	startTimerTimeline(false);
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

void AZPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
