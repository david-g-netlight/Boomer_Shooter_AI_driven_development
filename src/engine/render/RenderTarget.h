#pragma once
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// Off-screen color + depth buffer (raylib RenderTexture2D), e.g. editor viewport, minimap, gun screen.
	///
	///   target.Begin();  ClearBackground(...);  camera.Begin3D(); ... camera.End3D();  target.End();
	///   target.DrawToScreen({ x, y, w, h });
	///
	/// Must be created after the window exists. The destructor only releases the GPU texture while the
	/// window is still open; call Unload() explicitly in Application::Destroy().
	/// </summary>
	class RenderTarget
	{
	public:
		RenderTarget() = default;
		~RenderTarget();

		RenderTarget(const RenderTarget&) = delete;
		RenderTarget& operator = (const RenderTarget&) = delete;
		RenderTarget(RenderTarget&& other) noexcept;
		RenderTarget& operator = (RenderTarget&& other) noexcept;

		bool				Create(int width, int height);
		void				Resize(int width, int height);	// no-op if the size is unchanged
		void				Unload();

		bool				IsValid() const;
		int					GetWidth() const;
		int					GetHeight() const;

		void				Begin() const;
		void				End() const;

		const Texture2D&	GetTexture() const;

		/// <summary>
		/// Draws the color texture into a screen rectangle (handles the texture's flipped Y).
		/// </summary>
		void				DrawToScreen(const Rectangle& destination, Color tint = WHITE) const;

	private:
		RenderTexture2D		m_target = {};
	};
}
