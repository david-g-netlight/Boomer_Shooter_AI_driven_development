#include "AudioManager.h"
#include "core/Log.h"
#include <algorithm>

namespace eng
{
	bool AudioManager::Init()
	{
		if (m_isReady)
			return true;

		InitAudioDevice();
		m_isReady = IsAudioDeviceReady();
		if (!m_isReady)
		{
			Log::Error("AudioManager: audio device failed to initialize, sound is disabled");
			return false;
		}

		::SetMasterVolume(m_masterVolume);
		m_voices.reserve(MAX_VOICES);
		return true;
	}

	void AudioManager::Shutdown()
	{
		if (!m_isReady)
			return;

		ReleaseVoices();
		CloseAudioDevice();
		m_isReady = false;
	}

	bool AudioManager::IsReady() const
	{
		return m_isReady;
	}

	//////////////////////////////////////////
	/// Sound effects
	//////////////////////////////////////////
	VoiceHandle AudioManager::Play(const Sound& sound, float volume, float pitch, float pan)
	{
		if (!m_isReady || !IsSoundValid(sound))
			return {};

		const int index = AcquireVoice(sound);
		if (index < 0)
			return {};

		Voice& voice		= m_voices[index];
		voice.volume		= std::clamp(volume, 0.0f, 1.0f);
		voice.startedAt		= ++m_playCounter;
		++voice.generation;

		ApplyVolume(voice);
		SetSoundPitch(voice.alias, std::max(pitch, 0.01f));
		SetSoundPan(voice.alias, ToRaylibPan(pan));
		PlaySound(voice.alias);	// restarts it if this voice was stolen while playing
		if (m_isPaused)
			PauseSound(voice.alias);

		return { index, voice.generation };
	}

	void AudioManager::Stop(VoiceHandle handle)
	{
		if (Voice* voice = FindVoice(handle))
			StopSound(voice->alias);
	}

	bool AudioManager::IsPlaying(VoiceHandle handle) const
	{
		const Voice* voice = FindVoice(handle);
		return voice && IsSoundPlaying(voice->alias);
	}

	void AudioManager::SetVoiceVolume(VoiceHandle handle, float volume)
	{
		if (Voice* voice = FindVoice(handle))
		{
			voice->volume = std::clamp(volume, 0.0f, 1.0f);
			ApplyVolume(*voice);
		}
	}

	void AudioManager::SetVoicePan(VoiceHandle handle, float pan)
	{
		if (Voice* voice = FindVoice(handle))
			SetSoundPan(voice->alias, ToRaylibPan(pan));
	}

	void AudioManager::StopAll()
	{
		for (Voice& voice : m_voices)
			StopSound(voice.alias);
	}

	void AudioManager::PauseAll()
	{
		m_isPaused = true;
		for (Voice& voice : m_voices)
			PauseSound(voice.alias);
	}

	void AudioManager::ResumeAll()
	{
		m_isPaused = false;
		for (Voice& voice : m_voices)
			ResumeSound(voice.alias);
	}

	bool AudioManager::IsPaused() const
	{
		return m_isPaused;
	}

	//////////////////////////////////////////
	/// Volumes
	//////////////////////////////////////////
	void AudioManager::SetMasterVolume(float volume)
	{
		m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
		if (m_isReady)
			::SetMasterVolume(m_masterVolume);
	}

	float AudioManager::GetMasterVolume() const
	{
		return m_masterVolume;
	}

	void AudioManager::SetBusVolume(AudioBus bus, float volume)
	{
		volume = std::clamp(volume, 0.0f, 1.0f);
		if (bus == AudioBus::Music)
		{
			m_musicVolume = volume;
			return;
		}

		m_sfxVolume = volume;
		for (Voice& voice : m_voices)
			ApplyVolume(voice);
	}

	float AudioManager::GetBusVolume(AudioBus bus) const
	{
		return bus == AudioBus::Music ? m_musicVolume : m_sfxVolume;
	}

	void AudioManager::Duck(bool ducked)
	{
		m_isDucked = ducked;
	}

	bool AudioManager::IsDucked() const
	{
		return m_isDucked;
	}

	float AudioManager::GetDuckFactor() const
	{
		return m_duckFactor;
	}

	//////////////////////////////////////////
	/// Listener
	//////////////////////////////////////////
	void AudioManager::SetListener(const Vector3& position, const Vector3& right)
	{
		m_listenerPosition	= position;
		m_listenerRight		= right;
	}

	const Vector3& AudioManager::GetListenerPosition() const	{ return m_listenerPosition; }
	const Vector3& AudioManager::GetListenerRight() const		{ return m_listenerRight; }

	//////////////////////////////////////////
	/// Called by the Engine
	//////////////////////////////////////////
	void AudioManager::Update(float unscaledDeltaTime)
	{
		// Smooth duck in / out
		const float target	= m_isDucked ? DUCK_LEVEL : 1.0f;
		const float step	= (1.0f - DUCK_LEVEL) * unscaledDeltaTime / DUCK_FADE_SECONDS;
		m_duckFactor = m_duckFactor < target ? std::min(m_duckFactor + step, target)
											 : std::max(m_duckFactor - step, target);
	}

	void AudioManager::ReleaseVoices()
	{
		for (Voice& voice : m_voices)
		{
			StopSound(voice.alias);
			UnloadSoundAlias(voice.alias);
		}
		m_voices.clear();
	}

	int AudioManager::GetVoiceCount() const
	{
		return static_cast<int>(m_voices.size());
	}

	int AudioManager::GetActiveVoiceCount() const
	{
		int count = 0;
		for (const Voice& voice : m_voices)
			count += IsSoundPlaying(voice.alias) ? 1 : 0;
		return count;
	}

	//////////////////////////////////////////
	/// Private
	//////////////////////////////////////////
	AudioManager::Voice* AudioManager::FindVoice(VoiceHandle handle)
	{
		if (handle.index < 0 || handle.index >= static_cast<int>(m_voices.size()))
			return nullptr;
		Voice& voice = m_voices[handle.index];
		return voice.generation == handle.generation ? &voice : nullptr;
	}

	const AudioManager::Voice* AudioManager::FindVoice(VoiceHandle handle) const
	{
		return const_cast<AudioManager*>(this)->FindVoice(handle);
	}

	int AudioManager::AcquireVoice(const Sound& sound)
	{
		const void* source		= sound.stream.buffer;
		int			ownCount	= 0;
		int			ownOldest	= -1;
		int			oldest		= -1;

		for (int i = 0; i < static_cast<int>(m_voices.size()); ++i)
		{
			const Voice& voice = m_voices[i];
			if (oldest < 0 || voice.startedAt < m_voices[oldest].startedAt)
				oldest = i;

			if (voice.source != source)
				continue;

			if (!IsSoundPlaying(voice.alias) && !m_isPaused)
				return i;	// free voice of this sound

			++ownCount;
			if (ownOldest < 0 || voice.startedAt < m_voices[ownOldest].startedAt)
				ownOldest = i;
		}

		// Room for another alias of this sound?
		if (ownCount < VOICES_PER_SOUND && static_cast<int>(m_voices.size()) < MAX_VOICES)
		{
			Voice voice;
			voice.alias		= LoadSoundAlias(sound);
			voice.source	= source;
			m_voices.push_back(voice);
			return static_cast<int>(m_voices.size()) - 1;
		}

		// Steal: this sound's oldest voice, or (pool full, no voice of this sound) the oldest voice overall,
		// re-pointed at this sound
		if (ownOldest >= 0)
			return ownOldest;

		if (oldest >= 0)
		{
			Voice& voice = m_voices[oldest];
			StopSound(voice.alias);
			UnloadSoundAlias(voice.alias);
			voice.alias		= LoadSoundAlias(sound);
			voice.source	= source;
			return oldest;
		}
		return -1;
	}

	void AudioManager::ApplyVolume(Voice& voice) const
	{
		SetSoundVolume(voice.alias, voice.volume * m_sfxVolume);
	}

	float AudioManager::ToRaylibPan(float pan)
	{
		// eng: -1 left .. +1 right   raylib: 1 left .. 0 right (0.5 center)
		return 0.5f - 0.5f * std::clamp(pan, -1.0f, 1.0f);
	}
}
