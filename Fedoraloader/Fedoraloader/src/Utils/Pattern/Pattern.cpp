#include "Pattern.h"

#include <optional>

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
		if (cur == '\x00' || *pData == cur) { continue; }
		return false;
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

std::optional<Section> GetTextSection(DWORD modHandle)
{
	const auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(modHandle);
	const auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(modHandle + dosHeader->e_lfanew);

	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) { return {}; }

	auto pSection = IMAGE_FIRST_SECTION(ntHeaders);
	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
	{
		const auto sectionName = reinterpret_cast<const char*>(pSection->Name);
		if (strcmp(sectionName, ".text") == 0 && pSection->Characteristics & IMAGE_SCN_MEM_EXECUTE)
		{
			break;
		}
		pSection++;
	}

	const auto codeAddress = reinterpret_cast<PBYTE>(reinterpret_cast<ULONG_PTR>(dosHeader) + pSection->VirtualAddress);
	const auto codeSize = pSection->SizeOfRawData;

	return Section{
		.BaseAddress = codeAddress,
		.Size = codeSize
	};
}

PBYTE Pattern::Find(LPCSTR szModuleName, const std::vector<BYTE>& szPattern)
{
	const auto modHandle = reinterpret_cast<DWORD>(GetModuleHandleA(szModuleName));
	if (!modHandle) { return nullptr; }

	const auto textSection = GetTextSection(modHandle);
	if (!textSection) { return nullptr; }

	return FindPattern(textSection->BaseAddress, textSection->Size, szPattern);
}
