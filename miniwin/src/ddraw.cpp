#include <ddraw.h>

extern IDirect3DDevice8 * d3dDevice;
extern void DebugPrintf(const char * fmt, ...);

/** Currently active palette */
IDirect3DPalette8 * systemPalette;

struct DXUT_SCREEN_VERTEX
{
	float x, y, z, h;
	D3DCOLOR color;
	float tu, tv;

	static DWORD FVF;
};
DWORD DXUT_SCREEN_VERTEX::FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

struct DirectDrawImpl : IDirectDraw
{
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE Compact();
	HRESULT STDMETHODCALLTYPE CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
	HRESULT STDMETHODCALLTYPE CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpColorTable, LPDIRECTDRAWPALETTE *lplpDDPalette, IUnknown *pUnkOuter);
	HRESULT STDMETHODCALLTYPE CreateSurface(LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *);
	HRESULT STDMETHODCALLTYPE DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR *);
	HRESULT STDMETHODCALLTYPE EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK);
	HRESULT STDMETHODCALLTYPE EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK);
	HRESULT STDMETHODCALLTYPE FlipToGDISurface();
	HRESULT STDMETHODCALLTYPE GetCaps(LPDDCAPS, LPDDCAPS);
	HRESULT STDMETHODCALLTYPE GetDisplayMode(LPDDSURFACEDESC);
	HRESULT STDMETHODCALLTYPE GetFourCCCodes(LPDWORD, LPDWORD);
	HRESULT STDMETHODCALLTYPE GetGDISurface(LPDIRECTDRAWSURFACE FAR *);
	HRESULT STDMETHODCALLTYPE GetMonitorFrequency(LPDWORD);
	HRESULT STDMETHODCALLTYPE GetScanLine(LPDWORD);
	HRESULT STDMETHODCALLTYPE GetVerticalBlankStatus(LPBOOL);
	HRESULT STDMETHODCALLTYPE Initialize(GUID FAR *);
	HRESULT STDMETHODCALLTYPE RestoreDisplayMode();
	HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND, DWORD);
	HRESULT STDMETHODCALLTYPE SetDisplayMode(DWORD, DWORD,DWORD);
	HRESULT STDMETHODCALLTYPE WaitForVerticalBlank(DWORD, HANDLE);

private:
	DWORD width;
	DWORD height;
};

struct DirectDrawPaletteImpl : IDirectDrawPalette
{
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE GetCaps(LPDWORD lpdwCaps);
	HRESULT STDMETHODCALLTYPE GetEntries(DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries);
	HRESULT STDMETHODCALLTYPE Initialize(LPDIRECTDRAW, DWORD, LPPALETTEENTRY);
	HRESULT STDMETHODCALLTYPE SetEntries(DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries);

private:
	friend struct DirectDrawImpl;
	friend struct DirectDrawSurfaceImpl;

	IDirect3DPalette8 * palette;
};

struct DirectDrawSurfaceImpl : IDirectDrawSurface
{
	DirectDrawSurfaceImpl(LPDDSURFACEDESC lpDDSurfaceDesc);
	~DirectDrawSurfaceImpl();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE AddAttachedSurface(LPDIRECTDRAWSURFACE);
	HRESULT STDMETHODCALLTYPE AddOverlayDirtyRect(LPRECT);
	HRESULT STDMETHODCALLTYPE Blt(LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, LPDDBLTFX);
	HRESULT STDMETHODCALLTYPE BltBatch(LPDDBLTBATCH, DWORD, DWORD);
	HRESULT STDMETHODCALLTYPE BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT STDMETHODCALLTYPE DeleteAttachedSurface(DWORD,LPDIRECTDRAWSURFACE);
	HRESULT STDMETHODCALLTYPE EnumAttachedSurfaces(LPVOID,LPDDENUMSURFACESCALLBACK);
	HRESULT STDMETHODCALLTYPE EnumOverlayZOrders(DWORD,LPVOID,LPDDENUMSURFACESCALLBACK);
	HRESULT STDMETHODCALLTYPE Flip(LPDIRECTDRAWSURFACE, DWORD);
	HRESULT STDMETHODCALLTYPE GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *);
	HRESULT STDMETHODCALLTYPE GetBltStatus(DWORD);
	HRESULT STDMETHODCALLTYPE GetCaps(LPDDSCAPS lpDDSCaps);
	HRESULT STDMETHODCALLTYPE GetClipper(LPDIRECTDRAWCLIPPER FAR*);
	HRESULT STDMETHODCALLTYPE GetColorKey(DWORD, LPDDCOLORKEY);
	HRESULT STDMETHODCALLTYPE GetDC(HDC *lphDC);
	HRESULT STDMETHODCALLTYPE GetFlipStatus(DWORD);
	HRESULT STDMETHODCALLTYPE GetOverlayPosition(LPLONG, LPLONG);
	HRESULT STDMETHODCALLTYPE GetPalette(LPDIRECTDRAWPALETTE FAR*);
	HRESULT STDMETHODCALLTYPE GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT STDMETHODCALLTYPE GetSurfaceDesc(LPDDSURFACEDESC);
	HRESULT STDMETHODCALLTYPE Initialize(LPDIRECTDRAW, LPDDSURFACEDESC);
	HRESULT STDMETHODCALLTYPE IsLost();
	HRESULT STDMETHODCALLTYPE Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT STDMETHODCALLTYPE ReleaseDC(HDC hDC);
	HRESULT STDMETHODCALLTYPE Restore();
	HRESULT STDMETHODCALLTYPE SetClipper(LPDIRECTDRAWCLIPPER);
	HRESULT STDMETHODCALLTYPE SetColorKey(DWORD, LPDDCOLORKEY);
	HRESULT STDMETHODCALLTYPE SetOverlayPosition(LONG, LONG);
	HRESULT STDMETHODCALLTYPE SetPalette(LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT STDMETHODCALLTYPE Unlock(LPVOID lpSurfaceData);
	HRESULT STDMETHODCALLTYPE UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX);
	HRESULT STDMETHODCALLTYPE UpdateOverlayDisplay(DWORD);
	HRESULT STDMETHODCALLTYPE UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE);

	friend struct DirectDrawImpl;

private:
	BOOL isPrimary;
	IDirect3DPalette8 * palette;
	IDirect3DTexture8 * texture;
};

static DWORD DDrawFlagsToD3DFlags(DWORD dDrawFlags)
{
	switch (dDrawFlags) {
		case DDLOCK_READONLY:
			return D3DLOCK_READONLY;
		case DDLOCK_NOOVERWRITE:
			return D3DLOCK_NOOVERWRITE;
	}

	return 0;
}

HRESULT WINAPI DirectDrawCreate(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter)
{
	if (lplpDD == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);

		return DD_FALSE;
	}

	*lplpDD = new DirectDrawImpl();

	return DD_OK;
}

//-------------------------------------------------------------------------
// DirectDrawImpl
//-------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE DirectDrawImpl::AddRef()
{
	DebugPrintf("DirectDrawImpl::AddRef()\n");

	return 1;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::Compact()
{
	DebugPrintf("DirectDrawImpl::Compact()\n");

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	DebugPrintf(
		"DirectDrawImpl::CreateClipper(0x%08X, %p, %p)\n",
		dwFlags,
		lplpDDClipper,
		pUnkOuter);

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpColorTable, LPDIRECTDRAWPALETTE *lplpDDPalette, IUnknown *pUnkOuter)
{
	DebugPrintf(
		"DirectDrawImpl::CreatePalette(0x%08X, 0x%p, 0x%p, 0x%p)\n",
		dwFlags,
		lpColorTable,
		lplpDDPalette,
		pUnkOuter);

	*lplpDDPalette = new DirectDrawPaletteImpl();

	return d3dDevice->CreatePalette(D3DPALETTE_256, &((DirectDrawPaletteImpl *)(*lplpDDPalette))->palette);
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface, IUnknown *pUnkOuter)
{
	DebugPrintf("DirectDrawImpl::CreateSurface(0x%p, 0x%p, 0x%p)\n", lpDDSurfaceDesc, lplpDDSurface, pUnkOuter);

	if (lpDDSurfaceDesc == NULL || lplpDDSurface == NULL) {
		return DDERR_INVALIDPARAMS;
	}

	*lplpDDSurface = new DirectDrawSurfaceImpl(lpDDSurfaceDesc);

	// TODO: implement

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::DuplicateSurface(LPDIRECTDRAWSURFACE surface, LPDIRECTDRAWSURFACE FAR * lplpDDSurface)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::FlipToGDISurface()
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetCaps(LPDDCAPS, LPDDCAPS)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetDisplayMode(LPDDSURFACEDESC lpSurfaceDesc)
{
	if (lpSurfaceDesc == NULL) {
		return DDERR_INVALIDPARAMS;
	}

	lpSurfaceDesc->dwHeight = height;
	lpSurfaceDesc->dwRefreshRate = 60;
	lpSurfaceDesc->dwWidth = width;
	lpSurfaceDesc->dwZBufferBitDepth = 24;

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetFourCCCodes(LPDWORD, LPDWORD)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetGDISurface(LPDIRECTDRAWSURFACE FAR *)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetMonitorFrequency(LPDWORD lpFrequency)
{
	if (lpFrequency != NULL) {
		*lpFrequency = 60;
	}

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetScanLine(LPDWORD)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetVerticalBlankStatus(LPBOOL)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::Initialize(GUID FAR *)
{
	return DDERR_ALREADYINITIALIZED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::QueryInterface(REFIID riid, LPVOID FAR * ppvObj)
{
	return DD_OK;
}

ULONG STDMETHODCALLTYPE DirectDrawImpl::Release()
{
	systemPalette->Release();

	return 0;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::RestoreDisplayMode()
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	DebugPrintf("DirectDrawImpl::SetCooperativeLevel(0x%p, 0x%08X)\n", hWnd, dwFlags);

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	DebugPrintf("DirectDrawImpl::SetDisplayMode(%u, %u, %u)\n", dwWidth, dwHeight, dwBPP);

	width = dwWidth;
	height = dwHeight;

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::WaitForVerticalBlank(DWORD dwFlags, HANDLE hWnd)
{
	DebugPrintf("DirectDrawImpl::WaitForVerticalBlank(0x%08X, 0x%p)\n", dwFlags, hWnd);

	// TODO: implement

	return DD_OK;
}

//-------------------------------------------------------------------------
// DirectDrawPaletteImpl
//-------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE DirectDrawPaletteImpl::AddRef()
{
	return palette->AddRef();
}

HRESULT STDMETHODCALLTYPE GetCaps(LPDWORD lpdwCaps)
{
	// TODO: implement

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawPaletteImpl::GetCaps(LPDWORD lpdwCaps)
{
	if (lpdwCaps != NULL) {
		*lpdwCaps = 0;
	}

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawPaletteImpl::GetEntries(DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries)
{
	D3DCOLOR * colors;

	palette->Lock(&colors, D3DLOCK_READONLY);

	for (DWORD s = dwBase, d = 0; d < dwNumEntries; s++, d++) {
		const BYTE r = (BYTE)((colors[s] >> 16) & 0xFF);
		const BYTE g = (BYTE)((colors[s] >>  8) & 0xFF);
		const BYTE b = (BYTE)(colors[s] & 0xFF);

		lpEntries[d].peFlags = 0;
		lpEntries[d].peRed = r;
		lpEntries[d].peGreen = g;
		lpEntries[d].peBlue = b;
	}

	palette->Unlock();

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawPaletteImpl::Initialize(LPDIRECTDRAW, DWORD, LPPALETTEENTRY)
{
	return DDERR_ALREADYINITIALIZED;
}

HRESULT STDMETHODCALLTYPE DirectDrawPaletteImpl::QueryInterface(REFIID riid, LPVOID FAR * ppvObj)
{
	return DD_OK;
}

ULONG STDMETHODCALLTYPE DirectDrawPaletteImpl::Release()
{
	return palette->Release();
}

HRESULT STDMETHODCALLTYPE DirectDrawPaletteImpl::SetEntries(DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries)
{
	D3DCOLOR * colors;

	palette->Lock(&colors, 0);

	for (DWORD d = dwStartingEntry, s = 0; s < dwCount; d++, s++) {
		colors[d] = D3DCOLOR_XRGB(lpEntries[s].peRed, lpEntries[s].peGreen, lpEntries[s].peBlue);
	}

	palette->Unlock();

	return DD_OK;
}

//-------------------------------------------------------------------------
// DirectDrawSurfaceImpl
//-------------------------------------------------------------------------

DirectDrawSurfaceImpl::DirectDrawSurfaceImpl(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	DebugPrintf("DirectDrawSurfaceImpl::DirectDrawSurfaceImpl(0x%p)\n", lpDDSurfaceDesc);

	isPrimary = lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE;

	if (isPrimary) {
		/* no need to allocate a texture, since the primary surface doesn't really exist
		 * instead, we directly 'blt' any surface to the screen when asked to do a blt */
		return;
	}

	d3dDevice->CreateTexture(
		lpDDSurfaceDesc->dwWidth,
		lpDDSurfaceDesc->dwHeight,
		1,
		0,
		D3DFMT_P8,
		0,
		&texture);
}

DirectDrawSurfaceImpl::~DirectDrawSurfaceImpl()
{
	texture->Release();
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::AddAttachedSurface(LPDIRECTDRAWSURFACE)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::AddOverlayDirtyRect(LPRECT)
{
	return DD_OK;
}

ULONG STDMETHODCALLTYPE DirectDrawSurfaceImpl::AddRef()
{
	// TODO: implement

	return 1;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::Blt(LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, LPDDBLTFX)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::BltBatch(LPDDBLTBATCH, DWORD, DWORD)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	LONG w = lpSrcRect->right - lpSrcRect->left + 1;
	LONG h = lpSrcRect->bottom - lpSrcRect->top + 1;
	RECT dstRect = { dwX, dwY, dwX + w - 1, dwY + h - 1 };

	DXUT_SCREEN_VERTEX verts[] = {
		{ dstRect.left - 0.5f, dstRect.top - 0.5f, 0.5f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f },
		{ dstRect.right - 0.5f, dstRect.top - 0.5f, 0.5f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f },
		{ dstRect.right - 0.5f, dstRect.bottom - 0.5f, 0.5f, 1.0f, 0xFFFFFFFF, 1.0f, 1.0f },
		{ dstRect.left - 0.5f, dstRect.bottom - 0.5f, 0.5f, 1.0f, 0xFFFFFFFF, 0.0f, 1.0f }
	};

	if (palette != NULL) {
		d3dDevice->SetPalette(0, palette);
	} else {
		d3dDevice->SetPalette(0, systemPalette);
	}

	d3dDevice->SetTexture(0, texture);

	d3dDevice->SetVertexShader(DXUT_SCREEN_VERTEX::FVF);

	d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(DXUT_SCREEN_VERTEX));

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::DeleteAttachedSurface(DWORD,LPDIRECTDRAWSURFACE)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::EnumAttachedSurfaces(LPVOID,LPDDENUMSURFACESCALLBACK)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::EnumOverlayZOrders(DWORD,LPVOID,LPDDENUMSURFACESCALLBACK)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::Flip(LPDIRECTDRAWSURFACE, DWORD)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetBltStatus(DWORD)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetCaps(LPDDSCAPS lpDDSCaps)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetClipper(LPDIRECTDRAWCLIPPER FAR*)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetColorKey(DWORD, LPDDCOLORKEY)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetDC(HDC *lphDC)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetFlipStatus(DWORD)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetOverlayPosition(LPLONG, LPLONG)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetPalette(LPDIRECTDRAWPALETTE FAR*)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	if (lpDDPixelFormat == NULL) {
		return DDERR_INVALIDPARAMS;
	}

	lpDDPixelFormat->dwRGBBitCount = 8;

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	DebugPrintf("DirectDrawSurfaceImpl::GetSurfaceDesc(0x%p)\n", lpDDSurfaceDesc);

	// TODO: implement

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::Initialize(LPDIRECTDRAW, LPDDSURFACEDESC)
{
	return DDERR_ALREADYINITIALIZED;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::IsLost()
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	DebugPrintf(
		"DirectDrawSurfaceImpl::Lock(0x%p, 0x%p, 0x%08X, 0x%p)\n",
		lpDestRect,
		lpDDSurfaceDesc,
		dwFlags,
		hEvent);

	// TODO: see if we need to grant access to the actual back buffer when locking our primary buffer

	if (!isPrimary) {
		D3DLOCKED_RECT lockedRect;

		texture->LockRect(0, &lockedRect, lpDestRect, DDrawFlagsToD3DFlags(dwFlags));
		lpDDSurfaceDesc->lpSurface = lockedRect.pBits;
		lpDDSurfaceDesc->lPitch = lockedRect.Pitch;
	}

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::ReleaseDC(HDC hDC)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::Restore()
{
	return DD_OK;
}


HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::QueryInterface(REFIID riid, LPVOID FAR * ppvObj)
{
	return DD_OK;
}

ULONG STDMETHODCALLTYPE DirectDrawSurfaceImpl::Release()
{
	if (!isPrimary) {
		palette->Release();
		texture->Release();
	}

	return 0;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::SetClipper(LPDIRECTDRAWCLIPPER)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::SetColorKey(DWORD, LPDDCOLORKEY)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::SetOverlayPosition(LONG, LONG)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	DebugPrintf("DirectDrawSurfaceImpl::SetPalette(0x%p)\n", lpDDPalette);

	if (isPrimary) {
		systemPalette = ((DirectDrawPaletteImpl *)lpDDPalette)->palette;
	}

	// TODO: implement

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::Unlock(LPVOID lpSurfaceData)
{
	if (!isPrimary) {
		texture->UnlockRect(0);
	}

	// TODO: implement

	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::UpdateOverlayDisplay(DWORD)
{
	return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawSurfaceImpl::UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE)
{
	return DD_OK;
}
