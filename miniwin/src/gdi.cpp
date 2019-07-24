#define _GDI32_
#include <Windows.h>

static DWORD _batchLimit;
static COLORREF _backgroundColor;
PALETTEENTRY _palette[256];

WINGDIAPI HFONT WINAPI CreateFontA(IN int cHeight, IN int cWidth, IN int cEscapement, IN int cOrientation, IN int cWeight,
								   IN DWORD bItalic, IN DWORD bUnderline, IN DWORD bStrikeOut, IN DWORD iCharSet, IN DWORD iOutPrecision,
								   IN DWORD iClipPrecision, IN DWORD iQuality, IN DWORD iPitchAndFamily, IN LPCSTR pszFaceName)
{
	// TODO: implement
	return NULL;
}

WINGDIAPI HFONT WINAPI CreateFontIndirectA(IN CONST LOGFONTA *lplf)
{
	// TODO: implement
	return NULL;
}

WINGDIAPI HFONT WINAPI CreateFontIndirectW(IN CONST LOGFONTW *lplf)
{
	// TODO: implement
	return NULL;
}

WINGDIAPI HFONT WINAPI CreateFontW(IN int cHeight, IN int cWidth, IN int cEscapement, IN int cOrientation, IN int cWeight,
								   IN DWORD bItalic, IN DWORD bUnderline, IN DWORD bStrikeOut, IN DWORD iCharSet, IN DWORD iOutPrecision,
								   IN DWORD iClipPrecision, IN DWORD iQuality, IN DWORD iPitchAndFamily, IN LPCWSTR pszFaceName)
{
	// TODO: implement
	return NULL;
}

WINGDIAPI HPALETTE WINAPI CreatePalette(IN CONST LOGPALETTE * plpal)
{
	// TODO: implement
	return NULL;
}

WINGDIAPI BOOL WINAPI DeleteObject(IN HGDIOBJ ho)
{
	// TODO: implement
	return TRUE;
}

WINGDIAPI BOOL WINAPI ExtTextOutA(IN HDC hdc, IN int x, IN int y, IN UINT options, IN CONST RECT * lprect, IN LPCSTR lpString, IN UINT c, IN CONST INT *lpDx)
{
	// TODO: implement
	return FALSE;
}

WINGDIAPI BOOL WINAPI ExtTextOutW(IN HDC hdc, IN int x, IN int y, IN UINT options, IN CONST RECT * lprect, IN LPCWSTR lpString, IN UINT c, IN CONST INT *lpDx)
{
	// TODO: implement
	return FALSE;
}

WINGDIAPI DWORD WINAPI GdiSetBatchLimit(IN DWORD dw)
{
	DWORD oldBatchLimit = _batchLimit;

	_batchLimit = dw;

	return oldBatchLimit;
}

WINGDIAPI BOOL WINAPI GetCurrentPositionEx(IN HDC hdc, OUT LPPOINT lppt)
{
	// TODO: implement
	return FALSE;
}

WINGDIAPI int WINAPI GetDeviceCaps(IN HDC hdc, IN int index)
{
	// TODO: implement
	return 0;
}

WINGDIAPI int WINAPI GetObjectA(IN HGDIOBJ h, IN int c, OUT LPVOID pv)
{
	// TODO: implement
	return 0;
}

WINGDIAPI int WINAPI GetObjectW(IN HGDIOBJ h, IN int c, OUT LPVOID pv)
{
	// TODO: implement
	return 0;
}

WINGDIAPI UINT WINAPI GetPaletteEntries(IN HPALETTE hpal, IN UINT iStart, IN UINT cEntries, OUT LPPALETTEENTRY pPalEntries)
{
	if (iStart + cEntries > 256) {
		SetLastError(ERROR_INVALID_PARAMETER);

		return 0;
	}

	// TODO: implement
	return 0;
}

WINGDIAPI HGDIOBJ WINAPI GetStockObject(IN int i)
{
	// TODO: implement
	return NULL;
}

WINGDIAPI UINT WINAPI GetSystemPaletteEntries(IN HDC hdc, IN UINT iStart, IN UINT cEntries, OUT LPPALETTEENTRY pPalEntries)
{
	if (iStart + cEntries > 256) {
		SetLastError(ERROR_INVALID_PARAMETER);

		return 0;
	}

	// TODO: proper implementation
	if (pPalEntries == NULL) {
		return 256;
	}

	memcpy(pPalEntries, &_palette[iStart], cEntries);

	return cEntries;
}

WINGDIAPI BOOL WINAPI GetTextMetricsA(IN HDC hdc, OUT LPTEXTMETRICA lptm)
{
	// TODO: implement
	return TRUE;
}

WINGDIAPI BOOL WINAPI GetTextMetricsW(IN HDC hdc, OUT LPTEXTMETRICW lptm)
{
	// TODO: implement
	return TRUE;
}

WINGDIAPI BOOL WINAPI MoveToEx(IN HDC hdc, IN int x, IN int y, OUT LPPOINT lppt)
{
	// TODO: implement
	return TRUE;
}

WINGDIAPI HGDIOBJ WINAPI SelectObject(IN HDC, IN HGDIOBJ)
{
	// TODO: implement
	return NULL;
}

WINGDIAPI COLORREF WINAPI SetBkColor(IN HDC hdc, IN COLORREF color)
{
	COLORREF oldColor = _backgroundColor;

	_backgroundColor = color;

	return oldColor;
}

WINGDIAPI UINT WINAPI SetTextAlign(IN HDC hdc, IN UINT align)
{
	return TA_LEFT;
}

WINGDIAPI COLORREF WINAPI SetTextColor(IN HDC hdc, IN COLORREF color)
{
	//COLORREF oldColor = hdc->color;
	//hdc->color = color;
	//return oldColor;
	return CLR_INVALID;
}

WINGDIAPI BOOL WINAPI TextOutA(IN HDC hdc, IN int x, IN int y, IN LPCSTR lpString, IN int c)
{
	// TODO: implement
	return TRUE;
}

WINGDIAPI BOOL WINAPI TextOutW(IN HDC hdc, IN int x, IN int y, IN LPCWSTR lpString, IN int c)
{
	// TODO: implement
	return TRUE;
}
