#include "Native.h"

#include <format>

#include "../../../../Utils/Utils.h"
#include "../../../../Utils/Pattern/Pattern.h"

PBYTE InitRtlInsertInvertedFunctionTable()
{
	if (const PBYTE result = Pattern::Find("ntdll.dll", {0x8B, 0xFF, 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x00, 0x53, 0x56, 0x57, 0x8D, 0x45, 0x00, 0x8B, 0xFA}))
	{
		return result;
	}

#ifdef _DEBUG
	{
		DWORD major, minor, buildNubmer;
		Utils::GetVersionNumbers(&major, &minor, &buildNubmer);
		const auto msg = std::format("Windows Version not supported:\n{:d}.{:d}.{:d}", major, minor, buildNubmer);
		MessageBoxA(nullptr, msg.c_str(), "Error", MB_OK | MB_ICONERROR);
	}
#endif

	return nullptr;
}

TRtlInsertInvertedFunctionTable* Native::GetRtlInsertInvertedFunctionTable()
{
	static PBYTE fn = InitRtlInsertInvertedFunctionTable();
	return reinterpret_cast<TRtlInsertInvertedFunctionTable*>(fn);
}
