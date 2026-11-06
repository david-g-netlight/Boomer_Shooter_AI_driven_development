#pragma once
#include <cstdint>

namespace eng
{
	constexpr float	FIXED_DELTA_TIME			= 1.0f / 50.0f;	// 0.02 s, Unity's default fixed step
	constexpr float	MAX_DELTA_TIME				= 0.1f;			// Clamp after hitches (breakpoints, window drag)
	constexpr int	MAX_FIXED_STEPS_PER_FRAME	= 5;			// Avoids the "spiral of death"

	/// <summary>
	/// Frame time and the fixed-step accumulator. Does not call raylib:
	/// the Engine feeds it the raw frame time, so it stays testable.
	///
	/// Usage per frame:
	///		time.BeginFrame(GetFrameTime());
	///		time.AdvanceAccumulator();				// only while the simulation runs
	///		while (time.ConsumeFixedStep()) { FixedUpdate(time.GetFixedDeltaTime()); }
	/// </summary>
	class Time
	{
	public:
		void		BeginFrame(float rawDeltaTime);

		/// <summary>
		/// Adds this frame's scaled delta to the accumulator. Skipped while paused, in menus or in the editor.
		/// </summary>
		void		AdvanceAccumulator();

		/// <summary>
		/// True while at least one fixed step is due; consumes it. Stops after MAX_FIXED_STEPS_PER_FRAME
		/// and drops the remaining time.
		/// </summary>
		bool		ConsumeFixedStep();

		void		ResetAccumulator();

		void		SetTimeScale(float timeScale);
		float		GetTimeScale() const;

		float		GetDeltaTime() const;			// clamped and scaled
		float		GetUnscaledDeltaTime() const;	// clamped, not scaled (UI, editor camera)
		float		GetFixedDeltaTime() const;
		float		GetFixedAlpha() const;			// accumulator / fixed step, for render interpolation
		double		GetTotalTime() const;			// scaled time since start
		uint64_t	GetFrameCount() const;
		uint64_t	GetFixedStepCount() const;

	private:
		float		m_deltaTime				= 0.0f;
		float		m_unscaledDeltaTime		= 0.0f;
		float		m_timeScale				= 1.0f;
		float		m_accumulator			= 0.0f;
		int			m_stepsThisFrame		= 0;
		double		m_totalTime				= 0.0;
		uint64_t	m_frameCount			= 0;
		uint64_t	m_fixedStepCount		= 0;
	};
}
