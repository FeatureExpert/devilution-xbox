#include "storm.h"
#include <stdio.h>

const char * aSoftwareBlizza = "Software\\Blizzard Entertainment\\";
const char * aSoftwareBattle = "Software\\Battle.net\\D1\\";

//@implemented
BOOL STORMAPI SRegGetBaseKey(int type, char * dest, int max_length)
{
	if (dest == NULL || max_length == 0) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	if (type != 2) {
		SStrCopy(dest, aSoftwareBattle, max_length);

		return TRUE;
	}

	SStrCopy(dest, aSoftwareBlizza, max_length);

	return TRUE;
}

BOOL STORMAPI SRegLoadData(const char *keyname, const char *valuename, int size, LPBYTE lpData, BYTE flags, LPDWORD lpcbData)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SRegLoadString(const char *keyname, const char *valuename, BYTE flags, char *buffer, size_t buffersize)
{
	return GetPrivateProfileString(keyname, valuename, NULL, buffer, buffersize, "T:\\diablo.reg") > 0;
}

BOOL STORMAPI SRegLoadValue(const char *keyname, const char *valuename, BYTE flags, int *value)
{
	char string[10];

	if (!GetPrivateProfileString(keyname, valuename, NULL, string, 10, "T:\\diablo.reg")) {
		return FALSE;
	}

	*value = strtoul(string, NULL, 0);

	return TRUE;
}

BOOL STORMAPI SRegSaveData(const char *keyname, const char *valuename, int size, BYTE *lpData, DWORD cbData)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SRegSaveString(const char *keyname, const char *valuename, BYTE flags, char *string)
{
	WritePrivateProfileString(keyname, valuename, string, "T:\\diablo.reg");

	return TRUE;
}

BOOL STORMAPI SRegSaveValue(const char *keyname, const char *valuename, BYTE flags, DWORD result)
{
	if (keyname == NULL || *keyname == '\0' || valuename == NULL || *valuename == '\0') {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	char string[16];

	_snprintf(string, 16, "%u", result);

	WritePrivateProfileString(keyname, valuename, string, "T:\\diablo.reg");

	return TRUE;
}

BOOL STORMAPI SRegDeleteValue(const char *keyname, const char *valuename, BYTE flags)
{
	WritePrivateProfileString(keyname, valuename, NULL, "T:\\diablo.reg");

	return TRUE;
}
