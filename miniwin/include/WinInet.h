#if !defined(_WININET_)
#define _WININET_

/*
 * Set up Structure Packing to be 4 bytes
 * for all wininet structures
 */
#if defined(_WIN64)
#include <pshpack8.h>
#else
#include <pshpack4.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(_WINX32_)
#define INTERNETAPI        EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define INTERNETAPI_(type) EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#define URLCACHEAPI        EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define URLCACHEAPI_(type) EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#else
#define INTERNETAPI        EXTERN_C HRESULT STDAPICALLTYPE
#define INTERNETAPI_(type) EXTERN_C type STDAPICALLTYPE
#define URLCACHEAPI        EXTERN_C HRESULT STDAPICALLTYPE
#define URLCACHEAPI_(type) EXTERN_C type STDAPICALLTYPE
#endif

#define BOOLAPI INTERNETAPI_(BOOL)

//
// internet types
//

typedef LPVOID HINTERNET;
typedef HINTERNET * LPHINTERNET;

typedef WORD INTERNET_PORT;
typedef INTERNET_PORT * LPINTERNET_PORT;

//
// Internet APIs
//

#if defined(__cplusplus)
}
#endif

/*
 * Return packing to whatever it was before we
 * entered this file
 */
#include <poppack.h>

#endif // !defined(_WININET_)
