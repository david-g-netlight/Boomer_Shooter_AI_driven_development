#include "SoundFXBank.h"
#include <eng.h>

namespace
{
	/// <summary>One sound: its file, its volume before the SFX bus, and the pitch range it is played in.</summary>
	struct SfxDef
	{
		const char*	path;
		float		volume;
		float		pitchMin;
		float		pitchMax;		// equal to pitchMin for a fixed pitch
	};

	// Same order as the Sfx enum. Values of the first four are the ones the bank used before the real files.
	// The others are first guesses (pitch 1.0): tune them by ear in S3, when they are played.
	constexpr SfxDef SFX_TABLE[] =
	{
		{ "assets/audio/sound_fx/player/761501__valhallaproject__energy-pistol-fireexplosion-short.wav",	0.3f,	0.95f,	1.05f },	// Shot
		{ "assets/audio/sound_fx/player/469567__far_box_creature__die8.wav",							0.8f,	1.0f,	1.0f },		// PlayerDie
		{ "assets/audio/sound_fx/player/386486__iimck__jump.wav",										0.5f,	1.0f,	1.0f },		// PlayerJump
		{ "assets/audio/sound_fx/player/583714__dexd73__running-on-metal-loop.wav",						0.4f,	1.0f,	1.0f },		// PlayerStep
		{ "assets/audio/sound_fx/enemy/711986__gizemhozdemir__hit-man-with-noise.wav",					0.7f,	0.45f,	0.45f },	// EnemyHitsPlayer
		{ "assets/audio/sound_fx/enemy/874627__duhduhduh__slow-pitch-growl-hit.wav",					0.4f,	1.35f,	1.45f },	// EnemyHit
		{ "assets/audio/sound_fx/enemy/874627__duhduhduh__slow-pitch-growl.wav",						0.6f,	0.55f,	0.65f },	// EnemyDeath
		{ "assets/audio/sound_fx/announcements/767627__valhallaproject__announcer-you-win.ogg",			0.8f,	1.0f,	1.0f },		// Win
		{ "assets/audio/sound_fx/announcements/767626__valhallaproject__announcer-match-begins.ogg",	0.8f,	1.0f,	1.0f },		// MatchBegins
	};

	static_assert(sizeof(SFX_TABLE) / sizeof(SFX_TABLE[0]) == static_cast<int>(Sfx::Count), "SFX_TABLE must have one entry per Sfx");
}

SoundFXBank& SoundFXBank::Instance()
{
	static SoundFXBank bank;
	return bank;
}

void SoundFXBank::Load(eng::ResourceManager& resources)
{
	for (int i = 0; i < static_cast<int>(Sfx::Count); ++i)
		m_sounds[i] = resources.LoadSound(SFX_TABLE[i].path);
}

void SoundFXBank::Clear()
{
	for (const Sound*& sound : m_sounds)
		sound = nullptr;
}

eng::VoiceHandle SoundFXBank::Play(Sfx sfx) const
{
	const int index = static_cast<int>(sfx);
	const Sound* sound = m_sounds[index];
	if (!sound)
		return {};

	const SfxDef& def = SFX_TABLE[index];
	const float pitch = def.pitchMin + (def.pitchMax - def.pitchMin) * static_cast<float>(GetRandomValue(0, 100)) / 100.0f;
	return eng::Engine::GetInstance().GetAudio().Play(*sound, def.volume, pitch);
}

bool SoundFXBank::PlayShot() const				{ return Play(Sfx::Shot).IsValid(); }
bool SoundFXBank::PlayEnemyHit() const			{ return Play(Sfx::EnemyHit).IsValid(); }
bool SoundFXBank::PlayEnemyDeath() const		{ return Play(Sfx::EnemyDeath).IsValid(); }
bool SoundFXBank::PlayPlayerHurt() const		{ return Play(Sfx::EnemyHitsPlayer).IsValid(); }
