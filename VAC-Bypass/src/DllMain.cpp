#include <Windows.h>
#include "Utils/Utils.h"
#include "Hooks/Hooks.h"

void Init()
{
	if (GetModuleHandle(TEXT("steamservice")))
	{
		const auto toPatch = reinterpret_cast<PBYTE>(Pattern::Find("steamservice", "74 47 6A 01 6A"));
		if (!toPatch)
		{
			MessageBoxA(nullptr, "Failed to find signature", "Fedoraloader", MB_OK | MB_ICONERROR);
			return;
		}

		DWORD flOldProtect;
		VirtualProtect(toPatch, 1, PAGE_EXECUTE_READWRITE, &flOldProtect);
		*toPatch = 0xEB;
		VirtualProtect(toPatch, 1, flOldProtect, &flOldProtect);

		if (!Utils::HookImport(L"steamservice", "kernel32.dll", "LoadLibraryExW", Hooks::Hk_LoadLibraryExW))
		{
			MessageBoxA(nullptr, "Failed to hook LoadLibraryExW", "Fedoraloader", MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		Utils::HookImport(nullptr, "kernel32.dll", "LoadLibraryExW", Hooks::Hk_LoadLibraryExW_SteamClient);
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		Init();
		DisableThreadLibraryCalls(hinstDLL);
	}

	return TRUE;
}
