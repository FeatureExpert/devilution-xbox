#include <Windows.h>

extern BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

LD_LAUNCH_DASHBOARD launchDashboard;
LAUNCH_DATA launchData;

void _cdecl main(void)
{
	DWORD dwLaunchDataType;
	CHAR * szCmdLine = "";

	// Get any startup parameters
	if (XGetLaunchInfo(&dwLaunchDataType, &launchData) == ERROR_SUCCESS) {
		if (dwLaunchDataType == LDT_FROM_DASHBOARD) {
			LD_FROM_DASHBOARD * pLaunchData = (LD_FROM_DASHBOARD *)&launchData;

			// TODO: use this
		} else if (dwLaunchDataType == LDT_FROM_DEBUGGER_CMDLINE) {
			LD_FROM_DEBUGGER_CMDLINE * pLaunchData = (LD_FROM_DEBUGGER_CMDLINE *)&launchData;

			szCmdLine = pLaunchData->szCmdLine;
		} else if (dwLaunchDataType == LDT_FROM_UPDATE) {
			LD_FROM_UPDATE * pLaunchData = (LD_FROM_UPDATE *)&launchData;

			// TODO: use this
		} else {
			// TODO: use this
		}
	}

	// fire up DiabloUI
	DllMain(NULL, 1, NULL);

	// Diablo entry point
	WinMain(NULL, NULL, szCmdLine, 0);

	// return to the dashboard
	launchDashboard.dwReason = XLD_LAUNCH_DASHBOARD_MAIN_MENU;
	XLaunchNewImage(NULL, (PLAUNCH_DATA)&launchDashboard);
}
