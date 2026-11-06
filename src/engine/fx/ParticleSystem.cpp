#include "ParticleSystem.h"
#include "render/Camera.h"
#include <algorithm>
#include <cmath>
#include <raymath.h>
#include <rlgl.h>

namespace eng
{
	ParticleSystem::ParticleSystem()
		: m_particles()
		, m_random(0x5CA1E)	// fixed seed: the same burst sequence every run
	{
	}

	float ParticleSystem::RandomRange(float minimum, float maximum)
	{
		return std::uniform_real_distribution<float>(minimum, maximum)(m_random);
	}

	Vector3 ParticleSystem::RandomInCone(const Vector3& direction, float spreadDegrees)
	{
		const Vector3 axis = Vector3LengthSqr(direction) > 1e-8f ? Vector3Normalize(direction) : Vector3{ 0.0f, 1.0f, 0.0f };

		// Uniform over the spherical cap: cos(theta) uniform in [cos(spread), 1]
		const float cosSpread	= std::cos(std::clamp(spreadDegrees, 0.0f, 180.0f) * DEG2RAD);
		const float cosTheta	= RandomRange(cosSpread, 1.0f);
		const float sinTheta	= std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
		const float phi			= RandomRange(0.0f, 2.0f * PI);

		// Basis around the axis
		const Vector3 helper	= std::fabs(axis.y) < 0.99f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
		const Vector3 tangent	= Vector3Normalize(Vector3CrossProduct(helper, axis));
		const Vector3 bitangent	= Vector3CrossProduct(axis, tangent);

		return Vector3Add(Vector3Scale(axis, cosTheta),
			   Vector3Add(Vector3Scale(tangent, sinTheta * std::cos(phi)), Vector3Scale(bitangent, sinTheta * std::sin(phi))));
	}

	void ParticleSystem::Emit(const ParticleEmitterSettings& settings, const Vector3& position, const Vector3& direction)
	{
		for (int i = 0; i < settings.burstCount; ++i)
		{
			if (m_aliveCount >= MAX_PARTICLES)
			{
				m_droppedCount += static_cast<size_t>(settings.burstCount - i);
				return;
			}

			Particle& p		= m_particles[m_aliveCount++];
			p.position		= position;
			p.velocity		= Vector3Scale(RandomInCone(direction, settings.spreadDegrees),
								settings.speed * RandomRange(1.0f - settings.speedVariation, 1.0f + settings.speedVariation));
			p.gravity		= settings.gravity;
			p.age			= 0.0f;
			p.lifetime		= std::max(0.01f, settings.lifetime
								* RandomRange(1.0f - settings.lifetimeVariation, 1.0f + settings.lifetimeVariation));
			p.drag			= settings.drag;
			p.startSize		= settings.startSize;
			p.endSize		= settings.endSize;
			p.startColor	= settings.startColor;
			p.endColor		= settings.endColor;
			p.additive		= settings.additive;
		}
	}

	void ParticleSystem::Update(float deltaTime)
	{
		size_t i = 0;
		while (i < m_aliveCount)
		{
			Particle& p = m_particles[i];
			p.age += deltaTime;
			if (p.age >= p.lifetime)
			{
				// Swap-remove: keep alive particles packed at the front
				p = m_particles[--m_aliveCount];
				continue;
			}

			p.velocity = Vector3Add(p.velocity, Vector3Scale(p.gravity, deltaTime));
			p.velocity = Vector3Scale(p.velocity, std::exp(-p.drag * deltaTime));
			p.position = Vector3Add(p.position, Vector3Scale(p.velocity, deltaTime));
			++i;
		}
	}

	void ParticleSystem::Draw(const Camera& camera) const
	{
		if (m_aliveCount == 0)
			return;

		// raylib's 1x1 white texture, tinted per particle
		const Texture2D white	= { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
		const Camera3D view		= camera.ToRaylib();

		// Transparent: test depth against the scene but don't write it
		rlDrawRenderBatchActive();
		rlDisableDepthMask();
		for (int pass = 0; pass < 2; ++pass)
		{
			const bool additive = pass == 1;
			BeginBlendMode(additive ? BLEND_ADDITIVE : BLEND_ALPHA);
			for (size_t i = 0; i < m_aliveCount; ++i)
			{
				const Particle& p = m_particles[i];
				if (p.additive != additive)
					continue;
				const float t = p.age / p.lifetime;
				DrawBillboard(view, white, p.position, Lerp(p.startSize, p.endSize, t), ColorLerp(p.startColor, p.endColor, t));
			}
			EndBlendMode();
		}
		rlDrawRenderBatchActive();
		rlEnableDepthMask();
	}

	void ParticleSystem::Clear()
	{
		m_aliveCount = 0;
	}

	size_t ParticleSystem::GetAliveCount() const	{ return m_aliveCount; }
	size_t ParticleSystem::GetDroppedCount() const	{ return m_droppedCount; }
}
