#if ENG_DEV_CONSOLE

#include "ConsoleArgs.h"
#include <cstdlib>

namespace eng
{
	namespace
	{
		bool ParseFloat(const std::string& text, float& out)
		{
			if (text.empty())
				return false;
			char* end = nullptr;
			float value = std::strtof(text.c_str(), &end);
			if (*end != '\0')
				return false;
			out = value;
			return true;
		}
	}

	std::vector<std::string> ConsoleArgs::Tokenize(const std::string& line)
	{
		std::vector<std::string> tokens;
		std::string current;
		bool inQuotes = false;
		bool hasToken = false;

		for (char c : line)
		{
			if (c == '"')
			{
				inQuotes = !inQuotes;
				hasToken = true;
			}
			else if ((c == ' ' || c == '\t') && !inQuotes)
			{
				if (hasToken)
					tokens.push_back(current);
				current.clear();
				hasToken = false;
			}
			else
			{
				current += c;
				hasToken = true;
			}
		}
		if (hasToken)
			tokens.push_back(current);
		return tokens;
	}

	ConsoleArgs::ConsoleArgs(const std::string& line)
	{
		std::vector<std::string> tokens = Tokenize(line);
		if (tokens.empty())
			return;
		m_command = tokens[0];
		m_args.assign(tokens.begin() + 1, tokens.end());
	}

	const std::string& ConsoleArgs::GetString(int index) const
	{
		static const std::string empty;
		return (index >= 0 && index < Count()) ? m_args[index] : empty;
	}

	bool ConsoleArgs::GetInt(int index, int& out) const
	{
		if (index < 0 || index >= Count() || m_args[index].empty())
			return false;
		char* end = nullptr;
		long value = std::strtol(m_args[index].c_str(), &end, 10);
		if (*end != '\0')
			return false;
		out = (int)value;
		return true;
	}

	bool ConsoleArgs::GetFloat(int index, float& out) const
	{
		if (index < 0 || index >= Count())
			return false;
		return ParseFloat(m_args[index], out);
	}

	bool ConsoleArgs::GetVector3(int index, Vector3& out) const
	{
		if (index < 0 || index >= Count())
			return false;

		Vector3 v;
		if (m_args[index].find(',') != std::string::npos)
		{
			// "1,2,3": split the single token at the commas
			std::string parts[3];
			int part = 0;
			for (char c : m_args[index])
			{
				if (c == ',')
				{
					if (++part > 2)
						return false;
				}
				else
					parts[part] += c;
			}
			if (part != 2 || !ParseFloat(parts[0], v.x) || !ParseFloat(parts[1], v.y) || !ParseFloat(parts[2], v.z))
				return false;
		}
		else if (!GetFloat(index, v.x) || !GetFloat(index + 1, v.y) || !GetFloat(index + 2, v.z))
			return false;

		out = v;
		return true;
	}

	std::string ConsoleArgs::Rest(int index) const
	{
		std::string result;
		for (int i = index < 0 ? 0 : index; i < Count(); ++i)
		{
			if (!result.empty())
				result += ' ';
			result += m_args[i];
		}
		return result;
	}
}

#endif
