#include "Loader.h"
#include "Web/Web.h"
#include "Zip/Zip.h"
#include "Bypass/Bypass.h"
#include "Injector/LoadLibrary/LoadLibrary.h"
#include "Injector/ManualMap/ManualMap.h"
#include "../Utils/Utils.h"

#include <fstream>
#include <stdexcept>

constexpr WORD ZIP_SIGNATURE = 0x4B50;
LPCWSTR ACTION_URL = L"https://nightly.link/Fedoraware/Fedoraware/workflows/msbuild/main/Fedoraware.zip";

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
	if (dosHeader->e_magic == ZIP_SIGNATURE)
	{
		Zip::UnpackFile(binary);
	}

	return binary;
}

// Loads and injects Fware
bool Loader::Load(const LaunchInfo& launchInfo)
{
	// Retrieve the binary
	const Binary binary = GetBinary(launchInfo);
	if (!binary.Data || binary.Size < 0x1000)
	{
		throw std::runtime_error("Invalid binary file");
	}

	// (Optional) Restart Steam/TF2 and inject VAC Bypass
	if (!launchInfo.NoBypass)
	{
		Bypass::Run();
	}

	// Find the game
	const HANDLE hGame = Utils::WaitForProcessHandle("hl2.exe", 90);
	if (hGame == INVALID_HANDLE_VALUE || hGame == nullptr)
	{
		throw std::runtime_error("Timeout while waiting for game");
	}

	// Inject the binary
	const bool result = MM::Inject(hGame, binary);

	// Cleanup
	std::free(binary.Data);
	return result;
}

bool Loader::Debug(const LaunchInfo& launchInfo)
{
	if (!launchInfo.File)
	{
		throw std::runtime_error("LoadLibrary required a file path");
	}

	// Find the game
	const HANDLE hGame = Utils::GetProcessHandle("hl2.exe");
	if (hGame == INVALID_HANDLE_VALUE || hGame == nullptr)
	{
		throw std::runtime_error("Failed to get game handle");
	}

	// Inject the binary
	return LL::Inject(hGame, launchInfo.File);
}
