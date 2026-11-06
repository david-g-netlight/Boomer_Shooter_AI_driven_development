#pragma once
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// Streams one music track at a time, with fade in / out and cross-fades. Owned by the AudioManager,
	/// which calls Update() every frame (raylib needs UpdateMusicStream per frame).
	///
	/// Final volume = track volume (SetVolume) * fade * music bus * duck.
	/// Music streams are not owned (ResourceManager). One Music can only be in one place at a time:
	/// playing the track that is fading out brings it back instead of restarting it.
	/// </summary>
	class MusicPlayer
	{
	public:
		/// <summary>
		/// Starts `music` from the beginning and fades it in. If another track is playing it fades out over
		/// the same time (cross-fade). Playing the current track again only resumes its fade-in.
		/// </summary>
		void			Play(Music& music, float fadeSeconds = 0.0f, bool loop = true);
		void			Stop(float fadeSeconds = 0.0f);
		void			Pause();
		void			Resume();

		void			SetVolume(float volume);	// track volume, 0..1
		float			GetVolume() const;

		bool			IsPlaying() const;
		bool			IsPaused() const;
		const Music*	GetCurrent() const;
		float			GetFade() const;			// current track's fade, 0..1
		float			GetTimePlayed() const;

		/// <summary>
		/// Called by the AudioManager. busGain = music bus volume * duck factor.
		/// </summary>
		void			Update(float unscaledDeltaTime, float busGain);

		/// <summary>
		/// Stops everything and forgets the tracks. Called before the ResourceManager unloads them.
		/// </summary>
		void			Release();

	private:
		struct Track
		{
			Music*	music		= nullptr;
			float	fade		= 0.0f;		// 0..1
			float	fadeSpeed	= 0.0f;		// per second, > 0 fading in, < 0 fading out
		};

		static void		StepFade(Track& track, float deltaTime);
		static float	FadeSpeed(float seconds);

		Track	m_current;
		Track	m_outgoing;		// fading out during a cross-fade
		float	m_volume		= 1.0f;
		bool	m_isPaused		= false;
	};
}
