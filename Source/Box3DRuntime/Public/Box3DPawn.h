#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Box3DPawn.generated.h"

class UCapsuleComponent;
class UBox3DBodyComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBox3DPawnLandedSignature);

/// Blueprint-ready kinematic pawn driven by Box3D capsule casts. Input is supplied
/// in world space with AddBox3DMovementInput; the pawn's collision is owned by
/// Box3D, not Unreal's CharacterMovement/Chaos simulation.
UCLASS(BlueprintType, Blueprintable, ClassGroup = (Physics))
class BOX3DRUNTIME_API ABox3DPawn : public APawn
{
	GENERATED_BODY()

public:
	ABox3DPawn();

	/// Box3D body is the root and owns the simulated capsule.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Box3D|Pawn")
	TObjectPtr<UBox3DBodyComponent> Box3DBody;

	/// Non-colliding capsule component used for authoring/visual attachment and
	/// auto-fitting the Box3D collision shape.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Box3D|Pawn")
	TObjectPtr<UCapsuleComponent> Capsule;

	/// Capsule dimensions in centimeters. Set before play; the Box3D body shape is
	/// created at BeginPlay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Box3D|Movement", meta = (ClampMin = "1.0"))
	float CapsuleRadius = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Box3D|Movement", meta = (ClampMin = "1.0"))
	float CapsuleHalfHeight = 88.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Box3D|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Box3D|Movement", meta = (ClampMin = "0.0"))
	float Gravity = 980.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Box3D|Movement", meta = (ClampMin = "0.0"))
	float JumpSpeed = 420.0f;

	/// Small downward cast distance used to maintain grounded state (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Box3D|Movement", meta = (ClampMin = "0.0"))
	float GroundCheckDistance = 3.0f;

	UPROPERTY(BlueprintAssignable, Category = "Box3D|Events")
	FBox3DPawnLandedSignature OnBox3DLanded;

	/// Add a world-space movement direction. Call from Blueprint each frame or
	/// from input events; accumulated input is consumed on the next actor tick.
	UFUNCTION(BlueprintCallable, Category = "Box3D|Movement")
	void AddBox3DMovementInput(FVector WorldDirection, float Scale = 1.0f);

	/// Request a jump. The request is consumed on the next tick if grounded.
	UFUNCTION(BlueprintCallable, Category = "Box3D|Movement")
	void Jump();

	UFUNCTION(BlueprintCallable, Category = "Box3D|Movement")
	void StopJumping();

	UFUNCTION(BlueprintPure, Category = "Box3D|Movement")
	bool IsBox3DGrounded() const { return bIsGrounded; }

	/// Pawn velocity in centimeters per second.
	UFUNCTION(BlueprintPure, Category = "Box3D|Movement")
	FVector GetBox3DVelocity() const { return Velocity; }

	/// Set velocity in centimeters per second (for launch pads, knockback, etc.).
	UFUNCTION(BlueprintCallable, Category = "Box3D|Movement")
	void SetBox3DVelocity(FVector NewVelocity);

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

private:
	bool QueryGrounded(const FVector& Position) const;
	FVector ConsumeMovement(const FVector& Start, const FVector& DesiredDelta, bool& bHitWalkableSurface);

	FVector PendingInput = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	bool bJumpRequested = false;
	bool bIsGrounded = false;
};
