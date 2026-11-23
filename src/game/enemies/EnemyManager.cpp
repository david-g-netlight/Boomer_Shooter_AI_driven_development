#include "EnemyManager.h"
#include "AnimationSystem.h"
#include "ai/EnemyBrain.h"
#include "systems/Systems.h"
#include "EnemyComponents.h"
#include "player/Gun.h"
#include "state/SoundFXBank.h"
#include "EnemyDebugDraw.h"
#include <raymath.h>
#include <cmath>

namespace enemies
{
	void EnemyManager::Init()
	{
		m_pool.Init(m_registry);
		m_waves.Init(m_pool, m_registry);
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

	void EnemyManager::SetPlayer(const Vector3& chest, bool alive, bool isPlaying)
	{
		m_playerTarget = chest;
		m_playerAlive = alive && isPlaying;
		m_isPlaying = isPlaying;
	}

	void EnemyManager::CollectSpawnPoints(const eng::Level& level)
	{
		std::vector<Vector3> points;
		for (const auto& actor : level.GetActors())
		{
			const EnemySpawnPoint* spawn = actor->GetComponent<EnemySpawnPoint>();
			if (spawn && spawn->IsSpawnEnabled())
				points.push_back(actor->GetWorldPosition());
		}

		if (points.empty())
		{
			constexpr float FINITE = 1.0e5f;
			if (m_arena.max.x < FINITE)
			{
				eng::Log::Warning("EnemyManager: the level has no EnemySpawnPoint, the waves use the arena's 4 upper corners");
				for (const float x : { m_arena.min.x, m_arena.max.x })
					for (const float z : { m_arena.min.z, m_arena.max.z })
						points.push_back({ x, m_arena.max.y, z });
			}
			else
			{
				eng::Log::Warning("EnemyManager: the level has no EnemySpawnPoint and no arena: the waves have nowhere to spawn");
			}
		}
		m_waves.SetSpawnPoints(std::move(points));
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
		SoundFXBank::Instance().Play(Sfx::EnemyHit);

		// Fake aggro (E-D15): an Idle or Wander enemy (the player is outside its chase range) turns toward the shot origin
		const Brain* brain = m_registry.Get<Brain>(shot.enemy);
		Alert* alert = m_registry.Get<Alert>(shot.enemy);
		if (brain && alert && life->hp > 0.0f && (brain->state == AiState::Idle || brain->state == AiState::Wander))
		{
			alert->shotFrom = shot.origin;
			alert->pending = true;
		}
	}

	void EnemyManager::Reset()
	{
		m_pool.Clear();
		m_waves.Reset();
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
			SystemContext ctx{ engine.GetPhysics(), m_arena, engine.GetEvents(), engine.GetParticles(), m_playerTarget, m_playerAlive };
			ScanSystem(m_registry, ctx, fixedDeltaTime);
			BrainSystem(m_registry, ctx, fixedDeltaTime);
			IdleFloatSystem(m_registry, ctx, fixedDeltaTime);
			WanderSystem(m_registry, ctx, fixedDeltaTime);
			ChaseSystem(m_registry, ctx, fixedDeltaTime);
			AvoidOthersSystem(m_registry, ctx, fixedDeltaTime);
			MovementSystem(m_registry, ctx, fixedDeltaTime);
			DealDamageSystem(m_registry, ctx, fixedDeltaTime);
			LifeSystem(m_registry, ctx, fixedDeltaTime);
			
#if ENG_DEV_CONSOLE
		}
#endif	
		m_pool.FlushReleased();

		if (m_isPlaying)
			m_waves.Update(fixedDeltaTime, m_playerTarget);
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
