#include "PlayerState.h"
#include <algorithm>

void PlayerState::Reset()
{
	m_cooldownLeft	= 0.0f;
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
