#include "Utils.h"
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
