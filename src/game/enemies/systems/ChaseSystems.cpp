#include "Systems.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include "../EnemyComponents.h"
#include "../EnemyEvents.h"

namespace enemies
{
	namespace
	{
		constexpr float	LUNGE_TIME	= 0.25f;	// s, out and back (the AnimationSystem plays it)

		/// `from` turned toward `to` (both unit vectors) by at most maxAngle radians
		Vector3 TurnToward(const Vector3& from, const Vector3& to, float maxAngle)
		{
			const float angle = std::acos(std::clamp(Vector3DotProduct(from, to), -1.0f, 1.0f));
			if (angle <= maxAngle)
				return to;

			Vector3 axis = Vector3CrossProduct(from, to);
			axis = Vector3LengthSqr(axis) > 1.0e-8f ? Vector3Normalize(axis) : Vector3{ 0.0f, 1.0f, 0.0f };	// straight behind: turn around the up axis
			return Vector3Normalize(Vector3RotateByAxisAngle(from, axis, maxAngle));
		}
	}

	void ChaseSystem(eng::ecs::Registry& registry, const SystemContext& /*ctx*/, float dt)
	{
		registry.Each<Brain, Transform, Motion, ChasePlayer, ScanForPlayer, Alert>([&](eng::ecs::Entity, Brain& brain, Transform& transform, Motion& motion, ChasePlayer& chase, ScanForPlayer& scan, Alert& alert)
		{
			if (brain.state != AiState::Chase && brain.state != AiState::Attack)
				return;

			alert.pending = false;

			motion.forward = TurnToward(motion.forward, scan.toPlayerDir, motion.turnRateRad * dt);
			transform.rotation = eng::LookRotation(motion.forward);
			motion.velocity = brain.state == AiState::Chase ? Vector3Scale(motion.forward, chase.speed) : Vector3{ 0.0f, 0.0f, 0.0f };
		});
	}

	void DealDamageSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt)
	{
		registry.Each<Brain, DealDamage, Transform, EnemyLook>([&](eng::ecs::Entity, Brain& brain, DealDamage& attack, Transform& transform, EnemyLook& look)
		{
			attack.cooldownLeft = std::max(0.0f, attack.cooldownLeft - dt);	// keeps ticking outside Attack (E-D5)
			if (brain.state != AiState::Attack || attack.cooldownLeft > 0.0f)
				return;

			ctx.events.Publish(PlayerHitEvent{ attack.damage, transform.position });
			attack.cooldownLeft = attack.cooldown;
			look.lungeLeft = LUNGE_TIME;
		});
	}
}
