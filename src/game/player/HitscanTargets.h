#pragma once
#include <raylib.h>
#include <eng.h>
#include <optional>

/// <summary>
/// What a shot found among the shootable things that are not physics colliders (the enemies).
/// </summary>
struct HitscanHit
{
	eng::ecs::Entity	entity;
	Vector3				point;
	Vector3				normal;
	float				distance;
};

/// <summary>
/// The Gun's only view of what it can shoot besides walls and props (plan §9.1). The Gun does not know the ECS or
/// the enemies: a later shootable (a barrel, an NPC) only has to implement this.
/// </summary>
class HitscanTargets
{
public:
	virtual ~HitscanTargets() = default;

	/// <summary>The nearest target hit by the ray within maxDistance, or nothing.</summary>
	virtual std::optional<HitscanHit> Raycast(const Vector3& origin, const Vector3& dir, float maxDistance) const = 0;
};
