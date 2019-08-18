#include "storm.h"

#include <assert.h>

LPDIRECTDRAW ddraw;
LPDIRECTDRAWSURFACE ddBackBuffer;
LPDIRECTDRAWPALETTE ddPal;
LPDIRECTDRAWSURFACE ddPrimary;
HPALETTE palette;
HWND windowHandle;

BOOL STORMAPI SDrawAutoInitialize(HINSTANCE hInst, LPCSTR lpClassName, LPCSTR lpWindowName, WNDPROC pfnWndProc, int nMode, int nWidth, int nHeight, int nBits)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SDrawCaptureScreen(const char *source)
{
	// TODO: implement

	return TRUE;
}

//@implemented
HWND STORMAPI SDrawGetFrameWindow(HWND *sdraw_framewindow)
{
	if (sdraw_framewindow) {
		*sdraw_framewindow = windowHandle;
	}

	return windowHandle;
}

BOOL STORMAPI SDrawGetObjects(LPDIRECTDRAW *ddInterface, LPDIRECTDRAWSURFACE *primarySurface, LPDIRECTDRAWSURFACE *surface2, LPDIRECTDRAWSURFACE *surface3, LPDIRECTDRAWSURFACE *backSurface, LPDIRECTDRAWPALETTE *ddPalette, HPALETTE *hPalette)
{
	if (ddraw == NULL) {
		return FALSE;
	}

	if (ddInterface) {
		*ddInterface = ddraw;
	}

	if (primarySurface) {
		*primarySurface = ddPrimary;
	}

	if (surface2) {
		// unused
		*surface2 = NULL;
	}

	if (surface3) {
		// unused
		*surface3 = NULL;
	}

	if (backSurface) {
		*backSurface = ddBackBuffer;
	}

	if (ddPalette) {
		*ddPalette = ddPal;
	}

	if (hPalette) {
		*hPalette = palette;
	}

	return TRUE;
}

BOOL STORMAPI SDrawGetScreenSize(DWORD *pdwWidth, DWORD *pdwHeight, DWORD *pdwBpp)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SDrawLockSurface(int surfacenumber, RECT *lpDestRect, void **lplpSurface, int *lpPitch, int arg_unused)
{
	if (lplpSurface) {
		*lplpSurface = NULL;
	}

	if (lpPitch) {
		lpPitch = 0;
	}

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SDrawManualInitialize(HWND hWnd, LPDIRECTDRAW ddInterface, LPDIRECTDRAWSURFACE primarySurface, LPDIRECTDRAWSURFACE surface2, LPDIRECTDRAWSURFACE surface3, LPDIRECTDRAWSURFACE backSurface, LPDIRECTDRAWPALETTE ddPalette, HPALETTE hPalette)
{
	palette = hPalette;
	ddraw = ddInterface;
	ddPrimary = primarySurface;
	ddBackBuffer = backSurface;
	ddPal = ddPalette;
	windowHandle = hWnd;

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SDrawPostClose()
{
	if (windowHandle == NULL) {
		return FALSE;
	}

	PostMessage(windowHandle, WM_CLOSE, 0, 0);

	return TRUE;
}

BOOL STORMAPI SDrawUnlockSurface(int surfacenumber, void *lpSurface, int a3, RECT *lpRect)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SDrawUpdatePalette(unsigned int firstentry, unsigned int numentries, PALETTEENTRY *pPalEntries, int a4)
{
	if (firstentry + numentries > 256 || pPalEntries == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	ddPal->SetEntries(0, firstentry, numentries, pPalEntries);

	return TRUE;
}

void __cdecl SDrawRealizePalette(void)
{
	// TODO: implement
}

void STORMAPI SDrawClearSurface(int a1)
{
	// TODO: implement
}

void STORMAPI SDrawMessageBox(const char *,const char *,int)
{
	// TODO: implement
}

void __cdecl SDrawDestroy(void)
{
	// TODO: implement
}

void STORMAPI SDrawSetClientRect(RECT * rct)
{
	if (rct == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return;
	}

	// TODO: implement
}
