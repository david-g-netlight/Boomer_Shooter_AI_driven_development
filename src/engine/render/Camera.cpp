#include "Camera.h"
#include <algorithm>
#include <cmath>
#include <raymath.h>
#include <rlgl.h>

namespace eng
{
	namespace
	{
		constexpr float MAX_PITCH_DEGREES = 89.0f;
		constexpr Vector3 WORLD_UP = { 0.0f, 1.0f, 0.0f };
	}

	Camera::Camera(const PerspectiveParams& params)
		: m_perspective(params)
	{
	}

	//////////////////////////////////////////
	/// Pose
	//////////////////////////////////////////
	void Camera::SetPosition(const Vector3& position)	{ m_position = position; }
	const Vector3& Camera::GetPosition() const			{ return m_position; }
	float Camera::GetYaw() const						{ return m_yaw; }
	float Camera::GetPitch() const						{ return m_pitch; }

	void Camera::SetYawPitch(float yawDegrees, float pitchDegrees)
	{
		m_yaw	= std::fmod(yawDegrees, 360.0f);
		m_pitch	= std::clamp(pitchDegrees, -MAX_PITCH_DEGREES, MAX_PITCH_DEGREES);
	}

	void Camera::Rotate(float deltaYawDegrees, float deltaPitchDegrees)
	{
		SetYawPitch(m_yaw + deltaYawDegrees, m_pitch + deltaPitchDegrees);
	}

	void Camera::LookAt(const Vector3& target)
	{
		const Vector3 direction = Vector3Subtract(target, m_position);
		if (Vector3LengthSqr(direction) < 1e-8f)
			return;

		const Vector3 dir = Vector3Normalize(direction);
		SetYawPitch(std::atan2(dir.x, -dir.z) * RAD2DEG, std::asin(std::clamp(dir.y, -1.0f, 1.0f)) * RAD2DEG);
	}

	void Camera::MoveLocal(float forward, float right, float up)
	{
		m_position = Vector3Add(m_position, Vector3Scale(Forward(), forward));
		m_position = Vector3Add(m_position, Vector3Scale(Right(), right));
		m_position = Vector3Add(m_position, Vector3Scale(WORLD_UP, up));
	}

	Vector3 Camera::Forward() const
	{
		const float yaw		= m_yaw * DEG2RAD;
		const float pitch	= m_pitch * DEG2RAD;
		return { std::cos(pitch) * std::sin(yaw), std::sin(pitch), -std::cos(pitch) * std::cos(yaw) };
	}

	Vector3 Camera::Right() const
	{
		return Vector3Normalize(Vector3CrossProduct(Forward(), WORLD_UP));
	}

	Vector3 Camera::Up() const
	{
		return Vector3CrossProduct(Right(), Forward());
	}

	//////////////////////////////////////////
	/// Projection
	//////////////////////////////////////////
	void Camera::SetPerspective(const PerspectiveParams& params)	{ m_perspective = params; }
	const PerspectiveParams& Camera::GetPerspective() const			{ return m_perspective; }

	Camera3D Camera::ToRaylib() const
	{
		Camera3D camera		= {};
		camera.position		= m_position;
		camera.target		= Vector3Add(m_position, Forward());
		camera.up			= WORLD_UP;
		camera.fovy			= m_perspective.fovY;
		camera.projection	= CAMERA_PERSPECTIVE;
		return camera;
	}

	Matrix Camera::GetViewMatrix() const
	{
		return GetCameraMatrix(ToRaylib());
	}

	void Camera::Begin3D() const
	{
		rlSetClipPlanes(m_perspective.nearPlane, m_perspective.farPlane);
		BeginMode3D(ToRaylib());
	}

	void Camera::End3D() const
	{
		EndMode3D();
	}

	//////////////////////////////////////////
	/// ActiveCamera
	//////////////////////////////////////////
	void ActiveCamera::SetActiveCamera(Camera& camera)
	{
		m_currentCamera = &camera;
	}

	Camera* ActiveCamera::GetActiveCamera() const
	{
		return m_currentCamera;
	}

	void ActiveCamera::Begin3D() const
	{
		if (m_currentCamera)
			m_currentCamera->Begin3D();
	}

	void ActiveCamera::End3D() const
	{
		if (m_currentCamera)
			m_currentCamera->End3D();
	}
}
