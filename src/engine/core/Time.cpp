#include "Time.h"
#include <algorithm>

namespace eng
{
	void Time::BeginFrame(float rawDeltaTime)
	{
		m_unscaledDeltaTime	= std::clamp(rawDeltaTime, 0.0f, MAX_DELTA_TIME);
		m_deltaTime			= m_unscaledDeltaTime * m_timeScale;
		m_totalTime			+= m_deltaTime;
		m_stepsThisFrame	= 0;
		++m_frameCount;
	}

	void Time::AdvanceAccumulator()
	{
		m_accumulator += m_deltaTime;
	}

	bool Time::ConsumeFixedStep()
	{
		if (m_accumulator < FIXED_DELTA_TIME)
			return false;

		if (m_stepsThisFrame >= MAX_FIXED_STEPS_PER_FRAME)
		{
			// Too far behind: drop the backlog instead of trying to catch up forever
			m_accumulator = 0.0f;
			return false;
		}

		m_accumulator -= FIXED_DELTA_TIME;
		++m_stepsThisFrame;
		++m_fixedStepCount;
		return true;
	}

	void Time::ResetAccumulator()
	{
		m_accumulator = 0.0f;
	}

	void Time::SetTimeScale(float timeScale)
	{
		m_timeScale = std::max(timeScale, 0.0f);
	}

	float		Time::GetTimeScale() const			{ return m_timeScale; }
	float		Time::GetDeltaTime() const			{ return m_deltaTime; }
	float		Time::GetUnscaledDeltaTime() const	{ return m_unscaledDeltaTime; }
	float		Time::GetFixedDeltaTime() const		{ return FIXED_DELTA_TIME; }
	float		Time::GetFixedAlpha() const			{ return m_accumulator / FIXED_DELTA_TIME; }
	double		Time::GetTotalTime() const			{ return m_totalTime; }
	uint64_t	Time::GetFrameCount() const			{ return m_frameCount; }
	uint64_t	Time::GetFixedStepCount() const		{ return m_fixedStepCount; }
}
