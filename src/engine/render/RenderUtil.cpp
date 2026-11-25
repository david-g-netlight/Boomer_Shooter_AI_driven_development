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

	bool HasTransparentTexels(const Texture2D& texture)
	{
		Image image = LoadImageFromTexture(texture);
		ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
		const Color* pixels = static_cast<const Color*>(image.data);
		bool hasTransparency = false;
		for (int i = 0; pixels && i < image.width * image.height && !hasTransparency; ++i)
			hasTransparency = pixels[i].a < 255;
		UnloadImage(image);
		return hasTransparency;
	}
}
