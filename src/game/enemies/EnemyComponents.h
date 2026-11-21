#pragma once
#include <raylib.h>
#include <cstdint>
#include "EnemyTypes.h"

/// <summary>
/// Plain structs, no virtuals (plan §5). Simulation components are kept apart from the render-only EnemyLook.
/// The values are copied from the archetype at spawn (E-D8). The namespace avoids a clash with raylib's ::Transform.
/// </summary>
namespace enemies
{
	enum class AiState : uint8_t { Idle, Wander, Chase, Attack, Dying };

	struct Transform
	{
		Vector3		position	= { 0.0f, 0.0f, 0.0f };
		Quaternion	rotation	= { 0.0f, 0.0f, 0.0f, 1.0f };
	};

	struct Motion
	{
		Vector3	velocity	= { 0.0f, 0.0f, 0.0f };
		Vector3	forward		= { 0.0f, 0.0f, -1.0f };
		float	turnRateRad	= 0.0f;
	};

	struct Brain
	{
		AiState	state			= AiState::Idle;
		float	stateTime		= 0.0f;		// seconds in the current state
		float	idleDuration	= 5.0f;
	};

	struct ScanForPlayer
	{
		float	triggerRadiusSq	= 0.0f;
		float	loseRadiusSq	= 0.0f;		// 1.44 x trigger (E-D6)
		float	distSq			= 0.0f;		// out, written by ScanSystem
		Vector3	toPlayerDir		= { 0.0f, 0.0f, 0.0f };
	};

	struct Wander
	{
		float		speed			= 0.0f;
		float		probeDistance	= 0.0f;	// speed x 0.4 s, at least 1 m (§7.3)
		uint32_t	rngState		= 1;	// xorshift state, never 0
	};

	struct ChasePlayer
	{
		float	speed	= 0.0f;
	};

	struct AvoidOthers
	{
		float	strength	= 1.0f;
	};

	struct DealDamage
	{
		float	damage			= 0.0f;
		float	rangeSq			= 0.0f;
		float	exitRangeSq		= 0.0f;		// 1.44 x range (E-D6)
		float	cooldown		= 1.0f;		// seconds, the same for every type
		float	cooldownLeft	= 0.0f;		// keeps ticking outside Attack (E-D5)
	};

	struct Life
	{
		float	hp			= 0.0f;
		float	maxHp		= 0.0f;
		float	hitFlash	= 0.0f;		// seconds left of the white flash
		float	dyingTime	= 0.0f;
		bool	isDead		= false;	// set by LifeSystem; FlushReleased destroys the entity
	};

	/// <summary>
	/// Fake aggro (E-D15): where the player was when they shot this enemy from outside its chase range.
	/// </summary>
	struct Alert
	{
		Vector3	shotFrom	= { 0.0f, 0.0f, 0.0f };
		bool	pending		= false;
	};

	struct CollisionSphere
	{
		float	radius	= 0.5f;
	};

	struct EnemyKind
	{
		EnemyType	type	= EnemyType::Drifter;
	};

	/// <summary>
	/// Render-only data (cold). Materials are not stored here: they come from the manager's per-type table.
	/// </summary>
	struct EnemyLook
	{
		float		scale			= 1.0f;
		float		spawnTime		= 0.0f;	// scale-in: 0.3 s from 0 to 1 (§8.2)
		float		lunge			= 0.0f;	// metres of forward offset of the body
		float		bobPhase		= 0.0f;
		Quaternion	renderRotation	= { 0.0f, 0.0f, 0.0f, 1.0f };	// smoothed toward Transform.rotation
	};
}
