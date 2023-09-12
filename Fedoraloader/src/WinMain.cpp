#include "LaunchInfo.h"
#include "GUI/GUI.h"
#include "Loader/Loader.h"

LaunchInfo GetLaunchInfo()
{
    int nArgs;
    const LPWSTR* szArglist  =  CommandLineToArgvW(GetCommandLineW(), &nArgs);
    
	LaunchInfo info{};
    for (int i = 0; i < nArgs; i++)
    {
        const auto arg = szArglist[i];

	    if (wcscmp(arg, L"-silent") == 0) { info.Silent = true; continue; }
        if (wcscmp(arg, L"-unprotected") == 0) { info.Unprotected = true; continue; }
        if (wcscmp(arg, L"-debug") == 0) { info.Debug = true; continue; }

        if (wcscmp(arg, L"-file") == 0 && i < nArgs - 1)
        {
            i++;
        	const auto nextArg = szArglist[i];
            info.File = nextArg;
        }
    }

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
