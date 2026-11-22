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
			Life* life = registry.Get<Life>(e);
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
				case AiState::Dying:
					motion->velocity = { 0.0f, 0.0f, 0.0f };
					if (life)
						life->dyingTime = 0.0f;
					break;
				default:
					break;	// Chase and Attack need nothing on entry: ChaseSystem steers every tick
			}
		}
	}

	AiState NextState(const Brain& brain, const ScanForPlayer& scan, const DealDamage& attack, const Life& life, const Alert& alert, bool playerAlive)
	{
		if (brain.state == AiState::Dying || life.hp <= 0.0f)
			return AiState::Dying;

		switch (brain.state)
		{
			case AiState::Idle:
				if (playerAlive && scan.distSq < scan.triggerRadiusSq)
					return AiState::Chase;
				// A shot wakes an idle enemy at once (fake aggro, E-D15)
				return alert.pending || brain.stateTime >= brain.idleDuration ? AiState::Wander : AiState::Idle;
			case AiState::Wander:
				return playerAlive && scan.distSq < scan.triggerRadiusSq ? AiState::Chase : AiState::Wander;
			case AiState::Chase:
				if (!playerAlive || scan.distSq >= scan.loseRadiusSq)
					return AiState::Idle;
				return scan.distSq <= attack.rangeSq ? AiState::Attack : AiState::Chase;
			case AiState::Attack:
				if (!playerAlive || scan.distSq >= scan.loseRadiusSq)
					return AiState::Idle;
				return scan.distSq > attack.exitRangeSq ? AiState::Chase : AiState::Attack;
			default:
				return brain.state;
		}
	}

	void BrainSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt)
	{
		registry.Each<Brain, ScanForPlayer, DealDamage, Life, Alert>([&](eng::ecs::Entity e, Brain& brain, ScanForPlayer& scan, DealDamage& attack, Life& life, Alert& alert)
		{
			brain.stateTime += dt;

			const AiState next = NextState(brain, scan, attack, life, alert, ctx.playerAlive);
			if (next == brain.state)
				return;

			brain.state = next;
			brain.stateTime = 0.0f;
			OnEnter(registry, e, brain, next);
		});
	}
}
