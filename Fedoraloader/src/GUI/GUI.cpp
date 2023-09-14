#include "GUI.h"
#include "Window/Window.h"
#include "../Loader/Loader.h"

#include <imgui/imgui.h>
#include <stdexcept>

void Render(const LaunchInfo& launchInfo)
{
	const auto& io = ImGui::GetIO();

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ io.DisplaySize.x, io.DisplaySize.y });
	if (ImGui::Begin("Fedoraloader", &Window::IsRunning, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
	{
		if (ImGui::Button("Load"))
		{
			Loader::Load(launchInfo);
		}

		ImGui::End();
	}
}

void GUI::Run(const LaunchInfo& launchInfo)
{
	Window::Create();

	// Render loop
	while (Window::IsRunning)
	{
		Window::BeginFrame();
		Render(launchInfo);
		Window::EndFrame();

		Sleep(1);
	}

	Window::Destroy();
}
