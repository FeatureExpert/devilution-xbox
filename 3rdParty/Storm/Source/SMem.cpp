#include "storm.h"

void *STORMAPI SMemAlloc(size_t amount, const char *logfilename, int logline, int defaultValue)
{
	// TODO: log

	void * mem = malloc(amount);

	if (mem != NULL) {
		SMemFill(mem, amount, defaultValue);
	}

	return mem;
}

BOOL STORMAPI SMemFree(void *location, const char *logfilename, int logline, char defaultValue)
{
	// TODO: zero freed region?
	// TODO: log

	free(location);

	return TRUE;
}

void* STORMAPI SMemReAlloc(void *location, size_t amount, const char *logfilename, int logline, char defaultValue)
{
	// TODO: zero (freed) region?
	// TODO: log

	return realloc(location, amount);
}

//@implemented
void __declspec(naked) STORMAPI SMemCopy(void *dest, const void *source, size_t size)
{
	__asm {
		mov     ecx, [esp + 12] ; size
		push    esi
		mov     esi, [esp + 12] ; source
		mov     eax, ecx
		push    edi
		mov     edi, [esp + 12] ; dest
		shr     ecx, 2
		rep     movsd
		mov     ecx, eax
		and     ecx, 3
		rep     movsb
		pop     edi
		pop     esi
		ret     12
	}
}

//@implemented
void __declspec(naked) STORMAPI SMemFill(void *location, size_t length, char fillWith)
{
	__asm {
		mov     eax, dword ptr [esp + 12]   ; fillWith
		mov     ecx, [esp + 8]              ; length
		and     eax, 0FFh
		push    ebx
		mov     bl, al
		mov     edx, ecx
		mov     bh, bl
		push    edi
		mov     edi, [esp + 12]             ; location
		mov     eax, ebx
		shl     eax, 16
		mov     ax, bx
		shr     ecx, 2
		rep     stosd
		mov     ecx, edx
		and     ecx, 3
		rep     stosb
		pop     edi
		pop     ebx
		ret     12
	}
}

//@implemented
void STORMAPI SMemMove(void *dest, const void *source, size_t size)
{
	memmove(dest, source, size);
}

//@implemented
void __declspec(naked) STORMAPI SMemZero(void *location, DWORD length)
{
	__asm {
		mov     ecx, [esp + 8]   ; length
		push    edi
		mov     edi, [esp + 8]  ; location
		mov     edx, ecx
		xor     eax, eax
		shr     ecx, 2
		rep     stosd
		mov     ecx, edx
		and     ecx, 3
		rep     stosb
		pop     edi
		ret     8
	}
}

int STORMAPI SMemCmp(void *location1, void *location2, DWORD size)
{
	return memcmp(location1, location2, size);
}
