#pragma once
#include <string>
#include "Windows.h"

struct Binary
{
	BYTE* Data = nullptr;
	SIZE_T Size = 0u;
};

namespace Utils
{
	// Process utils
	DWORD FindProcess(const char* procName);
	HANDLE GetProcessHandle(const char* procName);

	// Wait for process
	DWORD WaitForProcess(const char* procName, DWORD sTimeout = 10);
	HANDLE WaitForProcessHandle(const char* procName, DWORD sTimeout = 10);
	bool WaitCloseProcess(const char* procName, DWORD sTimeout = 10);
	bool WaitForModule(DWORD processId, LPCSTR moduleName, DWORD sTimeout = 10);

	// Binary utils
	Binary ReadBinaryFile(LPCWSTR fileName);
	Binary GetBinaryResource(WORD id);

	// STL & WinApi utils
	bool IsElevated();
	LPCWSTR CopyString(LPCWSTR src);
	void GetVersionNumbers(LPDWORD major, LPDWORD minor, LPDWORD build);
}
