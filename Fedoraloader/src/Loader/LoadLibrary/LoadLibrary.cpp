#include "LoadLibrary.h"

bool LL::Inject(HANDLE hTarget, LPWSTR fileName)
{
	WCHAR fullPath[MAX_PATH];
	if (GetFullPathNameW(fileName, MAX_PATH, fullPath, nullptr) == 0)
	{
		return false;
	}

	const SIZE_T fullPathSize = wcslen(fullPath);

	// Allocate and write the dll path
	const LPVOID lpPathAddress = VirtualAllocEx(hTarget, nullptr, fullPathSize + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (lpPathAddress == nullptr) { return false; }

	const DWORD dwWriteResult = WriteProcessMemory(hTarget, lpPathAddress, fullPath, fullPathSize + 1, nullptr);
    if (dwWriteResult == 0) { return false; }

	// Get the LoadLibraryW address
	const HMODULE hModule = GetModuleHandleA("kernel32.dll");
	if (hModule == INVALID_HANDLE_VALUE || hModule == nullptr) { return false; }

    const FARPROC lpFunctionAddress = GetProcAddress(hModule, "LoadLibraryW");
    if (lpFunctionAddress == nullptr) { return false; }

	// Load the dll
	const HANDLE hThread = CreateRemoteThread(hTarget, nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(lpFunctionAddress), lpPathAddress, NULL, nullptr);
	if (!hThread) { return false; }

	// Cleanup
	CloseHandle(hTarget);
	VirtualFreeEx(hTarget, lpPathAddress, 0, MEM_RELEASE);

	return true;
}
