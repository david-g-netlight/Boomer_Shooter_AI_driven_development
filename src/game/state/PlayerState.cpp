#include "PlayerState.h"
#include <algorithm>

void PlayerState::Reset()
{
	m_cooldownLeft	= 0.0f;
}

float PlayerState::GetReach() const				{ return m_reach; }
void PlayerState::SetReach(float meters)		{ m_reach = std::max(0.1f, meters); }
float PlayerState::GetCooldown() const			{ return m_cooldown; }
void PlayerState::SetCooldown(float seconds)	{ m_cooldown = std::max(0.01f, seconds); }

bool PlayerState::IsReadyToShoot() const	{ return m_cooldownLeft <= 0.0f; }
void PlayerState::StartCooldown()			{ m_cooldownLeft = m_cooldown; }
float PlayerState::GetCooldownLeft() const	{ return m_cooldownLeft; }

void PlayerState::TickCooldown(float deltaTime)
{
	m_cooldownLeft = std::max(0.0f, m_cooldownLeft - deltaTime);
}

float PlayerState::GetCooldownFraction() const
{
	return std::min(1.0f, m_cooldownLeft / m_cooldown);
}
