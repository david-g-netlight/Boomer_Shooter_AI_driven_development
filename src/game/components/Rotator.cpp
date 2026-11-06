#include "Rotator.h"
#include <raymath.h>

Rotator::Rotator(float degreesPerSecond, Vector3 axis)
	: m_degreesPerSecond(degreesPerSecond)
	, m_axis(Vector3Normalize(axis))
{
}

void Rotator::SetSpeed(float degreesPerSecond)	{ m_degreesPerSecond = degreesPerSecond; }
float Rotator::GetSpeed() const					{ return m_degreesPerSecond; }

void Rotator::Update(float deltaTime)
{
	::Transform&		transform	= GetOwner()->GetTransform();
	const Quaternion	step		= QuaternionFromAxisAngle(m_axis, m_degreesPerSecond * DEG2RAD * deltaTime);
	transform.rotation = QuaternionNormalize(QuaternionMultiply(transform.rotation, step));
}
