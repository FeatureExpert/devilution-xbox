#include "storm.h"

typedef struct PCXHeader {
	char manufacturer;
	char version;
	char encoding;
	char bitsPerPixel;
	short xmin, ymin;
	short xmax, ymax;
	short horzRes, vertRes;
	char palette[48];
	char reserved;
	char numColorPlanes;
	short bytesPerScanLine;
	short paletteType;
	short horzSize, vertSize;
	char padding[54];
} PCXHeader;

// TODO: move to storm.h
BOOL STORMAPI SBmpSaveImageEx(const char *pszFileName, PALETTEENTRY *pPalette, void *pBuffer, DWORD dwWidth, DWORD dwHeight, DWORD dwBpp, int a1 = 0);

BOOL STORMAPI SBmpDecodeImage(DWORD dwImgType, void *pSrcBuffer, DWORD dwSrcBuffersize, PALETTEENTRY *pPalette, void *pDstBuffer, DWORD dwDstBuffersize, DWORD *pdwWidth, DWORD *pdwHeight, DWORD *pdwBpp)
{
	if (pdwWidth) {
		*pdwWidth = 0;
	}

	if (pdwHeight) {
		*pdwHeight = 0;
	}

	if (pdwBpp) {
		*pdwBpp = 0;
	}

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SBmpLoadImage(const char *pszFileName, PALETTEENTRY *pPalette, BYTE *pBuffer, DWORD dwBuffersize, DWORD *pdwWidth, DWORD *pdwHeight, DWORD *pdwBpp)
{
	HANDLE hFile;
	size_t size;
	PCXHeader pcxhdr;
	BYTE paldata[256][3];
	BYTE *dataPtr, *fileBuffer;
	BYTE byte;

	if (pdwWidth) {
		*pdwWidth = 0;
	}

	if (pdwHeight) {
		*pdwHeight = 0;
	}

	if (pdwBpp) {
		*pdwBpp = 0;
	}

	if (!pszFileName || !*pszFileName) {
		return FALSE;
	}

	if (pBuffer && !dwBuffersize) {
		return FALSE;
	}

	if (!pPalette && !pBuffer && !pdwWidth && !pdwHeight) {
		return FALSE;
	}

	if (!SFileOpenFile(pszFileName, &hFile)) {
		return FALSE;
	}

	while (strchr(pszFileName, 92)) {
		pszFileName = strchr(pszFileName, 92) + 1;
	}

	while (strchr(pszFileName + 1, 46)) {
		pszFileName = strchr(pszFileName, 46);
	}

	// omit all types except PCX
	if (!pszFileName || _strcmpi(pszFileName, ".pcx")) {
		return FALSE;
	}

	if (!SFileReadFile(hFile, &pcxhdr, 128, 0, 0)) {
		SFileCloseFile(hFile);
		return FALSE;
	}

	int width = pcxhdr.xmax - pcxhdr.xmin + 1;
	int height = pcxhdr.ymax - pcxhdr.ymin + 1;

	if (pdwWidth) {
		*pdwWidth = width;
	}

	if (pdwHeight) {
		*pdwHeight = height;
	}

	if (pdwBpp) {
		*pdwBpp = pcxhdr.bitsPerPixel;
	}

	if (!pBuffer) {
		SFileSetFilePointer(hFile, 0, 0, 2);
		fileBuffer = NULL;
	} else {
		size = SFileGetFileSize(hFile, 0) - SFileSetFilePointer(hFile, 0, 0, 1);
		fileBuffer = (BYTE *)malloc(size);
	}

	if (fileBuffer) {
		SFileReadFile(hFile, fileBuffer, size, 0, 0);
		dataPtr = fileBuffer;

		for (int j = 0; j < height; j++) {
			for (int x = 0; x < width; dataPtr++) {
				byte = *dataPtr;

				if (byte < 0xC0) {
					*pBuffer = byte;
					pBuffer++;
					x++;
					continue;
				}

				dataPtr++;

				for (int i = 0; i < (byte & 0x3F); i++) {
					*pBuffer = *dataPtr;
					pBuffer++;
					x++;
				}
			}
		}

		free(fileBuffer);
	}

	if (pPalette && pcxhdr.bitsPerPixel == 8) {
		SFileSetFilePointer(hFile, -768, 0, 1);
		SFileReadFile(hFile, paldata, 768, 0, 0);

		for (int i = 0; i < 256; i++) {
			pPalette[i].peRed = paldata[i][0];
			pPalette[i].peGreen = paldata[i][1];
			pPalette[i].peBlue = paldata[i][2];
			pPalette[i].peFlags = 0;
		}
	}

	SFileCloseFile(hFile);

	return TRUE;
}

BOOL STORMAPI SBmpSaveImage(const char *pszFileName, PALETTEENTRY *pPalette, void *pBuffer, DWORD dwWidth, DWORD dwHeight, DWORD dwBpp)
{
	return SBmpSaveImageEx(pszFileName, pPalette, pBuffer, dwWidth, dwHeight, dwBpp);
}

BOOL STORMAPI SBmpSaveImageEx(const char *pszFileName, PALETTEENTRY *pPalette, void *pBuffer, DWORD dwWidth, DWORD dwHeight, DWORD dwBpp, int a1)
{
	// TODO: implement

	return TRUE;
}

HANDLE STORMAPI SBmpAllocLoadImage(const char *fileName, PALETTEENTRY *palette, void **buffer, int *width, int *height, int unused6, int unused7, void *(STORMAPI *allocFunction)(DWORD))
{
	// TODO: implement

	return NULL;
}
