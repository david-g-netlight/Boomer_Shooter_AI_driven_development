#include "Component.h"

namespace eng
{
	GameActor* Component::GetOwner() const
	{
		return m_owner;
	}

	bool Component::IsEnabled() const
	{
		return m_isEnabled;
	}

	void Component::SetEnabled(bool enabled)
	{
		if (enabled == m_isEnabled)
			return;

		m_isEnabled = enabled;
		if (enabled)
			OnEnable();
		else
			OnDisable();
	}
}
