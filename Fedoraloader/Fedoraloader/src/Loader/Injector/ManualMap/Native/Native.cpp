#include "Native.h"

#include <format>
#include <vector>

#include "../../../../Utils/Utils.h"
#include "../../../../Utils/Pattern/Pattern.h"

// 6.1.7601: \x8B\xFF\x55\x8B\xEC\x56\x68

using namespace std::string_literals;

const std::vector IIFT_Patterns = {
	"\x8B\xFF\x55\x8B\xEC\x83\xEC\x00\x53\x56\x57\x8D\x45\x00\x8B\xFA"s, // 10.0.19041.3393
	"\x8B\xFF\x55\x8B\xEC\x83\xEC\x00\x8D\x45\x00\x53\x56\x57\x8B\xDA"s, // 10.0.10240
};

PBYTE InitRtlInsertInvertedFunctionTable()
{
	for (const auto& pattern : IIFT_Patterns)
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
