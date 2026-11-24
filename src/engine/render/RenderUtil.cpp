#include "RenderUtil.h"
#include <rlgl.h>

#ifdef __APPLE__
	#define GL_SILENCE_DEPRECATION
	#include <OpenGL/gl3.h>
#endif

namespace eng
{
	void ClearDepthBuffer()
	{
		rlDrawRenderBatchActive();
#ifdef __APPLE__
		glClear(GL_DEPTH_BUFFER_BIT);
#endif		// other platforms: not implemented yet (the spike targets macOS)
	}
}
