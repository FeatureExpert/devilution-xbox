#include "storm.h"

#define rBool { return TRUE; }
#define rPVoid { return NULL; }
#define rVoid { return; }
#define rInt { return 0; }

BOOL STORMAPI SNetCreateGame(const char *pszGameName, const char *pszGamePassword, const char *pszGameStatString, DWORD dwGameType, char *GameTemplateData, int GameTemplateSize, int playerCount, const char *creatorName, const char *a11, int *playerID) rBool;
BOOL STORMAPI SNetDestroy() rBool;

BOOL STORMAPI SNetDropPlayer(int playerid, DWORD flags) rBool;
BOOL STORMAPI SNetGetGameInfo(int type, void *dst, size_t length, size_t *byteswritten) rBool;

BOOL STORMAPI SNetGetNumPlayers(int *firstplayerid, int *lastplayerid, int *activeplayers) rBool;

BOOL STORMAPI SNetGetPlayerCaps(char playerid, PCAPS playerCaps) rBool;
BOOL STORMAPI SNetGetPlayerName(int playerid, char *buffer, size_t buffersize) rBool;
//BOOL STORMAPI SNetGetProviderCaps(PCAPS providerCaps) rBool;
BOOL STORMAPI SNetGetTurnsInTransit(int *turns) rBool;
BOOL STORMAPI SNetInitializeDevice(int a1, int a2, int a3, int a4, int *a5) rBool;
//BOOL STORMAPI SNetInitializeProvider(DWORD providerName, client_info *gameClientInfo, user_info *userData, battle_info *bnCallbacks, module_info *moduleData) rBool;
BOOL STORMAPI SNetJoinGame(int id, char *gameName, char *gamePassword, char *playerName, char *userStats, int *playerid) rBool;
BOOL STORMAPI SNetLeaveGame(int type) rBool;
BOOL STORMAPI SNetPerformUpgrade(DWORD *upgradestatus) rBool;
BOOL STORMAPI SNetReceiveMessage(int *senderplayerid, char **data, int *databytes) rBool;
BOOL STORMAPI SNetReceiveTurns(int a1, int arraysize, char **arraydata, DWORD *arraydatabytes, DWORD *arrayplayerstatus) rBool;
//HANDLE STORMAPI SNetRegisterEventHandler(int type, void (STORMAPI *sEvent)(PS_EVT)) rPVoid;

int STORMAPI SNetSelectGame(int a1, int a2, int a3, int a4, int a5, int *playerid) rInt;

BOOL STORMAPI SNetSendMessage(int playerID, void *data, size_t databytes) rBool;
BOOL STORMAPI SNetSendTurn(char *data, size_t databytes) rBool;

BOOL STORMAPI SNetSetGameMode(DWORD modeFlags, bool makePublic) rBool;

BOOL STORMAPI SNetEnumGamesEx(int a1, int a2, int (__fastcall *callback)(DWORD, DWORD, DWORD), int *hintnextcall) rBool;
BOOL STORMAPI SNetSendServerChatCommand(const char *command) rBool;

BOOL STORMAPI SNetDisconnectAll(DWORD flags) rBool;
BOOL STORMAPI SNetCreateLadderGame(const char *pszGameName, const char *pszGamePassword, const char *pszGameStatString, DWORD dwGameType, DWORD dwGameLadderType, DWORD dwGameModeFlags, char *GameTemplateData, int GameTemplateSize, int playerCount, char *creatorName, char *a11, int *playerID) rBool;
BOOL STORMAPI SNetReportGameResult(unsigned a1, int size, int *results, const char* headerInfo, const char* detailInfo) rBool;

int STORMAPI SNetSendLeagueCommand(char *cmd, char *callback) rInt;
int STORMAPI SNetSendReplayPath(int a1, int a2, char *replayPath) rInt;
int STORMAPI SNetGetLeagueName(int leagueID) rInt;
BOOL STORMAPI SNetGetPlayerNames(char **names) rBool;
int STORMAPI SNetLeagueLogout(char *bnetName) rInt;
int STORMAPI SNetGetLeaguePlayerName(char *curPlayerLeageName, size_t nameSize) rInt;


int STORMAPI Ordinal224(int a1) rInt;

BOOL STORMAPI SBltROP3(void *lpDstBuffer, void *lpSrcBuffer, int width, int height, int a5, int a6, int a7, DWORD rop) rBool;
BOOL STORMAPI SBltROP3Clipped(void *lpDstBuffer, RECT *lpDstRect, POINT *lpDstPt, int a4, void *lpSrcBuffer, RECT *lpSrcRect, POINT *lpSrcPt, int a8, int a9, DWORD rop) rBool;
BOOL STORMAPI SBltROP3Tiled(void *lpDstBuffer, RECT *lpDstRect, POINT *lpDstPt, int a4, void *lpSrcBuffer, RECT *lpSrcRect, POINT *lpSrcPt, int a8, int a9, DWORD rop) rBool;

BOOL STORMAPI SCodeCompile(char *directives1, char *directives2, char *loopstring, unsigned int maxiterations, unsigned int flags, HANDLE handle) rBool;
BOOL STORMAPI SCodeDelete(HANDLE handle) rBool;

int  STORMAPI SCodeExecute(HANDLE handle, int a2) rInt;



BOOL STORMAPI SEvtDispatch(DWORD dwMessageID, DWORD dwFlags, int type, PS_EVT pEvent) rBool;

BOOL STORMAPI SGdiDeleteObject(HANDLE handle) rBool;

BOOL STORMAPI SGdiExtTextOut(int a1, int a2, int a3, int a4, unsigned int a8, signed int a6, signed int a7, const char *string, unsigned int arg20) rBool;
BOOL STORMAPI SGdiImportFont(HGDIOBJ handle, int windowsfont) rBool;

BOOL STORMAPI SGdiSelectObject(int handle) rBool;
BOOL STORMAPI SGdiSetPitch(int pitch) rBool;

BOOL STORMAPI Ordinal393(char *string, int, int) rBool;

BOOL STORMAPI STransBlt(void *lpSurface, int x, int y, int width, HANDLE hTrans) rBool;
BOOL STORMAPI STransBltUsingMask(void *lpSurface, void *lpSource, int pitch, int width, HANDLE hTrans) rBool;

BOOL STORMAPI STransDelete(HANDLE hTrans) rBool;

BOOL STORMAPI STransDuplicate(HANDLE hTransSource, HANDLE hTransDest) rBool;
BOOL STORMAPI STransIntersectDirtyArray(HANDLE hTrans, char * dirtyarraymask, unsigned flags, HANDLE * phTransResult) rBool;
BOOL STORMAPI STransInvertMask(HANDLE hTrans, HANDLE * phTransResult) rBool;

BOOL STORMAPI STransSetDirtyArrayInfo(int width, int height, int depth, int bits) rBool;

BOOL STORMAPI STransPointInMask(HANDLE hTrans, int x, int y) rBool;
BOOL STORMAPI STransCombineMasks(HANDLE hTransA, HANDLE hTransB, int left, int top, int flags, HANDLE * phTransResult) rBool;

BOOL STORMAPI STransCreateE(void *pBuffer, int width, int height, int bpp, int a5, int bufferSize, HANDLE *phTransOut) rBool;
BOOL STORMAPI STransCreateI(void *pBuffer, int width, int height, int bpp, int a5, int bufferSize, HANDLE *phTransOut) rBool;

BOOL STORMAPI SErrDisplayError(DWORD dwErrMsg, const char *logfilename, int logline, const char *message, BOOL allowOption, int exitCode) rBool;
BOOL STORMAPI SErrGetErrorStr(DWORD dwErrCode, char *buffer, size_t bufferchars) rBool;
DWORD STORMAPI SErrGetLastError() rInt;

void STORMAPI SErrSetLastError(DWORD dwErrCode) rVoid;

void STORMAPI SErrSuppressErrors(BOOL suppressErrors) rVoid;

void STORMAPI SRgn523(HANDLE hRgn, RECT *pRect, int a3, int a4) rVoid;
void STORMAPI SRgnCreateRegion(HANDLE *hRgn, int a2) rVoid;
void STORMAPI SRgnDeleteRegion(HANDLE hRgn) rVoid;

void STORMAPI SRgn529i(int handle, int a2, int a3) rVoid;

BOOL SErrDisplayErrorFmt(DWORD dwErrMsg, const char *logfilename, int logline, BOOL allowOption, int exitCode, const char *format, ...) rBool;

void STORMAPI SErrCatchUnhandledExceptions() rVoid;

int STORMAPI SBigDel(void *buffer) rInt;

int STORMAPI SBigFromBinary(void *buffer, const void *str, size_t size) rInt;

int STORMAPI SBigNew(void **buffer) rInt;

int STORMAPI SBigPowMod(void *buffer1, void *buffer2, int a3, int a4) rInt;

int STORMAPI SBigToBinaryBuffer(void *buffer, int length, int a3, int a4) rInt;
//

BOOLEAN __cdecl StormDestroy(void) rBool;
BOOL __stdcall SNetGetOwnerTurnsWaiting(DWORD *) rBool;
BOOL __stdcall SNetUnregisterEventHandler(int,SEVTHANDLER) rPVoid;
BOOL __stdcall SNetRegisterEventHandler(int,SEVTHANDLER) rPVoid;
BOOLEAN __stdcall SNetSetBasePlayer(int) rBool;
int __stdcall SNetInitializeProvider(unsigned long,struct _SNETPROGRAMDATA *,struct _SNETPLAYERDATA *,struct _SNETUIDATA *,struct _SNETVERSIONDATA *) rInt;
int __stdcall SNetGetProviderCaps(struct _SNETCAPS *) rInt;
BOOL __stdcall SGdiTextOut(void *pBuffer, int x, int y, int mask, char *str, int len) rBool;
