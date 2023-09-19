#pragma once
#include <Windows.h>
#include "Pattern/Pattern.h"

namespace Utils
{
	void HookImport(LPCWSTR moduleName, LPCSTR importModuleName, LPCSTR functionName, PVOID fun);
}
