#pragma once
#include "Windows.h"

struct LaunchInfo
{
	bool Silent = false;
    bool Unprotected = false;
    bool Debug = false;

    LPWSTR File = nullptr;
    LPWSTR URL = nullptr;
};
