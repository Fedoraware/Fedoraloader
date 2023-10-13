#pragma once
#include <format>
#include <iostream>

namespace Log
{
	enum class LogLevel
	{
		Debug = 0,
		Info,
		Warn,
		Error,
		None
	};

	inline LogLevel Level = LogLevel::Debug;

	template<typename... Args>
	void LogPrefix(const std::string& prefix, std::format_string<Args...> fmt, Args&&... args)
	{
		std::cout << "[" << prefix << "] " << std::format(fmt, args...);
	}

	template<typename... Args>
	void Debug(std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Debug) { return; }
		LogPrefix("Debug", fmt, args...);
	}

	template<typename... Args>
	void Info(std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Info) { return; }
		LogPrefix("Info", fmt, args...);
	}

	template<typename... Args>
	void Warn(std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Warn) { return; }
		LogPrefix("Warn", fmt, args...);
	}

	template<typename... Args>
	void Error(std::format_string<Args...> fmt, Args&&... args)
	{
		if (Level > LogLevel::Error) { return; }
		LogPrefix("Error", fmt, args...);
	}
}