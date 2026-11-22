#include "Systems.h"
#include <raymath.h>
#include <algorithm>
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
		constexpr float	AWAY_FROM_WALL		= 0.5f;							// dot(new direction, wall normal): at most 60 degrees from the normal
		constexpr float	AVOID_PUSH_SPEED	= 3.0f;							// m/s, a full overlap pushes this hard
		constexpr float	AVOID_GAP			= 0.5f;							// m, the space kept between two enemies

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
		registry.Each<Brain, Transform, Motion, Wander, CollisionSphere, Alert>([&](eng::ecs::Entity, Brain& brain, Transform& transform, Motion& motion, Wander& wander, CollisionSphere& sphere, Alert& alert)
		{
			if (brain.state != AiState::Wander)
				return;

			// 0. A raised Alert: turn toward where the player stood when they shot (E-D15), once
			if (alert.pending)
			{
				const Vector3 toShooter = Vector3Subtract(alert.shotFrom, transform.position);
				if (Vector3LengthSqr(toShooter) > 1.0e-4f)
				{
					motion.forward = Vector3Normalize(toShooter);
					transform.rotation = eng::LookRotation(motion.forward);
				}
				alert.pending = false;
			}

			// 1. Move along forward
			motion.velocity = Vector3Scale(motion.forward, wander.speed);

			// 2. One probe ahead: no wall within the probe, nothing to do
			const float radius = sphere.radius;
			const std::optional<eng::RaycastHit> wall = WallWithin(ctx, transform.position, motion.forward, radius + wander.probeDistance);
			if (!wall)
				return;

			// 3-4. A wall is near: try random directions, up to ROOM_TRIES, that lead away from the wall and have room;
			// else reflect off the wall. Away matters: a direction along the wall passes the room check, and the enemy
			// would then slide along the wall without the probe ever firing again.
			Vector3 candidate = motion.forward;
			bool found = false;
			for (int tries = 0; tries < ROOM_TRIES && !found; ++tries)
			{
				candidate = RandomUnitDirection(wander.rngState);
				found = Vector3DotProduct(candidate, wall->normal) >= AWAY_FROM_WALL
					&& HasRoom(ctx, transform.position, candidate, radius);
			}
			if (!found)
				candidate = Vector3Reflect(motion.forward, wall->normal);

			// 5. Snap: the simulation turns at once, the AnimationSystem smooths the render rotation
			motion.forward = candidate;
			transform.rotation = eng::LookRotation(candidate);
		});
	}

	void AvoidOthersSystem(eng::ecs::Registry& registry, const SystemContext& /*ctx*/, float /*dt*/)
	{
		// Pairs of the CollisionSphere entities (the span is not changed while this runs)
		const auto entities = registry.Pool<CollisionSphere>().Entities();
		for (size_t i = 0; i < entities.size(); ++i)
		{
			for (size_t j = i + 1; j < entities.size(); ++j)
			{
				Transform* ti = registry.Get<Transform>(entities[i]);
				Transform* tj = registry.Get<Transform>(entities[j]);
				Motion* mi = registry.Get<Motion>(entities[i]);
				Motion* mj = registry.Get<Motion>(entities[j]);
				AvoidOthers* ai = registry.Get<AvoidOthers>(entities[i]);
				AvoidOthers* aj = registry.Get<AvoidOthers>(entities[j]);
				CollisionSphere* si = registry.Get<CollisionSphere>(entities[i]);
				CollisionSphere* sj = registry.Get<CollisionSphere>(entities[j]);
				if (!ti || !tj || !mi || !mj || !ai || !aj || !si || !sj)
					continue;

				const float minDist = si->radius + sj->radius + AVOID_GAP;
				const Vector3 offset = Vector3Subtract(tj->position, ti->position);
				const float dist = Vector3Length(offset);
				if (dist >= minDist)
					continue;

				// Pushed apart along the line between the centres, half each, scaled by the overlap
				const Vector3 dir = dist > 1.0e-4f ? Vector3Scale(offset, 1.0f / dist) : Vector3{ 1.0f, 0.0f, 0.0f };
				const float push = (minDist - dist) / minDist * AVOID_PUSH_SPEED * 0.5f;
				mi->velocity = Vector3Subtract(mi->velocity, Vector3Scale(dir, push * ai->strength));
				mj->velocity = Vector3Add(mj->velocity, Vector3Scale(dir, push * aj->strength));
			}
		}
	}

	void MovementSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt)
	{
		// 1. Move, sweeping for walls: stop at the wall, then slide along it for the rest of the step
		registry.Each<Transform, Motion, CollisionSphere>([&](eng::ecs::Entity, Transform& transform, Motion& motion, CollisionSphere& sphere)
		{
			const Vector3 delta = Vector3Scale(motion.velocity, dt);
			const float length = Vector3Length(delta);
			if (length > 0.0f)
			{
				const Vector3 dir = Vector3Scale(delta, 1.0f / length);
				const std::optional<eng::RaycastHit> wall = WallWithin(ctx, transform.position, dir, length + sphere.radius);
				if (!wall)
				{
					transform.position = Vector3Add(transform.position, delta);
				}
				else
				{
					const float travel = std::max(0.0f, wall->distance - sphere.radius);
					transform.position = Vector3Add(transform.position, Vector3Scale(dir, travel));

					motion.velocity = Vector3Subtract(motion.velocity, Vector3Scale(wall->normal, Vector3DotProduct(motion.velocity, wall->normal)));
					const float rest = (length - travel) / length;
					transform.position = Vector3Add(transform.position, Vector3Scale(motion.velocity, dt * rest));
				}
			}

			// 2. The height band (E-D3) and the arena's walls, as a safety net
			const float radius = sphere.radius;
			if (transform.position.y < ctx.bounds.min.y + radius || transform.position.y > ctx.bounds.max.y - radius)
			{
				transform.position.y = std::clamp(transform.position.y, ctx.bounds.min.y + radius, ctx.bounds.max.y - radius);
				motion.velocity.y = 0.0f;
			}
			transform.position.x = std::clamp(transform.position.x, ctx.bounds.min.x + radius, ctx.bounds.max.x - radius);
			transform.position.z = std::clamp(transform.position.z, ctx.bounds.min.z + radius, ctx.bounds.max.z - radius);
		});

		// 3. Overlaps left after the moves: each enemy takes half of the overlap
		const auto entities = registry.Pool<CollisionSphere>().Entities();
		for (size_t i = 0; i < entities.size(); ++i)
		{
			for (size_t j = i + 1; j < entities.size(); ++j)
			{
				Transform* ti = registry.Get<Transform>(entities[i]);
				Transform* tj = registry.Get<Transform>(entities[j]);
				CollisionSphere* si = registry.Get<CollisionSphere>(entities[i]);
				CollisionSphere* sj = registry.Get<CollisionSphere>(entities[j]);
				if (!ti || !tj || !si || !sj)
					continue;

				const float minDist = si->radius + sj->radius;
				const Vector3 offset = Vector3Subtract(tj->position, ti->position);
				const float dist = Vector3Length(offset);
				if (dist >= minDist)
					continue;

				const Vector3 dir = dist > 1.0e-4f ? Vector3Scale(offset, 1.0f / dist) : Vector3{ 1.0f, 0.0f, 0.0f };
				const Vector3 half = Vector3Scale(dir, (minDist - dist) * 0.5f);
				ti->position = Vector3Subtract(ti->position, half);
				tj->position = Vector3Add(tj->position, half);
			}
		}
	}
}
