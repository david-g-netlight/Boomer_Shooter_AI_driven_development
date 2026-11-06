#include "ParticleEmitter.h"
#include "core/Engine.h"
#include "level/GameActor.h"
#include <raymath.h>

namespace eng
{
	ParticleEmitter::ParticleEmitter(const ParticleEmitterSettings& settings)
		: m_settings(settings)
	{
	}

	void ParticleEmitter::SetSettings(const ParticleEmitterSettings& settings)	{ m_settings = settings; }
	ParticleEmitterSettings& ParticleEmitter::GetSettings()						{ return m_settings; }
	const ParticleEmitterSettings& ParticleEmitter::GetSettings() const			{ return m_settings; }

	void ParticleEmitter::Emit()
	{
		const GameActor* owner = GetOwner();
		if (!owner)
			return;

		// Local -Z (World::worldForward) in world space
		const Matrix	world	= owner->GetWorldMatrix();
		const Vector3	forward	= Vector3Normalize({ -world.m8, -world.m9, -world.m10 });
		Emit(forward);
	}

	void ParticleEmitter::Emit(const Vector3& direction)
	{
		if (const GameActor* owner = GetOwner())
			Engine::GetInstance().GetParticles().Emit(m_settings, owner->GetWorldPosition(), direction);
	}
}
