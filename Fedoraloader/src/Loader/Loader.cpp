#include "Loader.h"
#include "LoadLibrary/LoadLibrary.h"
#include "ManualMap/ManualMap.h"
#include "../Utils/Utils.h"

#include <fstream>

const char* ACTION_URL = "https://nightly.link/Fedoraware/Fedoraware/workflows/msbuild/main/Fedoraware.zip";

int ReadBinaryFile(LPWSTR fileName, BinData& outData)
{
	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	if (inFile.fail()) { inFile.close(); return -1; }

	const auto fileSize = inFile.tellg();
	BYTE* data = new BYTE[static_cast<size_t>(fileSize)];
	if (!data) { inFile.close(); return -2; }

	inFile.seekg(0, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(data), fileSize);
	inFile.close();

	outData.Data = data;
	outData.Size = static_cast<size_t>(fileSize);
	return 0;
}

int GetBinary(const LaunchInfo& launchInfo, BinData& outData)
{
	if (launchInfo.File)
	{
		return ReadBinaryFile(launchInfo.File, outData);
	}
	else
	{
		// TODO: Download binary
	}

	return 0;
}

int Loader::Load(const LaunchInfo& launchInfo)
{
	// Retrieve the binary
	BinData binary{};
	const int binResult = GetBinary(launchInfo, binary);
	if (binResult != 0) { return binResult; }

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
