#pragma once
#include <eng.h>
#include "EnemyPool.h"
#include "RenderSystem.h"
#include "systems/Systems.h"
#include "EnemySpawnPoint.h"
#include "WaveDirector.h"
#include "player/HitscanTargets.h"

struct ShotFiredEvent;

namespace enemies
{
	/// <summary>
	/// Owns the enemy registry and the pool, and runs the enemy systems in their fixed order (plan §5.3).
	/// ArenaGame calls it from its FixedUpdate and SubmitRenderables, and resets it in LoadLevelFile (the game's loader).
	/// </summary>
	class EnemyManager : public HitscanTargets
	{
	public:
		/// <summary>Registers the components, reserves the pool and makes the 9 materials. Call once, after the lit shader is set.</summary>
		void			Init();

		/// <summary>
		/// The player's chest, whether they are alive, and whether the game is Playing. ArenaGame sets it before every
		/// fixed step. Enemies only attack a living player in a Playing game, and the waves only run then.
		/// </summary>
		void			SetPlayer(const Vector3& chest, bool alive, bool isPlaying);

		/// <summary>
		/// Reads the world position of every enabled EnemySpawnPoint actor of the level. With none, the 4 upper corners
		/// of the arena bounds are used and a warning is logged. Call after SetArena.
		/// </summary>
		void			CollectSpawnPoints(const eng::Level& level);

		/// <summary>Stops listening to the shots. Call before the engine shuts down.</summary>
		void			Shutdown();

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

		/// <summary>
		/// A ray-sphere test against every living enemy that is not dying (20 enemies, one quadratic each); the nearest wins (§9.1).
		/// </summary>
		std::optional<HitscanHit>	Raycast(const Vector3& origin, const Vector3& dir, float maxDistance) const override;

		/// <summary>The ai_debug and ai_freeze cvars point here.</summary>
		struct DebugSettings
		{
			bool	draw	= false;	// ai_debug: probes and state labels
			bool	freeze	= false;	// ai_freeze: systems 2-8 skipped, the enemies hang in the air
		};

		DebugSettings&				GetDebugSettings()	{ return m_debug; }

		/// <summary>For the console and tests. Gameplay code goes through the pool.</summary>
		EnemyPool&					GetPool()		{ return m_pool; }
		WaveDirector&				GetWaves()		{ return m_waves; }
		eng::ecs::Registry&			GetRegistry()	{ return m_registry; }

	private:
		static constexpr float	HIT_FLASH_TIME	= 0.1f;	// s of the white flash after a hit

		void					OnShot(const ShotFiredEvent& shot);

		eng::EventBus::Token	m_shotToken	= 0;
		eng::ecs::Registry	m_registry{ EnemyPool::CAPACITY };
		ArenaBounds			m_arena;
		Vector3				m_playerTarget	= { 0.0f, 0.0f, 0.0f };
		bool				m_playerAlive	= false;
		bool				m_isPlaying		= false;
		DebugSettings		m_debug;
		EnemyPool			m_pool;
		WaveDirector		m_waves;
		EnemyMaterials		m_materials;
	};
}
