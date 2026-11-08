#pragma once
#include <cstdint>
#include <raylib.h>

namespace eng
{
	class Camera;
	class GameActor;

	enum class GizmoMode : uint8_t
	{
		Move,	// world X / Y / Z arrows
		Scale,	// the actor's own X / Y / Z axes, boxes at the tips
	};

	/// <summary>
	/// Axis handles for the selected actor. Hover an axis and drag with LMB: the mouse ray is projected onto
	/// the axis line, so the actor follows the cursor along that axis. Handles keep a constant size on screen.
	///
	/// Physics actors are written through RigidBody::Teleport (pose + zero velocity) or CharacterMotor::Warp;
	/// scale edits also call RigidBody::OnScaleChanged / Collider::SyncScaleFromActor.
	/// </summary>
	class TransformGizmo
	{
	public:
		/// <summary>
		/// Returns true while the gizmo owns the mouse (cursor over a handle, or dragging): the caller must
		/// not pick or select. Ends a drag by itself when LMB is released or the target is gone.
		/// </summary>
		bool		Update(const Camera& camera, const Vector2& mouse, bool leftPressed, bool leftDown, GameActor* target);

		void		Draw3D(const Camera& camera, const GameActor* target) const;

		void		SetMode(GizmoMode mode);
		GizmoMode	GetMode() const;
		bool		IsDragging() const;
		void		CancelDrag();

	private:
		static constexpr int	NO_AXIS				= -1;
		static constexpr float	HANDLE_SCREEN_SIZE	= 0.18f;	// handle length as a fraction of the camera distance
		static constexpr float	PICK_RADIUS			= 0.10f;	// of the handle length
		static constexpr float	MIN_SCALE			= 0.05f;

		struct AxisFrame
		{
			Vector3 origin;
			Vector3 dir[3];
			float	length;
		};

		AxisFrame	MakeFrame(const Camera& camera, const GameActor& target) const;
		int			HitTest(const Ray& ray, const AxisFrame& frame) const;
		void		Apply(GameActor& target, const Vector3& position, const Vector3& scale) const;

		GizmoMode	m_mode			= GizmoMode::Move;
		int			m_hoverAxis		= NO_AXIS;
		int			m_dragAxis		= NO_AXIS;

		// Captured when the drag starts
		AxisFrame	m_dragFrame		= {};
		float		m_dragStartT	= 0.0f;
		Vector3		m_startWorldPos	= {};
		Vector3		m_startScale	= {};
	};
}
