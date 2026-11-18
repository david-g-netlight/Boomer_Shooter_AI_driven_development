#pragma once
#include <raylib.h>

namespace eng { class ResourceManager; }

enum class Sfx
{
	Shot,
	Count,
};

/// <summary>
/// The game's sound effects, loaded once through the ResourceManager and played by name. More come with enemies.
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
	/// Plays through the AudioManager's voice pool; false if the sound is missing.
	/// </summary>
	bool	Play(Sfx sfx, float volume = 0.8f, float pitch = 1.0f) const;

	/// <summary>
	/// The gun's sound, with a little random pitch so rapid fire doesn't sound mechanical.
	/// </summary>
	bool	PlayShot() const;

private:
	const Sound*	m_sounds[static_cast<int>(Sfx::Count)] = {};
};
