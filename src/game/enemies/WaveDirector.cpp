#include "WaveDirector.h"
#include <raymath.h>
#include <algorithm>
#include "EnemyComponents.h"

namespace enemies
{
	namespace
	{
		// v1 table, 20 enemies in total (E-D4)
		constexpr std::array<Wave, WaveDirector::WAVE_COUNT> WAVES = { {
			{ { { { EnemyType::Drifter, 7 }, { EnemyType::Stalker, 0 }, { EnemyType::Brute, 0 } } }, 0.6f },	// 7
			{ { { { EnemyType::Drifter, 5 }, { EnemyType::Stalker, 2 }, { EnemyType::Brute, 0 } } }, 0.5f },	// 7
			{ { { { EnemyType::Drifter, 3 }, { EnemyType::Stalker, 2 }, { EnemyType::Brute, 1 } } }, 0.5f },	// 6
		} };
	}

	void WaveDirector::Init(EnemyPool& pool, eng::ecs::Registry& registry)
	{
		m_pool		= &pool;
		m_registry	= &registry;
	}

	void WaveDirector::Reset()
	{
		m_state		= State::Waiting;
		m_wave		= 0;
		m_spawned	= 0;
		m_timer		= WAIT_TIME;
		m_nextPoint	= 0;
	}

	void WaveDirector::SetSpawnPoints(std::vector<Vector3> points)
	{
		m_points	= std::move(points);
		m_nextPoint	= 0;
	}

	int WaveDirector::GetWaveSize(int waveIndex)
	{
		int total = 0;
		for (const WaveEntry& entry : WAVES[static_cast<size_t>(waveIndex)].entries)
			total += entry.count;
		return total;
	}

	int WaveDirector::GetEnemiesLeft() const
	{
		if (m_state == State::Cleared)
			return 0;
		const int toSpawn = m_state == State::Fighting ? 0 : GetWaveSize(m_wave) - m_spawned;
		return toSpawn + m_pool->GetAliveCount();
	}

	EnemyType WaveDirector::TypeOfSpawn(int spawnIndex) const
	{
		int index = spawnIndex;
		for (const WaveEntry& entry : WAVES[static_cast<size_t>(m_wave)].entries)
		{
			if (index < entry.count)
				return entry.type;
			index -= entry.count;
		}
		return EnemyType::Drifter;
	}

	void WaveDirector::BeginWave(int waveIndex)
	{
		m_wave		= waveIndex;
		m_spawned	= 0;
		m_timer		= 0.0f;			// the first enemy comes at once
		m_state		= State::Spawning;
		eng::Engine::GetInstance().GetEvents().Publish(WaveStartedEvent{ m_wave + 1, GetWaveSize(m_wave) });
	}

	void WaveDirector::StartWave(int number)
	{
		BeginWave(std::clamp(number, 1, WAVE_COUNT) - 1);
	}

	void WaveDirector::SkipWave()
	{
		if (m_state == State::Cleared)
			return;

		m_spawned = GetWaveSize(m_wave);
		m_state = State::Fighting;
		m_registry->Each<Life>([](eng::ecs::Entity, Life& life) { life.hp = 0.0f; });
	}

	bool WaveDirector::TrySpawn(const Vector3& playerPosition)
	{
		if (m_points.empty() || m_pool->IsFull())
			return false;

		// Round robin: the next point that is clear of the player and of other enemies
		for (size_t tried = 0; tried < m_points.size(); ++tried)
		{
			const size_t index = (m_nextPoint + tried) % m_points.size();
			const Vector3& point = m_points[index];

			if (Vector3Distance(point, playerPosition) < MIN_PLAYER_DISTANCE)
				continue;

			bool isBlocked = false;
			m_registry->Each<Transform>([&](eng::ecs::Entity, Transform& transform)
			{
				if (Vector3Distance(point, transform.position) < MIN_ENEMY_DISTANCE)
					isBlocked = true;
			});
			if (isBlocked)
				continue;

			// Facing the player; the seed differs for every enemy
			Vector3 toPlayer = Vector3Subtract(playerPosition, point);
			toPlayer.y = 0.0f;
			const Vector3 forward = Vector3LengthSqr(toPlayer) > 1.0e-4f ? Vector3Normalize(toPlayer) : Vector3{ 0.0f, 0.0f, -1.0f };
			m_seed = m_seed * 1664525u + 1013904223u;
			m_pool->Spawn(TypeOfSpawn(m_spawned), point, forward, m_seed);
			m_nextPoint = index + 1;
			return true;
		}
		return false;
	}

	void WaveDirector::Update(float dt, const Vector3& playerPosition)
	{
		if (!m_enabled)
			return;

		switch (m_state)
		{
			case State::Waiting:
				m_timer -= dt;
				if (m_timer <= 0.0f)
					BeginWave(m_wave);
				break;

			case State::Spawning:
				m_timer -= dt;
				if (m_timer > 0.0f)
					break;
				// Blocked this tick (no free point): the timer stays at 0 and it tries again on the next tick
				if (TrySpawn(playerPosition))
				{
					++m_spawned;
					m_timer = WAVES[static_cast<size_t>(m_wave)].spawnInterval;
					if (m_spawned >= GetWaveSize(m_wave))
						m_state = State::Fighting;
				}
				break;

			case State::Fighting:
				if (m_pool->GetAliveCount() > 0)
					break;
				if (m_wave + 1 >= WAVE_COUNT)
				{
					m_state = State::Cleared;
					eng::Engine::GetInstance().GetEvents().Publish(WavesClearedEvent{});
				}
				else
				{
					++m_wave;
					m_spawned = 0;
					m_timer = WAIT_TIME;
					m_state = State::Waiting;
				}
				break;

			case State::Cleared:
				break;
		}
	}
}
