#include "Utils.h"

void Utils::HookImport(LPCWSTR moduleName, LPCSTR importModuleName, LPCSTR functionName, PVOID fun)
{
	const auto module = reinterpret_cast<PBYTE>(GetModuleHandleW(moduleName));
	if (!module) { return; }

	const auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(module + reinterpret_cast<PIMAGE_DOS_HEADER>(module)->e_lfanew);
	const auto imports = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(module + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	for (PIMAGE_IMPORT_DESCRIPTOR import = imports; import->Name; import++)
	{
		if (_strcmpi(reinterpret_cast<const char*>(module + import->Name), importModuleName))
		{
			continue;
		}

		for (auto originalFirstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(module + import->OriginalFirstThunk), firstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(module + import->FirstThunk);
		     originalFirstThunk->u1.AddressOfData;
		     originalFirstThunk++, firstThunk++)
		{
			if (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(module + originalFirstThunk->u1.AddressOfData)->Name, functionName) != 0)
			{
				continue;
			}

			const PDWORD functionAddress = &firstThunk->u1.Function;

			DWORD old;
			if (VirtualProtect(functionAddress, sizeof(fun), PAGE_READWRITE, &old))
			{
				*functionAddress = reinterpret_cast<DWORD>(fun);
				VirtualProtect(functionAddress, sizeof(fun), old, &old);
			}
			break;
		}
		break;
	}
}
