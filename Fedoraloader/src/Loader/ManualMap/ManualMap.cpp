#include "ManualMap.h"
#include <cstdio>
#include <format>
#include <stdexcept>

#define RELOC_FLAG(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)

using TLoadLibraryA = decltype(LoadLibraryA);
using TGetProcAddress = decltype(GetProcAddress);
using TDllMain = BOOL(WINAPI*)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

struct ManualMapData
{
	BYTE* ImageBase;
	HINSTANCE Module;

	TLoadLibraryA* FnLoadLibraryA;
	TGetProcAddress* FnGetProcAddress;
};

// Options
constexpr bool ERASE_PEH = true;
constexpr bool CLEAR_SECTIONS = true;
constexpr bool ADJUST_PROTECTION = true;

#pragma runtime_checks( "", off )
void __stdcall LibraryLoader(ManualMapData* pData)
{
	if (!pData)
	{
		pData->Module = (HINSTANCE)0x404040;
		return;
	}

	BYTE* pBase = pData->ImageBase;
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
					if (RELOC_FLAG(*pRelativeInfo))
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

	if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
	{
		auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
		auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);
		for (; pCallback && *pCallback; ++pCallback)
		{
			(*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
		}
	}

	// Invoke original entry point
	_DllMain(reinterpret_cast<HINSTANCE>(pBase), DLL_PROCESS_ATTACH, nullptr);

	pData->Module = reinterpret_cast<HINSTANCE>(pBase);
}
#pragma runtime_checks( "", restore )

bool MM::Inject(HANDLE hTarget, const BinData& binary)
{
	DWORD flOldProtect = 0;
	BYTE* pSrcData = binary.Data;

	// Retrieve the file headers
	const auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		throw std::invalid_argument("The file is not a valid PE");
	}

	const auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + dosHeader->e_lfanew);
	const IMAGE_FILE_HEADER* fileHeader = &ntHeaders->FileHeader;
	const IMAGE_OPTIONAL_HEADER* optHeader = &ntHeaders->OptionalHeader;

	// Allocate memory for the binary file
	const auto pTargetBase = static_cast<BYTE*>(VirtualAllocEx(hTarget, nullptr, optHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!pTargetBase)
	{
		throw std::runtime_error("Failed to allocate memory in the target process");
	}

	VirtualProtectEx(hTarget, pTargetBase, optHeader->SizeOfImage, PAGE_EXECUTE_READWRITE, &flOldProtect);

	// Init manual map data
	ManualMapData mapData{};
	mapData.FnLoadLibraryA = LoadLibraryA;
	mapData.FnGetProcAddress = GetProcAddress;
	mapData.ImageBase = pTargetBase;

	// Write file header (first 0x1000 bytes)
	if (!WriteProcessMemory(hTarget, pTargetBase, pSrcData, 0x1000, nullptr))
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to write PE header");
	}

	// Write sections
	auto pSectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
	for (UINT i = 0; i != fileHeader->NumberOfSections; ++i, ++pSectionHeader)
	{
		if (pSectionHeader->SizeOfRawData)
		{
			if (!WriteProcessMemory(hTarget, pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr))
			{
				VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
				throw std::runtime_error("Failed to map sections");
			}
		}
	}

	// Write manual map data
	const auto pMapData = static_cast<BYTE*>(VirtualAllocEx(hTarget, nullptr, sizeof(ManualMapData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!pMapData)
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to allocate manual map data memory");
	}

	if (!WriteProcessMemory(hTarget, pMapData, &mapData, sizeof(ManualMapData), nullptr))
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to write manual map data");
	}

	// Write library loader
	const LPVOID pLoader = VirtualAllocEx(hTarget, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pLoader)
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to allocate library loader memory");
	}

	if (!WriteProcessMemory(hTarget, pLoader, &LibraryLoader, 0x1000, nullptr))
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to write library loader");
	}

	// Run the library loader
	const HANDLE hThread = CreateRemoteThread(hTarget, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoader), pMapData, 0, nullptr);
	if (!hThread)
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to create the remote thread");
	}
	CloseHandle(hThread);

	// Wait for the target thread
	HINSTANCE hCheck = nullptr;
	while (!hCheck)
	{
		// Retrieve the exit code
		DWORD exitCode = 0;
		GetExitCodeProcess(hTarget, &exitCode);
		if (exitCode != STILL_ACTIVE)
		{
			throw std::runtime_error(std::format("Process crashed with exit code: %d", exitCode));
		}

		ManualMapData resultData{};
		ReadProcessMemory(hTarget, pMapData, &resultData, sizeof(resultData), nullptr);
		hCheck = resultData.Module;

		if (hCheck == reinterpret_cast<HINSTANCE>(0x404040))
		{
			VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
			VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
			VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
			throw std::runtime_error("Manual map data was invalid");
		}

		Sleep(10);
	}

	const BYTE* nullBuffer = new BYTE[1024 * 1024 * 20]{};
	if (nullBuffer == nullptr)
	{
		throw std::runtime_error("Failed to allocate null buffer");
	}

	// (Optional) Clear PE header
	if (ERASE_PEH)
	{
		if (!WriteProcessMemory(hTarget, pTargetBase, nullBuffer, 0x1000, nullptr))
		{
			std::printf("Failed to erase PE header\n");
		}
	}

	// (Optional) Clear unneeded sections
	if (CLEAR_SECTIONS)
	{
		pSectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
		for (UINT i = 0; i != fileHeader->NumberOfSections; ++i, ++pSectionHeader)
		{
			if (pSectionHeader->Misc.VirtualSize)
			{
				const auto sectionName = reinterpret_cast<const char*>(pSectionHeader->Name);
				if ((strcmp(sectionName, ".pdata") == 0) ||
					strcmp(sectionName, ".rsrc") == 0 ||
					strcmp(sectionName, ".reloc") == 0)
				{
					if (!WriteProcessMemory(hTarget, pTargetBase + pSectionHeader->VirtualAddress, nullBuffer, pSectionHeader->Misc.VirtualSize, nullptr))
					{
						std::printf("Failed to clear unneeded section %s\n", sectionName);
					}
				}
			}
		}
	}

	// (Optional) Adjust the section protection
	if (ADJUST_PROTECTION)
	{
		pSectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
		for (UINT i = 0; i != fileHeader->NumberOfSections; ++i, ++pSectionHeader)
		{
			if (pSectionHeader->Misc.VirtualSize)
			{
				DWORD flNewProtect = PAGE_READONLY;

				if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) > 0)
				{
					flNewProtect = PAGE_READWRITE;
				}
				else if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) > 0)
				{
					flNewProtect = PAGE_EXECUTE_READ;
				}
				if (!VirtualProtectEx(hTarget, pTargetBase + pSectionHeader->VirtualAddress, pSectionHeader->Misc.VirtualSize, flNewProtect, &flOldProtect))
				{
					std::printf("Failed to set %s to %lX\n", reinterpret_cast<char*>(pSectionHeader->Name), flNewProtect);
				}
			}
		}

		VirtualProtectEx(hTarget, pTargetBase, IMAGE_FIRST_SECTION(ntHeaders)->VirtualAddress, PAGE_READONLY, &flOldProtect);
	}

	// Cleanup
	WriteProcessMemory(hTarget, pLoader, nullBuffer, 0x1000, nullptr);
	VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
	VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
	delete[] nullBuffer;

	return true;
}
