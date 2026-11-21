#pragma once
#include "Entity.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

namespace eng::ecs
{
	/// <summary>
	/// Type-erased base, so the Registry can remove an entity from every pool when it is destroyed.
	/// </summary>
	class IComponentPool
	{
	public:
		virtual ~IComponentPool() = default;
		virtual void Remove(Entity e) = 0;
	};

	/// <summary>
	/// A sparse set: one packed array of components of type T. Entity index → dense index through a sparse array.
	/// Removing swaps the last element into the hole, so the array never has gaps. Iteration is over the packed array.
	/// </summary>
	template<typename T>
	class ComponentPool final : public IComponentPool
	{
		static_assert(std::is_trivially_copyable_v<T>, "ECS components must be plain, trivially copyable structs");

	public:
		explicit ComponentPool(uint16_t capacity)
			: m_sparse(capacity, NONE)
		{
			m_dense.reserve(capacity);
			m_denseEntities.reserve(capacity);
		}

		/// <summary>Adds T to e. e must not already have T.</summary>
		T& Add(Entity e, const T& value)
		{
			m_sparse[e.index] = static_cast<uint16_t>(m_dense.size());
			m_dense.push_back(value);
			m_denseEntities.push_back(e);
			return m_dense.back();
		}

		void Remove(Entity e) override
		{
			if (!Has(e))
				return;

			uint16_t hole = m_sparse[e.index];
			uint16_t last = static_cast<uint16_t>(m_dense.size() - 1);

			if (hole != last)
			{
				m_dense[hole] = m_dense[last];
				m_denseEntities[hole] = m_denseEntities[last];
				m_sparse[m_denseEntities[hole].index] = hole;
			}

			m_dense.pop_back();
			m_denseEntities.pop_back();
			m_sparse[e.index] = NONE;
		}

		bool Has(Entity e) const
		{
			return e.index < m_sparse.size() && m_sparse[e.index] != NONE && m_denseEntities[m_sparse[e.index]] == e;
		}

		/// <summary>Null if e does not have T.</summary>
		T* Get(Entity e)
		{
			return Has(e) ? &m_dense[m_sparse[e.index]] : nullptr;
		}

		std::span<T> Data()
		{
			return m_dense;
		}

		std::span<const Entity> Entities() const
		{
			return m_denseEntities;
		}

		size_t Size() const
		{
			return m_dense.size();
		}

	private:
		static constexpr uint16_t NONE = 0xFFFF;

		std::vector<T>			m_dense;			// the components, packed, no holes
		std::vector<Entity>		m_denseEntities;	// m_dense[i] belongs to m_denseEntities[i]
		std::vector<uint16_t>	m_sparse;			// entity index → dense index (NONE if missing)
	};
}
