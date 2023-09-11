#pragma once
#include "Windows.h"

struct BinData
{
	BYTE* Data = nullptr;
	SIZE_T Size = 0u;
};

namespace Utils
{
	DWORD FindProcess(const char* procName);
}
