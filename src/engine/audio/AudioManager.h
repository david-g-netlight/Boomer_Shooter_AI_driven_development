#pragma once
#include <cstdint>
#include <vector>
#include <raylib.h>
#include "MusicPlayer.h"

namespace eng
{
	/// <summary>
	/// Identifies one playing voice. Stays safe to use after the voice is reused (generation check).
	/// </summary>
	struct VoiceHandle
	{
		int			index		= -1;
		uint32_t	generation	= 0;

		bool IsValid() const { return index >= 0; }
	};

	enum class AudioBus : uint8_t
	{
		Sfx,
		Music,
	};

	/// <summary>
	/// Owns the audio device and a pool of voices for sound effects, owned by the Engine.
	///
	/// Each Play() uses a raylib *alias* of the sound (shared sample data), so the same sound can overlap
	/// with itself: up to VOICES_PER_SOUND voices per sound, MAX_VOICES in total; when a sound has no free
	/// voice, its oldest one is restarted.
	///
	/// Pan is -1 (left) .. +1 (right). raylib's own pan is reversed (1 = left), this class converts it.
	/// Duck() lowers the music bus smoothly (Pause / Editor); PauseAll() / ResumeAll() hold SFX.
	///
	/// Shutdown order (Engine): ReleaseVoices() -> ResourceManager::UnloadAll() -> Shutdown() -> CloseWindow().
	/// </summary>
	class AudioManager
	{
	public:
		static constexpr int	VOICES_PER_SOUND	= 4;
		static constexpr int	MAX_VOICES			= 64;

		AudioManager() = default;
		AudioManager(const AudioManager&) = delete;
		AudioManager& operator = (const AudioManager&) = delete;

		bool		Init();
		void		Shutdown();
		bool		IsReady() const;

		//////////////////////////////////////////
		/// Sound effects
		//////////////////////////////////////////
		VoiceHandle	Play(const Sound& sound, float volume = 1.0f, float pitch = 1.0f, float pan = 0.0f);
		void		Stop(VoiceHandle voice);
		bool		IsPlaying(VoiceHandle voice) const;
		void		SetVoiceVolume(VoiceHandle voice, float volume);
		void		SetVoicePan(VoiceHandle voice, float pan);

		void		StopAll();
		void		PauseAll();
		void		ResumeAll();
		bool		IsPaused() const;

		//////////////////////////////////////////
		/// Music (one streamed track at a time, cross-fades)
		//////////////////////////////////////////
		MusicPlayer&		GetMusic();
		const MusicPlayer&	GetMusic() const;

		//////////////////////////////////////////
		/// Volumes
		//////////////////////////////////////////
		void		SetMasterVolume(float volume);
		float		GetMasterVolume() const;
		void		SetBusVolume(AudioBus bus, float volume);
		float		GetBusVolume(AudioBus bus) const;

		/// <summary>
		/// Ducked: the music bus fades to DUCK_LEVEL over DUCK_FADE_SECONDS. Read by the MusicPlayer.
		/// </summary>
		void		Duck(bool ducked);
		bool		IsDucked() const;
		float		GetDuckFactor() const;		// current smoothed factor, 1 = not ducked

		//////////////////////////////////////////
		/// Listener (set by the Engine from the active camera every frame)
		//////////////////////////////////////////
		void			SetListener(const Vector3& position, const Vector3& right);
		const Vector3&	GetListenerPosition() const;
		const Vector3&	GetListenerRight() const;

		//////////////////////////////////////////
		/// Called by the Engine
		//////////////////////////////////////////
		void		Update(float unscaledDeltaTime);

		/// <summary>
		/// Unloads every alias and releases the music tracks. Must run before the ResourceManager unloads
		/// the sounds and music they refer to.
		/// </summary>
		void		ReleaseVoices();

		int			GetVoiceCount() const;			// aliases created
		int			GetActiveVoiceCount() const;	// currently playing

	private:
		static constexpr float	DUCK_LEVEL			= 0.3f;
		static constexpr float	DUCK_FADE_SECONDS	= 0.25f;

		struct Voice
		{
			Sound			alias		= {};
			const void*		source		= nullptr;	// the source sound's buffer, identifies which sound this aliases
			float			volume		= 1.0f;		// before the SFX bus
			uint32_t		generation	= 0;
			uint64_t		startedAt	= 0;		// play order, to find the oldest voice
		};

		Voice*		FindVoice(VoiceHandle handle);
		const Voice*FindVoice(VoiceHandle handle) const;
		int			AcquireVoice(const Sound& sound);
		void		ApplyVolume(Voice& voice) const;
		static float ToRaylibPan(float pan);

		bool				m_isReady		= false;
		bool				m_isPaused		= false;
		std::vector<Voice>	m_voices;
		uint64_t			m_playCounter	= 0;

		float				m_masterVolume	= 1.0f;
		float				m_sfxVolume		= 1.0f;
		float				m_musicVolume	= 1.0f;

		bool				m_isDucked		= false;
		float				m_duckFactor	= 1.0f;

		MusicPlayer			m_music;

		Vector3				m_listenerPosition	= { 0.0f, 0.0f, 0.0f };
		Vector3				m_listenerRight		= { 1.0f, 0.0f, 0.0f };
	};
}
