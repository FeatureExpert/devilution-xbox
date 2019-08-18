#define _ADVAPI32_
#define _USER32_
#include <Windows.h>
#include <stdio.h>
#include <ShlObj.h>
#include <ShellAPI.h>

#include <assert.h>

extern LD_LAUNCH_DASHBOARD launchDashboard;
extern void DebugPrintf(const char * fmt, ...);

#define MAX_LINE_LENGTH    80
static BOOL read_line(FILE * fp, char *bp);

static inline void __cpuid(int CPUInfo[], const int InfoType)
{
	__asm {
		mov 	eax, InfoType
		cpuid
		mov 	CPUInfo[0], eax
		mov 	CPUInfo[1], ebx
		mov 	CPUInfo[2], ecx
		mov 	CPUInfo[3], edx
	}
}

uintptr_t _beginthreadex(void * security, unsigned stack_size, unsigned (__stdcall *start_address)(void *), void * arglist, unsigned initflag, unsigned * thrdaddr)
{
	return (uintptr_t)CreateThread((LPSECURITY_ATTRIBUTES)security, stack_size, (LPTHREAD_START_ROUTINE)start_address, arglist, initflag, (LPDWORD)thrdaddr);
}

WINBASEAPI HANDLE WINAPI CreateFileMappingA(IN HANDLE hFile, IN LPSECURITY_ATTRIBUTES lpFileMappingAttributes, IN DWORD flProtect, IN DWORD dwMaximumSizeHigh, IN DWORD dwMaximumSizeLow, IN LPCSTR lpName)
{
	// TODO: implement
	return NULL;
}

WINBASEAPI HANDLE WINAPI CreateFileMappingW(IN HANDLE hFile, IN LPSECURITY_ATTRIBUTES lpFileMappingAttributes, IN DWORD flProtect, IN DWORD dwMaximumSizeHigh, IN DWORD dwMaximumSizeLow, IN LPCWSTR lpName)
{
	// TODO: implement
	return NULL;
}

WINBASEAPI BOOL WINAPI CreateProcessA(IN LPCSTR lpApplicationName, IN LPSTR lpCommandLine, IN LPSECURITY_ATTRIBUTES lpProcessAttributes, IN LPSECURITY_ATTRIBUTES lpThreadAttributes, IN BOOL bInheritHandles, IN DWORD dwCreationFlags, IN LPVOID lpEnvironment, IN LPCSTR lpCurrentDirectory, IN LPSTARTUPINFOA lpStartupInfo, OUT LPPROCESS_INFORMATION lpProcessInformation)
{
	// TODO: implement, if possible
	return FALSE;
}

WINBASEAPI BOOL WINAPI CreateProcessW(IN LPCWSTR lpApplicationName, IN LPWSTR lpCommandLine, IN LPSECURITY_ATTRIBUTES lpProcessAttributes, IN LPSECURITY_ATTRIBUTES lpThreadAttributes, IN BOOL bInheritHandles, IN DWORD dwCreationFlags, IN LPVOID lpEnvironment, IN LPCWSTR lpCurrentDirectory, IN LPSTARTUPINFOW lpStartupInfo, OUT LPPROCESS_INFORMATION lpProcessInformation)
{
	// TODO: implement, if possible
	return FALSE;
}

WINBASEAPI DECLSPEC_NORETURN VOID WINAPI ExitProcess(IN UINT uExitCode)
{
	// return to the dashboard
	launchDashboard.dwReason = XLD_LAUNCH_DASHBOARD_MAIN_MENU;
	XLaunchNewImage(NULL, (PLAUNCH_DATA)&launchDashboard);
}

WINBASEAPI HRSRC WINAPI FindResourceA(IN HMODULE hModule, IN LPCSTR lpName, IN LPCSTR lpType)
{
	// TODO: implement
	return NULL;
}

WINBASEAPI HRSRC WINAPI FindResourceW(IN HMODULE hModule, IN LPCWSTR lpName, IN LPCWSTR lpType)
{
	// TODO: implement
	return NULL;
}

WINBASEAPI DWORD WINAPI FormatMessageA(IN DWORD dwFlags, IN LPCVOID lpSource, IN DWORD dwMessageId, IN DWORD dwLanguageId, OUT LPSTR lpBuffer, IN DWORD nSize, IN va_list *Arguments)
{
	// TODO: implement
	return 0;
}

WINBASEAPI DWORD WINAPI FormatMessageW(IN DWORD dwFlags, IN LPCVOID lpSource, IN DWORD dwMessageId, IN DWORD dwLanguageId, OUT LPWSTR lpBuffer, IN DWORD nSize, IN va_list *Arguments)
{
	// TODO: implement
	return 0;
}

WINBASEAPI BOOL WINAPI FreeResource(IN HGLOBAL hResData)
{
	// TODO: implement
	return TRUE;
}

WINBASEAPI BOOL WINAPI GetComputerNameA(OUT LPSTR lpBuffer, IN OUT LPDWORD nSize)
{
	// TODO: implement
	return FALSE;
}

WINBASEAPI BOOL WINAPI GetComputerNameW(OUT LPWSTR lpBuffer, IN OUT LPDWORD nSize)
{
	// TODO: implement
	return FALSE;
}

WINBASEAPI DWORD WINAPI GetCurrentDirectoryA(IN DWORD nBufferLength, OUT LPSTR lpBuffer)
{
	DWORD copyLen = nBufferLength > 2 ? 2 : nBufferLength;

	if (copyLen == 0) {
		return 3;
	}

	strncpy(lpBuffer, "D:", copyLen);
	lpBuffer[2] = '\0';

	if (copyLen < 2) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);

		return 3;
	} else {
		SetLastError(ERROR_SUCCESS);
	}

	return copyLen;
}

WINBASEAPI DWORD WINAPI GetCurrentDirectoryW(IN DWORD nBufferLength, OUT LPWSTR lpBuffer)
{
	DWORD copyLen = nBufferLength > 3 ? 3 : nBufferLength;

	if (copyLen == 0) {
		return 3;
	}

	wcsncpy(lpBuffer, L"D:\\", copyLen);

	if (copyLen < 3) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
	} else {
		SetLastError(ERROR_SUCCESS);
	}

	return copyLen;
}

WINBASEAPI DWORD WINAPI GetCurrentProcessId(VOID)
{
	// There's only ever a single process on Xbox, so simply return 0
	return 0;
}

WINBASEAPI UINT WINAPI GetDriveTypeA(IN LPCSTR lpRootPathName)
{
	if (lpRootPathName == NULL || lpRootPathName[0] == 'D') {
		// Current drive is always the DVD drive
		return DRIVE_CDROM;
	}

	// T:, U: and Z: are always present
	if (lpRootPathName[0] == 'T' || lpRootPathName[0] == 'U' || lpRootPathName[0] == 'Z') {
		return DRIVE_FIXED;
	}

	// TODO: reliably determine remaining mappings
	return DRIVE_UNKNOWN;
}

WINBASEAPI UINT WINAPI GetDriveTypeW(IN LPCWSTR lpRootPathName)
{
	if (lpRootPathName == NULL || lpRootPathName[0] == 'D') {
		// Current drive is always the DVD drive
		return DRIVE_CDROM;
	}

	// T:, U: and Z: are always present
	if (lpRootPathName[0] == 'T' || lpRootPathName[0] == 'U' || lpRootPathName[0] == 'Z') {
		return DRIVE_FIXED;
	}

	// TODO: reliably determine remaining mappings
	return DRIVE_UNKNOWN;
}

BOOL APIENTRY GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
	// TODO: implement
	return FALSE;
}

DWORD APIENTRY GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle)
{
	// TODO: implement
	return 0;
}

DWORD APIENTRY GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle)
{
	// TODO: implement
	return 0;
}

WINBASEAPI DWORD WINAPI GetLogicalDriveStringsA(IN DWORD nBufferLength, OUT LPSTR lpBuffer)
{
	DWORD copyLen = nBufferLength > 16 ? 16 : nBufferLength;

	// Fixed mappings at
	// D:\ (DVD drive or current working dir, depending on retail vs dev. i.e. a game's executable is always present at D:\<name>.xbe)
	// T:\ (Persistent data region)
	// U:\ (User data region, not mapped directly)
	// Z:\ (Utility partition, 750MB of scratch space)
	// dynamic mappings are between F:\ and R:\ and could be, for example, memory units

	// TODO: return a more reliable list

	if (copyLen == 0) {
		return 16;
	}

	strncpy(lpBuffer, "D:\\\0T:\\\0U:\\\0Z:\\\0\0", copyLen);

	if (copyLen < 16) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
	} else {
		SetLastError(ERROR_SUCCESS);
	}

	return copyLen;
}

WINBASEAPI DWORD WINAPI GetLogicalDriveStringsW(IN DWORD nBufferLength, OUT LPWSTR lpBuffer)
{
	DWORD copyLen = nBufferLength > 16 ? 16 : nBufferLength;

	// Fixed mappings at
	// D:\ (DVD drive or current working dir, depending on retail vs dev. i.e. a game's executable is always present at D:\<name>.xbe)
	// T:\ (Persistent data region)
	// U:\ (User data region, not mapped directly)
	// Z:\ (Utility partition, 750MB of scratch space)
	// dynamic mappings are between F:\ and R:\ and could be, for example, memory units

	// TODO: return a more reliable list

	if (copyLen == 0) {
		return 16;
	}

	wcsncpy(lpBuffer, L"D:\\\0T:\\\0U:\\\0Z:\\\0\0", copyLen);

	if (copyLen < 16) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
	} else {
		SetLastError(ERROR_SUCCESS);
	}

	return copyLen;
}

WINBASEAPI DWORD WINAPI GetModuleFileNameA(IN HMODULE hModule, OUT LPSTR lpFilename, IN DWORD nSize)
{
	DebugPrintf("GetModuleFileNameA(0x%p, 0x%p, %u)\n", hModule, lpFilename, nSize);

	DWORD result;

	if (hModule == NULL) {
		result = XCreateSaveGame("U:\\", L"Diablo", OPEN_ALWAYS, 0, lpFilename, nSize);

		if (result == ERROR_SUCCESS) {
			return nSize;
		}

		return SetLastError(result), 0;
	}

	return SetLastError(ERROR_NOT_SUPPORTED), 0;
}

WINBASEAPI DWORD WINAPI GetModuleFileNameW(IN HMODULE hModule, OUT LPWSTR lpFilename, IN DWORD nSize)
{
	// TODO: implement

	return SetLastError(ERROR_NOT_SUPPORTED), 0;
}

WINBASEAPI HMODULE WINAPI GetModuleHandleA(IN LPCSTR lpModuleName)
{
	DebugPrintf("GetModuleHandleA(\"%s\")\n", lpModuleName);

	// TODO: implement
	return NULL;
}

WINBASEAPI HMODULE WINAPI GetModuleHandleW(IN LPCWSTR lpModuleName)
{
	// TODO: implement
	return NULL;
}

WINBASEAPI DWORD WINAPI GetPrivateProfileStringA(IN LPCSTR lpAppName, IN LPCSTR lpKeyName, IN LPCSTR lpDefault, OUT LPSTR lpReturnedString, IN DWORD nSize, IN LPCSTR lpFileName)
{
	FILE *fp = fopen(lpFileName, "r");
	char buff[MAX_LINE_LENGTH];
	char *ep;
	char t_section[MAX_LINE_LENGTH];
	int len = strlen(lpKeyName);

	if (!fp) {
		return SetLastError(ERROR_FILE_NOT_FOUND), 0;
	}

	sprintf(t_section, "[%s]", lpAppName);    /* Format the section name */
	/* Move through file 1 line at a time until a section is matched or EOF */
	do {
		if (!read_line(fp, buff)) {
			fclose(fp);
			strncpy(lpReturnedString, lpDefault == NULL ? "" : lpDefault, nSize);
			lpReturnedString[nSize - 1] = '\0';
			return strlen(lpReturnedString);
		}
	} while (strcmp(buff, t_section));

	/* Now that the section has been found, find the entry.
	 * Stop searching upon leaving the section's area. */
	do {
		if (!read_line(fp, buff) || buff[0] == '\0') {
			fclose(fp);
			strncpy(lpReturnedString, lpDefault == NULL ? "" : lpDefault, nSize);
			lpReturnedString[nSize - 1] = '\0';
			return strlen(lpReturnedString);
		}
	} while (strncmp(buff, lpKeyName, len));

	ep = strrchr(buff, '=');    /* Parse out the equal sign */
	ep++;

	/* Copy up to buffer_len chars to buffer */
	strncpy(lpReturnedString, ep, nSize - 1);
	lpReturnedString[nSize - 1] = '\0';
	fclose(fp);                 /* Clean up and return the amount copied */
	return strlen(lpReturnedString);
}

WINBASEAPI DWORD WINAPI GetPrivateProfileStringW(IN LPCWSTR lpAppName, IN LPCWSTR lpKeyName, IN LPCWSTR lpDefault, OUT LPWSTR lpReturnedString, IN DWORD nSize, IN LPCWSTR lpFileName)
{
	// TODO: implement
	return 0;
}

WINBASEAPI VOID WINAPI GetSystemInfo(OUT LPSYSTEM_INFO lpSystemInfo)
{
	static SYSTEM_INFO XboxInfo;
	int tmp[4];

	XboxInfo.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
	XboxInfo.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
	XboxInfo.dwPageSize = 0x1000;
	XboxInfo.lpMinimumApplicationAddress = (LPVOID)0x10000;
	//XboxInfo.lpMaximumApplicationAddress
	XboxInfo.dwActiveProcessorMask = 1;
	XboxInfo.dwNumberOfProcessors = 1;
	//XboxInfo.wProcessorLevel

	__cpuid(tmp, 1);

	// calculate the model
	if (((tmp[0] >> 8) & 0xF) == 6) {
		XboxInfo.wProcessorRevision = ((tmp[0] >> 12) & 0xFF00) + ((tmp[0] & 0xF0) << 4);
	} else {
		XboxInfo.wProcessorRevision = ((tmp[0] << 4) & 0x0F00);
	}

	// add the stepping
	XboxInfo.wProcessorRevision |= (tmp[0] & 0xF);

	if (lpSystemInfo == NULL)
	{
		return;
	}

	memcpy(lpSystemInfo, &XboxInfo, sizeof(SYSTEM_INFO));
}

WINUSERAPI int WINAPI GetSystemMetrics(IN int nIndex)
{
	DebugPrintf("GetSystemMetrics(%i)\n", nIndex);

	switch(nIndex)
	{
	case SM_CMONITORS:
		return 1;
	case SM_CXSCREEN:
		return 640; // TODO: make this dependant on the selected video mode and standard
	case SM_CYSCREEN:
		return 480; // TODO: make this dependant on the selected video mode and standard
	}

	return 0;
}

WINADVAPI BOOL WINAPI GetUserNameA(OUT LPSTR lpBuffer, IN OUT LPDWORD nSize)
{
	// TODO: implement
	return FALSE;
}

WINADVAPI BOOL WINAPI GetUserNameW(OUT LPWSTR lpBuffer, IN OUT LPDWORD nSize)
{
	// TODO: implement
	return FALSE;
}

WINBASEAPI BOOL WINAPI GetVersionExA(IN OUT LPOSVERSIONINFOA lpVersionInformation)
{
	if (lpVersionInformation == NULL ||
		lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA) &&
		lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA)) {
		return SetLastError(ERROR_INVALID_PARAMETER), FALSE;
	}

	lpVersionInformation->dwMajorVersion = 5;
	lpVersionInformation->dwMinorVersion = 0;
	lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
	strcpy(lpVersionInformation->szCSDVersion, "Xbox"); // TODO: maybe add things like the kernel version?

	return TRUE;
}

WINBASEAPI BOOL WINAPI GetVersionExW(IN OUT LPOSVERSIONINFOW lpVersionInformation)
{
	if (lpVersionInformation == NULL ||
		lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOW) ||
		lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)) {
		return SetLastError(ERROR_INVALID_PARAMETER), FALSE;
	}

	lpVersionInformation->dwMajorVersion = 5;
	lpVersionInformation->dwMinorVersion = 0;
	lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
	wcscpy(lpVersionInformation->szCSDVersion, L"Xbox"); // TODO: maybe add things like the kernel version?

	return TRUE;
}

WINBASEAPI UINT WINAPI GetWindowsDirectoryA(OUT LPSTR lpBuffer, IN UINT uSize)
{
	if (lpBuffer == NULL) {
		return SetLastError(ERROR_INVALID_PARAMETER), 0;
	}

	if (uSize < 2) {
		return SetLastError(ERROR_INSUFFICIENT_BUFFER), 2;
	}

	strcpy(lpBuffer, "T:");

	return 2;
}

WINBASEAPI UINT WINAPI GetWindowsDirectoryW(OUT LPWSTR lpBuffer, IN UINT uSize)
{
	if (lpBuffer == NULL) {
		return SetLastError(ERROR_INVALID_PARAMETER), 0;
	}

	if (uSize < 2) {
		return SetLastError(ERROR_INSUFFICIENT_BUFFER), 2;
	}

	wcscpy(lpBuffer, L"T:");

	return 2;
}

WINUSERAPI DWORD WINAPI GetWindowThreadProcessId(IN HWND hWnd, OUT LPDWORD lpdwProcessId)
{
	// TODO: implement
	return 0;
}

WINBASEAPI HGLOBAL WINAPI LoadResource(IN HMODULE hModule, IN HRSRC hResInfo)
{
	// TODO: implement
	return NULL;
}

WINBASEAPI LPVOID WINAPI LockResource(IN HGLOBAL hResData)
{
	// TODO: implement
	return NULL;
}

WINBASEAPI LPVOID WINAPI MapViewOfFile(IN HANDLE hFileMappingObject, IN DWORD dwDesiredAccess, IN DWORD dwFileOffsetHigh, IN DWORD dwFileOffsetLow, IN SIZE_T dwNumberOfBytesToMap)
{
	// TODO: implement
	return NULL;
}

WINBASEAPI HFILE WINAPI OpenFile(IN LPCSTR lpFileName, OUT LPOFSTRUCT lpReOpenBuff, IN UINT uStyle)
{
	return 0;
}

BOOL read_line(FILE * fp, char *bp)
{
	char c = '\0';
	int i = 0;

	/* Read one line from the source file */
	while ((c = getc(fp)) != '\n') {
		if (c == EOF) {       /* return FALSE on unexpected EOF */
			return FALSE;
		}

		bp[i++] = c;
	}

	bp[i] = '\0';

	return TRUE;
}

HINSTANCE WINAPI ShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
	return NULL;
}

HINSTANCE WINAPI ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	return NULL;
}

BOOL STDAPICALLTYPE SHGetPathFromIDListA(LPCITEMIDLIST pidl, LPSTR pszPath)
{
	// TODO: implement
	return FALSE;
}

BOOL STDAPICALLTYPE SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath)
{
	// TODO: implement
	return FALSE;
}

HRESULT STDAPICALLTYPE SHGetSpecialFolderLocation(HWND hwnd, int csidl, LPITEMIDLIST *ppidl)
{
	// TODO: implement
	return 0;
}

WINBASEAPI DWORD WINAPI SizeofResource(IN HMODULE hModule, IN HRSRC hResInfo)
{
	// TODO: implement
	return 0;
}

WINBASEAPI BOOL WINAPI UnmapViewOfFile(IN LPCVOID lpBaseAddress)
{
	// TODO: implement
	return FALSE;
}

BOOL APIENTRY VerQueryValueA(const LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen)
{
	// TODO: implement
	return FALSE;
}

BOOL APIENTRY VerQueryValueW(const LPVOID pBlock, LPWSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen)
{
	// TODO: implement
	return FALSE;
}

WINUSERAPI DWORD WINAPI WaitForInputIdle(IN HANDLE hProcess, IN DWORD dwMilliseconds)
{
	// TODO: implement
	return 0;
}

WINBASEAPI BOOL WINAPI WritePrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName)
{
	FILE *rfp, *wfp;
	char tmp_name[15];
	char buff[MAX_LINE_LENGTH];
	char t_section[MAX_LINE_LENGTH];
	int len = strlen(lpKeyName);
	tmpnam(tmp_name); /* Get a temporary file name to copy to */
	sprintf(t_section,"[%s]", lpAppName);/* Format the section name */

	if (!(rfp = fopen(lpFileName, "r"))) { /* If the .ini file doesn't exist */
		if (!(wfp = fopen(lpFileName, "w"))) { /* then make one */
			return FALSE;
		}

		fprintf(wfp, "%s\n", t_section);
		fprintf(wfp, "%s=%s\n", lpKeyName, lpString);
		fclose(wfp);
		return TRUE;
	}

	if (!(wfp = fopen(tmp_name, "w"))) {
		fclose(rfp);

		return FALSE;
	}

	/* Move through the file one line at a time until a section is
	 * matched or until EOF. Copy to temp file as it is read. */
	do {
		if (!read_line(rfp, buff)) { /* Failed to find section, so add one to the end */
			fprintf(wfp, "\n%s\n", t_section);
			fprintf(wfp, "%s=%s\n", lpKeyName, lpString);

			/* Clean up and rename */
			fclose(rfp);
			fclose(wfp);
			unlink(lpFileName);
			rename(tmp_name, lpFileName);
			return TRUE;
		}

		fprintf(wfp, "%s\n", buff);
	} while(strcmp(buff, t_section));

	/* Now that the section has been found, find the entry. Stop searching
	 * upon leaving the section's area. Copy the file as it is read
	 * and create an entry if one is not found. */
	while (1) {
		if (!read_line(rfp, buff)) {
			/* EOF without an entry so make one */
			fprintf(wfp, "%s=%s\n", lpKeyName, lpString);

			/* Clean up and rename */
			fclose(rfp);
			fclose(wfp);
			unlink(lpFileName);
			rename(tmp_name, lpFileName);
			return(1);
		}

		if (!strncmp(buff, lpKeyName, len) || buff[0] == '\0') {
			break;
		}

		fprintf(wfp, "%s\n", buff);
	}

	if (buff[0] == '\0') {
		fprintf(wfp, "%s=%s\n", lpKeyName, lpString);

		do {
			fprintf(wfp, "%s\n", buff);
		} while(read_line(rfp, buff));
	} else {
		fprintf(wfp, "%s=%s\n", lpKeyName, lpString);

		while (read_line(rfp, buff)) {
			fprintf(wfp, "%s\n", buff);
		}
	}

	/* Clean up and rename */
	fclose(wfp);
	fclose(rfp);
	unlink(lpFileName);
	rename(tmp_name, lpFileName);

	return TRUE;
}
