#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <raylib.h>

namespace eng
{
	class Camera;

	/// <summary>
	/// What one burst looks like. Color goes from startColor to endColor over each particle's life
	/// (alpha included: endColor.a = 0 fades out, like the Unity gradients' alpha keys 1 -> 0).
	/// </summary>
	struct ParticleEmitterSettings
	{
		int		burstCount			= 24;
		float	lifetime			= 0.6f;		// seconds
		float	lifetimeVariation	= 0.3f;		// +- fraction of lifetime
		float	speed				= 4.0f;		// m/s
		float	speedVariation		= 0.5f;		// +- fraction of speed
		float	spreadDegrees		= 35.0f;	// cone half-angle around the emit direction; 180 = sphere
		float	startSize			= 0.12f;	// m
		float	endSize				= 0.02f;
		Vector3	gravity				= { 0.0f, -4.0f, 0.0f };
		float	drag				= 1.5f;		// velocity loss per second (exponential)
		Color	startColor			= { 255, 255, 255, 255 };
		Color	endColor			= { 255, 255, 255, 0 };
		bool	additive			= false;	// glow: colors add up where particles overlap
	};

	/// <summary>
	/// All particles of the game in one fixed pool, owned by the Engine: no allocation per burst.
	/// Updated with game time (frozen while paused) and drawn as camera-facing quads after the opaque
	/// geometry, without writing depth. A full pool drops new particles (counted in GetDroppedCount()).
	/// </summary>
	class ParticleSystem
	{
	public:
		static constexpr size_t MAX_PARTICLES = 1024;

		ParticleSystem();

		void	Emit(const ParticleEmitterSettings& settings, const Vector3& position, const Vector3& direction);
		void	Update(float deltaTime);
		void	Draw(const Camera& camera) const;		// inside BeginMode3D / EndMode3D
		void	Clear();

		size_t	GetAliveCount() const;
		size_t	GetDroppedCount() const;

	private:
		struct Particle
		{
			Vector3	position;
			Vector3	velocity;
			Vector3	gravity;
			float	age;
			float	lifetime;
			float	drag;
			float	startSize;
			float	endSize;
			Color	startColor;
			Color	endColor;
			bool	additive;
		};

		Vector3	RandomInCone(const Vector3& direction, float spreadDegrees);
		float	RandomRange(float minimum, float maximum);

		std::array<Particle, MAX_PARTICLES>	m_particles;
		size_t								m_aliveCount	= 0;	// alive particles are packed at the front
		size_t								m_droppedCount	= 0;
		std::mt19937						m_random;
	};
}
