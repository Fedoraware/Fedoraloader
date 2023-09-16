#include "Loader.h"
#include "Web/Web.h"
#include "Zip/Zip.h"
#include "LoadLibrary/LoadLibrary.h"
#include "ManualMap/ManualMap.h"
#include "../Utils/Utils.h"
#include "../../resource.h"

#include <fstream>
#include <stdexcept>
#include <shellapi.h>

LPCWSTR ACTION_URL = L"https://nightly.link/Fedoraware/Fedoraware/workflows/msbuild/main/Fedoraware.zip";
LPCSTR DLL_FILE_NAME = "Fware-Release.dll";

// Retrieves the Fware binary from web/disk
Binary GetBinary(const LaunchInfo& launchInfo)
{
	Binary binary;

	// Read the file from web/disk
	if (launchInfo.File)
	{
		binary = Utils::ReadBinaryFile(launchInfo.File);
	}
	else
	{
		const LPCWSTR url = launchInfo.URL ? launchInfo.URL : ACTION_URL;
		binary = Web::DownloadFile(url);
	}

	// Check if the file is packed
	const auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(binary.Data);
	if (dosHeader->e_magic == 0x4B50)
	{
		Zip::UnpackFile(binary, DLL_FILE_NAME);
	}

	return binary;
}

// Loads and injects Fware
bool Loader::Load(const LaunchInfo& launchInfo)
{
	throw std::runtime_error("testz");

	// Retrieve the binary
	const Binary binary = GetBinary(launchInfo);
	if (!binary.Data || binary.Size < 0x1000)
	{
		throw std::runtime_error("Invalid binary file");
	}

	// (Optional) Restart Steam/TF2 and inject VAC Bypass
	if (!launchInfo.Unprotected)
	{
		// Close Steam and TF2
		Utils::WaitCloseProcess("hl2.exe");
		Utils::WaitCloseProcess("steam.exe");
		Utils::WaitCloseProcess("SteamService.exe");
		Utils::WaitCloseProcess("steamwebhelper.exe");

		Sleep(1000);

		// Run TF2 (and Steam)
		ShellExecuteA(nullptr, nullptr, "steam://run/440", nullptr, nullptr, SW_SHOWNORMAL);

		// Inject VAC Bypass
		const HANDLE hSteam = Utils::WaitForProcessHandle("steam.exe", 60);
		const Binary vacBypass = Utils::GetBinaryResource(IDR_VACBYPASS);
		MM::Inject(hSteam, vacBypass);
	}

	// Find the game
	const HANDLE hGame = Utils::WaitForProcessHandle("hl2.exe", 60);
	if (hGame == INVALID_HANDLE_VALUE || hGame == nullptr)
	{
		throw std::runtime_error("Timeout while waiting for game handle");
	}

	// Inject the binary
	const bool result = MM::Inject(hGame, binary);

	// Cleanup
	std::free(binary.Data);
	return result;
}

bool Loader::Debug(const LaunchInfo& launchInfo)
{
	// Find the game
	const HANDLE hGame = Utils::GetProcessHandle("hl2.exe");
	if (hGame == INVALID_HANDLE_VALUE || hGame == nullptr)
	{
		throw std::runtime_error("Failed to get game handle");
	}

	// Inject the binary
	return LL::Inject(hGame, launchInfo.File);
}
