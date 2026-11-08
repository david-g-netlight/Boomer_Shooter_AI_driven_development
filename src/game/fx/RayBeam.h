#pragma once
#include <raylib.h>
#include <eng.h>

/// <summary>
/// The beam of one shot (Unity RayBeamController + BeamImpactParticle). A LineRenderer runs from the tail to
/// the hit point; the tail travels toward the hit at SPEED and the beam destroys itself when it gets there.
/// Colors: stretch blue to green, shrink yellow to red. Needs a LineRenderer on the same actor.
/// </summary>
class RayBeam : public eng::Component
{
public:
	static constexpr float	SPEED	= 20.0f;	// m/s of the tail

	/// <summary>
	/// Sets the colors and the two ends. sign > 0 stretch, < 0 shrink.
	/// </summary>
	void			Launch(float sign, const Vector3& tail, const Vector3& head);

	/// <summary>
	/// Burst where the beam hits a surface: blue (stretch) or yellow (shrink) glow, alpha 1 to 0.
	/// </summary>
	static void		EmitImpact(float sign, const Vector3& point, const Vector3& normal);

	void			Update(float deltaTime) override;
	const char*		TypeName() const override { return "RayBeam"; }

private:
	Vector3			m_tail	= { 0.0f, 0.0f, 0.0f };
	Vector3			m_head	= { 0.0f, 0.0f, 0.0f };
};
