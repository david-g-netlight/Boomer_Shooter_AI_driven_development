#include "EnemyDebugDraw.h"
#include <raymath.h>
#include <cstdio>
#include "EnemyComponents.h"

namespace enemies
{
	namespace
	{
		const char* StateName(AiState state)
		{
			switch (state)
			{
				case AiState::Idle:		return "Idle";
				case AiState::Wander:	return "Wander";
				case AiState::Chase:	return "Chase";
				case AiState::Attack:	return "Attack";
				case AiState::Dying:	return "Dying";
			}
			return "?";
		}
	}

	void DrawEnemyDebug(eng::ecs::Registry& registry, eng::DebugDraw& debug)
	{
		registry.Each<Brain, Transform, Motion, CollisionSphere>([&](eng::ecs::Entity e, Brain& brain, Transform& transform, Motion& motion, CollisionSphere& sphere)
		{
			const Vector3 center = transform.position;

			// The probe: the ray Wander casts, from the surface along forward
			const Wander* wander = registry.Get<Wander>(e);
			if (brain.state == AiState::Wander && wander)
			{
				const float length = sphere.radius + wander->probeDistance;
				debug.Line(center, Vector3Add(center, Vector3Scale(motion.forward, length)), YELLOW);
			}

			char text[48];
			std::snprintf(text, sizeof(text), "%s %.1fs", StateName(brain.state), static_cast<double>(brain.stateTime));
			debug.Label(Vector3Add(center, { 0.0f, sphere.radius + 0.3f, 0.0f }), text, WHITE);
		});
	}
}
