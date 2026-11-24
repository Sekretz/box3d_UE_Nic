## Blueprint kinematic pawn

`ABox3DPawn` is a Blueprintable Pawn with a kinematic Box3D capsule body and a small Blueprint movement API. Add it to a level or derive a Blueprint, set capsule size/speed/gravity/jump speed, and call `AddBox3DMovementInput` with a world-space direction from your input events. `Jump`, `StopJumping`, `IsBox3DGrounded`, `GetBox3DVelocity`, `SetBox3DVelocity`, and the `OnBox3DLanded` event are exposed to Blueprint. It sweeps and slides using Box3D capsule queries; it does not use `UCharacterMovementComponent` or Chaos collision.

The default collision filter puts pawns in the `Pawn` category and collides with `WorldStatic` and `WorldDynamic`. Pawn-to-pawn collision can be enabled by editing the Box3D body's filter in the derived Blueprint. The capsule dimensions and body shape are initialized at play start, so configure them before BeginPlay.

