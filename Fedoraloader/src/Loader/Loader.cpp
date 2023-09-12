#include "Loader.h"
#include "Web/Web.h"
#include "Zip/Zip.h"
#include "LoadLibrary/LoadLibrary.h"
#include "ManualMap/ManualMap.h"
#include "../Utils/Utils.h"

#include <fstream>
#include <stdexcept>

LPCWSTR ACTION_URL = L"https://nightly.link/Fedoraware/Fedoraware/workflows/msbuild/main/Fedoraware.zip";
LPCSTR DLL_FILE_NAME = "Fware-Release.dll";

// Reads the given binary file from disk
BinData ReadBinaryFile(LPWSTR fileName)
{
	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	if (inFile.fail()) { inFile.close(); return {}; }

	const auto fileSize = inFile.tellg();
	BYTE* data = new BYTE[static_cast<size_t>(fileSize)];
	if (!data) { inFile.close(); return {}; }

	inFile.seekg(0, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(data), fileSize);
	inFile.close();

	return {
		.Data = data,
		.Size = static_cast<SIZE_T>(fileSize)
	};
}

BinData DownloadBinaryFile(LPCWSTR url)
{
	BinData dlFile = Web::DownloadFile(url);

	// Check if the file is packed
	const auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(dlFile.Data);
	if (dosHeader->e_magic == 0x4B50)
	{
		Zip::UnpackFile(dlFile, DLL_FILE_NAME);
	}

	return dlFile;
}

// Retrieves the Fware binary from web/disk
BinData GetBinary(const LaunchInfo& launchInfo)
{
	if (launchInfo.File)
	{
		return ReadBinaryFile(launchInfo.File);
	}

	const LPCWSTR url = launchInfo.URL ? launchInfo.URL : ACTION_URL;
	return DownloadBinaryFile(url);
}

// Loads and injects Fware
bool Loader::Load(const LaunchInfo& launchInfo)
{
	// Retrieve the binary
	const BinData binary = GetBinary(launchInfo);
	if (!binary.Data || binary.Size < 0x1000) { return false; }

	// Find the game
	const HANDLE hGame = Utils::GetProcessHandle("hl2.exe");
	if (hGame == INVALID_HANDLE_VALUE || hGame == nullptr) { return false; }

	// Inject the binary
	const bool result = MM::Inject(hGame, binary);

	// Cleanup
	delete[] binary.Data;
	return result;
}

bool Loader::Debug(const LaunchInfo& launchInfo)
{
	// Find the game
	const HANDLE hGame = Utils::GetProcessHandle("hl2.exe");
	if (hGame == INVALID_HANDLE_VALUE || hGame == nullptr) { return false; }

	// Inject the binary
	const bool result = LL::Inject(hGame, launchInfo.File);
	return result;
}
