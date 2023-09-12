#include "LoadLibrary.h"

bool LL::Inject(HANDLE hTarget, LPCWSTR fileName)
{
	WCHAR fullPath[MAX_PATH];
	if (GetFullPathNameW(fileName, MAX_PATH, fullPath, nullptr) == 0)
	{
		return false;
	}

	// Allocate and write the dll path
	const LPVOID lpPathAddress = VirtualAllocEx(hTarget, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (lpPathAddress == nullptr) { return false; }

	const DWORD dwWriteResult = WriteProcessMemory(hTarget, lpPathAddress, fullPath, MAX_PATH, nullptr);
    if (dwWriteResult == 0) { return false; }

	// Load the dll
	const HANDLE hThread = CreateRemoteThread(hTarget, nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryW), lpPathAddress, NULL, nullptr);
	if (!hThread) { return false; }

	// Cleanup
	CloseHandle(hTarget);
	VirtualFreeEx(hTarget, lpPathAddress, 0, MEM_RELEASE);

	return true;
}
