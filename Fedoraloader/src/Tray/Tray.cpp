#include "Tray.h"

#include <functional>

#include "../resource.h"
#include "../Utils/Utils.h"
#include "../Loader/Loader.h"

#include <stdexcept>
#include <unordered_map>

#define WM_TRAY (WM_USER + 1)

NOTIFYICONDATA g_NotifyData;
WNDCLASS g_WindowClass;
HWND g_WindowHandle;

// Action IDsSh
constexpr int IDM_LOAD = 111;
constexpr int IDM_ABOUT = 112;
constexpr int IDM_EXIT = 113;

std::unordered_map<int, std::function<void()>> g_MenuCallbacks;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	// Initialize icon
	case WM_CREATE:
		g_NotifyData = {};
		g_NotifyData.cbSize = sizeof g_NotifyData;
		g_NotifyData.hWnd = hWnd;
		g_NotifyData.uID = 1;
		g_NotifyData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		g_NotifyData.uCallbackMessage = WM_USER + 1;
		g_NotifyData.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_ICON));
		lstrcpy(g_NotifyData.szTip, TEXT("Fedoraloader"));

		Shell_NotifyIcon(NIM_ADD, &g_NotifyData);
		return false;

	// Create the menu
	case WM_TRAY:
		if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP || lParam == WM_CONTEXTMENU)
		{
			SetForegroundWindow(g_WindowHandle);

			const HMENU hPopup = CreatePopupMenu();
			AppendMenu(hPopup, MF_STRING, IDM_LOAD, TEXT("Load"));
			AppendMenu(hPopup, MF_SEPARATOR, 1, nullptr);
			AppendMenu(hPopup, MF_STRING, IDM_ABOUT, TEXT("About"));
			AppendMenu(hPopup, MF_STRING, IDM_EXIT, TEXT("Exit"));

			POINT pt;
			GetCursorPos(&pt);
			TrackPopupMenu(hPopup, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_WindowHandle, nullptr);
			DestroyMenu(hPopup);
		}
		return false;

	// Handle buttons
	case WM_COMMAND:
		{
			const int itemId = LOWORD(wParam);
			const auto callback = g_MenuCallbacks.find(itemId);
			if (callback != g_MenuCallbacks.end())
			{
				callback->second();
			}
		}
		return false;

	case WM_HELP:
		ShellExecute(hWnd, nullptr, "https://github.com/Fedoraware/Fedoraware", nullptr, nullptr, SW_SHOW);
		return false;

	// Destroy the icon
	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &g_NotifyData);
		PostQuitMessage(0);
		return false;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CreateTray(HINSTANCE hInstance)
{
	// Create the window class
	g_WindowClass = {};
	g_WindowClass.lpfnWndProc = WindowProc;
	g_WindowClass.hInstance = hInstance;
	g_WindowClass.lpszClassName = TEXT("fl001");

	if (!RegisterClass(&g_WindowClass))
	{
		throw std::runtime_error("Failed to register try window class");
	}

	// Create the main window (hidden)
	g_WindowHandle = CreateWindow(g_WindowClass.lpszClassName, TEXT("Fedoraloader"), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (g_WindowHandle == nullptr || g_WindowHandle == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to create tray window");
	}
}

void RegisterCallbacks(const LaunchInfo& launchInfo)
{
	g_MenuCallbacks[IDM_LOAD] = [launchInfo]
	{
		Loader::Load(launchInfo);
	};

	g_MenuCallbacks[IDM_ABOUT] = []
	{
		MessageBoxA(
			g_WindowHandle,
			"Fedoraloader v2 (by lnx00)\n"
			"\n"
			"A loader and injector for the free and open-source,\n"
			"community-driven training software Fedoraware.\n"
			"\n"
			"Press 'Help' to view the official GitHub repository.",
			"About Fedoraloader",
			MB_HELP | MB_ICONINFORMATION | MB_SYSTEMMODAL
		);
	};

	g_MenuCallbacks[IDM_EXIT] = []
	{
		PostQuitMessage(0);
	};
}

void Tray::Run(const LaunchInfo& launchInfo, HINSTANCE hInstance)
{
	// Create tray menu
	RegisterCallbacks(launchInfo);
	CreateTray(hInstance);

	// Message loop
	MSG windowMsg;
	while (GetMessage(&windowMsg, nullptr, 0, 0))
	{
		TranslateMessage(&windowMsg);
		DispatchMessage(&windowMsg);
	}

	// Cleanup
	UnregisterClass(g_WindowClass.lpszClassName, g_WindowClass.hInstance);
}
