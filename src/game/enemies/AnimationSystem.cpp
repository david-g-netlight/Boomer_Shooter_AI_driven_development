#include "AnimationSystem.h"
#include <algorithm>
#include <cmath>
#include <raymath.h>
#include "EnemyComponents.h"

namespace enemies
{
	namespace
	{
		constexpr float	SPAWN_SCALE_IN_TIME	= 0.3f;		// seconds
		constexpr float	TURN_SMOOTHING_RATE	= 12.0f;	// per second
		constexpr float	LUNGE_TIME			= 0.25f;	// s, the same as DealDamageSystem's
		constexpr float	LUNGE_REACH			= 0.6f;		// x a body radius, at the peak
		constexpr float	DYING_SHRINK_TIME	= 0.25f;	// s, the same as LifeSystem's DYING_TIME
	}

	void AnimationSystem(eng::ecs::Registry& registry, float dt)
	{
		const float turnBlend = 1.0f - std::exp(-TURN_SMOOTHING_RATE * dt);

		registry.Each<EnemyLook, Transform, Life, EnemyKind>([&](eng::ecs::Entity, EnemyLook& look, Transform& transform, Life& life, EnemyKind& kind)
		{
			life.hitFlash = std::max(0.0f, life.hitFlash - dt);

			look.spawnTime = std::min(look.spawnTime + dt, SPAWN_SCALE_IN_TIME);
			const float t = look.spawnTime / SPAWN_SCALE_IN_TIME;
			look.scale = 1.0f - (1.0f - t) * (1.0f - t);	// ease-out
			if (life.dyingTime > 0.0f)
				look.scale *= std::max(0.0f, 1.0f - life.dyingTime / DYING_SHRINK_TIME);	// death: shrinks to nothing

			// The lunge: the body goes out and back along forward (a half sine)
			look.lungeLeft = std::max(0.0f, look.lungeLeft - dt);
			const float lungeT = 1.0f - look.lungeLeft / LUNGE_TIME;
			look.lunge = look.lungeLeft > 0.0f ? LUNGE_REACH * GetArchetype(kind.type).bodyRadius * std::sin(PI * lungeT) : 0.0f;

			look.renderRotation = QuaternionSlerp(look.renderRotation, transform.rotation, turnBlend);
		});
	}
}
