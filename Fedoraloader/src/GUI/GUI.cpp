#include "GUI.h"
#include "../Loader/Loader.h"
#include <Windows.h>

void GUI::Run()
{
	MessageBoxA(nullptr, "Hello, World!", "Fedoraloader", MB_OK);
	if (!Loader::Load())
	{
		MessageBoxA(nullptr, "Failed to load!", "Fedoraloader", MB_OK);
	}
}
