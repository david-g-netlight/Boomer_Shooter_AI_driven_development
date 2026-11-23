#pragma once
#include <eng.h>

namespace eng { class ResourceManager; }

enum class Sfx
{
	Shot,
	PlayerDie,
	PlayerJump,
	PlayerStep,
	EnemyHitsPlayer,
	EnemyHit,
	EnemyDeath,
	Win,
	MatchBegins,
	Count,
};

/// <summary>
/// The game's sound effects, loaded once through the ResourceManager and played by name. Each sound's file,
/// volume and pitch range are in SFX_TABLE (SoundFXBank.cpp).
/// A sound that failed to load is skipped silently after the loader's warning.
/// </summary>
class SoundFXBank
{
public:
	/// <summary>
	/// The game's one bank (the Application loads and clears it); anything plays through it without holding a pointer.
	/// </summary>
	static SoundFXBank&	Instance();

	void	Load(eng::ResourceManager& resources);
	void	Clear();								// before the ResourceManager unloads

	/// <summary>
	/// Plays the sound with a pitch picked from its table range. Returns an invalid handle if the sound is missing.
	/// </summary>
	eng::VoiceHandle	Play(Sfx sfx) const;

	/// <summary>
	/// The gun's sound, with a little random pitch so rapid fire doesn't sound mechanical.
	/// </summary>
	bool	PlayShot() const;

	/// <summary>A shot that hit an enemy: higher and short.</summary>
	bool	PlayEnemyHit() const;

	/// <summary>An enemy dies: lower and louder.</summary>
	bool	PlayEnemyDeath() const;

	/// <summary>The player takes damage: low and short.</summary>
	bool	PlayPlayerHurt() const;

private:
	const Sound*	m_sounds[static_cast<int>(Sfx::Count)] = {};
};
