#pragma once
#include <Windows.h>
#include "Log.h"

#include <string>
#include <vector>

using Binary = std::vector<BYTE>;

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
	Binary ReadBinaryFile(const std::wstring& fileName);
	Binary GetBinaryResource(WORD id);

	// STL & WinApi utils
	bool IsElevated();
	LPCWSTR CopyString(LPCWSTR src);
	void GetVersionNumbers(LPDWORD major, LPDWORD minor, LPDWORD build);

	void ShowConsole();
	void HideConsole();
}
