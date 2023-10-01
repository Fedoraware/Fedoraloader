#pragma once
#include <Windows.h>

typedef int(__fastcall TRtlInsertInvertedFunctionTable)(PVOID BaseAddress, ULONG ImageSize);

namespace Native
{
	TRtlInsertInvertedFunctionTable* GetRtlInsertInvertedFunctionTable();
}