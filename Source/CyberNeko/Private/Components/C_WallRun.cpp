// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/C_WallRun.h"

#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>
#include "../../Public/Characters/ZPlayer.h"

#define IsValidWallRuNVec(vec) (vec.Z >= -0.52f && vec.Z <= 0.52f)

// Sets default values for this component's properties
UC_WallRun::UC_WallRun()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UC_WallRun::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void UC_WallRun::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UC_WallRun::Initialize(ACharacter* character)
{
	m_character = character;
	m_movementComp = m_character->GetCharacterMovement();

	m_defaultGravityScale = m_movementComp->GravityScale;

	GetWorld()->GetTimerManager().SetTimer(m_updateTimer, this, 
		&UC_WallRun::updateWallRun, 0.02f, true, 0.f);
}

void UC_WallRun::updateWallRun()
{
	if (m_isWallRunSuppressed)
		return;

	if (AZPlayer* player = dynamic_cast<AZPlayer*>(m_character))
	{
		if (!player->HasAbilityFlag(EAbilityFlags::CanWallRun) || player->IsDiving())
			return;
	}

	bool isOnWall = wallRunMovement(m_character->GetActorLocation(),
		getEndVector(1), -1.f);

	if (isOnWall)
	{
		m_isWallRunning = true;
		m_isOnRightWall = true;
		m_isOnLeftWall = false;
		setGravityScale();

		if (GetOwner()->GetClass()->ImplementsInterface(UCharacterActions::StaticClass()))
			ICharacterActions::Execute_CharIsWallRunning(GetOwner());
		return;
	}
	else if (m_isOnRightWall)
	{
		endWallRun(1.f);
		return;
	}

	isOnWall = wallRunMovement(m_character->GetActorLocation(),
		getEndVector(-1), 1.f);

	if (!isOnWall)
	{
		endWallRun(1.f);
		return;
	}

	m_isWallRunning = true;
	m_isOnRightWall = false;
	m_isOnLeftWall = true;

	if (GetOwner()->GetClass()->ImplementsInterface(UCharacterActions::StaticClass()))
		ICharacterActions::Execute_CharIsWallRunning(GetOwner());

	setGravityScale();
}

void UC_WallRun::JumpOffWall()
{
	if (!m_isWallRunning) return;

	endWallRun(0.35f);

	FVector forwardLaunch = m_wallRunNormal * m_wallRunJumpForce;

	m_character->LaunchCharacter(
		FVector(forwardLaunch.X, forwardLaunch.Y, m_wallRunJumpHeight),
		false, true);
}

void UC_WallRun::Land()
{
	endWallRun(0.f);
	m_isWallRunSuppressed = false;
}

bool UC_WallRun::wallRunMovement(FVector start, FVector end, float direction)
{
	FHitResult hit;

	FCollisionQueryParams traceParams;
	GetWorld()->LineTraceSingleByChannel(hit, start, end, ECC_Visibility, traceParams);
	// DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 1.f);

	if (!hit.bBlockingHit) return false;
	
	FVector normal = hit.Normal;
	
	if (!m_movementComp->IsFalling() || !IsValidWallRuNVec(normal)) return false;

	m_wallRunNormal = normal;

	FVector playerToWall = normal * (normal - m_character->GetActorLocation().Length());

	// Stick to wall
	m_character->LaunchCharacter(playerToWall, false, false);

	float moveSpeed = direction * m_wallRunSpeed;

	// Move forward on wall
	m_character->LaunchCharacter(
		FVector::CrossProduct(normal, FVector(0.f, 0.f, 1.f)) * (moveSpeed),
		true, !m_hasGravity);

	return true;
}

void UC_WallRun::endWallRun(float resetTime)
{
	if (!m_isWallRunning) return;

	m_isWallRunning = false;
	m_isOnRightWall = false;
	m_isOnLeftWall = false;
	m_movementComp->GravityScale = m_defaultGravityScale;
	suppressWallRun(resetTime);
}

void UC_WallRun::suppressWallRun(float delay)
{
	m_isWallRunSuppressed = true;
	GetWorld()->GetTimerManager().SetTimer(m_suppressTimer, this,
		&UC_WallRun::resetWallRunSuppression, delay);
}

void UC_WallRun::resetWallRunSuppression()
{
	GetWorld()->GetTimerManager().ClearTimer(m_suppressTimer);
	m_isWallRunSuppressed = false;
}

// 1 = right | -1 = left
FVector UC_WallRun::getEndVector(int direction)
{
	FVector sideVec = m_character->GetActorRightVector() * (m_sideRayLength * direction);
	FVector forwardVec = m_character->GetActorForwardVector() * m_forwardRayAngle;

	return m_character->GetActorLocation() + sideVec + forwardVec;
}

void UC_WallRun::setGravityScale()
{
	m_movementComp->GravityScale = FMath::FInterpTo(
		m_movementComp->GravityScale, 
		m_wallRunTargetGravity,
		GetWorld()->GetDeltaSeconds(), 
		10.f
	);
}