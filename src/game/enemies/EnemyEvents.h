#pragma once
#include <raylib.h>
#include "EnemyTypes.h"

namespace enemies
{
	/// <summary>Published by DealDamageSystem for each bite. ArenaGame applies it to the player (§9.2).</summary>
	struct PlayerHitEvent
	{
		float		damage;
		Vector3		from;		// the enemy's position
	};

	/// <summary>Published by LifeSystem when an enemy finishes dying (the waves count these in E5).</summary>
	struct EnemyKilledEvent
	{
		EnemyType	type;
		Vector3		position;
	};
}
