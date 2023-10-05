#pragma once
#include <string>

#include "Windows.h"

struct LaunchInfo
{
	bool Silent = false;
    bool NoBypass = false;
    bool Debug = false;

    std::wstring File;
    std::wstring URL;
};
