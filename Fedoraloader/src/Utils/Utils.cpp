#include "Utils.h"
#include "../resource.h"

#include <fstream>
#include <TlHelp32.h>

DWORD Utils::FindProcess(const char* procName)
{
	DWORD processId = 0;

	PROCESSENTRY32 procEntry{};
	procEntry.dwSize = sizeof(procEntry);

	const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) { return 0; }
	if (!Process32First(hSnapshot, &procEntry)) { return 0; }

	do
	{
		if (strcmp(procEntry.szExeFile, procName) == 0)
		{
			processId = procEntry.th32ProcessID;
			break;
		}
	}
	while (Process32Next(hSnapshot, &procEntry));

	CloseHandle(hSnapshot);
	return processId;
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

		Sleep(100);
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

		Sleep(100);
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

bool Utils::WaitForModule(DWORD processId, LPCSTR moduleName, DWORD sTimeout)
{
	const auto startTime = GetTickCount64();
	bool moduleFound = false;

	while (!moduleFound)
	{
		const HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
		if (moduleSnapshot == INVALID_HANDLE_VALUE) { continue; }

		MODULEENTRY32 moduleEntry;
        moduleEntry.dwSize = sizeof(moduleEntry);

		// Find the target module
		if (Module32First(moduleSnapshot, &moduleEntry))
		{
            do
			{
                if (!strcmp(moduleEntry.szModule, moduleName))
				{
                    moduleFound = true;
                    break;
                }
            } while (Module32Next(moduleSnapshot, &moduleEntry));
        }

        CloseHandle(moduleSnapshot);

		// Check timeout
		const auto currentTime = GetTickCount64();
		const auto elapsedTime = currentTime - startTime;
		if (elapsedTime >= static_cast<ULONGLONG>(sTimeout) * 1000) { break; }
	}

	return moduleFound;
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

	const auto binData = static_cast<BYTE*>(LockResource(resData));
	return {
		.Data = binData,
		.Size = resSize
	};
}

LPCWSTR Utils::CopyString(LPCWSTR src)
{
	const SIZE_T size = std::wcslen(src);
	const auto dest = new WCHAR[size + 1];
	if (!dest) { return nullptr; }

	const auto err = wcsncpy_s(dest, size + 1, src, size);
	if (err != 0) { return nullptr; }

	return dest;
}

void Utils::GetVersionNumbers(LPDWORD major, LPDWORD minor, LPDWORD build)
{
	using TRtlGetNtVersionNumbers = void (WINAPI)(LPDWORD, LPDWORD, LPDWORD);
	static TRtlGetNtVersionNumbers* fn = nullptr;
	if (fn == nullptr)
	{
		const auto hMod = GetModuleHandle("ntdll.dll");
		if (hMod == nullptr) { return; }

		fn = reinterpret_cast<TRtlGetNtVersionNumbers*>(GetProcAddress(hMod, "RtlGetNtVersionNumbers"));
		if (fn == nullptr) { return; }
	}

	fn(major, minor, build);
	*build &= ~0xF0000000;
}
