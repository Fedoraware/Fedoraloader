#pragma once
#include "Windows.h"

struct Binary
{
	BYTE* Data = nullptr;
	SIZE_T Size = 0u;
};

namespace Utils
{
	DWORD FindProcess(const char* procName);
	HANDLE GetProcessHandle(const char* procName);
	HANDLE WaitForProcess(const char* procName, DWORD sTimeout = 10);
	bool WaitCloseProcess(const char* procName, DWORD sTimeout = 10);

	Binary ReadBinaryFile(LPCWSTR fileName);
	Binary GetBinaryResource(WORD id);
}
