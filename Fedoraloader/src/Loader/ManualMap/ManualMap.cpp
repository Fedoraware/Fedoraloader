#include "ManualMap.h"

using PLoadLibraryA = HMODULE(__stdcall*)(LPCSTR lpLibFileName);
using PGetProcAddress = FARPROC(__stdcall*)(HMODULE hModule, LPCSTR lpProcName);
using PDllMain = BOOL(WINAPI*)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

struct ManualMapData
{
	LPVOID ImageBase;

	PLoadLibraryA FnLoadLibraryA;
	PGetProcAddress FnGetProcAddress;
};

bool MM::Inject(HANDLE hTarget, const BinData& data)
{
	// Retrieve the file headers
	const auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(data.Data);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) { return false; }

	const auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(data.Data + dosHeader->e_lfanew);
	const IMAGE_FILE_HEADER* fileHeader = &ntHeaders->FileHeader;
	const IMAGE_OPTIONAL_HEADER* optHeader = &ntHeaders->OptionalHeader;

	// Allocate memory for the binary file
	const LPVOID pTargetBase = VirtualAllocEx(hTarget, nullptr, optHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pTargetBase) { return false; }

	DWORD flOldProtect;
	VirtualProtectEx(hTarget, pTargetBase, optHeader->SizeOfImage, PAGE_EXECUTE_READWRITE, &flOldProtect);

	// Init manual map data
	ManualMapData mapData = {};
	mapData.ImageBase = pTargetBase;
	mapData.FnLoadLibraryA = LoadLibraryA;
	mapData.FnGetProcAddress = GetProcAddress;

	// Cleanup
	VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);

	return true;
}
