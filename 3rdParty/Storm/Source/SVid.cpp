#include "storm.h"
#include <smacker.h>

#include <Windows.h>

extern IDirect3DDevice8 * d3dDevice;

double SVidFrameEnd;
double SVidFrameLength;
smk SVidSMK;
PALETTEENTRY SVidPalette[256];
IDirect3DSurface8 *SVidSurface;
BYTE *SVidBuffer;
BOOL SVidLoop;
IDirectSoundBuffer8 *deviceId;
unsigned long SVidWidth, SVidHeight;

static LPDIRECTSOUND dsound;

#if defined(_M_IX86) || defined(__i386__) || defined(__386__)
#define PREFIX16    0x66
#define STORE_BYTE  0xAA
#define STORE_WORD  0xAB
#define LOAD_BYTE   0xAC
#define LOAD_WORD   0xAD
#define RETURN      0xC3
#else
#error Need assembly opcodes for this architecture
#endif

static __declspec(align(16)) BYTE copy_row[4096];

static int generate_rowbytes(int src_w, int dst_w, int bpp)
{
	static struct
	{
		int bpp;
		int src_w;
		int dst_w;
		int status;
	} last;

	int i;
	int pos, inc;
	unsigned char *eip, *fence;
	unsigned char load, store;

	/* See if we need to regenerate the copy buffer */
	if ((src_w == last.src_w) && (dst_w == last.dst_w) && (bpp == last.bpp)) {
		return (last.status);
	}
	last.bpp = bpp;
	last.src_w = src_w;
	last.dst_w = dst_w;
	last.status = -1;

	switch (bpp) {
	case 1:
		load = LOAD_BYTE;
		store = STORE_BYTE;
		break;
	case 2:
	case 4:
		load = LOAD_WORD;
		store = STORE_WORD;
		break;
	default:
		//return SDL_SetError("ASM stretch of %d bytes isn't supported", bpp);
		return -2;
	}

	pos = 0x10000;
	inc = (src_w << 16) / dst_w;
	eip = copy_row;
	fence = copy_row + sizeof(copy_row) - 2;

	for (i = 0; i < dst_w; ++i) {
		while (pos >= 0x10000L) {
			if (eip == fence) {
				return -1;
			}

			if (bpp == 2) {
				*eip++ = PREFIX16;
			}

			*eip++ = load;
			pos -= 0x10000L;
		}

		if (eip == fence) {
			return -1;
		}

		if (bpp == 2) {
			*eip++ = PREFIX16;
		}

		*eip++ = store;
		pos += inc;
	}

	*eip++ = RETURN;

	last.status = 0;
	return 0;
}

#define DEFINE_COPY_ROW(name, type)         \
static void name(type *src, int src_w, type *dst, int dst_w)    \
{                                           \
	int i;                                  \
	int pos, inc;                           \
	type pixel = 0;                         \
											\
	pos = 0x10000;                          \
	inc = (src_w << 16) / dst_w;            \
	for ( i=dst_w; i>0; --i ) {             \
		while ( pos >= 0x10000L ) {         \
			pixel = *src++;                 \
			pos -= 0x10000L;                \
		}                                   \
		*dst++ = pixel;                     \
		pos += inc;                         \
	}                                       \
}
/* *INDENT-OFF* */
DEFINE_COPY_ROW(copy_row1, BYTE)
DEFINE_COPY_ROW(copy_row2, USHORT)
DEFINE_COPY_ROW(copy_row4, DWORD)
/* *INDENT-ON* */

/* The ASM code doesn't handle 24-bpp stretch blits */
static void copy_row3(BYTE * src, int src_w, BYTE * dst, int dst_w)
{
	int i;
	int pos, inc;
	BYTE pixel[3] = { 0, 0, 0 };

	pos = 0x10000;
	inc = (src_w << 16) / dst_w;

	for (i = dst_w; i > 0; --i) {
		while (pos >= 0x10000L) {
			pixel[0] = *src++;
			pixel[1] = *src++;
			pixel[2] = *src++;
			pos -= 0x10000L;
		}

		*dst++ = pixel[0];
		*dst++ = pixel[1];
		*dst++ = pixel[2];
		pos += inc;
	}
}

#define BlitScaled UpperBlitScaled

static HRESULT LowerBlitScaled(IDirect3DSurface8 * const src, const RECT * srcRect, IDirect3DSurface8 * const dst, const RECT * dstRect)
{
	HRESULT result;
	int pos, inc;
	int dst_maxrow;
	int src_row, dst_row;
	BYTE *srcp = NULL;
	BYTE *dstp;
	RECT full_src;
	RECT full_dst;
	D3DSURFACE_DESC srcDesc;
	D3DLOCKED_RECT  srcLock;
	D3DSURFACE_DESC dstDesc;
	D3DLOCKED_RECT  dstLock;
	BOOL use_asm = TRUE;

	if (src == NULL || dst == NULL) {
		return E_INVALIDARG;
	}

	if (FAILED(result = src->GetDesc(&srcDesc))) {
		return result;
	}

	if (FAILED(result = dst->GetDesc(&dstDesc))) {
		return result;
	}

	/* Verify the blit rectangles */
	if (srcRect) {
		if ((srcRect->left < 0) || (srcRect->top < 0) ||
			(srcRect->right > srcDesc.Width) ||
			(srcRect->bottom > srcDesc.Height)) {
			return E_INVALIDARG;
		}
	} else {
		full_src.left = 0;
		full_src.top = 0;
		full_src.right = srcDesc.Width;
		full_src.bottom = srcDesc.Height;
		srcRect = &full_src;
	}

	if (dstRect) {
		if ((dstRect->left < 0) || (dstRect->top < 0) ||
			(dstRect->right > dstDesc.Width) ||
			(dstRect->bottom > dstDesc.Height)) {
			return E_INVALIDARG;
		}
	} else {
		full_dst.left = 0;
		full_dst.top = 0;
		full_dst.right = dstDesc.Width;
		full_dst.bottom = dstDesc.Height;
		dstRect = &full_dst;
	}

	if (FAILED(result = dst->LockRect(&dstLock, NULL, D3DLOCK_TILED))) {
		return result;
	}

	if (FAILED(result = src->LockRect(&srcLock, srcRect, D3DLOCK_READONLY))) {
		dst->UnlockRect();

		return result;
	}

	/* Set up the data... */
	pos = 0x10000;
	inc = ((srcRect->bottom - srcRect->top) << 16) / (dstRect->bottom - dstRect->top);
	src_row = srcRect->top;
	dst_row = dstRect->top;

	if (generate_rowbytes(srcRect->right - srcRect->left, dstRect->right - dstRect->left, 4) < 0) {
		use_asm = FALSE;
	}

	/* Perform the stretch blit */
	for (dst_maxrow = dst_row + (dstRect->bottom - dstRect->top); dst_row < dst_maxrow; ++dst_row) {
		dstp = (BYTE *)dstLock.pBits + (dst_row * dstLock.Pitch) + (dstRect->left * 4);

		while (pos >= 0x10000L) {
			srcp = (BYTE *)srcLock.pBits + (src_row * srcLock.Pitch) + (srcRect->left * 4);
			++src_row;
			pos -= 0x10000L;
		}

		if (use_asm) {
			/* *INDENT-OFF* */
			{
				void *code = copy_row;
				__asm {
					push	edi
					push	esi
					mov 	edi, dstp
					mov 	esi, srcp
					call	dword ptr code
					pop 	esi
					pop 	edi
				}
			}
			/* *INDENT-ON* */
		} else
			switch (4) {
			case 1:
				copy_row1(srcp, srcRect->right - srcRect->left, dstp, dstRect->right - dstRect->left);
				break;
			case 2:
				copy_row2((USHORT *)srcp, srcRect->right - srcRect->left,
						  (USHORT *)dstp, dstRect->right - dstRect->left);
				break;
			case 3:
				copy_row3(srcp, srcRect->right - srcRect->left, dstp, dstRect->right - dstRect->left);
				break;
			case 4:
				copy_row4((DWORD *)srcp, srcRect->right - srcRect->left,
						  (DWORD *)dstp, dstRect->right - dstRect->left);
				break;
			}

		pos += inc;
	}

	dst->UnlockRect();
	src->UnlockRect();

	return D3D_OK;
}

static BOOL SVidLoadNextFrame()
{
	SVidFrameEnd += SVidFrameLength;

	if (smk_next(SVidSMK) == SMK_DONE) {
		if (!SVidLoop) {
			return FALSE;
		}

		smk_first(SVidSMK);
	}

	return TRUE;
}

BOOL STORMAPI SVidPlayBegin(const char *filename, int a2, int a3, int a4, int a5, int flags, HANDLE *video)
{
	if (flags & 0x10000 || flags & 0x20000000) {
		return FALSE;
	}

	HRESULT result;
	SVidLoop = flags & 0x40000;
	BOOL enableVideo = !(flags & 0x100000);
	BOOL enableAudio = !(flags & 0x1000000);
	//0x8 // Non-interlaced
	//0x200, 0x800 // Upscale video
	//0x80000 // Center horizontally
	//0x800000 // Edge detection
	//0x200800 // Clear FB

	SFileOpenFile(filename, video);

	int bytestoread = SFileGetFileSize(*video, 0);
	SVidBuffer = (BYTE *)SMemAlloc(bytestoread, "P:\\Projects\\Storm\\Storm-SWAR\\Source\\SVid.cpp", __LINE__, 0);
	SFileReadFile(*video, SVidBuffer, bytestoread, NULL, 0);

	SVidSMK = smk_open_memory(SVidBuffer, bytestoread);

	if (SVidSMK == NULL) {
		return FALSE;
	}

	deviceId = 0;
	unsigned char channels[7], depth[7];
	unsigned long rate[7];
	smk_info_audio(SVidSMK, NULL, channels, depth, rate);

	if (enableAudio && depth[0] != 0) {
		smk_enable_audio(SVidSMK, 0, enableAudio);
		WAVEFORMATEX wfx;
		ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
		XAudioCreatePcmFormat(channels[0], rate[0], depth[0], &wfx);

		DSBUFFERDESC dsbd;
		ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwBufferBytes = 0;
		dsbd.lpwfxFormat = &wfx;

		if (FAILED(result = IDirectSound_CreateSoundBuffer(dsound, &dsbd, &deviceId, NULL))) {
			// TODO: log error

			return FALSE;
		}

		deviceId->Play(0, 0, 0); /* start audio playing. */
	}

	unsigned long nFrames;
	smk_info_all(SVidSMK, NULL, &nFrames, &SVidFrameLength);
	smk_info_video(SVidSMK, &SVidWidth, &SVidHeight, NULL);

	smk_enable_video(SVidSMK, enableVideo);
	smk_first(SVidSMK); // Decode first frame

	smk_info_video(SVidSMK, &SVidWidth, &SVidHeight, NULL);

	if (FAILED(result = d3dDevice->CreateImageSurface(SVidWidth, SVidHeight, D3DFMT_LIN_X8R8G8B8, &SVidSurface))) {
		// TODO: log error

		return FALSE;
	}

	const unsigned char *palette_data = smk_get_palette(SVidSMK);
	const unsigned char *video_data = smk_get_video(SVidSMK);

	D3DLOCKED_RECT rect;
	SVidSurface->LockRect(&rect, NULL, 0);

	for (unsigned long h = 0; h < SVidHeight; h++) {
		D3DCOLOR * cols = (D3DCOLOR *)((BYTE *)rect.pBits + (h * rect.Pitch));
		DWORD vidcols = h * SVidWidth;

		for (unsigned long w = 0; w < SVidWidth; w++) {
			const BYTE index = video_data[vidcols + w];

			cols[w] = D3DCOLOR_XRGB(palette_data[index * 3 + 0], palette_data[index * 3 + 1], palette_data[index * 3 + 2]);
		}
	}

	SVidSurface->UnlockRect();

	SVidFrameEnd = GetTickCount() * 1000 + SVidFrameLength;

	return TRUE;
}

BOOL __cdecl SVidPlayContinue(void)
{
	HRESULT result;

	const unsigned char *palette_data = smk_get_palette(SVidSMK);
	const unsigned char *video_data = smk_get_video(SVidSMK);

	D3DLOCKED_RECT rect;
	SVidSurface->LockRect(&rect, NULL, 0);

	for (unsigned long h = 0; h < SVidHeight; h++) {
		D3DCOLOR * cols = (D3DCOLOR *)((BYTE *)rect.pBits + h * rect.Pitch);
		DWORD vidcols = h * SVidWidth;

		for (unsigned long w = 0; w < SVidWidth; w++) {
			const BYTE index = video_data[vidcols + w];

			cols[w] = D3DCOLOR_XRGB(palette_data[index * 3 + 0], palette_data[index * 3 + 1], palette_data[index * 3 + 2]);
		}
	}

	SVidSurface->UnlockRect();

	if (GetTickCount() * 1000 >= SVidFrameEnd) {
		return SVidLoadNextFrame(); // Skip video and audio if the system is too slow
	}

	if (FAILED(result = deviceId->SetBufferData((LPVOID)smk_get_audio(SVidSMK, 0), smk_get_audio_size(SVidSMK, 0)))) {
		// TODO: log error

		return FALSE;
	}

	/*if (deviceId && SDL_QueueAudio(deviceId, smk_get_audio(SVidSMK, 0), smk_get_audio_size(SVidSMK, 0)) <= -1) {
		SDL_Log(SDL_GetError());
		return false;
	}*/

	if (GetTickCount() * 1000 >= SVidFrameEnd) {
		return SVidLoadNextFrame(); // Skip video if the system is to slow
	}

	int factor;
	int wFactor = 640 / SVidWidth;
	int hFactor = 480 / SVidHeight;
	if (wFactor > hFactor && 480 > SVidHeight) {
		factor = hFactor;
	} else {
		factor = wFactor;
	}
	int scaledW = SVidWidth * factor;
	int scaledH = SVidHeight * factor;

	RECT pal_surface_offset = { (640 - scaledW) / 2, (480 - scaledH) / 2, ((640 - scaledW) / 2) + scaledW, ((480 - scaledH) / 2) + scaledH };

	d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF000000, 1.0f, 0);

	IDirect3DSurface8 * surface;
	d3dDevice->GetBackBuffer(0, 0, &surface);

	LowerBlitScaled(SVidSurface, NULL, surface, &pal_surface_offset);

	surface->Release();

	d3dDevice->Present(NULL, NULL, NULL, NULL);

	double now = GetTickCount() * 1000;
	if (now < SVidFrameEnd) {
		Sleep((SVidFrameEnd - now) / 1000); // wait with next frame if the system is too fast
	}

	return SVidLoadNextFrame();
}

BOOL STORMAPI SVidPlayEnd(HANDLE video)
{
	if (!video) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	if (deviceId) {
		deviceId->Release();
		deviceId = NULL;
	}

	if (SVidSMK) {
		smk_close(SVidSMK);
	}

	if (SVidBuffer) {
		SMemFree(SVidBuffer, "P:\\Projects\\Storm\\Storm-SWAR\\Source\\SVid.cpp", __LINE__, 0);

		SVidBuffer = NULL;
	}

	SVidSurface->Release();

	SFileCloseFile(video);
	video = NULL;

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SVidDestroy()
{
	dsound = NULL;

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SVidGetSize(HANDLE video, int *width, int *height, int *zero)
{
	if (width) {
		*width = 0;
	}

	if (height) {
		*height = 0;
	}

	if (zero) {
		*zero = 0;
	}

	if (!video) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SVidInitialize(HANDLE directsound)
{
	dsound = (LPDIRECTSOUND)directsound;

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SVidPlayContinueSingle(HANDLE video, int a2, int * a3)
{
	if (a3 != NULL) {
		*a3 = 0;
	}

	if (video == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	// TODO: implement

	return TRUE;
}
