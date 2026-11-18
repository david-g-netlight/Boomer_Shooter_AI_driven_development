#pragma once
#if ENG_DEV_CONSOLE

#include <string>
#include <vector>
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// A console line split into tokens. Quotes keep spaces: select "Crate A".
	/// Token 0 is the command name; the arguments start at index 0 of the accessors below.
	/// Getters return false (and leave `out` alone) when the argument is missing or malformed.
	/// </summary>
	class ConsoleArgs
	{
	public:
		explicit ConsoleArgs(const std::string& line);

		const std::string&	GetCommand() const { return m_command; }
		int					Count() const { return (int)m_args.size(); }

		const std::string&	GetString(int index) const;		// empty string when out of range
		bool				GetInt(int index, int& out) const;
		bool				GetFloat(int index, float& out) const;
		/// <summary>"1 2 3" (three tokens) or "1,2,3" (one token)</summary>
		bool				GetVector3(int index, Vector3& out) const;

		/// <summary>The arguments from `index` on, joined with single spaces</summary>
		std::string			Rest(int index) const;

		static std::vector<std::string> Tokenize(const std::string& line);

	private:
		std::string					m_command;
		std::vector<std::string>	m_args;
	};
}

#endif
