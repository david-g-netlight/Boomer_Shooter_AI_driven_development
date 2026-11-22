#pragma once
#include <eng.h>
#include "../EnemyComponents.h"
#include "../systems/Systems.h"

namespace enemies
{
	/// <summary>
	/// The transition rule (§7.1) as a pure function: data in, state out. The checks run in the order of the table:
	/// hp, player dead, trigger radius, Alert, idle time; Chase and Attack end at the lose radius (1.2 × trigger)
	/// and the attack exit range (1.2 × attack range), so nothing flickers at an edge (E-D6).
	/// </summary>
	AiState NextState(const Brain& brain, const ScanForPlayer& scan, const DealDamage& attack, const Life& life, const Alert& alert, bool playerAlive);

	/// <summary>
	/// stateTime += dt, then the transition. On a change: stateTime = 0 and the entry action of the new state
	/// (Idle stores the bob centre, Wander picks a first direction, Dying stops the enemy). Writes only Brain, and on entry Transform and Motion.
	/// </summary>
	void BrainSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt);
}
