#pragma once
#include <Windows.h>

namespace Hooks
{
	HMODULE WINAPI Hk_LoadLibraryExW(LPCWSTR, HANDLE, DWORD);
	HMODULE WINAPI Hk_LoadLibraryExW_SteamClient(LPCWSTR, HANDLE, DWORD);
	FARPROC WINAPI Hk_GetProcAddress(HMODULE, LPCSTR);
	VOID WINAPI Hk_GetSystemInfo(LPSYSTEM_INFO);
	BOOL WINAPI Hk_GetVersionExA(LPOSVERSIONINFOEXA);
	UINT WINAPI Hk_GetSystemDirectoryW(LPWSTR, UINT);
	UINT WINAPI Hk_GetWindowsDirectoryW(LPWSTR, UINT);
	DWORD WINAPI Hk_GetCurrentProcessId(VOID);
	DWORD WINAPI Hk_GetCurrentThreadId(VOID);
}
