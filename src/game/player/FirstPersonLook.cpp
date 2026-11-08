#include "FirstPersonLook.h"
#include <algorithm>
#include <cmath>
#include <raymath.h>
#include "player/PlayerInput.h"

void FirstPersonLook::Bind(PlayerInput* input, eng::Camera* camera)
{
	m_input		= input;
	m_camera	= camera;
}

void FirstPersonLook::SetYawPitch(float yawDegrees, float pitchDegrees)
{
	m_yaw	= yawDegrees;
	m_pitch	= std::clamp(pitchDegrees, -MAX_PITCH, MAX_PITCH);
	Apply();
}

float FirstPersonLook::GetYaw() const	{ return m_yaw; }
float FirstPersonLook::GetPitch() const	{ return m_pitch; }

Vector3 FirstPersonLook::GetPlanarForward() const
{
	const float yaw = m_yaw * DEG2RAD;
	return { std::sin(yaw), 0.0f, -std::cos(yaw) };		// yaw 0 looks along -Z, positive yaw turns right
}

Vector3 FirstPersonLook::GetPlanarRight() const
{
	const float yaw = m_yaw * DEG2RAD;
	return { std::cos(yaw), 0.0f, std::sin(yaw) };
}

void FirstPersonLook::OnEnable()
{
	Apply();
}

void FirstPersonLook::Update(float /*deltaTime*/)
{
	if (m_input)
	{
		const Vector2 look = m_input->GetLook();	// zero while the player has no control
		m_yaw	+= look.x * SENSITIVITY;
		m_pitch	= std::clamp(m_pitch - look.y * SENSITIVITY, -MAX_PITCH, MAX_PITCH);	// screen y grows downwards
	}
	Apply();
}

void FirstPersonLook::Apply()
{
	eng::GameActor* owner = GetOwner();
	if (!owner)
		return;

	// Body: yaw about +Y (counter-clockwise), the camera's positive yaw turns right
	owner->SetRotationEuler({ 0.0f, -m_yaw, 0.0f });

	if (m_camera)
	{
		m_camera->SetYawPitch(m_yaw, m_pitch);
		m_camera->SetPosition(Vector3Add(owner->GetWorldPosition(), { 0.0f, EYE_HEIGHT, 0.0f }));
	}
}
