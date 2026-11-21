#include "EnemyBrain.h"
#include <raymath.h>
#include "../EnemyComponents.h"

namespace enemies
{
	namespace
	{
		void OnEnter(eng::ecs::Registry& registry, eng::ecs::Entity e, Brain& brain, AiState state)
		{
			Transform* transform = registry.Get<Transform>(e);
			Motion* motion = registry.Get<Motion>(e);
			Wander* wander = registry.Get<Wander>(e);
			if (!transform || !motion)
				return;

			switch (state)
			{
				case AiState::Idle:
					brain.baseY = transform->position.y;
					break;
				case AiState::Wander:
					if (wander)
					{
						motion->forward = RandomUnitDirection(wander->rngState);
						transform->rotation = eng::LookRotation(motion->forward);
					}
					break;
				default:
					break;
			}
		}
	}

	AiState NextState(const Brain& brain)
	{
		switch (brain.state)
		{
			case AiState::Idle:
				return brain.stateTime >= brain.idleDuration ? AiState::Wander : AiState::Idle;
			case AiState::Wander:
			default:
				return brain.state;
		}
	}

	void BrainSystem(eng::ecs::Registry& registry, const SystemContext& /*ctx*/, float dt)
	{
		registry.Each<Brain>([&](eng::ecs::Entity e, Brain& brain)
		{
			brain.stateTime += dt;

			const AiState next = NextState(brain);
			if (next == brain.state)
				return;

			brain.state = next;
			brain.stateTime = 0.0f;
			OnEnter(registry, e, brain, next);
		});
	}
}
