#include "Gun.h"
#include <raymath.h>
#include "player/PlayerInput.h"
#include "state/PlayerState.h"

namespace
{
	constexpr uint32_t SHOT_MASK = eng::Layer::Ground | eng::Layer::Prop;
}

void Gun::Bind(PlayerInput* input, PlayerState* state, eng::Camera* camera, const HitscanTargets* targets)
{
	m_input		= input;
	m_state		= state;
	m_camera	= camera;
	m_targets	= targets;
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

	// An enemy in front of the wall (or the prop) takes the aim
	const float maxDistance = m_aim.hasHit ? m_aim.distance : m_state->GetReach();
	if (m_targets)
	{
		if (const auto enemy = m_targets->Raycast(m_camera->GetPosition(), m_camera->Forward(), maxDistance))
		{
			m_aim.hasHit	= true;
			m_aim.isTarget	= true;
			m_aim.distance	= enemy->distance;
			m_aim.target	= {};
		}
	}
}

const GunAim& Gun::GetAimHit() const	{ return m_aim; }

Vector3 Gun::MuzzlePoint() const
{
	const Vector3 offset = m_state->GetMuzzleOffset();
	return Vector3Add(m_camera->GetPosition(), Vector3Add(Vector3Scale(m_camera->Right(), offset.x),
				Vector3Add(Vector3Scale(m_camera->Up(), offset.y), Vector3Scale(m_camera->Forward(), offset.z))));
}

void Gun::DrawMuzzleProbe() const
{
	if (!m_state || !m_camera || !m_state->IsMuzzleProbeOn())
		return;

	eng::DebugDraw& debug = eng::Engine::GetInstance().GetDebugDraw();
	if (!debug.IsEnabled())
		debug.SetEnabled(true);	// the engine drops debug items while it is disabled

	const Vector3 muzzle = MuzzlePoint();
	const Vector3 offset = m_state->GetMuzzleOffset();
	debug.WireSphere(muzzle, 0.02f, YELLOW);
	debug.Axes(muzzle, 0.1f);
	// The tracer's path: from the muzzle to where the screen center aims (the hit, or the end of the reach)
	const float distance = m_aim.hasHit ? m_aim.distance : m_state->GetReach();
	debug.Line(muzzle, Vector3Add(m_camera->GetPosition(), Vector3Scale(m_camera->Forward(), distance)), YELLOW);
	debug.Label(muzzle, std::string(TextFormat("muzzle %.2f %.2f %.2f", offset.x, offset.y, offset.z)), YELLOW);
}

void Gun::FixedUpdate(float fixedDeltaTime)
{
	if (!m_input || !m_state)
		return;

	m_state->TickCooldown(fixedDeltaTime);
	m_state->TickHurtFlash(fixedDeltaTime);
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
	shot.origin		= origin;
	shot.muzzle		= MuzzlePoint();
	shot.point		= Vector3Add(origin, Vector3Scale(forward, reach));

	if (const auto hit = eng::Engine::GetInstance().GetPhysics().Raycast(origin, forward, reach, SHOT_MASK))
	{
		shot.hasHit	= true;
		shot.point	= hit->point;
		shot.normal	= hit->normal;
		shot.target	= hit->actor;
	}

	// The wall (or prop) distance first, then the enemies in front of it
	if (m_targets)
	{
		const float maxDistance = shot.hasHit ? Vector3Distance(origin, shot.point) : reach;
		if (const auto enemy = m_targets->Raycast(origin, forward, maxDistance))
		{
			shot.hasHit	= true;
			shot.point	= enemy->point;
			shot.normal	= enemy->normal;
			shot.target	= {};
			shot.enemy	= enemy->entity;
			shot.damage	= PlayerState::GUN_DAMAGE;
		}
	}

	m_lastShot = shot;
	++m_shotCount;
	eng::Engine::GetInstance().GetEvents().Publish(shot);
	return true;
}

const ShotFiredEvent& Gun::GetLastShot() const	{ return m_lastShot; }
int Gun::GetShotCount() const					{ return m_shotCount; }
