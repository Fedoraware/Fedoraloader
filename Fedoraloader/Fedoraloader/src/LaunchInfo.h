#pragma once
#include "Windows.h"

struct LaunchInfo
{
	bool Silent = false;
    bool NoBypass = false;
    bool Debug = false;

    LPCWSTR File = nullptr;
    LPCWSTR URL = nullptr;
};
