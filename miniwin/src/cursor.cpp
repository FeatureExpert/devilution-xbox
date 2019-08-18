#define _USER32_
#include <Windows.h>

extern void DebugPrintf(const char * fmt, ...);

static HCURSOR _currentCursor;
static int _cursorCounter;

WINUSERAPI HCURSOR WINAPI SetCursor(IN HCURSOR hCursor)
{
	DebugPrintf("SetCursor(0x%p)\n", hCursor);

	HCURSOR oldCursor = _currentCursor;

	_currentCursor = hCursor;

	return oldCursor;
}

WINUSERAPI BOOL WINAPI SetCursorPos(IN int X, IN int Y)
{
	DebugPrintf("SetCursorPos(%i, %i)\n", X, Y);

	// TODO: implement
	return TRUE;
}

WINUSERAPI int WINAPI ShowCursor(IN BOOL bShow)
{
	DebugPrintf("ShowCursor(%s)\n", bShow ? "TRUE" : "FALSE");

	return bShow ? ++_cursorCounter : --_cursorCounter;
}
