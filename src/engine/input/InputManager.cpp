#include "InputManager.h"

namespace eng
{
	void InputManager::Poll()
	{
		// KEY_NULL (0) is not a key, start at 1
		for (int key = 1; key < KEYS_SIZE; ++key)
		{
			m_keysDown[key]		= ::IsKeyDown(key);
			m_keysPressed[key]	= ::IsKeyPressed(key);
			m_keysReleased[key]	= ::IsKeyReleased(key);
		}

		for (int button = 0; button < MOUSE_BUTTONS_SIZE; ++button)
		{
			m_mouseDown[button]		= ::IsMouseButtonDown(button);
			m_mousePressed[button]	= ::IsMouseButtonPressed(button);
			m_mouseReleased[button]	= ::IsMouseButtonReleased(button);
		}

		m_mousePosition	= ::GetMousePosition();
		m_mouseDelta	= ::GetMouseDelta();
		if (m_suppressDeltaFrames > 0)
		{
			m_mouseDelta = { 0.0f, 0.0f };
			--m_suppressDeltaFrames;
		}
		m_mouseWheel	= ::GetMouseWheelMove();
	}

	bool InputManager::IsValidKey(int key)			{ return key > 0 && key < KEYS_SIZE; }
	bool InputManager::IsValidButton(int button)	{ return button >= 0 && button < MOUSE_BUTTONS_SIZE; }

	bool InputManager::IsKeyDown(int key) const		{ return IsValidKey(key) && m_keysDown[key]; }
	bool InputManager::IsKeyPressed(int key) const	{ return IsValidKey(key) && m_keysPressed[key]; }
	bool InputManager::IsKeyReleased(int key) const	{ return IsValidKey(key) && m_keysReleased[key]; }

	bool InputManager::IsMouseButtonDown(int button) const		{ return IsValidButton(button) && m_mouseDown[button]; }
	bool InputManager::IsMouseButtonPressed(int button) const	{ return IsValidButton(button) && m_mousePressed[button]; }
	bool InputManager::IsMouseButtonReleased(int button) const	{ return IsValidButton(button) && m_mouseReleased[button]; }

	bool InputManager::IsLeftMouseButtonDown() const	{ return m_mouseDown[MOUSE_BUTTON_LEFT]; }
	bool InputManager::IsRightMouseButtonDown() const	{ return m_mouseDown[MOUSE_BUTTON_RIGHT]; }

	const Vector2&	InputManager::GetMousePosition() const	{ return m_mousePosition; }
	const Vector2&	InputManager::GetMouseDelta() const		{ return m_mouseDelta; }
	float			InputManager::GetMouseWheel() const		{ return m_mouseWheel; }

	void InputManager::SetCursorLocked(bool locked)
	{
		if (locked == m_isCursorLocked)
			return;

		if (locked)
			::DisableCursor();
		else
			::EnableCursor();

		m_isCursorLocked		= locked;
		m_suppressDeltaFrames	= 1;
	}

	bool InputManager::IsCursorLocked() const
	{
		return m_isCursorLocked;
	}

	InputMap&		InputManager::GetMap()			{ return m_map; }
	const InputMap&	InputManager::GetMap() const	{ return m_map; }
}
