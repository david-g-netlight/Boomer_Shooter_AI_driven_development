#include "EnemyManager.h"
#include "AnimationSystem.h"
#include "ai/EnemyBrain.h"
#include "systems/Systems.h"

namespace enemies
{
	void EnemyManager::Init()
	{
		m_pool.Init(m_registry);

		const auto* lit = eng::Engine::GetInstance().GetResources().GetDefaultLitShader();
		for (size_t i = 0; i < m_materials.byType.size(); ++i)
		{
			const EnemyArchetype& archetype = GetArchetype(static_cast<EnemyType>(i));
			// Made here, not in EnemyMaterials' constructor: a Material takes raylib's default texture from the
			// window, and the constructor of ArenaGame runs before the window exists (the texture would be id 0)
			EnemyMaterials::Set& set = m_materials.byType[i];
			set.body = eng::Material(lit);
			set.body.SetTint(archetype.bodyColor);
			set.bodyFlash = eng::Material(lit);
			set.bodyFlash.SetTint(WHITE);
			set.spike = eng::Material(lit);
			set.spike.SetTint(archetype.spikeColor);
		}
	}

	void EnemyManager::Reset()
	{
		m_pool.Clear();
	}

	void EnemyManager::FixedUpdate(float fixedDeltaTime)
	{
		SystemContext ctx{ eng::Engine::GetInstance().GetPhysics(), m_arena };
		BrainSystem(m_registry, ctx, fixedDeltaTime);
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
