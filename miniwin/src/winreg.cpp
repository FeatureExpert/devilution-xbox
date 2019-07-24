#include <Windows.h>

WINADVAPI LONG APIENTRY RegCloseKey(IN HKEY hKey)
{
	// TODO: implement
	return 0;
}

WINADVAPI LONG APIENTRY RegOpenKeyExA(IN HKEY hKey, IN LPCSTR lpSubKey, IN DWORD ulOptions, IN REGSAM samDesired, OUT PHKEY phkResult)
{
	// TODO: implement
	return 0;
}

WINADVAPI LONG APIENTRY RegOpenKeyExW(IN HKEY hKey, IN LPCWSTR lpSubKey, IN DWORD ulOptions, IN REGSAM samDesired, OUT PHKEY phkResult)
{
	// TODO: implement
	return 0;
}

WINADVAPI LONG APIENTRY RegQueryValueExA(IN HKEY hKey, IN LPCSTR lpValueName, IN LPDWORD lpReserved, OUT LPDWORD lpType, IN OUT LPBYTE lpData, IN OUT LPDWORD lpcbData)
{
	// TODO: implement
	return 0;
}

WINADVAPI LONG APIENTRY RegQueryValueExW(IN HKEY hKey, IN LPCWSTR lpValueName, IN LPDWORD lpReserved, OUT LPDWORD lpType, IN OUT LPBYTE lpData, IN OUT LPDWORD lpcbData)
{
	// TODO: implement
	return 0;
}

WINADVAPI LONG APIENTRY RegSetValueExA(IN HKEY hKey, IN LPCSTR lpValueName, IN DWORD Reserved, IN DWORD dwType, IN CONST BYTE* lpData, IN DWORD cbData)
{
	// TODO: implement
	return 0;
}

WINADVAPI LONG APIENTRY RegSetValueExW(IN HKEY hKey, IN LPCWSTR lpValueName, IN DWORD Reserved, IN DWORD dwType, IN CONST BYTE* lpData, IN DWORD cbData)
{
	// TODO: implement
	return 0;
}
