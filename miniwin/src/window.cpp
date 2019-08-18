#define _USER32_
#include <Windows.h>

#include <map>

extern void DebugPrintf(const char * fmt, ...);

WNDPROC _currentWndProc;
static std::map<HWND, std::map<LPCSTR, HANDLE> > _wndProps;

WINUSERAPI LRESULT WINAPI CallWindowProcA(IN WNDPROC lpPrevWndFunc, IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return 0;
}

WINUSERAPI LRESULT WINAPI CallWindowProcW(IN WNDPROC lpPrevWndFunc, IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return 0;
}

WINUSERAPI HWND WINAPI CreateWindowExA(IN DWORD dwExStyle, IN LPCSTR lpClassName, IN LPCSTR lpWindowName, IN DWORD dwStyle, IN int X, IN int Y, IN int nWidth, IN int nHeight, IN HWND hWndParent, IN HMENU hMenu, IN HINSTANCE hInstance, IN LPVOID lpParam)
{
	DebugPrintf(
		"CreateWindowExA(0x%08X, \"%s\", \"%s\", 0x%08X, %i, %i, %i, %i, %p, %p, %p, %p)\n",
		dwExStyle,
		lpClassName,
		lpWindowName,
		dwStyle,
		X,
		Y,
		nWidth,
		nHeight,
		hWndParent,
		hMenu,
		hInstance,
		lpParam);

	// TODO: implement

	return (HWND)1;
}

WINUSERAPI HWND WINAPI CreateWindowExW(IN DWORD dwExStyle, IN LPCWSTR lpClassName, IN LPCWSTR lpWindowName, IN DWORD dwStyle, IN int X, IN int Y, IN int nWidth, IN int nHeight, IN HWND hWndParent, IN HMENU hMenu, IN HINSTANCE hInstance, IN LPVOID lpParam)
{
	// TODO: implement

	return NULL;
}

WINUSERAPI
#ifndef _MAC
LRESULT
WINAPI
#else
LRESULT
CALLBACK
#endif
DefWindowProcA(IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	if (Msg == WM_QUERYENDSESSION) {
		exit(0);
	}

	return 0;
}

WINUSERAPI
#ifndef _MAC
LRESULT
WINAPI
#else
LRESULT
CALLBACK
#endif
DefWindowProcW(IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return 0;
}

WINUSERAPI BOOL WINAPI DestroyWindow(IN HWND hWnd)
{
	// TODO: implement
	return TRUE;
}

WINUSERAPI HWND WINAPI FindWindowA(IN LPCSTR lpClassName, IN LPCSTR lpWindowName)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI FindWindowW(IN LPCWSTR lpClassName, IN LPCWSTR lpWindowName)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI GetFocus(VOID)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI GetDesktopWindow(VOID)
{
	// TODO: verify
	return HWND_DESKTOP;
}

WINUSERAPI HWND WINAPI GetForegroundWindow(VOID)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI GetLastActivePopup(IN HWND hWnd)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI GetParent(IN HWND hWnd)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HANDLE WINAPI GetPropA(IN HWND hWnd, IN LPCSTR lpString)
{
	std::map<LPCSTR, HANDLE>::iterator it;

	// TODO: proper implementation
	if (((uintptr_t)lpString & ~0xFFFF) == 0) {
		// it's an atom, translate to string
	}

	it = _wndProps[hWnd].find(lpString);

	if (it != _wndProps[hWnd].end()) {
		return it->second;
	}

	return NULL;
}

WINUSERAPI HANDLE WINAPI GetPropW(IN HWND hWnd, IN LPCWSTR lpString)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI GetTopWindow(IN HWND hWnd)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI GetWindow(IN HWND hWnd, IN UINT uCmd)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI LONG WINAPI GetWindowLongA(IN HWND hWnd, IN int nIndex)
{
	// TODO: implement
	return 0;
}

WINUSERAPI LONG WINAPI GetWindowLongW(IN HWND hWnd, IN int nIndex)
{
	// TODO: implement
	return 0;
}

WINUSERAPI BOOL WINAPI GetWindowRect(IN HWND hWnd, OUT LPRECT lpRect)
{
	// TODO: implement properly
	lpRect->bottom = 480;
	lpRect->left = 0;
	lpRect->right = 640;
	lpRect->top = 0;

	return TRUE;
}

WINUSERAPI int WINAPI GetWindowTextA(IN HWND hWnd, OUT LPSTR lpString, IN int nMaxCount)
{
	// TODO: implement
	return 0;
}

WINUSERAPI int WINAPI GetWindowTextW(IN HWND hWnd, OUT LPWSTR lpString, IN int nMaxCount)
{
	// TODO: implement
	return 0;
}

WINUSERAPI BOOL WINAPI IsWindowEnabled(IN HWND hWnd)
{
	// TODO: implement
	return TRUE;
}

WINUSERAPI BOOL WINAPI IsWindowVisible(IN HWND hWnd)
{
	// TODO: implement
	return TRUE;
}

WINUSERAPI ATOM WINAPI RegisterClassA(IN CONST WNDCLASSA *lpWndClass)
{
	DebugPrintf(
		"RegisterClassA(%p)\n    WNDCLASSA.lpszClassName: %s\n    WNDCLASSA.style: 0x%X\n    WNDCLASSA.lpfnWndProc: %p\n",
		lpWndClass,
		lpWndClass->lpszClassName,
		lpWndClass->style,
		lpWndClass->lpfnWndProc);

	// TODO: implement
	return 0;
}

WINUSERAPI ATOM WINAPI RegisterClassW(IN CONST WNDCLASSW *lpWndClass)
{
	// TODO: implement
	return 0;
}

WINUSERAPI BOOL WINAPI ReleaseCapture(VOID)
{
	// TODO: implement
	return TRUE;
}

WINUSERAPI HANDLE WINAPI RemovePropA(IN HWND hWnd, IN LPCSTR lpString)
{
	std::map<LPCSTR, HANDLE>::iterator it;

	// TODO: proper implementation
	if (((uintptr_t)lpString & ~0xFFFF) == 0) {
		// it's an atom, translate to string
	}

	it = _wndProps[hWnd].find(lpString);

	if (it != _wndProps[hWnd].end()) {
		HANDLE val = it->second;

		_wndProps[hWnd].erase(it);

		return val;
	}

	return NULL;
}

WINUSERAPI HANDLE WINAPI RemovePropW(IN HWND hWnd, IN LPCWSTR lpString)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI SetActiveWindow(IN HWND hWnd)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI HWND WINAPI SetCapture(IN HWND hWnd)
{
	// TODO: implement
	return NULL;
}

WINUSERAPI BOOL WINAPI SetDlgItemTextA(IN HWND hDlg, IN int nIDDlgItem, IN LPCSTR lpString)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI BOOL WINAPI SetDlgItemTextW(IN HWND hDlg, IN int nIDDlgItem, IN LPCWSTR lpString)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI HWND WINAPI SetFocus(IN HWND hWnd)
{
	_currentWndProc(hWnd, WM_ACTIVATEAPP, TRUE, 0);

	return hWnd;
}

WINUSERAPI BOOL WINAPI SetForegroundWindow(IN HWND hWnd)
{
	// TODO: implement
	return TRUE;
}

WINUSERAPI BOOL WINAPI SetPropA(IN HWND hWnd, IN LPCSTR lpString, IN HANDLE hData)
{
	// TODO: proper implementation
	if (((uintptr_t)lpString & ~0xFFFF) == 0) {
		// it's an atom, translate to string
	}

	_wndProps[hWnd][lpString] = hData;

	return TRUE;
}

WINUSERAPI BOOL WINAPI SetPropW(IN HWND hWnd, IN LPCWSTR lpString, IN HANDLE hData)
{
	return SetLastError(ERROR_NOT_SUPPORTED), FALSE;
}

WINUSERAPI LONG WINAPI SetWindowLongA(IN HWND hWnd, IN int nIndex, IN LONG dwNewLong)
{
	// TODO: implement
	return 0;
}

WINUSERAPI LONG WINAPI SetWindowLongW(IN HWND hWnd, IN int nIndex, IN LONG dwNewLong)
{
	// TODO: implement
	return 0;
}

WINUSERAPI BOOL WINAPI SetWindowPos(IN HWND hWnd, IN HWND hWndInsertAfter, IN int X, IN int Y, IN int cx, IN int cy, IN UINT uFlags)
{
	DebugPrintf(
		"SetWindowPos(0x%p, 0x%p, %i, %i, %i, %i, %u)\n",
		hWnd,
		hWndInsertAfter,
		X,
		Y,
		cx,
		cy,
		uFlags);

	return TRUE;
}

WINUSERAPI BOOL WINAPI SetWindowTextA(IN HWND hWnd, IN LPCSTR lpString)
{
	return TRUE;
}

WINUSERAPI BOOL WINAPI SetWindowTextW(IN HWND hWnd, IN LPCWSTR lpString)
{
	return TRUE;
}

WINUSERAPI BOOL WINAPI ShowWindow(IN HWND hWnd, IN int nCmdShow)
{
	DebugPrintf("ShowWindow(0x%p, 0x%X)\n", hWnd, nCmdShow);

	if (nCmdShow == SW_HIDE) {
		_currentWndProc(hWnd, WM_SHOWWINDOW, FALSE, 0);
	} else if (nCmdShow == SW_SHOWNORMAL) {
		_currentWndProc(hWnd, WM_SHOWWINDOW, TRUE, 0);
	}

	return TRUE;
}

WINUSERAPI BOOL WINAPI UpdateWindow(IN HWND hWnd)
{
	DebugPrintf("UpdateWindow(0x%p)\n", hWnd);

	return TRUE;
}
