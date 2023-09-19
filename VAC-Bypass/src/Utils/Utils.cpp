#include "Utils.h"

bool Utils::HookImport(LPCWSTR moduleName, LPCSTR importModuleName, LPCSTR functionName, PVOID fun)
{
	const auto module = reinterpret_cast<PBYTE>(GetModuleHandleW(moduleName));
	if (!module) { return false; }

	const auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(module + reinterpret_cast<PIMAGE_DOS_HEADER>(module)->e_lfanew);
	const auto imports = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(module + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	for (PIMAGE_IMPORT_DESCRIPTOR import = imports; import->Name; import++)
	{
		// Is it the desired module?
		if (_strcmpi(reinterpret_cast<const char*>(module + import->Name), importModuleName))
		{
			continue;
		}

		for (auto originalFirstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(module + import->OriginalFirstThunk), firstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(module + import->FirstThunk);
		     originalFirstThunk->u1.AddressOfData;
		     originalFirstThunk++, firstThunk++)
		{
			// Is it the desired function?
			if (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(module + originalFirstThunk->u1.AddressOfData)->Name, functionName) != 0)
			{
				continue;
			}

			const PDWORD functionAddress = &firstThunk->u1.Function;

			// Swap the function pointer
			DWORD flOldProtect;
			if (VirtualProtect(functionAddress, sizeof(fun), PAGE_READWRITE, &flOldProtect))
			{
				*functionAddress = reinterpret_cast<DWORD>(fun);
				VirtualProtect(functionAddress, sizeof(fun), flOldProtect, &flOldProtect);
			}

			return true;
		}
	}

	return false;
}
