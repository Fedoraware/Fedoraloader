#include "Window.h"

#include <stdexcept>

WNDCLASSEX WindowClass;
HWND WindowHandle;

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Window::Create()
{
	// Initialize the window
	WindowClass = {
		.cbSize = sizeof WindowClass,
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = GetModuleHandle(nullptr),
		.hIcon = nullptr,
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = CreateSolidBrush(RGB(0, 0, 0)),
		.lpszMenuName = nullptr,
		.lpszClassName = "fl001",
		.hIconSm = nullptr
	};

	if (!RegisterClassEx(&WindowClass))
	{
		throw std::runtime_error("Failed to register window class");
	}

	// Create the window
	WindowHandle = CreateWindowEx(
		0,
		WindowClass.lpszClassName,
		"Fedoraloader",
		WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		400, 250,
		nullptr,
		nullptr,
		WindowClass.hInstance,
		nullptr
	);

	if (WindowHandle == nullptr || WindowHandle == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to create window");
	}

	// Show the window
	ShowWindow(WindowHandle, SW_SHOWDEFAULT);
	UpdateWindow(WindowHandle);
}

void Window::Destroy()
{
	DestroyWindow(WindowHandle);
	UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
}
