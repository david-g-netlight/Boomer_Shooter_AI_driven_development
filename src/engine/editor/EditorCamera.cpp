#include "EditorCamera.h"
#include <algorithm>
#include <cmath>
#include "input/InputManager.h"

namespace eng
{
	namespace
	{
		constexpr float LOOK_DEGREES_PER_POINT	= 0.15f;
		constexpr float MIN_FLY_SPEED			= 0.5f;
		constexpr float MAX_FLY_SPEED			= 100.0f;
		constexpr float FAST_MULTIPLIER			= 3.0f;
		constexpr float WHEEL_SPEED_FACTOR		= 1.15f;	// each wheel notch scales the speed
	}

	void EditorCamera::CopyPoseFrom(const Camera& source)
	{
		m_camera	= source;
		m_isLooking	= false;	// the ModeMachine sets the cursor for the mode
	}

	void EditorCamera::Update(InputManager& input, float unscaledDeltaTime)
	{
		// Looking captures the cursor, so it can't leave the window mid-drag
		const bool wantsLook = input.IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
		if (wantsLook != m_isLooking)
		{
			m_isLooking = wantsLook;
			input.SetCursorLocked(m_isLooking);
		}

		if (!m_isLooking)
			return;

		const Vector2& delta = input.GetMouseDelta();
		m_camera.Rotate(delta.x * LOOK_DEGREES_PER_POINT, -delta.y * LOOK_DEGREES_PER_POINT);	// screen y grows downwards

		const float wheel = input.GetMouseWheel();
		if (wheel != 0.0f)
			m_flySpeed = std::clamp(m_flySpeed * std::pow(WHEEL_SPEED_FACTOR, wheel), MIN_FLY_SPEED, MAX_FLY_SPEED);

		const float forward	= static_cast<float>(input.IsKeyDown(KEY_W)) - static_cast<float>(input.IsKeyDown(KEY_S));
		const float right	= static_cast<float>(input.IsKeyDown(KEY_D)) - static_cast<float>(input.IsKeyDown(KEY_A));
		const float up		= static_cast<float>(input.IsKeyDown(KEY_E)) - static_cast<float>(input.IsKeyDown(KEY_Q));
		const float speed	= m_flySpeed * (input.IsKeyDown(KEY_LEFT_SHIFT) ? FAST_MULTIPLIER : 1.0f) * unscaledDeltaTime;
		m_camera.MoveLocal(forward * speed, right * speed, up * speed);
	}

	void EditorCamera::FocusOn(const Vector3& center, float radius)
	{
		// Back off so the sphere fills about half of the vertical field of view
		const float halfFov	= m_camera.GetPerspective().fovY * 0.5f * DEG2RAD;
		const float distance = std::max(radius, 0.25f) / std::sin(halfFov) * 1.2f;
		const Vector3 forward = m_camera.Forward();
		m_camera.SetPosition({ center.x - forward.x * distance, center.y - forward.y * distance, center.z - forward.z * distance });
	}

	Camera& EditorCamera::GetCamera()					{ return m_camera; }
	const Camera& EditorCamera::GetCamera() const		{ return m_camera; }
	float EditorCamera::GetFlySpeed() const				{ return m_flySpeed; }
	void EditorCamera::SetFlySpeed(float metersPerSecond)	{ m_flySpeed = std::clamp(metersPerSecond, MIN_FLY_SPEED, MAX_FLY_SPEED); }
}
