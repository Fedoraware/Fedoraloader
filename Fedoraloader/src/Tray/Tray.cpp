#include "Tray.h"
#include "../resource.h"
#include "../Utils/Utils.h"
#include "../Loader/Loader.h"

#include <stdexcept>

#define WM_TRAY (WM_USER + 1)

NOTIFYICONDATA g_NotifyData;
HINSTANCE g_Instance;
WNDCLASS g_WindowClass;
HWND g_WindowHandle;

constexpr int IDM_LOAD = 111;
constexpr int IDM_EXIT = 112;

namespace Callbacks
{
	void OnLoad()
	{
		MessageBox(nullptr, "Loading...", "Fedoralaoder", MB_OK);
	}
}

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
		g_NotifyData.hIcon = LoadIcon(g_Instance, MAKEINTRESOURCE(IDR_ICON));
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
			AppendMenu(hPopup, MF_STRING, IDM_EXIT, TEXT("Exit"));

			POINT pt;
            GetCursorPos(&pt);
            TrackPopupMenu(hPopup, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_WindowHandle, nullptr);
            DestroyMenu(hPopup);
		}
		return false;

	// Handle buttons
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_LOAD:
			Callbacks::OnLoad();
			break;

		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		}
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
    RegisterClass(&g_WindowClass);

	// Create the main window (hidden)
    g_WindowHandle = CreateWindow(g_WindowClass.lpszClassName, TEXT("Fedoraloader"), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (g_WindowHandle == nullptr || g_WindowHandle == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to create tray window");
	}
}

void Tray::Run(const LaunchInfo& launchInfo, HINSTANCE hInstance)
{
	g_Instance = hInstance;
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
