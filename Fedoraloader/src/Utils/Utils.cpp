#include "Utils.h"

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

HANDLE Utils::WaitForProcess(const char* procName, DWORD sTimeout)
{
	const DWORD startTime = GetTickCount();
	HANDLE hProc = nullptr;

	while (!hProc)
	{
		// Try to get the handle
		hProc = GetProcessHandle(procName);
		if (hProc) { return hProc; }

		// Check timeout
		const DWORD currentTime = GetTickCount();
        const DWORD elapsedTime = currentTime - startTime;
		if (elapsedTime >= sTimeout * 1000) { break; }

		Sleep (10);
	}

	return nullptr;
}

Binary Utils::ReadBinaryFile(LPCWSTR fileName)
{
	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	if (inFile.fail())
	{
		inFile.close();
		return {};
	}

	// Allocate a buffer
	const auto fileSize = inFile.tellg();
	const auto data = new BYTE[static_cast<size_t>(fileSize)];
	if (!data)
	{
		inFile.close();
		return {};
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
