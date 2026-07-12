#pragma once

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#if defined(__ANDROID__) || defined(__linux__)
#include <arpa/inet.h>
#endif
#if defined(__ANDROID__)
#include <dirent.h>
#include <fnmatch.h>
#else
#include <glob.h>
#endif
#include <pthread.h>
#include <stdio.h>
#define random macos_random
#include <stdlib.h>
#undef random
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0
#endif

#if defined(__APPLE__) && defined(BIG_ENDIAN)
#undef BIG_ENDIAN
#endif

#ifndef WIN32
#define WIN32 1
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define WINAPI
#define APIENTRY
#define CALLBACK
#define PASCAL
#define FAR
#define NEAR
#define far
#define near
#define _export
#define __cdecl
#define __stdcall
#define __fastcall
#define cdecl

#ifndef _MSC_VER
#define __declspec(x)
#endif

#ifndef BOOL
typedef int BOOL;
#endif
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef int INT;
typedef uint32_t DWORD;
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef int32_t HRESULT;
#ifndef VOID
typedef void VOID;
#endif
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef WORD ATOM;
typedef void *HANDLE;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef void *HWND;
typedef void *HDC;
typedef void *HICON;
typedef void *HCURSOR;
typedef void *HBRUSH;
typedef void *HMENU;
typedef void *HBITMAP;
typedef void *HGLOBAL;
typedef void *HPALETTE;
typedef void *HKEY;
typedef void *LPVOID;
typedef void *LPSECURITY_ATTRIBUTES;
typedef void *LPOVERLAPPED;
typedef char *LPSTR;
typedef char const *LPCSTR;
typedef char const *LPCTSTR;
typedef BYTE *LPBYTE;
typedef BYTE *PBYTE;
typedef WORD *LPWORD;
typedef DWORD *LPDWORD;

typedef int SOCKET;

#if defined(__ANDROID__) || defined(__linux__)
typedef struct in_addr IN_ADDR;
#else
typedef struct in_addr {
	ULONG s_addr;
} IN_ADDR;
#endif

typedef struct WSAData {
	WORD wVersion;
	WORD wHighVersion;
	char szDescription[257];
	char szSystemStatus[129];
	WORD iMaxSockets;
	WORD iMaxUdpDg;
	char *lpVendorInfo;
} WSADATA, *LPWSADATA;

#ifndef INVALID_SOCKET
#define INVALID_SOCKET ((SOCKET)-1)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#ifndef MAXGETHOSTSTRUCT
#define MAXGETHOSTSTRUCT 1024
#endif

class IDirectSound;
class IDirectSoundBuffer;
struct tWAVEFORMATEX;
struct tDSBUFFERDESC;

typedef IDirectSound *LPDIRECTSOUND;
typedef IDirectSoundBuffer *LPDIRECTSOUNDBUFFER;
typedef tWAVEFORMATEX WAVEFORMATEX;
typedef WAVEFORMATEX *LPWAVEFORMATEX;
typedef tDSBUFFERDESC DSBUFFERDESC;
typedef DSBUFFERDESC *LPDSBUFFERDESC;

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif

#ifndef _MAX_DRIVE
#define _MAX_DRIVE 3
#endif

#ifndef _MAX_DIR
#define _MAX_DIR 256
#endif

#ifndef _MAX_FNAME
#define _MAX_FNAME 256
#endif

#ifndef _MAX_EXT
#define _MAX_EXT 256
#endif

typedef struct tagPOINT {
	LONG x;
	LONG y;
} POINT, *LPPOINT;

typedef struct tagRECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT, *LPRECT;

typedef struct tagMSG {
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	POINT pt;
} MSG, *LPMSG;

extern "C" BOOL MacWin_PeekMessage(MSG *msg, HWND hwnd, UINT min_filter, UINT max_filter, UINT remove);
extern "C" BOOL MacWin_GetMessage(MSG *msg, HWND hwnd, UINT min_filter, UINT max_filter);
extern "C" BOOL MacWin_TranslateMessage(MSG const *msg);
extern "C" LRESULT MacWin_DispatchMessage(MSG const *msg);
extern "C" BOOL MacWin_PostMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
extern "C" void MacWin_PostQuitMessage(int code);
extern "C" short MacWin_GetKeyState(int key);
extern "C" short MacWin_GetAsyncKeyState(int key);
extern "C" BOOL MacWin_GetCursorPos(LPPOINT point);
extern "C" int MacWin_ToAscii(UINT virt_key, UINT scan_code, BYTE const *key_state, LPWORD trans_key, UINT flags);

typedef struct _STARTUPINFOA {
	DWORD cb;
	LPSTR lpReserved;
	LPSTR lpDesktop;
	LPSTR lpTitle;
	DWORD dwX;
	DWORD dwY;
	DWORD dwXSize;
	DWORD dwYSize;
	DWORD dwXCountChars;
	DWORD dwYCountChars;
	DWORD dwFillAttribute;
	DWORD dwFlags;
	WORD wShowWindow;
	WORD cbReserved2;
	LPBYTE lpReserved2;
	HANDLE hStdInput;
	HANDLE hStdOutput;
	HANDLE hStdError;
} STARTUPINFO, STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
	HANDLE hProcess;
	HANDLE hThread;
	DWORD dwProcessId;
	DWORD dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

typedef struct _OVERLAPPED {
	ULONG Internal;
	ULONG InternalHigh;
	DWORD Offset;
	DWORD OffsetHigh;
	HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED_STRUCT;

typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct tagWNDCLASSA {
	UINT style;
	WNDPROC lpfnWndProc;
	int cbClsExtra;
	int cbWndExtra;
	HINSTANCE hInstance;
	HICON hIcon;
	HCURSOR hCursor;
	HBRUSH hbrBackground;
	LPCSTR lpszMenuName;
	LPCSTR lpszClassName;
} WNDCLASS, WNDCLASSA, *LPWNDCLASSA;

typedef struct tagPALETTEENTRY {
	BYTE peRed;
	BYTE peGreen;
	BYTE peBlue;
	BYTE peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagLOGPALETTE {
	WORD palVersion;
	WORD palNumEntries;
	PALETTEENTRY palPalEntry[1];
} LOGPALETTE, *LPLOGPALETTE;

typedef struct tagRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD, *LPRGBQUAD;

typedef struct tagBITMAPFILEHEADER {
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagBITMAPCOREHEADER {
	DWORD bcSize;
	WORD bcWidth;
	WORD bcHeight;
	WORD bcPlanes;
	WORD bcBitCount;
} BITMAPCOREHEADER, *LPBITMAPCOREHEADER;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;

typedef struct _RTL_CRITICAL_SECTION {
	pthread_mutex_t mutex;
	int initialized;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef struct _MEMORYSTATUS {
	DWORD dwLength;
	DWORD dwMemoryLoad;
	DWORD dwTotalPhys;
	DWORD dwAvailPhys;
	DWORD dwTotalPageFile;
	DWORD dwAvailPageFile;
	DWORD dwTotalVirtual;
	DWORD dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

typedef struct _WIN32_FIND_DATA {
	DWORD dwFileAttributes;
	char cFileName[MAX_PATH];
	char cAlternateFileName[14];
} WIN32_FIND_DATA, *LPWIN32_FIND_DATA;

typedef struct _FILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *LPFILETIME;

typedef struct _BY_HANDLE_FILE_INFORMATION {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD dwVolumeSerialNumber;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD nNumberOfLinks;
	DWORD nFileIndexHigh;
	DWORD nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

#define BI_RGB 0
#define GMEM_MOVEABLE 0x0002
#define GMEM_ZEROINIT 0x0040

#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001
#define PM_NOYIELD 0x0002

#define WM_NULL 0x0000
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_ACTIVATEAPP 0x001C
#define WM_SYSCOMMAND 0x0112
#define WM_QUIT 0x0012
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_RBUTTONDBLCLK 0x0206
#define WM_MBUTTONDOWN 0x0207
#define WM_MBUTTONUP 0x0208
#define WM_MBUTTONDBLCLK 0x0209
#define WM_USER 0x0400

#define SC_CLOSE 0xF060
#define SC_SCREENSAVE 0xF140

#ifndef VK_LBUTTON
#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_MBUTTON 0x04
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_0 0x30
#define VK_1 0x31
#define VK_2 0x32
#define VK_3 0x33
#define VK_4 0x34
#define VK_5 0x35
#define VK_6 0x36
#define VK_7 0x37
#define VK_8 0x38
#define VK_9 0x39
#define VK_A 0x41
#define VK_B 0x42
#define VK_C 0x43
#define VK_D 0x44
#define VK_E 0x45
#define VK_F 0x46
#define VK_G 0x47
#define VK_H 0x48
#define VK_I 0x49
#define VK_J 0x4A
#define VK_K 0x4B
#define VK_L 0x4C
#define VK_M 0x4D
#define VK_N 0x4E
#define VK_O 0x4F
#define VK_P 0x50
#define VK_Q 0x51
#define VK_R 0x52
#define VK_S 0x53
#define VK_T 0x54
#define VK_U 0x55
#define VK_V 0x56
#define VK_W 0x57
#define VK_X 0x58
#define VK_Y 0x59
#define VK_Z 0x5A
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_NONE_BA 0xBA
#define VK_NONE_BB 0xBB
#define VK_NONE_BC 0xBC
#define VK_NONE_BD 0xBD
#define VK_NONE_BE 0xBE
#define VK_NONE_BF 0xBF
#define VK_NONE_C0 0xC0
#define VK_NONE_DB 0xDB
#define VK_NONE_DC 0xDC
#define VK_NONE_DD 0xDD
#define VK_NONE_DE 0xDE
#endif

#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002

#define WS_POPUP 0x80000000UL
#define WS_EX_TOPMOST 0x00000008UL

#define SM_CXSCREEN 0
#define SM_CYSCREEN 1

#define MAKEINTRESOURCE(i) ((LPCSTR)(uintptr_t)((WORD)(i)))

#define SW_SHOWMAXIMIZED 3
#define SW_SHOWMINIMIZED 2
#define SW_MINIMIZE 6
#define SW_RESTORE 9

#define MB_OK 0x00000000L
#define MB_YESNO 0x00000004L
#define MB_ICONSTOP 0x00000010L
#define MB_ICONQUESTION 0x00000020L
#define MB_ICONEXCLAMATION 0x00000030L
#define IDOK 1
#define IDYES 6
#define IDNO 7

#define GENERIC_READ 0x80000000UL
#define GENERIC_WRITE 0x40000000UL
#define FILE_SHARE_READ 0x00000001UL
#define FILE_SHARE_WRITE 0x00000002UL
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define TRUNCATE_EXISTING 5
#define FILE_ATTRIBUTE_READONLY 0x00000001UL
#define FILE_ATTRIBUTE_HIDDEN 0x00000002UL
#define FILE_ATTRIBUTE_SYSTEM 0x00000004UL
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010UL
#define FILE_ATTRIBUTE_NORMAL 0x00000080UL
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100UL
#define FILE_FLAG_OVERLAPPED 0x40000000UL
#define DUPLICATE_SAME_ACCESS 0x00000002UL
#define THREAD_ALL_ACCESS 0x001F03FFUL
#define THREAD_PRIORITY_TIME_CRITICAL 15
#define FILE_BEGIN SEEK_SET
#define FILE_CURRENT SEEK_CUR
#define FILE_END SEEK_END

#define SEM_FAILCRITICALERRORS 0x0001
#define SEM_NOOPENFILEERRORBOX 0x8000

#define ERROR_SUCCESS 0L
#define ERROR_FILE_NOT_FOUND 2L
#define ERROR_PATH_NOT_FOUND 3L
#define ERROR_ACCESS_DENIED 5L
#define ERROR_INVALID_HANDLE 6L
#define ERROR_NOT_ENOUGH_MEMORY 8L
#define ERROR_ALREADY_EXISTS 183L
#define ERROR_NO_MORE_ITEMS 259L

#define HKEY_CLASSES_ROOT ((HKEY)(intptr_t)0x80000000UL)
#define HKEY_LOCAL_MACHINE ((HKEY)(intptr_t)0x80000002UL)
#define KEY_READ 0x20019UL
#define KEY_WRITE 0x20006UL
#define REG_SZ 1
#define REG_DWORD 4
#define CLRDTR 6

#define LOWORD(l) ((WORD)((DWORD)(l) & 0xffff))
#define HIWORD(l) ((WORD)((DWORD)(l) >> 16))
#define MAKELONG(low, high) ((LONG)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))

#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline DWORD *__win_last_error_storage(void)
{
	static DWORD value = ERROR_SUCCESS;
	return &value;
}

static inline void SetLastError(DWORD error) { *__win_last_error_storage() = error; }
static inline DWORD GetLastError(void) { return *__win_last_error_storage(); }

static inline void __win_set_last_error_from_errno(void)
{
	switch (errno) {
		case ENOENT:
			SetLastError(ERROR_FILE_NOT_FOUND);
			break;
		case EACCES:
		case EPERM:
			SetLastError(ERROR_ACCESS_DENIED);
			break;
		case ENOMEM:
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			break;
		case EEXIST:
			SetLastError(ERROR_ALREADY_EXISTS);
			break;
		default:
			SetLastError((DWORD)errno);
			break;
	}
}

static inline HANDLE __win_handle_from_fd(int fd)
{
	return (HANDLE)(intptr_t)(fd + 1);
}

static inline int __win_fd_from_handle(HANDLE handle)
{
	return (int)((intptr_t)handle - 1);
}

static inline DWORD GetTickCount(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (DWORD)((tv.tv_sec * 1000UL) + (tv.tv_usec / 1000UL));
}

static inline void GlobalMemoryStatus(LPMEMORYSTATUS status)
{
	if (!status) {
		return;
	}

	long page_size = sysconf(_SC_PAGESIZE);
	long pages = sysconf(_SC_PHYS_PAGES);
	DWORD total = 64UL * 1024UL * 1024UL;

	if (page_size > 0 && pages > 0) {
		total = (DWORD)((unsigned long)page_size * (unsigned long)pages);
	}

	status->dwLength = sizeof(*status);
	status->dwTotalPhys = total;
	status->dwAvailPhys = total / 2;
	status->dwTotalPageFile = total;
	status->dwAvailPageFile = total / 2;
	status->dwTotalVirtual = total;
	status->dwAvailVirtual = total / 2;
	status->dwMemoryLoad = 50;
}

static inline UINT SetErrorMode(UINT) { return 0; }

static inline void GetSystemTime(LPSYSTEMTIME time)
{
	struct timeval tv;
	struct tm tmv;
	if (!time) {
		return;
	}
	gettimeofday(&tv, 0);
	gmtime_r(&tv.tv_sec, &tmv);
	time->wYear = (WORD)(tmv.tm_year + 1900);
	time->wMonth = (WORD)(tmv.tm_mon + 1);
	time->wDayOfWeek = (WORD)tmv.tm_wday;
	time->wDay = (WORD)tmv.tm_mday;
	time->wHour = (WORD)tmv.tm_hour;
	time->wMinute = (WORD)tmv.tm_min;
	time->wSecond = (WORD)tmv.tm_sec;
	time->wMilliseconds = (WORD)(tv.tv_usec / 1000);
}

static inline void Sleep(DWORD milliseconds)
{
	usleep(milliseconds * 1000);
}

static inline BOOL SetCommBreak(HANDLE) { return TRUE; }
static inline BOOL ClearCommBreak(HANDLE) { return TRUE; }
static inline BOOL EscapeCommFunction(HANDLE, DWORD) { return TRUE; }

static inline HGLOBAL GlobalAlloc(UINT flags, size_t bytes)
{
	void *ptr = malloc(bytes);
	if ((flags & GMEM_ZEROINIT) && ptr) {
		memset(ptr, 0, bytes);
	}
	return ptr;
}

static inline HGLOBAL GlobalReAlloc(HGLOBAL mem, size_t bytes, UINT flags)
{
	void *ptr = realloc(mem, bytes);
	if ((flags & GMEM_ZEROINIT) && ptr) {
		(void)flags;
	}
	return ptr;
}

static inline LPVOID GlobalLock(HGLOBAL mem) { return mem; }
static inline BOOL GlobalUnlock(HGLOBAL) { return FALSE; }
static inline HGLOBAL GlobalFree(HGLOBAL mem) { free(mem); return 0; }

static inline HANDLE CreateFile(LPCSTR filename, DWORD access, DWORD, LPSECURITY_ATTRIBUTES, DWORD creation, DWORD, HANDLE)
{
	int flags = 0;
	if ((access & GENERIC_READ) && (access & GENERIC_WRITE)) {
		flags |= O_RDWR;
	} else if (access & GENERIC_WRITE) {
		flags |= O_WRONLY;
	} else {
		flags |= O_RDONLY;
	}

	switch (creation) {
		case CREATE_NEW:
			flags |= O_CREAT | O_EXCL;
			break;
		case CREATE_ALWAYS:
			flags |= O_CREAT | O_TRUNC;
			break;
		case OPEN_ALWAYS:
			flags |= O_CREAT;
			break;
		case TRUNCATE_EXISTING:
			flags |= O_TRUNC;
			break;
		case OPEN_EXISTING:
		default:
			break;
	}

	int fd = open(filename, flags, 0666);
	if (fd < 0) {
		__win_set_last_error_from_errno();
		return INVALID_HANDLE_VALUE;
	}

	SetLastError(ERROR_SUCCESS);
	return __win_handle_from_fd(fd);
}

static inline BOOL CloseHandle(HANDLE handle)
{
	int fd = __win_fd_from_handle(handle);
	if (handle == INVALID_HANDLE_VALUE || fd < 0) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (close(fd) == 0) {
		SetLastError(ERROR_SUCCESS);
		return TRUE;
	}
	__win_set_last_error_from_errno();
	return FALSE;
}

static inline BOOL ReadFile(HANDLE handle, LPVOID buffer, DWORD bytes_to_read, LPDWORD bytes_read, LPOVERLAPPED)
{
	int fd = __win_fd_from_handle(handle);
	if (bytes_read) {
		*bytes_read = 0;
	}
	if (handle == INVALID_HANDLE_VALUE || fd < 0) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	ssize_t result = read(fd, buffer, bytes_to_read);
	if (result < 0) {
		__win_set_last_error_from_errno();
		return FALSE;
	}
	if (bytes_read) {
		*bytes_read = (DWORD)result;
	}
	SetLastError(ERROR_SUCCESS);
	return TRUE;
}

static inline BOOL WriteFile(HANDLE handle, void const *buffer, DWORD bytes_to_write, LPDWORD bytes_written, LPOVERLAPPED)
{
	int fd = __win_fd_from_handle(handle);
	if (bytes_written) {
		*bytes_written = 0;
	}
	if (handle == INVALID_HANDLE_VALUE || fd < 0) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	ssize_t result = write(fd, buffer, bytes_to_write);
	if (result < 0) {
		__win_set_last_error_from_errno();
		return FALSE;
	}
	if (bytes_written) {
		*bytes_written = (DWORD)result;
	}
	SetLastError(ERROR_SUCCESS);
	return TRUE;
}

static inline DWORD SetFilePointer(HANDLE handle, LONG distance, LONG *, DWORD move_method)
{
	int fd = __win_fd_from_handle(handle);
	if (handle == INVALID_HANDLE_VALUE || fd < 0) {
		SetLastError(ERROR_INVALID_HANDLE);
		return 0xFFFFFFFFUL;
	}
	off_t result = lseek(fd, distance, (int)move_method);
	if (result < 0) {
		__win_set_last_error_from_errno();
		return 0xFFFFFFFFUL;
	}
	SetLastError(ERROR_SUCCESS);
	return (DWORD)result;
}

static inline DWORD GetFileSize(HANDLE handle, LPDWORD high_size)
{
	int fd = __win_fd_from_handle(handle);
	struct stat info;
	if (high_size) {
		*high_size = 0;
	}
	if (handle == INVALID_HANDLE_VALUE || fd < 0) {
		SetLastError(ERROR_INVALID_HANDLE);
		return 0xFFFFFFFFUL;
	}
	if (fstat(fd, &info) != 0) {
		__win_set_last_error_from_errno();
		return 0xFFFFFFFFUL;
	}
	if (high_size) {
		*high_size = (DWORD)(((uint64_t)info.st_size) >> 32);
	}
	SetLastError(ERROR_SUCCESS);
	return (DWORD)info.st_size;
}

static inline BOOL DeleteFile(LPCSTR filename)
{
	if (unlink(filename) == 0) {
		SetLastError(ERROR_SUCCESS);
		return TRUE;
	}
	__win_set_last_error_from_errno();
	return FALSE;
}

static inline BOOL CreateDirectory(LPCSTR path, LPSECURITY_ATTRIBUTES)
{
	if (mkdir(path, 0777) == 0 || errno == EEXIST) {
		SetLastError(ERROR_SUCCESS);
		return TRUE;
	}
	__win_set_last_error_from_errno();
	return FALSE;
}

static inline BOOL GetFileInformationByHandle(HANDLE handle, LPBY_HANDLE_FILE_INFORMATION info)
{
	int fd = __win_fd_from_handle(handle);
	struct stat st;
	if (!info || handle == INVALID_HANDLE_VALUE || fd < 0) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (fstat(fd, &st) != 0) {
		__win_set_last_error_from_errno();
		return FALSE;
	}
	memset(info, 0, sizeof(*info));
	info->nFileSizeLow = (DWORD)st.st_size;
	info->nFileSizeHigh = (DWORD)(((uint64_t)st.st_size) >> 32);
	info->ftCreationTime.dwLowDateTime = (DWORD)st.st_ctime;
	info->ftLastAccessTime.dwLowDateTime = (DWORD)st.st_atime;
	info->ftLastWriteTime.dwLowDateTime = (DWORD)st.st_mtime;
	SetLastError(ERROR_SUCCESS);
	return TRUE;
}

static inline BOOL GetFileTime(HANDLE handle, LPFILETIME creation, LPFILETIME access, LPFILETIME write)
{
	BY_HANDLE_FILE_INFORMATION info;
	if (!GetFileInformationByHandle(handle, &info)) {
		return FALSE;
	}
	if (creation) *creation = info.ftCreationTime;
	if (access) *access = info.ftLastAccessTime;
	if (write) *write = info.ftLastWriteTime;
	return TRUE;
}

static inline BOOL FileTimeToDosDateTime(FILETIME const *filetime, LPWORD dosdate, LPWORD dostime)
{
	time_t raw = filetime ? (time_t)filetime->dwLowDateTime : 0;
	struct tm tmv;
	if (!localtime_r(&raw, &tmv)) {
		return FALSE;
	}
	if (dosdate) {
		*dosdate = (WORD)(((tmv.tm_year - 80) << 9) | ((tmv.tm_mon + 1) << 5) | tmv.tm_mday);
	}
	if (dostime) {
		*dostime = (WORD)((tmv.tm_hour << 11) | (tmv.tm_min << 5) | (tmv.tm_sec / 2));
	}
	return TRUE;
}

static inline BOOL DosDateTimeToFileTime(WORD dosdate, WORD dostime, LPFILETIME filetime)
{
	struct tm tmv;
	memset(&tmv, 0, sizeof(tmv));
	tmv.tm_year = ((dosdate >> 9) & 0x7f) + 80;
	tmv.tm_mon = ((dosdate >> 5) & 0x0f) - 1;
	tmv.tm_mday = dosdate & 0x1f;
	tmv.tm_hour = (dostime >> 11) & 0x1f;
	tmv.tm_min = (dostime >> 5) & 0x3f;
	tmv.tm_sec = (dostime & 0x1f) * 2;
	if (!filetime) {
		return FALSE;
	}
	filetime->dwLowDateTime = (DWORD)mktime(&tmv);
	filetime->dwHighDateTime = 0;
	return TRUE;
}

static inline BOOL SetFileTime(HANDLE, FILETIME const *, FILETIME const *, FILETIME const *)
{
	return TRUE;
}

static inline BOOL GetVolumeInformation(LPCSTR, LPSTR volume_name, DWORD volume_name_size, LPDWORD serial, LPDWORD max_component, LPDWORD flags, LPSTR fs_name, DWORD fs_name_size)
{
	if (volume_name && volume_name_size) {
		snprintf(volume_name, volume_name_size, "Macintosh");
	}
	if (serial) *serial = 0;
	if (max_component) *max_component = 255;
	if (flags) *flags = 0;
	if (fs_name && fs_name_size) {
		snprintf(fs_name, fs_name_size, "APFS");
	}
	return TRUE;
}

static inline LONG RegOpenKeyEx(HKEY, LPCSTR, DWORD, DWORD, HKEY *)
{
	return ERROR_FILE_NOT_FOUND;
}

static inline LONG RegQueryValueEx(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD)
{
	return ERROR_FILE_NOT_FOUND;
}

static inline LONG RegQueryValue(HKEY, LPCSTR, LPSTR, LONG *)
{
	return ERROR_FILE_NOT_FOUND;
}

static inline LONG RegSetValueEx(HKEY, LPCSTR, DWORD, DWORD, LPBYTE, DWORD)
{
	return ERROR_ACCESS_DENIED;
}

static inline LONG RegDeleteValue(HKEY, LPCSTR)
{
	return ERROR_FILE_NOT_FOUND;
}

static inline LONG RegCloseKey(HKEY)
{
	return ERROR_SUCCESS;
}

static inline int MessageBox(HWND, LPCSTR text, LPCSTR title, UINT)
{
	if (title || text) {
		fprintf(stderr, "%s%s%s\n", title ? title : "MessageBox", title && text ? ": " : "", text ? text : "");
	}
	return IDOK;
}
static inline void OutputDebugString(LPCSTR text) { if (text) fputs(text, stderr); }
static inline DWORD GetVersion(void) { return 0; }
static inline DWORD GetModuleFileName(HMODULE, LPSTR buffer, DWORD size)
{
	if (!buffer || size == 0) {
		return 0;
	}
	if (!getcwd(buffer, size)) {
		strncpy(buffer, ".", size - 1);
		buffer[size - 1] = 0;
		return (DWORD)strlen(buffer);
	}
	size_t len = strlen(buffer);
	if (len + 1 < size) {
		buffer[len++] = '/';
		buffer[len] = 0;
	}
	return (DWORD)len;
}
static inline BOOL ShowWindow(HWND, int) { return TRUE; }
static inline BOOL UpdateWindow(HWND) { return TRUE; }
static inline BOOL SetForegroundWindow(HWND) { return TRUE; }
static inline HWND FindWindow(LPCSTR, LPCSTR) { return 0; }
static inline BOOL IsWindow(HWND window) { return window != 0; }
static inline HWND GetTopWindow(HWND) { return 0; }
static inline int ShowCursor(BOOL) { return 0; }
static inline BOOL GetCursorPos(LPPOINT point) { return MacWin_GetCursorPos(point); }
static inline BOOL ClipCursor(LPRECT) { return TRUE; }
static inline HMODULE LoadLibrary(LPCSTR) { return 0; }
static inline void *GetProcAddress(HMODULE, LPCSTR) { return 0; }
static inline BOOL FreeLibrary(HMODULE) { return TRUE; }
static inline BOOL CreateProcess(LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCSTR, STARTUPINFO *, PROCESS_INFORMATION *process)
{
	if (process) {
		memset(process, 0, sizeof(*process));
	}
	return FALSE;
}
static inline DWORD WaitForSingleObject(HANDLE, DWORD) { return 0; }
static inline BOOL GetExitCodeProcess(HANDLE, LPDWORD code) { if (code) *code = 0; return FALSE; }
static inline BOOL TerminateProcess(HANDLE, UINT) { return FALSE; }
static inline void ExitProcess(UINT code) { exit((int)code); }
static inline ATOM RegisterClass(WNDCLASS const *) { return 1; }
static inline HWND CreateWindowEx(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID) { return (HWND)(intptr_t)1; }
static inline int GetSystemMetrics(int metric) { return metric == SM_CYSCREEN ? 480 : 640; }
static inline HWND SetFocus(HWND window) { return window; }
static inline UINT RegisterWindowMessage(LPCSTR) { return WM_USER + 100; }
static inline int DialogBox(HANDLE, LPCTSTR, HWND, DLGPROC) { return 0; }

typedef struct __win_find_handle {
#if defined(__ANDROID__)
	char **paths;
	size_t count;
#else
	glob_t globbuf;
#endif
	size_t index;
} __win_find_handle;

#if defined(__ANDROID__)
static inline char *__win_strdup(char const *text)
{
	size_t length = strlen(text);
	char *copy = (char *)malloc(length + 1);
	if (copy) {
		memcpy(copy, text, length + 1);
	}
	return copy;
}

static inline char *__win_join_path(char const *directory, char const *name)
{
	size_t dir_len = strlen(directory);
	size_t name_len = strlen(name);
	int needs_slash = dir_len > 0 && strcmp(directory, ".") != 0 && directory[dir_len - 1] != '/';
	char *path = (char *)malloc(dir_len + (needs_slash ? 1 : 0) + name_len + 1);
	if (!path) {
		return NULL;
	}
	if (strcmp(directory, ".") == 0) {
		memcpy(path, name, name_len + 1);
	} else {
		memcpy(path, directory, dir_len);
		if (needs_slash) {
			path[dir_len++] = '/';
		}
		memcpy(path + dir_len, name, name_len + 1);
	}
	return path;
}

static inline void __win_free_find_paths(__win_find_handle *state)
{
	if (!state) {
		return;
	}
	if (state->paths) {
		size_t index;
		for (index = 0; index < state->count; ++index) {
			free(state->paths[index]);
		}
		free(state->paths);
	}
	state->paths = NULL;
	state->count = 0;
}

static inline BOOL __win_add_find_path(__win_find_handle *state, char const *path)
{
	char **paths = (char **)realloc(state->paths, sizeof(char *) * (state->count + 1));
	if (!paths) {
		return FALSE;
	}
	state->paths = paths;
	state->paths[state->count] = __win_strdup(path);
	if (!state->paths[state->count]) {
		return FALSE;
	}
	state->count++;
	return TRUE;
}

static inline BOOL __win_split_pattern(char const *pattern, char *directory, size_t directory_size, char *name, size_t name_size)
{
	char const *slash = strrchr(pattern, '/');
	size_t dir_len;
	if (!pattern || !directory || !name || directory_size == 0 || name_size == 0) {
		return FALSE;
	}
	if (!slash) {
		strncpy(directory, ".", directory_size - 1);
		directory[directory_size - 1] = '\0';
		strncpy(name, pattern, name_size - 1);
		name[name_size - 1] = '\0';
		return TRUE;
	}
	dir_len = (size_t)(slash - pattern);
	if (dir_len == 0) {
		dir_len = 1;
	}
	if (dir_len >= directory_size) {
		return FALSE;
	}
	memcpy(directory, pattern, dir_len);
	directory[dir_len] = '\0';
	strncpy(name, slash + 1, name_size - 1);
	name[name_size - 1] = '\0';
	return TRUE;
}
#endif

static inline void __win_fill_find_data(char const *path, WIN32_FIND_DATA *data)
{
	struct stat st;
	char const *name = strrchr(path, '/');
	name = name ? name + 1 : path;
	memset(data, 0, sizeof(*data));
	strncpy(data->cFileName, name, sizeof(data->cFileName) - 1);
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
		data->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	} else {
		data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	}
}

static inline HANDLE FindFirstFile(LPCSTR pattern, WIN32_FIND_DATA *data)
{
	__win_find_handle *state;
#if defined(__ANDROID__)
	char directory[1024];
	char name_pattern[260];
	DIR *dir;
	struct dirent *entry;
#endif
	if (!pattern || !data) {
		return INVALID_HANDLE_VALUE;
	}
	state = (__win_find_handle *)malloc(sizeof(*state));
	if (!state) {
		return INVALID_HANDLE_VALUE;
	}
	memset(state, 0, sizeof(*state));
#if defined(__ANDROID__)
	if (!__win_split_pattern(pattern, directory, sizeof(directory), name_pattern, sizeof(name_pattern))) {
		free(state);
		return INVALID_HANDLE_VALUE;
	}
	dir = opendir(directory);
	if (!dir) {
		free(state);
		return INVALID_HANDLE_VALUE;
	}
	while ((entry = readdir(dir)) != NULL) {
		char *full_path;
		if (fnmatch(name_pattern, entry->d_name, FNM_CASEFOLD) != 0) {
			continue;
		}
		full_path = __win_join_path(directory, entry->d_name);
		if (!full_path) {
			closedir(dir);
			__win_free_find_paths(state);
			free(state);
			return INVALID_HANDLE_VALUE;
		}
		if (!__win_add_find_path(state, full_path)) {
			free(full_path);
			closedir(dir);
			__win_free_find_paths(state);
			free(state);
			return INVALID_HANDLE_VALUE;
		}
		free(full_path);
	}
	closedir(dir);
	if (state->count == 0) {
		__win_free_find_paths(state);
		free(state);
		return INVALID_HANDLE_VALUE;
	}
	state->index = 0;
	__win_fill_find_data(state->paths[state->index], data);
#else
	if (glob(pattern, 0, NULL, &state->globbuf) != 0 || state->globbuf.gl_pathc == 0) {
		globfree(&state->globbuf);
		free(state);
		return INVALID_HANDLE_VALUE;
	}
	state->index = 0;
	__win_fill_find_data(state->globbuf.gl_pathv[state->index], data);
#endif
	return (HANDLE)state;
}

static inline BOOL FindNextFile(HANDLE handle, WIN32_FIND_DATA *data)
{
	__win_find_handle *state = (__win_find_handle *)handle;
	if (!state || state == (__win_find_handle *)INVALID_HANDLE_VALUE || !data) {
		return FALSE;
	}
	state->index++;
#if defined(__ANDROID__)
	if (state->index >= state->count) {
		return FALSE;
	}
	__win_fill_find_data(state->paths[state->index], data);
#else
	if (state->index >= state->globbuf.gl_pathc) {
		return FALSE;
	}
	__win_fill_find_data(state->globbuf.gl_pathv[state->index], data);
#endif
	return TRUE;
}

static inline BOOL FindClose(HANDLE handle)
{
	__win_find_handle *state = (__win_find_handle *)handle;
	if (!state || state == (__win_find_handle *)INVALID_HANDLE_VALUE) {
		return FALSE;
	}
#if defined(__ANDROID__)
	__win_free_find_paths(state);
#else
	globfree(&state->globbuf);
#endif
	free(state);
	return TRUE;
}
static inline void PostQuitMessage(int code) { MacWin_PostQuitMessage(code); }
static inline LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
static inline BOOL PeekMessage(MSG *msg, HWND hwnd, UINT min_filter, UINT max_filter, UINT remove) { return MacWin_PeekMessage(msg, hwnd, min_filter, max_filter, remove); }
static inline BOOL GetMessage(MSG *msg, HWND hwnd, UINT min_filter, UINT max_filter) { return MacWin_GetMessage(msg, hwnd, min_filter, max_filter); }
static inline BOOL TranslateMessage(MSG const *msg) { return MacWin_TranslateMessage(msg); }
static inline LRESULT DispatchMessage(MSG const *msg) { return MacWin_DispatchMessage(msg); }
static inline short GetKeyState(int key) { return MacWin_GetKeyState(key); }
static inline short GetAsyncKeyState(int key) { return MacWin_GetAsyncKeyState(key); }
static inline int VkKeyScan(int ch) { return ch & 0xFF; }
static inline UINT MapVirtualKey(UINT code, UINT) { return code; }
static inline int ToAscii(UINT virt_key, UINT scan_code, BYTE const *key_state, LPWORD trans_key, UINT flags) { return MacWin_ToAscii(virt_key, scan_code, key_state, trans_key, flags); }
static inline HCURSOR SetCursor(HCURSOR cursor) { return cursor; }
static inline HCURSOR LoadCursor(HINSTANCE, LPCSTR) { return 0; }
static inline HICON LoadIcon(HINSTANCE, LPCSTR) { return 0; }
static inline HMODULE GetModuleHandle(LPCSTR) { return 0; }
static inline BOOL GetClientRect(HWND, LPRECT rect)
{
	if (rect) {
		rect->left = rect->top = 0;
		rect->right = 320;
		rect->bottom = 200;
	}
	return TRUE;
}
static inline BOOL ClientToScreen(HWND, LPPOINT) { return TRUE; }
static inline BOOL InvalidateRect(HWND, const RECT *, BOOL) { return TRUE; }
static inline int ReleaseDC(HWND, HDC) { return 1; }
static inline HDC GetDC(HWND) { return 0; }
static inline BOOL SendMessage(HWND, UINT, WPARAM, LPARAM) { return TRUE; }
static inline BOOL PostMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) { return MacWin_PostMessage(hwnd, message, wparam, lparam); }

#define DRIVE_UNKNOWN 0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED 3
#define DRIVE_REMOTE 4
#define DRIVE_CDROM 5
#define DRIVE_RAMDISK 6

static inline UINT GetDriveType(LPCSTR root)
{
	if (root && (root[0] == 'c' || root[0] == 'C' || root[0] == 'd' || root[0] == 'D') && root[1] == ':') {
		return DRIVE_CDROM;
	}
	return DRIVE_FIXED;
}

typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);

static inline HANDLE GetCurrentProcess(void) { return (HANDLE)1; }
static inline HANDLE GetCurrentThread(void) { return (HANDLE)2; }
static inline BOOL DuplicateHandle(HANDLE, HANDLE source, HANDLE, HANDLE *target, DWORD, BOOL, DWORD)
{
	if (target) {
		*target = source;
	}
	return TRUE;
}
static inline HANDLE CreateThread(LPSECURITY_ATTRIBUTES, DWORD, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD)
{
	return 0;
}
static inline BOOL SetThreadPriority(HANDLE, int) { return TRUE; }

static inline void InitializeCriticalSection(LPCRITICAL_SECTION section)
{
	if (!section || section->initialized) return;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&section->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	section->initialized = TRUE;
}

static inline void EnterCriticalSection(LPCRITICAL_SECTION section)
{
	if (!section) return;
	if (!section->initialized) InitializeCriticalSection(section);
	pthread_mutex_lock(&section->mutex);
}

static inline void LeaveCriticalSection(LPCRITICAL_SECTION section)
{
	if (!section || !section->initialized) return;
	pthread_mutex_unlock(&section->mutex);
}

static inline void DeleteCriticalSection(LPCRITICAL_SECTION section)
{
	if (!section || !section->initialized) return;
	pthread_mutex_destroy(&section->mutex);
	section->initialized = FALSE;
}

static inline int stricmp(char const *left, char const *right) { return strcasecmp(left, right); }
static inline int strcmpi(char const *left, char const *right) { return strcasecmp(left, right); }
static inline int strnicmp(char const *left, char const *right, size_t count) { return strncasecmp(left, right, count); }
static inline int memicmp(void const *left, void const *right, size_t count)
{
	unsigned char const *lptr = (unsigned char const *)left;
	unsigned char const *rptr = (unsigned char const *)right;
	for (size_t index = 0; index < count; ++index) {
		unsigned char lch = lptr[index];
		unsigned char rch = rptr[index];
		if (lch >= 'a' && lch <= 'z') lch = (unsigned char)(lch - ('a' - 'A'));
		if (rch >= 'a' && rch <= 'z') rch = (unsigned char)(rch - ('a' - 'A'));
		if (lch != rch) {
			return (int)lch - (int)rch;
		}
	}
	return 0;
}

static inline char *strupr(char *text)
{
	if (!text) {
		return text;
	}
	for (char *ptr = text; *ptr; ++ptr) {
		if (*ptr >= 'a' && *ptr <= 'z') {
			*ptr = (char)(*ptr - ('a' - 'A'));
		}
	}
	return text;
}

static inline char *_strlwr(char *text)
{
	if (!text) {
		return text;
	}
	for (char *ptr = text; *ptr; ++ptr) {
		*ptr = (char)tolower((unsigned char)*ptr);
	}
	return text;
}

static inline char *strrev(char *text)
{
	if (!text) {
		return text;
	}
	size_t len = strlen(text);
	for (size_t left = 0, right = len ? len - 1 : 0; left < right; ++left, --right) {
		char tmp = text[left];
		text[left] = text[right];
		text[right] = tmp;
	}
	return text;
}

#ifndef _stricmp
#define _stricmp stricmp
#endif

#ifndef _strnicmp
#define _strnicmp strnicmp
#endif

#ifndef _strupr
#define _strupr strupr
#endif

#ifndef _strrev
#define _strrev strrev
#endif

static inline void _makepath(char *path, char const *drive, char const *dir, char const *fname, char const *ext)
{
	path[0] = 0;
	if (drive && drive[0]) {
		strcat(path, drive);
	}
	if (dir && dir[0]) {
		strcat(path, dir);
	}
	if (fname && fname[0]) {
		strcat(path, fname);
	}
	if (ext && ext[0]) {
		if (ext[0] != '.') {
			strcat(path, ".");
		}
		strcat(path, ext);
	}
}

static inline void _splitpath(char const *path, char *drive, char *dir, char *fname, char *ext)
{
	char const *name_start = path ? strrchr(path, '/') : 0;
	char const *dot = path ? strrchr(path, '.') : 0;
	if (drive) {
		drive[0] = 0;
	}
	if (!path) {
		if (dir) dir[0] = 0;
		if (fname) fname[0] = 0;
		if (ext) ext[0] = 0;
		return;
	}
	name_start = name_start ? name_start + 1 : path;
	if (dot && dot < name_start) {
		dot = 0;
	}
	if (dir) {
		size_t len = (size_t)(name_start - path);
		memcpy(dir, path, len);
		dir[len] = 0;
	}
	if (fname) {
		size_t len = dot ? (size_t)(dot - name_start) : strlen(name_start);
		memcpy(fname, name_start, len);
		fname[len] = 0;
	}
	if (ext) {
		strcpy(ext, dot ? dot : "");
	}
}

#ifndef _lrotl
static inline long _lrotl(long value, int shift)
{
	unsigned long x = (unsigned long)value;
	unsigned int bits = (unsigned int)(sizeof(unsigned long) * 8);
	shift &= (int)(bits - 1);
	return (long)((x << shift) | (x >> (bits - shift)));
}
#endif

static inline char *itoa(int value, char *buffer, int radix)
{
	if (radix == 16) {
		sprintf(buffer, "%x", value);
	} else {
		sprintf(buffer, "%d", value);
	}
	return buffer;
}

static inline char *ltoa(long value, char *buffer, int radix)
{
	if (radix == 16) {
		sprintf(buffer, "%lx", value);
	} else {
		sprintf(buffer, "%ld", value);
	}
	return buffer;
}

#ifdef __cplusplus
}
#endif
