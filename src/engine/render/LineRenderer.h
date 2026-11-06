#pragma once
#include <raylib.h>
#include "level/Component.h"

namespace eng
{
	/// <summary>
	/// A thick line with a color / width gradient between two world-space points (beams, lasers).
	/// Optional fade-out: after FadeOut(seconds) the alpha drops to 0 and the line hides itself.
	/// Drawn in the RenderQueue's transparent pass.
	/// </summary>
	class LineRenderer : public Component
	{
	public:
		void			SetPoints(const Vector3& start, const Vector3& end);
		void			SetColors(Color start, Color end);
		void			SetWidth(float startWidth, float endWidth);	// meters; 0 = 1 pixel
		void			SetVisible(bool visible);
		bool			IsVisible() const;

		/// <summary>
		/// Fades the alpha linearly to 0 over `seconds`, then hides the line. Restarts the fade if called again.
		/// </summary>
		void			FadeOut(float seconds);
		float			GetOpacity() const;	// 1 = as set, 0 = faded

		const Vector3&	GetStart() const;
		const Vector3&	GetEnd() const;

		void			Update(float deltaTime) override;
		void			SubmitRenderables(RenderQueue& queue) const override;
		const char*		TypeName() const override { return "LineRenderer"; }

	private:
		Vector3	m_start			= { 0.0f, 0.0f, 0.0f };
		Vector3	m_end			= { 0.0f, 0.0f, 0.0f };
		Color	m_startColor	= { 255, 255, 255, 255 };
		Color	m_endColor		= { 255, 255, 255, 255 };
		float	m_startWidth	= 0.04f;
		float	m_endWidth		= 0.04f;
		bool	m_isVisible		= true;

		float	m_fadeDuration	= 0.0f;		// 0 = not fading
		float	m_fadeElapsed	= 0.0f;
	};
}
