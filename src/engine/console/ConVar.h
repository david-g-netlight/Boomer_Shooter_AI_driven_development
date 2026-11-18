#pragma once
#if ENG_DEV_CONSOLE

#include <functional>
#include <string>

namespace eng
{
	/// <summary>
	/// A named value bound to a variable owned by someone else (the variable must outlive the registration).
	/// Typing `name` prints it, `name value` sets it. Numbers are clamped to [min, max].
	/// </summary>
	class ConVar
	{
	public:
		using OnChanged = std::function<void()>;

		static ConVar Bool	(const char* name, bool* target, const char* help);
		static ConVar Int	(const char* name, int* target, int min, int max, const char* help);
		static ConVar Float	(const char* name, float* target, float min, float max, const char* help);
		static ConVar String(const char* name, std::string* target, const char* help);

		ConVar&				SetOnChanged(OnChanged callback) { m_onChanged = std::move(callback); return *this; }

		const std::string&	GetName() const { return m_name; }
		const std::string&	GetHelp() const { return m_help; }
		/// <summary>"name <0|1>", "name <int>", ...</summary>
		std::string			GetUsage() const;

		std::string			GetValueText() const;
		/// <summary>Parses and stores the value. On failure returns false and explains why in `error`.</summary>
		bool				SetFromText(const std::string& text, std::string& error);

	private:
		enum class Type { Bool, Int, Float, String };

		Type			m_type		= Type::Bool;
		std::string		m_name;
		std::string		m_help;
		void*			m_target	= nullptr;
		float			m_min		= 0.0f;
		float			m_max		= 0.0f;
		OnChanged		m_onChanged;
	};
}

#endif
