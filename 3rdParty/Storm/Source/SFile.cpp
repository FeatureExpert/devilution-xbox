#include "storm.h"
#include "StormLib.h"
#include "StormCommon.h"
#include "FileStream.h"
#include "mpk.h"
#include "sqp.h"

#include <assert.h>
#include <stdio.h>
#include <tchar.h>

#define _countof(array) (sizeof(array) / sizeof(array[0])

#define HEADER_SEARCH_BUFFER_SIZE   0x1000
#define STORM_BUFFER_SIZE           0x500
#define HASH_INDEX_MASK(ha) (ha->pHeader->dwHashTableSize ? (ha->pHeader->dwHashTableSize - 1) : 0)

#define MPQ_HASH_TABLE_INDEX            0x000
#define MPQ_HASH_NAME_A                 0x100
#define MPQ_HASH_NAME_B                 0x200
#define MPQ_HASH_FILE_KEY               0x300
#define MPQ_HASH_KEY2_MIX               0x400
#define MPQ_STRONG_SIGNATURE_ID         0x5349474E      // ID of the strong signature ("NGIS")
#define MPQ_STRONG_SIGNATURE_SIZE       256

static DWORD StormBuffer[STORM_BUFFER_SIZE];    // Buffer for the decryption engine

static BOOL directFileAccess;
static LCID lcFileLocale;

LPDIRECTSOUND sound;
LPDIRECTSOUNDBUFFER  soundBuffer;
char * BasePath = "";
HANDLE archive;

extern void DebugPrintf(const char * fmt, ...);

static void BaseFile_Init(TFileStream *pStream);
static BOOL BaseFile_Open(TFileStream *pStream, const TCHAR *szFileName, DWORD dwStreamFlags);
static BOOL BaseFile_Read(TFileStream *pStream, ULONGLONG *pByteOffset, void *pvBuffer, DWORD dwBytesToRead);
static BOOL BaseFile_Resize(TFileStream *pStream, ULONGLONG NewFileSize);
static BOOL BaseFile_Write(TFileStream *pStream, ULONGLONG *pByteOffset, const void *pvBuffer, DWORD dwBytesToWrite);
static void BaseNone_Init(TFileStream *);
static int BuildFileTableFromBlockTable(TMPQArchive *ha, TMPQBlock *pBlockTable);
static ULONGLONG CalculateRawSectorOffset(TMPQFile *hf, DWORD dwSectorOffset);
static int ConvertSqpHeaderToFormat4(TMPQArchive * ha, ULONGLONG FileSize, DWORD dwFlags);
static void DecryptMpqBlock(void *pvDataBlock, DWORD dwLength, DWORD dwKey1);
static DWORD DetectFileKeyByKnownContent(void *pvEncryptedData, DWORD dwDecrypted0, DWORD dwDecrypted1);
static DWORD DetectFileKeyBySectorSize(LPDWORD EncryptedData, DWORD dwSectorSize, DWORD dwDecrypted0);
static TMPQHash *DefragmentHashTable(TMPQArchive *ha, TMPQHash *pHashTable, TMPQBlock *pBlockTable);
static ULONGLONG DetermineArchiveSize_V1(TMPQArchive *ha, TMPQHeader *pHeader, ULONGLONG MpqOffset, ULONGLONG FileSize);
static ULONGLONG DetermineArchiveSize_V2(TMPQHeader *pHeader, ULONGLONG MpqOffset, ULONGLONG FileSize);
static ULONGLONG FileOffsetFromMpqOffset(TMPQArchive *ha, ULONGLONG MpqOffset);
static BOOL FlatStream_LoadBitmap(TBlockStream *pStream);
static TFileStream *FlatStream_Open(const TCHAR *szFileName, DWORD dwStreamFlags);
static void FlatStream_UpdateBitmap(TBlockStream *pStream, ULONGLONG StartOffset, ULONGLONG EndOffset);
static TMPQHash *GetFirstHashEntry(TMPQArchive *ha, const char *szFileName);
static TMPQHash *GetHashEntryExact(TMPQArchive *ha, const char *szFileName, LCID lcLocale);
static TMPQHash *GetHashEntryLocale(TMPQArchive *ha, const char *szFileName, LCID lcLocale, BYTE Platform);
static DWORD GetNearestPowerOfTwo(DWORD dwFileCount);
static TMPQHash *GetNextHashEntry(TMPQArchive *ha, TMPQHash *pFirstHash, TMPQHash *pHash);
static DWORD HashString(const char *szFileName, DWORD dwHashType);
static DWORD HashStringLower(const char *szFileName, DWORD dwHashType);
static DWORD HashStringSlash(const char *szFileName, DWORD dwHashType);
static TMPQHash *LoadHashTable(TMPQArchive *ha);
static void *LoadMpqTable(TMPQArchive *ha, ULONGLONG ByteOffset, DWORD dwCompressedSize, DWORD dwTableSize, DWORD dwKey, BOOL *pbTableIsCut);
static TFileStream *MpqeStream_Open(const TCHAR *szFileName, DWORD dwStreamFlags);
static BOOL PartStream_LoadBitmap(TBlockStream *pStream);
static void PartStream_UpdateBitmap(TBlockStream *pStream, ULONGLONG StartOffset, ULONGLONG EndOffset, ULONGLONG RealOffset);
static TFileStream *PartStream_Open(const TCHAR *szFileName, DWORD dwStreamFlags);
static int ReadMpqSectors(TMPQFile *hf, LPBYTE pbBuffer, DWORD dwByteOffset, DWORD dwBytesToRead, LPDWORD pdwBytesRead);
static DWORD Rol32(DWORD dwValue, DWORD dwRolCount);
static DWORD StringToInt(const char *szString);
static BOOL VerifyDataBlockHash(void *pvDataBlock, DWORD cbDataBlock, LPBYTE expected_md5);

// Converts ASCII characters to lowercase
// Converts slash (0x2F) to backslash (0x5C)
unsigned char AsciiToLowerTable[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x5C,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

// Converts ASCII characters to lowercase
// Converts slash (0x2F) / backslash (0x5C) to system file-separator
unsigned char AsciiToLowerTable_Path[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
#ifdef _WIN32
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x5C,
#else
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
#endif
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
#ifdef _WIN32
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
#else
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x2F, 0x5D, 0x5E, 0x5F,
#endif
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

// Converts ASCII characters to uppercase
// Converts slash (0x2F) to backslash (0x5C)
unsigned char AsciiToUpperTable[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x5C,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

// Converts ASCII characters to uppercase
// Does NOT convert slash (0x2F) to backslash (0x5C)
unsigned char AsciiToUpperTable_Slash[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static const char *AuthCodeArray[] =
{
	// Starcraft II (Heart of the Swarm)
	// Authentication code URL: http://dist.blizzard.com/mediakey/hots-authenticationcode-bgdl.txt
	//                                                                                          -0C-    -1C--08-    -18--04-    -14--00-    -10-
	"S48B6CDTN5XEQAKQDJNDLJBJ73FDFM3U",         // SC2 Heart of the Swarm-all : "expand 32-byte kQAKQ0000FM3UN5XE000073FD6CDT0000LJBJS48B0000DJND"

	// Diablo III: Agent.exe (1.0.0.954)
	// Address of decryption routine: 00502b00                             
	// Pointer to decryptor object: ECX
	// Pointer to key: ECX+0x5C
	// Authentication code URL: http://dist.blizzard.com/mediakey/d3-authenticationcode-enGB.txt
	//                                                                                           -0C-    -1C--08-    -18--04-    -14--00-    -10-
	"UCMXF6EJY352EFH4XFRXCFH2XC9MQRZK",         // Diablo III Installer (deDE): "expand 32-byte kEFH40000QRZKY3520000XC9MF6EJ0000CFH2UCMX0000XFRX"
	"MMKVHY48RP7WXP4GHYBQ7SL9J9UNPHBP",         // Diablo III Installer (enGB): "expand 32-byte kXP4G0000PHBPRP7W0000J9UNHY4800007SL9MMKV0000HYBQ"
	"8MXLWHQ7VGGLTZ9MQZQSFDCLJYET3CPP",         // Diablo III Installer (enSG): "expand 32-byte kTZ9M00003CPPVGGL0000JYETWHQ70000FDCL8MXL0000QZQS"
	"EJ2R5TM6XFE2GUNG5QDGHKQ9UAKPWZSZ",         // Diablo III Installer (enUS): "expand 32-byte kGUNG0000WZSZXFE20000UAKP5TM60000HKQ9EJ2R00005QDG"
	"PBGFBE42Z6LNK65UGJQ3WZVMCLP4HQQT",         // Diablo III Installer (esES): "expand 32-byte kK65U0000HQQTZ6LN0000CLP4BE420000WZVMPBGF0000GJQ3"
	"X7SEJJS9TSGCW5P28EBSC47AJPEY8VU2",         // Diablo III Installer (esMX): "expand 32-byte kW5P200008VU2TSGC0000JPEYJJS90000C47AX7SE00008EBS"
	"5KVBQA8VYE6XRY3DLGC5ZDE4XS4P7YA2",         // Diablo III Installer (frFR): "expand 32-byte kRY3D00007YA2YE6X0000XS4PQA8V0000ZDE45KVB0000LGC5"
	"478JD2K56EVNVVY4XX8TDWYT5B8KB254",         // Diablo III Installer (itIT): "expand 32-byte kVVY40000B2546EVN00005B8KD2K50000DWYT478J0000XX8T"
	"8TS4VNFQRZTN6YWHE9CHVDH9NVWD474A",         // Diablo III Installer (koKR): "expand 32-byte k6YWH0000474ARZTN0000NVWDVNFQ0000VDH98TS40000E9CH"
	"LJ52Z32DF4LZ4ZJJXVKK3AZQA6GABLJB",         // Diablo III Installer (plPL): "expand 32-byte k4ZJJ0000BLJBF4LZ0000A6GAZ32D00003AZQLJ520000XVKK"
	"K6BDHY2ECUE2545YKNLBJPVYWHE7XYAG",         // Diablo III Installer (ptBR): "expand 32-byte k545Y0000XYAGCUE20000WHE7HY2E0000JPVYK6BD0000KNLB"
	"NDVW8GWLAYCRPGRNY8RT7ZZUQU63VLPR",         // Diablo III Installer (ruRU): "expand 32-byte kXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	"6VWCQTN8V3ZZMRUCZXV8A8CGUX2TAA8H",         // Diablo III Installer (zhTW): "expand 32-byte kMRUC0000AA8HV3ZZ0000UX2TQTN80000A8CG6VWC0000ZXV8"
//  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",         // Diablo III Installer (zhCN): "expand 32-byte kXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

	// Starcraft II (Wings of Liberty): Installer.exe (4.1.1.4219)
	// Address of decryption routine: 0053A3D0
	// Pointer to decryptor object: ECX
	// Pointer to key: ECX+0x5C
	// Authentication code URL: http://dist.blizzard.com/mediakey/sc2-authenticationcode-enUS.txt
	//                                                                                          -0C-    -1C--08-    -18--04-    -14--00-    -10-
	"Y45MD3CAK4KXSSXHYD9VY64Z8EKJ4XFX",         // SC2 Wings of Liberty (deDE): "expand 32-byte kSSXH00004XFXK4KX00008EKJD3CA0000Y64ZY45M0000YD9V"
	"G8MN8UDG6NA2ANGY6A3DNY82HRGF29ZH",         // SC2 Wings of Liberty (enGB): "expand 32-byte kANGY000029ZH6NA20000HRGF8UDG0000NY82G8MN00006A3D"
	"W9RRHLB2FDU9WW5B3ECEBLRSFWZSF7HW",         // SC2 Wings of Liberty (enSG): "expand 32-byte kWW5B0000F7HWFDU90000FWZSHLB20000BLRSW9RR00003ECE"
	"3DH5RE5NVM5GTFD85LXGWT6FK859ETR5",         // SC2 Wings of Liberty (enUS): "expand 32-byte kTFD80000ETR5VM5G0000K859RE5N0000WT6F3DH500005LXG"
	"8WLKUAXE94PFQU4Y249PAZ24N4R4XKTQ",         // SC2 Wings of Liberty (esES): "expand 32-byte kQU4Y0000XKTQ94PF0000N4R4UAXE0000AZ248WLK0000249P"
	"A34DXX3VHGGXSQBRFE5UFFDXMF9G4G54",         // SC2 Wings of Liberty (esMX): "expand 32-byte kSQBR00004G54HGGX0000MF9GXX3V0000FFDXA34D0000FE5U"
	"ZG7J9K938HJEFWPQUA768MA2PFER6EAJ",         // SC2 Wings of Liberty (frFR): "expand 32-byte kFWPQ00006EAJ8HJE0000PFER9K9300008MA2ZG7J0000UA76"
	"NE7CUNNNTVAPXV7E3G2BSVBWGVMW8BL2",         // SC2 Wings of Liberty (itIT): "expand 32-byte kXV7E00008BL2TVAP0000GVMWUNNN0000SVBWNE7C00003G2B"
	"3V9E2FTMBM9QQWK7U6MAMWAZWQDB838F",         // SC2 Wings of Liberty (koKR): "expand 32-byte kQWK70000838FBM9Q0000WQDB2FTM0000MWAZ3V9E0000U6MA"
	"2NSFB8MELULJ83U6YHA3UP6K4MQD48L6",         // SC2 Wings of Liberty (plPL): "expand 32-byte k83U6000048L6LULJ00004MQDB8ME0000UP6K2NSF0000YHA3"
	"QA2TZ9EWZ4CUU8BMB5WXCTY65F9CSW4E",         // SC2 Wings of Liberty (ptBR): "expand 32-byte kU8BM0000SW4EZ4CU00005F9CZ9EW0000CTY6QA2T0000B5WX"
	"VHB378W64BAT9SH7D68VV9NLQDK9YEGT",         // SC2 Wings of Liberty (ruRU): "expand 32-byte k9SH70000YEGT4BAT0000QDK978W60000V9NLVHB30000D68V"
	"U3NFQJV4M6GC7KBN9XQJ3BRDN3PLD9NE",         // SC2 Wings of Liberty (zhTW): "expand 32-byte k7KBN0000D9NEM6GC0000N3PLQJV400003BRDU3NF00009XQJ"

	NULL
};

static STREAM_INIT StreamBaseInit[2] =
{
	BaseFile_Init,
	BaseNone_Init
};

static const char *szKeyTemplate = "expand 32-byte k000000000000000000000000000000000000000000000000";

static void AllocateFileName(TMPQArchive * ha, TFileEntry * pFileEntry, const char * szFileName)
{
	// Sanity check
	assert(pFileEntry != NULL);

	// If the file name is pseudo file name, free it at this point
	if (IsPseudoFileName(pFileEntry->szFileName, NULL)) {
		if (pFileEntry->szFileName != NULL) {
			STORM_FREE(pFileEntry->szFileName);
		}

		pFileEntry->szFileName = NULL;
	}

	// Only allocate new file name if it's not there yet
	if (pFileEntry->szFileName == NULL) {
		pFileEntry->szFileName = STORM_ALLOC(char, strlen(szFileName) + 1);

		if (pFileEntry->szFileName != NULL) {
			strcpy(pFileEntry->szFileName, szFileName);
		}
	}

#ifdef FULL
	// We also need to create the file name hash
	if (ha->pHetTable != NULL) {
		ULONGLONG AndMask64 = ha->pHetTable->AndMask64;
		ULONGLONG OrMask64 = ha->pHetTable->OrMask64;

		pFileEntry->FileNameHash = (HashStringJenkins(szFileName) & AndMask64) | OrMask64;
	}
#endif
}

static TFileStream *AllocateFileStream(const TCHAR *szFileName, size_t StreamSize, DWORD dwStreamFlags)
{
	TFileStream *pMaster = NULL;
	TFileStream *pStream;
	const TCHAR *szNextFile = szFileName;
	size_t FileNameSize;

	// Sanity check
	assert(StreamSize != 0);

	// The caller can specify chain of files in the following form:
	// C:\archive.MPQ*http://www.server.com/MPQs/archive-server.MPQ
	// In that case, we use the part after "*" as master file name
	while (szNextFile[0] != 0 && szNextFile[0] != _T('*')) {
		szNextFile++;
	}

	FileNameSize = (size_t)((szNextFile - szFileName) * sizeof(TCHAR));

	// If we have a next file, we need to open it as master stream
	// Note that we don't care if the master stream exists or not,
	// If it doesn't, later attempts to read missing file block will fail
	if (szNextFile[0] == _T('*')) {
		// Don't allow another master file in the string
		if (_tcschr(szNextFile + 1, _T('*')) != NULL) {
			SErrSetLastError(ERROR_INVALID_PARAMETER);
			return NULL;
		}
		
		// Open the master file
		pMaster = FileStream_OpenFile(szNextFile + 1, STREAM_FLAG_READ_ONLY);
	}

	// Allocate the stream structure for the given stream type
	pStream = (TFileStream *)STORM_ALLOC(BYTE, StreamSize + FileNameSize + sizeof(TCHAR));

	if (pStream != NULL) {
		// Zero the entire structure
		memset(pStream, 0, StreamSize);
		pStream->pMaster = pMaster;
		pStream->dwFlags = dwStreamFlags;

		// Initialize the file name
		pStream->szFileName = (TCHAR *)((BYTE *)pStream + StreamSize);
		memcpy(pStream->szFileName, szFileName, FileNameSize);
		pStream->szFileName[FileNameSize / sizeof(TCHAR)] = 0;

		// Initialize the stream functions
		StreamBaseInit[dwStreamFlags & 0x03](pStream);
	}

	return pStream;
}

// Allocates sector offset table
static int AllocatePatchInfo(TMPQFile *hf, BOOL bLoadFromFile)
{
	TMPQArchive *ha = hf->ha;
	DWORD dwLength = sizeof(TPatchInfo);

	// The following conditions must be true
	assert(hf->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE);
	assert(hf->pPatchInfo == NULL);

__AllocateAndLoadPatchInfo:

	// Allocate space for patch header. Start with default size,
	// and if its size if bigger, then we reload them
	hf->pPatchInfo = STORM_ALLOC(TPatchInfo, 1);

	if (hf->pPatchInfo == NULL) {
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	// Do we have to load the patch header from the file ?
	if (bLoadFromFile) {
		// Load the patch header
		if (!FileStream_Read(ha->pStream, &hf->RawFilePos, hf->pPatchInfo, dwLength)) {
			// Free the patch info
			STORM_FREE(hf->pPatchInfo);
			hf->pPatchInfo = NULL;
			return GetLastError();
		}

		// Perform necessary swapping
		hf->pPatchInfo->dwLength = hf->pPatchInfo->dwLength;
		hf->pPatchInfo->dwFlags = hf->pPatchInfo->dwFlags;
		hf->pPatchInfo->dwDataSize = hf->pPatchInfo->dwDataSize;

		// Verify the size of the patch header
		// If it's not default size, we have to reload them
		if (hf->pPatchInfo->dwLength > dwLength) {
			// Free the patch info
			dwLength = hf->pPatchInfo->dwLength;
			STORM_FREE(hf->pPatchInfo);
			hf->pPatchInfo = NULL;

			// If the length is out of all possible ranges, fail the operation
			if (dwLength > 0x400) {
				return ERROR_FILE_CORRUPT;
			}

			goto __AllocateAndLoadPatchInfo;
		}

		// Patch file data size according to the patch header
		hf->dwDataSize = hf->pPatchInfo->dwDataSize;
	} else {
		memset(hf->pPatchInfo, 0, dwLength);
	}

	// Save the final length to the patch header
	hf->pPatchInfo->dwLength = dwLength;
	hf->pPatchInfo->dwFlags  = 0x80000000;

	return ERROR_SUCCESS;
}

// Allocates sector buffer and sector offset table
static int AllocateSectorBuffer(TMPQFile *hf)
{
	TMPQArchive *ha = hf->ha;

	// Caller of AllocateSectorBuffer must ensure these
	assert(hf->pbFileSector == NULL);
	assert(hf->pFileEntry != NULL);
	assert(hf->ha != NULL);

	// Don't allocate anything if the file has zero size
	if (hf->pFileEntry->dwFileSize == 0 || hf->dwDataSize == 0) {
		return ERROR_SUCCESS;
	}

	// Determine the file sector size and allocate buffer for it
	hf->dwSectorSize = (hf->pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT) ? hf->dwDataSize : ha->dwSectorSize;
	hf->pbFileSector = STORM_ALLOC(BYTE, hf->dwSectorSize);
	hf->dwSectorOffs = SFILE_INVALID_POS;

	// Return result
	return (hf->pbFileSector != NULL) ? (int)ERROR_SUCCESS : (int)ERROR_NOT_ENOUGH_MEMORY;
}

static int AllocateSectorChecksums(TMPQFile *hf, BOOL bLoadFromFile)
{
	TMPQArchive *ha = hf->ha;
	TFileEntry *pFileEntry = hf->pFileEntry;
	ULONGLONG RawFilePos;
	DWORD dwCompressedSize = 0;
	DWORD dwExpectedSize;
	DWORD dwCrcOffset;                      // Offset of the CRC table, relative to file offset in the MPQ
	DWORD dwCrcSize;

	// Caller of AllocateSectorChecksums must ensure these
	assert(hf->SectorChksums == NULL);
	assert(hf->SectorOffsets != NULL);
	assert(hf->pFileEntry != NULL);
	assert(hf->ha != NULL);

	// Single unit files don't have sector checksums
	if (pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT) {
		return ERROR_SUCCESS;
	}

	// Caller must ensure that we are only called when we have sector checksums
	assert(pFileEntry->dwFlags & MPQ_FILE_SECTOR_CRC);

	//
	// Older MPQs store an array of CRC32's after
	// the raw file data in the MPQ.
	//
	// In newer MPQs, the (since Cataclysm BETA) the (attributes) file
	// contains additional 32-bit values beyond the sector table.
	// Their number depends on size of the (attributes), but their
	// meaning is unknown. They are usually zeroed in retail game files,
	// but contain some sort of checksum in BETA MPQs
	//

	// Does the size of the file table match with the CRC32-based checksums?
	dwExpectedSize = (hf->dwSectorCount + 2) * sizeof(DWORD);

	if (hf->SectorOffsets[0] != 0 && hf->SectorOffsets[0] == dwExpectedSize) {
		// If we are not loading from the MPQ file, we just allocate the sector table
		// In that case, do not check any sizes
		if (!bLoadFromFile) {
			hf->SectorChksums = STORM_ALLOC(DWORD, hf->dwSectorCount);

			if (hf->SectorChksums == NULL) {
				return ERROR_NOT_ENOUGH_MEMORY;
			}

			// Fill the checksum table with zeros
			memset(hf->SectorChksums, 0, hf->dwSectorCount * sizeof(DWORD));
			return ERROR_SUCCESS;
		} else {
			// Is there valid size of the sector checksums?
			if (hf->SectorOffsets[hf->dwSectorCount + 1] >= hf->SectorOffsets[hf->dwSectorCount]) {
				dwCompressedSize = hf->SectorOffsets[hf->dwSectorCount + 1] - hf->SectorOffsets[hf->dwSectorCount];
			}

			// Ignore cases when the length is too small or too big.
			if (dwCompressedSize < sizeof(DWORD) || dwCompressedSize > hf->dwSectorSize) {
				return ERROR_SUCCESS;
			}

			// Calculate offset of the CRC table
			dwCrcSize = hf->dwSectorCount * sizeof(DWORD);
			dwCrcOffset = hf->SectorOffsets[hf->dwSectorCount];
			RawFilePos = CalculateRawSectorOffset(hf, dwCrcOffset);

			// Now read the table from the MPQ
			hf->SectorChksums = (DWORD *)LoadMpqTable(ha, RawFilePos, dwCompressedSize, dwCrcSize, 0, NULL);
			if (hf->SectorChksums == NULL) {
				return ERROR_NOT_ENOUGH_MEMORY;
			}
		}
	}

	// If the size doesn't match, we ignore sector checksums
//  assert(false);
	return ERROR_SUCCESS;
}

// Allocates sector offset table
static int AllocateSectorOffsets(TMPQFile *hf, BOOL bLoadFromFile)
{
	TMPQArchive *ha = hf->ha;
	TFileEntry *pFileEntry = hf->pFileEntry;
	DWORD dwSectorOffsLen;
	BOOL bSectorOffsetTableCorrupt = FALSE;

	// Caller of AllocateSectorOffsets must ensure these
	assert(hf->SectorOffsets == NULL);
	assert(hf->pFileEntry != NULL);
	assert(hf->dwDataSize != 0);
	assert(hf->ha != NULL);

	// If the file is stored as single unit, just set number of sectors to 1
	if (pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT) {
		hf->dwSectorCount = 1;
		return ERROR_SUCCESS;
	}

	// Calculate the number of data sectors
	// Note that this doesn't work if the file size is zero
	hf->dwSectorCount = ((hf->dwDataSize - 1) / hf->dwSectorSize) + 1;

	// Calculate the number of file sectors
	dwSectorOffsLen = (hf->dwSectorCount + 1) * sizeof(DWORD);

	// If MPQ_FILE_SECTOR_CRC flag is set, there will either be extra DWORD
	// or an array of MD5's. Either way, we read at least 4 bytes more
	// in order to save additional read from the file.
	if (pFileEntry->dwFlags & MPQ_FILE_SECTOR_CRC) {
		dwSectorOffsLen += sizeof(DWORD);
	}

	// Only allocate and load the table if the file is compressed
	if (pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK) {
__LoadSectorOffsets:

		// Allocate the sector offset table
		hf->SectorOffsets = STORM_ALLOC(DWORD, (dwSectorOffsLen / sizeof(DWORD)));

		if (hf->SectorOffsets == NULL) {
			return ERROR_NOT_ENOUGH_MEMORY;
		}

		// Only read from the file if we are supposed to do so
		if (bLoadFromFile) {
			ULONGLONG RawFilePos = hf->RawFilePos;

			// Append the length of the patch info, if any
			if (hf->pPatchInfo != NULL) {
				if ((RawFilePos + hf->pPatchInfo->dwLength) < RawFilePos) {
					return ERROR_FILE_CORRUPT;
				}

				RawFilePos += hf->pPatchInfo->dwLength;
			}

			// Load the sector offsets from the file
			if (!FileStream_Read(ha->pStream, &RawFilePos, hf->SectorOffsets, dwSectorOffsLen)) {
				// Free the sector offsets
				STORM_FREE(hf->SectorOffsets);
				hf->SectorOffsets = NULL;
				return GetLastError();
			}

			// Decrypt loaded sector positions if necessary
			if (pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED) {
				// If we don't know the file key, try to find it.
				if (hf->dwFileKey == 0) {
					hf->dwFileKey = DetectFileKeyBySectorSize(hf->SectorOffsets, ha->dwSectorSize, dwSectorOffsLen);

					if (hf->dwFileKey == 0) {
						STORM_FREE(hf->SectorOffsets);
						hf->SectorOffsets = NULL;
						return ERROR_UNKNOWN_FILE_KEY;
					}
				}

				// Decrypt sector positions
				DecryptMpqBlock(hf->SectorOffsets, dwSectorOffsLen, hf->dwFileKey - 1);
			}

			//
			// Validate the sector offset table
			//
			// Note: Some MPQ protectors put the actual file data before the sector offset table.
			// In this case, the sector offsets are negative (> 0x80000000).
			//

			for (DWORD i = 0; i < hf->dwSectorCount; i++) {
				DWORD dwSectorOffset1 = hf->SectorOffsets[i+1];
				DWORD dwSectorOffset0 = hf->SectorOffsets[i];

				// Every following sector offset must be bigger than the previous one
				if (dwSectorOffset1 <= dwSectorOffset0) {
					bSectorOffsetTableCorrupt = TRUE;
					break;
				}

				// The sector size must not be bigger than compressed file size
				// Edit: Yes, but apparently, in original Storm.dll, the compressed
				// size is not checked anywhere. However, we need to do this check
				// in order to sector offset table malformed by MPQ protectors
				if ((dwSectorOffset1 - dwSectorOffset0) > ha->dwSectorSize) {
					bSectorOffsetTableCorrupt = TRUE;
					break;
				}
			}

			// If data corruption detected, free the sector offset table
			if (bSectorOffsetTableCorrupt) {
				STORM_FREE(hf->SectorOffsets);
				hf->SectorOffsets = NULL;
				return ERROR_FILE_CORRUPT;
			}

			//
			// There may be various extra DWORDs loaded after the sector offset table.
			// They are mostly empty on WoW release MPQs, but on MPQs from PTR,
			// they contain random non-zero data. Their meaning is unknown.
			//
			// These extra values are, however, include in the dwCmpSize in the file
			// table. We cannot ignore them, because compacting archive would fail
			//

			if (hf->SectorOffsets[0] > dwSectorOffsLen) {
				// MPQ protectors put some ridiculous values there. We must limit the extra bytes
				if (hf->SectorOffsets[0] > (dwSectorOffsLen + 0x400)) {
					return ERROR_FILE_CORRUPT;
				}

				// Free the old sector offset table
				dwSectorOffsLen = hf->SectorOffsets[0];
				STORM_FREE(hf->SectorOffsets);
				goto __LoadSectorOffsets;
			}
		} else {
			memset(hf->SectorOffsets, 0, dwSectorOffsLen);
			hf->SectorOffsets[0] = dwSectorOffsLen;
		}
	}

	return ERROR_SUCCESS;
}

static void BaseFile_Close(TFileStream *pStream)
{
	if (pStream->Base.File.hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(pStream->Base.File.hFile);
	}

	// Also invalidate the handle
	pStream->Base.File.hFile = INVALID_HANDLE_VALUE;
}

static BOOL BaseFile_Create(TFileStream *pStream)
{
	DWORD dwWriteShare = (pStream->dwFlags & STREAM_FLAG_WRITE_SHARE) ? FILE_SHARE_WRITE : 0;

	pStream->Base.File.hFile = CreateFile(pStream->szFileName,
										GENERIC_READ | GENERIC_WRITE,
										dwWriteShare | FILE_SHARE_READ,
										NULL,
										CREATE_ALWAYS,
										0,
										NULL);
	if (pStream->Base.File.hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	// Reset the file size and position
	pStream->Base.File.FileSize = 0;
	pStream->Base.File.FilePos = 0;
	return TRUE;
}

// Gives the current file position
static BOOL BaseFile_GetPos(TFileStream *pStream, ULONGLONG *pByteOffset)
{
	// Note: Used by all thre base providers.
	// Requires the TBaseData union to have the same layout for all three base providers
	*pByteOffset = pStream->Base.File.FilePos;
	return TRUE;
}

// Gives the current file size
static BOOL BaseFile_GetSize(TFileStream *pStream, ULONGLONG *pFileSize)
{
	// Note: Used by all thre base providers.
	// Requires the TBaseData union to have the same layout for all three base providers
	*pFileSize = pStream->Base.File.FileSize;
	return TRUE;
}

// Initializes base functions for the disk file
static void BaseFile_Init(TFileStream *pStream)
{
	pStream->BaseCreate  = BaseFile_Create;
	pStream->BaseOpen    = BaseFile_Open;
	pStream->BaseRead    = BaseFile_Read;
	pStream->BaseWrite   = BaseFile_Write;
	pStream->BaseResize  = BaseFile_Resize;
	pStream->BaseGetSize = BaseFile_GetSize;
	pStream->BaseGetPos  = BaseFile_GetPos;
	pStream->BaseClose   = BaseFile_Close;
}

static BOOL BaseFile_Open(TFileStream *pStream, const TCHAR *szFileName, DWORD dwStreamFlags)
{
	ULARGE_INTEGER FileSize;
	DWORD dwWriteAccess = (dwStreamFlags & STREAM_FLAG_READ_ONLY) ? 0 : FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_ATTRIBUTES;
	DWORD dwWriteShare = (dwStreamFlags & STREAM_FLAG_WRITE_SHARE) ? FILE_SHARE_WRITE : 0;

	// Open the file
	pStream->Base.File.hFile = CreateFile(szFileName,
											FILE_READ_DATA | FILE_READ_ATTRIBUTES | dwWriteAccess,
											FILE_SHARE_READ | dwWriteShare,
											NULL,
											OPEN_EXISTING,
											0,
											NULL);
	if (pStream->Base.File.hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	// Query the file size
	FileSize.LowPart = GetFileSize(pStream->Base.File.hFile, &FileSize.HighPart);
	pStream->Base.File.FileSize = FileSize.QuadPart;

	// Query last write time
	GetFileTime(pStream->Base.File.hFile, NULL, NULL, (LPFILETIME)&pStream->Base.File.FileTime);

	// Reset the file position
	pStream->Base.File.FilePos = 0;
	return TRUE;
}

static BOOL BaseFile_Read(
	TFileStream *pStream,                   // Pointer to an open stream
	ULONGLONG *pByteOffset,                 // Pointer to file byte offset. If NULL, it reads from the current position
	void *pvBuffer,                         // Pointer to data to be read
	DWORD dwBytesToRead)                    // Number of bytes to read from the file
{
	ULONGLONG ByteOffset = (pByteOffset != NULL) ? *pByteOffset : pStream->Base.File.FilePos;
	DWORD dwBytesRead = 0;                  // Must be set by platform-specific code

	// Note: StormLib no longer supports Windows 9x.
	// Thus, we can use the OVERLAPPED structure to specify
	// file offset to read from file. This allows us to skip
	// one system call to SetFilePointer

	// Update the byte offset
	pStream->Base.File.FilePos = ByteOffset;

	// Read the data
	if (dwBytesToRead != 0) {
		OVERLAPPED Overlapped;

		Overlapped.OffsetHigh = (DWORD)(ByteOffset >> 32);
		Overlapped.Offset = (DWORD)ByteOffset;
		Overlapped.hEvent = NULL;

		if (!ReadFile(pStream->Base.File.hFile, pvBuffer, dwBytesToRead, &dwBytesRead, &Overlapped)) {
			return FALSE;
		}
	}

	// Increment the current file position by number of bytes read
	// If the number of bytes read doesn't match to required amount, return false
	pStream->Base.File.FilePos = ByteOffset + dwBytesRead;

	if (dwBytesRead != dwBytesToRead) {
		SErrSetLastError(ERROR_HANDLE_EOF);
	}

	return (dwBytesRead == dwBytesToRead);
}

/**
 * \a pStream Pointer to an open stream
 * \a NewFileSize New size of the file
 */
static BOOL BaseFile_Resize(TFileStream *pStream, ULONGLONG NewFileSize)
{
	LONG FileSizeHi = (LONG)(NewFileSize >> 32);
	LONG FileSizeLo;
	DWORD dwNewPos;
	BOOL bResult;

	// Set the position at the new file size
	dwNewPos = SetFilePointer(pStream->Base.File.hFile, (LONG)NewFileSize, &FileSizeHi, FILE_BEGIN);

	if (dwNewPos == INVALID_SET_FILE_POINTER && GetLastError() != ERROR_SUCCESS) {
		return FALSE;
	}

	// Set the current file pointer as the end of the file
	bResult = SetEndOfFile(pStream->Base.File.hFile);

	if (bResult) {
		pStream->Base.File.FileSize = NewFileSize;
	}

	// Restore the file position
	FileSizeHi = (LONG)(pStream->Base.File.FilePos >> 32);
	FileSizeLo = (LONG)(pStream->Base.File.FilePos);
	SetFilePointer(pStream->Base.File.hFile, FileSizeLo, &FileSizeHi, FILE_BEGIN);
	return bResult;
}

static BOOL BaseFile_Write(TFileStream *pStream, ULONGLONG *pByteOffset, const void *pvBuffer, DWORD dwBytesToWrite)
{
	ULONGLONG ByteOffset = (pByteOffset != NULL) ? *pByteOffset : pStream->Base.File.FilePos;
	DWORD dwBytesWritten = 0;               // Must be set by platform-specific code

	// Note: StormLib no longer supports Windows 9x.
	// Thus, we can use the OVERLAPPED structure to specify
	// file offset to read from file. This allows us to skip
	// one system call to SetFilePointer

	// Update the byte offset
	pStream->Base.File.FilePos = ByteOffset;

	// Read the data
	if (dwBytesToWrite != 0) {
		OVERLAPPED Overlapped;

		Overlapped.OffsetHigh = (DWORD)(ByteOffset >> 32);
		Overlapped.Offset = (DWORD)ByteOffset;
		Overlapped.hEvent = NULL;

		if (!WriteFile(pStream->Base.File.hFile, pvBuffer, dwBytesToWrite, &dwBytesWritten, &Overlapped)) {
			return FALSE;
		}
	}

	// Increment the current file position by number of bytes read
	pStream->Base.File.FilePos = ByteOffset + dwBytesWritten;

	// Also modify the file size, if needed
	if (pStream->Base.File.FilePos > pStream->Base.File.FileSize) {
		pStream->Base.File.FileSize = pStream->Base.File.FilePos;
	}

	if (dwBytesWritten != dwBytesToWrite) {
		SErrSetLastError(ERROR_DISK_FULL);
	}

	return (dwBytesWritten == dwBytesToWrite);
}

static void BaseNone_Init(TFileStream *)
{
	// Nothing here
}

static void BlockStream_Close(TBlockStream *pStream)
{
	// Free the data map, if any
	if (pStream->FileBitmap != NULL) {
		STORM_FREE(pStream->FileBitmap);
	}

	pStream->FileBitmap = NULL;

	// Call the base class for closing the stream
	pStream->BaseClose(pStream);
}

static int BuildFileTable_Classic(TMPQArchive *ha)
{
	TMPQHeader *pHeader = ha->pHeader;
	TMPQBlock *pBlockTable;
	int nError = ERROR_SUCCESS;

	// Sanity checks
	assert(ha->pHashTable != NULL);
	assert(ha->pFileTable != NULL);

	// If the MPQ has no block table, do nothing
	if (pHeader->dwBlockTableSize == 0) {
		return ERROR_SUCCESS;
	}

	assert(ha->dwFileTableSize >= pHeader->dwBlockTableSize);

	// Load the block table
	// WARNING! ha->pFileTable can change in the process!!
	pBlockTable = (TMPQBlock *)LoadBlockTable(ha);

	if (pBlockTable != NULL) {
		nError = BuildFileTableFromBlockTable(ha, pBlockTable);
		STORM_FREE(pBlockTable);
	} else {
		nError = ERROR_NOT_ENOUGH_MEMORY;
	}

	// Load the hi-block table
	if (nError == ERROR_SUCCESS && pHeader->HiBlockTablePos64 != 0) {
		ULONGLONG ByteOffset;
		USHORT * pHiBlockTable = NULL;
		DWORD dwTableSize = pHeader->dwBlockTableSize * sizeof(USHORT);

		// Allocate space for the hi-block table
		// Note: pHeader->dwBlockTableSize can be zero !!!
		pHiBlockTable = STORM_ALLOC(USHORT, pHeader->dwBlockTableSize + 1);

		if (pHiBlockTable != NULL) {
			// Load the hi-block table. It is not encrypted, nor compressed
			ByteOffset = ha->MpqPos + pHeader->HiBlockTablePos64;
			if (!FileStream_Read(ha->pStream, &ByteOffset, pHiBlockTable, dwTableSize)) {
				nError = GetLastError();
			}

			// Now merge the hi-block table to the file table
			if (nError == ERROR_SUCCESS) {
				TFileEntry * pFileEntry = ha->pFileTable;

				// Add the high file offset to the base file offset.
				for (DWORD i = 0; i < pHeader->dwBlockTableSize; i++, pFileEntry++) {
					pFileEntry->ByteOffset = MAKE_OFFSET64(pHiBlockTable[i], pFileEntry->ByteOffset);
				}
			}

			// Free the hi-block table
			STORM_FREE(pHiBlockTable);
		} else {
			nError = ERROR_NOT_ENOUGH_MEMORY;
		}
	}

	return nError;
}

static int BuildFileTable(TMPQArchive * ha)
{
	DWORD dwFileTableSize;
	bool bFileTableCreated = false;

	// Sanity checks
	assert(ha->pFileTable == NULL);
	assert(ha->dwFileTableSize == 0);
	assert(ha->dwMaxFileCount != 0);

	// Determine the allocation size for the file table
	dwFileTableSize = max(ha->pHeader->dwBlockTableSize, ha->dwMaxFileCount);

	// Allocate the file table with size determined before
	ha->pFileTable = STORM_ALLOC(TFileEntry, dwFileTableSize);

	if (ha->pFileTable == NULL) {
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	// Fill the table with zeros
	memset(ha->pFileTable, 0, dwFileTableSize * sizeof(TFileEntry));
	ha->dwFileTableSize = dwFileTableSize;

#ifdef FULL
	// If we have HET table, we load file table from the BET table
	// Note: If BET table is corrupt or missing, we set the archive as read only
	if (ha->pHetTable != NULL) {
		if (BuildFileTable_HetBet(ha) != ERROR_SUCCESS) {
			ha->dwFlags |= MPQ_FLAG_READ_ONLY;
		} else {
			bFileTableCreated = true;
		}
	}
#endif

	// If we have hash table, we load the file table from the block table
	// Note: If block table is corrupt or missing, we set the archive as read only
	if (ha->pHashTable != NULL) {
		if (BuildFileTable_Classic(ha) != ERROR_SUCCESS) {
			ha->dwFlags |= MPQ_FLAG_READ_ONLY;
		} else {
			bFileTableCreated = true;
		}
	}

	// Return result
	return bFileTableCreated ? ERROR_SUCCESS : ERROR_FILE_CORRUPT;
}

static DWORD DetectFileKeyByContent(void *pvEncryptedData, DWORD dwSectorSize, DWORD dwFileSize)
{
	DWORD dwFileKey;

	// Try to break the file encryption key as if it was a WAVE file
	if (dwSectorSize >= 0x0C) {
		dwFileKey = DetectFileKeyByKnownContent(pvEncryptedData, 0x46464952, dwFileSize - 8);

		if (dwFileKey != 0) {
			return dwFileKey;
		}
	}

	// Try to break the encryption key as if it was an EXE file
	if (dwSectorSize > 0x40) {
		dwFileKey = DetectFileKeyByKnownContent(pvEncryptedData, 0x00905A4D, 0x00000003);

		if (dwFileKey != 0) {
			return dwFileKey;
		}
	}

	// Try to break the encryption key as if it was a XML file
	if (dwSectorSize > 0x04) {
		dwFileKey = DetectFileKeyByKnownContent(pvEncryptedData, 0x6D783F3C, 0x6576206C);

		if (dwFileKey != 0) {
			return dwFileKey;
		}
	}

	// Not detected, sorry
	return 0;
}

// Function tries to detect file encryption key based on expected file content
// It is the same function like before, except that we know the value of the second DWORD
static DWORD DetectFileKeyByKnownContent(void *pvEncryptedData, DWORD dwDecrypted0, DWORD dwDecrypted1)
{
	LPDWORD EncryptedData = (LPDWORD)pvEncryptedData;
	DWORD dwKey1PlusKey2;
	DWORD DataBlock[2];

	// Get the value of the combined encryption key
	dwKey1PlusKey2 = (EncryptedData[0] ^ dwDecrypted0) - 0xEEEEEEEE;

	// Try all 256 combinations of dwKey1
	for (DWORD i = 0; i < 0x100; i++) {
		DWORD dwSaveKey1;
		DWORD dwKey1 = dwKey1PlusKey2 - StormBuffer[MPQ_HASH_KEY2_MIX + i];
		DWORD dwKey2 = 0xEEEEEEEE;

		// Modify the second key and decrypt the first DWORD
		dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
		DataBlock[0] = EncryptedData[0] ^ (dwKey1 + dwKey2);

		// Did we obtain the same value like dwDecrypted0?
		if (DataBlock[0] == dwDecrypted0) {
			// Save this key value
			dwSaveKey1 = dwKey1;

			// Rotate both keys
			dwKey1 = ((~dwKey1 << 0x15) + 0x11111111) | (dwKey1 >> 0x0B);
			dwKey2 = DataBlock[0] + dwKey2 + (dwKey2 << 5) + 3;

			// Modify the second key again and decrypt the second DWORD
			dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
			DataBlock[1] = EncryptedData[1] ^ (dwKey1 + dwKey2);

			// Now compare the results
			if (DataBlock[1] == dwDecrypted1) {
				return dwSaveKey1;
			}
		}
	}

	// Key not found
	return 0;
}

/**
 * Functions tries to get file decryption key. This comes from these facts
 *
 * - We know the decrypted value of the first DWORD in the encrypted data
 * - We know the decrypted value of the second DWORD (at least aproximately)
 * - There is only 256 variants of how the second key is modified
 *
 *  The first iteration of dwKey1 and dwKey2 is this:
 *
 *  dwKey2 = 0xEEEEEEEE + StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)]
 *  dwDecrypted0 = DataBlock[0] ^ (dwKey1 + dwKey2);
 *
 *  This means:
 *
 *  (dwKey1 + dwKey2) = DataBlock[0] ^ dwDecrypted0;
 *
 */

static DWORD DetectFileKeyBySectorSize(LPDWORD EncryptedData, DWORD dwSectorSize, DWORD dwDecrypted0)
{
	DWORD dwDecrypted1Max = dwSectorSize + dwDecrypted0;
	DWORD dwKey1PlusKey2;
	DWORD DataBlock[2];

	// We must have at least 2 DWORDs there to be able to decrypt something
	if (dwSectorSize < 0x08) {
		return 0;
	}

	// Get the value of the combined encryption key
	dwKey1PlusKey2 = (EncryptedData[0] ^ dwDecrypted0) - 0xEEEEEEEE;

	// Try all 256 combinations of dwKey1
	for (DWORD i = 0; i < 0x100; i++) {
		DWORD dwSaveKey1;
		DWORD dwKey1 = dwKey1PlusKey2 - StormBuffer[MPQ_HASH_KEY2_MIX + i];
		DWORD dwKey2 = 0xEEEEEEEE;

		// Modify the second key and decrypt the first DWORD
		dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
		DataBlock[0] = EncryptedData[0] ^ (dwKey1 + dwKey2);

		// Did we obtain the same value like dwDecrypted0?
		if (DataBlock[0] == dwDecrypted0) {
			// Save this key value. Increment by one because
			// we are decrypting sector offset table
			dwSaveKey1 = dwKey1 + 1;

			// Rotate both keys
			dwKey1 = ((~dwKey1 << 0x15) + 0x11111111) | (dwKey1 >> 0x0B);
			dwKey2 = DataBlock[0] + dwKey2 + (dwKey2 << 5) + 3;

			// Modify the second key again and decrypt the second DWORD
			dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
			DataBlock[1] = EncryptedData[1] ^ (dwKey1 + dwKey2);

			// Now compare the results
			if (DataBlock[1] <= dwDecrypted1Max) {
				return dwSaveKey1;
			}
		}
	}

	// Key not found
	return 0;
}

// Hash entry verification when the file table does not exist yet
static bool IsValidHashEntry1(TMPQArchive *ha, TMPQHash *pHash, TMPQBlock *pBlockTable)
{
	ULONGLONG ByteOffset;
	TMPQBlock *pBlock;

	// The block index is considered valid if it's less than block table size
	if (MPQ_BLOCK_INDEX(pHash) < ha->pHeader->dwBlockTableSize) {
		// Calculate the block table position
		pBlock = pBlockTable + MPQ_BLOCK_INDEX(pHash);

		// Check whether this is an existing file
		// Also we do not allow to be file size greater than 2GB
		if ((pBlock->dwFlags & MPQ_FILE_EXISTS) && (pBlock->dwFSize & 0x80000000) == 0) {
			// The begin of the file must be within the archive
			ByteOffset = FileOffsetFromMpqOffset(ha, pBlock->dwFilePos);
			return (ByteOffset < ha->FileSize);
		}
	}

	return false;
}

static int BuildFileTableFromBlockTable(TMPQArchive *ha, TMPQBlock *pBlockTable)
{
	TFileEntry *pFileEntry;
	TMPQHeader *pHeader = ha->pHeader;
	TMPQBlock *pBlock;
	TMPQHash *pHashTableEnd;
	TMPQHash *pHash;
	LPDWORD DefragmentTable = NULL;
	DWORD dwItemCount = 0;
	DWORD dwFlagMask;

	// Sanity checks
	assert(ha->pFileTable != NULL);
	assert(ha->dwFileTableSize >= ha->dwMaxFileCount);

	// MPQs for Warcraft III doesn't know some flags, namely MPQ_FILE_SINGLE_UNIT and MPQ_FILE_PATCH_FILE
	dwFlagMask = (ha->dwFlags & MPQ_FLAG_WAR3_MAP) ? MPQ_FILE_VALID_FLAGS_W3X : MPQ_FILE_VALID_FLAGS;

	// Defragment the hash table, if needed
	if (ha->dwFlags & MPQ_FLAG_HASH_TABLE_CUT) {
		ha->pHashTable = DefragmentHashTable(ha, ha->pHashTable, pBlockTable);
		ha->dwMaxFileCount = pHeader->dwHashTableSize;
	}

	// If the hash table or block table is cut,
	// we will defragment the block table
	if (ha->dwFlags & (MPQ_FLAG_HASH_TABLE_CUT | MPQ_FLAG_BLOCK_TABLE_CUT)) {
		// Sanity checks
		assert(pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1);
		assert(pHeader->HiBlockTablePos64 == 0);

		// Allocate the translation table
		DefragmentTable = STORM_ALLOC(DWORD, pHeader->dwBlockTableSize);

		if (DefragmentTable == NULL) {
			return ERROR_NOT_ENOUGH_MEMORY;
		}

		// Fill the translation table
		memset(DefragmentTable, 0xFF, pHeader->dwBlockTableSize * sizeof(DWORD));
	}

	// Parse the entire hash table
	pHashTableEnd = ha->pHashTable + pHeader->dwHashTableSize;

	for (pHash = ha->pHashTable; pHash < pHashTableEnd; pHash++) {
		//
		// We need to properly handle these cases:
		// - Multiple hash entries (same file name) point to the same block entry
		// - Multiple hash entries (different file name) point to the same block entry
		//
		// Ignore all hash table entries where:
		// - Block Index >= BlockTableSize
		// - Flags of the appropriate block table entry
		//

		if (IsValidHashEntry1(ha, pHash, pBlockTable)) {
			DWORD dwOldIndex = MPQ_BLOCK_INDEX(pHash);
			DWORD dwNewIndex = MPQ_BLOCK_INDEX(pHash);

			// Determine the new block index
			if (DefragmentTable != NULL) {
				// Need to handle case when multiple hash
				// entries point to the same block entry
				if (DefragmentTable[dwOldIndex] == HASH_ENTRY_FREE) {
					DefragmentTable[dwOldIndex] = dwItemCount;
					dwNewIndex = dwItemCount++;
				} else {
					dwNewIndex = DefragmentTable[dwOldIndex];
				}

				// Fix the pointer in the hash entry
				pHash->dwBlockIndex = dwNewIndex;

				// Dump the relocation entry
//              printf("Relocating hash entry %08X-%08X: %08X -> %08X\n", pHash->dwName1, pHash->dwName2, dwBlockIndex, dwNewIndex);
			}

			// Get the pointer to the file entry and the block entry
			pFileEntry = ha->pFileTable + dwNewIndex;
			pBlock = pBlockTable + dwOldIndex;

			// ByteOffset is only valid if file size is not zero
			pFileEntry->ByteOffset = pBlock->dwFilePos;

			if (pFileEntry->ByteOffset == 0 && pBlock->dwFSize == 0) {
				pFileEntry->ByteOffset = ha->pHeader->dwHeaderSize;
			}

			// Fill the rest of the file entry
			pFileEntry->dwFileSize  = pBlock->dwFSize;
			pFileEntry->dwCmpSize   = pBlock->dwCSize;
			pFileEntry->dwFlags     = pBlock->dwFlags & dwFlagMask;
		}
	}

	// Free the translation table
	if (DefragmentTable != NULL) {
		// If we defragmented the block table in the process,
		// free some memory by shrinking the file table
		if (ha->dwFileTableSize > ha->dwMaxFileCount) {
			ha->pFileTable = STORM_REALLOC(TFileEntry, ha->pFileTable, ha->dwMaxFileCount);
			ha->pHeader->BlockTableSize64 = ha->dwMaxFileCount * sizeof(TMPQBlock);
			ha->pHeader->dwBlockTableSize = ha->dwMaxFileCount;
			ha->dwFileTableSize = ha->dwMaxFileCount;
		}

//      DumpFileTable(ha->pFileTable, ha->dwFileTableSize);

		// Free the translation table
		STORM_FREE(DefragmentTable);
	}

	return ERROR_SUCCESS;
}

static ULONGLONG CalculateRawSectorOffset(TMPQFile *hf, DWORD dwSectorOffset)
{
	ULONGLONG RawFilePos;

	// Must be used for files within a MPQ
	assert(hf->ha != NULL);
	assert(hf->ha->pHeader != NULL);

	//
	// Some MPQ protectors place the sector offset table after the actual file data.
	// Sector offsets in the sector offset table are negative. When added
	// to MPQ file offset from the block table entry, the result is a correct
	// position of the file data in the MPQ.
	//
	// For MPQs version 1.0, the offset is purely 32-bit
	//

	RawFilePos = hf->RawFilePos + dwSectorOffset;


	if (hf->ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1) {
		RawFilePos = (DWORD)hf->ha->MpqPos + (DWORD)hf->pFileEntry->ByteOffset + dwSectorOffset;
	}

	// We also have to add patch header size, if patch header is present
	if (hf->pPatchInfo != NULL) {
		RawFilePos += hf->pPatchInfo->dwLength;
	}

	// Return the result offset
	return RawFilePos;
}

static DWORD ConvertMpkFlagsToMpqFlags(DWORD dwMpkFlags)
{
	DWORD dwMpqFlags = MPQ_FILE_EXISTS;

	// Check for flags that are always present
	assert((dwMpkFlags & MPK_FILE_UNKNOWN_0001) != 0);
	assert((dwMpkFlags & MPK_FILE_UNKNOWN_0010) != 0);
	assert((dwMpkFlags & MPK_FILE_UNKNOWN_2000) != 0);
	assert((dwMpkFlags & MPK_FILE_EXISTS) != 0);

	// Append the compressed flag
	dwMpqFlags |= (dwMpkFlags & MPK_FILE_COMPRESSED) ? MPQ_FILE_COMPRESS : 0;

	// All files in the MPQ seem to be single unit files
	dwMpqFlags |= MPQ_FILE_ENCRYPTED | MPQ_FILE_SINGLE_UNIT;

	return dwMpqFlags;
}

// This function converts MPK file header into MPQ file header
static int ConvertMpkHeaderToFormat4(TMPQArchive *ha, ULONGLONG FileSize, DWORD dwFlags)
{
	TMPKHeader *pMpkHeader = (TMPKHeader *)ha->HeaderData;
	TMPQHeader Header;

	// Can't open the archive with certain flags
	if (dwFlags & MPQ_OPEN_FORCE_MPQ_V1) {
		return ERROR_FILE_CORRUPT;
	}

	// Translate the MPK header into a MPQ header
	// Note: Hash table size and block table size are in bytes, not in entries
	memset(&Header, 0, sizeof(TMPQHeader));
	Header.dwID = pMpkHeader->dwID;
	Header.dwArchiveSize = pMpkHeader->dwArchiveSize;
	Header.dwHeaderSize = pMpkHeader->dwHeaderSize;
	Header.dwHashTablePos = pMpkHeader->dwHashTablePos;
	Header.dwHashTableSize = pMpkHeader->dwHashTableSize / sizeof(TMPKHash);
	Header.dwBlockTablePos = pMpkHeader->dwBlockTablePos;
	Header.dwBlockTableSize = pMpkHeader->dwBlockTableSize / sizeof(TMPKBlock);
//  Header.dwUnknownPos = pMpkHeader->dwUnknownPos;
//  Header.dwUnknownSize = pMpkHeader->dwUnknownSize;
	assert(Header.dwHeaderSize == sizeof(TMPKHeader));

	// Verify the MPK header
	if (Header.dwID == ID_MPK && Header.dwHeaderSize == sizeof(TMPKHeader) && Header.dwArchiveSize == (DWORD)FileSize) {
		// The header ID must be ID_MPQ
		Header.dwID = ID_MPQ;
		Header.wFormatVersion = MPQ_FORMAT_VERSION_1;
		Header.wSectorSize = 3;

		// Initialize the fields of 3.0 header
		Header.ArchiveSize64    = Header.dwArchiveSize;
		Header.HashTableSize64  = Header.dwHashTableSize * sizeof(TMPQHash);
		Header.BlockTableSize64 = Header.dwBlockTableSize * sizeof(TMPQBlock);

		// Copy the converted MPQ header back
		memcpy(ha->HeaderData, &Header, sizeof(TMPQHeader));

		// Mark this file as MPK file
		ha->pfnHashString = HashStringLower;
		ha->dwFlags |= MPQ_FLAG_READ_ONLY;
		ha->dwSubType = MPQ_SUBTYPE_MPK;
		return ERROR_SUCCESS;
	}

	return ERROR_FILE_CORRUPT;
}

// This function converts the MPQ header so it always looks like version 4
static int ConvertMpqHeaderToFormat4(TMPQArchive *ha, ULONGLONG MpqOffset, ULONGLONG FileSize, DWORD dwFlags, BOOL bIsWarcraft3Map)
{
	TMPQHeader *pHeader = (TMPQHeader *)ha->HeaderData;
	ULONGLONG BlockTablePos64 = 0;
	ULONGLONG HashTablePos64 = 0;
	ULONGLONG BlockTableMask = (ULONGLONG)-1;
	ULONGLONG ByteOffset;
	USHORT wFormatVersion = pHeader->wFormatVersion;
	int nError = ERROR_SUCCESS;

	// If version 1.0 is forced, then the format version is forced to be 1.0
	// Reason: Storm.dll in Warcraft III ignores format version value
	if ((dwFlags & MPQ_OPEN_FORCE_MPQ_V1) || bIsWarcraft3Map) {
		wFormatVersion = MPQ_FORMAT_VERSION_1;
	}

	// Format-specific fixes
	switch(wFormatVersion) {
		case MPQ_FORMAT_VERSION_1:
			// Check for malformed MPQ header version 1.0
			if (pHeader->wFormatVersion != MPQ_FORMAT_VERSION_1 || pHeader->dwHeaderSize != MPQ_HEADER_SIZE_V1) {
				pHeader->wFormatVersion = MPQ_FORMAT_VERSION_1;
				pHeader->dwHeaderSize = MPQ_HEADER_SIZE_V1;
				ha->dwFlags |= MPQ_FLAG_MALFORMED;
			}

			//
			// Note: The value of "dwArchiveSize" member in the MPQ header
			// is ignored by Storm.dll and can contain garbage value
			// ("w3xmaster" protector).
			//

			Label_ArchiveVersion1:
			if (pHeader->dwBlockTableSize > 1) { // Prevent empty MPQs being marked as malformed
				if (pHeader->dwHashTablePos <= pHeader->dwHeaderSize || (pHeader->dwHashTablePos & 0x80000000)) {
					ha->dwFlags |= MPQ_FLAG_MALFORMED;
				}

				if (pHeader->dwBlockTablePos <= pHeader->dwHeaderSize || (pHeader->dwBlockTablePos & 0x80000000)) {
					ha->dwFlags |= MPQ_FLAG_MALFORMED;
				}
			}

			// Only low byte of sector size is really used
			if (pHeader->wSectorSize & 0xFF00) {
				ha->dwFlags |= MPQ_FLAG_MALFORMED;
			}

			pHeader->wSectorSize = pHeader->wSectorSize & 0xFF;

			// Fill the rest of the header
			memset((LPBYTE)pHeader + MPQ_HEADER_SIZE_V1, 0, sizeof(TMPQHeader) - MPQ_HEADER_SIZE_V1);
			pHeader->BlockTableSize64 = pHeader->dwBlockTableSize * sizeof(TMPQBlock);
			pHeader->HashTableSize64 = pHeader->dwHashTableSize * sizeof(TMPQHash);
			pHeader->ArchiveSize64 = pHeader->dwArchiveSize;

			// Block table position must be calculated as 32-bit value
			// Note: BOBA protector puts block table before the MPQ header, so it is negative
			BlockTablePos64 = (ULONGLONG)((DWORD)MpqOffset + pHeader->dwBlockTablePos);
			BlockTableMask = 0xFFFFFFF0;

			// Determine the archive size on malformed MPQs
			if (ha->dwFlags & MPQ_FLAG_MALFORMED) {
				// Calculate the archive size
				pHeader->ArchiveSize64 = DetermineArchiveSize_V1(ha, pHeader, MpqOffset, FileSize);
				pHeader->dwArchiveSize = (DWORD)pHeader->ArchiveSize64;
			}
			break;

		case MPQ_FORMAT_VERSION_2:
			// Check for malformed MPQ header version 1.0
			if (pHeader->wFormatVersion != MPQ_FORMAT_VERSION_2 || pHeader->dwHeaderSize != MPQ_HEADER_SIZE_V2) {
				pHeader->wFormatVersion = MPQ_FORMAT_VERSION_1;
				pHeader->dwHeaderSize = MPQ_HEADER_SIZE_V1;
				ha->dwFlags |= MPQ_FLAG_MALFORMED;
				goto Label_ArchiveVersion1;
			}

			// Fill the rest of the header with zeros
			memset((LPBYTE)pHeader + MPQ_HEADER_SIZE_V2, 0, sizeof(TMPQHeader) - MPQ_HEADER_SIZE_V2);

			// Calculate the expected hash table size
			pHeader->HashTableSize64 = (pHeader->dwHashTableSize * sizeof(TMPQHash));
			HashTablePos64 = MAKE_OFFSET64(pHeader->wHashTablePosHi, pHeader->dwHashTablePos);

			// Calculate the expected block table size
			pHeader->BlockTableSize64 = (pHeader->dwBlockTableSize * sizeof(TMPQBlock));
			BlockTablePos64 = MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos);

			// We require the block table to follow hash table
			if (BlockTablePos64 >= HashTablePos64) {
				// HashTableSize64 may be less than TblSize * sizeof(TMPQHash).
				// That means that the hash table is compressed.
				pHeader->HashTableSize64 = BlockTablePos64 - HashTablePos64;

				// Calculate the compressed block table size
				if (pHeader->HiBlockTablePos64 != 0) {
					// BlockTableSize64 may be less than TblSize * sizeof(TMPQBlock).
					// That means that the block table is compressed.
					pHeader->BlockTableSize64 = pHeader->HiBlockTablePos64 - BlockTablePos64;
					assert(pHeader->BlockTableSize64 <= (pHeader->dwBlockTableSize * sizeof(TMPQBlock)));

					// Determine real archive size
					pHeader->ArchiveSize64 = DetermineArchiveSize_V2(pHeader, MpqOffset, FileSize);

					// Calculate the size of the hi-block table
					pHeader->HiBlockTableSize64 = pHeader->ArchiveSize64 - pHeader->HiBlockTablePos64;
					assert(pHeader->HiBlockTableSize64 == (pHeader->dwBlockTableSize * sizeof(USHORT)));
				} else {
					// Determine real archive size
					pHeader->ArchiveSize64 = DetermineArchiveSize_V2(pHeader, MpqOffset, FileSize);

					// Calculate size of the block table
					pHeader->BlockTableSize64 = pHeader->ArchiveSize64 - BlockTablePos64;
					assert(pHeader->BlockTableSize64 <= (pHeader->dwBlockTableSize * sizeof(TMPQBlock)));
				}
			} else {
				pHeader->ArchiveSize64 = pHeader->dwArchiveSize;
				ha->dwFlags |= MPQ_FLAG_MALFORMED;
			}

			// Add the MPQ Offset
			BlockTablePos64 += MpqOffset;
			break;

		case MPQ_FORMAT_VERSION_3:
			// In MPQ format 3.0, the entire header is optional
			// and the size of the header can actually be identical
			// to size of header 2.0
			if (pHeader->dwHeaderSize < MPQ_HEADER_SIZE_V3) {
				pHeader->ArchiveSize64 = pHeader->dwArchiveSize;
				pHeader->HetTablePos64 = 0;
				pHeader->BetTablePos64 = 0;
			}

			//
			// We need to calculate the compressed size of each table. We assume the following order:
			// 1) HET table
			// 2) BET table
			// 3) Classic hash table
			// 4) Classic block table
			// 5) Hi-block table
			//

			// Fill the rest of the header with zeros
			memset((LPBYTE)pHeader + MPQ_HEADER_SIZE_V3, 0, sizeof(TMPQHeader) - MPQ_HEADER_SIZE_V3);
			BlockTablePos64 = MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos);
			HashTablePos64 = MAKE_OFFSET64(pHeader->wHashTablePosHi, pHeader->dwHashTablePos);
			ByteOffset = pHeader->ArchiveSize64;

			// Size of the hi-block table
			if (pHeader->HiBlockTablePos64) {
				pHeader->HiBlockTableSize64 = ByteOffset - pHeader->HiBlockTablePos64;
				ByteOffset = pHeader->HiBlockTablePos64;
			}

			// Size of the block table
			if (BlockTablePos64) {
				pHeader->BlockTableSize64 = ByteOffset - BlockTablePos64;
				ByteOffset = BlockTablePos64;
			}

			// Size of the hash table
			if (HashTablePos64) {
				pHeader->HashTableSize64 = ByteOffset - HashTablePos64;
				ByteOffset = HashTablePos64;
			}

			// Size of the BET table
			if (pHeader->BetTablePos64) {
				pHeader->BetTableSize64 = ByteOffset - pHeader->BetTablePos64;
				ByteOffset = pHeader->BetTablePos64;
			}

			// Size of the HET table
			if (pHeader->HetTablePos64) {
				pHeader->HetTableSize64 = ByteOffset - pHeader->HetTablePos64;
//              ByteOffset = pHeader->HetTablePos64;
			}

			// Add the MPQ Offset
			BlockTablePos64 += MpqOffset;
			break;

		case MPQ_FORMAT_VERSION_4:
			// Verify header MD5. Header MD5 is calculated from the MPQ header since the 'MPQ\x1A'
			// signature until the position of header MD5 at offset 0xC0
			if (!VerifyDataBlockHash(pHeader, MPQ_HEADER_SIZE_V4 - MD5_DIGEST_SIZE, pHeader->MD5_MpqHeader)) {
				nError = ERROR_FILE_CORRUPT;
			}

			// Calculate the block table position
			BlockTablePos64 = MpqOffset + MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos);
			break;

		default:
			// Check if it's a War of the Immortal data file (SQP)
			// If not, we treat it as malformed MPQ version 1.0
			if (ConvertSqpHeaderToFormat4(ha, FileSize, dwFlags) != ERROR_SUCCESS) {
				pHeader->wFormatVersion = MPQ_FORMAT_VERSION_1;
				pHeader->dwHeaderSize = MPQ_HEADER_SIZE_V1;
				ha->dwFlags |= MPQ_FLAG_MALFORMED;
				goto Label_ArchiveVersion1;
			}

			// Calculate the block table position
			BlockTablePos64 = MpqOffset + MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos);
			break;
	}

	// Handle case when block table is placed before the MPQ header
	// Used by BOBA protector
	if (BlockTablePos64 < MpqOffset) {
		ha->dwFlags |= MPQ_FLAG_MALFORMED;
	}

	return nError;
}

// This function converts SQP file header into MPQ file header
static int ConvertSqpHeaderToFormat4(TMPQArchive *ha, ULONGLONG FileSize, DWORD dwFlags)
{
	TSQPHeader *pSqpHeader = (TSQPHeader *)ha->HeaderData;
	TMPQHeader Header;

	// SQP files from War of the Immortal use MPQ file format with slightly
	// modified structure. These fields have different position:
	//
	//  Offset  TMPQHeader             TSQPHeader
	//  ------  ----------             -----------
	//   000C   wFormatVersion         dwHashTablePos (lo)
	//   000E   wSectorSize            dwHashTablePos (hi)
	//   001C   dwBlockTableSize (lo)  wBlockSize
	//   001E   dwHashTableSize (hi)   wFormatVersion

	// Can't open the archive with certain flags
	if (dwFlags & MPQ_OPEN_FORCE_MPQ_V1) {
		return ERROR_FILE_CORRUPT;
	}

	// The file must not be greater than 4 GB
	if ((FileSize >> 0x20) != 0) {
		return ERROR_FILE_CORRUPT;
	}

	// Translate the SQP header into a MPQ header
	memset(&Header, 0, sizeof(TMPQHeader));
	Header.dwID = pSqpHeader->dwID;
	Header.dwHeaderSize = pSqpHeader->dwHeaderSize;
	Header.dwArchiveSize = pSqpHeader->dwArchiveSize;
	Header.dwHashTablePos = pSqpHeader->dwHashTablePos;
	Header.dwBlockTablePos = pSqpHeader->dwBlockTablePos;
	Header.dwHashTableSize = pSqpHeader->dwHashTableSize;
	Header.dwBlockTableSize = pSqpHeader->dwBlockTableSize;
	Header.wFormatVersion = pSqpHeader->wFormatVersion;
	Header.wSectorSize = pSqpHeader->wSectorSize;

	// Verify the SQP header
	if (Header.dwID == ID_MPQ && Header.dwHeaderSize == sizeof(TSQPHeader) && Header.dwArchiveSize == FileSize) {
		// Check for fixed values of version and sector size
		if (Header.wFormatVersion == MPQ_FORMAT_VERSION_1 && Header.wSectorSize == 3) {
			// Initialize the fields of 3.0 header
			Header.ArchiveSize64    = Header.dwArchiveSize;
			Header.HashTableSize64  = Header.dwHashTableSize * sizeof(TMPQHash);
			Header.BlockTableSize64 = Header.dwBlockTableSize * sizeof(TMPQBlock);

			// Copy the converted MPQ header back
			memcpy(ha->HeaderData, &Header, sizeof(TMPQHeader));
			
			// Mark this file as SQP file
			ha->pfnHashString = HashStringSlash;
			ha->dwFlags |= MPQ_FLAG_READ_ONLY;
			ha->dwSubType = MPQ_SUBTYPE_SQP;
			return ERROR_SUCCESS;
		}
	}

	return ERROR_FILE_CORRUPT;
}

static TMPQFile *CreateFileHandle(TMPQArchive *ha, TFileEntry *pFileEntry)
{
	TMPQFile *hf;

	// Allocate space for TMPQFile
	hf = STORM_ALLOC(TMPQFile, 1);

	if (hf != NULL) {
		// Fill the file structure
		memset(hf, 0, sizeof(TMPQFile));
		hf->dwMagic = ID_MPQ;
		hf->pStream = NULL;
		hf->ha = ha;

		// If the called entered a file entry, we also copy informations from the file entry
		if (ha != NULL && pFileEntry != NULL) {
			// Set the raw position and MPQ position
			hf->RawFilePos = FileOffsetFromMpqOffset(ha, pFileEntry->ByteOffset);
			hf->MpqFilePos = pFileEntry->ByteOffset;

			// Set the data size
			hf->dwDataSize = pFileEntry->dwFileSize;
			hf->pFileEntry = pFileEntry;
		}
	}

	return hf;
}

static void CreateKeyFromAuthCode(LPBYTE pbKeyBuffer, const char *szAuthCode)
{
	LPDWORD KeyPosition = (LPDWORD)(pbKeyBuffer + 0x10);
	LPDWORD AuthCode32 = (LPDWORD)szAuthCode;

	memcpy(pbKeyBuffer, szKeyTemplate, MPQE_CHUNK_SIZE);
	KeyPosition[0x00] = AuthCode32[0x03];
	KeyPosition[0x02] = AuthCode32[0x07];
	KeyPosition[0x03] = AuthCode32[0x02];
	KeyPosition[0x05] = AuthCode32[0x06];
	KeyPosition[0x06] = AuthCode32[0x01];
	KeyPosition[0x08] = AuthCode32[0x05];
	KeyPosition[0x09] = AuthCode32[0x00];
	KeyPosition[0x0B] = AuthCode32[0x04];
}

static int CreateHashTable(TMPQArchive *ha, DWORD dwHashTableSize)
{
	TMPQHash *pHashTable;

	// Sanity checks
	assert((dwHashTableSize & (dwHashTableSize - 1)) == 0);
	assert(ha->pHashTable == NULL);

	// If the required hash table size is zero, don't create anything
	if (dwHashTableSize == 0) {
		dwHashTableSize = HASH_TABLE_SIZE_DEFAULT;
	}

	// Create the hash table
	pHashTable = STORM_ALLOC(TMPQHash, dwHashTableSize);

	if (pHashTable == NULL) {
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	// Fill it
	memset(pHashTable, 0xFF, dwHashTableSize * sizeof(TMPQHash));
	ha->pHeader->dwHashTableSize = dwHashTableSize;
	ha->dwMaxFileCount = dwHashTableSize;
	ha->pHashTable = pHashTable;
	return ERROR_SUCCESS;
}

static void DecryptFileChunk(DWORD *MpqData, LPBYTE pbKey, ULONGLONG ByteOffset, DWORD dwLength)
{
	ULONGLONG ChunkOffset;
	DWORD KeyShuffled[0x10];
	DWORD KeyMirror[0x10];
	DWORD RoundCount = 0x14;

	// Prepare the key
	ChunkOffset = ByteOffset / MPQE_CHUNK_SIZE;
	memcpy(KeyMirror, pbKey, MPQE_CHUNK_SIZE);
	KeyMirror[0x05] = (DWORD)(ChunkOffset >> 32);
	KeyMirror[0x08] = (DWORD)(ChunkOffset);

	while (dwLength >= MPQE_CHUNK_SIZE) {
		// Shuffle the key - part 1
		KeyShuffled[0x0E] = KeyMirror[0x00];
		KeyShuffled[0x0C] = KeyMirror[0x01];
		KeyShuffled[0x05] = KeyMirror[0x02];
		KeyShuffled[0x0F] = KeyMirror[0x03];
		KeyShuffled[0x0A] = KeyMirror[0x04];
		KeyShuffled[0x07] = KeyMirror[0x05];
		KeyShuffled[0x0B] = KeyMirror[0x06];
		KeyShuffled[0x09] = KeyMirror[0x07];
		KeyShuffled[0x03] = KeyMirror[0x08];
		KeyShuffled[0x06] = KeyMirror[0x09];
		KeyShuffled[0x08] = KeyMirror[0x0A];
		KeyShuffled[0x0D] = KeyMirror[0x0B];
		KeyShuffled[0x02] = KeyMirror[0x0C];
		KeyShuffled[0x04] = KeyMirror[0x0D];
		KeyShuffled[0x01] = KeyMirror[0x0E];
		KeyShuffled[0x00] = KeyMirror[0x0F];
		
		// Shuffle the key - part 2
		for (DWORD i = 0; i < RoundCount; i += 2) {
			KeyShuffled[0x0A] = KeyShuffled[0x0A] ^ Rol32((KeyShuffled[0x0E] + KeyShuffled[0x02]), 0x07);
			KeyShuffled[0x03] = KeyShuffled[0x03] ^ Rol32((KeyShuffled[0x0A] + KeyShuffled[0x0E]), 0x09);
			KeyShuffled[0x02] = KeyShuffled[0x02] ^ Rol32((KeyShuffled[0x03] + KeyShuffled[0x0A]), 0x0D);
			KeyShuffled[0x0E] = KeyShuffled[0x0E] ^ Rol32((KeyShuffled[0x02] + KeyShuffled[0x03]), 0x12);

			KeyShuffled[0x07] = KeyShuffled[0x07] ^ Rol32((KeyShuffled[0x0C] + KeyShuffled[0x04]), 0x07);
			KeyShuffled[0x06] = KeyShuffled[0x06] ^ Rol32((KeyShuffled[0x07] + KeyShuffled[0x0C]), 0x09);
			KeyShuffled[0x04] = KeyShuffled[0x04] ^ Rol32((KeyShuffled[0x06] + KeyShuffled[0x07]), 0x0D);
			KeyShuffled[0x0C] = KeyShuffled[0x0C] ^ Rol32((KeyShuffled[0x04] + KeyShuffled[0x06]), 0x12);

			KeyShuffled[0x0B] = KeyShuffled[0x0B] ^ Rol32((KeyShuffled[0x05] + KeyShuffled[0x01]), 0x07);
			KeyShuffled[0x08] = KeyShuffled[0x08] ^ Rol32((KeyShuffled[0x0B] + KeyShuffled[0x05]), 0x09);
			KeyShuffled[0x01] = KeyShuffled[0x01] ^ Rol32((KeyShuffled[0x08] + KeyShuffled[0x0B]), 0x0D);
			KeyShuffled[0x05] = KeyShuffled[0x05] ^ Rol32((KeyShuffled[0x01] + KeyShuffled[0x08]), 0x12);

			KeyShuffled[0x09] = KeyShuffled[0x09] ^ Rol32((KeyShuffled[0x0F] + KeyShuffled[0x00]), 0x07);
			KeyShuffled[0x0D] = KeyShuffled[0x0D] ^ Rol32((KeyShuffled[0x09] + KeyShuffled[0x0F]), 0x09);
			KeyShuffled[0x00] = KeyShuffled[0x00] ^ Rol32((KeyShuffled[0x0D] + KeyShuffled[0x09]), 0x0D);
			KeyShuffled[0x0F] = KeyShuffled[0x0F] ^ Rol32((KeyShuffled[0x00] + KeyShuffled[0x0D]), 0x12);

			KeyShuffled[0x04] = KeyShuffled[0x04] ^ Rol32((KeyShuffled[0x0E] + KeyShuffled[0x09]), 0x07);
			KeyShuffled[0x08] = KeyShuffled[0x08] ^ Rol32((KeyShuffled[0x04] + KeyShuffled[0x0E]), 0x09);
			KeyShuffled[0x09] = KeyShuffled[0x09] ^ Rol32((KeyShuffled[0x08] + KeyShuffled[0x04]), 0x0D);
			KeyShuffled[0x0E] = KeyShuffled[0x0E] ^ Rol32((KeyShuffled[0x09] + KeyShuffled[0x08]), 0x12);

			KeyShuffled[0x01] = KeyShuffled[0x01] ^ Rol32((KeyShuffled[0x0C] + KeyShuffled[0x0A]), 0x07);
			KeyShuffled[0x0D] = KeyShuffled[0x0D] ^ Rol32((KeyShuffled[0x01] + KeyShuffled[0x0C]), 0x09);
			KeyShuffled[0x0A] = KeyShuffled[0x0A] ^ Rol32((KeyShuffled[0x0D] + KeyShuffled[0x01]), 0x0D);
			KeyShuffled[0x0C] = KeyShuffled[0x0C] ^ Rol32((KeyShuffled[0x0A] + KeyShuffled[0x0D]), 0x12);

			KeyShuffled[0x00] = KeyShuffled[0x00] ^ Rol32((KeyShuffled[0x05] + KeyShuffled[0x07]), 0x07);
			KeyShuffled[0x03] = KeyShuffled[0x03] ^ Rol32((KeyShuffled[0x00] + KeyShuffled[0x05]), 0x09);
			KeyShuffled[0x07] = KeyShuffled[0x07] ^ Rol32((KeyShuffled[0x03] + KeyShuffled[0x00]), 0x0D);
			KeyShuffled[0x05] = KeyShuffled[0x05] ^ Rol32((KeyShuffled[0x07] + KeyShuffled[0x03]), 0x12);

			KeyShuffled[0x02] = KeyShuffled[0x02] ^ Rol32((KeyShuffled[0x0F] + KeyShuffled[0x0B]), 0x07);
			KeyShuffled[0x06] = KeyShuffled[0x06] ^ Rol32((KeyShuffled[0x02] + KeyShuffled[0x0F]), 0x09);
			KeyShuffled[0x0B] = KeyShuffled[0x0B] ^ Rol32((KeyShuffled[0x06] + KeyShuffled[0x02]), 0x0D);
			KeyShuffled[0x0F] = KeyShuffled[0x0F] ^ Rol32((KeyShuffled[0x0B] + KeyShuffled[0x06]), 0x12);
		}

		// Decrypt one data chunk
		MpqData[0x00] = MpqData[0x00] ^ (KeyShuffled[0x0E] + KeyMirror[0x00]);
		MpqData[0x01] = MpqData[0x01] ^ (KeyShuffled[0x04] + KeyMirror[0x0D]);
		MpqData[0x02] = MpqData[0x02] ^ (KeyShuffled[0x08] + KeyMirror[0x0A]);
		MpqData[0x03] = MpqData[0x03] ^ (KeyShuffled[0x09] + KeyMirror[0x07]);
		MpqData[0x04] = MpqData[0x04] ^ (KeyShuffled[0x0A] + KeyMirror[0x04]);
		MpqData[0x05] = MpqData[0x05] ^ (KeyShuffled[0x0C] + KeyMirror[0x01]);
		MpqData[0x06] = MpqData[0x06] ^ (KeyShuffled[0x01] + KeyMirror[0x0E]);
		MpqData[0x07] = MpqData[0x07] ^ (KeyShuffled[0x0D] + KeyMirror[0x0B]);
		MpqData[0x08] = MpqData[0x08] ^ (KeyShuffled[0x03] + KeyMirror[0x08]);
		MpqData[0x09] = MpqData[0x09] ^ (KeyShuffled[0x07] + KeyMirror[0x05]);
		MpqData[0x0A] = MpqData[0x0A] ^ (KeyShuffled[0x05] + KeyMirror[0x02]);
		MpqData[0x0B] = MpqData[0x0B] ^ (KeyShuffled[0x00] + KeyMirror[0x0F]);
		MpqData[0x0C] = MpqData[0x0C] ^ (KeyShuffled[0x02] + KeyMirror[0x0C]);
		MpqData[0x0D] = MpqData[0x0D] ^ (KeyShuffled[0x06] + KeyMirror[0x09]);
		MpqData[0x0E] = MpqData[0x0E] ^ (KeyShuffled[0x0B] + KeyMirror[0x06]);
		MpqData[0x0F] = MpqData[0x0F] ^ (KeyShuffled[0x0F] + KeyMirror[0x03]);

		// Update byte offset in the key
		KeyMirror[0x08]++;

		if (KeyMirror[0x08] == 0) {
			KeyMirror[0x05]++;
		}

		// Move pointers and decrease number of bytes to decrypt
		MpqData  += (MPQE_CHUNK_SIZE / sizeof(DWORD));
		dwLength -= MPQE_CHUNK_SIZE;
	}
}

static DWORD DecryptFileKey(const char *szFileName, ULONGLONG MpqPos, DWORD dwFileSize, DWORD dwFlags)
{
	DWORD dwFileKey;
	DWORD dwMpqPos = (DWORD)MpqPos;

	// File key is calculated from plain name
	szFileName = GetPlainFileName(szFileName);
	dwFileKey = HashString(szFileName, MPQ_HASH_FILE_KEY);

	// Fix the key, if needed
	if (dwFlags & MPQ_FILE_FIX_KEY) {
		dwFileKey = (dwFileKey + dwMpqPos) ^ dwFileSize;
	}

	// Return the key
	return dwFileKey;
}

static void DecryptMpkTable(void *pvMpkTable, size_t cbSize)
{
	LPBYTE pbMpkTable = (LPBYTE)pvMpkTable;

	for (size_t i = 0; i < cbSize; i++) {
		pbMpkTable[i] = MpkDecryptionKey[pbMpkTable[i]];
	}
}

static void DecryptMpqBlock(void *pvDataBlock, DWORD dwLength, DWORD dwKey1)
{
	LPDWORD DataBlock = (LPDWORD)pvDataBlock;
	DWORD dwValue32;
	DWORD dwKey2 = 0xEEEEEEEE;

	// Round to DWORDs
	dwLength >>= 2;

	// Decrypt the data block at array of DWORDs
	for (DWORD i = 0; i < dwLength; i++) {
		// Modify the second key
		dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];

		DataBlock[i] = DataBlock[i] ^ (dwKey1 + dwKey2);
		dwValue32 = DataBlock[i];

		dwKey1 = ((~dwKey1 << 0x15) + 0x11111111) | (dwKey1 >> 0x0B);
		dwKey2 = dwValue32 + dwKey2 + (dwKey2 << 5) + 3;
	}
}

// Defragment the file table so it does not contain any gaps
// Note: As long as all values of all TMPQHash::dwBlockIndex
// are not HASH_ENTRY_FREE, the startup search index does not matter.
// Hash table is circular, so as long as there is no terminator,
// all entries will be found.
static TMPQHash *DefragmentHashTable(TMPQArchive *ha, TMPQHash *pHashTable, TMPQBlock *pBlockTable)
{
	TMPQHeader *pHeader = ha->pHeader;
	TMPQHash *pHashTableEnd = pHashTable + pHeader->dwHashTableSize;
	TMPQHash *pSource = pHashTable;
	TMPQHash *pTarget = pHashTable;
	DWORD dwFirstFreeEntry;
	DWORD dwNewTableSize;

	// Sanity checks
	assert(pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1);
	assert(pHeader->HiBlockTablePos64 == 0);

	// Parse the hash table and move the entries to the begin of it
	for (pSource = pHashTable; pSource < pHashTableEnd; pSource++) {
		// Check whether this is a valid hash table entry
		if (IsValidHashEntry1(ha, pSource, pBlockTable)) {
			// Copy the hash table entry back
			if (pSource > pTarget) {
				pTarget[0] = pSource[0];
			}

			// Move the target
			pTarget++;
		}
	}

	// Calculate how many entries in the hash table we really need
	dwFirstFreeEntry = (DWORD)(pTarget - pHashTable);
	dwNewTableSize = GetNearestPowerOfTwo(dwFirstFreeEntry);

	// Fill the rest with entries that look like deleted
	pHashTableEnd = pHashTable + dwNewTableSize;
	pSource = pHashTable + dwFirstFreeEntry;
	memset(pSource, 0xFF, (dwNewTableSize - dwFirstFreeEntry) * sizeof(TMPQHash));

	// Mark the block indexes as deleted
	for (; pSource < pHashTableEnd; pSource++) {
		pSource->dwBlockIndex = HASH_ENTRY_DELETED;
	}

	// Free some of the space occupied by the hash table
	if (dwNewTableSize < pHeader->dwHashTableSize) {
		pHashTable = STORM_REALLOC(TMPQHash, pHashTable, dwNewTableSize);
		ha->pHeader->BlockTableSize64 = dwNewTableSize * sizeof(TMPQHash);
		ha->pHeader->dwHashTableSize = dwNewTableSize;
	}

	return pHashTable;
}

static ULONGLONG DetermineArchiveSize_V1(TMPQArchive *ha, TMPQHeader *pHeader, ULONGLONG MpqOffset, ULONGLONG FileSize)
{
	ULONGLONG ByteOffset;
	ULONGLONG EndOfMpq = FileSize;
	DWORD SignatureHeader = 0;
	DWORD dwArchiveSize32;

	// This could only be called for MPQs version 1.0
	assert(pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1);

	// Check if we can rely on the archive size in the header
	if (pHeader->dwBlockTablePos < pHeader->dwArchiveSize) {
		// The block table cannot be compressed, so the sizes must match
		if ((pHeader->dwArchiveSize - pHeader->dwBlockTablePos) == (pHeader->dwBlockTableSize * sizeof(TMPQBlock))) {
			return pHeader->dwArchiveSize;
		}

		// If the archive size in the header is less than real file size
		dwArchiveSize32 = (DWORD)(FileSize - MpqOffset);

		if (pHeader->dwArchiveSize == dwArchiveSize32) {
			return pHeader->dwArchiveSize;
		}
	}

	// Check if there is a signature header
	if ((EndOfMpq - MpqOffset) > (MPQ_STRONG_SIGNATURE_SIZE + 4)) {
		ByteOffset = EndOfMpq - MPQ_STRONG_SIGNATURE_SIZE - 4;

		if (FileStream_Read(ha->pStream, &ByteOffset, &SignatureHeader, sizeof(DWORD))) {
			if (SignatureHeader == MPQ_STRONG_SIGNATURE_ID) {
				EndOfMpq = EndOfMpq - MPQ_STRONG_SIGNATURE_SIZE - 4;
			}
		}
	}

	// Return the returned archive size
	return (EndOfMpq - MpqOffset);
}

static ULONGLONG DetermineArchiveSize_V2(TMPQHeader *pHeader, ULONGLONG MpqOffset, ULONGLONG FileSize)
{
	ULONGLONG EndOfMpq = FileSize;
	DWORD dwArchiveSize32;

	// This could only be called for MPQs version 2.0
	assert(pHeader->wFormatVersion == MPQ_FORMAT_VERSION_2);

	// Check if we can rely on the archive size in the header
	if ((FileSize >> 0x20) == 0) {
		if (pHeader->dwBlockTablePos < pHeader->dwArchiveSize) {
			if ((pHeader->dwArchiveSize - pHeader->dwBlockTablePos) <= (pHeader->dwBlockTableSize * sizeof(TMPQBlock))) {
				return pHeader->dwArchiveSize;
			}

			// If the archive size in the header is less than real file size
			dwArchiveSize32 = (DWORD)(FileSize - MpqOffset);

			if (pHeader->dwArchiveSize <= dwArchiveSize32) {
				return pHeader->dwArchiveSize;
			}
		}
	}

	// Return the calculated archive size
	return (EndOfMpq - MpqOffset);
}

static ULONGLONG FileOffsetFromMpqOffset(TMPQArchive *ha, ULONGLONG MpqOffset)
{
	if (ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1) {
		// For MPQ archive v1, any file offset is only 32-bit
		return (ULONGLONG)((DWORD)ha->MpqPos + (DWORD)MpqOffset);
	} else {
		// For MPQ archive v2+, file offsets are full 64-bit
		return ha->MpqPos + MpqOffset;
	}
}

static bool FlatStream_BlockCheck(
	TBlockStream *pStream,                 // Pointer to an open stream
	ULONGLONG BlockOffset)
{
	LPBYTE FileBitmap = (LPBYTE)pStream->FileBitmap;
	DWORD BlockIndex;
	BYTE BitMask;

	// Sanity checks
	assert((BlockOffset & (pStream->BlockSize - 1)) == 0);
	assert(FileBitmap != NULL);
	
	// Calculate the index of the block
	BlockIndex = (DWORD)(BlockOffset / pStream->BlockSize);
	BitMask = (BYTE)(1 << (BlockIndex & 0x07));

	// Check if the bit is present
	return (FileBitmap[BlockIndex / 0x08] & BitMask) ? true : false;
}

static BOOL FlatStream_BlockRead(
	TBlockStream * pStream,                // Pointer to an open stream
	ULONGLONG StartOffset,
	ULONGLONG EndOffset,
	LPBYTE BlockBuffer,
	DWORD BytesNeeded,
	BOOL bAvailable)
{
	DWORD BytesToRead = (DWORD)(EndOffset - StartOffset);

	// The starting offset must be aligned to size of the block
	assert(pStream->FileBitmap != NULL);
	assert((StartOffset & (pStream->BlockSize - 1)) == 0);
	assert(StartOffset < EndOffset);

	// If the blocks are not available, we need to load them from the master
	// and then save to the mirror
	if (bAvailable == FALSE) {
		// If we have no master, we cannot satisfy read request
		if (pStream->pMaster == NULL) {
			return FALSE;
		}

		// Load the blocks from the master stream
		// Note that we always have to read complete blocks
		// so they get properly stored to the mirror stream
		if (!FileStream_Read(pStream->pMaster, &StartOffset, BlockBuffer, BytesToRead)) {
			return FALSE;
		}

		// Store the loaded blocks to the mirror file.
		// Note that this operation is not required to succeed
		if (pStream->BaseWrite(pStream, &StartOffset, BlockBuffer, BytesToRead)) {
			FlatStream_UpdateBitmap(pStream, StartOffset, EndOffset);
		}

		return TRUE;
	} else {
		if (BytesToRead > BytesNeeded) {
			BytesToRead = BytesNeeded;
		}

		return pStream->BaseRead(pStream, &StartOffset, BlockBuffer, BytesToRead);
	}
}

static void FlatStream_Close(TBlockStream *pStream)
{
	FILE_BITMAP_FOOTER Footer;

	if (pStream->FileBitmap && pStream->IsModified) {
		// Write the file bitmap
		pStream->BaseWrite(pStream, &pStream->StreamSize, pStream->FileBitmap, pStream->BitmapSize);
		
		// Prepare and write the file footer
		Footer.Signature   = ID_FILE_BITMAP_FOOTER;
		Footer.Version     = 3;
		Footer.BuildNumber = pStream->BuildNumber;
		Footer.MapOffsetLo = (DWORD)(pStream->StreamSize & 0xFFFFFFFF);
		Footer.MapOffsetHi = (DWORD)(pStream->StreamSize >> 0x20);
		Footer.BlockSize   = pStream->BlockSize;
		pStream->BaseWrite(pStream, NULL, &Footer, sizeof(FILE_BITMAP_FOOTER));
	}

	// Close the base class
	BlockStream_Close(pStream);
}

static BOOL FlatStream_CreateMirror(TBlockStream *pStream)
{
	ULONGLONG MasterSize = 0;
	ULONGLONG MirrorSize = 0;
	LPBYTE FileBitmap = NULL;
	DWORD dwBitmapSize;
	DWORD dwBlockCount;
	BOOL bNeedCreateMirrorStream = TRUE;
	BOOL bNeedResizeMirrorStream = TRUE;

	// Do we have master function and base creation function?
	if (pStream->pMaster == NULL || pStream->BaseCreate == NULL) {
		return FALSE;
	}

	// Retrieve the master file size, block count and bitmap size
	FileStream_GetSize(pStream->pMaster, &MasterSize);
	dwBlockCount = (DWORD)((MasterSize + DEFAULT_BLOCK_SIZE - 1) / DEFAULT_BLOCK_SIZE);
	dwBitmapSize = (DWORD)((dwBlockCount + 7) / 8);

	// Setup stream size and position
	pStream->BuildNumber = DEFAULT_BUILD_NUMBER;        // BUGBUG: Really???
	pStream->StreamSize = MasterSize;
	pStream->StreamPos = 0;

	// Open the base stream for write access
	if (pStream->BaseOpen(pStream, pStream->szFileName, 0)) {
		// If the file open succeeded, check if the file size matches required size
		pStream->BaseGetSize(pStream, &MirrorSize);

		if (MirrorSize == MasterSize + dwBitmapSize + sizeof(FILE_BITMAP_FOOTER)) {
			// Attempt to load an existing file bitmap
			if (FlatStream_LoadBitmap(pStream)) {
				return TRUE;
			}

			// We need to create new file bitmap
			bNeedResizeMirrorStream = FALSE;
		}

		// We need to create mirror stream
		bNeedCreateMirrorStream = FALSE;
	}

	// Create a new stream, if needed
	if (bNeedCreateMirrorStream) {
		if (!pStream->BaseCreate(pStream)) {
			return FALSE;
		}
	}

	// If we need to, then resize the mirror stream
	if (bNeedResizeMirrorStream) {
		if (!pStream->BaseResize(pStream, MasterSize + dwBitmapSize + sizeof(FILE_BITMAP_FOOTER))) {
			return FALSE;
		}
	}

	// Allocate the bitmap array
	FileBitmap = STORM_ALLOC(BYTE, dwBitmapSize);

	if (FileBitmap == NULL) {
		return FALSE;
	}

	// Initialize the bitmap
	memset(FileBitmap, 0, dwBitmapSize);
	pStream->FileBitmap = FileBitmap;
	pStream->BitmapSize = dwBitmapSize;
	pStream->BlockSize  = DEFAULT_BLOCK_SIZE;
	pStream->BlockCount = dwBlockCount;
	pStream->IsComplete = 0;
	pStream->IsModified = 1;

	// Note: Don't write the stream bitmap right away.
	// Doing so would cause sparse file resize on NTFS,
	// which would take long time on larger files.
	return TRUE;
}

static BOOL BlockStream_GetPos(TFileStream *pStream, ULONGLONG *pByteOffset)
{
	*pByteOffset = pStream->StreamPos;
	return TRUE;
}

static BOOL BlockStream_GetSize(TFileStream *pStream, ULONGLONG *pFileSize)
{
	*pFileSize = pStream->StreamSize;
	return TRUE;
}

// Generic function that loads blocks from the file
// The function groups the block with the same availability,
// so the called BlockRead can finish the request in a single system call
static BOOL BlockStream_Read(
	TBlockStream * pStream,                 // Pointer to an open stream
	ULONGLONG * pByteOffset,                // Pointer to file byte offset. If NULL, it reads from the current position
	void * pvBuffer,                        // Pointer to data to be read
	DWORD dwBytesToRead)                    // Number of bytes to read from the file
{
	ULONGLONG BlockOffset0;
	ULONGLONG BlockOffset;
	ULONGLONG ByteOffset;
	ULONGLONG EndOffset;
	LPBYTE TransferBuffer;
	LPBYTE BlockBuffer;
	DWORD BlockBufferOffset;                // Offset of the desired data in the block buffer
	DWORD BytesNeeded;                      // Number of bytes that really need to be read
	DWORD BlockSize = pStream->BlockSize;
	DWORD BlockCount;
	BOOL bPrevBlockAvailable;
	BOOL bCallbackCalled = FALSE;
	BOOL bBlockAvailable;
	BOOL bResult = TRUE;

	// The base block read function must be present
	assert(pStream->BlockRead != NULL);

	// NOP reading of zero bytes
	if (dwBytesToRead == 0) {
		return TRUE;
	}

	// Get the current position in the stream
	ByteOffset = (pByteOffset != NULL) ? pByteOffset[0] : pStream->StreamPos;
	EndOffset = ByteOffset + dwBytesToRead;

	if (EndOffset > pStream->StreamSize) {
		SErrSetLastError(ERROR_HANDLE_EOF);
		return FALSE;
	}

	// Calculate the block parameters
	BlockOffset0 = BlockOffset = ByteOffset & ~((ULONGLONG)BlockSize - 1);
	BlockCount  = (DWORD)(((EndOffset - BlockOffset) + (BlockSize - 1)) / BlockSize);
	BytesNeeded = (DWORD)(EndOffset - BlockOffset);

	// Remember where we have our data
	assert((BlockSize & (BlockSize - 1)) == 0);
	BlockBufferOffset = (DWORD)(ByteOffset & (BlockSize - 1));

	// Allocate buffer for reading blocks
	TransferBuffer = BlockBuffer = STORM_ALLOC(BYTE, (BlockCount * BlockSize));

	if (TransferBuffer == NULL) {
		SErrSetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	// If all blocks are available, just read all blocks at once
	if (pStream->IsComplete == 0) {
		// Now parse the blocks and send the block read request
		// to all blocks with the same availability
		assert(pStream->BlockCheck != NULL);
		bPrevBlockAvailable = pStream->BlockCheck(pStream, BlockOffset);

		// Loop as long as we have something to read
		while (BlockOffset < EndOffset) {
			// Determine availability of the next block
			bBlockAvailable = pStream->BlockCheck(pStream, BlockOffset);

			// If the availability has changed, read all blocks up to this one
			if (bBlockAvailable != bPrevBlockAvailable) {
				// Call the file stream callback, if the block is not available
				if (pStream->pMaster && pStream->pfnCallback && bPrevBlockAvailable == FALSE) {
					pStream->pfnCallback(pStream->UserData, BlockOffset0, (DWORD)(BlockOffset - BlockOffset0));
					bCallbackCalled = TRUE;
				}

				// Load the continuous blocks with the same availability
				assert(BlockOffset > BlockOffset0);
				bResult = pStream->BlockRead(pStream, BlockOffset0, BlockOffset, BlockBuffer, BytesNeeded, bPrevBlockAvailable);

				if (!bResult) {
					break;
				}

				// Move the block offset
				BlockBuffer += (DWORD)(BlockOffset - BlockOffset0);
				BytesNeeded -= (DWORD)(BlockOffset - BlockOffset0);
				bPrevBlockAvailable = bBlockAvailable;
				BlockOffset0 = BlockOffset;
			}

			// Move to the block offset in the stream
			BlockOffset += BlockSize;
		}

		// If there is a block(s) remaining to be read, do it
		if (BlockOffset > BlockOffset0) {
			// Call the file stream callback, if the block is not available
			if (pStream->pMaster && pStream->pfnCallback && bPrevBlockAvailable == FALSE) {
				pStream->pfnCallback(pStream->UserData, BlockOffset0, (DWORD)(BlockOffset - BlockOffset0));
				bCallbackCalled = TRUE;
			}

			// Read the complete blocks from the file
			if (BlockOffset > pStream->StreamSize) {
				BlockOffset = pStream->StreamSize;
			}

			bResult = pStream->BlockRead(pStream, BlockOffset0, BlockOffset, BlockBuffer, BytesNeeded, bPrevBlockAvailable);
		}
	} else {
		// Read the complete blocks from the file
		if (EndOffset > pStream->StreamSize) {
			EndOffset = pStream->StreamSize;
		}

		bResult = pStream->BlockRead(pStream, BlockOffset, EndOffset, BlockBuffer, BytesNeeded, TRUE);
	}

	// Now copy the data to the user buffer
	if (bResult) {
		memcpy(pvBuffer, TransferBuffer + BlockBufferOffset, dwBytesToRead);
		pStream->StreamPos = ByteOffset + dwBytesToRead;
	} else {
		// If the block read failed, set the last error
		SErrSetLastError(ERROR_FILE_INCOMPLETE);
	}

	// Call the callback to indicate we are done
	if (bCallbackCalled) {
		pStream->pfnCallback(pStream->UserData, 0, 0);
	}

	// Free the block buffer and return
	STORM_FREE(TransferBuffer);
	return bResult;
}

//-----------------------------------------------------------------------------
// Local functions - Block4 stream support

#define BLOCK4_BLOCK_SIZE   0x4000          // Size of one block
#define BLOCK4_HASH_SIZE    0x20            // Size of MD5 hash that is after each block
#define BLOCK4_MAX_BLOCKS   0x00002000      // Maximum amount of blocks per file
#define BLOCK4_MAX_FSIZE    0x08040000      // Max size of one file

static BOOL Block4Stream_BlockRead(
	TBlockStream *pStream,                // Pointer to an open stream
	ULONGLONG StartOffset,
	ULONGLONG EndOffset,
	LPBYTE BlockBuffer,
	DWORD BytesNeeded,
	BOOL bAvailable)
{
	TBaseProviderData *BaseArray = (TBaseProviderData *)pStream->FileBitmap;
	ULONGLONG ByteOffset;
	DWORD BytesToRead;
	DWORD StreamIndex;
	DWORD BlockIndex;
	BOOL bResult;

	// The starting offset must be aligned to size of the block
	assert(pStream->FileBitmap != NULL);
	assert((StartOffset & (pStream->BlockSize - 1)) == 0);
	assert(StartOffset < EndOffset);
	assert(bAvailable == TRUE);

	// Keep compiler happy
	bAvailable = bAvailable;
	EndOffset = EndOffset;

	while (BytesNeeded != 0) {
		// Calculate the block index and the file index
		StreamIndex = (DWORD)((StartOffset / pStream->BlockSize) / BLOCK4_MAX_BLOCKS);
		BlockIndex  = (DWORD)((StartOffset / pStream->BlockSize) % BLOCK4_MAX_BLOCKS);

		if (StreamIndex > pStream->BitmapSize) {
			return false;
		}

		// Calculate the block offset
		ByteOffset = ((ULONGLONG)BlockIndex * (BLOCK4_BLOCK_SIZE + BLOCK4_HASH_SIZE));
		BytesToRead = STORMLIB_MIN(BytesNeeded, BLOCK4_BLOCK_SIZE);

		// Read from the base stream
		pStream->Base = BaseArray[StreamIndex];
		bResult = pStream->BaseRead(pStream, &ByteOffset, BlockBuffer, BytesToRead);
		BaseArray[StreamIndex] = pStream->Base;

		// Did the result succeed?
		if (bResult == FALSE) {
			return FALSE;
		}

		// Move pointers
		StartOffset += BytesToRead;
		BlockBuffer += BytesToRead;
		BytesNeeded -= BytesToRead;
	}

	return TRUE;
}

static void Block4Stream_Close(TBlockStream *pStream)
{
	TBaseProviderData *BaseArray = (TBaseProviderData *)pStream->FileBitmap;

	// If we have a non-zero count of base streams,
	// we have to close them all
	if (BaseArray != NULL) {
		// Close all base streams
		for (DWORD i = 0; i < pStream->BitmapSize; i++) {
			memcpy(&pStream->Base, BaseArray + i, sizeof(TBaseProviderData));
			pStream->BaseClose(pStream);
		}
	}

	// Free the data map, if any
	if (pStream->FileBitmap != NULL) {
		STORM_FREE(pStream->FileBitmap);
	}

	pStream->FileBitmap = NULL;

	// Do not call the BaseClose function,
	// we closed all handles already
	return;
}

static TFileStream *Block4Stream_Open(const TCHAR *szFileName, DWORD dwStreamFlags)
{
	TBaseProviderData *NewBaseArray = NULL;
	ULONGLONG RemainderBlock;
	ULONGLONG BlockCount;
	ULONGLONG FileSize;
	TBlockStream *pStream;
	TCHAR *szNameBuff;
	size_t nNameLength;
	DWORD dwBaseFiles = 0;
	DWORD dwBaseFlags;

	// Create new empty stream
	pStream = (TBlockStream *)AllocateFileStream(szFileName, sizeof(TBlockStream), dwStreamFlags);

	if (pStream == NULL) {
		return NULL;
	}

	// Sanity check
	assert(pStream->BaseOpen != NULL);

	// Get the length of the file name without numeric suffix
	nNameLength = _tcslen(pStream->szFileName);

	if (pStream->szFileName[nNameLength - 2] == '.' && pStream->szFileName[nNameLength - 1] == '0') {
		nNameLength -= 2;
	}

	pStream->szFileName[nNameLength] = 0;

	// Supply the stream functions
	pStream->StreamRead    = (STREAM_READ)BlockStream_Read;
	pStream->StreamGetSize = BlockStream_GetSize;
	pStream->StreamGetPos  = BlockStream_GetPos;
	pStream->StreamClose   = (STREAM_CLOSE)Block4Stream_Close;
	pStream->BlockRead     = (BLOCK_READ)Block4Stream_BlockRead;

	// Allocate work space for numeric names
	szNameBuff = STORM_ALLOC(TCHAR, nNameLength + 4);

	if (szNameBuff != NULL) {
		// Set the base flags
		dwBaseFlags = (dwStreamFlags & STREAM_PROVIDERS_MASK) | STREAM_FLAG_READ_ONLY;

		// Go all suffixes from 0 to 30
		for (int nSuffix = 0; nSuffix < 30; nSuffix++) {
			// Open the n-th file
			_stprintf(szNameBuff, _T("%s.%u"), pStream->szFileName, nSuffix);

			if (!pStream->BaseOpen(pStream, szNameBuff, dwBaseFlags)) {
				break;
			}

			// If the open succeeded, we re-allocate the base provider array
			NewBaseArray = STORM_ALLOC(TBaseProviderData, dwBaseFiles + 1);

			if (NewBaseArray == NULL) {
				SErrSetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return NULL;
			}

			// Copy the old base data array to the new base data array
			if (pStream->FileBitmap != NULL) {
				memcpy(NewBaseArray, pStream->FileBitmap, sizeof(TBaseProviderData) * dwBaseFiles);
				STORM_FREE(pStream->FileBitmap);
			}

			// Also copy the opened base array
			memcpy(NewBaseArray + dwBaseFiles, &pStream->Base, sizeof(TBaseProviderData));
			pStream->FileBitmap = NewBaseArray;
			dwBaseFiles++;

			// Get the size of the base stream
			pStream->BaseGetSize(pStream, &FileSize);
			assert(FileSize <= BLOCK4_MAX_FSIZE);
			RemainderBlock = FileSize % (BLOCK4_BLOCK_SIZE + BLOCK4_HASH_SIZE);
			BlockCount = FileSize / (BLOCK4_BLOCK_SIZE + BLOCK4_HASH_SIZE);
			
			// Increment the stream size and number of blocks            
			pStream->StreamSize += (BlockCount * BLOCK4_BLOCK_SIZE);
			pStream->BlockCount += (DWORD)BlockCount;

			// Is this the last file?
			if (FileSize < BLOCK4_MAX_FSIZE) {
				if (RemainderBlock) {
					pStream->StreamSize += (RemainderBlock - BLOCK4_HASH_SIZE);
					pStream->BlockCount++;
				}

				break;
			}
		}

		// Fill the remainining block stream variables
		pStream->BitmapSize = dwBaseFiles;
		pStream->BlockSize  = BLOCK4_BLOCK_SIZE;
		pStream->IsComplete = 1;
		pStream->IsModified = 0;

		// Fill the remaining stream variables
		pStream->StreamPos = 0;
		pStream->dwFlags |= STREAM_FLAG_READ_ONLY;

		STORM_FREE(szNameBuff);
	}

	// If we opened something, return success
	if (dwBaseFiles == 0) {
		FileStream_Close(pStream);
		SErrSetLastError(ERROR_FILE_NOT_FOUND);
		pStream = NULL;
	}

	return pStream;
}

/**
 * This function opens an existing file for read or read-write access
 * - If the current platform supports file sharing,
 *   the file must be open for read sharing (i.e. another application
 *   can open the file for read, but not for write)
 * - If the file does not exist, the function must return NULL
 * - If the file exists but cannot be open, then function must return NULL
 * - The parameters of the function must be validate by the caller
 * - The function must initialize all stream function pointers in TFileStream
 * - If the function fails from any reason, it must close all handles
 *   and free all memory that has been allocated in the process of stream creation,
 *   including the TFileStream structure itself
 *
 * \a szFileName Name of the file to open
 * \a dwStreamFlags specifies the provider and base storage type
 */

static TFileStream *FileStream_OpenFile(const TCHAR *szFileName, DWORD dwStreamFlags)
{
	DWORD dwProvider = dwStreamFlags & STREAM_PROVIDERS_MASK;
	size_t nPrefixLength = FileStream_Prefix(szFileName, &dwProvider);

	// Re-assemble the stream flags
	dwStreamFlags = (dwStreamFlags & STREAM_OPTIONS_MASK) | dwProvider;
	szFileName += nPrefixLength;

	// Perform provider-specific open
	switch (dwStreamFlags & STREAM_PROVIDER_MASK) {
		case STREAM_PROVIDER_FLAT:
			return FlatStream_Open(szFileName, dwStreamFlags);

		case STREAM_PROVIDER_PARTIAL:
			return PartStream_Open(szFileName, dwStreamFlags);

		case STREAM_PROVIDER_MPQE:
			return MpqeStream_Open(szFileName, dwStreamFlags);

		case STREAM_PROVIDER_BLOCK4:
			return Block4Stream_Open(szFileName, dwStreamFlags);

		default:
			SErrSetLastError(ERROR_INVALID_PARAMETER);
			return NULL;
	}
}

// Attempts to search a free hash entry in the hash table being converted.
// The created hash table must always be of nonzero size,
// should have no duplicated items and no deleted entries
static TMPQHash *FindFreeHashEntry(TMPQHash *pHashTable, DWORD dwHashTableSize, DWORD dwStartIndex)
{
	TMPQHash *pHash;
	DWORD dwIndex;

	// Set the initial index
	dwStartIndex = dwIndex = (dwStartIndex & (dwHashTableSize - 1));
	assert(dwHashTableSize != 0);

	// Search the hash table and return the found entries in the following priority:
	for (;;) {
		// We are not expecting to find matching entry in the hash table being built
		// We are not expecting to find deleted entry either
		pHash = pHashTable + dwIndex;

		// If we found a free entry, we need to stop searching
		if (pHash->dwBlockIndex == HASH_ENTRY_FREE) {
			return pHash;
		}

		// Move to the next hash entry.
		// If we reached the starting entry, it's failure.
		dwIndex = (dwIndex + 1) & (dwHashTableSize - 1);

		if (dwIndex == dwStartIndex) {
			break;
		}
	}

	// We haven't found anything
	assert(FALSE);
	return NULL;
}

/**
 * This function closes an archive file and frees any data buffers
 * that have been allocated for stream management. The function must also
 * support partially allocated structure, i.e. one or more buffers
 * can be NULL, if there was an allocation failure during the process
 *
 * \a pStream Pointer to an open stream
 */
static void FileStream_Close(TFileStream *pStream)
{
	// Check if the stream structure is allocated at all
	if (pStream != NULL) {
		// Free the master stream, if any
		if (pStream->pMaster != NULL) {
			FileStream_Close(pStream->pMaster);
		}

		pStream->pMaster = NULL;

		// Close the stream provider ...
		if (pStream->StreamClose != NULL) {
			pStream->StreamClose(pStream);
		}
		
		// ... or close base stream, if any
		else if (pStream->BaseClose != NULL) {
			pStream->BaseClose(pStream);
		}

		// Free the stream itself
		STORM_FREE(pStream);
	}
}

/**
 * Returns the stream flags
 *
 * \a pStream Pointer to an open stream
 * \a pdwStreamFlags Pointer where to store the stream flags
 */
static BOOL FileStream_GetFlags(TFileStream *pStream, LPDWORD pdwStreamFlags)
{
	*pdwStreamFlags = pStream->dwFlags;
	return TRUE;
}

/**
 * This function returns the current file position
 * \a pStream
 * \a pByteOffset
 */
static BOOL FileStream_GetPos(TFileStream *pStream, ULONGLONG *pByteOffset)
{
	assert(pStream->StreamGetPos != NULL);

	return pStream->StreamGetPos(pStream, pByteOffset);
}

/**
 * Returns the size of a file
 *
 * \a pStream Pointer to an open stream
 * \a FileSize Pointer where to store the file size
 */
static BOOL FileStream_GetSize(TFileStream *pStream, ULONGLONG *pFileSize)
{
	assert(pStream->StreamGetSize != NULL);

	return pStream->StreamGetSize(pStream, pFileSize);
}

/**
 * Returns the length of the provider prefix. Returns zero if no prefix
 *
 * \a szFileName Pointer to a stream name (file, mapped file, URL)
 * \a pdwStreamProvider Pointer to a DWORD variable that receives stream provider (STREAM_PROVIDER_XXX)
 */

static size_t FileStream_Prefix(const TCHAR *szFileName, DWORD *pdwProvider)
{
	size_t nPrefixLength1 = 0;
	size_t nPrefixLength2 = 0;
	DWORD dwProvider = 0;

	if (szFileName != NULL) {
		//
		// Determine the stream provider
		//

		if (!_tcsnicmp(szFileName, _T("flat-"), 5)) {
			dwProvider |= STREAM_PROVIDER_FLAT;
			nPrefixLength1 = 5;
		} else if (!_tcsnicmp(szFileName, _T("part-"), 5)) {
			dwProvider |= STREAM_PROVIDER_PARTIAL;
			nPrefixLength1 = 5;
		} else if (!_tcsnicmp(szFileName, _T("mpqe-"), 5)) {
			dwProvider |= STREAM_PROVIDER_MPQE;
			nPrefixLength1 = 5;
		} else if (!_tcsnicmp(szFileName, _T("blk4-"), 5)) {
			dwProvider |= STREAM_PROVIDER_BLOCK4;
			nPrefixLength1 = 5;
		}

		//
		// Determine the base provider
		//

		if (!_tcsnicmp(szFileName+nPrefixLength1, _T("file:"), 5)) {
			dwProvider |= BASE_PROVIDER_FILE;
			nPrefixLength2 = 5;
		} else if(!_tcsnicmp(szFileName+nPrefixLength1, _T("map:"), 4)) {
			dwProvider |= BASE_PROVIDER_MAP;
			nPrefixLength2 = 4;
		} else if(!_tcsnicmp(szFileName+nPrefixLength1, _T("http:"), 5)) {
			dwProvider |= BASE_PROVIDER_HTTP;
			nPrefixLength2 = 5;
		}

		// Only accept stream provider if we recognized the base provider
		if (nPrefixLength2 != 0) {
			// It is also allowed to put "//" after the base provider, e.g. "file://", "http://"
			if (szFileName[nPrefixLength1+nPrefixLength2] == '/' && szFileName[nPrefixLength1+nPrefixLength2+1] == '/') {
				nPrefixLength2 += 2;
			}

			if (pdwProvider != NULL) {
				*pdwProvider = dwProvider;
			}

			return nPrefixLength1 + nPrefixLength2;
		}
	}

	return 0;
}

/**
 * Reads data from the stream
 *
 * - Returns true if the read operation succeeded and all bytes have been read
 * - Returns false if either read failed or not all bytes have been read
 * - If the pByteOffset is NULL, the function must read the data from the current file position
 * - The function can be called with dwBytesToRead = 0. In that case, pvBuffer is ignored
 *   and the function just adjusts file pointer.
 *
 * \a pStream Pointer to an open stream
 * \a pByteOffset Pointer to file byte offset. If NULL, it reads from the current position
 * \a pvBuffer Pointer to data to be read
 * \a dwBytesToRead Number of bytes to read from the file
 *
 * \returns
 * - If the function reads the required amount of bytes, it returns true.
 * - If the function reads less than required bytes, it returns false and GetLastError() returns ERROR_HANDLE_EOF
 * - If the function fails, it reads false and GetLastError() returns an error code different from ERROR_HANDLE_EOF
 */
static BOOL FileStream_Read(TFileStream *pStream, ULONGLONG *pByteOffset, void *pvBuffer, DWORD dwBytesToRead)
{
	assert(pStream->StreamRead != NULL);

	return pStream->StreamRead(pStream, pByteOffset, pvBuffer, dwBytesToRead);
}

static DWORD FlatStream_CheckFile(TBlockStream *pStream)
{
	LPBYTE FileBitmap = (LPBYTE)pStream->FileBitmap;
	DWORD WholeByteCount = (pStream->BlockCount / 8);
	DWORD ExtraBitsCount = (pStream->BlockCount & 7);
	BYTE ExpectedValue;

	// Verify the whole bytes - their value must be 0xFF
	for (DWORD i = 0; i < WholeByteCount; i++) {
		if (FileBitmap[i] != 0xFF) {
			return 0;
		}
	}

	// If there are extra bits, calculate the mask
	if (ExtraBitsCount != 0)
	{
		ExpectedValue = (BYTE)((1 << ExtraBitsCount) - 1);
		if (FileBitmap[WholeByteCount] != ExpectedValue) {
			return 0;
		}
	}

	// Yes, the file is complete
	return 1;
}

static BOOL FlatStream_LoadBitmap(TBlockStream *pStream)
{
	FILE_BITMAP_FOOTER Footer;
	ULONGLONG ByteOffset; 
	LPBYTE FileBitmap;
	DWORD BlockCount;
	DWORD BitmapSize;

	// Do not load the bitmap if we should not have to
	if (!(pStream->dwFlags & STREAM_FLAG_USE_BITMAP)) {
		return FALSE;
	}

	// Only if the size is greater than size of bitmap footer
	if (pStream->Base.File.FileSize > sizeof(FILE_BITMAP_FOOTER)) {
		// Load the bitmap footer
		ByteOffset = pStream->Base.File.FileSize - sizeof(FILE_BITMAP_FOOTER);
		if(pStream->BaseRead(pStream, &ByteOffset, &Footer, sizeof(FILE_BITMAP_FOOTER)))
		{
			// Verify if there is actually a footer
			if (Footer.Signature == ID_FILE_BITMAP_FOOTER && Footer.Version == 0x03) {
				// Get the offset of the bitmap, number of blocks and size of the bitmap
				ByteOffset = MAKE_OFFSET64(Footer.MapOffsetHi, Footer.MapOffsetLo);
				BlockCount = (DWORD)(((ByteOffset - 1) / Footer.BlockSize) + 1);
				BitmapSize = ((BlockCount + 7) / 8);

				// Check if the sizes match
				if (ByteOffset + BitmapSize + sizeof(FILE_BITMAP_FOOTER) == pStream->Base.File.FileSize) {
					// Allocate space for the bitmap
					FileBitmap = STORM_ALLOC(BYTE, BitmapSize);

					if (FileBitmap != NULL) {
						// Load the bitmap bits
						if (!pStream->BaseRead(pStream, &ByteOffset, FileBitmap, BitmapSize)) {
							STORM_FREE(FileBitmap);
							return FALSE;
						}

						// Update the stream size
						pStream->BuildNumber = Footer.BuildNumber;
						pStream->StreamSize = ByteOffset;

						// Fill the bitmap information
						pStream->FileBitmap = FileBitmap;
						pStream->BitmapSize = BitmapSize;
						pStream->BlockSize  = Footer.BlockSize;
						pStream->BlockCount = BlockCount;
						pStream->IsComplete = FlatStream_CheckFile(pStream);
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

static TFileStream *FlatStream_Open(const TCHAR *szFileName, DWORD dwStreamFlags)
{
	TBlockStream *pStream;    
	ULONGLONG ByteOffset = 0;

	// Create new empty stream
	pStream = (TBlockStream *)AllocateFileStream(szFileName, sizeof(TBlockStream), dwStreamFlags);

	if (pStream == NULL) {
		return NULL;
	}

	// Do we have a master stream?
	if (pStream->pMaster != NULL) {
		if (!FlatStream_CreateMirror(pStream)) {
			FileStream_Close(pStream);
			SErrSetLastError(ERROR_FILE_NOT_FOUND);
			return NULL;
		}
	} else {
		// Attempt to open the base stream
		if (!pStream->BaseOpen(pStream, pStream->szFileName, dwStreamFlags)) {
			FileStream_Close(pStream);
			return NULL;
		}

		// Load the bitmap, if required to
		if (dwStreamFlags & STREAM_FLAG_USE_BITMAP) {
			FlatStream_LoadBitmap(pStream);
		}
	}

	// If we have a stream bitmap, set the reading functions
	// which check presence of each file block
	if (pStream->FileBitmap != NULL) {
		// Set the stream position to zero. Stream size is already set
		assert(pStream->StreamSize != 0);
		pStream->StreamPos = 0;
		pStream->dwFlags |= STREAM_FLAG_READ_ONLY;

		// Supply the stream functions
		pStream->StreamRead    = (STREAM_READ)BlockStream_Read;
		pStream->StreamGetSize = BlockStream_GetSize;
		pStream->StreamGetPos  = BlockStream_GetPos;
		pStream->StreamClose   = (STREAM_CLOSE)FlatStream_Close;

		// Supply the block functions
		pStream->BlockCheck    = (BLOCK_CHECK)FlatStream_BlockCheck;
		pStream->BlockRead     = (BLOCK_READ)FlatStream_BlockRead;
	} else {
		// Reset the base position to zero
		pStream->BaseRead(pStream, &ByteOffset, NULL, 0);

		// Setup stream size and position
		pStream->StreamSize = pStream->Base.File.FileSize;
		pStream->StreamPos = 0;

		// Set the base functions
		pStream->StreamRead    = pStream->BaseRead;
		pStream->StreamWrite   = pStream->BaseWrite;
		pStream->StreamResize  = pStream->BaseResize;
		pStream->StreamGetSize = pStream->BaseGetSize;
		pStream->StreamGetPos  = pStream->BaseGetPos;
		pStream->StreamClose   = pStream->BaseClose;
	}

	return pStream;
}

static void FlatStream_UpdateBitmap(
	TBlockStream *pStream,                 // Pointer to an open stream
	ULONGLONG StartOffset,
	ULONGLONG EndOffset)
{
	LPBYTE FileBitmap = (LPBYTE)pStream->FileBitmap;
	DWORD BlockIndex;
	DWORD BlockSize = pStream->BlockSize;
	DWORD ByteIndex;
	BYTE BitMask;

	// Sanity checks
	assert((StartOffset & (BlockSize - 1)) == 0);
	assert(FileBitmap != NULL);

	// Calculate the index of the block
	BlockIndex = (DWORD)(StartOffset / BlockSize);
	ByteIndex = (BlockIndex / 0x08);
	BitMask = (BYTE)(1 << (BlockIndex & 0x07));

	// Set all bits for the specified range
	while (StartOffset < EndOffset) {
		// Set the bit
		FileBitmap[ByteIndex] |= BitMask;

		// Move all
		StartOffset += BlockSize;
		ByteIndex += (BitMask >> 0x07);
		BitMask = (BitMask >> 0x07) | (BitMask << 0x01);
	}

	// Increment the bitmap update count
	pStream->IsModified = 1;
}

// Frees the MPQ archive
static void FreeArchiveHandle(TMPQArchive *& ha)
{
	if (ha != NULL) {
		// First of all, free the patch archive, if any
		if (ha->haPatch != NULL) {
			FreeArchiveHandle(ha->haPatch);
		}

		// Free the patch prefix, if any
		if (ha->pPatchPrefix != NULL) {
			STORM_FREE(ha->pPatchPrefix);
		}

		// Close the file stream
		FileStream_Close(ha->pStream);
		ha->pStream = NULL;

		// Free the file names from the file table
		if (ha->pFileTable != NULL) {
			for (DWORD i = 0; i < ha->dwFileTableSize; i++) {
				if(ha->pFileTable[i].szFileName != NULL) {
					STORM_FREE(ha->pFileTable[i].szFileName);
				}

				ha->pFileTable[i].szFileName = NULL;
			}

			// Then free all buffers allocated in the archive structure
			STORM_FREE(ha->pFileTable);
		}

		if (ha->pHashTable != NULL) {
			STORM_FREE(ha->pHashTable);
		}
#ifdef FULL
		if (ha->pHetTable != NULL) {
			FreeHetTable(ha->pHetTable);
		}
#endif
		STORM_FREE(ha);
		ha = NULL;
	}
}

// Frees the structure for MPQ file
static void FreeFileHandle(TMPQFile *& hf)
{
	if (hf != NULL) {
		// If we have patch file attached to this one, free it first
		if (hf->hfPatch != NULL) {
			FreeFileHandle(hf->hfPatch);
		}

		// Then free all buffers allocated in the file structure
		if (hf->pbFileData != NULL) {
			STORM_FREE(hf->pbFileData);
		}

		if (hf->pPatchInfo != NULL) {
			STORM_FREE(hf->pPatchInfo);
		}

		if (hf->SectorOffsets != NULL) {
			STORM_FREE(hf->SectorOffsets);
		}

		if (hf->SectorChksums != NULL) {
			STORM_FREE(hf->SectorChksums);
		}

		if (hf->pbFileSector != NULL) {
			STORM_FREE(hf->pbFileSector);
		}

		if (hf->pStream != NULL) {
			FileStream_Close(hf->pStream);
		}

		STORM_FREE(hf);
		hf = NULL;
	}
}

static TFileEntry *GetFileEntryLocale2(TMPQArchive *ha, const char *szFileName, LCID lcLocale, LPDWORD PtrHashIndex)
{
	TMPQHash *pHash;
	DWORD dwFileIndex;

	// First, we have to search the classic hash table
	// This is because on renaming, deleting, or changing locale,
	// we will need the pointer to hash table entry
	if (ha->pHashTable != NULL) {
		pHash = GetHashEntryLocale(ha, szFileName, lcLocale, 0);

		if (pHash != NULL && MPQ_BLOCK_INDEX(pHash) < ha->dwFileTableSize) {
			if (PtrHashIndex != NULL) {
				PtrHashIndex[0] = (DWORD)(pHash - ha->pHashTable);
			}

			return ha->pFileTable + MPQ_BLOCK_INDEX(pHash);
		}
	}

#ifdef FULL
	// If we have HET table in the MPQ, try to find the file in HET table
	if (ha->pHetTable != NULL) {
		dwFileIndex = GetFileIndex_Het(ha, szFileName);

		if (dwFileIndex != HASH_ENTRY_FREE) {
			return ha->pFileTable + dwFileIndex;
		}
	}
#endif

	// Not found
	return NULL;
}

static TFileEntry *GetFileEntryLocale(TMPQArchive *ha, const char *szFileName, LCID lcLocale)
{
	return GetFileEntryLocale2(ha, szFileName, lcLocale, NULL);
}

// Returns a hash table entry in the following order:
// 1) A hash table entry with the preferred locale and platform
// 2) A hash table entry with the neutral|matching locale and neutral|matching platform
// 3) NULL
// Storm_2016.dll: 15020940
static TMPQHash *GetHashEntryLocale(TMPQArchive *ha, const char *szFileName, LCID lcLocale, BYTE Platform)
{
	TMPQHash *pFirstHash = GetFirstHashEntry(ha, szFileName);
	TMPQHash *pBestEntry = NULL;
	TMPQHash *pHash = pFirstHash;

	// Parse the found hashes
	while (pHash != NULL) {
		// Storm_2016.dll: 150209CB
		// If the hash entry matches both locale and platform, return it immediately
		// Note: We only succeed this check if the locale is non-neutral, because
		// some Warcraft III maps have several items with neutral locale&platform, which leads
		// to wrong item being returned
		if ((lcLocale || Platform) && pHash->lcLocale == lcLocale && pHash->Platform == Platform) {
			return pHash;
		}

		// Storm_2016.dll: 150209D9
		// If (locale matches or is neutral) OR (platform matches or is neutral)
		// remember this as the best entry
		if (pHash->lcLocale == 0 || pHash->lcLocale == lcLocale) {
			if (pHash->Platform == 0 || pHash->Platform == Platform) {
				pBestEntry = pHash;
			}
		}

		// Get the next hash entry for that file
		pHash = GetNextHashEntry(ha, pFirstHash, pHash);
	}

	// At the end, return neutral hash (if found), otherwise NULL
	return pBestEntry;
}

// Returns the nearest higher power of two.
// If the value is already a power of two, returns the same value
static DWORD GetNearestPowerOfTwo(DWORD dwFileCount)
{
	dwFileCount --;

	dwFileCount |= dwFileCount >> 1;
	dwFileCount |= dwFileCount >> 2;
	dwFileCount |= dwFileCount >> 4;
	dwFileCount |= dwFileCount >> 8;
	dwFileCount |= dwFileCount >> 16;

	return dwFileCount + 1;
}

//
// Note: Implementation of this function in WorldEdit.exe and storm.dll
// incorrectly treats the character as signed, which leads to the
// a buffer underflow if the character in the file name >= 0x80:
// The following steps happen when *pbKey == 0xBF and dwHashType == 0x0000
// (calculating hash index)
//
// 1) Result of AsciiToUpperTable_Slash[*pbKey++] is sign-extended to 0xffffffbf
// 2) The "ch" is added to dwHashType (0xffffffbf + 0x0000 => 0xffffffbf)
// 3) The result is used as index to the StormBuffer table,
// thus dereferences a random value BEFORE the begin of StormBuffer.
//
// As result, MPQs containing files with non-ANSI characters will not work between
// various game versions and localizations. Even WorldEdit, after importing a file
// with Korean characters in the name, cannot open the file back.
//
static DWORD HashString(const char *szFileName, DWORD dwHashType)
{
	LPBYTE pbKey   = (BYTE *)szFileName;
	DWORD  dwSeed1 = 0x7FED7FED;
	DWORD  dwSeed2 = 0xEEEEEEEE;
	DWORD  ch;

	while (*pbKey != 0) {
		// Convert the input character to uppercase
		// Convert slash (0x2F) to backslash (0x5C)
		ch = AsciiToUpperTable[*pbKey++];

		dwSeed1 = StormBuffer[dwHashType + ch] ^ (dwSeed1 + dwSeed2);
		dwSeed2 = ch + dwSeed1 + dwSeed2 + (dwSeed2 << 5) + 3;
	}

	return dwSeed1;
}

static DWORD HashStringLower(const char *szFileName, DWORD dwHashType)
{
	LPBYTE pbKey   = (BYTE *)szFileName;
	DWORD  dwSeed1 = 0x7FED7FED;
	DWORD  dwSeed2 = 0xEEEEEEEE;
	DWORD  ch;

	while(*pbKey != 0) {
		// Convert the input character to lower
		// DON'T convert slash (0x2F) to backslash (0x5C)
		ch = AsciiToLowerTable[*pbKey++];

		dwSeed1 = StormBuffer[dwHashType + ch] ^ (dwSeed1 + dwSeed2);
		dwSeed2 = ch + dwSeed1 + dwSeed2 + (dwSeed2 << 5) + 3;
	}

	return dwSeed1;
}

static BOOL IsAviFile(DWORD * headerData)
{
	return headerData[0] == FOURCC_RIFF && headerData[2] == MAKEFOURCC('A', 'V', 'I', ' ') && headerData[3] == FOURCC_LIST;
}

static BOOL IsPartHeader(PPART_FILE_HEADER pPartHdr)
{
	// Version number must be 2
	if (pPartHdr->PartialVersion == 2) {
		// GameBuildNumber must be an ASCII number
		if (isdigit(pPartHdr->GameBuildNumber[0]) && isdigit(pPartHdr->GameBuildNumber[1]) && isdigit(pPartHdr->GameBuildNumber[2])) {
			// Block size must be power of 2
			if ((pPartHdr->BlockSize & (pPartHdr->BlockSize - 1)) == 0) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

// Verifies if the file name is a pseudo-name
BOOL IsPseudoFileName(const char * szFileName, DWORD * pdwFileIndex)
{
	DWORD dwFileIndex = 0;

	if (szFileName != NULL) {
		// Must be "File########.ext"
		if (!_strnicmp(szFileName, "File", 4)) {
			// Check 8 digits
			for (int i = 4; i < 4+8; i++) {
				if (szFileName[i] < '0' || szFileName[i] > '9') {
					return FALSE;
				}

				dwFileIndex = (dwFileIndex * 10) + (szFileName[i] - '0');
			}

			// An extension must follow
			if (szFileName[12] == '.') {
				if (pdwFileIndex != NULL) {
					*pdwFileIndex = dwFileIndex;
				}

				return TRUE;
			}
		}
	}

	// Not a pseudo-name
	return FALSE;
}

static TMPQArchive *IsValidMpqHandle(HANDLE hMpq)
{
	TMPQArchive *ha = (TMPQArchive *)hMpq;

	return (ha != NULL && ha->pHeader != NULL && ha->pHeader->dwID == ID_MPQ) ? ha : NULL;
}

static TMPQFile *IsValidFileHandle(HANDLE hFile)
{
	TMPQFile *hf = (TMPQFile *)hFile;

	// Must not be NULL
	if (hf != NULL && hf->dwMagic == ID_MPQ) {
		// Local file handle?
		if (hf->pStream != NULL) {
			return hf;
		}

		// Also verify the MPQ handle within the file handle
		if (IsValidMpqHandle(hf->ha)) {
			return hf;
		}
	}

	return NULL;
}

static TMPQUserData *IsValidMpqUserData(ULONGLONG ByteOffset, ULONGLONG FileSize, void * pvUserData)
{
	TMPQUserData *pUserData;

	pUserData = (TMPQUserData *)pvUserData;

	// Check the sizes
	if (pUserData->cbUserDataHeader <= pUserData->cbUserDataSize && pUserData->cbUserDataSize <= pUserData->dwHeaderOffs) {
		// Move to the position given by the userdata
		ByteOffset += pUserData->dwHeaderOffs;

		// The MPQ header should be within range of the file size
		if ((ByteOffset + MPQ_HEADER_SIZE_V1) < FileSize) {
			// Note: We should verify if there is the MPQ header.
			// However, the header could be at any position below that
			// that is multiplier of 0x200
			return (TMPQUserData *)pvUserData;
		}
	}

	return NULL;
}

static BOOL IsWarcraft3Map(DWORD * HeaderData)
{
	return (HeaderData[0] == 0x57334D48 && HeaderData[1] == 0x00000000);
}

static int LoadAnyHashTable(TMPQArchive *ha)
{
	TMPQHeader *pHeader = ha->pHeader;

	// If the MPQ archive is empty, don't bother trying to load anything
	if (pHeader->dwHashTableSize == 0 && pHeader->HetTableSize64 == 0) {
		return CreateHashTable(ha, HASH_TABLE_SIZE_DEFAULT);
	}

#ifdef FULL
	// Try to load HET table
	if (pHeader->HetTablePos64 != 0) {
		ha->pHetTable = LoadHetTable(ha);
	}
#endif
	// Try to load classic hash table
	if (pHeader->dwHashTableSize) {
		ha->pHashTable = LoadHashTable(ha);
	}

	// At least one of the tables must be present
	if (ha->pHetTable == NULL && ha->pHashTable == NULL) {
		return ERROR_FILE_CORRUPT;
	}

	// Set the maximum file count to the size of the hash table.
	// Note: We don't care about HET table limits, because HET table is rebuilt
	// after each file add/rename/delete.
	ha->dwMaxFileCount = (ha->pHashTable != NULL) ? pHeader->dwHashTableSize : HASH_TABLE_SIZE_MAX;

	return ERROR_SUCCESS;
}

static TMPQBlock *LoadBlockTable(TMPQArchive *ha, BOOL /* bDontFixEntries */)
{
	TMPQHeader *pHeader = ha->pHeader;
	TMPQBlock *pBlockTable = NULL;
	ULONGLONG ByteOffset;
	DWORD dwTableSize;
	DWORD dwCmpSize;
	BOOL bBlockTableIsCut = FALSE;

	// Note: It is possible that the block table starts at offset 0
	// Example: MPQ_2016_v1_ProtectedMap_HashOffsIsZero.w3x
//  if(pHeader->dwBlockTablePos == 0 && pHeader->wBlockTablePosHi == 0)
//      return NULL;

	// Do nothing if the block table size is zero
	if (pHeader->dwBlockTableSize == 0) {
		return NULL;
	}

	// Load the block table for MPQ variations
	switch (ha->dwSubType) {
		case MPQ_SUBTYPE_MPQ:
			// Calculate byte position of the block table
			ByteOffset = FileOffsetFromMpqOffset(ha, MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos));
			dwTableSize = pHeader->dwBlockTableSize * sizeof(TMPQBlock);
			dwCmpSize = (DWORD)pHeader->BlockTableSize64;

			// Read, decrypt and uncompress the block table
			pBlockTable = (TMPQBlock * )LoadMpqTable(ha, ByteOffset, dwCmpSize, dwTableSize, MPQ_KEY_BLOCK_TABLE, &bBlockTableIsCut);

			// If the block table was cut, we need to remember it
			if (pBlockTable != NULL && bBlockTableIsCut) {
				ha->dwFlags |= (MPQ_FLAG_MALFORMED | MPQ_FLAG_BLOCK_TABLE_CUT);
			}

			break;

		case MPQ_SUBTYPE_SQP:
			pBlockTable = LoadSqpBlockTable(ha);
			break;

		case MPQ_SUBTYPE_MPK:
			pBlockTable = LoadMpkBlockTable(ha);
			break;
	}

	return pBlockTable;
}

static TMPQHash *LoadHashTable(TMPQArchive *ha)
{
	TMPQHeader *pHeader = ha->pHeader;
	ULONGLONG ByteOffset;
	TMPQHash *pHashTable = NULL;
	DWORD dwTableSize;
	DWORD dwCmpSize;
	BOOL bHashTableIsCut = FALSE;

	// Note: It is allowed to load hash table if it is at offset 0.
	// Example: MPQ_2016_v1_ProtectedMap_HashOffsIsZero.w3x
//  if (pHeader->dwHashTablePos == 0 && pHeader->wHashTablePosHi == 0) {
//      return NULL;
//  }

	// If the hash table size is zero, do nothing
	if (pHeader->dwHashTableSize == 0) {
		return NULL;
	}

	// Load the hash table for MPQ variations
	switch (ha->dwSubType) {
		case MPQ_SUBTYPE_MPQ:
			// Calculate the position and size of the hash table
			ByteOffset = FileOffsetFromMpqOffset(ha, MAKE_OFFSET64(pHeader->wHashTablePosHi, pHeader->dwHashTablePos));
			dwTableSize = pHeader->dwHashTableSize * sizeof(TMPQHash);
			dwCmpSize = (DWORD)pHeader->HashTableSize64;

			// Read, decrypt and uncompress the hash table
			pHashTable = (TMPQHash *)LoadMpqTable(ha, ByteOffset, dwCmpSize, dwTableSize, MPQ_KEY_HASH_TABLE, &bHashTableIsCut);
//          DumpHashTable(pHashTable, pHeader->dwHashTableSize);

			// If the hash table was cut, we can/have to defragment it
			if (pHashTable != NULL && bHashTableIsCut) {
				ha->dwFlags |= (MPQ_FLAG_MALFORMED | MPQ_FLAG_HASH_TABLE_CUT);
			}

			break;

		case MPQ_SUBTYPE_SQP:
			pHashTable = LoadSqpHashTable(ha);
			break;

		case MPQ_SUBTYPE_MPK:
			pHashTable = LoadMpkHashTable(ha);
			break;
	}

	// Remember the size of the hash table
	return pHashTable;
}

static TMPQBlock *LoadMpkBlockTable(TMPQArchive *ha)
{
	TMPQHeader *pHeader = ha->pHeader;
	TMPKBlock *pMpkBlockTable;
	TMPKBlock *pMpkBlockEnd;
	TMPQBlock *pBlockTable = NULL;
	TMPKBlock *pMpkBlock;
	TMPQBlock *pMpqBlock;

	// Load and decrypt the MPK block table
	pMpkBlockTable = pMpkBlock = (TMPKBlock *)LoadMpkTable(ha, pHeader->dwBlockTablePos, pHeader->dwBlockTableSize * sizeof(TMPKBlock));

	if (pMpkBlockTable != NULL) {
		// Allocate buffer for MPQ-like block table
		pBlockTable = pMpqBlock = STORM_ALLOC(TMPQBlock, pHeader->dwBlockTableSize);

		if (pBlockTable != NULL) {
			// Convert the MPK block table to MPQ block table
			pMpkBlockEnd = pMpkBlockTable + pHeader->dwBlockTableSize;

			while (pMpkBlock < pMpkBlockEnd) {
				// Translate the MPK block table entry to MPQ block table entry
				pMpqBlock->dwFilePos = pMpkBlock->dwFilePos;
				pMpqBlock->dwCSize = pMpkBlock->dwCSize;
				pMpqBlock->dwFSize = pMpkBlock->dwFSize;
				pMpqBlock->dwFlags = ConvertMpkFlagsToMpqFlags(pMpkBlock->dwFlags);

				// Move both
				pMpkBlock++;
				pMpqBlock++;
			}
		}

		// Free the MPK block table
		STORM_FREE(pMpkBlockTable);
	}

	return pBlockTable;
}

static TMPQHash *LoadMpkHashTable(TMPQArchive *ha)
{
	TMPQHeader *pHeader = ha->pHeader;
	TMPQHash *pHashTable = NULL;
	TMPKHash *pMpkHash;
	TMPQHash *pHash = NULL;
	DWORD dwHashTableSize = pHeader->dwHashTableSize;

	// MPKs use different hash table searching.
	// Instead of using MPQ_HASH_TABLE_INDEX hash as index,
	// they store the value directly in the hash table.
	// Also for faster searching, the hash table is sorted ascending by the value

	// Load and decrypt the MPK hash table.
	pMpkHash = (TMPKHash *)LoadMpkTable(ha, pHeader->dwHashTablePos, pHeader->dwHashTableSize * sizeof(TMPKHash));

	if (pMpkHash != NULL) {
		// Calculate the hash table size as if it was real MPQ hash table
		pHeader->dwHashTableSize = GetNearestPowerOfTwo(pHeader->dwHashTableSize);
		pHeader->HashTableSize64 = pHeader->dwHashTableSize * sizeof(TMPQHash);

		// Now allocate table that will serve like a true MPQ hash table,
		// so we translate the MPK hash table to MPQ hash table
		pHashTable = STORM_ALLOC(TMPQHash, pHeader->dwHashTableSize);

		if (pHashTable != NULL) {
			// Set the entire hash table to free
			memset(pHashTable, 0xFF, (size_t)pHeader->HashTableSize64);

			// Copy the MPK hash table into MPQ hash table
			for (DWORD i = 0; i < dwHashTableSize; i++) {
				// Finds the free hash entry in the hash table
				// We don't expect any errors here, because we are putting files to empty hash table
				pHash = FindFreeHashEntry(pHashTable, pHeader->dwHashTableSize, pMpkHash[i].dwName1);
				assert(pHash->dwBlockIndex == HASH_ENTRY_FREE);

				// Copy the MPK hash entry to the hash table
				pHash->dwBlockIndex = pMpkHash[i].dwBlockIndex;
				pHash->Platform = 0;
				pHash->lcLocale = 0;
				pHash->dwName1 = pMpkHash[i].dwName2;
				pHash->dwName2 = pMpkHash[i].dwName3;
			}
		}

		// Free the temporary hash table
		STORM_FREE(pMpkHash);
	}

	return pHashTable;
}

static void *LoadMpkTable(TMPQArchive *ha, DWORD dwByteOffset, DWORD cbTableSize)
{
	ULONGLONG ByteOffset;
	LPBYTE pbMpkTable = NULL;

	// Allocate space for the table
	pbMpkTable = STORM_ALLOC(BYTE, cbTableSize);

	if (pbMpkTable != NULL) {
		// Load and the MPK hash table
		ByteOffset = ha->MpqPos + dwByteOffset;
		if (FileStream_Read(ha->pStream, &ByteOffset, pbMpkTable, cbTableSize)) {
			// Decrypt the table
			DecryptMpkTable(pbMpkTable, cbTableSize);
			return pbMpkTable;
		}

		// Free the MPK table
		STORM_FREE(pbMpkTable);
		pbMpkTable = NULL;
	}

	// Return the table
	return pbMpkTable;
}

static BOOL MpqeStream_BlockRead(
	TEncryptedStream *pStream,
	ULONGLONG StartOffset,
	ULONGLONG EndOffset,
	LPBYTE BlockBuffer,
	DWORD BytesNeeded,
	BOOL bAvailable)
{
	DWORD dwBytesToRead;

	assert((StartOffset & (pStream->BlockSize - 1)) == 0);
	assert(StartOffset < EndOffset);
	assert(bAvailable != FALSE);
	BytesNeeded = BytesNeeded;
	bAvailable = bAvailable;

	// Read the file from the stream as-is
	// Limit the reading to number of blocks really needed
	dwBytesToRead = (DWORD)(EndOffset - StartOffset);
	if (!pStream->BaseRead(pStream, &StartOffset, BlockBuffer, dwBytesToRead)) {
		return FALSE;
	}

	// Decrypt the data
	dwBytesToRead = (dwBytesToRead + MPQE_CHUNK_SIZE - 1) & ~(MPQE_CHUNK_SIZE - 1);
	DecryptFileChunk((LPDWORD)BlockBuffer, pStream->Key, StartOffset, dwBytesToRead);
	return TRUE;
}

static bool MpqeStream_DetectFileKey(TEncryptedStream *pStream)
{
	ULONGLONG ByteOffset = 0;
	BYTE EncryptedHeader[MPQE_CHUNK_SIZE];
	BYTE FileHeader[MPQE_CHUNK_SIZE];

	// Read the first file chunk
	if (pStream->BaseRead(pStream, &ByteOffset, EncryptedHeader, sizeof(EncryptedHeader))) {
		// We just try all known keys one by one
		for (int i = 0; AuthCodeArray[i] != NULL; i++) {
			// Prepare they decryption key from game serial number
			CreateKeyFromAuthCode(pStream->Key, AuthCodeArray[i]);

			// Try to decrypt with the given key 
			memcpy(FileHeader, EncryptedHeader, MPQE_CHUNK_SIZE);
			DecryptFileChunk((LPDWORD)FileHeader, pStream->Key, ByteOffset, MPQE_CHUNK_SIZE);

			// We check the decrypted data
			// All known encrypted MPQs have header at the begin of the file,
			// so we check for MPQ signature there.
			if (FileHeader[0] == 'M' && FileHeader[1] == 'P' && FileHeader[2] == 'Q') {
				// Update the stream size
				pStream->StreamSize = pStream->Base.File.FileSize;

				// Fill the block information
				pStream->BlockSize  = MPQE_CHUNK_SIZE;
				pStream->BlockCount = (DWORD)(pStream->Base.File.FileSize + MPQE_CHUNK_SIZE - 1) / MPQE_CHUNK_SIZE;
				pStream->IsComplete = 1;
				return TRUE;
			}
		}
	}

	// Key not found, sorry
	return FALSE;
}

static TFileStream *MpqeStream_Open(const TCHAR *szFileName, DWORD dwStreamFlags)
{
	TEncryptedStream *pStream;

	// Create new empty stream
	pStream = (TEncryptedStream *)AllocateFileStream(szFileName, sizeof(TEncryptedStream), dwStreamFlags);
	if (pStream == NULL) {
		return NULL;
	}

	// Attempt to open the base stream
	assert(pStream->BaseOpen != NULL);

	if (!pStream->BaseOpen(pStream, pStream->szFileName, dwStreamFlags)) {
		return NULL;
	}

	// Determine the encryption key for the MPQ
	if (MpqeStream_DetectFileKey(pStream)) {
		// Set the stream position and size
		assert(pStream->StreamSize != 0);
		pStream->StreamPos = 0;
		pStream->dwFlags |= STREAM_FLAG_READ_ONLY;

		// Set new function pointers
		pStream->StreamRead    = (STREAM_READ)BlockStream_Read;
		pStream->StreamGetPos  = BlockStream_GetPos;
		pStream->StreamGetSize = BlockStream_GetSize;
		pStream->StreamClose   = pStream->BaseClose;

		// Supply the block functions
		pStream->BlockRead     = (BLOCK_READ)MpqeStream_BlockRead;
		return pStream;
	}

	// Cleanup the stream and return
	FileStream_Close(pStream);
	SErrSetLastError(ERROR_UNKNOWN_FILE_KEY);
	return NULL;
}

// Loads a table from MPQ.
// Can be used for hash table, block table, sector offset table or sector checksum table
static void *LoadMpqTable(TMPQArchive *ha, ULONGLONG ByteOffset, DWORD dwCompressedSize, DWORD dwTableSize, DWORD dwKey, BOOL *pbTableIsCut)
{
	ULONGLONG FileSize = 0;
	LPBYTE pbCompressed = NULL;
	LPBYTE pbMpqTable;
	LPBYTE pbToRead;
	DWORD dwBytesToRead = dwCompressedSize;
	int nError = ERROR_SUCCESS;

	// Allocate the MPQ table
	pbMpqTable = pbToRead = STORM_ALLOC(BYTE, dwTableSize);

	if (pbMpqTable != NULL) {
		// Check if the MPQ table is encrypted
		if (dwCompressedSize < dwTableSize) {
			// Allocate temporary buffer for holding compressed data
			pbCompressed = pbToRead = STORM_ALLOC(BYTE, dwCompressedSize);
			if (pbCompressed == NULL) {
				STORM_FREE(pbMpqTable);
				return NULL;
			}
		}

		// Get the file offset from which we will read the table
		// Note: According to Storm.dll from Warcraft III (version 2002),
		// if the hash table position is 0xFFFFFFFF, no SetFilePointer call is done
		// and the table is loaded from the current file offset
		if (ByteOffset == SFILE_INVALID_POS) {
			FileStream_GetPos(ha->pStream, &ByteOffset);
		}

		// On archives v 1.0, hash table and block table can go beyond EOF.
		// Storm.dll reads as much as possible, then fills the missing part with zeros.
		// Abused by Spazzler map protector which sets hash table size to 0x00100000
		if (ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1) {
			// Cut the table size
			FileStream_GetSize(ha->pStream, &FileSize);

			if ((ByteOffset + dwBytesToRead) > FileSize) {
				// Fill the extra data with zeros
				dwBytesToRead = (DWORD)(FileSize - ByteOffset);
				memset(pbMpqTable + dwBytesToRead, 0, (dwTableSize - dwBytesToRead));

				// Give the caller information that the table was cut
				if (pbTableIsCut != NULL) {
					pbTableIsCut[0] = true;
				}
			}
		}

		// If everything succeeded, read the raw table form the MPQ
		if (FileStream_Read(ha->pStream, &ByteOffset, pbToRead, dwBytesToRead)) {
			// First of all, decrypt the table
			if (dwKey != 0) {
				DecryptMpqBlock(pbToRead, dwCompressedSize, dwKey);
			}

			// If the table is compressed, decompress it
			if (dwCompressedSize < dwTableSize) {
				int cbOutBuffer = (int)dwTableSize;
				int cbInBuffer = (int)dwCompressedSize;

				if (!SCompDecompress2(pbMpqTable, &cbOutBuffer, pbCompressed, cbInBuffer)) {
					nError = GetLastError();
				}
			}
		} else {
			nError = GetLastError();
		}

		// If read failed, free the table and return
		if (nError != ERROR_SUCCESS) {
			STORM_FREE(pbMpqTable);
			pbMpqTable = NULL;
		}

		// Free the compression buffer, if any
		if (pbCompressed != NULL) {
			STORM_FREE(pbCompressed);
		}
	}

	// Return the MPQ table
	return pbMpqTable;
}

// Loads the SQP Block table and converts it to a MPQ block table
static TMPQBlock *LoadSqpBlockTable(TMPQArchive *ha)
{
	TMPQHeader *pHeader = ha->pHeader;
	TSQPBlock *pSqpBlockTable;
	TSQPBlock *pSqpBlockEnd;
	TSQPBlock *pSqpBlock;
	TMPQBlock *pMpqBlock;
	DWORD dwFlags;
	int nError = ERROR_SUCCESS;

	// Load the hash table
	pSqpBlockTable = (TSQPBlock *)LoadSqpTable(ha, pHeader->dwBlockTablePos, pHeader->dwBlockTableSize * sizeof(TSQPBlock), MPQ_KEY_BLOCK_TABLE);

	if (pSqpBlockTable != NULL) {
		// Parse the entire hash table and convert it to MPQ hash table
		pSqpBlockEnd = pSqpBlockTable + pHeader->dwBlockTableSize;
		pMpqBlock = (TMPQBlock *)pSqpBlockTable;

		for (pSqpBlock = pSqpBlockTable; pSqpBlock < pSqpBlockEnd; pSqpBlock++, pMpqBlock++) {
			// Check for valid flags
			if (pSqpBlock->dwFlags & ~MPQ_FILE_VALID_FLAGS) {
				nError = ERROR_FILE_CORRUPT;
			}

			// Convert SQP block table entry to MPQ block table entry
			dwFlags = pSqpBlock->dwFlags;
			pMpqBlock->dwCSize = pSqpBlock->dwCSize;
			pMpqBlock->dwFSize = pSqpBlock->dwFSize;
			pMpqBlock->dwFlags = dwFlags;
		}

		// If an error occured, we need to free the hash table
		if (nError != ERROR_SUCCESS) {
			STORM_FREE(pSqpBlockTable);
			pSqpBlockTable = NULL;
		}
	}

	// Return the converted hash table (or NULL on failure)
	return (TMPQBlock *)pSqpBlockTable;
}

static TMPQHash *LoadSqpHashTable(TMPQArchive *ha)
{
	TMPQHeader *pHeader = ha->pHeader;
	TSQPHash *pSqpHashTable;
	TSQPHash *pSqpHashEnd;
	TSQPHash *pSqpHash;
	TMPQHash *pMpqHash;
	int nError = ERROR_SUCCESS;

	// Load the hash table
	pSqpHashTable = (TSQPHash *)LoadSqpTable(ha, pHeader->dwHashTablePos, pHeader->dwHashTableSize * sizeof(TSQPHash), MPQ_KEY_HASH_TABLE);

	if (pSqpHashTable != NULL) {
		// Parse the entire hash table and convert it to MPQ hash table
		pSqpHashEnd = pSqpHashTable + pHeader->dwHashTableSize;
		pMpqHash = (TMPQHash *)pSqpHashTable;

		for (pSqpHash = pSqpHashTable; pSqpHash < pSqpHashEnd; pSqpHash++, pMpqHash++) {
			// Ignore free entries
			if (pSqpHash->dwBlockIndex != HASH_ENTRY_FREE) {
				// Check block index against the size of the block table
				if (pHeader->dwBlockTableSize <= MPQ_BLOCK_INDEX(pSqpHash) && pSqpHash->dwBlockIndex < HASH_ENTRY_DELETED) {
					nError = ERROR_FILE_CORRUPT;
				}

				// We do not support nonzero locale and platform ID
				if (pSqpHash->dwAlwaysZero != 0 && pSqpHash->dwAlwaysZero != HASH_ENTRY_FREE) {
					nError = ERROR_FILE_CORRUPT;
				}

				// Store the file name hash
				pMpqHash->dwName1 = pSqpHash->dwName1;
				pMpqHash->dwName2 = pSqpHash->dwName2;

				// Store the rest. Note that this must be done last,
				// because block index corresponds to pMpqHash->dwName2
				pMpqHash->dwBlockIndex = MPQ_BLOCK_INDEX(pSqpHash);
				pMpqHash->Platform = 0;
				pMpqHash->lcLocale = 0;
			}
		}

		// If an error occured, we need to free the hash table
		if (nError != ERROR_SUCCESS) {
			STORM_FREE(pSqpHashTable);
			pSqpHashTable = NULL;
		}
	}

	// Return the converted hash table (or NULL on failure)
	return (TMPQHash *)pSqpHashTable;
}

static void *LoadSqpTable(TMPQArchive *ha, DWORD dwByteOffset, DWORD cbTableSize, DWORD dwKey)
{
	ULONGLONG ByteOffset;
	LPBYTE pbSqpTable;

	// Allocate buffer for the table
	pbSqpTable = STORM_ALLOC(BYTE, cbTableSize);

	if (pbSqpTable != NULL) {
		// Load the table
		ByteOffset = ha->MpqPos + dwByteOffset;

		if (FileStream_Read(ha->pStream, &ByteOffset, pbSqpTable, cbTableSize)) {
			// Decrypt the SQP table
			DecryptMpqBlock(pbSqpTable, cbTableSize, dwKey);
			return pbSqpTable;
		}

		// Free the table
		STORM_FREE(pbSqpTable);
	}

	return NULL;
}

static int ReadMpkFileSingleUnit(TMPQFile *hf, void *pvBuffer, DWORD dwFilePos, DWORD dwToRead, LPDWORD pdwBytesRead)
{
	ULONGLONG RawFilePos = hf->RawFilePos + 0x0C;   // For some reason, MPK files start at position (hf->RawFilePos + 0x0C)
	TMPQArchive *ha = hf->ha;
	TFileEntry *pFileEntry = hf->pFileEntry;
	LPBYTE pbCompressed = NULL;
	LPBYTE pbRawData = hf->pbFileSector;
	int nError = ERROR_SUCCESS;

	// We do not support patch files in MPK archives
	assert(hf->pPatchInfo == NULL);

	// If the file buffer is not allocated yet, do it.
	if (hf->pbFileSector == NULL) {
		nError = AllocateSectorBuffer(hf);

		if (nError != ERROR_SUCCESS || hf->pbFileSector == NULL) {
			return nError;
		}

		pbRawData = hf->pbFileSector;
	}

	// If the file sector is not loaded yet, do it
	if (hf->dwSectorOffs != 0) {
		// Is the file compressed?
		if (pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK) {
			// Allocate space for compressed data
			pbCompressed = STORM_ALLOC(BYTE, pFileEntry->dwCmpSize);

			if (pbCompressed == NULL) {
				return ERROR_NOT_ENOUGH_MEMORY;
			}

			pbRawData = pbCompressed;
		}

		// Load the raw (compressed, encrypted) data
		if (!FileStream_Read(ha->pStream, &RawFilePos, pbRawData, pFileEntry->dwCmpSize)) {
			STORM_FREE(pbCompressed);
			return GetLastError();
		}

		// If the file is encrypted, we have to decrypt the data first
		if (pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED) {
			DecryptMpkTable(pbRawData, pFileEntry->dwCmpSize);
		}

		// If the file is compressed, we have to decompress it now
		if (pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK) {
#ifdef FULL
			int cbOutBuffer = (int)hf->dwDataSize;

			hf->dwCompression0 = pbRawData[0];

			if (!SCompDecompressMpk(hf->pbFileSector, &cbOutBuffer, pbRawData, (int)pFileEntry->dwCmpSize)) {
				nError = ERROR_FILE_CORRUPT;
			}
#else
			assert(0);
#endif
		} else {
			if (pbRawData != hf->pbFileSector) {
				memcpy(hf->pbFileSector, pbRawData, hf->dwDataSize);
			}
		}

		// Free the decompression buffer.
		if (pbCompressed != NULL) {
			STORM_FREE(pbCompressed);
		}

		// The file sector is now properly loaded
		hf->dwSectorOffs = 0;
	}

	// At this moment, we have the file loaded into the file buffer.
	// Copy as much as the caller wants
	if (nError == ERROR_SUCCESS && hf->dwSectorOffs == 0) {
		// File position is greater or equal to file size ?
		if (dwFilePos >= hf->dwDataSize) {
			*pdwBytesRead = 0;
			return ERROR_SUCCESS;
		}

		// If not enough bytes remaining in the file, cut them
		if ((hf->dwDataSize - dwFilePos) < dwToRead) {
			dwToRead = (hf->dwDataSize - dwFilePos);
		}

		// Copy the bytes
		memcpy(pvBuffer, hf->pbFileSector + dwFilePos, dwToRead);

		// Give the number of bytes read
		*pdwBytesRead = dwToRead;
		return ERROR_SUCCESS;
	}

	// An error, sorry
	return ERROR_CAN_NOT_COMPLETE;
}

static int ReadMpqFileLocalFile(TMPQFile * hf, void * pvBuffer, DWORD dwFilePos, DWORD dwToRead, LPDWORD pdwBytesRead)
{
	ULONGLONG FilePosition1 = dwFilePos;
	ULONGLONG FilePosition2;
	DWORD dwBytesRead = 0;
	int nError = ERROR_SUCCESS;

	assert(hf->pStream != NULL);

	// Because stream I/O functions are designed to read
	// "all or nothing", we compare file position before and after,
	// and if they differ, we assume that number of bytes read
	// is the difference between them

	if (!FileStream_Read(hf->pStream, &FilePosition1, pvBuffer, dwToRead)) {
		// If not all bytes have been read, then return the number of bytes read
		if ((nError = GetLastError()) == ERROR_HANDLE_EOF) {
			FileStream_GetPos(hf->pStream, &FilePosition2);
			dwBytesRead = (DWORD)(FilePosition2 - FilePosition1);
		}
	} else {
		dwBytesRead = dwToRead;
	}

	*pdwBytesRead = dwBytesRead;
	return nError;
}

static int ReadMpqFileSectorFile(TMPQFile *hf, void *pvBuffer, DWORD dwFilePos, DWORD dwBytesToRead, LPDWORD pdwBytesRead)
{
	TMPQArchive *ha = hf->ha;
	LPBYTE pbBuffer = (BYTE *)pvBuffer;
	DWORD dwTotalBytesRead = 0;                         // Total bytes read in all three parts
	DWORD dwSectorSizeMask = ha->dwSectorSize - 1;      // Mask for block size, usually 0x0FFF
	DWORD dwFileSectorPos;                              // File offset of the loaded sector
	DWORD dwBytesRead;                                  // Number of bytes read (temporary variable)
	int nError;

	// If the file position is at or beyond end of file, do nothing
	if (dwFilePos >= hf->dwDataSize) {
		*pdwBytesRead = 0;
		return ERROR_SUCCESS;
	}

	// If not enough bytes in the file remaining, cut them
	if (dwBytesToRead > (hf->dwDataSize - dwFilePos)) {
		dwBytesToRead = (hf->dwDataSize - dwFilePos);
	}

	// Compute sector position in the file
	dwFileSectorPos = dwFilePos & ~dwSectorSizeMask;  // Position in the block

	// If the file sector buffer is not allocated yet, do it now
	if (hf->pbFileSector == NULL) {
		nError = AllocateSectorBuffer(hf);

		if (nError != ERROR_SUCCESS || hf->pbFileSector == NULL) {
			return nError;
		}
	}

	// Load the first (incomplete) file sector
	if (dwFilePos & dwSectorSizeMask) {
		DWORD dwBytesInSector = ha->dwSectorSize;
		DWORD dwBufferOffs = dwFilePos & dwSectorSizeMask;
		DWORD dwToCopy;

		// Is the file sector already loaded ?
		if (hf->dwSectorOffs != dwFileSectorPos) {
			// Load one MPQ sector into archive buffer
			nError = ReadMpqSectors(hf, hf->pbFileSector, dwFileSectorPos, ha->dwSectorSize, &dwBytesInSector);

			if (nError != ERROR_SUCCESS) {
				return nError;
			}

			// Remember that the data loaded to the sector have new file offset
			hf->dwSectorOffs = dwFileSectorPos;
		} else {
			if ((dwFileSectorPos + dwBytesInSector) > hf->dwDataSize) {
				dwBytesInSector = hf->dwDataSize - dwFileSectorPos;
			}
		}

		// Copy the data from the offset in the loaded sector to the end of the sector
		dwToCopy = dwBytesInSector - dwBufferOffs;

		if (dwToCopy > dwBytesToRead) {
			dwToCopy = dwBytesToRead;
		}

		// Copy data from sector buffer into target buffer
		memcpy(pbBuffer, hf->pbFileSector + dwBufferOffs, dwToCopy);

		// Update pointers and byte counts
		dwTotalBytesRead += dwToCopy;
		dwFileSectorPos  += dwBytesInSector;
		pbBuffer         += dwToCopy;
		dwBytesToRead    -= dwToCopy;
	}

	// Load the whole ("middle") sectors only if there is at least one full sector to be read
	if (dwBytesToRead >= ha->dwSectorSize) {
		DWORD dwBlockBytes = dwBytesToRead & ~dwSectorSizeMask;

		// Load all sectors to the output buffer
		nError = ReadMpqSectors(hf, pbBuffer, dwFileSectorPos, dwBlockBytes, &dwBytesRead);
		if (nError != ERROR_SUCCESS) {
			return nError;
		}

		// Update pointers
		dwTotalBytesRead += dwBytesRead;
		dwFileSectorPos  += dwBytesRead;
		pbBuffer         += dwBytesRead;
		dwBytesToRead    -= dwBytesRead;
	}

	// Read the terminating sector
	if (dwBytesToRead > 0) {
		DWORD dwToCopy = ha->dwSectorSize;

		// Is the file sector already loaded ?
		if (hf->dwSectorOffs != dwFileSectorPos) {
			// Load one MPQ sector into archive buffer
			nError = ReadMpqSectors(hf, hf->pbFileSector, dwFileSectorPos, ha->dwSectorSize, &dwBytesRead);
			if (nError != ERROR_SUCCESS) {
				return nError;
			}

			// Remember that the data loaded to the sector have new file offset
			hf->dwSectorOffs = dwFileSectorPos;
		}

		// Check number of bytes read
		if (dwToCopy > dwBytesToRead) {
			dwToCopy = dwBytesToRead;
		}

		// Copy the data from the cached last sector to the caller's buffer
		memcpy(pbBuffer, hf->pbFileSector, dwToCopy);

		// Update pointers
		dwTotalBytesRead += dwToCopy;
	}

	// Store total number of bytes read to the caller
	*pdwBytesRead = dwTotalBytesRead;
	return ERROR_SUCCESS;
}

static int ReadMpqSectors(TMPQFile *hf, LPBYTE pbBuffer, DWORD dwByteOffset, DWORD dwBytesToRead, LPDWORD pdwBytesRead)
{
	ULONGLONG RawFilePos;
	TMPQArchive *ha = hf->ha;
	TFileEntry *pFileEntry = hf->pFileEntry;
	LPBYTE pbRawSector = NULL;
	LPBYTE pbOutSector = pbBuffer;
	LPBYTE pbInSector = pbBuffer;
	DWORD dwRawBytesToRead;
	DWORD dwRawSectorOffset = dwByteOffset;
	DWORD dwSectorsToRead = dwBytesToRead / ha->dwSectorSize;
	DWORD dwSectorIndex = dwByteOffset / ha->dwSectorSize;
	DWORD dwSectorsDone = 0;
	DWORD dwBytesRead = 0;
	int nError = ERROR_SUCCESS;

	// Note that dwByteOffset must be aligned to size of one sector
	// Note that dwBytesToRead must be a multiplier of one sector size
	// This is local function, so we won't check if that's true.
	// Note that files stored in single units are processed by a separate function

	// If there is not enough bytes remaining, cut dwBytesToRead
	if ((dwByteOffset + dwBytesToRead) > hf->dwDataSize) {
		dwBytesToRead = hf->dwDataSize - dwByteOffset;
	}

	dwRawBytesToRead = dwBytesToRead;

	// Perform all necessary work to do with compressed files
	if (pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK) {
		// If the sector positions are not loaded yet, do it
		if (hf->SectorOffsets == NULL) {
			nError = AllocateSectorOffsets(hf, true);

			if (nError != ERROR_SUCCESS || hf->SectorOffsets == NULL) {
				return nError;
			}
		}

		// If the sector checksums are not loaded yet, load them now.
		if (hf->SectorChksums == NULL && (pFileEntry->dwFlags & MPQ_FILE_SECTOR_CRC) && hf->bLoadedSectorCRCs == false) {
			//
			// Sector CRCs is plain crap feature. It is almost never present,
			// often it's empty, or the end offset of sector CRCs is zero.
			// We only try to load sector CRCs once, and regardless if it fails
			// or not, we won't try that again for the given file.
			//

			AllocateSectorChecksums(hf, true);
			hf->bLoadedSectorCRCs = true;
		}

		// TODO: If the raw data MD5s are not loaded yet, load them now
		// Only do it if the MPQ is of format 4.0
//      if (ha->pHeader->wFormatVersion >= MPQ_FORMAT_VERSION_4 && ha->pHeader->dwRawChunkSize != 0) {
//          nError = AllocateRawMD5s(hf, true);
//
//          if (nError != ERROR_SUCCESS) {
//              return nError;
//          }
//      }

		// Assign the temporary buffer as target for read operation
		dwRawSectorOffset = hf->SectorOffsets[dwSectorIndex];
		dwRawBytesToRead = hf->SectorOffsets[dwSectorIndex + dwSectorsToRead] - dwRawSectorOffset;

		// If the file is compressed, also allocate secondary buffer
		pbInSector = pbRawSector = STORM_ALLOC(BYTE, dwRawBytesToRead);
		
		if (pbRawSector == NULL) {
			return ERROR_NOT_ENOUGH_MEMORY;
		}
	}

	// Calculate raw file offset where the sector(s) are stored.
	RawFilePos = CalculateRawSectorOffset(hf, dwRawSectorOffset);

	// Set file pointer and read all required sectors
	if (FileStream_Read(ha->pStream, &RawFilePos, pbInSector, dwRawBytesToRead)) {
		// Now we have to decrypt and decompress all file sectors that have been loaded
		for (DWORD i = 0; i < dwSectorsToRead; i++) {
			DWORD dwRawBytesInThisSector = ha->dwSectorSize;
			DWORD dwBytesInThisSector = ha->dwSectorSize;
			DWORD dwIndex = dwSectorIndex + i;

			// If there is not enough bytes in the last sector,
			// cut the number of bytes in this sector
			if (dwRawBytesInThisSector > dwBytesToRead) {
				dwRawBytesInThisSector = dwBytesToRead;
			}

			if (dwBytesInThisSector > dwBytesToRead) {
				dwBytesInThisSector = dwBytesToRead;
			}

			// If the file is compressed, we have to adjust the raw sector size
			if (pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK) {
				dwRawBytesInThisSector = hf->SectorOffsets[dwIndex + 1] - hf->SectorOffsets[dwIndex];
			}

			// If the file is encrypted, we have to decrypt the sector
			if (pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED) {
				// If we don't know the key, try to detect it by file content
				if (hf->dwFileKey == 0) {
					hf->dwFileKey = DetectFileKeyByContent(pbInSector, dwBytesInThisSector, hf->dwDataSize);

					if (hf->dwFileKey == 0) {
						nError = ERROR_UNKNOWN_FILE_KEY;
						break;
					}
				}

				DecryptMpqBlock(pbInSector, dwRawBytesInThisSector, hf->dwFileKey + dwIndex);
			}

#ifdef FULL
			// If the file has sector CRC check turned on, perform it
			if (hf->bCheckSectorCRCs && hf->SectorChksums != NULL) {
				DWORD dwAdlerExpected = hf->SectorChksums[dwIndex];
				DWORD dwAdlerValue = 0;

				// We can only check sector CRC when it's not zero
				// Neither can we check it if it's 0xFFFFFFFF.
				if (dwAdlerExpected != 0 && dwAdlerExpected != 0xFFFFFFFF) {
					dwAdlerValue = adler32(0, pbInSector, dwRawBytesInThisSector);

					if (dwAdlerValue != dwAdlerExpected) {
						nError = ERROR_CHECKSUM_ERROR;
						break;
					}
				}
			}
#endif

			// If the sector is really compressed, decompress it.
			// WARNING : Some sectors may not be compressed, it can be determined only
			// by comparing uncompressed and compressed size !!!
			if (dwRawBytesInThisSector < dwBytesInThisSector) {
				int cbOutSector = dwBytesInThisSector;
				int cbInSector = dwRawBytesInThisSector;
				int nResult = 0;

				// Is the file compressed by Blizzard's multiple compression ?
				if (pFileEntry->dwFlags & MPQ_FILE_COMPRESS) {
					// Remember the last used compression
					hf->dwCompression0 = pbInSector[0];

					// Decompress the data
					if (ha->pHeader->wFormatVersion >= MPQ_FORMAT_VERSION_2) {
						nResult = SCompDecompress2(pbOutSector, &cbOutSector, pbInSector, cbInSector);
					} else {
						nResult = SCompDecompress(pbOutSector, &cbOutSector, pbInSector, cbInSector);
					}
				}

				// Is the file compressed by PKWARE Data Compression Library ?
				else if (pFileEntry->dwFlags & MPQ_FILE_IMPLODE) {
					nResult = SCompExplode(pbOutSector, &cbOutSector, pbInSector, cbInSector);
				}

				// Did the decompression fail ?
				if (nResult == 0) {
					nError = ERROR_FILE_CORRUPT;
					break;
				}
			} else {
				if (pbOutSector != pbInSector) {
					memcpy(pbOutSector, pbInSector, dwBytesInThisSector);
				}
			}

			// Move pointers
			dwBytesToRead -= dwBytesInThisSector;
			dwByteOffset += dwBytesInThisSector;
			dwBytesRead += dwBytesInThisSector;
			pbOutSector += dwBytesInThisSector;
			pbInSector += dwRawBytesInThisSector;
			dwSectorsDone++;
		}
	} else {
		nError = GetLastError();
	}

	// Free all used buffers
	if (pbRawSector != NULL) {
		STORM_FREE(pbRawSector);
	}

	// Give the caller thenumber of bytes read
	*pdwBytesRead = dwBytesRead;
	return nError;
}

static int ReadMpqFileSingleUnit(TMPQFile *hf, void *pvBuffer, DWORD dwFilePos, DWORD dwToRead, LPDWORD pdwBytesRead)
{
	ULONGLONG RawFilePos = hf->RawFilePos;
	TMPQArchive *ha = hf->ha;
	TFileEntry *pFileEntry = hf->pFileEntry;
	LPBYTE pbCompressed = NULL;
	LPBYTE pbRawData;
	int nError = ERROR_SUCCESS;

	// If the file buffer is not allocated yet, do it.
	if (hf->pbFileSector == NULL) {
		nError = AllocateSectorBuffer(hf);

		if (nError != ERROR_SUCCESS || hf->pbFileSector == NULL) {
			return nError;
		}
	}

	// If the file is a patch file, adjust raw data offset
	if (hf->pPatchInfo != NULL) {
		RawFilePos += hf->pPatchInfo->dwLength;
	}

	pbRawData = hf->pbFileSector;

	// If the file sector is not loaded yet, do it
	if (hf->dwSectorOffs != 0) {
		// Is the file compressed?
		if (pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK) {
			// Allocate space for compressed data
			pbCompressed = STORM_ALLOC(BYTE, pFileEntry->dwCmpSize);

			if (pbCompressed == NULL) {
				return ERROR_NOT_ENOUGH_MEMORY;
			}

			pbRawData = pbCompressed;
		}

		// Load the raw (compressed, encrypted) data
		if (!FileStream_Read(ha->pStream, &RawFilePos, pbRawData, pFileEntry->dwCmpSize)) {
			STORM_FREE(pbCompressed);
			return GetLastError();
		}

		// If the file is encrypted, we have to decrypt the data first
		if (pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED) {
			DecryptMpqBlock(pbRawData, pFileEntry->dwCmpSize, hf->dwFileKey);
		}

		// If the file is compressed, we have to decompress it now
		if (pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK) {
			int cbOutBuffer = (int)hf->dwDataSize;
			int cbInBuffer = (int)pFileEntry->dwCmpSize;
			int nResult = 0;

			//
			// If the file is an incremental patch, the size of compressed data
			// is determined as pFileEntry->dwCmpSize - sizeof(TPatchInfo)
			//
			// In "wow-update-12694.MPQ" from Wow-Cataclysm BETA:
			//
			// File                                    CmprSize   DcmpSize DataSize Compressed?
			// --------------------------------------  ---------- -------- -------- ---------------
			// esES\DBFilesClient\LightSkyBox.dbc      0xBE->0xA2  0xBC     0xBC     Yes
			// deDE\DBFilesClient\MountCapability.dbc  0x93->0x77  0x77     0x77     No
			//

			if (pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) {
				cbInBuffer = cbInBuffer - sizeof(TPatchInfo);
			}

			// Is the file compressed by Blizzard's multiple compression ?
			if (pFileEntry->dwFlags & MPQ_FILE_COMPRESS) {
				// Remember the last used compression
				hf->dwCompression0 = pbRawData[0];

				// Decompress the file
				if (ha->pHeader->wFormatVersion >= MPQ_FORMAT_VERSION_2) {
					nResult = SCompDecompress2(hf->pbFileSector, &cbOutBuffer, pbRawData, cbInBuffer);
				} else {
					nResult = SCompDecompress(hf->pbFileSector, &cbOutBuffer, pbRawData, cbInBuffer);
				}
			}

			// Is the file compressed by PKWARE Data Compression Library ?
			// Note: Single unit files compressed with IMPLODE are not supported by Blizzard
			else if (pFileEntry->dwFlags & MPQ_FILE_IMPLODE) {
				nResult = SCompExplode(hf->pbFileSector, &cbOutBuffer, pbRawData, cbInBuffer);
			}

			nError = (nResult != 0) ? ERROR_SUCCESS : ERROR_FILE_CORRUPT;
		} else {
			if (hf->pbFileSector != NULL && pbRawData != hf->pbFileSector) {
				memcpy(hf->pbFileSector, pbRawData, hf->dwDataSize);
			}
		}

		// Free the decompression buffer.
		if (pbCompressed != NULL) {
			STORM_FREE(pbCompressed);
		}

		// The file sector is now properly loaded
		hf->dwSectorOffs = 0;
	}

	// At this moment, we have the file loaded into the file buffer.
	// Copy as much as the caller wants
	if (nError == ERROR_SUCCESS && hf->dwSectorOffs == 0) {
		// File position is greater or equal to file size ?
		if (dwFilePos >= hf->dwDataSize) {
			*pdwBytesRead = 0;
			return ERROR_SUCCESS;
		}

		// If not enough bytes remaining in the file, cut them
		if ((hf->dwDataSize - dwFilePos) < dwToRead) {
			dwToRead = (hf->dwDataSize - dwFilePos);
		}

		// Copy the bytes
		memcpy(pvBuffer, hf->pbFileSector + dwFilePos, dwToRead);

		// Give the number of bytes read
		*pdwBytesRead = dwToRead;
		return ERROR_SUCCESS;
	}

	// An error, sorry
	return ERROR_CAN_NOT_COMPLETE;
}

static DWORD Rol32(DWORD dwValue, DWORD dwRolCount)
{
	DWORD dwShiftRight = 32 - dwRolCount;

	return (dwValue << dwRolCount) | (dwValue >> dwShiftRight);
}

static BOOL VerifyDataBlockHash(void *pvDataBlock, DWORD cbDataBlock, LPBYTE expected_md5)
{
#ifdef FULL
	hash_state md5_state;
	BYTE md5_digest[MD5_DIGEST_SIZE];

	// Don't verify the block if the MD5 is not valid.
	if (!IsValidMD5(expected_md5)) {
		return TRUE;
	}

	// Calculate the MD5 of the data block
	md5_init(&md5_state);
	md5_process(&md5_state, (unsigned char *)pvDataBlock, cbDataBlock);
	md5_done(&md5_state, md5_digest);

	// Does the MD5's match?
	return (memcmp(md5_digest, expected_md5, MD5_DIGEST_SIZE) == 0);
#else
	assert(0);
	return FALSE;
#endif
}

// This function gets the right positions of the hash table and the block table.
static int VerifyMpqTablePositions(TMPQArchive * ha, ULONGLONG FileSize)
{
	TMPQHeader *pHeader = ha->pHeader;
	ULONGLONG ByteOffset;

	// Check the begin of HET table
	if (pHeader->HetTablePos64) {
		ByteOffset = ha->MpqPos + pHeader->HetTablePos64;

		if (ByteOffset > FileSize) {
			return ERROR_BAD_FORMAT;
		}
	}

	// Check the begin of BET table
	if (pHeader->BetTablePos64) {
		ByteOffset = ha->MpqPos + pHeader->BetTablePos64;

		if (ByteOffset > FileSize) {
			return ERROR_BAD_FORMAT;
		}
	}

	// Check the begin of hash table
	if (pHeader->wHashTablePosHi || pHeader->dwHashTablePos) {
		ByteOffset = FileOffsetFromMpqOffset(ha, MAKE_OFFSET64(pHeader->wHashTablePosHi, pHeader->dwHashTablePos));

		if (ByteOffset > FileSize) {
			return ERROR_BAD_FORMAT;
		}
	}

	// Check the begin of block table
	if (pHeader->wBlockTablePosHi || pHeader->dwBlockTablePos) {
		ByteOffset = FileOffsetFromMpqOffset(ha, MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos));

		if (ByteOffset > FileSize) {
			return ERROR_BAD_FORMAT;
		}
	}

	// Check the begin of hi-block table
	if (pHeader->HiBlockTablePos64 != 0) {
		ByteOffset = ha->MpqPos + pHeader->HiBlockTablePos64;

		if (ByteOffset > FileSize) {
			return ERROR_BAD_FORMAT;
		}
	}

	// All OK.
	return ERROR_SUCCESS;
}

/*****************************************************************************/
/* Public functions                                                          */
/*****************************************************************************/

BOOL STORMAPI SFileCloseArchive(HANDLE hMpq)
{
	TMPQArchive * ha = IsValidMpqHandle(hMpq);
	BOOL bResult = TRUE;

	// Only if the handle is valid
	if (ha == NULL) {
		SErrSetLastError(ERROR_INVALID_HANDLE);

		return FALSE;
	}

	// Invalidate the add file callback so it won't be called
	// when saving (listfile) and (attributes)
	ha->pfnAddFileCB = NULL;
	ha->pvAddFileUserData = NULL;

	// Free all memory used by MPQ archive
	FreeArchiveHandle(ha);

	return bResult;
}

BOOL STORMAPI SFileCloseFile(HANDLE hFile)
{
	TMPQFile *hf = (TMPQFile *)hFile;    

	if (!IsValidFileHandle(hFile)) {
		SErrSetLastError(ERROR_INVALID_HANDLE);

		return FALSE;
	}

	// Free the structure
	FreeFileHandle(hf);

	return TRUE;
}

#if 0
BOOL STORMAPI SFileDdaBegin(HANDLE hFile, DWORD flags, DWORD mask)
{
	return SFileDdaBeginEx(hFile, flags, mask, 0, INT_MAX, INT_MAX, 0);
}

BOOL STORMAPI SFileDdaBeginEx(HANDLE hFile, DWORD flags, DWORD mask, unsigned __int32 lDistanceToMove, signed __int32 volume, signed int pan, int a7)
{
	DWORD chunkType;
	DWORD chunkLength;
	BOOL found_FMT = FALSE;
	BOOL found_DATA = FALSE;
	WAVEFORMATEX wf;
	DWORD bytesRead;
	VOID *audioBuffer;
	DWORD audioBufferSize;

	/* WAV magic header */
	DWORD RIFFMagic;
	DWORD wavelen;
	DWORD WAVEmagic;

	SFileReadFile(hFile, &RIFFMagic, sizeof(DWORD), &bytesRead, NULL);
	SFileReadFile(hFile, &wavelen, sizeof(DWORD), &bytesRead, NULL);
	SFileReadFile(hFile, &WAVEmagic, sizeof(DWORD), &bytesRead, NULL);

	/* Read the chunks */
	for (; ;) {
		SFileReadFile(hFile, &chunkType, sizeof(DWORD), &bytesRead, NULL);
		SFileReadFile(hFile, &chunkLength, sizeof(DWORD), &bytesRead, NULL);

		if (chunkLength == 0) {
			break;
		}

		switch (chunkType) {
			case MAKEFOURCC('f', 'm', 't', ' '):
				if (chunkLength < 16) {
					// Wave format chunk too small
				} else {
					byte *buffer = (BYTE *)malloc(chunkLength);
					WORD nChannels;
					DWORD nSamplesPerSec;
					WORD wBitsPerSample;

					SFileReadFile(hFile, buffer, chunkLength, &bytesRead, NULL);

					nChannels = *(WORD *)&buffer[2];
					nSamplesPerSec = *(DWORD *)&buffer[4];
					wBitsPerSample = *(WORD *)&buffer[14];

					XAudioCreatePcmFormat(nChannels, nSamplesPerSec, wBitsPerSample, &wf);

					free(buffer);

					found_FMT = TRUE;
				}
				break;
			case MAKEFOURCC('d', 'a', 't', 'a'):
				audioBuffer = malloc(chunkLength);
				audioBufferSize = chunkLength;

				SFileReadFile(hFile, audioBuffer, chunkLength, &bytesRead, NULL);

				found_DATA = TRUE;
				break;
			case MAKEFOURCC('S', 'M', 'P', 'L'):
			default:
				SFileSetFilePointer(hFile, chunkLength, NULL, SEEK_CUR);
				break;
		}
	}

	if (found_FMT && found_DATA) {
		DSBUFFERDESC dsbd;
		ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwBufferBytes = 0;

		sound->CreateSoundBuffer(&dsbd, &soundBuffer, NULL);

		soundBuffer->SetBufferData(audioBuffer, audioBufferSize);

		return TRUE;
	}

	/*if (!SFileReadFile(hFile, &fileHeader, sizeof(WAVHEADER), &bytesRead, NULL) || bytesRead != sizeof(WAVHEADER)) {
		// TODO: maybe look for a better error code
		SErrSetLastError(ERROR_FILE_CORRUPT);
		return FALSE;
	}

	if (*(&fileHeader->chunkId) != FOURCC_RIFF) {
		// TODO: maybe look for a better error code
		SErrSetLastError(ERROR_FILE_CORRUPT);
		return FALSE;
	}
	
	if (*(&fileHeader->format) != FOURCC_WAVE) {
		// TODO: maybe look for a better error code
		SErrSetLastError(ERROR_FILE_CORRUPT);
		return FALSE;
	}*/

	return FALSE;
}

BOOL STORMAPI SFileDdaDestroy()
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SFileDdaEnd(HANDLE hFile)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SFileDdaGetPos(HANDLE hFile, DWORD *current, DWORD *end)
{
	if (current != NULL) {
		*current = 0;
	}

	if (end != NULL) {
		*end = 0;
	}

	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SFileDdaInitialize(HANDLE directsound)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SFileDdaSetVolume(HANDLE hFile, signed int bigvolume, signed int volume)
{
	// TODO: implement

	return TRUE;
}
#endif

BOOL STORMAPI SFileDestroy()
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SFileGetFileArchive(HANDLE hFile, HANDLE *archive)
{
	// TODO: implement

	return TRUE;
}

LONG STORMAPI SFileGetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
	ULONGLONG FileSize;
	TMPQFile * hf = (TMPQFile *)hFile;

	// Validate the file handle before we go on
	if (IsValidFileHandle(hFile)) {
		// Make sure that the variable is initialized
		FileSize = 0;

		// If the file is patched file, we have to get the size of the last version
		if (hf->hfPatch != NULL) {
			// Walk through the entire patch chain, take the last version
			while (hf != NULL) {
				// Get the size of the currently pointed version
				FileSize = hf->pFileEntry->dwFileSize;

				// Move to the next patch file in the hierarchy
				hf = hf->hfPatch;
			}
		} else {
			// Is it a local file ?
			if (hf->pStream != NULL) {
				FileStream_GetSize(hf->pStream, &FileSize);
			} else {
				FileSize = hf->dwDataSize;
			}
		}

		// If opened from archive, return file size
		if (lpFileSizeHigh != NULL) {
			*lpFileSizeHigh = (DWORD)(FileSize >> 32);
		}

		return (DWORD)FileSize;
	}

	SErrSetLastError(ERROR_INVALID_HANDLE);

	return SFILE_INVALID_SIZE;
}

static DWORD HashStringSlash(const char * szFileName, DWORD dwHashType)
{
	LPBYTE pbKey   = (BYTE *)szFileName;
	DWORD  dwSeed1 = 0x7FED7FED;
	DWORD  dwSeed2 = 0xEEEEEEEE;
	DWORD  ch;

	while (*pbKey != 0) {
		// Convert the input character to uppercase
		// DON'T convert slash (0x2F) to backslash (0x5C)
		ch = AsciiToUpperTable_Slash[*pbKey++];

		dwSeed1 = StormBuffer[dwHashType + ch] ^ (dwSeed1 + dwSeed2);
		dwSeed2 = ch + dwSeed1 + dwSeed2 + (dwSeed2 << 5) + 3;
	}

	return dwSeed1;
}

static bool  bMpqCryptographyInitialized = false;

static void InitializeMpqCryptography()
{
	DWORD dwSeed = 0x00100001;
	DWORD index1 = 0;
	DWORD index2 = 0;
	int   i;

	// Initialize the decryption buffer.
	// Do nothing if already done.
	if (!bMpqCryptographyInitialized) {
		for (index1 = 0; index1 < 0x100; index1++) {
			for (index2 = index1, i = 0; i < 5; i++, index2 += 0x100) {
				DWORD temp1, temp2;

				dwSeed = (dwSeed * 125 + 3) % 0x2AAAAB;
				temp1  = (dwSeed & 0xFFFF) << 0x10;

				dwSeed = (dwSeed * 125 + 3) % 0x2AAAAB;
				temp2  = (dwSeed & 0xFFFF);

				StormBuffer[index2] = (temp1 | temp2);
			}
		}

#ifdef FULL
		// Also register both MD5 and SHA1 hash algorithms
		register_hash(&md5_desc);
		register_hash(&sha1_desc);

		// Use LibTomMath as support math library for LibTomCrypt
		ltc_mp = ltm_desc;
#endif
		// Don't do that again
		bMpqCryptographyInitialized = true;
	}
}

static BOOL PartStream_BlockCheck(
	TBlockStream *pStream,                 // Pointer to an open stream
	ULONGLONG BlockOffset)
{
	PPART_FILE_MAP_ENTRY FileBitmap;

	// Sanity checks
	assert((BlockOffset & (pStream->BlockSize - 1)) == 0);
	assert(pStream->FileBitmap != NULL);
	
	// Calculate the block map entry
	FileBitmap = (PPART_FILE_MAP_ENTRY)pStream->FileBitmap + (BlockOffset / pStream->BlockSize);

	// Check if the flags are present
	return (FileBitmap->Flags & 0x03) ? TRUE : FALSE;
}

static BOOL PartStream_BlockRead(TBlockStream *pStream, ULONGLONG StartOffset, ULONGLONG EndOffset, LPBYTE BlockBuffer, DWORD BytesNeeded, BOOL bAvailable)
{
	PPART_FILE_MAP_ENTRY FileBitmap;
	ULONGLONG ByteOffset;
	DWORD BytesToRead;
	DWORD BlockIndex = (DWORD)(StartOffset / pStream->BlockSize);

	// The starting offset must be aligned to size of the block
	assert(pStream->FileBitmap != NULL);
	assert((StartOffset & (pStream->BlockSize - 1)) == 0);
	assert(StartOffset < EndOffset);

	// If the blocks are not available, we need to load them from the master
	// and then save to the mirror
	if (bAvailable == FALSE) {
		// If we have no master, we cannot satisfy read request
		if (pStream->pMaster == NULL) {
			return FALSE;
		}

		// Load the blocks from the master stream
		// Note that we always have to read complete blocks
		// so they get properly stored to the mirror stream
		BytesToRead = (DWORD)(EndOffset - StartOffset);

		if (!FileStream_Read(pStream->pMaster, &StartOffset, BlockBuffer, BytesToRead)) {
			return FALSE;
		}

		// The loaded blocks are going to be stored to the end of the file
		// Note that this operation is not required to succeed
		if (pStream->BaseGetSize(pStream, &ByteOffset)) {
			// Store the loaded blocks to the mirror file.
			if (pStream->BaseWrite(pStream, &ByteOffset, BlockBuffer, BytesToRead)) {
				PartStream_UpdateBitmap(pStream, StartOffset, EndOffset, ByteOffset);
			}
		}
	} else {
		// Get the file map entry
		FileBitmap = (PPART_FILE_MAP_ENTRY)pStream->FileBitmap + BlockIndex;

		// Read all blocks
		while (StartOffset < EndOffset) {
			// Get the number of bytes to be read
			BytesToRead = (DWORD)(EndOffset - StartOffset);
			if (BytesToRead > pStream->BlockSize) {
				BytesToRead = pStream->BlockSize;
			}

			if (BytesToRead > BytesNeeded) {
				BytesToRead = BytesNeeded;
			}

			// Read the block
			ByteOffset = MAKE_OFFSET64(FileBitmap->BlockOffsHi, FileBitmap->BlockOffsLo);

			if (!pStream->BaseRead(pStream, &ByteOffset, BlockBuffer, BytesToRead)) {
				return FALSE;
			}

			// Move the pointers
			StartOffset += pStream->BlockSize;
			BlockBuffer += pStream->BlockSize;
			BytesNeeded -= pStream->BlockSize;
			FileBitmap++;
		}
	}

	return TRUE;
}

static DWORD PartStream_CheckFile(TBlockStream *pStream)
{
	PPART_FILE_MAP_ENTRY FileBitmap = (PPART_FILE_MAP_ENTRY)pStream->FileBitmap;
	DWORD dwBlockCount;

	// Get the number of blocks
	dwBlockCount = (DWORD)((pStream->StreamSize + pStream->BlockSize - 1) / pStream->BlockSize);

	// Check all blocks
	for (DWORD i = 0; i < dwBlockCount; i++, FileBitmap++) {
		// Few sanity checks
		assert(FileBitmap->LargeValueHi == 0);
		assert(FileBitmap->LargeValueLo == 0);
		assert(FileBitmap->Flags == 0 || FileBitmap->Flags == 3);

		// Check if this block is present
		if (FileBitmap->Flags != 3) {
			return 0;
		}
	}

	// Yes, the file is complete
	return 1;
}

static void PartStream_Close(TBlockStream * pStream)
{
	PART_FILE_HEADER PartHeader;
	ULONGLONG ByteOffset = 0;

	if(pStream->FileBitmap && pStream->IsModified)
	{
		// Prepare the part file header
		memset(&PartHeader, 0, sizeof(PART_FILE_HEADER));
		PartHeader.PartialVersion = 2;
		PartHeader.FileSizeHi     = (DWORD)(pStream->StreamSize >> 0x20);
		PartHeader.FileSizeLo     = (DWORD)(pStream->StreamSize & 0xFFFFFFFF);
		PartHeader.BlockSize      = pStream->BlockSize;
		
		// Make sure that the header is properly BSWAPed
		sprintf(PartHeader.GameBuildNumber, "%u", (unsigned int)pStream->BuildNumber);

		// Write the part header
		pStream->BaseWrite(pStream, &ByteOffset, &PartHeader, sizeof(PART_FILE_HEADER));

		// Write the block bitmap
		pStream->BaseWrite(pStream, NULL, pStream->FileBitmap, pStream->BitmapSize);
	}

	// Close the base class
	BlockStream_Close(pStream);
}

static BOOL PartStream_CreateMirror(TBlockStream *pStream)
{
	ULONGLONG RemainingSize;
	ULONGLONG MasterSize = 0;
	ULONGLONG MirrorSize = 0;
	LPBYTE FileBitmap = NULL;
	DWORD dwBitmapSize;
	DWORD dwBlockCount;
	BOOL bNeedCreateMirrorStream = TRUE;
	BOOL bNeedResizeMirrorStream = TRUE;

	// Do we have master function and base creation function?
	if (pStream->pMaster == NULL || pStream->BaseCreate == NULL) {
		return FALSE;
	}

	// Retrieve the master file size, block count and bitmap size
	FileStream_GetSize(pStream->pMaster, &MasterSize);
	dwBlockCount = (DWORD)((MasterSize + DEFAULT_BLOCK_SIZE - 1) / DEFAULT_BLOCK_SIZE);
	dwBitmapSize = (DWORD)(dwBlockCount * sizeof(PART_FILE_MAP_ENTRY));

	// Setup stream size and position
	pStream->BuildNumber = DEFAULT_BUILD_NUMBER;        // BUGBUG: Really???
	pStream->StreamSize = MasterSize;
	pStream->StreamPos = 0;

	// Open the base stream for write access
	if (pStream->BaseOpen(pStream, pStream->szFileName, 0)) {
		// If the file open succeeded, check if the file size matches required size
		pStream->BaseGetSize(pStream, &MirrorSize);
		if (MirrorSize >= sizeof(PART_FILE_HEADER) + dwBitmapSize) {
			// Check if the remaining size is aligned to block
			RemainingSize = MirrorSize - sizeof(PART_FILE_HEADER) - dwBitmapSize;

			if ((RemainingSize & (DEFAULT_BLOCK_SIZE - 1)) == 0 || RemainingSize == MasterSize) {
				// Attempt to load an existing file bitmap
				if (PartStream_LoadBitmap(pStream)) {
					return TRUE;
				}
			}
		}

		// We need to create mirror stream
		bNeedCreateMirrorStream = FALSE;
	}

	// Create a new stream, if needed
	if (bNeedCreateMirrorStream) {
		if (!pStream->BaseCreate(pStream)) {
			return FALSE;
		}
	}

	// If we need to, then resize the mirror stream
	if (bNeedResizeMirrorStream) {
		if (!pStream->BaseResize(pStream, sizeof(PART_FILE_HEADER) + dwBitmapSize)) {
			return FALSE;
		}
	}

	// Allocate the bitmap array
	FileBitmap = STORM_ALLOC(BYTE, dwBitmapSize);

	if (FileBitmap == NULL) {
		return FALSE;
	}

	// Initialize the bitmap
	memset(FileBitmap, 0, dwBitmapSize);
	pStream->FileBitmap = FileBitmap;
	pStream->BitmapSize = dwBitmapSize;
	pStream->BlockSize  = DEFAULT_BLOCK_SIZE;
	pStream->BlockCount = dwBlockCount;
	pStream->IsComplete = 0;
	pStream->IsModified = 1;

	// Note: Don't write the stream bitmap right away.
	// Doing so would cause sparse file resize on NTFS,
	// which would take long time on larger files.
	return TRUE;
}

static BOOL PartStream_LoadBitmap(TBlockStream *pStream)
{
	PPART_FILE_MAP_ENTRY FileBitmap;
	PART_FILE_HEADER PartHdr;
	ULONGLONG ByteOffset = 0;
	ULONGLONG StreamSize = 0;
	DWORD BlockCount;
	DWORD BitmapSize;

	// Only if the size is greater than size of the bitmap header
	if (pStream->Base.File.FileSize > sizeof(PART_FILE_HEADER)) {
		// Attempt to read PART file header
		if (pStream->BaseRead(pStream, &ByteOffset, &PartHdr, sizeof(PART_FILE_HEADER))) {
			// Verify the PART file header
			if (IsPartHeader(&PartHdr)) {
				// Get the number of blocks and size of one block
				StreamSize = MAKE_OFFSET64(PartHdr.FileSizeHi, PartHdr.FileSizeLo);
				ByteOffset = sizeof(PART_FILE_HEADER);
				BlockCount = (DWORD)((StreamSize + PartHdr.BlockSize - 1) / PartHdr.BlockSize);
				BitmapSize = BlockCount * sizeof(PART_FILE_MAP_ENTRY);

				// Check if sizes match
				if ((ByteOffset + BitmapSize) < pStream->Base.File.FileSize) {
					// Allocate space for the array of PART_FILE_MAP_ENTRY
					FileBitmap = STORM_ALLOC(PART_FILE_MAP_ENTRY, BlockCount);

					if (FileBitmap != NULL) {
						// Load the block map
						if (!pStream->BaseRead(pStream, &ByteOffset, FileBitmap, BitmapSize)) {
							STORM_FREE(FileBitmap);
							return FALSE;
						}

						// Update the stream size
						pStream->BuildNumber = StringToInt(PartHdr.GameBuildNumber);
						pStream->StreamSize = StreamSize;

						// Fill the bitmap information
						pStream->FileBitmap = FileBitmap;
						pStream->BitmapSize = BitmapSize;
						pStream->BlockSize  = PartHdr.BlockSize;
						pStream->BlockCount = BlockCount;
						pStream->IsComplete = PartStream_CheckFile(pStream);
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

static TFileStream *PartStream_Open(const TCHAR *szFileName, DWORD dwStreamFlags)
{
	TBlockStream *pStream;

	// Create new empty stream
	pStream = (TBlockStream *)AllocateFileStream(szFileName, sizeof(TBlockStream), dwStreamFlags);

	if (pStream == NULL) {
		return NULL;
	}

	// Do we have a master stream?
	if (pStream->pMaster != NULL) {
		if (!PartStream_CreateMirror(pStream)) {
			FileStream_Close(pStream);
			SErrSetLastError(ERROR_FILE_NOT_FOUND);
			return NULL;
		}
	} else {
		// Attempt to open the base stream
		if (!pStream->BaseOpen(pStream, pStream->szFileName, dwStreamFlags)) {
			FileStream_Close(pStream);
			return NULL;
		}

		// Load the part stream block map
		if (!PartStream_LoadBitmap(pStream)) {
			FileStream_Close(pStream);
			SErrSetLastError(ERROR_BAD_FORMAT);
			return NULL;
		}
	}

	// Set the stream position to zero. Stream size is already set
	assert(pStream->StreamSize != 0);
	pStream->StreamPos = 0;
	pStream->dwFlags |= STREAM_FLAG_READ_ONLY;

	// Set new function pointers
	pStream->StreamRead    = (STREAM_READ)BlockStream_Read;
	pStream->StreamGetPos  = BlockStream_GetPos;
	pStream->StreamGetSize = BlockStream_GetSize;
	pStream->StreamClose   = (STREAM_CLOSE)PartStream_Close;

	// Supply the block functions
	pStream->BlockCheck    = (BLOCK_CHECK)PartStream_BlockCheck;
	pStream->BlockRead     = (BLOCK_READ)PartStream_BlockRead;
	return pStream;
}

static void PartStream_UpdateBitmap(
	TBlockStream *pStream,                 // Pointer to an open stream
	ULONGLONG StartOffset,
	ULONGLONG EndOffset,
	ULONGLONG RealOffset)
{
	PPART_FILE_MAP_ENTRY FileBitmap;
	DWORD BlockSize = pStream->BlockSize;

	// Sanity checks
	assert((StartOffset & (BlockSize - 1)) == 0);
	assert(pStream->FileBitmap != NULL);

	// Calculate the first entry in the block map
	FileBitmap = (PPART_FILE_MAP_ENTRY)pStream->FileBitmap + (StartOffset / BlockSize);

	// Set all bits for the specified range
	while (StartOffset < EndOffset) {
		// Set the bit
		FileBitmap->BlockOffsHi = (DWORD)(RealOffset >> 0x20);
		FileBitmap->BlockOffsLo = (DWORD)(RealOffset & 0xFFFFFFFF);
		FileBitmap->Flags = 3;

		// Move all
		StartOffset += BlockSize;
		RealOffset += BlockSize;
		FileBitmap++;
	}

	// Increment the bitmap update count
	pStream->IsModified = 1;
}

BOOL STORMAPI SFileOpenArchive(const char *szMpqName, DWORD dwPriority, DWORD dwFlags, HANDLE *phMpq)
{
	TMPQUserData *pUserData;
	TFileStream *pStream = NULL;       // Open file stream
	TMPQArchive *ha = NULL;            // Archive handle
	TFileEntry *pFileEntry;
	ULONGLONG FileSize = 0;            // Size of the file
	LPBYTE pbHeaderBuffer = NULL;      // Buffer for searching MPQ header
	DWORD dwStreamFlags = (dwFlags & STREAM_FLAGS_MASK);
	BOOL bIsWarcraft3Map = FALSE;
	int nError = ERROR_SUCCESS;

	// Verify the parameters
	if (szMpqName == NULL || *szMpqName == 0 || phMpq == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	// One time initialization of MPQ cryptography
	InitializeMpqCryptography();
	//dwPriority = dwPriority;

	// If not forcing MPQ v 1.0, also use file bitmap
	dwStreamFlags |= (dwFlags & MPQ_OPEN_FORCE_MPQ_V1) ? 0 : STREAM_FLAG_USE_BITMAP;

	// Open the MPQ archive file
	pStream = FileStream_OpenFile(szMpqName, dwStreamFlags);

	if (pStream == NULL) {
		return FALSE;
	}

	// Check the file size. There must be at least 0x20 bytes
	if (nError == ERROR_SUCCESS) {
		FileStream_GetSize(pStream, &FileSize);

		if (FileSize < MPQ_HEADER_SIZE_V1) {
			nError = ERROR_BAD_FORMAT;
		}
	}

	// Allocate the MPQhandle
	if (nError == ERROR_SUCCESS) {
		if ((ha = STORM_ALLOC(TMPQArchive, 1)) == NULL) {
			nError = ERROR_NOT_ENOUGH_MEMORY;
		}
	}

	// Allocate buffer for searching MPQ header
	if (nError == ERROR_SUCCESS) {
		pbHeaderBuffer = STORM_ALLOC(BYTE, HEADER_SEARCH_BUFFER_SIZE);

		if (pbHeaderBuffer == NULL) {
			nError = ERROR_NOT_ENOUGH_MEMORY;
		}
	}

	// Find the position of MPQ header
	if(nError == ERROR_SUCCESS) {
		ULONGLONG SearchOffset = 0;
		ULONGLONG EndOfSearch = FileSize;
		DWORD dwStrmFlags = 0;
		DWORD dwHeaderSize;
		DWORD dwHeaderID;
		BOOL bSearchComplete = FALSE;

		memset(ha, 0, sizeof(TMPQArchive));
		ha->pfnHashString = HashStringSlash;
		ha->pStream = pStream;
		pStream = NULL;

		// Set the archive read only if the stream is read-only
		FileStream_GetFlags(ha->pStream, &dwStrmFlags);
		ha->dwFlags |= (dwStrmFlags & STREAM_FLAG_READ_ONLY) ? MPQ_FLAG_READ_ONLY : 0;

		// Also remember if we shall check sector CRCs when reading file
		ha->dwFlags |= (dwFlags & MPQ_OPEN_CHECK_SECTOR_CRC) ? MPQ_FLAG_CHECK_SECTOR_CRC : 0;

		// Also remember if this MPQ is a patch
		ha->dwFlags |= (dwFlags & MPQ_OPEN_PATCH) ? MPQ_FLAG_PATCH : 0;

		// Limit the header searching to about 130 MB of data
		if (EndOfSearch > 0x08000000) {
			EndOfSearch = 0x08000000;
		}

		// Find the offset of MPQ header within the file
		while (!bSearchComplete && SearchOffset < EndOfSearch) {
			// Always read at least 0x1000 bytes for performance.
			// This is what Storm.dll (2002) does.
			DWORD dwBytesAvailable = HEADER_SEARCH_BUFFER_SIZE;
			DWORD dwInBufferOffset = 0;

			// Cut the bytes available, if needed
			if ((FileSize - SearchOffset) < HEADER_SEARCH_BUFFER_SIZE) {
				dwBytesAvailable = (DWORD)(FileSize - SearchOffset);
			}

			// Read the eventual MPQ header
			if (!FileStream_Read(ha->pStream, &SearchOffset, pbHeaderBuffer, dwBytesAvailable)) {
				nError = SErrGetLastError();
				break;
			}

			// There are AVI files from Warcraft III with 'MPQ' extension.
			if (SearchOffset == 0) {
				if (IsAviFile((DWORD *)pbHeaderBuffer)) {
					nError = ERROR_AVI_FILE;
					break;
				}

				bIsWarcraft3Map = IsWarcraft3Map((DWORD *)pbHeaderBuffer);
			}

			// Search the header buffer
			while (dwInBufferOffset < dwBytesAvailable) {
				// Copy the data from the potential header buffer to the MPQ header
				memcpy(ha->HeaderData, pbHeaderBuffer + dwInBufferOffset, sizeof(ha->HeaderData));

				// If there is the MPQ user data, process it
				// Note that Warcraft III does not check for user data, which is abused by many map protectors
				dwHeaderID = ha->HeaderData[0];

				if (!bIsWarcraft3Map && (dwFlags & MPQ_OPEN_FORCE_MPQ_V1) == 0) {
					if (ha->pUserData == NULL && dwHeaderID == ID_MPQ_USERDATA) {
						// Verify if this looks like a valid user data
						pUserData = IsValidMpqUserData(SearchOffset, FileSize, ha->HeaderData);

						if (pUserData != NULL) {
							// Fill the user data header
							ha->UserDataPos = SearchOffset;
							ha->pUserData = &ha->UserData;
							memcpy(ha->pUserData, pUserData, sizeof(TMPQUserData));

							// Continue searching from that position
							SearchOffset += ha->pUserData->dwHeaderOffs;
							break;
						}
					}
				}

				// There must be MPQ header signature. Note that STORM.dll from Warcraft III actually
				// tests the MPQ header size. It must be at least 0x20 bytes in order to load it
				// Abused by Spazzler Map protector. Note that the size check is not present
				// in Storm.dll v 1.00, so Diablo I code would load the MPQ anyway.
				dwHeaderSize = ha->HeaderData[1];

				if (dwHeaderID == ID_MPQ && dwHeaderSize >= MPQ_HEADER_SIZE_V1) {
					// Now convert the header to version 4
					nError = ConvertMpqHeaderToFormat4(ha, SearchOffset, FileSize, dwFlags, bIsWarcraft3Map);
					bSearchComplete = TRUE;
					break;
				}

				// Check for MPK archives (Longwu Online - MPQ fork)
				if (!bIsWarcraft3Map && dwHeaderID == ID_MPK) {
					// Now convert the MPK header to MPQ Header version 4
					nError = ConvertMpkHeaderToFormat4(ha, FileSize, dwFlags);
					bSearchComplete = TRUE;
					break;
				}

				// If searching for the MPQ header is disabled, return an error
				if (dwFlags & MPQ_OPEN_NO_HEADER_SEARCH) {
					nError = ERROR_NOT_SUPPORTED;
					bSearchComplete = TRUE;
					break;
				}

				// Move the pointers
				SearchOffset += 0x200;
				dwInBufferOffset += 0x200;
			}
		}

		// Did we identify one of the supported headers?
		if (nError == ERROR_SUCCESS) {
			// Set the user data position to the MPQ header, if none
			if (ha->pUserData == NULL) {
				ha->UserDataPos = SearchOffset;
			}

			// Set the position of the MPQ header
			ha->pHeader  = (TMPQHeader *)ha->HeaderData;
			ha->MpqPos   = SearchOffset;
			ha->FileSize = FileSize;

			// Sector size must be nonzero.
			if(SearchOffset >= FileSize || ha->pHeader->wSectorSize == 0) {
				nError = ERROR_BAD_FORMAT;
			}
		}
	}

	// Fix table positions according to format
	if (nError == ERROR_SUCCESS) {
		// Dump the header
		//DumpMpqHeader(ha->pHeader);

		// W3x Map Protectors use the fact that War3's Storm.dll ignores the MPQ user data,
		// and ignores the MPQ format version as well. The trick is to
		// fake MPQ format 2, with an improper hi-word position of hash table and block table
		// We can overcome such protectors by forcing opening the archive as MPQ v 1.0
		if (dwFlags & MPQ_OPEN_FORCE_MPQ_V1) {
			ha->pHeader->wFormatVersion = MPQ_FORMAT_VERSION_1;
			ha->pHeader->dwHeaderSize = MPQ_HEADER_SIZE_V1;
			ha->dwFlags |= MPQ_FLAG_READ_ONLY;
			ha->pUserData = NULL;
		}

		// Anti-overflow. If the hash table size in the header is
		// higher than 0x10000000, it would overflow in 32-bit version
		// Observed in the malformed Warcraft III maps
		// Example map: MPQ_2016_v1_ProtectedMap_TableSizeOverflow.w3x
		ha->pHeader->dwBlockTableSize = (ha->pHeader->dwBlockTableSize & BLOCK_INDEX_MASK);
		ha->pHeader->dwHashTableSize = (ha->pHeader->dwHashTableSize & BLOCK_INDEX_MASK);

		// Both MPQ_OPEN_NO_LISTFILE or MPQ_OPEN_NO_ATTRIBUTES trigger read only mode
		if (dwFlags & (MPQ_OPEN_NO_LISTFILE | MPQ_OPEN_NO_ATTRIBUTES)) {
			ha->dwFlags |= MPQ_FLAG_READ_ONLY;
		}

		// Remember whether whis is a map for Warcraft III
		if (bIsWarcraft3Map) {
			ha->dwFlags |= MPQ_FLAG_WAR3_MAP;
		}

		// Set the size of file sector
		ha->dwSectorSize = (0x200 << ha->pHeader->wSectorSize);

		// Verify if any of the tables doesn't start beyond the end of the file
		nError = VerifyMpqTablePositions(ha, FileSize);
	}

	// Read the hash table. Ignore the result, as hash table is no longer required
	// Read HET table. Ignore the result, as HET table is no longer required
	if (nError == ERROR_SUCCESS) {
		nError = LoadAnyHashTable(ha);
	}

	// Now, build the file table. It will be built by combining
	// the block table, BET table, hi-block table, (attributes) and (listfile).
	if (nError == ERROR_SUCCESS) {
		nError = BuildFileTable(ha);
	}

#ifdef FULL
	// Load the internal listfile and include it to the file table
	if (nError == ERROR_SUCCESS && (dwFlags & MPQ_OPEN_NO_LISTFILE) == 0) {
		// Quick check for (listfile)
		pFileEntry = GetFileEntryLocale(ha, LISTFILE_NAME, LANG_NEUTRAL);

		if (pFileEntry != NULL) {
			// Ignore result of the operation. (listfile) is optional.
			SFileAddListFile((HANDLE)ha, NULL);
			ha->dwFileFlags1 = pFileEntry->dwFlags;
		}
	}

	// Load the "(attributes)" file and merge it to the file table
	if (nError == ERROR_SUCCESS && (dwFlags & MPQ_OPEN_NO_ATTRIBUTES) == 0 && (ha->dwFlags & MPQ_FLAG_BLOCK_TABLE_CUT) == 0) {
		// Quick check for (attributes)
		pFileEntry = GetFileEntryLocale(ha, ATTRIBUTES_NAME, LANG_NEUTRAL);

		if (pFileEntry != NULL) {
			// Ignore result of the operation. (attributes) is optional.
			SAttrLoadAttributes(ha);
			ha->dwFileFlags2 = pFileEntry->dwFlags;
		}
	}
#endif

	// Remember whether the archive has weak signature. Only for MPQs format 1.0.
	if (nError == ERROR_SUCCESS) {
		// Quick check for (signature)
		pFileEntry = GetFileEntryLocale(ha, SIGNATURE_NAME, LANG_NEUTRAL);

		if (pFileEntry != NULL) {
			// Just remember that the archive is weak-signed
			assert((pFileEntry->dwFlags & MPQ_FILE_EXISTS) != 0);
			ha->dwFileFlags3 = pFileEntry->dwFlags;
		}

		// Finally, set the MPQ_FLAG_READ_ONLY if the MPQ was found malformed
		ha->dwFlags |= (ha->dwFlags & MPQ_FLAG_MALFORMED) ? MPQ_FLAG_READ_ONLY : 0;
	}

	// Cleanup and exit
	if (nError != ERROR_SUCCESS) {
		FileStream_Close(pStream);
		FreeArchiveHandle(ha);
		SErrSetLastError(nError);
		ha = NULL;
	}

	// Free the header buffer
	if (pbHeaderBuffer != NULL) {
		STORM_FREE(pbHeaderBuffer);
	}

	if (phMpq != NULL) {
		*phMpq = ha;
	}

	archive = ha;

	return (nError == ERROR_SUCCESS);
}

BOOL STORMAPI SFileOpenFile(const char *filename, HANDLE *phFile)
{
	// TODO: Storm calls a subroutine to determine the searchScope
	// TODO: this returns values of either 0, 1, 2, or 3

	return SFileOpenFileEx(archive, filename, 0, phFile);

#if 0
	BOOL result = FALSE;

	if (directFileAccess) {
		char directPath[MAX_PATH] = "\0";

		for (int i = 0; i < strlen(filename); i++) {
			directPath[i] = AsciiToLowerTable_Path[filename[i]];
		}

		result = SFileOpenFileEx((HANDLE)0, directPath, 0xFFFFFFFF, phFile);
	}

	if (!result && patch_rt_mpq) {
		result = SFileOpenFileEx((HANDLE)patch_rt_mpq, filename, 0, phFile);
	}

	if (!result) {
		result = SFileOpenFileEx((HANDLE)diabdat_mpq, filename, 0, phFile);
	}

	if (!result || !*phFile) {
		DebugPrintf("%s: Not found: %s\n", __FUNCTION__, filename);
	}

	return result;
#endif
}

static DWORD FindHashIndex(TMPQArchive * ha, DWORD dwFileIndex)
{
	TMPQHash *pHashTableEnd;
	TMPQHash *pHash;
	DWORD dwFirstIndex = HASH_ENTRY_FREE;

	// Should only be called if the archive has hash table
	assert(ha->pHashTable != NULL);

	// Multiple hash table entries can point to the file table entry.
	// We need to search all of them
	pHashTableEnd = ha->pHashTable + ha->pHeader->dwHashTableSize;

	for (pHash = ha->pHashTable; pHash < pHashTableEnd; pHash++) {
		if (MPQ_BLOCK_INDEX(pHash) == dwFileIndex) {
			// Duplicate hash entry found
			if (dwFirstIndex != HASH_ENTRY_FREE) {
				return HASH_ENTRY_FREE;
			}

			dwFirstIndex = (DWORD)(pHash - ha->pHashTable);
		}
	}

	// Return the hash table entry index
	return dwFirstIndex;
}

static TFileEntry *GetFileEntryExact(TMPQArchive *ha, const char *szFileName, LCID lcLocale, LPDWORD PtrHashIndex)
{
	TMPQHash * pHash;
	DWORD dwFileIndex;

	// If the hash table is present, find the entry from hash table
	if (ha->pHashTable != NULL) {
		pHash = GetHashEntryExact(ha, szFileName, lcLocale);

		if (pHash != NULL && MPQ_BLOCK_INDEX(pHash) < ha->dwFileTableSize) {
			if (PtrHashIndex != NULL) {
				PtrHashIndex[0] = (DWORD)(pHash - ha->pHashTable);
			}

			return ha->pFileTable + MPQ_BLOCK_INDEX(pHash);
		}
	}

#ifdef FULL
	// If we have HET table in the MPQ, try to find the file in HET table
	if (ha->pHetTable != NULL) {
		dwFileIndex = GetFileIndex_Het(ha, szFileName);

		if (dwFileIndex != HASH_ENTRY_FREE) {
			if(PtrHashIndex != NULL) {
				PtrHashIndex[0] = HASH_ENTRY_FREE;
			}

			return ha->pFileTable + dwFileIndex;
		}
	}
#endif

	// Not found
	return NULL;
}

// Retrieves the first hash entry for the given file.
// Every locale version of a file has its own hash entry
static TMPQHash *GetFirstHashEntry(TMPQArchive *ha, const char *szFileName)
{
	DWORD dwHashIndexMask = HASH_INDEX_MASK(ha);
	DWORD dwStartIndex = ha->pfnHashString(szFileName, MPQ_HASH_TABLE_INDEX);
	DWORD dwName1 = ha->pfnHashString(szFileName, MPQ_HASH_NAME_A);
	DWORD dwName2 = ha->pfnHashString(szFileName, MPQ_HASH_NAME_B);
	DWORD dwIndex;

	// Set the initial index
	dwStartIndex = dwIndex = (dwStartIndex & dwHashIndexMask);

	// Search the hash table
	for (;;) {
		TMPQHash * pHash = ha->pHashTable + dwIndex;

		// If the entry matches, we found it.
		if (pHash->dwName1 == dwName1 && pHash->dwName2 == dwName2 && MPQ_BLOCK_INDEX(pHash) < ha->dwFileTableSize) {
			return pHash;
		}

		// If that hash entry is a free entry, it means we haven't found the file
		if (pHash->dwBlockIndex == HASH_ENTRY_FREE) {
			return NULL;
		}

		// Move to the next hash entry. Stop searching
		// if we got reached the original hash entry
		dwIndex = (dwIndex + 1) & dwHashIndexMask;

		if (dwIndex == dwStartIndex) {
			return NULL;
		}
	}
}

// Returns a hash table entry in the following order:
// 1) A hash table entry with the preferred locale
// 2) NULL
static TMPQHash *GetHashEntryExact(TMPQArchive *ha, const char *szFileName, LCID lcLocale)
{
	TMPQHash *pFirstHash = GetFirstHashEntry(ha, szFileName);
	TMPQHash *pHash = pFirstHash;

	// Parse the found hashes
	while (pHash != NULL) {
		// If the locales match, return it
		if (pHash->lcLocale == lcLocale) {
			return pHash;
		}

		// Get the next hash entry for that file
		pHash = GetNextHashEntry(ha, pFirstHash, pHash);
	}

	// Not found
	return NULL;
}

static TMPQHash *GetNextHashEntry(TMPQArchive *ha, TMPQHash *pFirstHash, TMPQHash *pHash)
{
	DWORD dwHashIndexMask = HASH_INDEX_MASK(ha);
	DWORD dwStartIndex = (DWORD)(pFirstHash - ha->pHashTable);
	DWORD dwName1 = pHash->dwName1;
	DWORD dwName2 = pHash->dwName2;
	DWORD dwIndex = (DWORD)(pHash - ha->pHashTable);

	// Now go for any next entry that follows the pHash,
	// until either free hash entry was found, or the start entry was reached
	for (;;) {
		// Move to the next hash entry. Stop searching
		// if we got reached the original hash entry
		dwIndex = (dwIndex + 1) & dwHashIndexMask;

		if (dwIndex == dwStartIndex) {
			return NULL;
		}

		pHash = ha->pHashTable + dwIndex;

		// If the entry matches, we found it.
		if (pHash->dwName1 == dwName1 && pHash->dwName2 == dwName2 && MPQ_BLOCK_INDEX(pHash) < ha->dwFileTableSize) {
			return pHash;
		}

		// If that hash entry is a free entry, it means we haven't found the file
		if (pHash->dwBlockIndex == HASH_ENTRY_FREE) {
			return NULL;
		}
	}
}

static const char * GetPatchFileName(TMPQArchive * ha, const char * szFileName, char * szBuffer)
{
	TMPQNamePrefix *pPrefix;

	// Are there patches in the current MPQ?
	if (ha->dwFlags & MPQ_FLAG_PATCH) {
		// The patch prefix must be already known here
		assert(ha->pPatchPrefix != NULL);
		pPrefix = ha->pPatchPrefix;

		// The patch name for "OldWorld\\XXX\\YYY" is "Base\\XXX\YYY"
		// We need to remove the "OldWorld\\" prefix
		if (!_strnicmp(szFileName, "OldWorld\\", 9)) {
			szFileName += 9;
		}

		// Create the file name from the known patch entry
		memcpy(szBuffer, pPrefix->szPatchPrefix, pPrefix->nLength);
		strcpy(szBuffer + pPrefix->nLength, szFileName);
		szFileName = szBuffer;
	}

	return szFileName;
}

static bool OpenLocalFile(const char * szFileName, HANDLE * PtrFile)
{
	TFileStream *pStream;
	TMPQFile *hf = NULL;

	// Open the file and create the TMPQFile structure
	pStream = FileStream_OpenFile(szFileName, STREAM_FLAG_READ_ONLY);

	if (pStream != NULL) {
		// Allocate and initialize file handle
		hf = CreateFileHandle(NULL, NULL);

		if (hf != NULL) {
			hf->pStream = pStream;
			*PtrFile = hf;
			return TRUE;
		} else {
			FileStream_Close(pStream);
			SErrSetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
	}

	*PtrFile = NULL;
	return FALSE;
}

static BOOL OpenPatchedFile(HANDLE hMpq, const char * szFileName, HANDLE * PtrFile)
{
	TMPQArchive *haBase = NULL;
	TMPQArchive *ha = (TMPQArchive *)hMpq;
	TFileEntry *pFileEntry;
	TMPQFile *hfPatch;                     // Pointer to patch file
	TMPQFile *hfBase = NULL;               // Pointer to base open file
	TMPQFile *hf = NULL;
	HANDLE hPatchFile;
	char szNameBuffer[MAX_PATH];

	// First of all, find the latest archive where the file is in base version
	// (i.e. where the original, unpatched version of the file exists)
	while (ha != NULL) {
		// If the file is there, then we remember the archive
		pFileEntry = GetFileEntryExact(ha, GetPatchFileName(ha, szFileName, szNameBuffer), 0, NULL);

		if (pFileEntry != NULL && (pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) == 0) {
			haBase = ha;
		}

		// Move to the patch archive
		ha = ha->haPatch;
	}

	// If we couldn't find the base file in any of the patches, it doesn't exist
	if ((ha = haBase) == NULL) {
		SErrSetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
	}

	// Now open the base file
	if (SFileOpenFileEx((HANDLE)ha, GetPatchFileName(ha, szFileName, szNameBuffer), SFILE_OPEN_BASE_FILE, (HANDLE *)&hfBase)) {
		// The file must be a base file, i.e. without MPQ_FILE_PATCH_FILE
		assert((hfBase->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) == 0);
		hf = hfBase;

		// Now open all patches and attach them on top of the base file
		for (ha = ha->haPatch; ha != NULL; ha = ha->haPatch) {
			// Prepare the file name with a correct prefix
			if (SFileOpenFileEx((HANDLE)ha, GetPatchFileName(ha, szFileName, szNameBuffer), SFILE_OPEN_BASE_FILE, &hPatchFile)) {
				// Remember the new version
				hfPatch = (TMPQFile *)hPatchFile;

				// We should not find patch file
				assert((hfPatch->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) != 0);

				// Attach the patch to the base file
				hf->hfPatch = hfPatch;
				hf = hfPatch;
			}
		}
	}

	// Give the updated base MPQ
	if (PtrFile != NULL) {
		*PtrFile = (HANDLE)hfBase;
	}

	return (hfBase != NULL);
}

static DWORD StringToInt(const char *szString)
{
	DWORD dwValue = 0;

	while ('0' <= szString[0] && szString[0] <= '9') {
		dwValue = (dwValue * 10) + (szString[0] - '9');
		szString++;
	}

	return dwValue;
}

BOOL STORMAPI SFileOpenFileEx(HANDLE hMpq, const char *szFileName, DWORD dwSearchScope, HANDLE *phFile)
{
	TMPQArchive * ha = IsValidMpqHandle(hMpq);
	TFileEntry  * pFileEntry = NULL;
	TMPQFile    * hf = NULL;
	DWORD dwHashIndex = HASH_ENTRY_FREE;
	DWORD dwFileIndex = 0;
	BOOL bOpenByIndex = FALSE;
	int nError = ERROR_SUCCESS;

	// Don't accept NULL pointer to file handle
	if (szFileName == NULL || *szFileName == 0) {
		nError = ERROR_INVALID_PARAMETER;
	}

	// When opening a file from MPQ, the handle must be valid
	if (dwSearchScope != SFILE_OPEN_LOCAL_FILE && ha == NULL) {
		nError = ERROR_INVALID_HANDLE;
	}

	// When not checking for existence, the pointer to file handle must be valid
	if (dwSearchScope != SFILE_OPEN_CHECK_EXISTS && phFile == NULL) {
		nError = ERROR_INVALID_PARAMETER;
	}

	// Prepare the file opening
	if (nError == ERROR_SUCCESS) {
		switch (dwSearchScope) {
			case SFILE_OPEN_FROM_MPQ:
			case SFILE_OPEN_BASE_FILE:
			case SFILE_OPEN_CHECK_EXISTS:                
				// If this MPQ has no patches, open the file from this MPQ directly
				if (ha->haPatch == NULL || dwSearchScope == SFILE_OPEN_BASE_FILE) {
					pFileEntry = GetFileEntryLocale2(ha, szFileName, lcFileLocale, &dwHashIndex);
				}

				// If this MPQ is a patched archive, open the file as patched
				else {
					return OpenPatchedFile(hMpq, szFileName, phFile);
				}
				break;

			case SFILE_OPEN_ANY_LOCALE:
				// This open option is reserved for opening MPQ internal listfile.
				// No argument validation. Tries to open file with neutral locale first,
				// then any other available.
				pFileEntry = GetFileEntryLocale2(ha, szFileName, 0, &dwHashIndex);
				break;

			case SFILE_OPEN_LOCAL_FILE:
				// Open a local file
				return OpenLocalFile(szFileName, phFile); 

			default:
				// Don't accept any other value
				nError = ERROR_INVALID_PARAMETER;
				break;
		}
	}

	// Check whether the file really exists in the MPQ
	if (nError == ERROR_SUCCESS) {
		if (pFileEntry == NULL || (pFileEntry->dwFlags & MPQ_FILE_EXISTS) == 0) {
			// Check the pseudo-file name
			if ((bOpenByIndex = IsPseudoFileName(szFileName, &dwFileIndex))) {
				// Get the file entry for the file
				if (dwFileIndex < ha->dwFileTableSize) {
					pFileEntry = ha->pFileTable + dwFileIndex;
				}
			}

			nError = ERROR_FILE_NOT_FOUND;
		}

		// Ignore unknown loading flags (example: MPQ_2016_v1_WME4_4.w3x)
		//if (pFileEntry != NULL && pFileEntry->dwFlags & ~MPQ_FILE_VALID_FLAGS) {
		//	nError = ERROR_NOT_SUPPORTED;
		//}
	}

	// Did the caller just wanted to know if the file exists?
	if (nError == ERROR_SUCCESS && dwSearchScope != SFILE_OPEN_CHECK_EXISTS) {
		// Allocate file handle
		hf = CreateFileHandle(ha, pFileEntry);

		if (hf != NULL) {
			// Get the hash index for the file
			if (ha->pHashTable != NULL && dwHashIndex == HASH_ENTRY_FREE) {
				dwHashIndex = FindHashIndex(ha, dwFileIndex);
			}

			if (dwHashIndex != HASH_ENTRY_FREE) {
				hf->pHashEntry = ha->pHashTable + dwHashIndex;
			}

			hf->dwHashIndex = dwHashIndex;

			// If the MPQ has sector CRC enabled, enable if for the file
			if (ha->dwFlags & MPQ_FLAG_CHECK_SECTOR_CRC) {
				hf->bCheckSectorCRCs = TRUE;
			}

			// If we know the real file name, copy it to the file entry
			if (!bOpenByIndex) {
				// If there is no file name yet, allocate it
				AllocateFileName(ha, pFileEntry, szFileName);

				// If the file is encrypted, we should detect the file key
				if (pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED) {
					hf->dwFileKey = DecryptFileKey(szFileName,
												   pFileEntry->ByteOffset,
												   pFileEntry->dwFileSize,
												   pFileEntry->dwFlags);
				}
			}
		} else {
			nError = ERROR_NOT_ENOUGH_MEMORY;
		}
	}

	// Give the file entry
	if (phFile != NULL) {
		phFile[0] = hf;
	}

	// Return error code
	if (nError != ERROR_SUCCESS) {
		SErrSetLastError(nError);
	}

	return (nError == ERROR_SUCCESS);
}

BOOL STORMAPI SFileReadFile(HANDLE hFile, void *buffer, DWORD nNumberOfBytesToRead, DWORD *read, LPOVERLAPPED lpOverlapped)
{
	TMPQFile *hf = (TMPQFile *)hFile;
	DWORD dwBytesRead = 0;                      // Number of bytes read
	int nError = ERROR_SUCCESS;

	// Always zero the result
	if (read != NULL) {
		*read = 0;
	}

	lpOverlapped = lpOverlapped;

	// Check valid parameters
	if (!IsValidFileHandle(hFile)) {
		SErrSetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (buffer == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// If we didn't load the patch info yet, do it now
	if (hf->pFileEntry != NULL && (hf->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) && hf->pPatchInfo == NULL) {
		nError = AllocatePatchInfo(hf, TRUE);

		if (nError != ERROR_SUCCESS || hf->pPatchInfo == NULL) {
			SErrSetLastError(nError);
			return FALSE;
		}
	}

	// Clear the last used compression
	hf->dwCompression0 = 0;

	// If the file is local file, read the data directly from the stream
	if (hf->pStream != NULL) {
		nError = ReadMpqFileLocalFile(hf, buffer, hf->dwFilePos, nNumberOfBytesToRead, &dwBytesRead);
	}
#ifdef FULL
	// If the file is a patch file, we have to read it special way
	else if (hf->hfPatch != NULL && (hf->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) == 0) {
		nError = ReadMpqFilePatchFile(hf, buffer, hf->dwFilePos, dwToRead, &dwBytesRead);
	}
#endif
	// If the archive is a MPK archive, we need special way to read the file
	else if (hf->ha->dwSubType == MPQ_SUBTYPE_MPK) {
		nError = ReadMpkFileSingleUnit(hf, buffer, hf->dwFilePos, nNumberOfBytesToRead, &dwBytesRead);
	}

	// If the file is single unit file, redirect it to read file
	else if (hf->pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT) {
		nError = ReadMpqFileSingleUnit(hf, buffer, hf->dwFilePos, nNumberOfBytesToRead, &dwBytesRead);
	}

	// Otherwise read it as sector based MPQ file
	else {
		nError = ReadMpqFileSectorFile(hf, buffer, hf->dwFilePos, nNumberOfBytesToRead, &dwBytesRead);
	}

	// Increment the file position
	hf->dwFilePos += dwBytesRead;

	// Give the caller the number of bytes read
	if (read != NULL) {
		*read = dwBytesRead;
	}

	// If the read operation succeeded, but not full number of bytes was read,
	// set the last error to ERROR_HANDLE_EOF
	if (nError == ERROR_SUCCESS && (dwBytesRead < nNumberOfBytesToRead)) {
		nError = ERROR_HANDLE_EOF;
	}

	// If something failed, set the last error value
	if (nError != ERROR_SUCCESS) {
		SErrSetLastError(nError);
	}

	return (nError == ERROR_SUCCESS);
}

BOOL STORMAPI SFileGetArchiveName(HANDLE hArchive, char *name, int length)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SFileLoadFile(char *filename, void *buffer, int buffersize, int a4, int a5)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SFileLoadFileEx(void *hArchive, char *filename, int a3, int a4, int a5, DWORD searchScope, struct _OVERLAPPED *lpOverlapped)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SFileEnableDirectAccess(BOOL enable)
{
	directFileAccess = enable;

	return TRUE;
}

BOOL STORMAPI SFileGetFileName(HANDLE hFile, char *buffer, int length)
{
	// TODO: implement

	return TRUE;
}

BOOL STORMAPI SFileSetBasePath(const char * base_path)
{
	if (base_path == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	if (*base_path == '\0') {
		if (BasePath != NULL) {
			free(BasePath);
		}

		BasePath = NULL;
		return TRUE;
	}

	BasePath = strdup(base_path);

	// TODO: implement
	return TRUE;
}

DWORD STORMAPI SFileSetFilePointer(HANDLE hFile, LONG lFilePos, HANDLE plFilePosHigh, DWORD dwMoveMethod)
{
	TMPQFile *hf = (TMPQFile *)hFile;
	ULONGLONG OldPosition;
	ULONGLONG NewPosition;
	ULONGLONG FileSize;
	ULONGLONG DeltaPos;

	// If the hFile is not a valid file handle, return an error.
	if (!IsValidFileHandle(hFile)) {
		SErrSetLastError(ERROR_INVALID_HANDLE);

		return SFILE_INVALID_POS;
	}

	// Retrieve the file size for handling the limits
	if (hf->pStream != NULL) {
		FileStream_GetSize(hf->pStream, &FileSize);
	} else {
		FileSize = SFileGetFileSize(hFile, NULL);
	}

	// Handle the NULL and non-NULL values of plFilePosHigh
	// Non-NULL: The DeltaPos is combined from lFilePos and *lpFilePosHigh
	// NULL: The DeltaPos is sign-extended value of lFilePos
	DeltaPos = (plFilePosHigh != NULL) ? MAKE_OFFSET64(((LONG *)plFilePosHigh)[0], lFilePos) : (ULONGLONG)(LONGLONG)lFilePos;

	// Get the relative point where to move from
	switch (dwMoveMethod) {
		case FILE_BEGIN:
			// Move relative to the file begin.
			OldPosition = 0;
			break;

		case FILE_CURRENT:
			// Retrieve the current file position
			if (hf->pStream != NULL) {
				FileStream_GetPos(hf->pStream, &OldPosition);
			} else {
				OldPosition = hf->dwFilePos;
			}
			break;

		case FILE_END:
			// Move relative to the end of the file
			OldPosition = FileSize;
			break;

		default:
			SErrSetLastError(ERROR_INVALID_PARAMETER);
			return SFILE_INVALID_POS;
	}

	// Calculate the new position
	NewPosition = OldPosition + DeltaPos;

	// If moving backward, don't allow the new position go negative
	if ((LONGLONG)DeltaPos < 0) {
		if(NewPosition > FileSize) { // Position is negative
			SErrSetLastError(ERROR_NEGATIVE_SEEK);
			return SFILE_INVALID_POS;
		}
	}

	// If moving forward, don't allow the new position go past the end of the file
	else {
		if (NewPosition > FileSize) {
			NewPosition = FileSize;
		}
	}

	// Now apply the file pointer to the file
	if (hf->pStream != NULL) {
		if (!FileStream_Read(hf->pStream, &NewPosition, NULL, 0)) {
			return SFILE_INVALID_POS;
		}
	} else {
		hf->dwFilePos = (DWORD)NewPosition;
	}

	// Return the new file position
	if (plFilePosHigh != NULL) {
		*((LONG *)plFilePosHigh) = (LONG)(NewPosition >> 32);
	}

	return (DWORD)NewPosition;
}

BOOL STORMAPI SFileSetIoErrorMode(int mode, BOOL (STORMAPI *callback)(char*,int,int))
{
	// TODO: implement

	return TRUE;
}

void STORMAPI SFileSetLocale(LCID lcLocale)
{
	lcFileLocale = lcLocale;
}

BOOL STORMAPI SFileUnloadFile(HANDLE hFile)
{
	if (hFile == NULL) {
		SErrSetLastError(ERROR_INVALID_PARAMETER);

		return FALSE;
	}

	SMemFree(hFile, "P:\\Projects\\Storm\\Storm-SWAR\\Source\\SFile.cpp", 3216, 0);

	return TRUE;
}
