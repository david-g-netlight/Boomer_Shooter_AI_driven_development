#pragma once
#include <raylib.h>

namespace eng { class InputMap; }

/// <summary>
/// What the player asks for this frame (Unity InputManager): walk direction, look delta, jump and fire.
/// Reads the engine InputMap in Update(); the controller, the look and the gun read the results.
///
/// Jump and fire are *buffered* for a short time: FixedUpdate can run 0..N times per frame, so a click read in
/// Update() must still be there when the fixed step comes. Consume*() takes the request.
///
/// SetControl(false) cuts everything (win / lose screens): zero movement and look, no jump, no fire.
/// Control is also off whenever the engine is not in Play.
/// </summary>
class PlayerInput
{
public:
	static constexpr float	BUFFER_SECONDS	= 0.1f;

	/// <summary>
	/// Move (WASD / arrows), Look (mouse), Jump (Space), Fire (LMB).
	/// </summary>
	static void		BindDefaults(eng::InputMap& map);

	/// <summary>
	/// Once per frame, before anything reads it. Reads the map, then applies the result.
	/// </summary>
	void			Update(float deltaTime, const eng::InputMap& map);

	/// <summary>
	/// The part of Update() that doesn't touch the InputMap (also how the tests feed input).
	/// direction: x right, y forward, length <= 1. look: mouse delta in points.
	/// </summary>
	void			Apply(float deltaTime, Vector2 direction, Vector2 look, bool jumpPressed, bool firePressed, bool isInPlay);

	void			SetControl(bool hasControl);
	bool			HasControl() const;

	Vector2			GetDirection() const;
	Vector2			GetLook() const;
	bool			ConsumeJump();		// true once per press (buffered)
	bool			ConsumeFire();		// true once per click (buffered)

private:
	bool			m_hasControl	= true;
	Vector2			m_direction		= { 0.0f, 0.0f };
	Vector2			m_look			= { 0.0f, 0.0f };
	float			m_jumpBuffer	= 0.0f;
	float			m_fireBuffer	= 0.0f;
};
