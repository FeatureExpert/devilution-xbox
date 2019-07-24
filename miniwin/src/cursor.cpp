#define _USER32_
#include <Windows.h>

static HCURSOR _currentCursor;
static int _cursorCounter;

WINUSERAPI HCURSOR WINAPI SetCursor(IN HCURSOR hCursor)
{
	HCURSOR oldCursor = _currentCursor;

	_currentCursor = hCursor;

	return oldCursor;
}

WINUSERAPI BOOL WINAPI SetCursorPos(IN int X, IN int Y)
{
	// TODO: implement
	return TRUE;
}

WINUSERAPI int WINAPI ShowCursor(IN BOOL bShow)
{
	return bShow ? ++_cursorCounter : --_cursorCounter;
}
