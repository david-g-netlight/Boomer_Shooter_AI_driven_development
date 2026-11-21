#include "EnemyManager.h"
#include "AnimationSystem.h"
#include "ai/EnemyBrain.h"
#include "systems/Systems.h"
#include "EnemyDebugDraw.h"

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

	void EnemyManager::SetArena(const eng::GameActor* arena)
	{
		constexpr float MARGIN = 1.0f;	// m, keeps the enemies off the render cube's walls

		const eng::MeshRenderer* renderer = arena ? arena->GetComponent<eng::MeshRenderer>() : nullptr;
		if (!renderer)
		{
			m_arena = ArenaBounds{};
			eng::Log::Warning("EnemyManager: no \"Arena\" actor with a mesh; the enemies have no height band");
			return;
		}

		const BoundingBox box = renderer->GetWorldBounds();
		m_arena.min = { box.min.x + MARGIN, box.min.y + MARGIN, box.min.z + MARGIN };
		m_arena.max = { box.max.x - MARGIN, box.max.y - MARGIN, box.max.z - MARGIN };
	}

	void EnemyManager::FixedUpdate(float fixedDeltaTime)
	{
		if (!m_debug.freeze)
		{
			SystemContext ctx{ eng::Engine::GetInstance().GetPhysics(), m_arena };
			BrainSystem(m_registry, ctx, fixedDeltaTime);
			IdleFloatSystem(m_registry, ctx, fixedDeltaTime);
			WanderSystem(m_registry, ctx, fixedDeltaTime);
			AvoidOthersSystem(m_registry, ctx, fixedDeltaTime);
			MovementSystem(m_registry, ctx, fixedDeltaTime);
		}
		m_pool.FlushReleased();
	}

	void EnemyManager::Animate(float deltaTime)
	{
		AnimationSystem(m_registry, deltaTime);
	}

	void EnemyManager::Submit(eng::RenderQueue& queue)
	{
		RenderSystem(m_registry, m_materials, queue);

		if (m_debug.draw)
		{
			eng::DebugDraw& debug = eng::Engine::GetInstance().GetDebugDraw();
			if (!debug.IsEnabled())
				debug.SetEnabled(true);		// the engine drops debug items while it is disabled
			DrawEnemyDebug(m_registry, debug);
		}
	}
}
