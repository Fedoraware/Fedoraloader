#pragma once
#include <Windows.h>
#include "Pattern/Pattern.h"

namespace Utils
{
	bool HookImport(LPCWSTR moduleName, LPCSTR importModuleName, LPCSTR functionName, PVOID fun);
}
