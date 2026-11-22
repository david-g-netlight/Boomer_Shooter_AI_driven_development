#include "EnemyManager.h"
#include "AnimationSystem.h"
#include "ai/EnemyBrain.h"
#include "systems/Systems.h"
#include "EnemyComponents.h"
#include "player/Gun.h"
#include "EnemyDebugDraw.h"
#include <raymath.h>
#include <cmath>

namespace enemies
{
	void EnemyManager::Init()
	{
		m_pool.Init(m_registry);
		m_shotToken = eng::Engine::GetInstance().GetEvents().Subscribe<ShotFiredEvent>([this](const ShotFiredEvent& shot) { OnShot(shot); });

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

	void EnemyManager::Shutdown()
	{
		if (m_shotToken != 0)
			eng::Engine::GetInstance().GetEvents().Unsubscribe(m_shotToken);
		m_shotToken = 0;
	}

	void EnemyManager::OnShot(const ShotFiredEvent& shot)
	{
		// The event arrives at the end of the frame: the enemy may be gone, and its slot may hold a new one (generation check)
		if (shot.enemy == eng::ecs::INVALID_ENTITY || !m_registry.IsAlive(shot.enemy))
			return;

		Life* life = m_registry.Get<Life>(shot.enemy);
		if (!life || life->isDead || life->hp <= 0.0f)
			return;

		life->hp -= shot.damage;
		life->hitFlash = HIT_FLASH_TIME;
	}

	void EnemyManager::Reset()
	{
		m_pool.Clear();
	}

	void EnemyManager::SetArena(const eng::GameActor* arena)
	{
		constexpr float MARGIN = 0.5f;	// m, keeps the enemies off the render cube's walls; must stay under the 1 m probe reach, or the clamp pins an enemy just outside the probe (R11)

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
#if ENG_DEV_CONSOLE
		if (!m_debug.freeze)
		{
#endif		
			eng::Engine& engine = eng::Engine::GetInstance();
			SystemContext ctx{ engine.GetPhysics(), m_arena, engine.GetEvents(), engine.GetParticles() };
			BrainSystem(m_registry, ctx, fixedDeltaTime);
			IdleFloatSystem(m_registry, ctx, fixedDeltaTime);
			WanderSystem(m_registry, ctx, fixedDeltaTime);
			AvoidOthersSystem(m_registry, ctx, fixedDeltaTime);
			MovementSystem(m_registry, ctx, fixedDeltaTime);
			LifeSystem(m_registry, ctx, fixedDeltaTime);
			
#if ENG_DEV_CONSOLE
		}
#endif	
		m_pool.FlushReleased();
	}

	std::optional<HitscanHit> EnemyManager::Raycast(const Vector3& origin, const Vector3& dir, float maxDistance) const
	{
		std::optional<HitscanHit> best;
		float bestDistance = maxDistance;

		// Each is not const; the registry only holds enemy data and this test does not change it
		auto& registry = const_cast<eng::ecs::Registry&>(m_registry);
		registry.Each<Transform, CollisionSphere, Life>([&](eng::ecs::Entity e, Transform& transform, CollisionSphere& sphere, Life& life)
		{
			if (life.isDead || life.hp <= 0.0f)
				return;

			// |origin + t dir - centre|^2 = r^2 with |dir| = 1: t^2 + 2 b t + c = 0
			const Vector3 toOrigin = Vector3Subtract(origin, transform.position);
			const float b = Vector3DotProduct(toOrigin, dir);
			const float c = Vector3DotProduct(toOrigin, toOrigin) - sphere.radius * sphere.radius;
			const float discriminant = b * b - c;
			if (discriminant < 0.0f)
				return;

			const float t = -b - std::sqrt(discriminant);	// the entry point
			if (t < 0.0f || t > bestDistance)
				return;

			const Vector3 point = Vector3Add(origin, Vector3Scale(dir, t));
			bestDistance = t;
			best = HitscanHit{ e, point, Vector3Normalize(Vector3Subtract(point, transform.position)), t };
		});
		return best;
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
