#include "GUI.h"
#include "Window/Window.h"
#include "../Loader/Loader.h"

#include <stdexcept>

void GUI::Run(const LaunchInfo& launchInfo)
{
	Window::Create();

	MessageBoxA(nullptr, "Hello, World!", "Fedoraloader", MB_OK);
	try
	{
		Loader::Load(launchInfo);
	} catch (const std::exception& ex)
	{
		MessageBox(nullptr, ex.what(), "Runtime Exception", MB_OK | MB_ICONERROR);
	} catch (...)
	{
		MessageBox(nullptr, "Failed to load", "Fedoraloader", MB_OK | MB_ICONERROR);
	}

	Window::Destroy();
}
