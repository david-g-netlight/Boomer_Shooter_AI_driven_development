#pragma once
#include <eng.h>

namespace enemies
{
	/// <summary>
	/// Presentation only (plan §8.2): the spawn scale-in (0.3 s, ease-out), the lunge, the death shrink, the hit flash timer and the render rotation that
	/// slerps toward Transform.rotation. Runs every frame while the game is simulating. It never writes the simulation.
	/// </summary>
	void AnimationSystem(eng::ecs::Registry& registry, float dt);
}
