#include "DebugDraw.h"
#include "Camera.h"
#include <algorithm>
#include <raymath.h>

namespace eng
{
	void DebugDraw::SetEnabled(bool enabled)
	{
		m_enabled = enabled;
		if (!enabled)
		{
			m_shapes.clear();
			m_labels.clear();
		}
	}

	bool DebugDraw::IsEnabled() const { return m_enabled; }

	void DebugDraw::Line(const Vector3& from, const Vector3& to, Color color, float duration)
	{
		if (m_enabled) m_shapes.push_back({ ShapeType::Line, from, to, 0.0f, 0, color, duration });
	}

	void DebugDraw::WireBox(const Vector3& center, const Vector3& size, Color color, float duration)
	{
		if (m_enabled) m_shapes.push_back({ ShapeType::WireBox, center, size, 0.0f, 0, color, duration });
	}

	void DebugDraw::WireSphere(const Vector3& center, float radius, Color color, float duration)
	{
		if (m_enabled) m_shapes.push_back({ ShapeType::WireSphere, center, {}, radius, 0, color, duration });
	}

	void DebugDraw::Axes(const Vector3& origin, float length, float duration)
	{
		Line(origin, Vector3Add(origin, { length, 0.0f, 0.0f }), RED, duration);
		Line(origin, Vector3Add(origin, { 0.0f, length, 0.0f }), GREEN, duration);
		Line(origin, Vector3Add(origin, { 0.0f, 0.0f, length }), BLUE, duration);
	}

	void DebugDraw::Grid(int slices, float spacing, float duration)
	{
		if (m_enabled) m_shapes.push_back({ ShapeType::Grid, {}, {}, spacing, slices, WHITE, duration });
	}

	void DebugDraw::Label(const Vector3& position, const std::string& text, Color color, float duration)
	{
		if (m_enabled) m_labels.push_back({ position, text, color, duration });
	}

	void DebugDraw::Draw3D() const
	{
		for (const Shape& shape : m_shapes)
		{
			switch (shape.type)
			{
				case ShapeType::Line:		DrawLine3D(shape.a, shape.b, shape.color);						break;
				case ShapeType::WireBox:	DrawCubeWiresV(shape.a, shape.b, shape.color);					break;
				case ShapeType::WireSphere:	DrawSphereWires(shape.a, shape.radius, 8, 12, shape.color);		break;
				case ShapeType::Grid:		DrawGrid(shape.slices, shape.radius);							break;
			}
		}
	}

	void DebugDraw::DrawLabels(const Camera& camera) const
	{
		const Camera3D	raylibCamera	= camera.ToRaylib();
		const Vector3	forward			= camera.Forward();

		for (const LabelItem& label : m_labels)
		{
			// Skip labels behind the camera: GetWorldToScreen mirrors them onto the screen
			const Vector3 toLabel = Vector3Subtract(label.position, camera.GetPosition());
			if (Vector3DotProduct(toLabel, forward) <= 0.0f)
				continue;

			const Vector2	screen	= GetWorldToScreen(label.position, raylibCamera);
			const int		width	= MeasureText(label.text.c_str(), 10);
			DrawText(label.text.c_str(), static_cast<int>(screen.x) - width / 2, static_cast<int>(screen.y) - 5, 10, label.color);
		}
	}

	void DebugDraw::EndFrame(float unscaledDeltaTime)
	{
		auto expired = [unscaledDeltaTime](auto& item)
		{
			item.timeLeft -= unscaledDeltaTime;
			return item.timeLeft <= 0.0f;
		};
		m_shapes.erase(std::remove_if(m_shapes.begin(), m_shapes.end(), expired), m_shapes.end());
		m_labels.erase(std::remove_if(m_labels.begin(), m_labels.end(), expired), m_labels.end());
	}

	size_t DebugDraw::GetItemCount() const
	{
		return m_shapes.size() + m_labels.size();
	}
}
