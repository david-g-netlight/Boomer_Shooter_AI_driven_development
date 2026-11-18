#pragma once
#include <raylib.h>
#include <eng.h>

/// <summary>
/// The tracer of one shot (Unity RayBeamController + BeamImpactParticle). A LineRenderer runs from the tail to
/// the hit point; the tail travels toward the hit at SPEED and the tracer destroys itself when it gets there.
/// One colour (MuzzleFX). Needs a LineRenderer on the same actor.
/// </summary>
class Tracer : public eng::Component
{
public:
	static constexpr float	SPEED	= 80.0f;	// m/s of the tail

	/// <summary>
	/// Sets the colours and the two ends.
	/// </summary>
	void			Launch(const Vector3& tail, const Vector3& head);

	/// <summary>
	/// Burst where the shot hits a surface: sparks along the normal, alpha 1 to 0.
	/// </summary>
	static void		EmitImpact(const Vector3& point, const Vector3& normal);

	void			Update(float deltaTime) override;
	const char*		TypeName() const override { return "Tracer"; }

private:
	Vector3			m_tail	= { 0.0f, 0.0f, 0.0f };
	Vector3			m_head	= { 0.0f, 0.0f, 0.0f };
};
