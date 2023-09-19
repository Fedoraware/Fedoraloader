#include "Hooks.h"
#include "../Utils/Utils.h"

HMODULE WINAPI Hooks::Hk_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	const HMODULE result = LoadLibraryExW(lpLibFileName, hFile, dwFlags);

	Utils::HookImport(lpLibFileName, "kernel32.dll", "GetProcAddress", Hk_GetProcAddress);
	Utils::HookImport(lpLibFileName, "kernel32.dll", "GetSystemInfo", Hk_GetSystemInfo);

	return result;
}

HMODULE WINAPI Hooks::Hk_LoadLibraryExW_SteamClient(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	const HMODULE result = LoadLibraryExW(lpLibFileName, hFile, dwFlags);

	if (wcsstr(lpLibFileName, L"steamui.dll"))
	{
		Utils::HookImport(lpLibFileName, "kernel32.dll", "LoadLibraryExW", Hk_LoadLibraryExW_SteamClient);
	}
	else if (wcsstr(lpLibFileName, L"steamservice.dll"))
	{
		const auto toPatch = reinterpret_cast<PBYTE>(Pattern::Find("steamservice", "74 47 6A 01 6A"));
		if (toPatch)
		{
			DWORD old;
			VirtualProtect(toPatch, 1, PAGE_EXECUTE_READWRITE, &old);
			*toPatch = 0xEB;
			VirtualProtect(toPatch, 1, old, &old);
			if (!Utils::HookImport(L"steamservice", "kernel32.dll", "LoadLibraryExW", Hk_LoadLibraryExW))
			{
				MessageBoxA(nullptr, "Failed to hook LoadLibraryExW", "Fedoraloader", MB_OK | MB_ICONERROR);
			}
		}
	}
	return result;
}

FARPROC WINAPI Hooks::Hk_GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	const FARPROC result = GetProcAddress(hModule, lpProcName);

	if (result)
	{
		if (!strcmp(lpProcName, "GetProcAddress"))
		{
			return reinterpret_cast<FARPROC>(Hk_GetProcAddress);
		}
		if (!strcmp(lpProcName, "GetSystemInfo"))
		{
			return reinterpret_cast<FARPROC>(Hk_GetSystemInfo);
		}
		if (!strcmp(lpProcName, "GetVersionExA"))
		{
			return reinterpret_cast<FARPROC>(Hk_GetVersionExA);
		}
		if (!strcmp(lpProcName, "GetSystemDirectoryW"))
		{
			return reinterpret_cast<FARPROC>(Hk_GetSystemDirectoryW);
		}
		if (!strcmp(lpProcName, "GetWindowsDirectoryW"))
		{
			return reinterpret_cast<FARPROC>(Hk_GetWindowsDirectoryW);
		}
		if (!strcmp(lpProcName, "GetCurrentProcessId"))
		{
			return reinterpret_cast<FARPROC>(Hk_GetCurrentProcessId);
		}
		if (!strcmp(lpProcName, "GetCurrentThreadId"))
		{
			return reinterpret_cast<FARPROC>(Hk_GetCurrentThreadId);
		}
	}
	return result;
}

VOID WINAPI Hooks::Hk_GetSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
	GetSystemInfo(lpSystemInfo);
	lpSystemInfo->dwPageSize = 1270;
}

BOOL WINAPI Hooks::Hk_GetVersionExA(LPOSVERSIONINFOEXA lpVersionInformation)
{
	MessageBoxW(nullptr, L"VAC-Bypass malfunction detected! Steam will close...", L"Fedoraloader", MB_OK | MB_ICONERROR);
	ExitProcess(1);
}

UINT WINAPI Hooks::Hk_GetSystemDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
	MessageBoxW(nullptr, L"VAC-Bypass malfunction detected! Steam will close...", L"Fedoraloader", MB_OK | MB_ICONERROR);
	ExitProcess(1);
}

UINT WINAPI Hooks::Hk_GetWindowsDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
	MessageBoxW(nullptr, L"VAC-Bypass malfunction detected! Steam will close...", L"Fedoraloader", MB_OK | MB_ICONERROR);
	ExitProcess(1);
}

DWORD WINAPI Hooks::Hk_GetCurrentProcessId(VOID)
{
	return 0;
}

DWORD WINAPI Hooks::Hk_GetCurrentThreadId(VOID)
{
	return 0;
}
