#pragma once
#include <raylib.h>
#include <cstdint>
#include <eng.h>
#include "EnemyTypes.h"

namespace enemies
{
	/// <summary>
	/// Fixed-capacity enemy storage on top of the ECS registry (plan §6.1). Slots are reused through the registry's
	/// free list, and the memory is reserved in Init, so there is no allocation while enemies are spawned or removed.
	/// The pool is the only place that creates or destroys enemy entities.
	/// </summary>
	class EnemyPool
	{
	public:
		static constexpr uint16_t CAPACITY = 32;

		/// <summary>Registers every enemy component with the registry. Call once, before any Spawn.</summary>
		void					Init(eng::ecs::Registry& registry);

		/// <summary>Starts in Idle with the archetype's values. An invalid Entity (and a log warning) when the pool is full.</summary>
		eng::ecs::Entity		Spawn(EnemyType type, const Vector3& position, const Vector3& forward, uint32_t seed);

		/// <summary>Destroys the entities whose Life::isDead is set.</summary>
		void					FlushReleased();

		void					Clear();
		int						GetAliveCount() const;
		int						GetAliveCount(EnemyType type) const;
		bool					IsFull() const;

	private:
		eng::ecs::Registry*		m_registry = nullptr;
	};
}
