#include "GUI.h"
#include "Window/Window.h"
#include "../Loader/Loader.h"
#include "ImTheme.h"

#include <stdexcept>

void Render(const LaunchInfo& launchInfo)
{
	if (ImGui::Button("Load", ImVec2(-1, 35)))
	{
		Loader::Load(launchInfo);
	}

	ImGui::End();
}

void GUI::Run(const LaunchInfo& launchInfo)
{
	Window::Create();
	ImTheme::Enemymouse();

	// Render loop
	while (Window::IsRunning)
	{
		Window::BeginFrame();

		// Main window
		const auto& io = ImGui::GetIO();
		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ io.DisplaySize.x, io.DisplaySize.y });
		if (ImGui::Begin("Fedoraloader", &Window::IsRunning, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
		{
			Render(launchInfo);
		}

		Window::EndFrame();

		Sleep(5);
	}

	Window::Destroy();
}
