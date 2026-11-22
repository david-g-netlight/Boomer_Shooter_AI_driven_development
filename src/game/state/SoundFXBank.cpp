#include "SoundFXBank.h"
#include <eng.h>

SoundFXBank& SoundFXBank::Instance()
{
	static SoundFXBank bank;
	return bank;
}

void SoundFXBank::Load(eng::ResourceManager& resources)
{
	m_sounds[static_cast<int>(Sfx::Shot)]		= resources.LoadSound("assets/audio/sfx_ping.wav");	// placeholder
	m_sounds[static_cast<int>(Sfx::EnemyHit)]	= resources.LoadSound("assets/audio/sfx_ping.wav");	// placeholder
	m_sounds[static_cast<int>(Sfx::EnemyDeath)]	= resources.LoadSound("assets/audio/sfx_ping.wav");	// placeholder
	m_sounds[static_cast<int>(Sfx::PlayerHurt)]	= resources.LoadSound("assets/audio/sfx_ping.wav");	// placeholder
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

bool SoundFXBank::PlayShot() const
{
	const float pitch = 0.95f + 0.1f * static_cast<float>(GetRandomValue(0, 100)) / 100.0f;
	return Play(Sfx::Shot, 0.3f, pitch);
}

bool SoundFXBank::PlayEnemyHit() const
{
	const float pitch = 1.35f + 0.1f * static_cast<float>(GetRandomValue(0, 100)) / 100.0f;
	return Play(Sfx::EnemyHit, 0.4f, pitch);
}

bool SoundFXBank::PlayEnemyDeath() const
{
	const float pitch = 0.55f + 0.1f * static_cast<float>(GetRandomValue(0, 100)) / 100.0f;
	return Play(Sfx::EnemyDeath, 0.6f, pitch);
}

bool SoundFXBank::PlayPlayerHurt() const
{
	return Play(Sfx::PlayerHurt, 0.7f, 0.45f);
}
