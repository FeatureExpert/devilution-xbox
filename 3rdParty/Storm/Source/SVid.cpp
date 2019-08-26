#include "storm.h"
#include <smacker.h>

#include <Windows.h>

#include <XGraphics.h>

extern IDirect3DDevice8 * d3dDevice;
extern void DebugPrintf(const char * fmt, ...);

double SVidFrameEnd;
double SVidFrameLength;
smk SVidSMK;
IDirect3DTexture8 *SVidTexture;
IDirect3DPalette8 *SVidPalette;
BYTE *SVidBuffer;
BOOL SVidLoop;
BYTE *SVidTextureBuffer;
IDirectSoundBuffer8 *deviceId;
unsigned long SVidWidth, SVidHeight;
unsigned long SVidTexWidth, SVidTexHeight;

// FIXME: resolution is assumed to be 640x480. Tie this to the actual resolution, instead.

static inline DWORD roundup(DWORD v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

struct CUSTOMVERTEX
{
	float x, y, z, w;
	float tu, tv;
};

static LPDIRECTSOUND dsound;

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
	DebugPrintf(
		"SVidPlayBegin(\"%s\", %i, %i, %i, %i, %i, 0x%p)\n",
		filename,
		a2,
		a3,
		a4,
		a5,
		flags,
		video);

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
			DebugPrintf("IDirectSound_CreateSoundBuffer: 0x08X\n", result);
			// TODO: log error

			return FALSE;
		}

		if (FAILED(result = deviceId->Play(0, 0, 0))) { /* start audio playing. */
			DebugPrintf("deviceId->Play: 0x08X\n", result);
		}

		if (FAILED(result = deviceId->SetVolume(DSBVOLUME_MAX))) {
			DebugPrintf("deviceId->SetVolume: 0x08X\n", result);
		}
	}

	unsigned long nFrames;
	smk_info_all(SVidSMK, NULL, &nFrames, &SVidFrameLength);
	smk_info_video(SVidSMK, &SVidWidth, &SVidHeight, NULL);

	smk_enable_video(SVidSMK, enableVideo);
	smk_first(SVidSMK); // Decode first frame

	smk_info_video(SVidSMK, &SVidWidth, &SVidHeight, NULL);

	SVidTexWidth = roundup(SVidWidth);
	SVidTexHeight = roundup(SVidHeight);

	if (FAILED(result = d3dDevice->CreateTexture(SVidTexWidth, SVidTexHeight, 1, 0, D3DFMT_P8, 0, &SVidTexture))) {
		DebugPrintf("d3dDevice->CreateTexture: 0x08X\n", result);
		// TODO: log error

		return FALSE;
	}

	SVidTextureBuffer = (BYTE *)SMemAlloc(SVidTexWidth * SVidTexHeight, "P:\\Projects\\Storm\\Storm-SWAR\\Source\\SVid.cpp", __LINE__, 0);

	const unsigned char *palette_data = smk_get_palette(SVidSMK);
	const unsigned char *video_data = smk_get_video(SVidSMK);

	for (DWORD h = 0; h < SVidHeight; h++) {
		memcpy(&SVidTextureBuffer[h * SVidTexWidth], &video_data[h * SVidWidth], SVidWidth);
	}

	D3DLOCKED_RECT rect;
	SVidTexture->LockRect(0, &rect, NULL, 0);

	XGSwizzleRect(SVidTextureBuffer, 0, NULL, rect.pBits, SVidTexWidth, SVidTexHeight, NULL, sizeof(BYTE));

	SVidTexture->UnlockRect(0);

	d3dDevice->CreatePalette(D3DPALETTE_256, &SVidPalette);

	d3dDevice->SetTexture(0, SVidTexture);
	d3dDevice->SetPalette(0, SVidPalette);

	SVidFrameEnd = GetTickCount() * 1000 + SVidFrameLength;

	return TRUE;
}

BOOL __cdecl SVidPlayContinue(void)
{
	HRESULT result;

	const unsigned char *palette_data = smk_get_palette(SVidSMK);
	const unsigned char *video_data = smk_get_video(SVidSMK);

	if (smk_palette_updated(SVidSMK)) {
		D3DCOLOR * colors;

		SVidPalette->Lock(&colors, 0);

		for (DWORD i = 0; i < 256; i++) {
			colors[i] = D3DCOLOR_XRGB(palette_data[i * 3 + 0], palette_data[i * 3 + 1], palette_data[i * 3 + 2]);
		}

		SVidPalette->Unlock();

		d3dDevice->SetPalette(0, SVidPalette);
	}

	for (DWORD h = 0; h < SVidHeight; h++) {
		memcpy(&SVidTextureBuffer[h * SVidTexWidth], &video_data[h * SVidWidth], SVidWidth);
	}

	D3DLOCKED_RECT rect;
	SVidTexture->LockRect(0, &rect, NULL, 0);

	XGSwizzleRect(SVidTextureBuffer, 0, NULL, rect.pBits, SVidTexWidth, SVidTexHeight, NULL, sizeof(BYTE));

	SVidTexture->UnlockRect(0);

	if (GetTickCount() * 1000 >= SVidFrameEnd) {
		return SVidLoadNextFrame(); // Skip video and audio if the system is too slow
	}

	if (FAILED(result = deviceId->SetBufferData((LPVOID)smk_get_audio(SVidSMK, 0), smk_get_audio_size(SVidSMK, 0)))) {
		DebugPrintf("deviceId->SetBufferData: 0x%08X\n", result);
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

	CUSTOMVERTEX verts[] = {
		{ ((640 - scaledW) / 2) - 0.5f, ((480 - scaledH) / 2) - 0.5f, 0.5f, 1.0f, 0.0f, 0.0f },
		{ (((640 - scaledW) / 2) + scaledW) - 0.5f, ((480 - scaledH) / 2) - 0.5f, 0.5f, 1.0f, 0.625f, 0.0f },
		{ (((640 - scaledW) / 2) + scaledW) - 0.5f, (((480 - scaledH) / 2) + scaledH) - 0.5f, 0.5f, 1.0f, 0.625f, 0.609375f },
		{ ((640 - scaledW) / 2) - 0.5f, (((480 - scaledH) / 2) + scaledH) - 0.5f, 0.5f, 1.0f, 0.0f, 0.609375f }
	};

	d3dDevice->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
	d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));

	d3dDevice->Present(NULL, NULL, NULL, NULL);

	double now = GetTickCount() * 1000;
	if (now < SVidFrameEnd) {
		Sleep((SVidFrameEnd - now) / 1000); // wait with next frame if the system is too fast
	}

	return SVidLoadNextFrame();
}

BOOL STORMAPI SVidPlayEnd(HANDLE video)
{
	DebugPrintf("SVidPlayEnd(0x%p)\n", video);

	if (!video) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	if (deviceId) {
		deviceId->StopEx(0, DSBSTOPEX_IMMEDIATE);
		deviceId->SetBufferData(NULL, 0);
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

	if (SVidTextureBuffer) {
		SMemFree(SVidTextureBuffer, "P:\\Projects\\Storm\\Storm-SWAR\\Source\\SVid.cpp", __LINE__, 0);

		SVidTextureBuffer = NULL;
	}

	d3dDevice->SetTexture(0, NULL);
	SVidTexture->Release();
	d3dDevice->SetPalette(0, NULL);
	SVidPalette->Release();

	SFileCloseFile(video);
	video = NULL;

	return TRUE;
}

BOOL STORMAPI SVidDestroy()
{
	DebugPrintf("SVidDestroy()\n");

	dsound = NULL;

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SVidGetSize(HANDLE video, int *width, int *height, int *zero)
{
	DebugPrintf(
		"SVidGetSize(0x%p, 0x%p, 0x%p, 0x%p)\n",
		video,
		width,
		height,
		zero);

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
	DebugPrintf("SVidInitialize(0x%p)\n", directsound);

	dsound = (LPDIRECTSOUND)directsound;

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SVidPlayContinueSingle(HANDLE video, int a2, int * a3)
{
	DebugPrintf(
		"SVidPlayContinueSingle(0x%p, %i, 0x%p)\n",
		video,
		a2,
		a3);

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
