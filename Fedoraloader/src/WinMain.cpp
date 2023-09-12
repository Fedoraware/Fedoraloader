#include "LaunchInfo.h"
#include "GUI/GUI.h"
#include "Loader/Loader.h"

#define CHECK_ARG(szArg, bOut) if (wcscmp(arg, L##szArg) == 0) { (bOut) = true; continue; }
#define CHECK_ARG_STR(szArg, szOut) if (wcscmp(arg, L##szArg) == 0 && i < nArgs - 1) { i++; const auto nextArg = szArglist[i]; (szOut) = nextArg; continue; }

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
        CHECK_ARG("-unprotected", info.Unprotected)
        CHECK_ARG("-debug", info.Debug)

        CHECK_ARG_STR("-file", info.File)
        CHECK_ARG_STR("-url", info.URL)
    }

    LocalFree(szArglist);
    return info;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	const LaunchInfo launchInfo = GetLaunchInfo();

    if (launchInfo.Debug && launchInfo.File)
    {
	    Loader::Debug(launchInfo);
    }
	else if (launchInfo.Silent)
    {
		Loader::Load(launchInfo);
    }
    else
    {
	    GUI::Run(launchInfo);
    }

    return 0;
}
