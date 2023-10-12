#pragma once
#include "../LaunchInfo.h"

namespace Loader
{
	void Run(const LaunchInfo& launchInfo);
	bool Load(const LaunchInfo& launchInfo);
	bool Debug(const LaunchInfo& launchInfo);
}
