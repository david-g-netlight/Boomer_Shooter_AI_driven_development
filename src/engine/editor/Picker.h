#pragma once
#include <raylib.h>

namespace eng
{
	class Camera;
	class GameActor;
	class Level;
	class PhysicsWorld;

	/// <summary>
	/// Mouse picking for the editor. The mouse ray goes through the physics world first (exact, works with
	/// rotated and scaled shapes, includes triggers); only when it hits nothing does it fall back to the render
	/// bounds of actors that have a MeshRenderer but no Collider.
	/// </summary>
	namespace Picker
	{
		constexpr float MAX_PICK_DISTANCE = 1000.0f;

		/// <summary>
		/// `mouse` in screen points. nullptr when nothing is under the cursor.
		/// </summary>
		GameActor*	Pick(const Camera& camera, const Vector2& mouse, const Level& level, const PhysicsWorld& physics);

		/// <summary>
		/// Box used for the selection highlight and the F focus: the MeshRenderer's world bounds, otherwise a
		/// 1 m cube around the actor (triggers show their collider through the physics debug view instead).
		/// </summary>
		BoundingBox	GetActorBounds(const GameActor& actor);
	}
}
