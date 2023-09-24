#pragma once
#include "../../Utils/Utils.h"

namespace MM
{
	bool Inject(HANDLE hTarget, const Binary& binary, bool waitForThread = true);
}
