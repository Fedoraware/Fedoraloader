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
	if (!binary.Data) { return false; }

	// Find the game
	const DWORD pid = Utils::FindProcess("hl2.exe");
	if (pid == 0) { return false; }

	const HANDLE hGame = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hGame == INVALID_HANDLE_VALUE) { return false; }

	// Inject the binary
	bool result;
	if (launchInfo.Debug)
	{
		result = LL::Inject(hGame, binary);
	}
	else
	{
		result = MM::Inject();
	}

	Sleep(3000);

	// Cleanup
	delete[] binary.Data;
	return result;
}
