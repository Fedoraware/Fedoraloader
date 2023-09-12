#pragma once
#include "Windows.h"

struct LaunchInfo
{
	bool Silent = false;
    bool Unprotected = false;
    bool Debug = false;

    LPCWSTR File = nullptr;
    LPCWSTR URL = nullptr;
};
