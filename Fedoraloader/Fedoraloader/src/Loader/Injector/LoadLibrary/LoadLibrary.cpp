#include "LoadLibrary.h"

#include <filesystem>
#include <stdexcept>

bool LL::Inject(HANDLE hTarget, LPCWSTR fileName)
{
	// Get the full file path
	WCHAR fullPath[MAX_PATH];
	if (GetFullPathNameW(fileName, MAX_PATH, fullPath, nullptr) == 0)
	{
		throw std::runtime_error("Failed to retrieve full file path");
	}

	// Check if the file exists
	if (!std::filesystem::exists(fullPath))
	{
		throw std::invalid_argument("The given file does not exist");
	}

	// Allocate and write the dll path
	const LPVOID lpPathAddress = VirtualAllocEx(hTarget, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (lpPathAddress == nullptr)
    {
	    throw std::runtime_error("Failed to allocate library path memory");
    }

	const DWORD dwWriteResult = WriteProcessMemory(hTarget, lpPathAddress, fullPath, MAX_PATH, nullptr);
    if (dwWriteResult == 0)
    {
		VirtualFreeEx(hTarget, lpPathAddress, 0, MEM_RELEASE);
	    throw std::runtime_error("Failed to write library path");
    }

	Log::Debug("LL: Library path @ {:p}", lpPathAddress);

	// Load the dll
	const HANDLE hThread = CreateRemoteThread(hTarget, nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryW), lpPathAddress, NULL, nullptr);
	if (!hThread)
	{
		VirtualFreeEx(hTarget, lpPathAddress, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to create remote LoadLibrary thread");
	}

	// Wait for thread
	Log::Info("Waiting for target thread...");
	WaitForSingleObject(hThread, 15 * 1000);

	// Cleanup
	CloseHandle(hTarget);
	VirtualFreeEx(hTarget, lpPathAddress, 0, MEM_RELEASE);

	Log::Info("LL: Done!");
	return true;
}
