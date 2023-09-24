#include "Bypass.h"
#include "../ManualMap/ManualMap.h"
#include "../../Utils/Utils.h"
#include "../../../resource.h"

#include <stdexcept>

void ExitSteam()
{
	Utils::WaitCloseProcess("hl2.exe");
	Utils::WaitCloseProcess("steam.exe");
	Utils::WaitCloseProcess("SteamService.exe");
	Utils::WaitCloseProcess("steamwebhelper.exe");
}

LPCWSTR GetSteamPath()
{
	HKEY hKey = nullptr;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
	{
		throw std::runtime_error("Failed to open registry key");
	}

	WCHAR steamPath[MAX_PATH];
	DWORD bufferSize = sizeof(steamPath);
	if (RegQueryValueExW(hKey, L"SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(steamPath), &bufferSize) != ERROR_SUCCESS)
	{
		throw std::runtime_error("Failed to query registry key value");
	}

	return Utils::CopyString(steamPath);
}

void Bypass::Run()
{
	// Close Steam and TF2
	ExitSteam();
	Sleep(1000);

	// Start steam
	const LPCWSTR steamPath = GetSteamPath();
	const LPCWSTR launchArgs = L" -applaunch 440";

	STARTUPINFOW startupInfo = {};
    PROCESS_INFORMATION processInfo = {};
	if (!CreateProcessW(steamPath, const_cast<LPWSTR>(launchArgs), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
	{
		throw std::runtime_error("Failed to run steam");
	}

	// Wait for Steam
	Utils::WaitForModule(processInfo.dwProcessId, "steam.exe");
	SuspendThread(processInfo.hThread);

	// Inject VAC Bypass
	const Binary vacBypass = Utils::GetBinaryResource(IDR_VACBYPASS);
	MM::Inject(processInfo.hProcess, vacBypass, false);

	// Cleanup
	ResumeThread(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);
	delete[] steamPath;
}
