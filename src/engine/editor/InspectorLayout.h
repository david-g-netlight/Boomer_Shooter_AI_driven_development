#pragma once
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// Immediate-mode, top-to-bottom widget column for the inspector (raygui underneath). Components draw their
	/// fields through it in Component::DrawInspector(). Every widget returns true on the frame its value changed.
	///
	/// Number fields are text boxes: click, type, Enter (or click away) commits. At most one is edited at a time;
	/// fields are identified by their order, so a component must draw the same widgets in the same order each frame.
	/// </summary>
	class InspectorLayout
	{
	public:
		//////////////////////////////////////////
		/// Called by the InspectorPanel
		//////////////////////////////////////////
		void	Begin(const Rectangle& area, float scrollY);
		float	End();					// content height drawn since Begin()
		void	CancelEditing();		// the selection changed
		bool	IsEditing() const;		// a number field has the keyboard

		//////////////////////////////////////////
		/// Widgets
		//////////////////////////////////////////
		void	Header(const char* title);
		void	Info(const char* label, const char* text);
		bool	Float(const char* label, float& value);
		bool	Vec3(const char* label, Vector3& value);
		bool	Tint(const char* label, ::Color& value);		// R G B A, 0..255
		bool	Bool(const char* label, bool& value);
		bool	Choice(const char* label, const char* const* names, int count, int& index);	// click cycles
		bool	Button(const char* text);

	private:
		bool	NumberBox(const Rectangle& bounds, float& value, bool wholeNumber);
		Rectangle NextRow();

		float		m_x			= 0.0f;
		float		m_width		= 0.0f;
		float		m_startY	= 0.0f;
		float		m_y			= 0.0f;
		int			m_nextId	= 0;
		int			m_activeId	= -1;		// the number field being edited
		char		m_text[32]	= {};
	};
}
