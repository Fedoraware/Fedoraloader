#include "Bypass.h"
#include "../ManualMap/ManualMap.h"
#include "../../Utils/Utils.h"
#include "../../../resource.h"

#include <stdexcept>

void Bypass::Run()
{
	// Close Steam and TF2
	Utils::WaitCloseProcess("hl2.exe");
	Utils::WaitCloseProcess("steam.exe");
	Utils::WaitCloseProcess("SteamService.exe");
	Utils::WaitCloseProcess("steamwebhelper.exe");

	Sleep(1000);

	// Run TF2 (and Steam)
	ShellExecuteA(nullptr, nullptr, "steam://run/440", nullptr, nullptr, SW_SHOWNORMAL);

	// Inject VAC Bypass
	const HANDLE hSteam = Utils::WaitForProcessHandle("steam.exe", 90);
	if (hSteam == INVALID_HANDLE_VALUE || hSteam == nullptr)
	{
		throw std::runtime_error("Timeout while waiting for steam");
	}

	const Binary vacBypass = Utils::GetBinaryResource(IDR_VACBYPASS);
	MM::Inject(hSteam, vacBypass);
}
