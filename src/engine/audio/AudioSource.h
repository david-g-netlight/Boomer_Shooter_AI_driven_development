#pragma once
#include <raylib.h>
#include "AudioManager.h"

namespace eng
{
	/// <summary>
	/// A sound with its own volume / pitch settings, optionally placed in the world.
	/// Spatial: volume falls off between minDistance and maxDistance from the listener (the active camera)
	/// and pan follows the direction to the source. Plays through the Engine's AudioManager.
	///
	/// Standalone for now; it becomes a Component positioned by its GameActor in Phase 5 (decision D10).
	/// </summary>
	class AudioSource
	{
	public:
		AudioSource() = default;
		explicit AudioSource(const Sound* sound);

		void		SetSound(const Sound* sound);			// not owned (ResourceManager)
		void		SetVolume(float volume);
		void		SetPitch(float pitch);
		void		SetPitchVariation(float variation);	// each Play() picks pitch +- variation

		void		SetSpatial(bool spatial);
		void		SetPosition(const Vector3& position);
		void		SetRange(float minDistance, float maxDistance);
		const Vector3& GetPosition() const;

		void		Play();
		void		Stop();
		bool		IsPlaying() const;

		/// <summary>
		/// Re-applies distance volume and pan to the playing voice. Call every frame for moving sources.
		/// </summary>
		void		Update();

		/// <summary>
		/// Spatial gain (0..1) and pan (-1..+1) for the current listener, for debug display.
		/// </summary>
		float		ComputeGain() const;
		float		ComputePan() const;

	private:
		const Sound*	m_sound				= nullptr;
		float			m_volume			= 1.0f;
		float			m_pitch				= 1.0f;
		float			m_pitchVariation	= 0.0f;

		bool			m_isSpatial			= false;
		Vector3			m_position			= { 0.0f, 0.0f, 0.0f };
		float			m_minDistance		= 2.0f;
		float			m_maxDistance		= 30.0f;

		VoiceHandle		m_voice;
	};
}
