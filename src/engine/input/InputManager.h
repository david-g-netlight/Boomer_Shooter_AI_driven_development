#pragma once
#include <cstddef>
#include <raylib.h>

/**
* This Class is owned and controlled by the Engine only
*/

namespace eng
{
	constexpr int KEYS_SIZE				= KEY_KB_MENU + 1;			// raylib key codes go up to 348
	constexpr int MOUSE_BUTTONS_SIZE	= MOUSE_BUTTON_BACK + 1;	// raylib buttons 0..6

	/// <summary>
	/// One snapshot of keyboard and mouse state per frame, filled by Poll().
	/// Every system reads the same state during a frame.
	///   Down     = held this frame
	///   Pressed  = went down this frame
	///   Released = went up this frame
	/// Read input in Update(), not in FixedUpdate(): FixedUpdate can run 0..N times per frame.
	/// </summary>
	class InputManager
	{
	public:
		//////////////////////////////////////////
		/// Keyboard Actions (raylib KeyboardKey codes)
		//////////////////////////////////////////
		bool IsKeyDown		(int key) const;
		bool IsKeyPressed	(int key) const;
		bool IsKeyReleased	(int key) const;

		//////////////////////////////////////////
		/// Mouse Actions (raylib MouseButton codes)
		//////////////////////////////////////////
		bool IsMouseButtonDown		(int button) const;
		bool IsMouseButtonPressed	(int button) const;
		bool IsMouseButtonReleased	(int button) const;

		bool IsLeftMouseButtonDown() const;
		bool IsRightMouseButtonDown() const;

		const Vector2&	GetMousePosition() const;	// window coordinates, in screen points
		const Vector2&	GetMouseDelta() const;		// movement since last frame, in screen points
		float			GetMouseWheel() const;		// > 0 scroll up, < 0 scroll down

	private:
		//////////////////////////////////////////
		// Private Constructors and Destructors
		//////////////////////////////////////////
		InputManager() = default;
		InputManager(const InputManager&) = delete;		// Copy constructor deleted
		InputManager(InputManager&&) = delete;			// Move constructor deleted

		InputManager& operator = (const InputManager&) = delete;	// Copy operator deleted
		InputManager& operator = (InputManager&&) = delete;			// Move operator deleted

		//////////////////////////////////////////
		// Friend declaration
		//////////////////////////////////////////
		friend class Engine; // only Engine can create, poll and destroy InputManager

		/// <summary>
		/// Reads raylib's state. Called by the Engine once per frame, before any Update.
		/// </summary>
		void Poll();

		static bool IsValidKey(int key);
		static bool IsValidButton(int button);

		//////////////////////////////////////////
		/// Memory for raylib keys and buttons
		//////////////////////////////////////////
		bool	m_keysDown[KEYS_SIZE]					= { false };
		bool	m_keysPressed[KEYS_SIZE]				= { false };
		bool	m_keysReleased[KEYS_SIZE]				= { false };

		bool	m_mouseDown[MOUSE_BUTTONS_SIZE]			= { false };
		bool	m_mousePressed[MOUSE_BUTTONS_SIZE]		= { false };
		bool	m_mouseReleased[MOUSE_BUTTONS_SIZE]		= { false };

		Vector2	m_mousePosition		= { 0.0f, 0.0f };
		Vector2	m_mouseDelta		= { 0.0f, 0.0f };
		float	m_mouseWheel		= 0.0f;
	};
}
