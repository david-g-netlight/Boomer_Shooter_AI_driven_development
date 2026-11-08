#include "InspectorLayout.h"
#include <cstdio>
#include <cstdlib>
#include "editor/EditorUI.h"

namespace eng
{
	void InspectorLayout::Begin(const Rectangle& area, float scrollY)
	{
		m_x			= area.x + EditorUI::PADDING;
		m_width		= area.width - 2.0f * EditorUI::PADDING;
		m_startY	= area.y + scrollY;
		m_y			= m_startY;
		m_nextId	= 0;
	}

	float InspectorLayout::End()
	{
		return m_y - m_startY;
	}

	void InspectorLayout::CancelEditing()
	{
		m_activeId = -1;
	}

	bool InspectorLayout::IsEditing() const
	{
		return m_activeId != -1;
	}

	Rectangle InspectorLayout::NextRow()
	{
		const Rectangle row = { m_x, m_y, m_width, EditorUI::ROW_HEIGHT };
		m_y += EditorUI::ROW_HEIGHT + 3.0f;
		return row;
	}

	void InspectorLayout::Header(const char* title)
	{
		m_y += 4.0f;
		const Rectangle row = NextRow();
		DrawRectangleRec(row, Fade(DARKBLUE, 0.85f));
		DrawText(title, static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 5.0f), EditorUI::TEXT_SIZE, RAYWHITE);
	}

	void InspectorLayout::Info(const char* label, const char* text)
	{
		const Rectangle row = NextRow();
		GuiLabel({ row.x, row.y, row.width * 0.4f, row.height }, label);
		GuiLabel({ row.x + row.width * 0.4f, row.y, row.width * 0.6f, row.height }, text);
	}

	bool InspectorLayout::NumberBox(const Rectangle& bounds, float& value, bool wholeNumber)
	{
		const int id = m_nextId++;
		if (id == m_activeId)
		{
			if (GuiTextBox(bounds, m_text, sizeof(m_text), true))	// Enter, or a click elsewhere
			{
				m_activeId = -1;
				value = static_cast<float>(std::atof(m_text));
				return true;
			}
			return false;
		}

		char shown[32];
		std::snprintf(shown, sizeof(shown), wholeNumber ? "%.0f" : "%.3f", static_cast<double>(value));
		if (GuiTextBox(bounds, shown, sizeof(shown), false))
		{
			m_activeId = id;
			std::snprintf(m_text, sizeof(m_text), "%s", shown);
		}
		return false;
	}

	bool InspectorLayout::Float(const char* label, float& value)
	{
		const Rectangle row = NextRow();
		GuiLabel({ row.x, row.y, row.width * 0.4f, row.height }, label);
		return NumberBox({ row.x + row.width * 0.4f, row.y, row.width * 0.6f, row.height }, value, false);
	}

	bool InspectorLayout::Vec3(const char* label, Vector3& value)
	{
		GuiLabel(NextRow(), label);
		const Rectangle row = NextRow();
		const float w = (row.width - 2.0f * 4.0f) / 3.0f;
		bool changed = false;
		changed |= NumberBox({ row.x, row.y, w, row.height }, value.x, false);
		changed |= NumberBox({ row.x + w + 4.0f, row.y, w, row.height }, value.y, false);
		changed |= NumberBox({ row.x + 2.0f * (w + 4.0f), row.y, w, row.height }, value.z, false);
		return changed;
	}

	bool InspectorLayout::Tint(const char* label, ::Color& value)
	{
		GuiLabel(NextRow(), label);
		const Rectangle row = NextRow();
		const float w = (row.width - 3.0f * 4.0f) / 4.0f;
		unsigned char* channels[4] = { &value.r, &value.g, &value.b, &value.a };
		bool changed = false;
		for (int i = 0; i < 4; ++i)
		{
			float v = static_cast<float>(*channels[i]);
			if (NumberBox({ row.x + i * (w + 4.0f), row.y, w, row.height }, v, true))
			{
				*channels[i] = static_cast<unsigned char>(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
				changed = true;
			}
		}
		return changed;
	}

	bool InspectorLayout::Bool(const char* label, bool& value)
	{
		const Rectangle row = NextRow();
		const bool before = value;
		GuiCheckBox({ row.x, row.y + 3.0f, row.height - 6.0f, row.height - 6.0f }, label, &value);
		return value != before;
	}

	bool InspectorLayout::Choice(const char* label, const char* const* names, int count, int& index)
	{
		const Rectangle row = NextRow();
		GuiLabel({ row.x, row.y, row.width * 0.4f, row.height }, label);
		if (GuiButton({ row.x + row.width * 0.4f, row.y, row.width * 0.6f, row.height }, names[index]))
		{
			index = (index + 1) % count;
			return true;
		}
		return false;
	}

	bool InspectorLayout::Button(const char* text)
	{
		return GuiButton(NextRow(), text) != 0;
	}
}
