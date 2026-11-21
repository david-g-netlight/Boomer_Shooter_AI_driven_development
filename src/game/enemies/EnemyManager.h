#pragma once
#include <eng.h>
#include "EnemyPool.h"
#include "RenderSystem.h"

namespace enemies
{
	/// <summary>
	/// Owns the enemy registry and the pool, and runs the enemy systems in their fixed order (plan §5.3).
	/// ArenaGame calls it from its FixedUpdate, SubmitRenderables and OnLevelLoaded.
	/// </summary>
	class EnemyManager
	{
	public:
		/// <summary>Registers the components, reserves the pool and makes the 9 materials. Call once, after the lit shader is set.</summary>
		void			Init();

		/// <summary>Removes every enemy. Called when a level loads.</summary>
		void			Reset();

		/// <summary>The fixed step (50 Hz). Only the release step runs for now; the systems come in later phases.</summary>
		void			FixedUpdate(float /*fixedDeltaTime*/);

		/// <summary>Every frame while simulating: spawn scale-in and turn smoothing.</summary>
		void			Animate(float deltaTime);

		/// <summary>Queues the enemies' draw commands for this frame.</summary>
		void			Submit(eng::RenderQueue& queue);

	private:
		eng::ecs::Registry	m_registry{ EnemyPool::CAPACITY };
		EnemyPool			m_pool;
		EnemyMaterials		m_materials;
	};
}
