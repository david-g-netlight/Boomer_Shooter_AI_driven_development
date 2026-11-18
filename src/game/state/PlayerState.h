#pragma once

/// <summary>
/// The player's numbers (Unity PlayerState): walk speed, gun reach and the gun cooldown. Plain data plus rules,
/// no actors: the controller and the gun read and write it.
/// </summary>
class PlayerState
{
public:
	static constexpr float	WALK_SPEED		= 10.0f;	// m/s
	static constexpr float	RAY_REACH		= 30.0f;	// m
	static constexpr float	COOLDOWN		= 1.0f;		// s between shots

	/// <summary>
	/// A ready gun (a level starts).
	/// </summary>
	void	Reset();

	//////////////////////////////////////////
	/// Gun cooldown
	//////////////////////////////////////////
	bool	IsReadyToShoot() const;
	void	StartCooldown();
	void	TickCooldown(float deltaTime);			// FixedUpdate
	float	GetCooldownLeft() const;
	float	GetCooldownFraction() const;			// 0 ready .. 1 just fired (HUD bar)

private:
	float	m_cooldownLeft	= 0.0f;
};
