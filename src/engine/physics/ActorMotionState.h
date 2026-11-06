#pragma once

/**
* ENGINE-INTERNAL (includes Bullet).
*/

#include "BulletConvert.h"
#include <LinearMath/btMotionState.h>

namespace eng
{
	class GameActor;

	/// <summary>
	/// Connects a Bullet body to its GameActor.
	///   getWorldTransform: Bullet reads the actor's world pose (on creation, and every step for kinematic bodies).
	///   setWorldTransform: Bullet writes a moving dynamic body's pose back into the actor.
	/// Parented actors work: the world pose is converted to the parent's local space. The actor's scale is
	/// never touched (it lives on the collision shape). writeRotation = false keeps the actor's rotation
	/// (the character capsule never rotates, its yaw comes from mouse look).
	/// </summary>
	class ActorMotionState : public btMotionState
	{
	public:
		explicit ActorMotionState(GameActor& actor, bool writeRotation = true);

		void getWorldTransform(btTransform& worldTransform) const override;
		void setWorldTransform(const btTransform& worldTransform) override;

	private:
		GameActor&	m_actor;
		bool		m_writeRotation;
	};
}
