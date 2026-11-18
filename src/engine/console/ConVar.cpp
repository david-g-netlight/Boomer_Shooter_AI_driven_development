#if ENG_DEV_CONSOLE

#include "ConVar.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace eng
{
	namespace
	{
		std::string FormatFloat(float value)
		{
			char buffer[32];
			std::snprintf(buffer, sizeof(buffer), "%g", value);
			return buffer;
		}
	}

	ConVar ConVar::Bool(const char* name, bool* target, const char* help)
	{
		ConVar v;
		v.m_type = Type::Bool;
		v.m_name = name;
		v.m_help = help;
		v.m_target = target;
		return v;
	}

	ConVar ConVar::Int(const char* name, int* target, int min, int max, const char* help)
	{
		ConVar v;
		v.m_type = Type::Int;
		v.m_name = name;
		v.m_help = help;
		v.m_target = target;
		v.m_min = (float)min;
		v.m_max = (float)max;
		return v;
	}

	ConVar ConVar::Float(const char* name, float* target, float min, float max, const char* help)
	{
		ConVar v;
		v.m_type = Type::Float;
		v.m_name = name;
		v.m_help = help;
		v.m_target = target;
		v.m_min = min;
		v.m_max = max;
		return v;
	}

	ConVar ConVar::String(const char* name, std::string* target, const char* help)
	{
		ConVar v;
		v.m_type = Type::String;
		v.m_name = name;
		v.m_help = help;
		v.m_target = target;
		return v;
	}

	std::string ConVar::GetUsage() const
	{
		switch (m_type)
		{
			case Type::Bool:	return m_name + " [0|1]";
			case Type::Int:		return m_name + " [" + FormatFloat(m_min) + ".." + FormatFloat(m_max) + "]";
			case Type::Float:	return m_name + " [" + FormatFloat(m_min) + ".." + FormatFloat(m_max) + "]";
			case Type::String:	return m_name + " [text]";
		}
		return m_name;
	}

	std::string ConVar::GetValueText() const
	{
		switch (m_type)
		{
			case Type::Bool:	return *static_cast<bool*>(m_target) ? "1" : "0";
			case Type::Int:		return std::to_string(*static_cast<int*>(m_target));
			case Type::Float:	return FormatFloat(*static_cast<float*>(m_target));
			case Type::String:	return *static_cast<std::string*>(m_target);
		}
		return "";
	}

	bool ConVar::SetFromText(const std::string& text, std::string& error)
	{
		switch (m_type)
		{
			case Type::Bool:
			{
				if (text == "1" || text == "true" || text == "on")
					*static_cast<bool*>(m_target) = true;
				else if (text == "0" || text == "false" || text == "off")
					*static_cast<bool*>(m_target) = false;
				else
				{
					error = "expected 0 or 1";
					return false;
				}
				break;
			}
			case Type::Int:
			{
				char* end = nullptr;
				long value = std::strtol(text.c_str(), &end, 10);
				if (text.empty() || *end != '\0')
				{
					error = "expected an integer";
					return false;
				}
				*static_cast<int*>(m_target) = (int)std::clamp((float)value, m_min, m_max);
				break;
			}
			case Type::Float:
			{
				char* end = nullptr;
				float value = std::strtof(text.c_str(), &end);
				if (text.empty() || *end != '\0')
				{
					error = "expected a number";
					return false;
				}
				*static_cast<float*>(m_target) = std::clamp(value, m_min, m_max);
				break;
			}
			case Type::String:
				*static_cast<std::string*>(m_target) = text;
				break;
		}

		if (m_onChanged)
			m_onChanged();
		return true;
	}
}

#endif
