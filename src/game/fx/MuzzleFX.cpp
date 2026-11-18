#include "MuzzleFX.h"
#include <eng.h>

void MuzzleFX::Burst(const Vector3& position, const Vector3& direction)
{
	const Color start	= START_COLOR;
	const Color end		= END_COLOR;
	eng::ParticleSystem& particles = eng::Engine::GetInstance().GetParticles();

	// Sparks: a narrow fast cone along the beam
	eng::ParticleEmitterSettings sparks;
	sparks.burstCount		= 14;
	sparks.lifetime			= 0.35f;
	sparks.speed			= 5.0f;
	sparks.spreadDegrees	= 20.0f;
	sparks.startSize		= 0.05f;
	sparks.endSize			= 0.01f;
	sparks.gravity			= { 0.0f, -2.0f, 0.0f };
	sparks.additive			= true;
	sparks.startColor		= start;
	sparks.endColor			= end;
	particles.Emit(sparks, position, direction);

	// Wave: a few big slow glows that fade out around the muzzle
	eng::ParticleEmitterSettings wave;
	wave.burstCount			= 5;
	wave.lifetime			= 0.25f;
	wave.speed				= 0.8f;
	wave.spreadDegrees		= 180.0f;
	wave.startSize			= 0.06f;
	wave.endSize			= 0.16f;
	wave.gravity			= { 0.0f, 0.0f, 0.0f };
	wave.additive			= true;
	wave.startColor			= { start.r, start.g, start.b, 140 };
	wave.endColor			= { end.r, end.g, end.b, 0 };
	particles.Emit(wave, position, direction);
}
