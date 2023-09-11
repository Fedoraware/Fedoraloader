#include "Loader.h"
#include "LoadLibrary/LoadLibrary.h"
#include "ManualMap/ManualMap.h"

#include <fstream>

const char* ACTION_URL = "https://nightly.link/Fedoraware/Fedoraware/workflows/msbuild/main/Fedoraware.zip";

BYTE* ReadBinaryFile(LPWSTR fileName)
{
	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	if (inFile.fail()) { inFile.close(); return nullptr; }

	const auto fileSize = inFile.tellg();
	BYTE* data = new BYTE[static_cast<size_t>(fileSize)];
	if (!data) { inFile.close(); return nullptr; }

	inFile.seekg(0, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(data), fileSize);
	inFile.close();

	return data;
}

BYTE* GetBinary(const LaunchInfo& launchInfo)
{
	if (launchInfo.File)
	{
		return ReadBinaryFile(launchInfo.File);
	}
	else
	{
		// TODO: Download binary
	}

	return nullptr;
}

bool Loader::Load(const LaunchInfo& launchInfo)
{
	const BYTE* binary = GetBinary(launchInfo);
	if (!binary) { return false; }

	bool result = false;
	if (launchInfo.Debug)
	{
		result = LL::Inject();
	}
	else
	{
		result = MM::Inject();
	}

	Sleep(3000);

	// Cleanup
	delete[] binary;
	return result;
}
