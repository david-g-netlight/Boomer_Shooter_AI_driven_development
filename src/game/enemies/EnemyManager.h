#pragma once
#include <eng.h>
#include "EnemyPool.h"
#include "RenderSystem.h"
#include "systems/Systems.h"

namespace enemies
{
	/// <summary>
	/// Owns the enemy registry and the pool, and runs the enemy systems in their fixed order (plan §5.3).
	/// ArenaGame calls it from its FixedUpdate and SubmitRenderables, and resets it in LoadLevelFile (the game's loader).
	/// </summary>
	class EnemyManager
	{
	public:
		/// <summary>Registers the components, reserves the pool and makes the 9 materials. Call once, after the lit shader is set.</summary>
		void			Init();

		/// <summary>Removes every enemy. Called when a level loads.</summary>
		void			Reset();

		/// <summary>
		/// The walkable box from the "Arena" actor's world box, shrunk by 1 m (§6.2). A null actor means no limits.
		/// </summary>
		void			SetArena(const eng::GameActor* arena);

		/// <summary>The fixed step (50 Hz). Only the release step runs for now; the systems come in later phases.</summary>
		void			FixedUpdate(float /*fixedDeltaTime*/);

		/// <summary>Every frame while simulating: spawn scale-in and turn smoothing.</summary>
		void			Animate(float deltaTime);

		/// <summary>Queues the enemies' draw commands for this frame.</summary>
		void			Submit(eng::RenderQueue& queue);

		/// <summary>The ai_debug and ai_freeze cvars point here.</summary>
		struct DebugSettings
		{
			bool	draw	= false;	// ai_debug: probes and state labels
			bool	freeze	= false;	// ai_freeze: systems 2-8 skipped, the enemies hang in the air
		};

		DebugSettings&				GetDebugSettings()	{ return m_debug; }

		/// <summary>For the console and tests. Gameplay code goes through the pool.</summary>
		EnemyPool&					GetPool()		{ return m_pool; }
		eng::ecs::Registry&			GetRegistry()	{ return m_registry; }

	private:
		eng::ecs::Registry	m_registry{ EnemyPool::CAPACITY };
		ArenaBounds			m_arena;
		DebugSettings		m_debug;
		EnemyPool			m_pool;
		EnemyMaterials		m_materials;
	};
}
