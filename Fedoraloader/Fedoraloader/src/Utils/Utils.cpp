#include "Utils.h"

#include <fstream>
#include <TlHelp32.h>

DWORD Utils::FindProcess(LPCSTR procName)
{
	DWORD processId = 0;

	PROCESSENTRY32 procEntry{};
	procEntry.dwSize = sizeof(procEntry);

	// Get snapshot of processes
	const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) { return 0; }
	if (!Process32First(hSnapshot, &procEntry)) { return 0; }

	// Find the desired process
	do
	{
		if (_strcmpi(procEntry.szExeFile, procName) == 0)
		{
			processId = procEntry.th32ProcessID;
			break;
		}
	}
	while (Process32Next(hSnapshot, &procEntry));

	CloseHandle(hSnapshot);
	return processId;
}

HANDLE Utils::GetProcessHandle(LPCSTR procName)
{
	const DWORD pid = FindProcess(procName);
	if (pid == 0) { return nullptr; }

	const HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	return hProc;
}

DWORD Utils::WaitForProcess(LPCSTR procName, DWORD sTimeout)
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

HANDLE Utils::WaitForProcessHandle(LPCSTR procName, DWORD sTimeout)
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

bool Utils::WaitCloseProcess(LPCSTR procName, DWORD sTimeout)
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
				if (!_strcmpi(moduleEntry.szModule, moduleName))
				{
					moduleFound = true;
					break;
				}
			}
			while (Module32Next(moduleSnapshot, &moduleEntry));
		}

		CloseHandle(moduleSnapshot);

		// Check timeout
		const auto currentTime = GetTickCount64();
		const auto elapsedTime = currentTime - startTime;
		if (elapsedTime >= static_cast<ULONGLONG>(sTimeout) * 1000) { break; }
	}

	return moduleFound;
}

Binary Utils::ReadBinaryFile(const std::wstring& fileName)
{
	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	if (inFile.fail())
	{
		inFile.close();
		throw std::runtime_error("Failed to open file");
	}

	// Allocate a buffer
	const auto fileSize = static_cast<size_t>(inFile.tellg());
	Binary fileData(fileSize);

	// Read the file
	inFile.seekg(0, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(fileData.data()), fileSize);
	inFile.close();

	return fileData;
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
	return {binData, binData + resSize};
}

bool Utils::IsElevated()
{
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		return false;
	}

	TOKEN_ELEVATION elevation;
	DWORD dwSize;
	if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
	{
		CloseHandle(hToken);
		return false;
	}

	CloseHandle(hToken);
	return elevation.TokenIsElevated != 0;
}

void Utils::GetVersionNumbers(LPDWORD major, LPDWORD minor, LPDWORD build)
{
	using TRtlGetNtVersionNumbers = void (WINAPI)(LPDWORD, LPDWORD, LPDWORD);
	static TRtlGetNtVersionNumbers* fn = nullptr;
	if (fn == nullptr)
	{
		const auto hMod = GetModuleHandleA("ntdll.dll");
		if (hMod == nullptr) { return; }

		fn = reinterpret_cast<TRtlGetNtVersionNumbers*>(GetProcAddress(hMod, "RtlGetNtVersionNumbers"));
		if (fn == nullptr) { return; }
	}

	fn(major, minor, build);
	*build &= ~0xF0000000;
}

void Utils::ShowConsole()
{
	AllocConsole();
	FILE* fDummy = nullptr;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
}

void Utils::HideConsole()
{
	std::fclose(stdin);
	std::fclose(stderr);
	std::fclose(stdout);
	FreeConsole();
}
