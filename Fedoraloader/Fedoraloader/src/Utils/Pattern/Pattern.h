#pragma once
#include <Windows.h>
#include <vector>

namespace Pattern
{
	PBYTE Find(LPCSTR szModuleName, const std::vector<BYTE>& szPattern);
}