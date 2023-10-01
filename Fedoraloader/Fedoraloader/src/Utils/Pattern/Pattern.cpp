#include "Pattern.h"

struct Section
{
	PBYTE BaseAddress = nullptr;
	DWORD Size = 0x0;
};

bool CompareByteArray(PBYTE pData, const std::vector<BYTE>& szPattern)
{
	for (size_t i = 0; i < szPattern.size(); i++, pData++)
	{
		const BYTE cur = szPattern[i];
		if (cur == '\x00') { continue; } // Wildcard
		if (*pData != cur) { return false; } // Mismatch
	}

	return true;
}

PBYTE FindPattern(PBYTE baseAddress, DWORD dwSize, const std::vector<BYTE>& szPattern)
{
	const PBYTE max = baseAddress + dwSize - szPattern.size();

	for (; baseAddress < max; baseAddress++)
	{
		if (CompareByteArray(baseAddress, szPattern))
		{
			return baseAddress;
		}
	}

	return nullptr;
}

Section GetTextSection(DWORD modHandle)
{
	const auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(modHandle);
	const auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(modHandle + dosHeader->e_lfanew);
	//const auto* optHeader = &ntHeaders->OptionalHeader;

	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) { return {}; }

	auto pSection = IMAGE_FIRST_SECTION(ntHeaders);
	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
	{
		if (strcmp(reinterpret_cast<const char*>(pSection->Name), ".text") == 0 &&
			(pSection->Characteristics & IMAGE_SCN_MEM_EXECUTE))
		{
			break;
		}
		pSection++;
	}

	const auto pNtdllCode = reinterpret_cast<PVOID>(reinterpret_cast<ULONG_PTR>(dosHeader) + pSection->VirtualAddress);
	const auto uNtdllCodeSize = pSection->SizeOfRawData;

	return {
		static_cast<PBYTE>(pNtdllCode),
		uNtdllCodeSize
	};
}

PBYTE Pattern::Find(LPCSTR szModuleName, const std::vector<BYTE>& szPattern)
{
	const auto modHandle = reinterpret_cast<DWORD>(GetModuleHandleA(szModuleName));
	if (!modHandle) { return nullptr; }

	const Section textSection = GetTextSection(modHandle);
	if (!textSection.BaseAddress || textSection.Size == 0) { return nullptr; }

	return FindPattern(textSection.BaseAddress, textSection.Size, szPattern);
}
