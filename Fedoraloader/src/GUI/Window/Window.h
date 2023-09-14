#pragma once
#include <Windows.h>

namespace Window
{
	inline HWND WindowHandle;
	inline bool IsRunning = true;

	void Create();
	void Destroy();

	void BeginFrame();
	void EndFrame();
}
