#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <raylib.h>

namespace eng
{
	class Camera;

	/// <summary>
	/// Immediate-mode debug shapes, owned by the Engine. Submit from anywhere during the frame:
	/// 3D shapes are drawn at the end of the 3D pass, labels after it (2D, projected).
	/// duration 0 = this frame only; > 0 = seconds (unscaled time). Disabled = calls are ignored.
	/// Enabled by default in editor builds (ENG_EDITOR=1).
	/// </summary>
	class DebugDraw
	{
	public:
		void	SetEnabled(bool enabled);
		bool	IsEnabled() const;

		void	Line		(const Vector3& from, const Vector3& to, Color color, float duration = 0.0f);
		void	WireBox		(const Vector3& center, const Vector3& size, Color color, float duration = 0.0f);
		void	WireSphere	(const Vector3& center, float radius, Color color, float duration = 0.0f);
		void	Axes		(const Vector3& origin, float length = 1.0f, float duration = 0.0f);	// X red, Y green, Z blue
		void	Grid		(int slices, float spacing, float duration = 0.0f);
		void	Label		(const Vector3& position, const std::string& text, Color color, float duration = 0.0f);

		//////////////////////////////////////////
		/// Called by the Engine
		//////////////////////////////////////////
		void	Draw3D() const;
		void	DrawLabels(const Camera& camera) const;
		void	EndFrame(float unscaledDeltaTime);	// drops expired items

		size_t	GetItemCount() const;

	private:
		enum class ShapeType : uint8_t { Line, WireBox, WireSphere, Grid };

		struct Shape
		{
			ShapeType	type;
			Vector3		a;			// from / center
			Vector3		b;			// to / size
			float		radius;		// sphere radius / grid spacing
			int			slices;		// grid
			Color		color;
			float		timeLeft;
		};

		struct LabelItem
		{
			Vector3		position;
			std::string	text;
			Color		color;
			float		timeLeft;
		};

		bool					m_enabled	= (ENG_EDITOR == 1);
		std::vector<Shape>		m_shapes;
		std::vector<LabelItem>	m_labels;
	};
}
