#include "SoundFXBank.h"
#include <eng.h>

SoundFXBank& SoundFXBank::Instance()
{
	static SoundFXBank bank;
	return bank;
}

void SoundFXBank::Load(eng::ResourceManager& resources)
{
	m_sounds[static_cast<int>(Sfx::Stretch)]	= resources.LoadSound("assets/audio/sfx_stretch.wav");
	m_sounds[static_cast<int>(Sfx::Shrink)]		= resources.LoadSound("assets/audio/sfx_shrink.wav");
	m_sounds[static_cast<int>(Sfx::Beam)]		= resources.LoadSound("assets/audio/sfx_ping.wav");	// placeholder
}

void SoundFXBank::Clear()
{
	for (const Sound*& sound : m_sounds)
		sound = nullptr;
}

bool SoundFXBank::Play(Sfx sfx, float volume, float pitch) const
{
	const Sound* sound = m_sounds[static_cast<int>(sfx)];
	if (!sound)
		return false;

	eng::Engine::GetInstance().GetAudio().Play(*sound, volume, pitch);
	return true;
}

bool SoundFXBank::PlayScaling(float sign, float factor) const
{
	return sign > 0.0f ? Play(Sfx::Stretch, 0.8f, 1.0f + factor + 0.02f)
					   : Play(Sfx::Shrink,  0.8f, 1.0f - factor - 0.02f);
}
