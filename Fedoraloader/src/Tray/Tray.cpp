#include "Tray.h"
#include "../Utils/Utils.h"

#include <stdexcept>

NOTIFYICONDATA niData;
HWND g_WindowHandle;

constexpr int IDM_LOAD = 102;
constexpr int IDM_EXIT = 103;

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
		niData = {};
		niData.cbSize = sizeof niData;
		niData.hWnd = hWnd;
		niData.uID = 1;
		niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		niData.uCallbackMessage = WM_USER + 1;
		niData.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPLICATION));
		lstrcpy(niData.szTip, TEXT("Fedoraloader"));
		Shell_NotifyIcon(NIM_ADD, &niData);
		return false;

	// Create the menu
	case WM_USER + 1:
		if (lParam == WM_RBUTTONDOWN || lParam == WM_CONTEXTMENU) // TODO: RBUTTONUP?
		{
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
		Shell_NotifyIcon(NIM_DELETE, &niData);
		PostQuitMessage(0);
		return false;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CreateTray(HINSTANCE hInstance)
{
	// Create the window class
	WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("fl001");
    RegisterClass(&wc);

	// Create the main window (hidden)
    g_WindowHandle = CreateWindow(wc.lpszClassName, TEXT("Fedoraloader"), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (g_WindowHandle == nullptr || g_WindowHandle == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to create tray window");
	}
}

void Tray::Run(const LaunchInfo& launchInfo, HINSTANCE hInstance)
{
	CreateTray(hInstance);

	// Message loop
    MSG windowMsg;
    while (GetMessage(&windowMsg, nullptr, 0, 0))
	{
        TranslateMessage(&windowMsg);
        DispatchMessage(&windowMsg);
    }
}
