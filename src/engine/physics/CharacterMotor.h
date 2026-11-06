#pragma once
#include <memory>
#include <raylib.h>
#include "level/Component.h"

namespace eng
{
	class Collider;

	struct CharacterMotorSettings
	{
		float	mass				= 80.0f;	// kg: pushes light crates, heavy ones barely move
		float	gravity				= 32.0f;	// m/s^2, own gravity (Unity jump arc: 2h / t_apex^2, h 1 m, t 0.25 s)
		float	maxSlopeDegrees		= 45.0f;	// steeper ground is not ground: the capsule slides and can't climb it
		float	airControl			= 0.5f;		// fraction of the walk input applied per 0.1 s while airborne
		float	groundGlue			= -0.05f;	// m/s down while grounded, keeps contact on down-slopes (Unity grounded gravity)
		float	jumpGraceSeconds	= 0.1f;		// ground checks ignored after a jump
		float	maxVerticalSpeed	= 15.0f;	// m/s clamp (e.g. a crate scaled up under the player)
		float	groundProbe			= 0.1f;		// m below the capsule's bottom that still counts as touching
	};

	/// <summary>
	/// The player's body: a dynamic Bullet capsule that never rotates, built from the capsule Collider on the
	/// same actor (layer Player). Set the walk velocity every tick; the motor follows the ground, slides on
	/// slopes steeper than maxSlope, jumps, pushes props and rides moving ones.
	/// The actor's rotation is never written (yaw comes from mouse look); its position is the capsule center.
	/// </summary>
	class CharacterMotor : public Component
	{
	public:
		CharacterMotor();
		explicit CharacterMotor(const CharacterMotorSettings& settings);
		~CharacterMotor() override;

		/// <summary>
		/// Desired horizontal velocity (m/s, world space; y ignored). Applied on the next fixed tick.
		/// </summary>
		void			SetWalkVelocity(const Vector3& velocity);

		/// <summary>
		/// Requests a jump with this upward speed. Honoured on the next fixed tick if still grounded.
		/// Returns false if not grounded now.
		/// </summary>
		bool			Jump(float speed);

		void			SetGravity(float gravity);
		void			Warp(const Vector3& position);	// teleport: clears velocity, walk input and a pending jump

		bool			IsGrounded() const;
		const Vector3&	GetGroundNormal() const;
		float			GetGroundSlopeDegrees() const;	// slope under the capsule (also when too steep)
		bool			IsOnSteepSlope() const;
		Vector3			GetVelocity() const;
		const CharacterMotorSettings& GetSettings() const;

		//////////////////////////////////////////
		/// Component
		//////////////////////////////////////////
		void			OnEnable() override;
		void			OnDisable() override;
		void			FixedUpdate(float fixedDeltaTime) override;
		const char*		TypeName() const override { return "CharacterMotor"; }

	private:
		void			CreateBody();
		void			DestroyBody();
		void			ProbeGround();

		struct Impl;
		std::unique_ptr<Impl>	m_impl;
		CharacterMotorSettings	m_settings;
		Collider*				m_collider			= nullptr;

		Vector3					m_walkVelocity		= { 0.0f, 0.0f, 0.0f };
		float					m_jumpRequest		= 0.0f;		// > 0: jump speed to apply
		float					m_jumpGraceLeft		= 0.0f;

		bool					m_isGrounded		= false;
		bool					m_isOnSteepSlope	= false;
		Vector3					m_groundNormal		= { 0.0f, 1.0f, 0.0f };
		Vector3					m_groundVelocity	= { 0.0f, 0.0f, 0.0f };
	};
}
