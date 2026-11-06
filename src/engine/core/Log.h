#pragma once

#if defined(__clang__) || defined(__GNUC__)
	#define ENG_PRINTF_FORMAT(fmtIndex, argsIndex) __attribute__((format(printf, fmtIndex, argsIndex)))
#else
	#define ENG_PRINTF_FORMAT(fmtIndex, argsIndex)
#endif

namespace eng
{
	enum class LogLevel
	{
		Debug,
		Info,
		Warning,
		Error,
		None,
	};

	/// <summary>
	/// printf-style logging on top of raylib's TraceLog, so engine and raylib messages share one stream.
	/// Example: Log::Info("Loaded level %s", path);
	/// </summary>
	class Log
	{
	public:
		static void SetLevel(LogLevel level);

		static void Debug	(const char* format, ...) ENG_PRINTF_FORMAT(1, 2);
		static void Info	(const char* format, ...) ENG_PRINTF_FORMAT(1, 2);
		static void Warning	(const char* format, ...) ENG_PRINTF_FORMAT(1, 2);
		static void Error	(const char* format, ...) ENG_PRINTF_FORMAT(1, 2);
	};
}
