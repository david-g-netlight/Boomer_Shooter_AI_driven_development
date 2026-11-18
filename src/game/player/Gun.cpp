#include "Gun.h"
#include <raymath.h>
#include "player/PlayerInput.h"
#include "state/PlayerState.h"

namespace
{
	constexpr uint32_t SHOT_MASK = eng::Layer::Ground | eng::Layer::Prop;
}

void Gun::Bind(PlayerInput* input, PlayerState* state, eng::Camera* camera)
{
	m_input		= input;
	m_state		= state;
	m_camera	= camera;
}

void Gun::Update(float /*deltaTime*/)
{
	m_aim = {};
	if (!m_camera || !m_state)
		return;

	if (const auto hit = eng::Engine::GetInstance().GetPhysics().Raycast(m_camera->GetPosition(), m_camera->Forward(),
			m_state->GetReach(), SHOT_MASK))
	{
		m_aim.hasHit	= true;
		m_aim.isTarget	= hit->collider && hit->collider->GetLayer() == eng::Layer::Prop;
		m_aim.distance	= hit->distance;
		m_aim.target	= hit->actor;
	}
}

const GunAim& Gun::GetAimHit() const	{ return m_aim; }

void Gun::FixedUpdate(float fixedDeltaTime)
{
	if (!m_input || !m_state)
		return;

	m_state->TickCooldown(fixedDeltaTime);
	if (m_input->ConsumeFire())
		Fire();
}

bool Gun::Fire()
{
	if (!m_state || !m_camera || !m_state->IsReadyToShoot())
		return false;

	m_state->StartCooldown();

	const Vector3 origin	= m_camera->GetPosition();
	const Vector3 forward	= m_camera->Forward();
	const float reach		= m_state->GetReach();

	ShotFiredEvent shot;
	shot.muzzle		= Vector3Add(origin, Vector3Add(Vector3Scale(m_camera->Right(), MUZZLE_RIGHT),
					  Vector3Add(Vector3Scale(m_camera->Up(), -MUZZLE_DOWN), Vector3Scale(forward, MUZZLE_FORWARD))));
	shot.hasHit		= false;
	shot.point		= Vector3Add(origin, Vector3Scale(forward, reach));
	shot.normal		= { 0.0f, 1.0f, 0.0f };

	if (const auto hit = eng::Engine::GetInstance().GetPhysics().Raycast(origin, forward, reach, SHOT_MASK))
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

const ShotFiredEvent& Gun::GetLastShot() const	{ return m_lastShot; }
int Gun::GetShotCount() const					{ return m_shotCount; }
