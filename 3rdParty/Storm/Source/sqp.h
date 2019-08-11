#ifndef _STORM_SQP_H
#define _STORM_SQP_H

/*****************************************************************************/
/*                                                                           */
/*         Support for SQP file format (War of the Immortals)                */
/*                                                                           */
/*****************************************************************************/

typedef struct _TSQPHeader
{
    // The ID_MPQ ('MPQ\x1A') signature
    DWORD dwID;                         

    // Size of the archive header
    DWORD dwHeaderSize;                   

    // 32-bit size of MPQ archive
    DWORD dwArchiveSize;

    // Offset to the beginning of the hash table, relative to the beginning of the archive.
    DWORD dwHashTablePos;
    
    // Offset to the beginning of the block table, relative to the beginning of the archive.
    DWORD dwBlockTablePos;
    
    // Number of entries in the hash table. Must be a power of two, and must be less than 2^16 for
    // the original MoPaQ format, or less than 2^20 for the Burning Crusade format.
    DWORD dwHashTableSize;
    
    // Number of entries in the block table
    DWORD dwBlockTableSize;

    // Must be zero for SQP files
    USHORT wFormatVersion;

    // Power of two exponent specifying the number of 512-byte disk sectors in each file sector
    // in the archive. The size of each file sector in the archive is 512 * 2 ^ wSectorSize.
    USHORT wSectorSize;

} TSQPHeader;

typedef struct _TSQPHash
{
    // Most likely the lcLocale+wPlatform.
    DWORD dwAlwaysZero;

    // If the hash table entry is valid, this is the index into the block table of the file.
    // Otherwise, one of the following two values:
    //  - FFFFFFFFh: Hash table entry is empty, and has always been empty.
    //               Terminates searches for a given file.
    //  - FFFFFFFEh: Hash table entry is empty, but was valid at some point (a deleted file).
    //               Does not terminate searches for a given file.
    DWORD dwBlockIndex;

    // The hash of the file path, using method A.
    DWORD dwName1;
    
    // The hash of the file path, using method B.
    DWORD dwName2;

} TSQPHash;

typedef struct _TSQPBlock
{
    // Offset of the beginning of the file, relative to the beginning of the archive.
    DWORD dwFilePos;
    
    // Flags for the file. See MPQ_FILE_XXXX constants
    DWORD dwFlags;                      

    // Compressed file size
    DWORD dwCSize;
    
    // Uncompressed file size
    DWORD dwFSize;                      
    
} TSQPBlock;

static TMPQBlock *LoadSqpBlockTable(TMPQArchive *ha);
static TMPQHash *LoadSqpHashTable(TMPQArchive *ha);
static void *LoadSqpTable(TMPQArchive *ha, DWORD dwByteOffset, DWORD cbTableSize, DWORD dwKey);

#endif /* _STORM_SQP_H */
