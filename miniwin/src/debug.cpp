#include <xtl.h>

#include <stdio.h>

void DebugPrintf(const char * fmt, ...)
{
	va_list args;
	static char buffer[MAX_PATH];

	va_start(args, fmt);

	_vsnprintf(buffer, MAX_PATH, fmt, args);

	OutputDebugString(buffer);

#if !NDEBUG
	FILE * fp = fopen("D:\\debug.txt", "a+");

	if (fp == NULL) {
		return;
	}

	fprintf(fp, buffer);
	fclose(fp);
#endif
}
