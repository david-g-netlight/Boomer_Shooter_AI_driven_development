#pragma once
#include <eng.h>
#include "EnemyPool.h"

namespace enemies
{
	/// <summary>
	/// Owns the enemy registry and the pool, and runs the enemy systems in their fixed order (plan §5.3).
	/// ArenaGame calls it from its FixedUpdate, SubmitRenderables and OnLevelLoaded.
	/// </summary>
	class EnemyManager
	{
	public:
		/// <summary>Registers the components and reserves the pool. Call once, before the first level loads.</summary>
		void			Init();

		/// <summary>Removes every enemy. Called when a level loads.</summary>
		void			Reset();

		/// <summary>The fixed step (50 Hz). Only the release step runs for now; the systems come in later phases.</summary>
		void			FixedUpdate(float /*fixedDeltaTime*/);

		/// <summary>Queues the enemies' draw commands for this frame.</summary>
		void			Submit(eng::RenderQueue& /*queue*/) const;

	private:
		eng::ecs::Registry	m_registry{ EnemyPool::CAPACITY };
		EnemyPool			m_pool;
	};
}
