#include "Tracer.h"
#include <raymath.h>
#include "fx/MuzzleFX.h"

void Tracer::Launch(const Vector3& tail, const Vector3& head)
{
	m_tail = tail;
	m_head = head;

	if (eng::LineRenderer* line = GetOwner()->GetComponent<eng::LineRenderer>())
	{
		line->SetPoints(m_tail, m_head);
		line->SetColors({ 255, 150, 60, 140 }, { 255, 235, 170, 230 });	// faint tail, bright head
		line->SetWidth(0.025f, 0.035f);
	}
}

void Tracer::Update(float deltaTime)
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

void Tracer::EmitImpact(const Vector3& point, const Vector3& normal)
{
	eng::ParticleEmitterSettings impact;
	impact.burstCount		= 40;
	impact.speed			= 3.5f;
	impact.spreadDegrees	= 70.0f;
	impact.gravity			= { 0.0f, -3.0f, 0.0f };
	impact.additive			= true;
	impact.startColor		= MuzzleFX::START_COLOR;
	impact.endColor			= MuzzleFX::END_COLOR;
	eng::Engine::GetInstance().GetParticles().Emit(impact, point, normal);
}
