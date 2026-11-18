#pragma once
#include <raylib.h>
#include <eng.h>

class PlayerInput;
class PlayerState;

/// <summary>
/// Published on the EventBus for every shot that left the gun. The tracer, muzzle effects, the gun view and
/// sounds listen to it; damage will read `target` once there are enemies.
/// </summary>
struct ShotFiredEvent
{
	bool			hasHit;
	Vector3			muzzle;			// where the tracer starts
	Vector3			point;			// the hit point, or the end of the reach when nothing was hit
	Vector3			normal;			// surface normal at the hit (up when nothing was hit)
	eng::ActorHandle target;		// the actor that was hit
};

/// <summary>
/// What the centre of the screen is on this frame (crosshair). `isTarget`: the hit is something worth shooting
/// (layer Prop for now, enemies later).
/// </summary>
struct GunAim
{
	bool			hasHit		= false;
	bool			isTarget	= false;
	float			distance	= 0.0f;
	eng::ActorHandle target;
};

/// <summary>
/// The player's hitscan gun. Each fixed step it ticks the cooldown and takes the buffered click from
/// PlayerInput. Fire() does nothing while the cooldown runs; otherwise the shot starts the cooldown and casts a
/// ray from the screen center (the camera position along its forward, reach from PlayerState) against
/// Ground | Prop, so the player capsule and triggers are ignored. It reports what it hit; it deals no damage yet.
/// </summary>
class Gun : public eng::Component
{
public:
	/// <summary>
	/// Tracer start relative to the camera: a gun held low and to the right.
	/// </summary>
	static constexpr float	MUZZLE_RIGHT	= 0.3f;
	static constexpr float	MUZZLE_DOWN		= 0.25f;
	static constexpr float	MUZZLE_FORWARD	= 0.6f;

	void			Bind(PlayerInput* input, PlayerState* state, eng::Camera* camera);

	/// <summary>
	/// false while the gun is cooling down (or not bound).
	/// </summary>
	bool			Fire();

	/// <summary>
	/// The same ray as a shot, cast every frame (crosshair).
	/// </summary>
	const GunAim&	GetAimHit() const;

	const ShotFiredEvent&	GetLastShot() const;
	int				GetShotCount() const;

	void			Update(float deltaTime) override;
	void			FixedUpdate(float fixedDeltaTime) override;
	const char*		TypeName() const override { return "Gun"; }

private:
	PlayerInput*		m_input			= nullptr;
	PlayerState*		m_state			= nullptr;
	eng::Camera*		m_camera		= nullptr;
	ShotFiredEvent		m_lastShot		= {};
	int					m_shotCount		= 0;
	GunAim				m_aim;
};
