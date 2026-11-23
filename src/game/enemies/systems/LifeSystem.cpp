#include "Systems.h"
#include "../EnemyComponents.h"
#include "../EnemyEvents.h"
#include "state/SoundFXBank.h"

namespace enemies
{
	namespace
	{
		constexpr float	DYING_TIME	= 0.25f;	// s, the shrink of the AnimationSystem runs over this time (§8.2)

		void DeathBurst(eng::ParticleSystem& particles, const Vector3& position, EnemyType type)
		{
			const EnemyArchetype& archetype = GetArchetype(type);

			eng::ParticleEmitterSettings burst;
			burst.burstCount		= 30;
			burst.lifetime			= 0.6f;
			burst.speed				= 4.0f;
			burst.spreadDegrees		= 180.0f;
			burst.startSize			= 0.12f;
			burst.endSize			= 0.02f;
			burst.gravity			= { 0.0f, -4.0f, 0.0f };
			burst.startColor		= archetype.bodyColor;
			burst.endColor			= Color{ archetype.spikeColor.r, archetype.spikeColor.g, archetype.spikeColor.b, 0 };
			particles.Emit(burst, position, { 0.0f, 1.0f, 0.0f });
		}
	}

	void LifeSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt)
	{
		registry.Each<Life, Brain, Transform, Motion, EnemyKind>([&](eng::ecs::Entity, Life& life, Brain& brain, Transform& transform, Motion& motion, EnemyKind& kind)
		{
			if (life.isDead)
				return;

			if (brain.state != AiState::Dying)	// BrainSystem moves an enemy with hp <= 0 into Dying
				return;

			motion.velocity = { 0.0f, 0.0f, 0.0f };
			life.dyingTime += dt;
			if (life.dyingTime < DYING_TIME)
				return;

			life.isDead = true;
			SoundFXBank::Instance().Play(Sfx::EnemyDeath);
			DeathBurst(ctx.particles, transform.position, kind.type);
			ctx.events.Publish(EnemyKilledEvent{ kind.type, transform.position });
		});
	}
}
