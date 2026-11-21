#include "Systems.h"
#include <raymath.h>
#include <cmath>
#include "../EnemyComponents.h"

namespace enemies
{
	namespace
	{
		constexpr float	BOB_AMPLITUDE		= 0.25f;						// m
		constexpr float	BOB_PERIOD			= 2.5f;							// s
		constexpr float	IDLE_EASE_RATE		= 4.0f;							// per second: horizontal speed to 0
		constexpr uint32_t WALLS			= eng::Layer::Ground | eng::Layer::Prop;
		constexpr float	MIN_ROOM			= 2.0f;							// m, the candidate must have this much free
		constexpr int	ROOM_TRIES			= 8;

		/// The first wall along `direction` within `distance`, or nothing
		std::optional<eng::RaycastHit> WallWithin(const SystemContext& ctx, const Vector3& origin, const Vector3& direction, float distance)
		{
			return ctx.physics.Raycast(origin, direction, distance, WALLS);
		}

		/// A direction has room when no wall is in the way for MIN_ROOM past the radius, and the end stays in the height band
		bool HasRoom(const SystemContext& ctx, const Vector3& position, const Vector3& direction, float radius)
		{
			const float length = radius + MIN_ROOM;
			if (WallWithin(ctx, position, direction, length))
				return false;

			const float endY = position.y + direction.y * length;
			return endY >= ctx.bounds.min.y && endY <= ctx.bounds.max.y;
		}
	}

	void IdleFloatSystem(eng::ecs::Registry& registry, const SystemContext& /*ctx*/, float dt)
	{
		if (dt <= 0.0f)
			return;

		const float omega = 2.0f * PI / BOB_PERIOD;
		const float easeBlend = 1.0f - std::exp(-IDLE_EASE_RATE * dt);

		registry.Each<Brain, Transform, Motion>([&](eng::ecs::Entity, Brain& brain, Transform& transform, Motion& motion)
		{
			if (brain.state != AiState::Idle)
				return;

			// Written as a velocity. The vertical term also pulls the enemy onto the bob centre, so it does not drift
			const float targetY = brain.baseY + BOB_AMPLITUDE * std::sin(omega * brain.stateTime);
			motion.velocity.x *= 1.0f - easeBlend;
			motion.velocity.z *= 1.0f - easeBlend;
			motion.velocity.y = BOB_AMPLITUDE * omega * std::cos(omega * brain.stateTime) + (targetY - transform.position.y) / dt;
		});
	}

	void WanderSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float /*dt*/)
	{
		registry.Each<Brain, Transform, Motion, Wander, CollisionSphere>([&](eng::ecs::Entity, Brain& brain, Transform& transform, Motion& motion, Wander& wander, CollisionSphere& sphere)
		{
			if (brain.state != AiState::Wander)
				return;

			// 1. Move along forward
			motion.velocity = Vector3Scale(motion.forward, wander.speed);

			// 2. One probe ahead: no wall within the probe, nothing to do
			const float radius = sphere.radius;
			const std::optional<eng::RaycastHit> wall = WallWithin(ctx, transform.position, motion.forward, radius + wander.probeDistance);
			if (!wall)
				return;

			// 3-4. A wall is near: try random directions, up to ROOM_TRIES, that have room; else reflect off the wall
			Vector3 candidate = motion.forward;
			bool found = false;
			for (int tries = 0; tries < ROOM_TRIES && !found; ++tries)
			{
				candidate = RandomUnitDirection(wander.rngState);
				found = HasRoom(ctx, transform.position, candidate, radius);
			}
			if (!found)
				candidate = Vector3Reflect(motion.forward, wall->normal);

			// 5. Snap: the simulation turns at once, the AnimationSystem smooths the render rotation
			motion.forward = candidate;
			transform.rotation = eng::LookRotation(candidate);
		});
	}
}
