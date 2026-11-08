#include "RayBeamSpawner.h"
#include <raymath.h>
#include "fx/MuzzleFX.h"
#include "fx/RayBeam.h"
#include "player/ScalingGun.h"
#include "state/SoundFXBank.h"

RayBeamSpawner::~RayBeamSpawner()
{
	Shutdown();
}

void RayBeamSpawner::Init(eng::Level* level)
{
	Shutdown();
	m_level	= level;
	m_token	= eng::Engine::GetInstance().GetEvents().Subscribe<ScalingShotEvent>([this](const ScalingShotEvent& shot) { Spawn(shot); });
}

void RayBeamSpawner::Shutdown()
{
	if (m_token != 0)
		eng::Engine::GetInstance().GetEvents().Unsubscribe(m_token);
	m_token = 0;
	m_level = nullptr;
}

void RayBeamSpawner::Spawn(const ScalingShotEvent& shot)
{
	if (!m_level)
		return;

	const Vector3 direction = Vector3Normalize(Vector3Subtract(shot.point, shot.muzzle));
	MuzzleFX::Burst(shot.sign, shot.muzzle, direction);

	eng::GameActor* beam = m_level->CreateGameActor("RayBeam");
	beam->AddComponent<eng::LineRenderer>();
	beam->AddComponent<RayBeam>()->Launch(shot.sign, shot.muzzle, shot.point);

	if (shot.hasHit)
		RayBeam::EmitImpact(shot.sign, shot.point, shot.normal);

	SoundFXBank::Instance().Play(Sfx::Beam, 0.3f, shot.sign > 0.0f ? 1.0f : 0.8f);
}
