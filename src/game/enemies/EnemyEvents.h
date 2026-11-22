#pragma once
#include <raylib.h>
#include "EnemyTypes.h"

namespace enemies
{
	/// <summary>Published by LifeSystem when an enemy finishes dying (the waves count these in E5).</summary>
	struct EnemyKilledEvent
	{
		EnemyType	type;
		Vector3		position;
	};
}
