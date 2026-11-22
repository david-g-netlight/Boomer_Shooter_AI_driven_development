#pragma once
#include <raylib.h>
#include <array>
#include <vector>
#include <eng.h>
#include "EnemyPool.h"
#include "EnemyTypes.h"

namespace enemies
{
	/// <summary>Published when a wave starts spawning. `index` is the wave number from 1.</summary>
	struct WaveStartedEvent
	{
		int	index;
		int	count;
	};

	/// <summary>Published once, when the last enemy of the last wave is gone (ArenaGame turns it into GameState::WinGame).</summary>
	struct WavesClearedEvent {};

	struct WaveEntry	{ EnemyType type; int count; };
	struct Wave			{ std::array<WaveEntry, 3> entries; float spawnInterval; };	// seconds between two spawns

	/// <summary>
	/// Spawns the three waves of the v1 table (plan §6.2, E-D4):
	/// Waiting (2 s) -> Spawning (one enemy every spawnInterval, round robin over the spawn points) -> Fighting (until
	/// nobody is alive) -> the next wave, or WavesClearedEvent after the last. The pool is the source of truth for
	/// "alive". A spawn point is skipped while the player is closer than 4 m or another enemy closer than 1.5 m.
	/// </summary>
	class WaveDirector
	{
	public:
		enum class State { Waiting, Spawning, Fighting, Cleared };

		static constexpr int	WAVE_COUNT			= 3;
		static constexpr float	WAIT_TIME			= 2.0f;		// s before a wave
		static constexpr float	MIN_PLAYER_DISTANCE	= 4.0f;		// m
		static constexpr float	MIN_ENEMY_DISTANCE	= 1.5f;		// m

		void	Init(EnemyPool& pool, eng::ecs::Registry& registry);

		/// <summary>Back to the start: wave 1 waits 2 s. Called when a level loads or restarts. Keeps the spawn points and the on / off switch.</summary>
		void	Reset();

		/// <summary>The positions the enemies appear at. Empty: the caller should pass the fallback corners (see EnemyManager).</summary>
		void	SetSpawnPoints(std::vector<Vector3> points);

		/// <summary>One fixed step. Does nothing while the director is off or the wave table is done.</summary>
		void	Update(float dt, const Vector3& playerPosition);

		//////////////////////////////////////////
		/// Console and HUD
		//////////////////////////////////////////
		void	StartWave(int number);			// 1..WAVE_COUNT; enemies that are alive stay
		void	SkipWave();						// kills every alive enemy and stops the wave's spawning
		void	SetEnabled(bool enabled)		{ m_enabled = enabled; }
		bool	IsEnabled() const				{ return m_enabled; }

		int		GetWaveNumber() const			{ return m_wave + 1; }
		State	GetState() const				{ return m_state; }
		int		GetEnemiesLeft() const;			// still to spawn + alive
		size_t	GetSpawnPointCount() const		{ return m_points.size(); }

		/// <summary>The enemies in a wave of the table.</summary>
		static int	GetWaveSize(int waveIndex);

	private:
		bool			TrySpawn(const Vector3& playerPosition);
		EnemyType		TypeOfSpawn(int spawnIndex) const;
		void			BeginWave(int waveIndex);

		EnemyPool*				m_pool		= nullptr;
		eng::ecs::Registry*		m_registry	= nullptr;
		std::vector<Vector3>	m_points;
		size_t					m_nextPoint	= 0;
		State					m_state		= State::Waiting;
		int						m_wave		= 0;		// index in the table
		int						m_spawned	= 0;		// of this wave
		float					m_timer		= WAIT_TIME;
		uint32_t				m_seed		= 0x1234567u;
		bool					m_enabled	= true;
	};
}
