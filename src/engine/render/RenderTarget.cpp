#include "RenderTarget.h"
#include "core/Log.h"

namespace eng
{
	RenderTarget::~RenderTarget()
	{
		// After CloseWindow() there is no GL context left to release into
		if (IsValid() && IsWindowReady())
			Unload();
	}

	RenderTarget::RenderTarget(RenderTarget&& other) noexcept
		: m_target(other.m_target)
	{
		other.m_target = {};
	}

	RenderTarget& RenderTarget::operator = (RenderTarget&& other) noexcept
	{
		if (this != &other)
		{
			Unload();
			m_target		= other.m_target;
			other.m_target	= {};
		}
		return *this;
	}

	bool RenderTarget::Create(int width, int height)
	{
		Unload();
		if (width <= 0 || height <= 0)
		{
			Log::Warning("RenderTarget: invalid size %d x %d", width, height);
			return false;
		}

		m_target = LoadRenderTexture(width, height);
		if (!IsRenderTextureValid(m_target))
		{
			Log::Error("RenderTarget: failed to create %d x %d", width, height);
			m_target = {};
			return false;
		}

		SetTextureFilter(m_target.texture, TEXTURE_FILTER_BILINEAR);
		return true;
	}

	void RenderTarget::Resize(int width, int height)
	{
		if (IsValid() && width == GetWidth() && height == GetHeight())
			return;
		Create(width, height);
	}

	void RenderTarget::Unload()
	{
		if (IsValid())
			UnloadRenderTexture(m_target);
		m_target = {};
	}

	bool RenderTarget::IsValid() const			{ return m_target.id != 0; }
	int RenderTarget::GetWidth() const			{ return m_target.texture.width; }
	int RenderTarget::GetHeight() const			{ return m_target.texture.height; }
	const Texture2D& RenderTarget::GetTexture() const { return m_target.texture; }

	void RenderTarget::Begin() const
	{
		if (IsValid())
			BeginTextureMode(m_target);
	}

	void RenderTarget::End() const
	{
		if (IsValid())
			EndTextureMode();
	}

	void RenderTarget::DrawToScreen(const Rectangle& destination, Color tint) const
	{
		if (!IsValid())
			return;

		// OpenGL textures are stored bottom-up: a negative source height flips them
		const Rectangle source = { 0.0f, 0.0f, static_cast<float>(GetWidth()), -static_cast<float>(GetHeight()) };
		DrawTexturePro(m_target.texture, source, destination, { 0.0f, 0.0f }, 0.0f, tint);
	}
}
