#include "Tray.h"

#include <functional>

#include "../resource.h"
#include "../Utils/Utils.h"
#include "../Loader/Loader.h"

#include <stdexcept>
#include <system_error>
#include <unordered_map>

#define WM_TRAY (WM_USER + 1)

enum class PreferredAppMode
{
   Default,
   AllowDark,
   ForceDark,
   ForceLight,
   Max
};

using TSetPreferredAppMode = PreferredAppMode(WINAPI)(PreferredAppMode appMode);

// Action IDs
enum ACTION_ID {
	IDM_FIRST = 110,

	IDM_LOAD = IDM_FIRST,
	IDM_LOADEXIT,
	IDM_ABOUT,
	IDM_EXIT
};

NOTIFYICONDATA g_NotifyData;
WNDCLASS g_WindowClass;
HWND g_WindowHandle;

std::unordered_map<int, std::function<void()>> g_MenuCallbacks;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	// Initialize icon
	case WM_CREATE:
		{
			g_NotifyData = {};
			g_NotifyData.cbSize = sizeof g_NotifyData;
			g_NotifyData.hWnd = hWnd;
			g_NotifyData.uID = 1;
			g_NotifyData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO | NIF_REALTIME;
			g_NotifyData.uCallbackMessage = WM_USER + 1;
			g_NotifyData.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_ICON));
			lstrcpy(g_NotifyData.szTip, TEXT("Fedoraloader"));

			Shell_NotifyIcon(NIM_ADD, &g_NotifyData);
		}
		return false;

	// Create the menu
	case WM_TRAY:
		if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP || lParam == WM_CONTEXTMENU)
		{
			SetForegroundWindow(g_WindowHandle);

			const HMENU hPopup = CreatePopupMenu();
			AppendMenu(hPopup, MF_STRING, IDM_LOAD, TEXT("Load"));
			AppendMenu(hPopup, MF_STRING, IDM_LOADEXIT, TEXT("Load + Exit"));
			AppendMenu(hPopup, MF_SEPARATOR, 1, nullptr);
			AppendMenu(hPopup, MF_STRING, IDM_ABOUT, TEXT("About"));
			AppendMenu(hPopup, MF_STRING, IDM_EXIT, TEXT("Exit"));

			POINT cursorPos;
			GetCursorPos(&cursorPos);
			TrackPopupMenu(hPopup, TPM_RIGHTBUTTON, cursorPos.x, cursorPos.y, 0, g_WindowHandle, nullptr);
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
		{
			ShellExecute(hWnd, nullptr, "https://github.com/Fedoraware/Fedoraware", nullptr, nullptr, SW_SHOW);
		}
		return false;

	// Destroy the icon
	case WM_DESTROY:
		{
			Shell_NotifyIcon(NIM_DELETE, &g_NotifyData);
			PostQuitMessage(0);
		}
		return false;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL ShowNotification(LPCSTR title, LPCSTR text, DWORD flags = NIIF_INFO)
{
	lstrcpy(g_NotifyData.szInfoTitle, title);
	lstrcpy(g_NotifyData.szInfo, text);
	g_NotifyData.dwInfoFlags = flags;

	return Shell_NotifyIcon(NIM_MODIFY, &g_NotifyData);
}

void SetPreferredAppMode(PreferredAppMode mode)
{
	// Only supported for build 18362 and higher
	DWORD minor, major, buildNumber;
	Utils::GetVersionNumbers(&minor, &major, &buildNumber);
	if (buildNumber < 18362) { return; }

	static TSetPreferredAppMode* pSetPreferredAppMode = nullptr;
	if (pSetPreferredAppMode == nullptr)
	{
		const HMODULE hUxTheme = LoadLibraryEx("uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (hUxTheme == nullptr) { return; }

		const auto ord135 = GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135));
		if (ord135 == nullptr) { return; }

		pSetPreferredAppMode = reinterpret_cast<TSetPreferredAppMode*>(ord135);
		if (!pSetPreferredAppMode) { return; }

		FreeLibrary(hUxTheme);
	}

	pSetPreferredAppMode(mode);
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
		throw std::runtime_error("Failed to register tray window class");
	}

	// Create the main window (hidden)
	g_WindowHandle = CreateWindow(g_WindowClass.lpszClassName, TEXT("Fedoraloader"), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (g_WindowHandle == nullptr || g_WindowHandle == INVALID_HANDLE_VALUE)
	{
		throw std::system_error(GetLastError(), std::system_category(), "Failed to create tray window");
	}

	// Allow dark mode
	SetPreferredAppMode(PreferredAppMode::AllowDark);
}

void LoadSafe(const LaunchInfo& launchInfo)
{
	ShowNotification("Loading...", launchInfo.NoBypass
		                               ? "Please open the game now"
		                               : "Please wait until the game is ready");

	try
	{
		Loader::Load(launchInfo);
	}
	catch (const std::exception& ex)
	{
		ShowNotification("Error", ex.what(), NIIF_ERROR);
	}
	catch (...)
	{
		ShowNotification("Unexpected Error", "What did you do?", NIIF_ERROR);
	}
}

void RegisterCallbacks(const LaunchInfo& launchInfo)
{
	// "Load" button
	g_MenuCallbacks[IDM_LOAD] = [launchInfo]
	{
		LoadSafe(launchInfo);
	};

	// "Load + Exit" button
	g_MenuCallbacks[IDM_LOADEXIT] = [launchInfo]
	{
		LoadSafe(launchInfo);
		PostQuitMessage(0);
	};

	// "About" button
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
			"About",
			MB_HELP | MB_ICONINFORMATION | MB_SYSTEMMODAL
		);
	};

	// "Exit" button
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
