#pragma once
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// Clears only the depth buffer of the current framebuffer (rlgl has no depth-only clear). Flushes rlgl's
	/// pending batch first, so what was queued before is drawn with the old depth. Used by passes that must
	/// draw over the world with depth testing of their own (the view model).
	/// </summary>
	void ClearDepthBuffer();

	/// <summary>
	/// True when a texel of the texture has alpha below 255 (read back from the GPU: call it at load, not per frame).
	/// Decides whether a model's material needs alphaCutoff.
	/// </summary>
	bool HasTransparentTexels(const Texture2D& texture);
}
