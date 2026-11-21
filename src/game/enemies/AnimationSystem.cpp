#include "AnimationSystem.h"
#include <algorithm>
#include <cmath>
#include "EnemyComponents.h"

namespace enemies
{
	namespace
	{
		constexpr float	SPAWN_SCALE_IN_TIME	= 0.3f;		// seconds
		constexpr float	TURN_SMOOTHING_RATE	= 12.0f;	// per second
	}

	void AnimationSystem(eng::ecs::Registry& registry, float dt)
	{
		const float turnBlend = 1.0f - std::exp(-TURN_SMOOTHING_RATE * dt);

		registry.Each<EnemyLook, Transform>([&](eng::ecs::Entity, EnemyLook& look, Transform& transform)
		{
			look.spawnTime = std::min(look.spawnTime + dt, SPAWN_SCALE_IN_TIME);
			const float t = look.spawnTime / SPAWN_SCALE_IN_TIME;
			look.scale = 1.0f - (1.0f - t) * (1.0f - t);	// ease-out

			look.renderRotation = QuaternionSlerp(look.renderRotation, transform.rotation, turnBlend);
		});
	}
}
