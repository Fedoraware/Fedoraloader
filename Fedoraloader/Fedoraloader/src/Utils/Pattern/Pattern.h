#pragma once
#include <Windows.h>
#include <string>

namespace Pattern
{
	PBYTE Find(LPCSTR szModuleName, const std::string& szPattern);
}