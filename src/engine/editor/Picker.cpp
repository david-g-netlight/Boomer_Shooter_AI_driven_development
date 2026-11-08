#include "Picker.h"
#include <cfloat>
#include "level/GameActor.h"
#include "level/Level.h"
#include "physics/Collider.h"
#include "physics/PhysicsWorld.h"
#include "render/Camera.h"
#include "render/MeshRenderer.h"

namespace eng::Picker
{
	GameActor* Pick(const Camera& camera, const Vector2& mouse, const Level& level, const PhysicsWorld& physics)
	{
		const Ray ray = GetScreenToWorldRay(mouse, camera.ToRaylib());

		if (const auto hit = physics.Raycast(ray.position, ray.direction, MAX_PICK_DISTANCE))
			if (hit->actor)
				return hit->actor;

		// No physics hit: closest render bounds among actors without a collider
		GameActor*	closest			= nullptr;
		float		closestDistance	= FLT_MAX;
		for (const auto& owned : level.GetActors())
		{
			GameActor& actor = *owned;
			if (actor.IsMarkedForDestroy() || !actor.IsActiveInHierarchy() || actor.GetComponent<Collider>())
				continue;

			const MeshRenderer* renderer = actor.GetComponent<MeshRenderer>();
			if (!renderer)
				continue;

			const RayCollision collision = GetRayCollisionBox(ray, renderer->GetWorldBounds());
			if (collision.hit && collision.distance < closestDistance)
			{
				closest			= &actor;
				closestDistance	= collision.distance;
			}
		}
		return closest;
	}

	BoundingBox GetActorBounds(const GameActor& actor)
	{
		if (const MeshRenderer* renderer = actor.GetComponent<MeshRenderer>())
			return renderer->GetWorldBounds();

		const Vector3 p = actor.GetWorldPosition();
		return { { p.x - 0.5f, p.y - 0.5f, p.z - 0.5f }, { p.x + 0.5f, p.y + 0.5f, p.z + 0.5f } };
	}
}
