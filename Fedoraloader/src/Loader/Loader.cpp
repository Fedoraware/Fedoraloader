#include "Loader.h"
#include "LoadLibrary/LoadLibrary.h"
#include "ManualMap/ManualMap.h"
#include "../Utils/Utils.h"

#include <fstream>

const char* ACTION_URL = "https://nightly.link/Fedoraware/Fedoraware/workflows/msbuild/main/Fedoraware.zip";

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

// Retrieves the Fware binary from web/disk
BinData GetBinary(const LaunchInfo& launchInfo)
{
	if (launchInfo.File)
	{
		return ReadBinaryFile(launchInfo.File);
	}
	else
	{
		// TODO: Download binary
	}

	return {};
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
