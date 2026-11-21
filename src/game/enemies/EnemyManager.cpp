#include "EnemyManager.h"

namespace enemies
{
	void EnemyManager::Init()
	{
		m_pool.Init(m_registry);
	}

	void EnemyManager::Reset()
	{
		m_pool.Clear();
	}

	void EnemyManager::FixedUpdate(float /*fixedDeltaTime*/)
	{
		m_pool.FlushReleased();
	}

	void EnemyManager::Submit(eng::RenderQueue& /*queue*/) const
	{
	}
}
