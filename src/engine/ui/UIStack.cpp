#include "UIStack.h"
#include <algorithm>
#include <raygui.h>

namespace eng
{
	void UIStack::Push(std::unique_ptr<UIScreen> screen)
	{
		if (!screen)
			return;

		if (m_isBusy)
		{
			m_pendingPush.push_back(std::move(screen));
			return;
		}

		screen->OnPush();
		m_screens.push_back(std::move(screen));
	}

	void UIStack::Pop()
	{
		if (!m_screens.empty())
			Remove(m_screens.back().get());
	}

	void UIStack::Remove(const UIScreen* screen)
	{
		if (!screen)
			return;

		if (m_isBusy)
		{
			m_pendingRemove.push_back(screen);
			return;
		}

		auto it = std::find_if(m_screens.begin(), m_screens.end(), [screen](const auto& s) { return s.get() == screen; });
		if (it == m_screens.end())
			return;
		(*it)->OnPop();
		m_screens.erase(it);
	}

	void UIStack::Clear()
	{
		if (m_isBusy)
		{
			for (const auto& screen : m_screens)
				m_pendingRemove.push_back(screen.get());
			m_pendingPush.clear();
			return;
		}

		while (!m_screens.empty())
			Pop();
	}

	UIScreen* UIStack::Top() const
	{
		return m_screens.empty() ? nullptr : m_screens.back().get();
	}

	bool UIStack::Contains(const UIScreen* screen) const
	{
		return std::any_of(m_screens.begin(), m_screens.end(), [screen](const auto& s) { return s.get() == screen; });
	}

	bool UIStack::IsEmpty() const	{ return m_screens.empty(); }
	size_t UIStack::GetCount() const	{ return m_screens.size(); }

	void UIStack::Update(float unscaledDeltaTime)
	{
		m_isBusy = true;
		if (UIScreen* top = Top())
			top->Update(unscaledDeltaTime);
		m_isBusy = false;
		ApplyPending();
	}

	void UIStack::Draw()
	{
		if (m_screens.empty())
			return;

		// From the topmost opaque screen up to the top; only the top one is interactive
		size_t first = m_screens.size() - 1;
		while (first > 0 && !m_screens[first]->IsOpaque())
			--first;

		m_isBusy = true;
		for (size_t i = first; i < m_screens.size(); ++i)
		{
			const bool isTop = i + 1 == m_screens.size();
			if (!isTop)
				GuiLock();
			m_screens[i]->Draw();
			if (!isTop)
				GuiUnlock();
		}
		m_isBusy = false;
		ApplyPending();
	}

	void UIStack::ApplyPending()
	{
		std::vector<const UIScreen*> removals;
		removals.swap(m_pendingRemove);
		for (const UIScreen* screen : removals)
			Remove(screen);

		std::vector<std::unique_ptr<UIScreen>> pushes;
		pushes.swap(m_pendingPush);
		for (auto& screen : pushes)
			Push(std::move(screen));
	}
}
