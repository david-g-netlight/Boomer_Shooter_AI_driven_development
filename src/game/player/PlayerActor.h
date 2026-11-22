#pragma once
#include <eng.h>

class PlayerInput;
class PlayerState;
class HitscanTargets;

/// <summary>
/// What the player's components need. All of it is owned by the game and outlives the Level.
/// </summary>
struct PlayerContext
{
	PlayerInput*	input	= nullptr;
	PlayerState*	state	= nullptr;
	eng::Camera*	camera	= nullptr;
	const HitscanTargets*	targets	= nullptr;	// what the gun can hit besides walls and props (the enemies)
};

/// <summary>
/// Builds the player: an actor "Player" with a capsule Collider (layer Player), CharacterMotor,
/// FirstPersonController, FirstPersonLook and Gun.
/// The actor position is the capsule center (the feet are 1 m below). Level files don't contain the player
/// (its components aren't registered, so the serializer skips it): the game spawns it from the level's spawn point.
/// </summary>
namespace PlayerActor
{
	constexpr float CAPSULE_RADIUS	= 0.5f;
	constexpr float CAPSULE_HEIGHT	= 1.0f;		// between the sphere centers: 2 m tall in total

	/// <summary>
	/// `feetPosition` is where the soles go; `yawDegrees` the facing (positive turns right).
	/// withBodyMesh: a blue cylinder child, only to see the player in the minimap and from the editor.
	/// </summary>
	eng::GameActor*	Create(eng::Level& level, const PlayerContext& context, const Vector3& feetPosition, float yawDegrees,
						   bool withBodyMesh);
}
