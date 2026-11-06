#pragma once
#include <eng.h>

/// <summary>
/// Demo component: destroys its actor (and the actor's children) from inside its own Update
/// once the delay has passed. Shows that destroying mid-update is safe.
/// </summary>
class SelfDestructTimer : public eng::Component
{
public:
	explicit SelfDestructTimer(float delaySeconds) : m_timeLeft(delaySeconds) {}

	void Update(float deltaTime) override
	{
		m_timeLeft -= deltaTime;
		if (m_timeLeft <= 0.0f)
			GetOwner()->MarkForDestroy();
	}

	const char* TypeName() const override { return "SelfDestructTimer"; }

private:
	float m_timeLeft;
};
