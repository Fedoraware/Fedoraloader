#include "ManualMap.h"
#include "Native/Native.h"

#include <cstdio>
#include <format>
#include <stdexcept>

#pragma warning(push)
#pragma warning(disable: 28160)
#pragma warning(disable: 6250)

using TLoadLibraryA = decltype(LoadLibraryA);
using TGetProcAddress = decltype(GetProcAddress);
using TDllMain = BOOL(WINAPI*)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

enum class MapResult : short
{
	Success = 0,
	NoData = 1
};

struct ManualMapData
{
	BYTE* ImageBase;
	MapResult Result = MapResult::Success;

	TLoadLibraryA* FnLoadLibraryA;
	TGetProcAddress* FnGetProcAddress;
	TRtlInsertInvertedFunctionTable* FnIIFT;
};

// Options
constexpr bool HANDLE_TLS = true;
constexpr bool ENABLE_EXCEPTIONS = true;
constexpr bool ERASE_PEH = true;
constexpr bool CLEAR_SECTIONS = true;
constexpr bool ADJUST_PROTECTION = true;

#pragma runtime_checks( "", off )
namespace Shellcode
{
	void __stdcall LibraryLoader(ManualMapData* pData)
	{
		if (!pData)
		{
			pData->Result = MapResult::NoData;
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
		const auto pRtlInsertInvertedFunctionTable = pData->FnIIFT;
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

					for (UINT i = 0; i < nEntries; i++, pRelativeInfo++)
					{
						const int type = *pRelativeInfo >> 0xC;
						const int offset = *pRelativeInfo & 0xFFF;

						if (type == IMAGE_REL_BASED_HIGHLOW)
						{
							const auto pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + offset);
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
				if (!pThunkRef) { pThunkRef = pFuncRef; }

				for (; *pThunkRef; pThunkRef++, pFuncRef++)
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

		// Exception support
		if (ENABLE_EXCEPTIONS)
		{
			if (pRtlInsertInvertedFunctionTable)
			{
				pRtlInsertInvertedFunctionTable(pBase, optHeader->SizeOfImage);
			}
		}

		// Invoke original entry point
		pDllMain(reinterpret_cast<HINSTANCE>(pBase), DLL_PROCESS_ATTACH, nullptr);

		pData->Result = MapResult::Success;
	}

	DWORD __stdcall Stub() { return 0; }
}
#pragma runtime_checks( "", restore )

DWORD GetSectionProtection(DWORD characteristics)
{
	// [EXECUTE][READ][WRITE]
	constexpr int protectionFlags[2][2][2] = {
		{
			// Not executable
			{ PAGE_NOACCESS, PAGE_WRITECOPY },
			{ PAGE_READONLY, PAGE_READWRITE }
		},
		{
			// Executable
			{ PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY },
			{ PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE }
		}
	};

	const bool bExecute = (characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
	const bool bRead = (characteristics & IMAGE_SCN_MEM_READ) != 0;
	const bool bWrite = (characteristics & IMAGE_SCN_MEM_WRITE) != 0;

	return protectionFlags[bExecute][bRead][bWrite];
}

bool EraseMemory(HANDLE hTarget, LPVOID dest, SIZE_T size = 0)
{
	if (size == 0) { return false; }

	DWORD flOldProtect;
	const std::unique_ptr<BYTE> nullBuffer(new BYTE[size]{});

	if (!WriteProcessMemory(hTarget, dest, nullBuffer.get(), size, nullptr))
	{
		return false;
	}

	if (!VirtualProtectEx(hTarget, dest, size, PAGE_NOACCESS, &flOldProtect))
	{
		return false;
	}

	if (!VirtualFreeEx(hTarget, dest, size, MEM_DECOMMIT))
	{
		return false;
	}

	return true;
}

bool MM::Inject(HANDLE hTarget, const Binary& binary, HANDLE mainThread)
{
	if (mainThread) { SuspendThread(mainThread); }

	DWORD flOldProtect = 0;
	const BYTE* pSrcData = binary.data();

	// Retrieve the file headers
	const auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(pSrcData);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		throw std::invalid_argument("The file is not a valid PE");
	}

	const auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(pSrcData + dosHeader->e_lfanew);
	const IMAGE_FILE_HEADER* fileHeader = &ntHeaders->FileHeader;
	const IMAGE_OPTIONAL_HEADER* optHeader = &ntHeaders->OptionalHeader;

	// Allocate memory for the binary file
	const auto pTargetBase = static_cast<BYTE*>(VirtualAllocEx(hTarget, nullptr, optHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!pTargetBase)
	{
		throw std::runtime_error("Failed to allocate memory in the target process");
	}

	VirtualProtectEx(hTarget, pTargetBase, optHeader->SizeOfImage, PAGE_EXECUTE_READWRITE, &flOldProtect);
	
	// Write file header
	if (!WriteProcessMemory(hTarget, pTargetBase, pSrcData, optHeader->SizeOfHeaders, nullptr))
	{
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to write PE header to target");
	}

	Log::Debug("MM: PE header @ {:p}", static_cast<void*>(pTargetBase));

	// Init manual map data
	const ManualMapData mapData{
		.ImageBase = pTargetBase,
		.FnLoadLibraryA = LoadLibraryA,
		.FnGetProcAddress = GetProcAddress,
		.FnIIFT = Native::GetRtlInsertInvertedFunctionTable()
	};

	// Write sections
	auto pSectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
	for (UINT i = 0; i < fileHeader->NumberOfSections; i++, pSectionHeader++)
	{
		if (pSectionHeader->SizeOfRawData == 0) { continue; }

		if (pSectionHeader->Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE))
		{
			const auto sectionName = reinterpret_cast<LPCSTR>(pSectionHeader->Name);
			Log::Debug("MM: Writing section '{}'...", sectionName);
			if (!WriteProcessMemory(hTarget, pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr))
			{
				VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
				throw std::runtime_error(std::format("Failed to map section: {}", sectionName));
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
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to write mapping data to target");
	}

	Log::Debug("MM: Manual Map Data @ {:p}", static_cast<void*>(pMapData));

	// Write library loader
	const SIZE_T loaderSize = reinterpret_cast<DWORD>(Shellcode::Stub) - reinterpret_cast<DWORD>(Shellcode::LibraryLoader);
	const LPVOID pLoader = VirtualAllocEx(hTarget, nullptr, loaderSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pLoader)
	{
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to allocate library loader memory");
	}

	if (!WriteProcessMemory(hTarget, pLoader, &Shellcode::LibraryLoader, loaderSize, nullptr))
	{
		VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to write library loader to target");
	}

	Log::Debug("MM: Library Loader @ {:p}", static_cast<void*>(pLoader));

	// Run the library loader
	const HANDLE hThread = CreateRemoteThread(hTarget, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoader), pMapData, 0, nullptr);
	if (!hThread)
	{
		VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
		VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
		throw std::runtime_error("Failed to create the remote thread");
	}

	if (mainThread) { ResumeThread(mainThread); }

	// Wait for the library loader
	Log::Info("MM: Waiting for target thread...");
	if (WaitForSingleObject(hThread, 20 * 1000) != WAIT_OBJECT_0)
	{
		CloseHandle(hThread);
		throw std::runtime_error("Timeout while waiting for thread");
	}

	CloseHandle(hThread);

	// Check injection result
	{
		DWORD exitCode = 0;
		GetExitCodeProcess(hTarget, &exitCode);
		if (exitCode != STILL_ACTIVE)
		{
			throw std::runtime_error(std::format("Process crashed with exit code: {:X}", exitCode));
		}

		// Read the manual map data
		ManualMapData resultData{};
		ReadProcessMemory(hTarget, pMapData, &resultData, sizeof(resultData), nullptr);

		// Check the error code
		if (resultData.Result == MapResult::NoData)
		{
			VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
			VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);
			VirtualFreeEx(hTarget, pTargetBase, 0, MEM_RELEASE);
			throw std::runtime_error("Manual map data was invalid");
		}

		Log::Debug("MM: LibraryLoader result was: {:d}", static_cast<int>(resultData.Result));
	}

	// (Optional) Adjust the section protection
	if (ADJUST_PROTECTION)
	{
		Log::Info("MM: Adjusting protections...");

		pSectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
		for (UINT i = 0; i != fileHeader->NumberOfSections; i++, pSectionHeader++)
		{
			if (pSectionHeader->Misc.VirtualSize == 0) { continue; }

			const auto sectionName = reinterpret_cast<LPCSTR>(pSectionHeader->Name);
			const DWORD flNewProtect = GetSectionProtection(pSectionHeader->Characteristics);
			if (flNewProtect == PAGE_NOACCESS)
			{
				// Decommit NO_ACCESS pages
				if (!VirtualFreeEx(hTarget, pTargetBase + pSectionHeader->VirtualAddress, pSectionHeader->Misc.VirtualSize, MEM_DECOMMIT))
				{
					Log::Warn("MM: Failed to free NO_ACCESS section '{}'", sectionName);
					DebugBreak();
				}
			}
			else
			{
				// Change protection
				if (!VirtualProtectEx(hTarget, pTargetBase + pSectionHeader->VirtualAddress, pSectionHeader->Misc.VirtualSize, flNewProtect, &flOldProtect))
				{
					Log::Warn("MM: Failed to set section '{}' to {}", sectionName, flNewProtect);
					DebugBreak();
				}
			}
		}
	}

	// (Optional) Clear unneeded sections
	if (CLEAR_SECTIONS)
	{
		Log::Info("MM: Clearing sections...");

		pSectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
		for (UINT i = 0; i != fileHeader->NumberOfSections; ++i, ++pSectionHeader)
		{
			const DWORD discardable = pSectionHeader->Characteristics & IMAGE_SCN_MEM_DISCARDABLE;
			if (pSectionHeader->Misc.VirtualSize && discardable)
			{
				const auto sectionName = reinterpret_cast<LPCSTR>(pSectionHeader->Name);
				Log::Debug("MM: Discarding section '{}'...", sectionName);
				if (!EraseMemory(hTarget, pTargetBase + pSectionHeader->VirtualAddress, pSectionHeader->Misc.VirtualSize))
				{
					Log::Warn("MM: Failed to discard section '{}'", sectionName);
				}
			}
		}
	}

	// (Optional) Clear PE header
	if (ERASE_PEH)
	{
		Log::Info("MM: Erasing PE header...");

		if (!EraseMemory(hTarget, pTargetBase, optHeader->SizeOfHeaders))
		{
			Log::Warn("MM: Failed to erase PE header");
		}
	}

	// Cleanup
	VirtualFreeEx(hTarget, pLoader, 0, MEM_RELEASE);
	VirtualFreeEx(hTarget, pMapData, 0, MEM_RELEASE);

	Log::Info("MM: Done!");
	return true;
}

#pragma warning(pop)
