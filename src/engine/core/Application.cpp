#include "Application.h"
#include "EngineInfo.h"

namespace eng
{
	const char* Application::GetTitle() const
	{
		return GetEngineName();
	}

	void Application::SetNeedsToBeClosed(bool value)
	{
		m_needsToBeClosed = value;
	}

	bool Application::NeedsToBeClosed() const
	{
		return m_needsToBeClosed;
	}
}
