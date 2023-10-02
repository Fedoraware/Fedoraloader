#pragma once
#include <Windows.h>

using TRtlInsertInvertedFunctionTable = int(__fastcall)(PVOID BaseAddress, ULONG ImageSize);

namespace Native
{
	TRtlInsertInvertedFunctionTable* GetRtlInsertInvertedFunctionTable();
}