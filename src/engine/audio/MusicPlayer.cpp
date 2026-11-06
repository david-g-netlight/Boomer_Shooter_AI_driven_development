#include "MusicPlayer.h"
#include <algorithm>
#include <initializer_list>
#include <utility>

namespace eng
{
	namespace
	{
		constexpr float INSTANT = 1.0e6f;	// fade speed used for 0-second fades
	}

	float MusicPlayer::FadeSpeed(float seconds)
	{
		return seconds > 0.0f ? 1.0f / seconds : INSTANT;
	}

	void MusicPlayer::Play(Music& music, float fadeSeconds, bool loop)
	{
		if (!IsMusicValid(music))
			return;

		music.looping = loop;
		m_isPaused = false;

		// Already the current track: just (re)fade it in
		if (m_current.music == &music)
		{
			m_current.fadeSpeed = FadeSpeed(fadeSeconds);
			ResumeMusicStream(music);
			return;
		}

		// The track that is fading out comes back: swap instead of restarting it
		if (m_outgoing.music == &music)
		{
			std::swap(m_current, m_outgoing);
			m_current.fadeSpeed		= FadeSpeed(fadeSeconds);
			if (m_outgoing.music)
				m_outgoing.fadeSpeed = -FadeSpeed(fadeSeconds);
			return;
		}

		// A new track: the current one (if any) fades out, replacing an older outgoing track
		if (m_current.music)
		{
			if (m_outgoing.music)
				StopMusicStream(*m_outgoing.music);
			m_outgoing				= m_current;
			m_outgoing.fadeSpeed	= -FadeSpeed(fadeSeconds);
		}

		m_current = { &music, 0.0f, FadeSpeed(fadeSeconds) };
		SetMusicVolume(music, 0.0f);
		PlayMusicStream(music);	// from the beginning
	}

	void MusicPlayer::Stop(float fadeSeconds)
	{
		if (m_current.music)
			m_current.fadeSpeed = -FadeSpeed(fadeSeconds);
		if (m_outgoing.music)
			m_outgoing.fadeSpeed = std::min(m_outgoing.fadeSpeed, -FadeSpeed(fadeSeconds));
	}

	void MusicPlayer::Pause()
	{
		m_isPaused = true;
		if (m_current.music)	PauseMusicStream(*m_current.music);
		if (m_outgoing.music)	PauseMusicStream(*m_outgoing.music);
	}

	void MusicPlayer::Resume()
	{
		m_isPaused = false;
		if (m_current.music)	ResumeMusicStream(*m_current.music);
		if (m_outgoing.music)	ResumeMusicStream(*m_outgoing.music);
	}

	void MusicPlayer::SetVolume(float volume)	{ m_volume = std::clamp(volume, 0.0f, 1.0f); }
	float MusicPlayer::GetVolume() const		{ return m_volume; }
	bool MusicPlayer::IsPaused() const			{ return m_isPaused; }
	const Music* MusicPlayer::GetCurrent() const{ return m_current.music; }
	float MusicPlayer::GetFade() const			{ return m_current.fade; }

	bool MusicPlayer::IsPlaying() const
	{
		return m_current.music && IsMusicStreamPlaying(*m_current.music);
	}

	float MusicPlayer::GetTimePlayed() const
	{
		return m_current.music ? GetMusicTimePlayed(*m_current.music) : 0.0f;
	}

	void MusicPlayer::StepFade(Track& track, float deltaTime)
	{
		track.fade = std::clamp(track.fade + track.fadeSpeed * deltaTime, 0.0f, 1.0f);
		if (track.fade >= 1.0f && track.fadeSpeed > 0.0f)
			track.fadeSpeed = 0.0f;
	}

	void MusicPlayer::Update(float unscaledDeltaTime, float busGain)
	{
		for (Track* track : { &m_current, &m_outgoing })
		{
			if (!track->music)
				continue;

			UpdateMusicStream(*track->music);	// refills the stream buffers, needed every frame
			if (m_isPaused)
				continue;

			StepFade(*track, unscaledDeltaTime);
			SetMusicVolume(*track->music, m_volume * track->fade * busGain);

			// Faded out completely: stop and forget it
			if (track->fade <= 0.0f && track->fadeSpeed < 0.0f)
			{
				StopMusicStream(*track->music);
				*track = {};
			}
		}
	}

	void MusicPlayer::Release()
	{
		if (m_current.music)	StopMusicStream(*m_current.music);
		if (m_outgoing.music)	StopMusicStream(*m_outgoing.music);
		m_current	= {};
		m_outgoing	= {};
		m_isPaused	= false;
	}
}
