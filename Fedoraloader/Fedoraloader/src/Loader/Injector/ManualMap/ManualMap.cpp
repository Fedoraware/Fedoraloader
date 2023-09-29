#include "ManualMap.h"
#include <cstdio>
#include <format>
#include <stdexcept>

#define RELOC_FLAG(relInfo) (((relInfo) >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)

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
constexpr bool ADJUST_PROTECTION = false;
constexpr bool HANDLE_TLS = true;

#pragma runtime_checks( "", off )
void __stdcall LibraryLoader(ManualMapData* pData)
{
	if (!pData)
	{
		pData->Module = reinterpret_cast<HINSTANCE>(0x404040);
		return;
	}

	// Retrieve file headers
	BYTE* pBase = pData->ImageBase;
	const auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(pBase);
	const auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + dosHeader->e_lfanew);
	const IMAGE_OPTIONAL_HEADER* optHeader = &ntHeaders->OptionalHeader;

	// Retrieve our function pointers
	const auto pLoadLibraryA = pData->FnLoadLibraryA;
	const auto pGetProcAddress = pData->FnGetProcAddress;
	const auto pDllMain = reinterpret_cast<TDllMain>(pBase + optHeader->AddressOfEntryPoint);

	// Base address relocation
	BYTE* locationDelta = pBase - optHeader->ImageBase;
	if (locationDelta)
	{
		const auto relocData = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		if (relocData.Size)
		{
			auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pBase + relocData.VirtualAddress);
			const auto* pRelocEnd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(pRelocData) + relocData.Size);
			while (pRelocData < pRelocEnd && pRelocData->SizeOfBlock)
			{
				const UINT nEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
				auto pRelativeInfo = reinterpret_cast<WORD*>(pRelocData + 1);

				for (UINT i = 0; i < nEntries; ++i, ++pRelativeInfo)
				{
					if (RELOC_FLAG(*pRelativeInfo))
					{
						const auto pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
						*pPatch += reinterpret_cast<UINT_PTR>(locationDelta);
					}
				}
				pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
			}
		}
	}

	// Resolve imports
	const auto importData = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (importData.Size)
	{
		const auto* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + importData.VirtualAddress);
		while (pImportDescr->Name)
		{
			const auto szMod = reinterpret_cast<char*>(pBase + pImportDescr->Name);
			const HINSTANCE hDll = pLoadLibraryA(szMod);

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
					*pFuncRef = reinterpret_cast<ULONG_PTR>(pGetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF)));
				}
				else
				{
					const auto* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pBase + (*pThunkRef));
					*pFuncRef = reinterpret_cast<ULONG_PTR>(pGetProcAddress(hDll, pImport->Name));
				}
			}
			++pImportDescr;
		}
	}

	// Handle TLS callbacks
	if (HANDLE_TLS)
	{
		const auto tlsData = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
		if (tlsData.Size)
		{
			const auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + tlsData.VirtualAddress);
			const auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);

			while (pCallback && *pCallback)
			{
				(*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
				pCallback++;
			}
		}
	}

	// Invoke original entry point
	pDllMain(reinterpret_cast<HINSTANCE>(pBase), DLL_PROCESS_ATTACH, nullptr);

	pData->Module = reinterpret_cast<HINSTANCE>(pBase);
}

DWORD __stdcall Stub() { return 0; }
#pragma runtime_checks( "", restore )

bool MM::Inject(HANDLE hTarget, const Binary& binary, bool waitForThread)
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
		throw std::runtime_error("Failed to write PE header to target");
	}

	// Write sections
	auto pSectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
	for (UINT i = 0; i < fileHeader->NumberOfSections; ++i, ++pSectionHeader)
	{
		if (pSectionHeader->SizeOfRawData)
		{
			if (!WriteProcessMemory(hTarget, pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr))
			{
				VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
				throw std::runtime_error(std::format("Failed to map section: {}", reinterpret_cast<const char*>(pSectionHeader->Name)));
			}
		}
	}

	// Write manual map data
	const auto pMapData = static_cast<BYTE*>(VirtualAllocEx(hTarget, nullptr, sizeof(ManualMapData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!pMapData)
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to allocate mapping data memory");
	}

	if (!WriteProcessMemory(hTarget, pMapData, &mapData, sizeof(ManualMapData), nullptr))
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to write mapping data to target");
	}

	// Write library loader
	const LPVOID pLoader = VirtualAllocEx(hTarget, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pLoader)
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to allocate library loader memory");
	}

	const SIZE_T loaderSize = reinterpret_cast<DWORD>(Stub) - reinterpret_cast<DWORD>(LibraryLoader);
	if (!WriteProcessMemory(hTarget, pLoader, &LibraryLoader, loaderSize, nullptr))
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to write library loader to target");
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

	if (!waitForThread)
	{
		CloseHandle(hThread);
		return true;
	}

	// Wait for the library loader
	WaitForSingleObject(hThread, 15 * 1000);
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
			throw std::runtime_error(std::format("Process crashed with exit code: {:d}", exitCode));
		}

		// Read the manual map data
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
				if (strcmp(sectionName, ".pdata") == 0 ||
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

				// Change protection
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
