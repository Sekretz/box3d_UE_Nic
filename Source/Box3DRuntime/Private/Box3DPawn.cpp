#include "Box3DPawn.h"

#include "Box3DBodyComponent.h"
#include "Box3DQueryLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

namespace
{
	constexpr float WalkableNormalZ = 0.5f;
	constexpr float SweepBackoff = 0.001f;
	constexpr int32 MaxSlideIterations = 3;
}

ABox3DPawn::ABox3DPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Box3DBody = CreateDefaultSubobject<UBox3DBodyComponent>(TEXT("Box3DBody"));
	Box3DBody->BodyType = EBox3DBodyType::Kinematic;
	Box3DBody->ShapeType = EBox3DShapeType::Capsule;
	Box3DBody->bAutoFitShape = true;
	Box3DBody->Filter.CategoryBits = 1 << static_cast<int32>(EBox3DChannel::Pawn);
	// Exclude Pawn by default to avoid self/peer blocking; adjust the body filter
	// in a Blueprint when pawn-to-pawn collision is desired.
	Box3DBody->Filter.MaskBits = (1 << static_cast<int32>(EBox3DChannel::WorldStatic)) |
		(1 << static_cast<int32>(EBox3DChannel::WorldDynamic));
	SetRootComponent(Box3DBody);

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetupAttachment(Box3DBody);
	Capsule->SetCapsuleRadius(CapsuleRadius);
	Capsule->SetCapsuleHalfHeight(CapsuleHalfHeight);
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABox3DPawn::BeginPlay()
{
	Super::BeginPlay();

	if (Capsule)
	{
		Capsule->SetCapsuleRadius(CapsuleRadius);
		Capsule->SetCapsuleHalfHeight(CapsuleHalfHeight);
	}

	if (Box3DBody)
	{
		// These values are used by the Box3D shape cooker at body creation.
		Box3DBody->CapsuleRadius = CapsuleRadius;
		Box3DBody->CapsuleHalfHeight = CapsuleHalfHeight;
	}
	bIsGrounded = QueryGrounded(GetActorLocation());
}

void ABox3DPawn::AddBox3DMovementInput(FVector WorldDirection, float Scale)
{
	PendingInput += WorldDirection * Scale;
}

void ABox3DPawn::Jump()
{
	bJumpRequested = true;
}

void ABox3DPawn::StopJumping()
{
	bJumpRequested = false;
}

void ABox3DPawn::SetBox3DVelocity(FVector NewVelocity)
{
	Velocity = NewVelocity;
	if (Velocity.Z > 0.0f)
	{
		bIsGrounded = false;
	}
}

void ABox3DPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.0f || !Box3DBody || !Box3DBody->IsSimulating())
	{
		PendingInput = FVector::ZeroVector;
		return;
	}

	const FVector Input = PendingInput.GetClampedToMaxSize(1.0f);
	PendingInput = FVector::ZeroVector;
	Velocity.X = Input.X * MaxSpeed;
	Velocity.Y = Input.Y * MaxSpeed;

	const bool bWasGrounded = bIsGrounded;
	if (bJumpRequested && bIsGrounded)
	{
		Velocity.Z = JumpSpeed;
		bIsGrounded = false;
	}
	bJumpRequested = false;

	if (!bIsGrounded || Velocity.Z > 0.0f)
	{
		Velocity.Z -= Gravity * DeltaSeconds;
	}

	bool bHitWalkableSurface = false;
	const FVector NewLocation = ConsumeMovement(GetActorLocation(), Velocity * DeltaSeconds, bHitWalkableSurface);
	SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);

	bIsGrounded = bHitWalkableSurface || QueryGrounded(NewLocation);
	if (bIsGrounded && Velocity.Z < 0.0f)
	{
		Velocity.Z = 0.0f;
	}
	if (!bWasGrounded && bIsGrounded)
	{
		OnBox3DLanded.Broadcast();
	}
}

FVector ABox3DPawn::ConsumeMovement(const FVector& Start, const FVector& DesiredDelta, bool& bHitWalkableSurface)
{
	FVector Position = Start;
	FVector Remaining = DesiredDelta;
	FBox3DQueryFilter Filter;
	Filter.CategoryBits = Box3DBody->Filter.CategoryBits;
	Filter.MaskBits = Box3DBody->Filter.MaskBits;

	for (int32 Iteration = 0; Iteration < MaxSlideIterations && !Remaining.IsNearlyZero(); ++Iteration)
	{
		FBox3DHitResult Hit;
		const bool bHit = UBox3DQueryLibrary::Box3DCapsuleCast(
			this, Position, Position + Remaining, CapsuleRadius, CapsuleHalfHeight, Filter, Hit);
		if (!bHit || Hit.Actor == this || Hit.Component == Box3DBody)
		{
			Position += Remaining;
			break;
		}

		const float SafeFraction = FMath::Clamp(Hit.Fraction - SweepBackoff, 0.0f, 1.0f);
		Position += Remaining * SafeFraction;
		Remaining *= (1.0f - SafeFraction);
		Remaining = FVector::VectorPlaneProject(Remaining, Hit.Normal.GetSafeNormal());
		if (Hit.Normal.Z >= WalkableNormalZ && DesiredDelta.Z <= 0.0f)
		{
			bHitWalkableSurface = true;
		}
	}
	return Position;
}

bool ABox3DPawn::QueryGrounded(const FVector& Position) const
{
	if (!GetWorld() || GroundCheckDistance <= 0.0f)
	{
		return false;
	}

	FBox3DQueryFilter Filter;
	Filter.CategoryBits = Box3DBody ? Box3DBody->Filter.CategoryBits : -1;
	Filter.MaskBits = Box3DBody ? Box3DBody->Filter.MaskBits : -1;
	FBox3DHitResult Hit;
	const FVector End = Position - FVector::UpVector * GroundCheckDistance;
	return UBox3DQueryLibrary::Box3DCapsuleCast(
		const_cast<ABox3DPawn*>(this), Position, End, CapsuleRadius, CapsuleHalfHeight, Filter, Hit) &&
		Hit.Actor != this && Hit.Component != Box3DBody && Hit.Normal.Z >= WalkableNormalZ;
}
