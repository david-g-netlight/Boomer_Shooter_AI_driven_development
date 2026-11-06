#pragma once
#include <raylib.h>

namespace eng
{
	struct PerspectiveParams
	{
		float	fovY		= 60.0f;	// degrees, vertical
		float	nearPlane	= 0.05f;
		float	farPlane	= 500.0f;
	};

	/// <summary>
	/// A 3D camera described by position + yaw/pitch (degrees), converted to a raylib Camera3D for drawing.
	/// Yaw 0 looks along World::worldForward (-Z); positive yaw turns right; pitch is clamped to +-89.
	/// The aspect ratio comes from the current render target, so resizing needs no extra work.
	/// </summary>
	class Camera
	{
	public:
		Camera() = default;
		explicit Camera(const PerspectiveParams& params);

		//////////////////////////////////////////
		/// Pose
		//////////////////////////////////////////
		void			SetPosition(const Vector3& position);
		const Vector3&	GetPosition() const;

		void			SetYawPitch(float yawDegrees, float pitchDegrees);
		void			Rotate(float deltaYawDegrees, float deltaPitchDegrees);
		void			LookAt(const Vector3& target);
		float			GetYaw() const;
		float			GetPitch() const;

		/// <summary>
		/// Moves along the camera's own axes (forward / right / world up).
		/// </summary>
		void			MoveLocal(float forward, float right, float up);

		Vector3			Forward() const;
		Vector3			Right() const;
		Vector3			Up() const;

		//////////////////////////////////////////
		/// Projection
		//////////////////////////////////////////
		void						SetPerspective(const PerspectiveParams& params);
		const PerspectiveParams&	GetPerspective() const;

		/// <summary>
		/// raylib camera for BeginMode3D(), GetScreenToWorldRay(), GetWorldToScreen() ...
		/// </summary>
		Camera3D		ToRaylib() const;

		Matrix			GetViewMatrix() const;

		/// <summary>
		/// BeginMode3D with this camera, applying its clip planes (global in rlgl).
		/// </summary>
		void			Begin3D() const;
		void			End3D() const;

	private:
		Vector3				m_position		= { 0.0f, 2.0f, 5.0f };
		float				m_yaw			= 0.0f;
		float				m_pitch			= 0.0f;
		PerspectiveParams	m_perspective;
	};

	/// <summary>
	/// Points at the Camera used for rendering this frame. Non-owning, never null:
	/// the Engine sets its default camera when nothing else is active.
	/// </summary>
	class ActiveCamera
	{
	public:
		ActiveCamera() = default;
		~ActiveCamera() = default;

		ActiveCamera(const ActiveCamera&) = delete;
		ActiveCamera& operator = (const ActiveCamera&) = delete;
		ActiveCamera(ActiveCamera&&) = delete;
		ActiveCamera& operator = (ActiveCamera&&) = delete;

		void			SetActiveCamera(Camera& camera);
		Camera*			GetActiveCamera() const;

		/// <summary>
		/// BeginMode3D with the active camera, applying its clip planes (global in rlgl).
		/// </summary>
		void			Begin3D() const;
		void			End3D() const;

	private:
		Camera*			m_currentCamera = nullptr;
	};
}
