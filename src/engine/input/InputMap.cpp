#include "InputMap.h"
#include "InputManager.h"
#include "core/Log.h"
#include <cmath>

namespace eng
{
	InputMap::InputMap(const InputManager& input)
		: m_input(input)
	{
	}

	//////////////////////////////////////////
	/// Bindings
	//////////////////////////////////////////
	void InputMap::BindAction(const std::string& action, InputBinding binding)
	{
		m_actions[action].push_back(binding);
	}

	void InputMap::BindAxis(const std::string& axis, AxisBinding binding)
	{
		m_axes[axis].push_back(binding);
	}

	void InputMap::BindAxis2D(const std::string& axis, const std::string& xAxis, const std::string& yAxis, bool clampToUnitLength)
	{
		m_axes2D[axis] = { xAxis, yAxis, clampToUnitLength };
	}

	void InputMap::Unbind(const std::string& name)
	{
		m_actions.erase(name);
		m_axes.erase(name);
		m_axes2D.erase(name);
	}

	void InputMap::Clear()
	{
		m_actions.clear();
		m_axes.clear();
		m_axes2D.clear();
		m_warnedNames.clear();
	}

	bool InputMap::HasAction(const std::string& action) const
	{
		return m_actions.count(action) > 0;
	}

	bool InputMap::HasAxis(const std::string& axis) const
	{
		return m_axes.count(axis) > 0 || m_axes2D.count(axis) > 0;
	}

	//////////////////////////////////////////
	/// Queries
	//////////////////////////////////////////
	bool InputMap::IsActionDown(const std::string& action) const
	{
		const auto* bindings = FindAction(action);
		if (!bindings) return false;

		for (const InputBinding& binding : *bindings)
			if (IsBindingDown(binding)) return true;
		return false;
	}

	bool InputMap::IsActionPressed(const std::string& action) const
	{
		const auto* bindings = FindAction(action);
		if (!bindings) return false;

		for (const InputBinding& binding : *bindings)
			if (IsBindingPressed(binding)) return true;
		return false;
	}

	bool InputMap::IsActionReleased(const std::string& action) const
	{
		const auto* bindings = FindAction(action);
		if (!bindings) return false;

		// Released only when the last held binding goes up (e.g. Space released while LMB still held: not released)
		bool anyReleased = false;
		for (const InputBinding& binding : *bindings)
		{
			if (IsBindingDown(binding)) return false;
			anyReleased = anyReleased || IsBindingReleased(binding);
		}
		return anyReleased;
	}

	float InputMap::GetAxis(const std::string& axis) const
	{
		const auto it = m_axes.find(axis);
		if (it == m_axes.end())
		{
			WarnUnbound(axis);
			return 0.0f;
		}

		float value = 0.0f;
		for (const AxisBinding& binding : it->second)
		{
			switch (binding.source)
			{
				case AxisBinding::Source::Keys:
					value += ((m_input.IsKeyDown(binding.positive) ? 1.0f : 0.0f)
							- (m_input.IsKeyDown(binding.negative) ? 1.0f : 0.0f)) * binding.scale;
					break;
				case AxisBinding::Source::MouseDeltaX:	value += m_input.GetMouseDelta().x * binding.scale;	break;
				case AxisBinding::Source::MouseDeltaY:	value += m_input.GetMouseDelta().y * binding.scale;	break;
				case AxisBinding::Source::MouseWheel:	value += m_input.GetMouseWheel() * binding.scale;	break;
			}
		}
		return value;
	}

	Vector2 InputMap::GetAxis2D(const std::string& axis) const
	{
		const auto it = m_axes2D.find(axis);
		if (it == m_axes2D.end())
		{
			WarnUnbound(axis);
			return { 0.0f, 0.0f };
		}

		Vector2 value = { GetAxis(it->second.xAxis), GetAxis(it->second.yAxis) };
		if (it->second.clampToUnitLength)
		{
			const float length = std::sqrt(value.x * value.x + value.y * value.y);
			if (length > 1.0f)
			{
				value.x /= length;
				value.y /= length;
			}
		}
		return value;
	}

	//////////////////////////////////////////
	/// Private
	//////////////////////////////////////////
	const std::vector<InputBinding>* InputMap::FindAction(const std::string& action) const
	{
		const auto it = m_actions.find(action);
		if (it == m_actions.end())
		{
			WarnUnbound(action);
			return nullptr;
		}
		return &it->second;
	}

	bool InputMap::IsBindingDown(const InputBinding& binding) const
	{
		return binding.device == InputBinding::Device::Key
			? m_input.IsKeyDown(binding.code)
			: m_input.IsMouseButtonDown(binding.code);
	}

	bool InputMap::IsBindingPressed(const InputBinding& binding) const
	{
		return binding.device == InputBinding::Device::Key
			? m_input.IsKeyPressed(binding.code)
			: m_input.IsMouseButtonPressed(binding.code);
	}

	bool InputMap::IsBindingReleased(const InputBinding& binding) const
	{
		return binding.device == InputBinding::Device::Key
			? m_input.IsKeyReleased(binding.code)
			: m_input.IsMouseButtonReleased(binding.code);
	}

	void InputMap::WarnUnbound(const std::string& name) const
	{
		if (m_warnedNames.insert(name).second)
			Log::Warning("InputMap: '%s' is not bound", name.c_str());
	}
}
