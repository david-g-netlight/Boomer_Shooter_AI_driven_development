#pragma once
#include <eng.h>

/// <summary>
/// Spins its actor around a local axis at a constant speed (degrees per second).
/// </summary>
class Rotator : public eng::Component
{
public:
	explicit Rotator(float degreesPerSecond, Vector3 axis = { 0.0f, 1.0f, 0.0f });

	void		SetSpeed(float degreesPerSecond);
	float		GetSpeed() const;

	void		Update(float deltaTime) override;
	const char*	TypeName() const override { return "Rotator"; }

private:
	float		m_degreesPerSecond;
	Vector3		m_axis;
};
