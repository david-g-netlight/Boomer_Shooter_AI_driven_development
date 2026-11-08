#pragma once
#include <raylib.h>
#include <eng.h>

class PlayerInput;
class PlayerState;

/// <summary>
/// Published on the EventBus for every shot that left the gun (Unity ScalingProvider result). The beam, muzzle
/// effects and sounds listen to it.
/// </summary>
struct ScalingShotEvent
{
	float			sign;			// +1 stretch, -1 shrink
	float			factor;			// PlayerState factor at the moment of the shot
	bool			hasHit;
	Vector3			muzzle;			// where the beam starts
	Vector3			point;			// the hit point, or the end of the reach when nothing was hit
	Vector3			normal;			// surface normal at the hit (up when nothing was hit)
	eng::ActorHandle target;		// the actor that was hit
};

/// <summary>
/// The scaling gun (Unity ScalingProvider). Each fixed step it ticks the cooldown and takes the buffered click
/// from PlayerInput. Fire(sign) does nothing while the cooldown runs; otherwise the shot starts the cooldown and
/// casts a ray from the screen center (the camera position along its forward, reach from PlayerState) against
/// Ground | Prop, so the player capsule and triggers are ignored.
/// </summary>
class ScalingGun : public eng::Component
{
public:
	/// <summary>
	/// Beam start relative to the camera (Unity RayBeamProvider): a gun held low and to the right.
	/// </summary>
	static constexpr float	MUZZLE_RIGHT	= 0.3f;
	static constexpr float	MUZZLE_DOWN		= 0.25f;
	static constexpr float	MUZZLE_FORWARD	= 0.6f;

	void			Bind(PlayerInput* input, PlayerState* state, eng::Camera* camera);

	/// <summary>
	/// false while the gun is cooling down (or not bound).
	/// </summary>
	bool			Fire(float sign);

	/// <summary>
	/// The same ray as a shot, cast every frame: is the center of the screen on a Scalable prop within reach?
	/// (crosshair color)
	/// </summary>
	bool			IsAimingAtScalable() const;

	const ScalingShotEvent&	GetLastShot() const;
	int				GetShotCount() const;

	void			Update(float deltaTime) override;
	void			FixedUpdate(float fixedDeltaTime) override;
	const char*		TypeName() const override { return "ScalingGun"; }

private:
	PlayerInput*		m_input			= nullptr;
	PlayerState*		m_state			= nullptr;
	eng::Camera*		m_camera		= nullptr;
	ScalingShotEvent	m_lastShot		= {};
	int					m_shotCount		= 0;
	bool				m_isAimingAtScalable = false;
};
