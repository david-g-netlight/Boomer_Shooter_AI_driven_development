#include "PlayerInput.h"
#include <algorithm>
#include <eng.h>

void PlayerInput::BindDefaults(eng::InputMap& map)
{
	using eng::InputBinding;
	using eng::AxisBinding;

	// Movement: WASD and arrows, clamped so diagonals are not faster
	map.BindAxis("MoveX", AxisBinding::Keys(KEY_A, KEY_D));
	map.BindAxis("MoveX", AxisBinding::Keys(KEY_LEFT, KEY_RIGHT));
	map.BindAxis("MoveY", AxisBinding::Keys(KEY_S, KEY_W));
	map.BindAxis("MoveY", AxisBinding::Keys(KEY_DOWN, KEY_UP));
	map.BindAxis2D("Move", "MoveX", "MoveY", true);

	// Look: mouse delta in screen points
	map.BindAxis("LookX", AxisBinding::MouseX());
	map.BindAxis("LookY", AxisBinding::MouseY());
	map.BindAxis2D("Look", "LookX", "LookY", false);

	map.BindAction("Jump",		InputBinding::Key(KEY_SPACE));
	map.BindAction("Fire",		InputBinding::Mouse(MOUSE_BUTTON_LEFT));
}

void PlayerInput::Update(float deltaTime, const eng::InputMap& map)
{
	Apply(deltaTime, map.GetAxis2D("Move"), map.GetAxis2D("Look"), map.IsActionPressed("Jump"), map.IsActionPressed("Fire"),
		eng::Engine::GetInstance().GetModes().IsSimulating());
}

void PlayerInput::Apply(float deltaTime, Vector2 direction, Vector2 look, bool jumpPressed, bool firePressed, bool isInPlay)
{
	m_jumpBuffer = std::max(0.0f, m_jumpBuffer - deltaTime);
	m_fireBuffer = std::max(0.0f, m_fireBuffer - deltaTime);

	if (!m_hasControl || !isInPlay)
	{
		// Nothing leaks through a win screen or a menu, not even a click made just before it opened
		m_direction = m_look = { 0.0f, 0.0f };
		m_jumpBuffer = m_fireBuffer = 0.0f;
		return;
	}

	m_direction = direction;
	m_look		= look;

	if (jumpPressed)
		m_jumpBuffer = BUFFER_SECONDS;
	if (firePressed)
		m_fireBuffer = BUFFER_SECONDS;
}

void PlayerInput::SetControl(bool hasControl)
{
	m_hasControl = hasControl;
	if (!hasControl)
	{
		m_direction = m_look = { 0.0f, 0.0f };
		m_jumpBuffer = m_fireBuffer = 0.0f;
	}
}

bool PlayerInput::HasControl() const	{ return m_hasControl; }
Vector2 PlayerInput::GetDirection() const	{ return m_direction; }
Vector2 PlayerInput::GetLook() const		{ return m_look; }

bool PlayerInput::ConsumeJump()
{
	const bool wanted = m_jumpBuffer > 0.0f;
	m_jumpBuffer = 0.0f;
	return wanted;
}

bool PlayerInput::ConsumeFire()
{
	const bool wanted = m_fireBuffer > 0.0f;
	m_fireBuffer = 0.0f;
	return wanted;
}
