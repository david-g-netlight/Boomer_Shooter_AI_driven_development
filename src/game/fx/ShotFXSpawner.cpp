#include "ShotFXSpawner.h"
#include <raymath.h>
#include "fx/MuzzleFX.h"
#include "fx/Tracer.h"
#include "player/Gun.h"
#include "state/SoundFXBank.h"

ShotFXSpawner::~ShotFXSpawner()
{
	Shutdown();
}

void ShotFXSpawner::Init(eng::Level* level)
{
	Shutdown();
	m_level	= level;
	m_token	= eng::Engine::GetInstance().GetEvents().Subscribe<ShotFiredEvent>([this](const ShotFiredEvent& shot) { Spawn(shot); });
}

void ShotFXSpawner::Shutdown()
{
	if (m_token != 0)
		eng::Engine::GetInstance().GetEvents().Unsubscribe(m_token);
	m_token = 0;
	m_level = nullptr;
}

void ShotFXSpawner::Spawn(const ShotFiredEvent& shot)
{
	if (!m_level)
		return;

	const Vector3 direction = Vector3Normalize(Vector3Subtract(shot.point, shot.muzzle));
	MuzzleFX::Burst(shot.muzzle, direction);

	eng::GameActor* tracer = m_level->CreateGameActor("Tracer");
	tracer->AddComponent<eng::LineRenderer>();
	tracer->AddComponent<Tracer>()->Launch(shot.muzzle, shot.point);

	if (shot.hasHit)
		Tracer::EmitImpact(shot.point, shot.normal);

	SoundFXBank::Instance().Play(Sfx::Shot);
}
