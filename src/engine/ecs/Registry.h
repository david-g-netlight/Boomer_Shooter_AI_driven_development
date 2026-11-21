#pragma once
#include "ComponentPool.h"
#include "Entity.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace eng::ecs
{
	/// <summary>
	/// Sequential index per component type (the same idea as EventBus::TypeId, but small enough for an array).
	/// </summary>
	inline uint8_t NextComponentTypeIndex()
	{
		static uint8_t next = 0;
		return next++;
	}

	template<typename T>
	uint8_t ComponentTypeIndex()
	{
		static const uint8_t index = NextComponentTypeIndex();
		return index;
	}

	/// <summary>
	/// Entities and their components. Storage only: no systems, no events, no serialization.
	///
	///   Registry registry(32);
	///   registry.RegisterComponent<Position>();   // at Init, never in the hot path
	///   Entity e = registry.Create();
	///   registry.Add(e, Position{ 1, 2, 3 });
	///   registry.Each<Position>([](Entity, Position& p) { ... });
	/// </summary>
	class Registry
	{
	public:
		explicit Registry(uint16_t capacity)
			: m_generations(capacity, 0)
			, m_alive(capacity, false)
		{
			m_freeList.reserve(capacity);
			for (uint16_t i = capacity; i > 0; --i)
				m_freeList.push_back(static_cast<uint16_t>(i - 1));
		}

		template<typename T>
		void RegisterComponent()
		{
			uint8_t index = ComponentTypeIndex<T>();
			if (m_pools.size() <= index)
				m_pools.resize(index + 1);

			assert(!m_pools[index] && "component registered twice");
			m_pools[index] = std::make_unique<ComponentPool<T>>(GetCapacity());
		}

		/// <summary>An invalid Entity if the registry is full.</summary>
		Entity Create()
		{
			if (m_freeList.empty())
				return INVALID_ENTITY;

			uint16_t index = m_freeList.back();
			m_freeList.pop_back();
			m_alive[index] = true;
			++m_aliveCount;
			return Entity{ index, m_generations[index] };
		}

		/// <summary>Removes every component of e and frees its slot. Stale or invalid ids are ignored.</summary>
		void Destroy(Entity e)
		{
			if (!IsAlive(e))
				return;

			for (auto& pool : m_pools)
			{
				if (pool)
					pool->Remove(e);
			}

			m_alive[e.index] = false;
			++m_generations[e.index];
			m_freeList.push_back(e.index);
			--m_aliveCount;
		}

		bool IsAlive(Entity e) const
		{
			return e.index < m_alive.size() && m_alive[e.index] && m_generations[e.index] == e.generation;
		}

		/// <summary>Destroys every entity. Keeps all memory.</summary>
		void Clear()
		{
			for (uint16_t i = 0; i < GetCapacity(); ++i)
			{
				if (m_alive[i])
					Destroy(Entity{ i, m_generations[i] });
			}
		}

		template<typename T>
		T& Add(Entity e, const T& value = {})
		{
			assert(IsAlive(e) && "Add on a dead entity");
			return Pool<T>().Add(e, value);
		}

		template<typename T>
		void Remove(Entity e)
		{
			Pool<T>().Remove(e);
		}

		/// <summary>Null if e is dead or does not have T.</summary>
		template<typename T>
		T* Get(Entity e)
		{
			return IsAlive(e) ? Pool<T>().Get(e) : nullptr;
		}

		template<typename T>
		ComponentPool<T>& Pool()
		{
			uint8_t index = ComponentTypeIndex<T>();
			assert(index < m_pools.size() && m_pools[index] && "component not registered");
			return *static_cast<ComponentPool<T>*>(m_pools[index].get());
		}

		/// <summary>
		/// f(Entity, A&amp;, Rest&amp;...) for every entity that has all of them. It walks the pool of A
		/// (put the rarest component first) and looks up the others through their sparse arrays.
		/// f must not add or remove components of A.
		/// </summary>
		template<typename A, typename... Rest, typename F>
		void Each(F&& f)
		{
			ComponentPool<A>& poolA = Pool<A>();
			for (size_t i = 0; i < poolA.Size(); ++i)
			{
				Entity e = poolA.Entities()[i];
				if ((Pool<Rest>().Has(e) && ...))
					f(e, poolA.Data()[i], *Pool<Rest>().Get(e)...);
			}
		}

		size_t GetAliveCount() const
		{
			return m_aliveCount;
		}

		uint16_t GetCapacity() const
		{
			return static_cast<uint16_t>(m_generations.size());
		}

	private:
		std::vector<uint16_t>							m_generations;	// per slot, bumped on every Destroy
		std::vector<bool>								m_alive;		// per slot
		std::vector<uint16_t>							m_freeList;		// LIFO: reuses warm slots first
		std::vector<std::unique_ptr<IComponentPool>>	m_pools;		// indexed by ComponentTypeIndex<T>()
		size_t											m_aliveCount = 0;
	};
}
