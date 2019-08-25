#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <share.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "res2bin.h"
#include "rc.h"
#include "getmsg.h"
#include "msg.h"

//
// Globals
//

char *szInFile;
char *szOutFile;
BOOL fVerbose;
BOOL fWritable = TRUE;

#if _DEBUG
BOOL fDebug;
#endif /* _DEBUG */


void usage(int rc)
{
	printf(get_err(MSG_VERSION), VER_PRODUCTVERSION_STR);
	puts(get_err(MSG_COPYRIGHT));

	puts("usage: RES2BIN [options] ResFile\n"
         "\n"
         "   options:\n"
         "\n"
#if _DEBUG
         "      /DEBUG\n"
#endif /* _DEBUG */
	     "      /NOLOGO\n"
	     "      /OUT:filename\n"
	     "      /VERBOSE\n");

	exit(rc);
}

void main(int argc, char *argv[])

/*++

Routine Description:

    Determines options
    locates and opens input files
    reads input files
    writes output files
    exits

Exit Value:

        0 on success
        1 if error

--*/

{
	int i;
	char szDrive[_MAX_DRIVE];
	char szDir[_MAX_DIR];
	char szFname[_MAX_FNAME];
	char szExt[_MAX_EXT];
	char szInPath[_MAX_PATH];
	char szOutPath[_MAX_PATH];
	FILE *fh;
	FILE *fhOut;
	char *s1;
	long lbytes;
	BOOL result;
	BOOL fNoLogo       = FALSE;
	BOOL fReproducible = FALSE;
	DWORD timeDate;

	SetErrorFile("res2bin.err", _pgmptr, 1);

	if (argc == 1) {
		usage(0);
	}

	for (i = 1; i < argc; i++) {
		s1 = argv[i];

		if (*s1 == '/' || *s1 == '-') {
			s1++;

			if (!_stricmp(s1, "nologo")) {
				fNoLogo = TRUE;
			} else if (!_stricmp(s1, "o")) {
				szOutFile = argv[++i];
			} else if (!_strnicmp(s1, "out:", 4)) {
				szOutFile = s1 + 4;
			} else if (!_stricmp(s1, "readonly") || !_stricmp(s1, "r")) {
				fWritable = FALSE;
			} else if (!_stricmp(s1, "verbose") || !_stricmp(s1, "v")) {
				fVerbose = TRUE;
			} else if (!_stricmp(s1, "Brepro")) {
				fReproducible = TRUE;
#if _DEBUG
			} else if (!_stricmp(s1, "debug") || !_stricmp(s1, "d")) {
				fDebug   = TRUE;
				fVerbose = TRUE;
#endif /* _DEBUG */
			} else {
				usage(1);
			}
		} else {
			szInFile = s1;
		}
	}

	//
	// Make sure that we actually got a file
	//

	if (!szInFile) {
		usage(1);
	}

	if (!fNoLogo) {
		printf(get_err(MSG_VERSION), VER_PRODUCTVERSION_STR);
		puts(get_err(MSG_COPYRIGHT));
	}

	_splitpath(szInFile, szDrive, szDir, szFname, szExt);

	if (szExt[0] == '\0') {
		// If there is no extension, default to .res

		_makepath(szInPath, szDrive, szDir, szFname, ".res");

		szInFile = szInPath;
	}

	if ((fh = _fsopen(szInFile, "rb", _SH_DENYWR)) == NULL) {
		ErrorPrint(ERR_CANTREADFILE, szInFile);
		exit(1);
	}

#if _DEBUG
	printf("Reading %s\n", szInFile);
#endif /* _DEBUG */

	lbytes = MySeek(fh, 0L, SEEK_END);
	MySeek(fh, 0L, SEEK_SET);

	if (szOutFile == NULL) {
		// Default output file is input file with .obj extension

		_makepath(szOutPath, szDrive, szDir, szFname, ".obj");

		szOutFile = szOutPath;
	}

	if ((fhOut = _fsopen(szOutFile, "wb", _SH_DENYRW)) == NULL) {
		ErrorPrint(ERR_CANTWRITEFILE, szOutFile);
		exit(1);
	}

#if _DEBUG
	printf("Writing %s\n", szOutFile);
#endif /* _DEBUG */

	_tzset();
	// timeDate = (ULONG) time(NULL);
	timeDate = (ULONG)(fReproducible ? -1 : time(NULL));

	result = Res2Bin(fh, fhOut, lbytes, fWritable, timeDate);

	fclose(fh);
	fclose(fhOut);

	exit(result ? 0 : 1);
}

PVOID
MyAlloc(UINT nbytes)
{
    UCHAR       *s;

    if ((s = (UCHAR*)calloc( 1, nbytes )) != NULL) {
        return s;
    }

    ErrorPrint(ERR_OUTOFMEMORY, nbytes );
    exit(1);
}


PVOID
MyFree( PVOID p )
{
    if (p)
        free( p );
    return NULL;
}


USHORT
MyReadWord(FILE *fh, USHORT*p)
{
    UINT      n1;

    if ((n1 = fread(p, 1, sizeof(USHORT), fh)) != sizeof(USHORT)) {
        ErrorPrint( ERR_FILEREADERR);
        exit(1);
    }
    else
        return 0;
}


UINT
MyRead(FILE *fh, PVOID p, UINT n )
{
    UINT      n1;

    if ((n1 = fread( p, 1, n, fh)) != n) {
        ErrorPrint( ERR_FILEREADERR );
        exit(1);
    }
    else
        return 0;
}

LONG
MyTell( FILE *fh )
{
    long pos;

    if ((pos = ftell( fh )) == -1) {
        ErrorPrint(ERR_FILETELLERR );
        exit(1);
    }

    return pos;
}


LONG
MySeek( FILE *fh, long pos, int cmd )
{
    if ((pos = fseek( fh, pos, cmd )) == -1) {
        ErrorPrint(ERR_FILESEEKERR );
        exit(1);
    }

    return MyTell(fh);
}


ULONG
MoveFilePos( FILE *fh, USHORT pos, int alignment )
{
    long newpos;

    newpos = (long)pos;
    newpos <<= alignment;
    return MySeek( fh, newpos, SEEK_SET );
}


UINT
MyWrite( FILE *fh, PVOID p, UINT n )
{
    ULONG       n1;

    if ((n1 = fwrite( p, 1, n, fh )) != n) {
        ErrorPrint(ERR_FILEWRITEERR );
        exit(1);
    }
    else
        return 0;
}

#undef BUFSIZE
#define BUFSIZE 1024

int
MyCopy( FILE *srcfh, FILE *dstfh, ULONG nbytes )
{
    static UCHAR buffer[ BUFSIZE ];
    UINT        n;
    ULONG       cb=0L;

    while (nbytes) {
        if (nbytes <= BUFSIZE)
            n = (UINT)nbytes;
        else
            n = BUFSIZE;
        nbytes -= n;

        if (!MyRead( srcfh, buffer, n )) {
            cb += n;
            MyWrite( dstfh, buffer, n );
        }
        else {
            return cb;
        }
    }
    return cb;
}


void
ErrorPrint(
    USHORT errnum,
    ...
    )
{
    va_list va;
    va_start(va, errnum);

    printf(get_err(MSG_ERROR), errnum, ' ');

    vprintf(get_err(errnum), va);
    printf("\n");

    va_end(va);
}

void
WarningPrint(
    USHORT errnum,
    ...
    )
{
    va_list va;
    va_start(va, errnum);

    printf(get_err(MSG_WARNING), errnum, ' ');

    vprintf(get_err(errnum), va);
    printf("\n");

    va_end(va);
}
