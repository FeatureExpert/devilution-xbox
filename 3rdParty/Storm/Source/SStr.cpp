#include "storm.h"
#include <stdio.h>

//@implemented
int STORMAPI SStrToInt(const char *string)
{
	if (string == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	int value = 0;
	int sign = 1;

	if (*string == '-') {
		sign = -1;
		string++;
	}

	while (isdigit(*string)) {
		value *= 10;
		value += (int)(*string - '0');
		string++;
	}

	return value * sign;
}

char* STORMAPI SStrChr(const char *string, char c)
{
	char current = *string;

	if (string == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return NULL;
	}

	while (current != '\0') {
		if (current == c) {
			break;
		}

		current = string[1];
		string++;
	}

	return const_cast<char *>(string);
}

char* STORMAPI SStrChrR(const char *string, char c)
{
	char current = *string;
	char * p = NULL;

	if (string == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return NULL;
	}

	while (current != '\0') {
		if (current == c) {
			p = const_cast<char *>(string);
		}

		current = string[1];
		string++;
	}

	return p;
}

//@implemented
char* STORMAPI SStrPack(const char *str, char c, int type)
{
	if (str == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return 0;
	}

	if (type != 0) {
		return SStrChrR(str, c);
	}

	return SStrChr(str, c);
}

//@implemented
int STORMAPI SStrCmp(const char *string1, const char *string2, size_t size)
{
	if (string1 == NULL || string2 == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return string1 - string2;
	}

	return strncmp(string1, string2, size);
}

//@implemented
int STORMAPI SStrCmpI(const char *string1, const char *string2, size_t size)
{
	if (string1 == NULL || string2 == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return string1 - string2;
	}

	return _strnicmp(string1, string2, size);
}

int STORMAPI SStrCopy(char *dest, const char *src, int max_length)
{
	if (src == NULL || dest == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return 0;
	}

	if (max_length == 0) {
		return 0;
	}

	// TODO: implement
}

//@implemented
char* STORMAPI SStrDup(const char *string)
{
	if (string == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return NULL;
	}

	int length = SStrLen(string);
	char * dup = (char *)SMemAlloc(length + 1, "P:\\Projects\\Storm\\Storm-SWAR\\Source\\SStr.cpp", 628, 0);

	SMemCopy(dup, string, length);

	return dup;
}

int STORMAPI SStrLen(const char* string)
{
	if (string == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return 0;
	}

	// TODO: implement
}

int STORMAPI SStrNCat(char *dest, const char *src, int max_length)
{
	if (dest == NULL || src == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return 0;
	}

	if (max_length == 0) {
		return 0;
	}

	// TODO: implement

	strncat(dest, src, max_length);
}

//@implemented
size_t SStrVPrintf(char *dest, size_t size, const char *format, ...)
{
	if (format == NULL || dest == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return 0;
	}

	if (size == 0) {
		return 0;
	}

	va_list ap;
	va_start(ap, format);
	int count = _vsnprintf(dest, size, format, ap);

	if (count == 0) {
		dest[0] = '\0';
	}

	return count;
}

DWORD STORMAPI SStrHash(const char *string, DWORD flags, DWORD Seed)
{
	if (string == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return 0;
	}

	if (Seed == 0) {
		Seed = 0x7FED7FED;
	}

	if (flags != STORM_HASH_ABSOLUTE) {
		// TODO: implement
	}

	// TODO: implement

	return 1;
}

//@implemented
char* STORMAPI SStrUpper(char* string)
{
	return _strupr(string);
}

//@implemented
char* STORMAPI SStrLower(char* string)
{
	return _strlwr(string);
}
