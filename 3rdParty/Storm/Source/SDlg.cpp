#include "storm.h"

typedef struct {
	DWORD a0;
	DWORD a1;
	DWORD a2;
	DWORD a3;
	DWORD a4;
	void (__stdcall *Callback)(int, int, int, int);
	DWORD StartTicks;
} TIMER;

LPVOID __fastcall sub_15011130(HMODULE hModule, LPCSTR lpName);

BOOL STORMAPI SDlgCheckTimers()
{
	// TODO: implement

	return TRUE;
}

HANDLE STORMAPI SDlgCreateDialogIndirectParam(HMODULE hModule, LPCSTR lpName, HWND hWnd, LPVOID lpParam, LPARAM lParam)
{
	// TODO: implement

	return NULL;
}

HANDLE STORMAPI SDlgCreateDialogParam(HMODULE hModule, LPCSTR lpName, HWND hWndParent, LPVOID lpParam, LPARAM lParam)
{
	void * ptr = sub_15011130(hModule, lpName);

	if (ptr == NULL) {
		return NULL;
	}

	return SDlgCreateDialogIndirectParam(GetModuleHandle(NULL), (LPCSTR)ptr, hWndParent, lpParam, lParam);
}

LPVOID __fastcall sub_15011130(HMODULE hModule, LPCSTR lpName)
{
	HRSRC res = FindResource(hModule, lpName, RT_DIALOG);

	if (res == NULL) {
		return NULL;
	}

	HGLOBAL hglbl = LoadResource(hModule, res);

	if (hglbl == NULL) {
		return NULL;
	}

	return LockResource(hglbl);
}

HGDIOBJ STORMAPI SDlgDefDialogProc(HWND hDlg, signed int DlgType, HDC textLabel, HWND hWnd)
{
	// TODO: implement

	return NULL;
}

/*HANDLE*/int STORMAPI SDlgDialogBoxParam(HMODULE hModule, LPCSTR lpName, int /* HWND */ hWndParent, WNDPROC lpParam, LPARAM lParam)
{
	void * ptr = sub_15011130(hModule, lpName);

	if (ptr == NULL) {
		return NULL;
	}

	return (int)SDlgDialogBoxIndirectParam(GetModuleHandle(NULL), (LPCSTR)ptr, (HWND)hWndParent, lpParam, lParam);
}

HANDLE STORMAPI SDlgDialogBoxIndirectParam(HMODULE hModule, LPCSTR lpName, HWND hWndParent, LPVOID lpParam, LPARAM lParam)
{
	// TODO: implement

	return NULL;
}

//@implemented
BOOL STORMAPI SDlgEndDialog(HWND hDlg, HANDLE nResult)
{
	SetProp(hDlg, "SDlg_EndDialog", (HANDLE)1);
	SetProp(hDlg, "SDlg_EndResult", nResult);
	return EndDialog(hDlg, (INT_PTR)nResult);
}

BOOL STORMAPI SDlgSetControlBitmaps(HWND parentwindow, int *id, int a3, char *buffer2, char *buffer, int flags, int mask)
{
	// TODO: implement

	return TRUE;
}

//@implemented
BOOL STORMAPI SDlgBltToWindowI(HWND hWnd, HRGN a2, char *a3, int a4, void *buffer, RECT *rct, SIZE *size, int a8, int a9, DWORD rop)
{
	LONG var_4;
	LONG var_8;
	LONG var_C;
	LONG var_10;

	if (rct != NULL) {
		var_10 = rct->bottom;
		var_C = rct->left;
		var_8 = rct->right + 1;
		var_4 = rct->top + 1;

		rct = (RECT *)&var_4;
	}

	return SDlgBltToWindowE(hWnd, a2, a3, a4, buffer, rct, size, a8, a9, rop);
}

BOOL STORMAPI SDlgBltToWindowE(HWND hWnd, HRGN a2, char *a3, int a4, void *buffer, RECT *rct, SIZE *size, int a8, int a9, DWORD rop)
{
	/*tagRECT Rect;
	DWORD pdwHeight;
	DWORD pdwWidth;

	if (hWnd == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	if (IsWindow(hWnd)) {
		if (IsWindowVisible(hWnd)) {
			if (!IsIconic(hWnd)) {
				if (a8 == 0xFFFFFFFF) {
					SDrawGetScreenSize(&pdwWidth, &pdwHeight, NULL);

					GetClientRect(hWnd, &Rect);

					Rect.left += a3;
					Rect.top += a4;


				}
			}
		}
	}*/

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SDlgSetBitmapE(HWND hWnd, int a2, char *src, int mask1, int flags, int a6, int a7, int width, int a9, int mask2)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SDlgSetBitmapI(HWND hWnd, int a2, char *src, int mask1, int flags, void *pBuff, int a7, int width, int height, int mask2)
{
	// TODO: implement

	return TRUE;
}

void STORMAPI SDlgBeginPaint(HWND hWnd, char *a2)
{
}

void STORMAPI SDlgEndPaint(HWND hWnd, char *a2)
{
}

void STORMAPI SDlgSetSystemCursor(BYTE *a1, BYTE *a2, int *a3, int a4)
{
}

void STORMAPI SDlgSetCursor(HWND hWnd, HCURSOR a2, int a3, int *a4)
{
}

BOOL STORMAPI SDlgSetTimer(int a1, int a2, int a3, void (__stdcall *a4)(int, int, int, int))
{
	SDlgKillTimer(a1, a2);

	DWORD * mem = (DWORD *)SMemAlloc(28, ".?AUTIMERREC@@", SLOG_OBJECT, 8);

	if (mem) {
		mem[0] = 0;
		mem[1] = 0;
	}

	// a1 = hWnd
	// a2 = id?
	// a3 = interval?
	// a4 = callback
	//   param 0 = hWnd

	

	// TODO: implement

	mem[2] = a1;
	mem[3] = a2;
	mem[4] = a3;
	mem[5] = (DWORD)a4;
	mem[6] = GetTickCount();

	return TRUE;
}

BOOL STORMAPI SDlgKillTimer(int a1, int a2)
{
	// a1 = hWnd
	// a2 = id?

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SDlgDrawBitmap(HWND hWnd, int a2, int a3, int a4, int a5, int a6, int a7)
{
	// TODO: implement

	return TRUE;
}
