#include "Bypass.h"

#include "../Injector/ManualMap/ManualMap.h"
#include "../../Utils/Utils.h"
#include "../../../resource.h"

#include <stdexcept>
#include <format>

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
	Log::Info("Closing Steam & TF2...");
	ExitSteam();
	Sleep(1000);

	// Start Steam
	const LPCWSTR steamPath = GetSteamPath();
	const auto cmdLine = std::format(L"\"{:s}\" -applaunch 440", steamPath);
	delete[] steamPath;

	STARTUPINFOW startupInfo = {};
    PROCESS_INFORMATION processInfo = {};
	if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmdLine.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
	{
		throw std::system_error(GetLastError(), std::system_category(), "Failed to launch Steam");
	}

	// Wait for Steam
	Log::Info("Waiting for Steam...");
	if (!Utils::WaitForModule(processInfo.dwProcessId, "steam.exe", 60))
	{
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		throw std::runtime_error("Timeout while waiting for Steam");
	}

	// Inject VAC Bypass
	const Binary vacBypass = Utils::GetBinaryResource(IDR_VACBYPASS);
	MM::Inject(processInfo.hProcess, vacBypass, processInfo.hThread);

	// Cleanup
	ResumeThread(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);
}
