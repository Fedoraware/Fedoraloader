#pragma once
#include <Windows.h>
#include "Log.h"

#include <string>
#include <vector>

using Binary = std::vector<BYTE>;

namespace Utils
{
	// Process utils
	DWORD FindProcess(LPCSTR procName);
	HANDLE GetProcessHandle(LPCSTR procName);

	// Wait for process
	DWORD WaitForProcess(LPCSTR procName, DWORD sTimeout = 10);
	HANDLE WaitForProcessHandle(LPCSTR procName, DWORD sTimeout = 10);
	bool WaitCloseProcess(LPCSTR procName, DWORD sTimeout = 10);
	bool WaitForModule(DWORD processId, LPCSTR moduleName, DWORD sTimeout = 10);

	// Binary utils
	Binary ReadBinaryFile(const std::wstring& fileName);
	Binary GetBinaryResource(WORD id);

	// STL & WinApi utils
	bool IsElevated();
	void GetVersionNumbers(LPDWORD major, LPDWORD minor, LPDWORD build);

	void ShowConsole();
	void HideConsole();
}
