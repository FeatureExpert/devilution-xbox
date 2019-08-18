#include <Windows.h>

extern BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

LD_LAUNCH_DASHBOARD launchDashboard;
LAUNCH_DATA launchData;
IDirect3DDevice8 * d3dDevice;
IDirect3D8 * d3d;
D3DPRESENT_PARAMETERS d3dpp;

void InitD3D()
{
	// Set up the presentation parameters for a double-buffered, 640x480,
	// 32-bit display using depth-stencil. Override these parameters in
	// your derived class as your app requires.
	ZeroMemory(&d3dpp, sizeof(D3DPRESENT_PARAMETERS));
	d3dpp.BackBufferWidth        = 640;
	d3dpp.BackBufferHeight       = 480;
	d3dpp.BackBufferFormat       = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount        = 1;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;

	d3d = Direct3DCreate8(D3D_SDK_VERSION);
	d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice);

	// Persist the Xbox splash screen until we get properly initialized
	d3dDevice->PersistDisplay();
}

void _cdecl main(void)
{
	DWORD dwLaunchDataType;
	CHAR * szCmdLine = "";

	InitD3D();

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
