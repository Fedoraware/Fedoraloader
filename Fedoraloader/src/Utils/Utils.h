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
	DWORD WaitForProcess(const char* procName, DWORD sTimeout = 10);
	HANDLE WaitForProcessHandle(const char* procName, DWORD sTimeout = 10);
	bool WaitCloseProcess(const char* procName, DWORD sTimeout = 10);
	void WaitForModule(DWORD processId, LPCSTR moduleName);

	Binary ReadBinaryFile(LPCWSTR fileName);
	Binary GetBinaryResource(WORD id);

	LPCWSTR CopyString(LPCWSTR src);
	void GetVersionNumbers(LPDWORD major, LPDWORD minor, LPDWORD build);
}
