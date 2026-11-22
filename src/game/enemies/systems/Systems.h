#pragma once
#include <eng.h>
#include <cstdint>
#include <optional>

namespace enemies
{
	/// <summary>
	/// The walkable box of the enemies: the height band (E-D3) and the walls the sweep stays inside.
	/// The default has no limits; the "Arena" actor sets it (§6.2).
	/// </summary>
	struct ArenaBounds
	{
		Vector3	min = { -1.0e6f, -1.0e6f, -1.0e6f };
		Vector3	max = {  1.0e6f,  1.0e6f,  1.0e6f };
	};

	/// <summary>The shared inputs of the systems (§5.3). Each system takes what it needs.</summary>
	struct SystemContext
	{
		eng::PhysicsWorld&	physics;
		ArenaBounds			bounds;
		eng::EventBus&		events;
		eng::ParticleSystem&	particles;
	};

	/// <summary>Idle: the bob (§7.3). Writes the velocity only; MovementSystem moves the enemy.</summary>
	void IdleFloatSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt);

	/// <summary>Every pair closer than r_i + r_j + 0.5 m is pushed apart by its overlap (§5.3, step 6).</summary>
	void AvoidOthersSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt);

	/// <summary>Moves every enemy: sweep and slide at walls, the height band, then the overlap fix (§5.3, step 7).</summary>
	void MovementSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt);

	/// <summary>
	/// hp <= 0: Dying for 0.25 s, then isDead (FlushReleased destroys the entity), a particle burst and EnemyKilledEvent (§5.3, step 9).
	/// </summary>
	void LifeSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt);

	/// <summary>Wander: forward speed, the wall probe, and a new direction when there is a wall (§7.3).</summary>
	void WanderSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt);

	/// <summary>xorshift32 step (the RNG of Wander, §5.1). The state must never be 0.</summary>
	inline uint32_t NextRandom(uint32_t& state)
	{
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return state;
	}

	/// <summary>
	/// A random direction: a point inside the unit sphere (rejection sampling), y damped ×0.35, normalized (§7.3).
	/// </summary>
	inline Vector3 RandomUnitDirection(uint32_t& state)
	{
		Vector3 p{};
		float lengthSq = 0.0f;
		do
		{
			p.x = static_cast<float>(NextRandom(state)) / 4294967295.0f * 2.0f - 1.0f;
			p.y = static_cast<float>(NextRandom(state)) / 4294967295.0f * 2.0f - 1.0f;
			p.z = static_cast<float>(NextRandom(state)) / 4294967295.0f * 2.0f - 1.0f;
			lengthSq = p.x * p.x + p.y * p.y + p.z * p.z;
		} while (lengthSq > 1.0f || lengthSq < 0.01f);

		p.y *= 0.35f;
		return Vector3Normalize(p);
	}
}
