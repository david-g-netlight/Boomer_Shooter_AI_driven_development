#pragma once
#include <raylib.h>

namespace eng
{
	class GameActor;
	class Collider;

	/// <summary>
	/// Result of PhysicsWorld::Raycast. Pointers are valid until the end of the frame.
	/// </summary>
	struct RaycastHit
	{
		GameActor*	actor		= nullptr;
		Collider*	collider	= nullptr;
		Vector3		point		= { 0.0f, 0.0f, 0.0f };
		Vector3		normal		= { 0.0f, 1.0f, 0.0f };	// world space, pointing out of the hit surface
		float		distance	= 0.0f;
	};
}
