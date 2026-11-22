#include "Systems.h"
#include <raymath.h>
#include <cmath>
#include "../EnemyComponents.h"

namespace enemies
{
	void ScanSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float /*dt*/)
	{
		registry.Each<Transform, ScanForPlayer>([&](eng::ecs::Entity, Transform& transform, ScanForPlayer& scan)
		{
			const Vector3 toPlayer = Vector3Subtract(ctx.playerTarget, transform.position);
			scan.distSq = Vector3LengthSqr(toPlayer);
			if (scan.distSq < scan.loseRadiusSq && scan.distSq > 1.0e-6f)
				scan.toPlayerDir = Vector3Scale(toPlayer, 1.0f / std::sqrt(scan.distSq));
		});
	}
}
