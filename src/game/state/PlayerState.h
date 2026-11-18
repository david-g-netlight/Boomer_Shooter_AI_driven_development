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
};
