#pragma once
#include <eng.h>

class PlayerInput;
class PlayerState;
class FirstPersonLook;

/// <summary>
/// Walking and jumping (Unity FirstPersonController) on top of the engine's CharacterMotor.
/// Every fixed step: the camera-relative input direction times the walk speed becomes the motor's walk velocity.
/// The jump arc is set through the capsule's own gravity: apex time 0.25 s and height 1 m give
/// g = 2h / t^2 = 32 m/s^2 and v0 = 2h / t = 8 m/s.
/// A jump pressed shortly before landing is honored when the capsule touches the ground (PlayerInput buffers it).
/// </summary>
class FirstPersonController : public eng::Component
{
public:
	static constexpr float	JUMP_HEIGHT		= 1.0f;		// m
	static constexpr float	APEX_TIME		= 0.25f;	// s
	static constexpr float	GRAVITY			= 2.0f * JUMP_HEIGHT / (APEX_TIME * APEX_TIME);		// 32 m/s^2
	static constexpr float	JUMP_SPEED		= 2.0f * JUMP_HEIGHT / APEX_TIME;					// 8 m/s
	static constexpr float	STEP_MIN_SPEED	= 0.5f;		// m/s, slower than this and the footsteps stop

	void			Bind(PlayerInput* input, PlayerState* state);

	void			OnEnable() override;
	void			FixedUpdate(float fixedDeltaTime) override;
	// Stops the footstep loop (the arena calls it on death and on level load)
	void			StopFootsteps();
	const char*		TypeName() const override { return "FirstPersonController"; }

private:
	PlayerInput*	m_input		= nullptr;
	PlayerState*	m_state		= nullptr;
	eng::VoiceHandle	m_stepVoice;		// the footstep loop, while it plays
};
