#include "LaunchInfo.h"
#include "Tray/Tray.h"
#include "Loader/Loader.h"
#include "Utils/Utils.h"

#include <stdexcept>

#define CHECK_ARG(szArg, bOut) if (wcscmp(arg, L##szArg) == 0) { (bOut) = true; continue; }
#define CHECK_ARG_STR(szArg, szOut) if (wcscmp(arg, L##szArg) == 0 && i < nArgs - 1) { i++; const auto nextArg = szArglist[i]; (szOut) = Utils::CopyString(nextArg); continue; }

LaunchInfo GetLaunchInfo()
{
    int nArgs;
    LaunchInfo info{};
    LPWSTR* szArglist  =  CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (!szArglist) { return info; }
	
    for (int i = 0; i < nArgs; i++)
    {
        const auto arg = szArglist[i];

        CHECK_ARG("-silent", info.Silent)
        CHECK_ARG("-nobypass", info.NoBypass)
        CHECK_ARG("-debug", info.Debug)

        CHECK_ARG_STR("-file", info.File)
        CHECK_ARG_STR("-url", info.URL)
    }

    LocalFree(szArglist);
    return info;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	LaunchInfo launchInfo = GetLaunchInfo();

    if (!Utils::IsElevated())
    {
        MessageBoxA(nullptr, "Please run Fedoraloader as administrator!", "Missing elevation", MB_OK | MB_ICONWARNING);
	    return 0;
    }

    try
    {
    	if (launchInfo.Debug)
	    {
		    Loader::Debug(launchInfo);
	    }
		else if (launchInfo.Silent)
	    {
			Loader::Load(launchInfo);
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

    return 0;
}
