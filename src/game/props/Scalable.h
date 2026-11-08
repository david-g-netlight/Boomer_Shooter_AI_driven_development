#pragma once
#include <raylib.h>
#include <eng.h>

/// <summary>
/// A prop the scaling gun can grow and shrink (Unity ScalingController + ScalingEnabler + ScalingAudioFX).
///
/// TriggerScaling(multiplier) starts a smooth change: for 1 / speed seconds the scale follows
/// startScale * SmoothStep(1, multiplier, t). Every fixed step the actor scale is written, the Collider is
/// rescaled and the RigidBody recomputes inertia (and mass, with `scaleMass`) and wakes up. With `keepBottom` the
/// lowest point of the collider stays where it was, so a prop grows upward instead of into the floor.
///
/// Rules: a prop that is still scaling ignores new shots; stretching stops at MAX_RATIO times the scale it
/// had before the first shot (the target is clamped, so it never overshoots); once the largest axis is below
/// MIN_SIZE after a shrink, the prop is destroyed. A stretch is refused when the prop at its target size would
/// overlap static level geometry (layer Ground), so it can't grow into a wall or ceiling. The sound plays when a change actually starts.
/// Needs a Collider (and normally a RigidBody) on the same actor, added before it.
/// </summary>
class Scalable : public eng::Component
{
public:
	static constexpr float	DEFAULT_SPEED	= 5.0f;		// 1 / seconds: a change takes 0.2 s
	static constexpr float	MAX_RATIO		= 1.9f;		// of the base scale
	static constexpr float	MIN_SIZE		= 0.1f;		// m: the largest axis below this ends the prop

	/// <summary>
	/// multiplier > 1 stretches, < 1 shrinks (1 + factor * sign). False when nothing started: already scaling,
	/// already at the cap, or no change.
	/// </summary>
	bool			TriggerScaling(float multiplier);

	/// <summary>
	/// Would the prop, scaled by `multiplier`, intersect static level geometry? (Its own bottom resting on the floor
	/// doesn't count.)
	/// </summary>
	bool			WouldOverlapStatic(float multiplier) const;

	bool			IsScaling() const;
	float			GetScaleRatio() const;		// current / base scale (largest axis)
	float			GetSpeed() const;
	void			SetSpeed(float speed);

	void			FixedUpdate(float fixedDeltaTime) override;
	void			DrawInspector(eng::InspectorLayout& ui) override;
	void			Serialize(eng::PropertyBag& out) const override;
	void			Deserialize(const eng::PropertyBag& in) override;
	const char*		TypeName() const override { return "Scalable"; }

private:
	void			CaptureBase();				// scale and mass the ratio is measured from
	void			ApplyMultiplier(float multiplier);
	float			GetLowestPoint() const;

	float			m_speed			= DEFAULT_SPEED;
	bool			m_scaleMass		= true;
	bool			m_keepBottom	= true;

	bool			m_isScaling		= false;
	float			m_t				= 0.0f;
	float			m_target		= 1.0f;
	Vector3			m_startScale	= { 1.0f, 1.0f, 1.0f };
	float			m_startBottom	= 0.0f;

	bool			m_hasBase		= false;
	Vector3			m_baseScale		= { 1.0f, 1.0f, 1.0f };
	float			m_baseMass		= 1.0f;
	Vector3			m_lastScale		= { 1.0f, 1.0f, 1.0f };		// what this component wrote last: a different scale means an editor edit
};
