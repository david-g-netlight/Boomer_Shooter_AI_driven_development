#pragma once
#include <eng.h>

namespace enemies
{
	/// <summary>
	/// ai_debug: the wall probe of each wandering enemy (yellow) and a state label with the time in the state.
	/// Immediate: call it every frame while the cvar is on.
	/// </summary>
	void DrawEnemyDebug(eng::ecs::Registry& registry, eng::DebugDraw& debug);
}
