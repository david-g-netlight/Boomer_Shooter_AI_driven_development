#pragma once
#include <memory>
#include <raylib.h>

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
		/// One fixed tick: stepSimulation(fixedDeltaTime, 1, fixedDeltaTime). The Engine owns the
		/// accumulator, so FixedUpdate and physics ticks stay in lockstep.
		/// </summary>
		void		Step(float fixedDeltaTime);

		void		SetGravity(const Vector3& gravity);
		Vector3		GetGravity() const;

		int			GetObjectCount() const;		// collision objects in the world

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;

		// Physics components reach the Bullet world through Impl
		friend class RigidBody;
		friend class TriggerVolume;
		friend class CharacterMotor;
	};
}
