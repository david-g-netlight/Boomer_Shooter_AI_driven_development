#pragma once

/// <summary>
/// Published when the scale factor changes (HUD, SFX pitch).
/// </summary>
struct ScaleFactorChangedEvent
{
	int		index;
	float	factor;
};

/// <summary>
/// The player's numbers (Unity PlayerState): walk speed, the scale factor picked with the wheel and the gun
/// cooldown. Plain data plus rules, no actors: the controller, the input and the gun read and write it.
/// </summary>
class PlayerState
{
public:
	static constexpr float	WALK_SPEED		= 10.0f;	// m/s
	static constexpr float	RAY_REACH		= 30.0f;	// m
	static constexpr float	COOLDOWN		= 1.0f;		// s between shots
	static constexpr float	FACTOR_RATE		= 0.25f;
	static constexpr int	FACTOR_COUNT	= 3;		// x0.25, x0.5, x0.75

	/// <summary>
	/// Back to the first factor and a ready gun (a level starts).
	/// </summary>
	void	Reset();

	//////////////////////////////////////////
	/// Scale factor
	//////////////////////////////////////////
	int		GetFactorIndex() const;
	float	GetFactor() const;						// FACTOR_RATE * (index + 1)

	/// <summary>
	/// Wheel: +1 one step up, -1 one step down, clamped. Publishes ScaleFactorChangedEvent when it changed.
	/// </summary>
	bool	ChangeFactor(int direction);

	//////////////////////////////////////////
	/// Gun cooldown
	//////////////////////////////////////////
	bool	IsReadyToShoot() const;
	void	StartCooldown();
	void	TickCooldown(float deltaTime);			// FixedUpdate
	float	GetCooldownLeft() const;
	float	GetCooldownFraction() const;			// 0 ready .. 1 just fired (HUD bar)

private:
	int		m_factorIndex	= 0;
	float	m_cooldownLeft	= 0.0f;
};
