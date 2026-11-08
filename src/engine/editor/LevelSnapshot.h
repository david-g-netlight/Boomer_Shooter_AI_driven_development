#pragma once
#include <string>
#include <vector>
#include <raylib.h>

namespace eng
{
	class GameActor;
	class Level;

	/// <summary>
	/// The state of every actor of a Level at one moment: active flag, local transform and, for rigid bodies,
	/// the linear and angular velocity. Taken when the editor opens; Restore() is the toolbar's Reset.
	///
	/// Actors are matched by address and name. Actors created after the capture are marked for destroy;
	/// actors deleted after the capture are only reported (re-creating them needs the LevelSerializer, which
	/// will take over the capture format).
	/// </summary>
	class LevelSnapshot
	{
	public:
		struct RestoreResult
		{
			int restored	= 0;
			int removed		= 0;	// created since the capture
			int missing		= 0;	// deleted since the capture, cannot be brought back
		};

		void			Capture(const Level& level);

		/// <summary>
		/// Positions, scales, active flags and velocities go back to the captured values.
		/// Call between frames (not inside a level update).
		/// </summary>
		RestoreResult	Restore(Level& level) const;

		bool			IsEmpty() const;
		size_t			GetActorCount() const;
		void			Clear();

	private:
		struct ActorState
		{
			const GameActor*	actor		= nullptr;
			std::string			name;
			bool				isActive	= true;
			::Transform			transform	= {};
			bool				hasBody		= false;
			Vector3				linearVelocity	= {};
			Vector3				angularVelocity	= {};
			bool				hasMotor	= false;
		};

		std::vector<ActorState>	m_states;
		bool					m_hasCapture	= false;
	};
}
