#pragma once
#include <raylib.h>
#include <eng.h>

class PlayerInput;

/// <summary>
/// Mouse look and the first-person camera. Yaw turns the body (the actor's rotation about Y), pitch only tilts
/// the camera (clamped to +-89 degrees). Every Update() the camera is put on the eyes: actor position + eye height.
/// Runs only in Play (the Level doesn't update otherwise), so menus and the editor keep their own cameras.
/// </summary>
class FirstPersonLook : public eng::Component
{
public:
	static constexpr float	SENSITIVITY		= 0.1f;		// degrees per screen point
	static constexpr float	EYE_HEIGHT		= 0.7f;		// above the capsule center (1.7 m above the feet)
	static constexpr float	MAX_PITCH		= 89.0f;

	void			Bind(PlayerInput* input, eng::Camera* camera);

	void			SetYawPitch(float yawDegrees, float pitchDegrees);	// positive yaw turns right
	float			GetYaw() const;
	float			GetPitch() const;

	/// <summary>
	/// Where the player faces / its right, flattened onto the ground plane and normalized (walk directions).
	/// </summary>
	Vector3			GetPlanarForward() const;
	Vector3			GetPlanarRight() const;

	void			OnEnable() override;
	void			Update(float deltaTime) override;
	const char*		TypeName() const override { return "FirstPersonLook"; }

private:
	void			Apply();	// writes camera pose and the body's yaw

	PlayerInput*	m_input		= nullptr;
	eng::Camera*	m_camera	= nullptr;
	float			m_yaw		= 0.0f;
	float			m_pitch		= 0.0f;
};
