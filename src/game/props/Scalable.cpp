#include "Scalable.h"
#include <algorithm>
#include <cmath>
#include "state/SoundFXBank.h"

namespace
{
	float SmoothStep(float t)
	{
		return t * t * (3.0f - 2.0f * t);
	}

	float LargestAxis(const Vector3& v)
	{
		return std::max({ std::fabs(v.x), std::fabs(v.y), std::fabs(v.z) });
	}

	bool IsNear(const Vector3& a, const Vector3& b)
	{
		return std::fabs(a.x - b.x) < 1e-4f && std::fabs(a.y - b.y) < 1e-4f && std::fabs(a.z - b.z) < 1e-4f;
	}
}

void Scalable::CaptureBase()
{
	eng::GameActor* owner = GetOwner();
	const Vector3 scale = owner->GetTransform().scale;

	// Same scale as we left it: the base stays. Anything else (first use, the editor moved the gizmo) starts over
	if (m_hasBase && IsNear(scale, m_lastScale))
		return;

	m_hasBase	= true;
	m_baseScale	= scale;
	m_lastScale	= scale;
	const eng::RigidBody* body = owner->GetComponent<eng::RigidBody>();
	m_baseMass	= body ? body->GetMass() : 1.0f;
}

float Scalable::GetScaleRatio() const
{
	const Vector3 s = GetOwner()->GetTransform().scale;
	const Vector3 b = m_hasBase ? m_baseScale : s;
	return std::max({ s.x / b.x, s.y / b.y, s.z / b.z });
}

bool Scalable::TriggerScaling(float multiplier)
{
	if (m_isScaling || multiplier <= 0.0f)
		return false;

	CaptureBase();

	float target = multiplier;
	if (multiplier > 1.0f)
	{
		const float ratio = GetScaleRatio();
		if (ratio >= MAX_RATIO - 1e-4f)
			return false;						// at the cap
		target = std::min(multiplier, MAX_RATIO / ratio);
	}
	if (std::fabs(target - 1.0f) < 1e-4f)
		return false;

	m_isScaling		= true;
	m_t				= 0.0f;
	m_target		= target;
	m_startScale	= GetOwner()->GetTransform().scale;
	m_startBottom	= GetLowestPoint();

	// Unity ScalingAudioFX: the pitch follows the factor, up for a stretch and down for a shrink
	SoundFXBank::Instance().PlayScaling(multiplier > 1.0f ? +1.0f : -1.0f, std::fabs(multiplier - 1.0f));
	return true;
}

float Scalable::GetLowestPoint() const
{
	const eng::Collider* collider = GetOwner()->GetComponent<eng::Collider>();
	return collider ? collider->GetWorldBounds().min.y : GetOwner()->GetWorldPosition().y;
}

void Scalable::FixedUpdate(float fixedDeltaTime)
{
	if (!m_isScaling)
		return;

	m_t = std::min(1.0f, m_t + m_speed * fixedDeltaTime);
	ApplyMultiplier(1.0f + (m_target - 1.0f) * SmoothStep(m_t));

	if (m_t < 1.0f)
		return;

	m_isScaling = false;
	m_lastScale = GetOwner()->GetTransform().scale;
	if (m_target < 1.0f && LargestAxis(m_lastScale) < MIN_SIZE)
		GetOwner()->MarkForDestroy();	// the body leaves the world with the actor at the end of the frame
}

void Scalable::ApplyMultiplier(float multiplier)
{
	eng::GameActor*	owner	= GetOwner();
	eng::RigidBody*	body	= owner->GetComponent<eng::RigidBody>();
	eng::Collider*	collider = owner->GetComponent<eng::Collider>();

	owner->SetScale({ m_startScale.x * multiplier, m_startScale.y * multiplier, m_startScale.z * multiplier });

	if (body)
	{
		if (m_scaleMass)
			body->SetMass(std::max(0.01f, m_baseMass * std::pow(LargestAxis(owner->GetTransform().scale) / LargestAxis(m_baseScale), 3.0f)));
		body->OnScaleChanged();		// collider scale, inertia, AABB, wake up
	}
	else if (collider)
		collider->SyncScaleFromActor();

	// Keep the lowest point in place: grow upward, not into the floor. Not for children (their parent moves them)
	if (m_keepBottom && !owner->GetParent())
	{
		const float shift = m_startBottom - GetLowestPoint();
		if (std::fabs(shift) > 1e-5f)
		{
			::Transform pose = owner->GetTransform();
			pose.translation.y += shift;
			if (body)
				body->Teleport(pose, true);		// keeps the velocity of a falling prop
			else
				owner->GetTransform() = pose;
		}
	}
	m_lastScale = owner->GetTransform().scale;
}

bool Scalable::IsScaling() const	{ return m_isScaling; }
float Scalable::GetSpeed() const	{ return m_speed; }
void Scalable::SetSpeed(float speed){ m_speed = std::max(0.1f, speed); }

void Scalable::DrawInspector(eng::InspectorLayout& ui)
{
	float speed = m_speed;
	if (ui.Float("Speed", speed))
		SetSpeed(speed);
	ui.Bool("Scale mass", m_scaleMass);
	ui.Bool("Keep bottom", m_keepBottom);

	char text[32];
	std::snprintf(text, sizeof(text), "x%.2f of %.2f cap", static_cast<double>(m_hasBase ? GetScaleRatio() : 1.0f), static_cast<double>(MAX_RATIO));
	ui.Info("Size", text);
}

void Scalable::Serialize(eng::PropertyBag& out) const
{
	out.SetFloat("speed", m_speed);
	out.SetBool("scaleMass", m_scaleMass);
	out.SetBool("keepBottom", m_keepBottom);
}

void Scalable::Deserialize(const eng::PropertyBag& in)
{
	SetSpeed(in.GetFloat("speed", DEFAULT_SPEED));
	m_scaleMass		= in.GetBool("scaleMass", true);
	m_keepBottom	= in.GetBool("keepBottom", true);
}
