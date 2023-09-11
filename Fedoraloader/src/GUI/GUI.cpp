#include "GUI.h"
#include "../Loader/Loader.h"

void GUI::Run(const LaunchInfo& launchInfo)
{
	MessageBoxA(nullptr, "Hello, World!", "Fedoraloader", MB_OK);
	if (!Loader::Load(launchInfo))
	{
		MessageBoxA(nullptr, "Failed to load!", "Fedoraloader", MB_OK);
	}
}
