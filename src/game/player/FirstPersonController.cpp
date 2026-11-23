#include "FirstPersonController.h"
#include <raymath.h>
#include "player/FirstPersonLook.h"
#include "player/PlayerInput.h"
#include "state/PlayerState.h"
#include "state/SoundFXBank.h"

void FirstPersonController::Bind(PlayerInput* input, PlayerState* state)
{
	m_input	= input;
	m_state	= state;
}

void FirstPersonController::OnEnable()
{
	if (eng::CharacterMotor* motor = GetOwner()->GetComponent<eng::CharacterMotor>())
		motor->SetGravity(GRAVITY);
}

void FirstPersonController::FixedUpdate(float /*fixedDeltaTime*/)
{
	eng::CharacterMotor*	motor	= GetOwner()->GetComponent<eng::CharacterMotor>();
	const FirstPersonLook*	look	= GetOwner()->GetComponent<FirstPersonLook>();
	if (!motor || !look || !m_input || !m_state)
		return;

	const Vector2 direction	= m_input->GetDirection();		// x right, y forward; zero without control
	const Vector3 wish		= Vector3Add(Vector3Scale(look->GetPlanarForward(), direction.y), Vector3Scale(look->GetPlanarRight(), direction.x));
	motor->SetWalkVelocity(Vector3Scale(wish, PlayerState::WALK_SPEED));

	// Take the buffered jump only once there is ground under the capsule
	if (motor->IsGrounded() && m_input->ConsumeJump())
	{
		motor->Jump(JUMP_SPEED);
		SoundFXBank::Instance().Play(Sfx::PlayerJump);
	}
}
