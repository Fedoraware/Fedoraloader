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
		std::cout << "[" << prefix << "] " << std::format(fmt, args...) << std::endl;
	}

public:
	static void SetLevel(LogLevel level)
	{
		Level = level;
	}

	template<typename... Args>
	static void Debug(std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Debug) { return; }
		LogPrefix("Debug", fmt, args...);
	}

	template<typename... Args>
	static void Info(std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Info) { return; }
		LogPrefix("Info", fmt, args...);
	}

	template<typename... Args>
	static void Warn(std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Warn) { return; }
		LogPrefix("Warn", fmt, args...);
	}

	template<typename... Args>
	static void Error(std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Error) { return; }
		LogPrefix("Error", fmt, args...);
	}
};
