#include "Log.h"
#include <cstdarg>
#include <cstdio>
#include <raylib.h>

namespace eng
{
	namespace
	{
		constexpr int LOG_BUFFER_SIZE = 1024;

		Log::Sink g_sink = nullptr;

		int ToRaylibLevel(LogLevel level)
		{
			switch (level)
			{
				case LogLevel::Debug:	return LOG_DEBUG;
				case LogLevel::Info:	return LOG_INFO;
				case LogLevel::Warning:	return LOG_WARNING;
				case LogLevel::Error:	return LOG_ERROR;
				case LogLevel::None:	return LOG_NONE;
			}
			return LOG_INFO;
		}

		void Write(LogLevel level, const char* format, va_list args)
		{
			char buffer[LOG_BUFFER_SIZE];
			std::vsnprintf(buffer, sizeof(buffer), format, args);
			TraceLog(ToRaylibLevel(level), "ENG: %s", buffer);
			if (g_sink)
				g_sink(level, buffer);
		}
	}

	void Log::SetLevel(LogLevel level)
	{
		SetTraceLogLevel(ToRaylibLevel(level));
	}

	void Log::SetSink(Sink sink)
	{
		g_sink = sink;
	}

	void Log::Debug(const char* format, ...)
	{
		va_list args; va_start(args, format); Write(LogLevel::Debug, format, args); va_end(args);
	}

	void Log::Info(const char* format, ...)
	{
		va_list args; va_start(args, format); Write(LogLevel::Info, format, args); va_end(args);
	}

	void Log::Warning(const char* format, ...)
	{
		va_list args; va_start(args, format); Write(LogLevel::Warning, format, args); va_end(args);
	}

	void Log::Error(const char* format, ...)
	{
		va_list args; va_start(args, format); Write(LogLevel::Error, format, args); va_end(args);
	}
}
