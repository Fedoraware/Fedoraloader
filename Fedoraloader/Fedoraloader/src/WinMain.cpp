#include "LaunchInfo.h"
#include "Tray/Tray.h"
#include "Loader/Loader.h"
#include "Utils/Utils.h"

#include <VersionHelpers.h>
#include <stdexcept>

#define CHECK_ARG(szArg, bOut) if (wcscmp(arg, L##szArg) == 0) { (bOut) = true; continue; }
#define CHECK_ARG_STR(szArg, szOut) if (wcscmp(arg, L##szArg) == 0 && i < nArgs - 1) { const auto nextArg = szArglist[++i]; (szOut) = std::wstring(nextArg); continue; }

LaunchInfo GetLaunchInfo()
{
    int nArgs;
    LaunchInfo info{};
    LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (!szArglist) { return info; }
	
    for (int i = 0; i < nArgs; i++)
    {
        const auto arg = szArglist[i];

        CHECK_ARG("-silent", info.Silent)
        CHECK_ARG("-nobypass", info.NoBypass)
        CHECK_ARG("-ll", info.UseLL)
        CHECK_ARG("-debug", info.Debug)

        CHECK_ARG_STR("-file", info.File)
        CHECK_ARG_STR("-url", info.URL)
    }

    LocalFree(szArglist);
    return info;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	Log::SetLevel(LogLevel::Debug);
	LaunchInfo launchInfo = GetLaunchInfo();

	// Show debug console
    if (launchInfo.Debug)
    {
	    Utils::ShowConsole();
    }

    // Apparently people are still using Windows 7 in 2023...
    if (!IsWindows8OrGreater())
    {
	    MessageBoxA(nullptr, "Your Windows version is outdated and will most likely not work!", "Outdated OS", MB_OK | MB_ICONWARNING);
    }

    // Check privileges
    if (!Utils::IsElevated())
    {
        MessageBoxA(nullptr, "Please restart Fedoraloader as administrator!", "Missing elevation", MB_OK | MB_ICONWARNING);
	    return 0;
    }

    try
    {
    	if (launchInfo.Silent)
	    {
			Loader::Run(launchInfo);
	    }
	    else
	    {
		    Tray::Run(launchInfo, hInstance);
	    }
    }
	catch (const std::exception& ex)
    {
        // Runtime exceptions
    	MessageBoxA(nullptr, ex.what(), "Exception", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    }
	catch (...)
    {
        // Unexpected errors
	    MessageBoxA(nullptr, "An unexpected error occured!", "Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    }

    // Hide debug console
    if (launchInfo.Debug) { Utils::ShowConsole(); }

    return 0;
}
