#include "RayBeam.h"
#include <raymath.h>

void RayBeam::Launch(float sign, const Vector3& tail, const Vector3& head)
{
	m_tail = tail;
	m_head = head;

	if (eng::LineRenderer* line = GetOwner()->GetComponent<eng::LineRenderer>())
	{
		line->SetPoints(m_tail, m_head);
		line->SetColors(sign > 0.0f ? Color{ 40, 120, 255, 230 } : Color{ 255, 220, 40, 230 },
						sign > 0.0f ? Color{ 40, 255, 120, 200 } : Color{ 255, 60, 20, 200 });
		line->SetWidth(0.06f, 0.02f);
	}
}

void RayBeam::Update(float deltaTime)
{
	const Vector3 toHead	= Vector3Subtract(m_head, m_tail);
	const float remaining	= Vector3Length(toHead);
	const float step		= SPEED * deltaTime;

	if (remaining <= step)
	{
		GetOwner()->MarkForDestroy();	// the tail reached the hit point
		return;
	}

	m_tail = Vector3Add(m_tail, Vector3Scale(toHead, step / remaining));
	if (eng::LineRenderer* line = GetOwner()->GetComponent<eng::LineRenderer>())
		line->SetPoints(m_tail, m_head);
}

void RayBeam::EmitImpact(float sign, const Vector3& point, const Vector3& normal)
{
	eng::ParticleEmitterSettings impact;
	impact.burstCount		= 40;
	impact.speed			= 3.5f;
	impact.spreadDegrees	= 70.0f;
	impact.gravity			= { 0.0f, -3.0f, 0.0f };
	impact.additive			= true;
	impact.startColor		= sign > 0.0f ? Color{ 40, 120, 255, 255 } : Color{ 255, 220, 40, 255 };
	impact.endColor			= sign > 0.0f ? Color{ 40, 255, 120, 0 }   : Color{ 255, 60, 20, 0 };
	eng::Engine::GetInstance().GetParticles().Emit(impact, point, normal);
}
