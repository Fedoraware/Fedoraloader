#include "Native.h"

#include <format>
#include <vector>

#include "../../../../Utils/Utils.h"
#include "../../../../Utils/Pattern/Pattern.h"

using namespace std::string_literals;

PBYTE InitRtlInsertInvertedFunctionTable()
{
	static const std::vector PATTERNS = {
		"\x8B\xFF\x55\x8B\xEC\x83\xEC\x00\x53\x56\x57\x8D\x45\x00\x8B\xFA\x50\x8D\x55"s,	// 10.0.22621.1485
		"\x8B\xFF\x55\x8B\xEC\x83\xEC\x00\x53\x56\x57\x8D\x45\x00\x8B\xFA"s,				// 10.0.19041.3393
		"\x8B\xFF\x55\x8B\xEC\x83\xEC\x00\x8D\x45\x00\x53\x56\x57\x8B\xDA"s,				// 10.0.10240
		//"\x8B\xFF\x55\x8B\xEC\x56\x68"s, // 6.1.7601
	};

	for (const auto& pattern : PATTERNS)
	{
		if (const PBYTE result = Pattern::Find("ntdll.dll", pattern))
		{
			return result;
		}
	}

#ifdef _DEBUG
	{
		DWORD major, minor, buildNubmer;
		Utils::GetVersionNumbers(&major, &minor, &buildNubmer);
		const auto msg = std::format("Windows version not supported: {:d}.{:d}.{:d}\nPlease report this incident.", major, minor, buildNubmer);
		MessageBoxA(nullptr, msg.c_str(), "Warning", MB_OK | MB_ICONWARNING);
	}
#endif

	return nullptr;
}

TRtlInsertInvertedFunctionTable* Native::GetRtlInsertInvertedFunctionTable()
{
	static PBYTE fn = InitRtlInsertInvertedFunctionTable();
	return reinterpret_cast<TRtlInsertInvertedFunctionTable*>(fn);
}
