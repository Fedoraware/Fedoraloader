#pragma once
#include <string>

struct LaunchInfo
{
	bool Silent = false;
    bool NoBypass = false;
    bool UseLL = false;
    bool Debug = false;

    std::wstring File;
    std::wstring URL;
};
