#include "EnemyManager.h"
#include "AnimationSystem.h"

namespace enemies
{
	void EnemyManager::Init()
	{
		m_pool.Init(m_registry);

		const auto* lit = eng::Engine::GetInstance().GetResources().GetDefaultLitShader();
		for (size_t i = 0; i < m_materials.byType.size(); ++i)
		{
			const EnemyArchetype& archetype = GetArchetype(static_cast<EnemyType>(i));
			EnemyMaterials::Set& set = m_materials.byType[i];
			set.body.SetShader(lit);
			set.body.SetTint(archetype.bodyColor);
			set.bodyFlash.SetShader(lit);
			set.bodyFlash.SetTint(WHITE);
			set.spike.SetShader(lit);
			set.spike.SetTint(archetype.spikeColor);
		}
	}

	void EnemyManager::Reset()
	{
		m_pool.Clear();
	}

	void EnemyManager::FixedUpdate(float /*fixedDeltaTime*/)
	{
		m_pool.FlushReleased();
	}

	void EnemyManager::Animate(float deltaTime)
	{
		AnimationSystem(m_registry, deltaTime);
	}

	void EnemyManager::Submit(eng::RenderQueue& queue)
	{
		RenderSystem(m_registry, m_materials, queue);
	}
}
