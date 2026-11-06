#include "LineRenderer.h"
#include "render/RenderQueue.h"
#include <algorithm>

namespace eng
{
	void LineRenderer::SetPoints(const Vector3& start, const Vector3& end)	{ m_start = start; m_end = end; }
	void LineRenderer::SetColors(Color start, Color end)					{ m_startColor = start; m_endColor = end; }
	void LineRenderer::SetWidth(float startWidth, float endWidth)			{ m_startWidth = std::max(0.0f, startWidth); m_endWidth = std::max(0.0f, endWidth); }
	void LineRenderer::SetVisible(bool visible)								{ m_isVisible = visible; }
	bool LineRenderer::IsVisible() const									{ return m_isVisible; }
	const Vector3& LineRenderer::GetStart() const							{ return m_start; }
	const Vector3& LineRenderer::GetEnd() const								{ return m_end; }

	void LineRenderer::FadeOut(float seconds)
	{
		m_fadeDuration	= std::max(seconds, 0.001f);
		m_fadeElapsed	= 0.0f;
		m_isVisible		= true;
	}

	float LineRenderer::GetOpacity() const
	{
		return m_fadeDuration > 0.0f ? std::clamp(1.0f - m_fadeElapsed / m_fadeDuration, 0.0f, 1.0f) : 1.0f;
	}

	void LineRenderer::Update(float deltaTime)
	{
		if (m_fadeDuration <= 0.0f)
			return;

		m_fadeElapsed += deltaTime;
		if (m_fadeElapsed >= m_fadeDuration)
			m_isVisible = false;
	}

	void LineRenderer::SubmitRenderables(RenderQueue& queue) const
	{
		if (!m_isVisible)
			return;

		const float opacity = GetOpacity();
		Color start	= m_startColor;
		Color end	= m_endColor;
		start.a		= static_cast<unsigned char>(start.a * opacity);
		end.a		= static_cast<unsigned char>(end.a * opacity);

		LineCommand line;
		line.start			= m_start;
		line.end			= m_end;
		line.startColor		= start;
		line.endColor		= end;
		line.startRadius	= 0.5f * m_startWidth;
		line.endRadius		= 0.5f * m_endWidth;
		queue.SubmitLine(line);
	}
}
