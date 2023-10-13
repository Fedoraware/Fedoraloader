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
const std::wstring ACTION_URL = L"https://nightly.link/Fedoraware/Fedoraware/workflows/msbuild/main/Fedoraware.zip";

// Retrieves the Fware binary from web/disk
Binary GetBinary(const LaunchInfo& launchInfo)
{
	Binary binary;

	// Read the file from web/disk
	if (!launchInfo.File.empty())
	{
		Log::Info(L"Reading file: {}", launchInfo.File);
		binary = Utils::ReadBinaryFile(launchInfo.File);
	}
	else
	{
		const auto& url = launchInfo.URL.empty() ? ACTION_URL : launchInfo.URL;
		Log::Info(L"Downloading file: {}", url);

		binary = Web::DownloadFile(url);
	}

	// Check if the file is packed
	const auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(binary.data());
	if (dosHeader->e_magic == ZIP_SIGNATURE)
	{
		Log::Info("Zip file detected! Unpaking...");
		binary = Zip::UnpackFile(binary);
	}

	return binary;
}

void Loader::Run(const LaunchInfo& launchInfo)
{
	if (launchInfo.UseLL)
	{
		Debug(launchInfo);
	}
	else
	{
		Load(launchInfo);
	}
}

// Loads and injects Fware
bool Loader::Load(const LaunchInfo& launchInfo)
{
	// Retrieve the binary
	const Binary binary = GetBinary(launchInfo);
	if (binary.empty() || binary.size() < 0x1000)
	{
		throw std::runtime_error("Invalid binary file");
	}

	// (Optional) Restart Steam/TF2 and inject VAC Bypass
	if (!launchInfo.NoBypass)
	{
		Bypass::Run();
	}

	// Find the game
	Log::Info("Waiting for game...");
	const HANDLE hGame = Utils::WaitForProcessHandle("hl2.exe", 90);
	if (hGame == INVALID_HANDLE_VALUE || hGame == nullptr)
	{
		throw std::runtime_error("Timeout while waiting for game");
	}

	// Inject the binary
	Log::Info("Manual mapping {:d} bytes...", binary.size());
	return MM::Inject(hGame, binary);
}

bool Loader::Debug(const LaunchInfo& launchInfo)
{
	if (launchInfo.File.empty())
	{
		throw std::runtime_error("LoadLibrary required a file path");
	}

	// Find the game
	Log::Info("Waiting for game...");
	const HANDLE hGame = Utils::GetProcessHandle("hl2.exe");
	if (hGame == INVALID_HANDLE_VALUE || hGame == nullptr)
	{
		throw std::runtime_error("Failed to get game handle");
	}

	// Inject the binary
	Log::Info(L"Running LoadLibrary with file {}", launchInfo.File);
	return LL::Inject(hGame, launchInfo.File.c_str());
}
