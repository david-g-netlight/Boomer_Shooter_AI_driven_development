#pragma once
#include <raylib.h>
#include <eng.h>
#include "HitscanTargets.h"

class PlayerInput;
class PlayerState;

/// <summary>
/// Published on the EventBus for every shot that left the gun. The tracer, muzzle effects, the gun view and
/// sounds listen to it; the EnemyManager reads `enemy` and `damage`.
/// </summary>
struct ShotFiredEvent
{
	bool			hasHit		= false;
	Vector3			muzzle		= { 0.0f, 0.0f, 0.0f };		// where the tracer starts
	Vector3			point		= { 0.0f, 0.0f, 0.0f };		// the hit point, or the end of the reach when nothing was hit
	Vector3			normal		= { 0.0f, 1.0f, 0.0f };		// surface normal at the hit (up when nothing was hit)
	eng::ActorHandle target;									// the actor that was hit
	Vector3			origin		= { 0.0f, 0.0f, 0.0f };		// the camera position: where the player stood
	eng::ecs::Entity enemy		= eng::ecs::INVALID_ENTITY;	// the enemy that was hit, if any
	float			damage		= 0.0f;						// for `enemy`
};

/// <summary>
/// What the centre of the screen is on this frame (crosshair). `isTarget`: the hit is something worth shooting
/// (layer Prop, or an enemy).
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
/// Ground | Prop, so the player capsule and triggers are ignored. An enemy (through HitscanTargets) is tested up to the wall distance. The shot reports what it hit and the damage for an enemy.
/// </summary>
class Gun : public eng::Component
{
public:
	/// <summary>
	/// Where the tracer and the muzzle flash start: the camera plus the muzzle offset of PlayerState.
	/// </summary>
	Vector3			MuzzlePoint() const;

	/// <summary>
	/// Debug: while `muzzle_probe` is on, draws the muzzle point (axes, aim line and its offset) for this frame.
	/// </summary>
	void			DrawMuzzleProbe() const;

	void			Bind(PlayerInput* input, PlayerState* state, eng::Camera* camera, const HitscanTargets* targets = nullptr);

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
	const HitscanTargets*	m_targets		= nullptr;
	ShotFiredEvent		m_lastShot		= {};
	int					m_shotCount		= 0;
	GunAim				m_aim;
};
