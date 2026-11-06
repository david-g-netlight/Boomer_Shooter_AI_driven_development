#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <raylib.h>
#include "physics/PhysicsLayers.h"
#include "physics/RaycastHit.h"

class btDiscreteDynamicsWorld;	// Bullet type, only used by the engine's physics code

namespace eng
{
	class Collider;
	class TriggerVolume;
}

namespace eng
{
	/// <summary>
	/// The Bullet dynamics world, owned by the Engine and stepped once per fixed tick.
	/// This header does not include Bullet (pimpl): the game never depends on it.
	/// Physics components (RigidBody, TriggerVolume, CharacterMotor) register their Bullet objects here in
	/// OnEnable and remove them in OnDisable, which also runs when their actor is destroyed.
	/// </summary>
	class PhysicsWorld
	{
	public:
		PhysicsWorld();
		~PhysicsWorld();

		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator = (const PhysicsWorld&) = delete;

		bool		Init(const Vector3& gravity);
		void		Shutdown();
		bool		IsReady() const;

		/// <summary>
		/// One fixed tick: trigger poses from their actors, stepSimulation(fixedDeltaTime, 1, fixedDeltaTime),
		/// then trigger Enter / Exit callbacks. The Engine owns the accumulator, so FixedUpdate and physics
		/// ticks stay in lockstep.
		/// </summary>
		void		Step(float fixedDeltaTime);

		void		SetGravity(const Vector3& gravity);
		Vector3		GetGravity() const;

		int			GetObjectCount() const;		// collision objects in the world

		/// <summary>
		/// Bullet wireframes of every collision shape + contact normals. Drawn by the Engine in the 3D pass.
		/// </summary>
		void		SetDebugDrawEnabled(bool enabled);
		bool		IsDebugDrawEnabled() const;
		void		DebugDraw() const;			// inside BeginMode3D / EndMode3D; no-op when disabled

		/// <summary>
		/// Closest hit along the ray. `mask` = the layers the ray may hit (e.g. Layer::Ground | Layer::Prop);
		/// the hit objects' own masks are not checked. Triggers are hit only if Layer::Trigger is in the mask.
		/// </summary>
		std::optional<RaycastHit>	Raycast(const Vector3& origin, const Vector3& direction, float maxDistance,
											uint32_t mask = Layer::All) const;

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;

		/// <summary>
		/// nullptr when not initialized (or already shut down).
		/// </summary>
		btDiscreteDynamicsWorld*	GetNativeWorld() const;

		void		RegisterTrigger(TriggerVolume& trigger);
		void		UnregisterTrigger(TriggerVolume& trigger);
		void		SyncTriggers();
		void		DispatchTriggers();

		/// <summary>
		/// A body / ghost of `collider` left the world: forget it in every trigger (no Exit callback).
		/// </summary>
		void		OnColliderRemoved(const Collider* collider);

		// Physics components reach the Bullet world through GetNativeWorld()
		friend class RigidBody;
		friend class TriggerVolume;
		friend class CharacterMotor;
	};
}
