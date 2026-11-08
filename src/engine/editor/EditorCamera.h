#pragma once
#include "render/Camera.h"

namespace eng
{
	class InputManager;

	/// <summary>
	/// Free-fly camera of the editor. Hold RMB to look (cursor captured while held) and fly with
	/// W/A/S/D (forward/left/back/right) and E/Q (up/down); Shift is faster; the wheel changes the fly speed.
	/// Uses unscaled time, so it works while the simulation is frozen.
	/// </summary>
	class EditorCamera
	{
	public:
		/// <summary>
		/// Starts from the pose and projection of `source` (the game camera when the editor opens).
		/// </summary>
		void			CopyPoseFrom(const Camera& source);

		void			Update(InputManager& input, float unscaledDeltaTime);

		/// <summary>
		/// Flies to a spot that frames a sphere (the F key on the selection).
		/// </summary>
		void			FocusOn(const Vector3& center, float radius);

		Camera&			GetCamera();
		const Camera&	GetCamera() const;

		float			GetFlySpeed() const;
		void			SetFlySpeed(float metersPerSecond);

	private:
		Camera			m_camera;
		float			m_flySpeed	= 8.0f;
		bool			m_isLooking	= false;
	};
}
