#define _USER32_
#include <Windows.h>

extern WNDPROC _currentWndProc;

extern void DebugPrintf(const char * fmt, ...);

WINUSERAPI HDC WINAPI BeginPaint(IN HWND hWnd, OUT LPPAINTSTRUCT lpPaint)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI INT_PTR WINAPI DialogBoxParamA(IN HINSTANCE hInstance, IN LPCSTR lpTemplateName, IN HWND hWndParent, IN DLGPROC lpDialogFunc, IN LPARAM dwInitParam)
{
	// TODO: implement
	return 0;
}

WINUSERAPI INT_PTR WINAPI DialogBoxParamW(IN HINSTANCE hInstance, IN LPCWSTR lpTemplateName, IN HWND hWndParent, IN DLGPROC lpDialogFunc, IN LPARAM dwInitParam)
{
	// TODO: implement
	return 0;
}

WINUSERAPI int WINAPI DrawTextA(IN HDC hDC, IN LPCSTR lpString, IN int nCount, IN OUT LPRECT lpRect, IN UINT uFormat)
{
	// TODO: implement
	return 0;
}

WINUSERAPI int WINAPI DrawTextW(IN HDC hDC, IN LPCWSTR lpString, IN int nCount, IN OUT LPRECT lpRect, IN UINT uFormat)
{
	// TODO: implement
	return 0;
}

WINUSERAPI BOOL WINAPI EnableWindow(IN HWND hWnd, IN BOOL bEnable)
{
	// TODO: implement
	return TRUE;
}

WINUSERAPI BOOL WINAPI EndDialog(IN HWND hDlg, IN INT_PTR nResult)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI BOOL WINAPI EndPaint(IN HWND hWnd, IN CONST PAINTSTRUCT *lpPaint)
{
	// TODO: implement
	return TRUE;
}

WINUSERAPI SHORT WINAPI GetAsyncKeyState(IN int vKey)
{
	XINPUT_DEBUG_KEYSTROKE keystroke;

	if (XInputDebugGetKeystroke(&keystroke) != ERROR_HANDLE_EOF) {
		if (keystroke.VirtualKey == vKey) {
			return keystroke.Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP ? 0 : 0x8000;
		}
	}

	return 0;
}

WINUSERAPI int WINAPI GetClassNameA(IN HWND hWnd, OUT LPSTR lpClassName, IN int nMaxCount)
{
	// TODO: implement
	return 0;
}

WINUSERAPI int WINAPI GetClassNameW(IN HWND hWnd, OUT LPWSTR lpClassName, IN int nMaxCount)
{
	// TODO: implement
	return 0;
}

WINUSERAPI BOOL WINAPI GetClientRect(IN HWND hWnd, OUT LPRECT lpRect)
{
	// TODO: proper implementation
	if (lpRect != NULL) {
		lpRect->bottom = 480;
		lpRect->left = 0;
		lpRect->right = 640;
		lpRect->top = 0;
	}

	return TRUE;
}

WINUSERAPI HDC WINAPI GetDC(IN HWND hWnd)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI GetDlgItem(IN HWND hDlg, IN int nIDDlgItem)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI SHORT WINAPI GetKeyState(IN int nVirtKey)
{
	// TODO: implement
	return 0;
}

WINUSERAPI HWND WINAPI GetNextDlgGroupItem(IN HWND hDlg, IN HWND hCtl, IN BOOL bPrevious)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI BOOL WINAPI InvalidateRect(IN HWND hWnd, IN CONST RECT *lpRect, IN BOOL bErase)
{
	DebugPrintf("InvalidateRect(0x%p, 0x%p, %s)\n", hWnd, lpRect, bErase ? "true" : "false");

	// TODO: implement
	return TRUE;
}

WINUSERAPI HCURSOR WINAPI LoadCursorA(IN HINSTANCE hInstance, IN LPCSTR lpCursorName)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HCURSOR WINAPI LoadCursorW(IN HINSTANCE hInstance, IN LPCWSTR lpCursorName)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HICON WINAPI LoadIconA(IN HINSTANCE hInstance, IN LPCSTR lpIconName)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HICON WINAPI LoadIconW(IN HINSTANCE hInstance, IN LPCWSTR lpIconName)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HANDLE WINAPI LoadImageA(IN HINSTANCE hInstance, IN LPCSTR name, IN UINT type, IN int cx, IN int cy, IN UINT fuLoad)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HANDLE WINAPI LoadImageW(IN HINSTANCE hInstance, IN LPCWSTR name, IN UINT type, IN int cx, IN int cy, IN UINT fuLoad)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI int WINAPI LoadStringA(IN HINSTANCE hInstance, IN UINT uID, OUT LPSTR lpBuffer, IN int nBufferMax)
{
	// TODO: implement
	return 0;
}

WINUSERAPI int WINAPI LoadStringW(IN HINSTANCE hInstance, IN UINT uID, OUT LPWSTR lpBuffer, IN int nBufferMax)
{
	// TODO: implement
	return 0;
}

WINUSERAPI int WINAPI MessageBoxA(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType)
{
	DebugPrintf("MessageBoxA(0x%p, \"%s\", \"%s\", 0x%08X)\n", hWnd, lpText, lpCaption, uType);

	// TODO: implement
	return 0;
}

WINUSERAPI int WINAPI MessageBoxW(IN HWND hWnd, IN LPCWSTR lpText, IN LPCWSTR lpCaption, IN UINT uType)
{
	// TODO: implement
	return 0;
}

WINUSERAPI VOID WINAPI PostQuitMessage(IN int nExitCode)
{
	PostMessage(NULL, WM_QUERYENDSESSION, 0, 0);
}

WINUSERAPI ATOM WINAPI RegisterClassExA(IN CONST WNDCLASSEXA *Arg1)
{
	_currentWndProc = Arg1->lpfnWndProc;

	return 1;
}

WINUSERAPI ATOM WINAPI RegisterClassExW(IN CONST WNDCLASSEXW *Arg1)
{
	// TODO: implement
	return 0;
}

WINUSERAPI int WINAPI ReleaseDC(IN HWND hWnd, IN HDC hDC)
{
	// TODO: implement
	return 1;
}

WINUSERAPI BOOL WINAPI ScreenToClient(IN HWND hWnd, IN OUT LPPOINT lpPoint)
{
	// no-op, since anything running on Xbox is always full-screen
	return TRUE;
}

WINUSERAPI LRESULT WINAPI SendDlgItemMessageA(IN HWND hDlg, IN int nIDDlgItem, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return 0;
}

WINUSERAPI LRESULT WINAPI SendDlgItemMessageW(IN HWND hDlg, IN int nIDDlgItem, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return 0;
}

WINUSERAPI LRESULT WINAPI SendMessageA(IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return 0;
}

WINUSERAPI LRESULT WINAPI SendMessageW(IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return 0;
}
