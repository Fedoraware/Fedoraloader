#include <Windows.h>
#include "Utils/Utils.h"
#include "Hooks/Hooks.h"

void Init()
{
	if (const auto hMod = GetModuleHandle(TEXT("steamservice")))
	{
		const auto toPatch = reinterpret_cast<PBYTE>(Pattern::Find("steamservice", "74 47 6A 01 6A"));
		if (toPatch)
		{
			DWORD flOldProtect;
			VirtualProtect(toPatch, 1, PAGE_EXECUTE_READWRITE, &flOldProtect);
			*toPatch = 0xEB;
			VirtualProtect(toPatch, 1, flOldProtect, &flOldProtect);

			Utils::HookImport(L"steamservice", "kernel32.dll", "LoadLibraryExW", Hooks::Hk_LoadLibraryExW);
			MessageBoxW(nullptr, L"Initialization was successful!", L"VAC bypass", MB_OK | MB_ICONINFORMATION);
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
