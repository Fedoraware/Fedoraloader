#include "Utils.h"
#include "../resource.h"

#include <fstream>
#include <TlHelp32.h>

DWORD Utils::FindProcess(const char* procName)
{
	PROCESSENTRY32 procEntry{};
	procEntry.dwSize = sizeof(procEntry);

	const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) { return 0; }
	if (!Process32First(hSnapshot, &procEntry)) { return 0; }

	do
	{
		if (strcmp(procEntry.szExeFile, procName) == 0)
		{
			CloseHandle(hSnapshot);
			return procEntry.th32ProcessID;
		}
	}
	while (Process32Next(hSnapshot, &procEntry));

	CloseHandle(hSnapshot);
	return 0;
}

HANDLE Utils::GetProcessHandle(const char* procName)
{
	const DWORD pid = FindProcess(procName);
	if (pid == 0) { return nullptr; }

	const HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	return hProc;
}

DWORD Utils::WaitForProcess(const char* procName, DWORD sTimeout)
{
	const auto startTime = GetTickCount64();
	DWORD processId = 0;

	while (processId == 0)
	{
		// Try to get the handle
		processId = FindProcess(procName);
		if (processId > 0) { return processId; }

		// Check timeout
		const auto currentTime = GetTickCount64();
        const auto elapsedTime = currentTime - startTime;
		if (elapsedTime >= static_cast<ULONGLONG>(sTimeout) * 1000) { break; }

		Sleep (100);
	}

	return 0;
}

HANDLE Utils::WaitForProcessHandle(const char* procName, DWORD sTimeout)
{
	const auto startTime = GetTickCount64();
	HANDLE hProc = nullptr;

	while (!hProc)
	{
		// Try to get the handle
		hProc = GetProcessHandle(procName);
		if (hProc) { return hProc; }

		// Check timeout
		const auto currentTime = GetTickCount64();
        const auto elapsedTime = currentTime - startTime;
		if (elapsedTime >= static_cast<ULONGLONG>(sTimeout) * 1000) { break; }

		Sleep (100);
	}

	return nullptr;
}

bool Utils::WaitCloseProcess(const char* procName, DWORD sTimeout)
{
	const HANDLE hProc = GetProcessHandle(procName);
	if (hProc == nullptr || hProc == INVALID_HANDLE_VALUE) { return false; }

	// Terminate and wait for exit
	if (!TerminateProcess(hProc, 0)) { return false; }
	const DWORD result = WaitForSingleObject(hProc, sTimeout * 1000);

	CloseHandle(hProc);
	return result == WAIT_OBJECT_0;
}

Binary Utils::ReadBinaryFile(LPCWSTR fileName)
{
	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	if (inFile.fail())
	{
		inFile.close();
		throw std::runtime_error("Failed to open file");
	}

	// Allocate a buffer
	const auto fileSize = inFile.tellg();
	const auto data = static_cast<BYTE*>(std::malloc(static_cast<size_t>(fileSize) * sizeof(BYTE)));
	if (!data)
	{
		inFile.close();
		throw std::runtime_error("Failed to allocate file buffer");
	}

	// Read the file
	inFile.seekg(0, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(data), fileSize);
	inFile.close();

	return {
		.Data = data,
		.Size = static_cast<SIZE_T>(fileSize)
	};
}

Binary Utils::GetBinaryResource(WORD id)
{
	const HRSRC res = ::FindResource(nullptr, MAKEINTRESOURCE(id), RT_RCDATA);
	if (!res)
	{
		throw std::runtime_error("Failed to find resource");
	}

	const SIZE_T resSize = SizeofResource(nullptr, res);
	const HGLOBAL resData = LoadResource(nullptr, res);
	if (!resData)
	{
		throw std::runtime_error("Failed to load resource data");
	}

	BYTE* binData = static_cast<BYTE*>(LockResource(resData));
	return {
		.Data = binData,
		.Size = resSize
	};
}
