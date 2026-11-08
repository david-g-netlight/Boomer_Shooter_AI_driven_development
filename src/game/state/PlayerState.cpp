#include "PlayerState.h"
#include <algorithm>
#include <eng.h>

void PlayerState::Reset()
{
	m_factorIndex	= 0;
	m_cooldownLeft	= 0.0f;
}

int PlayerState::GetFactorIndex() const	{ return m_factorIndex; }
float PlayerState::GetFactor() const	{ return FACTOR_RATE * static_cast<float>(m_factorIndex + 1); }

bool PlayerState::ChangeFactor(int direction)
{
	const int next = std::clamp(m_factorIndex + (direction > 0 ? 1 : (direction < 0 ? -1 : 0)), 0, FACTOR_COUNT - 1);
	if (next == m_factorIndex)
		return false;

	m_factorIndex = next;
	eng::Engine::GetInstance().GetEvents().Publish(ScaleFactorChangedEvent{ m_factorIndex, GetFactor() });
	return true;
}

bool PlayerState::IsReadyToShoot() const	{ return m_cooldownLeft <= 0.0f; }
void PlayerState::StartCooldown()			{ m_cooldownLeft = COOLDOWN; }
float PlayerState::GetCooldownLeft() const	{ return m_cooldownLeft; }

void PlayerState::TickCooldown(float deltaTime)
{
	m_cooldownLeft = std::max(0.0f, m_cooldownLeft - deltaTime);
}

float PlayerState::GetCooldownFraction() const
{
	return m_cooldownLeft / COOLDOWN;
}
