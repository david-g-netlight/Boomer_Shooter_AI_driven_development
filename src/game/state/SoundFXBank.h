#pragma once
#include <raylib.h>

namespace eng { class ResourceManager; }

enum class Sfx
{
	Stretch,
	Shrink,
	Count,
};

/// <summary>
/// The game's sound effects, loaded once through the ResourceManager and played by name (Unity ScalingAudioFX).
/// A sound that failed to load is skipped silently after the loader's warning.
/// </summary>
class SoundFXBank
{
public:
	/// <summary>
	/// The game's one bank (ScaleGame loads and clears it); props play through it without holding a pointer.
	/// </summary>
	static SoundFXBank&	Instance();

	void	Load(eng::ResourceManager& resources);
	void	Clear();								// before the ResourceManager unloads

	/// <summary>
	/// Plays through the AudioManager's voice pool; false if the sound is missing.
	/// </summary>
	bool	Play(Sfx sfx, float volume = 0.8f, float pitch = 1.0f) const;

	/// <summary>
	/// The scaling gun's sound: stretch pitch = (1 + factor) + 0.02, shrink pitch = (1 - factor) - 0.02.
	/// sign > 0 stretches, sign < 0 shrinks.
	/// </summary>
	bool	PlayScaling(float sign, float factor) const;

private:
	const Sound*	m_sounds[static_cast<int>(Sfx::Count)] = {};
};
