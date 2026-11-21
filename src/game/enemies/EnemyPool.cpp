#include "EnemyPool.h"
#include <raymath.h>
#include <algorithm>
#include <array>
#include "EnemyComponents.h"

namespace enemies
{
	void EnemyPool::Init(eng::ecs::Registry& registry)
	{
		m_registry = &registry;
		registry.RegisterComponent<Transform>();
		registry.RegisterComponent<Motion>();
		registry.RegisterComponent<Brain>();
		registry.RegisterComponent<ScanForPlayer>();
		registry.RegisterComponent<Wander>();
		registry.RegisterComponent<ChasePlayer>();
		registry.RegisterComponent<AvoidOthers>();
		registry.RegisterComponent<DealDamage>();
		registry.RegisterComponent<Life>();
		registry.RegisterComponent<Alert>();
		registry.RegisterComponent<CollisionSphere>();
		registry.RegisterComponent<EnemyKind>();
		registry.RegisterComponent<EnemyLook>();
	}

	eng::ecs::Entity EnemyPool::Spawn(EnemyType type, const Vector3& position, const Vector3& forward, uint32_t seed)
	{
		eng::ecs::Entity e = m_registry->Create();
		if (e == eng::ecs::INVALID_ENTITY)
		{
			eng::Log::Warning("EnemyPool: pool full (%d enemies), spawn skipped", CAPACITY);
			return e;
		}

		const EnemyArchetype& archetype = GetArchetype(type);
		const float speed = archetype.wanderSpeed;

		m_registry->Add<EnemyKind>(e, EnemyKind{ type });
		m_registry->Add<Transform>(e, Transform{ position });
		m_registry->Add<Motion>(e, Motion{ Vector3Zero(), Vector3Normalize(forward), DEG2RAD * archetype.turnRateDeg });
		m_registry->Add<Brain>(e, Brain{});
		m_registry->Add<ScanForPlayer>(e, ScanForPlayer{
			archetype.triggerRadius * archetype.triggerRadius,
			archetype.loseRadius * archetype.loseRadius });
		m_registry->Add<Wander>(e, Wander{ speed, std::max(1.0f, speed * 0.4f), seed == 0 ? 1u : seed });
		m_registry->Add<ChasePlayer>(e, ChasePlayer{ archetype.chaseSpeed });
		m_registry->Add<AvoidOthers>(e, AvoidOthers{ archetype.avoidStrength });
		m_registry->Add<DealDamage>(e, DealDamage{
			archetype.damage,
			archetype.attackRange * archetype.attackRange,
			archetype.attackExitRange * archetype.attackExitRange });
		m_registry->Add<Life>(e, Life{ archetype.hp, archetype.hp });
		m_registry->Add<Alert>(e, Alert{});
		m_registry->Add<CollisionSphere>(e, CollisionSphere{ archetype.bodyRadius });
		m_registry->Add<EnemyLook>(e, EnemyLook{ 0.0f });
		return e;
	}

	void EnemyPool::FlushReleased()
	{
		// Collect first: Destroy changes the pools that Each walks
		std::array<eng::ecs::Entity, CAPACITY> released;
		size_t count = 0;
		m_registry->Each<Life>([&](eng::ecs::Entity e, Life& life)
		{
			if (life.isDead)
				released[count++] = e;
		});

		for (size_t i = 0; i < count; ++i)
			m_registry->Destroy(released[i]);
	}

	void EnemyPool::Clear()
	{
		m_registry->Clear();
	}

	int EnemyPool::GetAliveCount() const
	{
		return static_cast<int>(m_registry->GetAliveCount());
	}

	int EnemyPool::GetAliveCount(EnemyType type) const
	{
		int count = 0;
		m_registry->Each<EnemyKind>([&](eng::ecs::Entity, EnemyKind& kind)
		{
			if (kind.type == type)
				++count;
		});
		return count;
	}

	bool EnemyPool::IsFull() const
	{
		return m_registry->GetAliveCount() >= CAPACITY;
	}
}
