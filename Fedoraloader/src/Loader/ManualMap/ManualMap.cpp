#include "ManualMap.h"

#define RELOC_FLAG32(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)

using TLoadLibraryA = decltype(LoadLibraryA);
using TGetProcAddress = decltype(GetProcAddress);
using TDllMain = BOOL(WINAPI*)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

struct ManualMapData
{
	LPVOID ImageBase;
	HINSTANCE Module;

	TLoadLibraryA* FnLoadLibraryA;
	TGetProcAddress* FnGetProcAddress;
};

void __stdcall LibraryLoader(ManualMapData* pData)
{
	if (!pData)
	{
		pData->Module = reinterpret_cast<HINSTANCE>(0x404040);
		return;
	}

	auto pBase = static_cast<BYTE*>(pData->ImageBase);
	auto* pOpt = &reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + reinterpret_cast<IMAGE_DOS_HEADER*>((uintptr_t)pBase)->e_lfanew)->OptionalHeader;

	auto _LoadLibraryA = pData->FnLoadLibraryA;
	auto _GetProcAddress = pData->FnGetProcAddress;
	auto _DllMain = reinterpret_cast<TDllMain>(pBase + pOpt->AddressOfEntryPoint);

	BYTE* LocationDelta = pBase - pOpt->ImageBase;
	if (LocationDelta)
	{
		if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
		{
			auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
			const auto* pRelocEnd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(pRelocData) + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
			while (pRelocData < pRelocEnd && pRelocData->SizeOfBlock)
			{
				UINT AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
				auto pRelativeInfo = reinterpret_cast<WORD*>(pRelocData + 1);

				for (UINT i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo)
				{
					if (RELOC_FLAG32(*pRelativeInfo))
					{
						auto pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
						*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
					}
				}
				pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
			}
		}
	}

	if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
	{
		auto* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		while (pImportDescr->Name)
		{
			auto szMod = reinterpret_cast<char*>(pBase + pImportDescr->Name);
			HINSTANCE hDll = _LoadLibraryA(szMod);

			auto pThunkRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->OriginalFirstThunk);
			auto pFuncRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->FirstThunk);

			if (!pThunkRef)
			{
				pThunkRef = pFuncRef;
			}

			for (; *pThunkRef; ++pThunkRef, ++pFuncRef)
			{
				if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef))
				{
					*pFuncRef = (ULONG_PTR)_GetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF));
				}
				else
				{
					auto* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pBase + (*pThunkRef));
					*pFuncRef = (ULONG_PTR)_GetProcAddress(hDll, pImport->Name);
				}
			}
			++pImportDescr;
		}
	}

	// Handle TLS callbacks
	if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
	{
		auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
		auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);
		for (; pCallback && *pCallback; ++pCallback)
		{
			(*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
		}
	}

	// Invoke the target DllMain
	_DllMain(reinterpret_cast<HINSTANCE>(pBase), DLL_PROCESS_ATTACH, nullptr);

	pData->Module = reinterpret_cast<HINSTANCE>(pBase);
}

constexpr bool ERASE_PEH = true;

bool MM::Inject(HANDLE hTarget, const BinData& data)
{
	BYTE* pSrcData = data.Data;

	// Retrieve the file headers
	const auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) { return false; }

	const auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + dosHeader->e_lfanew);
	const IMAGE_FILE_HEADER* fileHeader = &ntHeaders->FileHeader;
	const IMAGE_OPTIONAL_HEADER* optHeader = &ntHeaders->OptionalHeader;

	// Allocate memory for the binary file
	const auto pTargetBase = static_cast<BYTE*>(VirtualAllocEx(hTarget, nullptr, optHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!pTargetBase) { return false; }

	DWORD flOldProtect;
	VirtualProtectEx(hTarget, pTargetBase, optHeader->SizeOfImage, PAGE_EXECUTE_READWRITE, &flOldProtect);

	// Init manual map data
	const ManualMapData mapData {
		.ImageBase = pTargetBase,

		.FnLoadLibraryA = LoadLibraryA,
		.FnGetProcAddress = GetProcAddress
	};

	// Write file header (First 0x1000 bytes)
	if (!WriteProcessMemory(hTarget, pTargetBase, pSrcData, 0x1000, nullptr))
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		return false;
	}

	IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
	for (UINT i = 0; i != fileHeader->NumberOfSections; ++i, ++pSectionHeader) {
		if (pSectionHeader->SizeOfRawData) {
			if (!WriteProcessMemory(hTarget, pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr)) {
				VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
				return false;
			}
		}
	}

	// Write manual map data
	const auto pMapData = static_cast<ManualMapData*>(VirtualAllocEx(hTarget, nullptr, sizeof(ManualMapData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!pMapData) {
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		return false;
	}

	if (!WriteProcessMemory(hTarget, pMapData, &mapData, sizeof(ManualMapData), nullptr)) {
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		return false;
	}

	// Write library loader
	const LPVOID pLoader = VirtualAllocEx(hTarget, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pLoader) {
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		return false;
	}

	if (!WriteProcessMemory(hTarget, pLoader, LibraryLoader, 0x1000, nullptr)) {
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
		return false;
	}

	//MessageBoxA(nullptr, "Dll mapped", "Fedoraloader", MB_OK);

	// Run the library loader
	const HANDLE hThread = CreateRemoteThread(hTarget, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoader), pMapData, 0, nullptr);
	if (!hThread) {
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
		return false;
	}
	CloseHandle(hThread);

	// Wait for the library loader
	WaitForSingleObject(hThread, INFINITE);

	// Cleanup
	VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
	VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);

	return true;
}
