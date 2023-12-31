#include <Windows.h>

#include "Hooks.h"
#include "../Utils/Utils.h"

HMODULE WINAPI Hooks_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE result = LoadLibraryExW(lpLibFileName, hFile, dwFlags);

	Utils_hookImport(lpLibFileName, "kernel32.dll", "GetProcAddress", Hooks_GetProcAddress);
	Utils_hookImport(lpLibFileName, "kernel32.dll", "GetSystemInfo", Hooks_GetSystemInfo);

	return result;
}

HMODULE WINAPI Hooks_LoadLibraryExW_SteamClient(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE result = LoadLibraryExW(lpLibFileName, hFile, dwFlags);

	if (wcsstr(lpLibFileName, L"steamui.dll"))
	{
		Utils_hookImport(lpLibFileName, "kernel32.dll", "LoadLibraryExW", Hooks_LoadLibraryExW_SteamClient);
	}
	else if (wcsstr(lpLibFileName, L"steamservice.dll"))
	{
		PBYTE toPatch = Utils_findPattern(L"steamservice", "\x74\x47\x6A\x01\x6A", 0);
		if (toPatch)
		{
			DWORD old;
			VirtualProtect(toPatch, 1, PAGE_EXECUTE_READWRITE, &old);
			*toPatch = 0xEB;
			VirtualProtect(toPatch, 1, old, &old);
			Utils_hookImport(L"steamservice", "kernel32.dll", "LoadLibraryExW", Hooks_LoadLibraryExW);
#ifdef _DEBUG
			MessageBoxW(NULL, L"Initialization was successful! (VAC Bypass Loader)", L"VAC bypass", MB_OK | MB_ICONINFORMATION);
#endif
		}
	}
	return result;
}

FARPROC WINAPI Hooks_GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	FARPROC result = GetProcAddress(hModule, lpProcName);

	if (result)
	{
		if (!strcmp(lpProcName, "GetProcAddress"))
		{
			return Hooks_GetProcAddress;
		}
		if (!strcmp(lpProcName, "GetSystemInfo"))
		{
			return Hooks_GetSystemInfo;
		}
		if (!strcmp(lpProcName, "GetVersionExA"))
		{
			return Hooks_GetVersionExA;
		}
		if (!strcmp(lpProcName, "GetSystemDirectoryW"))
		{
			return Hooks_GetSystemDirectoryW;
		}
		if (!strcmp(lpProcName, "GetWindowsDirectoryW"))
		{
			return Hooks_GetWindowsDirectoryW;
		}
		if (!strcmp(lpProcName, "GetCurrentProcessId"))
		{
			return Hooks_GetCurrentProcessId;
		}
		if (!strcmp(lpProcName, "GetCurrentThreadId"))
		{
			return Hooks_GetCurrentThreadId;
		}
	}
	return result;
}

VOID WINAPI Hooks_GetSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
	GetSystemInfo(lpSystemInfo);
	lpSystemInfo->dwPageSize = 1337;
}

BOOL WINAPI Hooks_GetVersionExA(LPOSVERSIONINFOEXA lpVersionInformation)
{
	MessageBoxW(NULL, L"Bypass malfunction detected! Steam will close...", L"VAC bypass", MB_OK | MB_ICONERROR);
	ExitProcess(1);
	return FALSE;
}

UINT WINAPI Hooks_GetSystemDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
	MessageBoxW(NULL, L"Bypass malfunction detected! Steam will close...", L"VAC bypass", MB_OK | MB_ICONERROR);
	ExitProcess(1);
	return 0;
}

UINT WINAPI Hooks_GetWindowsDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
	MessageBoxW(NULL, L"Bypass malfunction detected! Steam will close...", L"VAC bypass", MB_OK | MB_ICONERROR);
	ExitProcess(1);
	return 0;
}

DWORD WINAPI Hooks_GetCurrentProcessId(VOID)
{
	return 0;
}

DWORD WINAPI Hooks_GetCurrentThreadId(VOID)
{
	return 0;
}
