#pragma once
#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include "ui/UIScreen.h"

namespace eng
{
	/// <summary>
	/// Stack of UI screens, owned by the Engine and drawn on top of everything (after the HUD).
	/// The top screen gets input; screens below are drawn locked (visible, not interactive) up to the
	/// topmost opaque one. Changes made while the stack is updating or drawing apply right after.
	/// </summary>
	class UIStack
	{
	public:
		UIStack() = default;
		UIStack(const UIStack&) = delete;
		UIStack& operator = (const UIStack&) = delete;

		template<typename T, typename... Args>
		requires std::derived_from<T, UIScreen>
		T* Push(Args&&... args)
		{
			auto screen	= std::make_unique<T>(std::forward<Args>(args)...);
			T* raw		= screen.get();
			Push(std::move(screen));
			return raw;
		}

		void		Push(std::unique_ptr<UIScreen> screen);
		void		Pop();							// top screen
		void		Remove(const UIScreen* screen);	// any screen; ignored if not on the stack
		void		Clear();

		UIScreen*	Top() const;
		bool		Contains(const UIScreen* screen) const;
		bool		IsEmpty() const;
		size_t		GetCount() const;

		//////////////////////////////////////////
		/// Called by the Engine
		//////////////////////////////////////////
		void		Update(float unscaledDeltaTime);
		void		Draw();

	private:
		void		ApplyPending();

		std::vector<std::unique_ptr<UIScreen>>	m_screens;
		std::vector<std::unique_ptr<UIScreen>>	m_pendingPush;
		std::vector<const UIScreen*>			m_pendingRemove;
		bool									m_isBusy		= false;	// inside Update / Draw
	};
}
