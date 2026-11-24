#pragma once

namespace eng
{
	/// <summary>
	/// Clears only the depth buffer of the current framebuffer (rlgl has no depth-only clear). Flushes rlgl's
	/// pending batch first, so what was queued before is drawn with the old depth. Used by passes that must
	/// draw over the world with depth testing of their own (the view model).
	/// </summary>
	void ClearDepthBuffer();
}
