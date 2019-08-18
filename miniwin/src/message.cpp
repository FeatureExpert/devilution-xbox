#define _USER32_
#include <Windows.h>

#include <assert.h>
#include <deque>

static std::deque<MSG> message_queue;

extern WNDPROC _currentWndProc;

extern void DebugPrintf(const char * fmt, ...);

WINUSERAPI LRESULT WINAPI DispatchMessageA(IN CONST MSG *lpMsg)
{
	WNDPROC proc = _currentWndProc;

	if (lpMsg->message == WM_TIMER) {
		proc = (WNDPROC)lpMsg->lParam;
	}

	return proc(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
}

WINUSERAPI LRESULT WINAPI DispatchMessageW(IN CONST MSG *lpMsg)
{
	WNDPROC proc = _currentWndProc;

	if (lpMsg->message == WM_TIMER) {
		proc = (WNDPROC)lpMsg->lParam;
	}

	return proc(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
}

WINUSERAPI BOOL WINAPI GetMessageA(OUT LPMSG lpMsg, IN HWND hWnd, IN UINT wMsgFilterMin, IN UINT wMsgFilterMax)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI BOOL WINAPI GetMessageW(OUT LPMSG lpMsg, IN HWND hWnd, IN UINT wMsgFilterMin, IN UINT wMsgFilterMax)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI BOOL WINAPI PeekMessageA(OUT LPMSG lpMsg, IN HWND hWnd, IN UINT wMsgFilterMin, IN UINT wMsgFilterMax, IN UINT wRemoveMsg)
{
	if (wRemoveMsg == PM_NOREMOVE) {
		// This does not actually fill out lpMsg, but this is ok
		// since the engine never uses it in this case
		return !message_queue.empty() /*|| SDL_PollEvent(NULL)*/;
	}

	if (!message_queue.empty()) {
		*lpMsg = message_queue.front();
		message_queue.pop_front();
		return TRUE;
	}

	// TODO: implement
	return FALSE;
}

WINUSERAPI BOOL WINAPI PeekMessageW(OUT LPMSG lpMsg, IN HWND hWnd, IN UINT wMsgFilterMin, IN UINT wMsgFilterMax, IN UINT wRemoveMsg)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI BOOL WINAPI PostMessageA(IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	assert(hWnd == 0);

	MSG msg;
	msg.hwnd = hWnd;
	msg.message = Msg;
	msg.wParam = wParam;
	msg.lParam = lParam;

	message_queue.push_back(msg);

	return TRUE;
}

WINUSERAPI BOOL WINAPI PostMessageW(IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI BOOL WINAPI PostThreadMessageA(IN DWORD idThread, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI BOOL WINAPI PostThreadMessageW(IN DWORD idThread, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI BOOL WINAPI TranslateMessage(IN CONST MSG *lpMsg)
{
	assert(lpMsg->hwnd == 0);

	if (lpMsg->message == WM_KEYDOWN) {
		int key = lpMsg->wParam;
		unsigned mod = (DWORD)lpMsg->lParam >> 16;

		bool shift = (mod & MOD_SHIFT) != 0;
		bool upper = shift /*!= (mod & MOD_CAPS)*/; // caps lock is currently not supported

		bool is_alpha = (key >= 'A' && key <= 'Z');
		bool is_numeric = (key >= '0' && key <= '9');
		bool is_control = key == VK_SPACE || key == VK_BACK || key == VK_ESCAPE || key == VK_TAB || key == VK_RETURN;
		bool is_oem = (key >= VK_OEM_1 && key <= VK_OEM_7);

		if (is_control || is_alpha || is_numeric || is_oem) {
			if (!upper && is_alpha) {
				key = tolower(key);
			} else if (shift && is_numeric) {
				key = key == '0' ? ')' : key - 0x10;
			} else if (is_oem) {
				// XXX: This probably only supports US keyboard layout
				switch (key) {
				case VK_OEM_1:
					key = shift ? ':' : ';';
					break;
				case VK_OEM_2:
					key = shift ? '?' : '/';
					break;
				case VK_OEM_3:
					key = shift ? '~' : '`';
					break;
				case VK_OEM_4:
					key = shift ? '{' : '[';
					break;
				case VK_OEM_5:
					key = shift ? '|' : '\\';
					break;
				case VK_OEM_6:
					key = shift ? '}' : ']';
					break;
				case VK_OEM_7:
					key = shift ? '"' : '\'';
					break;

				case VK_OEM_MINUS:
					key = shift ? '_' : '-';
					break;
				case VK_OEM_PLUS:
					key = shift ? '+' : '=';
					break;
				case VK_OEM_PERIOD:
					key = shift ? '>' : '.';
					break;
				case VK_OEM_COMMA:
					key = shift ? '<' : ',';
					break;

				default:
					//UNIMPLEMENTED();
					break;
				}
			}

			if (key >= 32) {
				DebugPrintf("char: %c", key);
			}

			// XXX: This does not add extended info to lParam
			PostMessageA(lpMsg->hwnd, WM_CHAR, key, 0);
		}
	}

	return TRUE;
}
