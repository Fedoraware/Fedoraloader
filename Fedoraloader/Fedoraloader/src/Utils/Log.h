#pragma once
#include <format>
#include <iostream>

enum class LogLevel
{
	Debug = 0,
	Info,
	Warn,
	Error,
	None
};

class Log
{
	inline static LogLevel Level = LogLevel::Debug;

	template<typename... Args>
	static void LogPrefix(const std::string& prefix, std::format_string<Args...> fmt, Args&&... args)
	{
		std::cout << "[" << prefix << "] " << std::format(fmt, std::forward<Args>(args)...) << std::endl;
	}

public:
	static void SetLevel(LogLevel level)
	{
		Level = level;
	}

	template<typename... Args>
	static void Debug(const std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Debug) { return; }
		LogPrefix("Debug", fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Info(const std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Info) { return; }
		LogPrefix("Info", fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Warn(const std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Warn) { return; }
		LogPrefix("Warn", fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void Error(const std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Error) { return; }
		LogPrefix("Error", fmt, std::forward<Args>(args)...);
	}
};
