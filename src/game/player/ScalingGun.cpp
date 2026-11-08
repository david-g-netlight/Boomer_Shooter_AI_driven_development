#include "ScalingGun.h"
#include <raymath.h>
#include "player/PlayerInput.h"
#include "state/PlayerState.h"

void ScalingGun::Bind(PlayerInput* input, PlayerState* state, eng::Camera* camera)
{
	m_input		= input;
	m_state		= state;
	m_camera	= camera;
}

void ScalingGun::FixedUpdate(float fixedDeltaTime)
{
	if (!m_input || !m_state)
		return;

	m_state->TickCooldown(fixedDeltaTime);
	if (const int sign = m_input->ConsumeFire())
		Fire(static_cast<float>(sign));
}

bool ScalingGun::Fire(float sign)
{
	if (!m_state || !m_camera || !m_state->IsReadyToShoot())
		return false;

	m_state->StartCooldown();

	const Vector3 origin	= m_camera->GetPosition();
	const Vector3 forward	= m_camera->Forward();

	ScalingShotEvent shot;
	shot.sign		= sign > 0.0f ? +1.0f : -1.0f;
	shot.factor		= m_state->GetFactor();
	shot.muzzle		= Vector3Add(origin, Vector3Add(Vector3Scale(m_camera->Right(), MUZZLE_RIGHT),
					  Vector3Add(Vector3Scale(m_camera->Up(), -MUZZLE_DOWN), Vector3Scale(forward, MUZZLE_FORWARD))));
	shot.hasHit		= false;
	shot.point		= Vector3Add(origin, Vector3Scale(forward, PlayerState::RAY_REACH));
	shot.normal		= { 0.0f, 1.0f, 0.0f };

	if (const auto hit = eng::Engine::GetInstance().GetPhysics().Raycast(origin, forward, PlayerState::RAY_REACH,
			eng::Layer::Ground | eng::Layer::Prop))
	{
		shot.hasHit	= true;
		shot.point	= hit->point;
		shot.normal	= hit->normal;
		shot.target	= hit->actor;
	}

	m_lastShot = shot;
	++m_shotCount;
	eng::Engine::GetInstance().GetEvents().Publish(shot);
	return true;
}

const ScalingShotEvent& ScalingGun::GetLastShot() const	{ return m_lastShot; }
int ScalingGun::GetShotCount() const					{ return m_shotCount; }
