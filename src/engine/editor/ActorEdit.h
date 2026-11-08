#pragma once
#include <raylib.h>

namespace eng
{
	class GameActor;

	/// <summary>
	/// Editor writes to an actor's transform that keep physics in step. Used by the gizmo and the inspector.
	/// </summary>
	namespace ActorEdit
	{
		/// <summary>
		/// Sets the local transform. A RigidBody is teleported (pose + zero velocity) and rescaled, a
		/// CharacterMotor is warped, a lone Collider gets its scale synced.
		/// </summary>
		void SetLocalTransform(GameActor& actor, const ::Transform& transform);
	}
}
