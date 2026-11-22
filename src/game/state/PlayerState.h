#pragma once

/// <summary>
/// The player's numbers (Unity PlayerState): walk speed, gun reach and the gun cooldown. Plain data plus rules,
/// no actors: the controller and the gun read and write it. Reach and cooldown are tunable at runtime (developer
/// console); Reset() keeps them.
/// </summary>
class PlayerState
{
public:
	static constexpr float	WALK_SPEED		= 10.0f;	// m/s
	static constexpr float	DEFAULT_REACH		= 100.0f;	// m
	static constexpr float	DEFAULT_COOLDOWN	= 0.25f;	// s between shots
	static constexpr float	GUN_DAMAGE			= 10.0f;	// per hit: a Drifter (30 hp) takes 3, a Stalker 6, a Brute 12

	/// <summary>
	/// A ready gun (a level starts).
	/// </summary>
	void	Reset();

	//////////////////////////////////////////
	/// Gun tuning
	//////////////////////////////////////////
	float	GetReach() const;
	void	SetReach(float meters);					// clamped to > 0
	float	GetCooldown() const;
	void	SetCooldown(float seconds);				// clamped to >= 0.01

	//////////////////////////////////////////
	/// Developer flag (console `god`): placeholder, read by the damage code once there is some. Reset() keeps it.
	//////////////////////////////////////////
	bool	IsGod() const;
	bool*	GetGodFlag();							// the console binds its variable to it

	//////////////////////////////////////////
	/// Gun cooldown
	//////////////////////////////////////////
	bool	IsReadyToShoot() const;
	void	StartCooldown();
	void	TickCooldown(float deltaTime);			// FixedUpdate
	float	GetCooldownLeft() const;
	float	GetCooldownFraction() const;			// 0 ready .. 1 just fired (HUD bar)

private:
	float	m_reach			= DEFAULT_REACH;
	float	m_cooldown		= DEFAULT_COOLDOWN;
	float	m_cooldownLeft	= 0.0f;
	bool	m_god			= false;
};
