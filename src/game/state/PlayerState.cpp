#include "PlayerState.h"
#include <algorithm>

void PlayerState::Reset()
{
	m_cooldownLeft	= 0.0f;
	m_health		= MAX_HEALTH;
}

float PlayerState::GetReach() const				{ return m_reach; }
void PlayerState::SetReach(float meters)		{ m_reach = std::max(0.1f, meters); }
float PlayerState::GetCooldown() const			{ return m_cooldown; }
void PlayerState::SetCooldown(float seconds)	{ m_cooldown = std::max(0.01f, seconds); }

float PlayerState::GetHealth() const			{ return m_health; }
bool PlayerState::IsAlive() const				{ return m_health > 0.0f; }
void PlayerState::SetHealth(float health)		{ m_health = std::clamp(health, 0.0f, MAX_HEALTH); }

bool PlayerState::ApplyDamage(float damage)
{
	if (m_god || m_health <= 0.0f || damage <= 0.0f)
		return false;

	m_health = std::max(0.0f, m_health - damage);
	return m_health <= 0.0f;
}

bool PlayerState::IsGod() const				{ return m_god; }
bool* PlayerState::GetGodFlag()				{ return &m_god; }

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
