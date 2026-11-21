#pragma once
#include <eng.h>
#include "../EnemyComponents.h"
#include "../systems/Systems.h"

namespace enemies
{
	/// <summary>
	/// The transition rule (§7.1) as a pure function: data in, state out. E2 has Idle and Wander only.
	/// </summary>
	AiState NextState(const Brain& brain);

	/// <summary>
	/// stateTime += dt, then the transition. On a change: stateTime = 0 and the entry action of the new state
	/// (Idle stores the bob centre, Wander picks a first direction). Writes only Brain, and on entry Transform and Motion.
	/// </summary>
	void BrainSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt);
}
