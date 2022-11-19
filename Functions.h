#pragma once

#ifndef _SCPARK_FUNCTIONS_H
#define _SCPARK_FUNCTIONS_H

/*
MFCÀÇ ¾î¶² Å¬·¡½º¸¦ »ó¼Ó¹Ş¾Æ ¸¸µç ´Ù¸¥ cpp, hÆÄÀÏ°ú´Â ´Ş¸®
Common Æú´õ¿¡ Á¸ÀçÇÏ´Â ÀÏ¹İ cpp, hÆÄÀÏÀ» ¸¸µé¾ú´Ù¸é
MFCÀÇ Å¬·¡½º µîÀ» »ç¿ëÇÏ±â À§ÇØ¼­´Â Afxwin.h µî ¸î °³ÀÇ hÆÄÀÏµéÀ» include ½ÃÄÑÁà¾ß ÇÑ´Ù.
¿¹Àü¿¡´Â ±×³É cpp ÆÄÀÏ¿¡ #include "stdafx.h"¶ó°í¸¸ ÇØµµ h, cppÆÄÀÏ¿¡¼­
MFC Å¬·¡½ºµéÀ» »ç¿ëÇÒ ¼ö ÀÖ¾úÀ¸³ª Common Æú´õ¿¡¼­ ÇÁ·ÎÁ§Æ® Æú´õ¿¡ ÀÖ´Â Àú ÆÄÀÏÀ» ÀÎ½ÄÇÒ ¸® ¾ø´Ù.
ÇÁ·ÎÁ§Æ® ¼¼ÆÃ¿¡¼­ "Precompiled Header"°¡ "Use"·Î µÇ¾î ÀÖ¾î¼­ µ¿ÀÛÇß´Ù°í »ı°¢µÇÁö¸¸
IntelliSense¿¡ ÀÇÇØ #include "stdafx.h" ¹®±¸¿¡ »¡°£»ö ¿¡·¯ Ç¥½Ã°¡ ¶ß°í
´Ù¸¥ ÄÚµåµé¿¡µµ »¡°£»ö ¿¡·¯·Î Ç¥½ÃµÇ¸é¼­ ÅøÆÁ Á¤º¸°¡ Ç¥½ÃµÇÁö ¾Ê´Â ºÒÆíÇÔÀÌ »ı°å´Ù.
ÀÌ¸¦ Á¤¼®ÀûÀ¸·Î ÇØ°áÇÏ´Â ¹æ¹ıÀº MFC Å¬·¡½º ¹× È®Àå Å¬·¡½ºµéÀ» »ç¿ëÇÏ±â À§ÇØ¼­´Â
hÆÄÀÏ¿¡ ¾Æ·¡¿Í °°ÀÌ ÇÊ¿äÇÑ hÆÄÀÏµéÀ» includeÇÏ°í
cppÆÄÀÏÀº ÇÁ·ÎÁ§Æ® ¼¼ÆÃ¿¡¼­ "Precompiled Header"¸¦ »ç¿ë¾ÈÇÔÀ¸·Î ¼³Á¤ÇÏ´Â °ÍÀÌ¶ó°í »ı°¢µÈ´Ù.
[MFC Çì´õ¿Í ¶óÀÌºê·¯¸® ¼³¸í ÂüÁ¶]
http://www.devpia.com/MAEUL/Contents/Detail.aspx?BoardID=51&MAEULNo=20&no=567
*/
#include <Afxwin.h>
#include <Afxdisp.h>

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string>
#include <memory>
#include <WinPerf.h>	//for cpu usage
#include <sys/timeb.h>	//for elapsed time
#include <atlimage.h>
#include <stdarg.h>		// For va_start, etc.
#include <memory>		// For std::unique_ptr

#include <random>
#include <vector>
#include <deque>
#include <algorithm>
#include <gdiplus.h>

#ifdef GDIPVER
#undef GDIPVER
#endif

#define GDIPVER 0x0110


#include "../Common/colors.h"

#define _std_cpp11 201103L
#define _std_cpp14 201402L
#define _std_cpp17 201703L
#define _std_cpp20 202002L

#ifdef UNICODE
#define CHARSET _T(",ccs=UTF-8")
#define __function__ __FUNCTIONW__
#else
#define CHARSET _T("")
#define __function__ __FUNCTION__
#endif


#ifdef _MSC_VER
#define __class_func__ __function__
#endif

#ifdef __GNUG__
#include <cxxabi.h>
#include <execinfo.h>
char *class_func(const char *c, const char *f)
{
	int status;
	static char buff[100];
	char *demangled = abi::__cxa_demangle(c, NULL, NULL, &status);
	snprintf(buff, sizeof(buff), "%s::%s", demangled, f);
	free(demangled);
	return buff;
}
#define __class_func__ class_func(typeid(*this).name(), __func__)
#endif


#ifndef uint8_t
typedef unsigned char uint8_t;
#endif // !uchar

#ifndef byte
typedef unsigned char byte;
#endif

#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )

//À§ÀÇ µ¥ÀÌÅÍ Å¸ÀÔÀ» #define uchar unsigned char °ú °°ÀÌ ¼±¾ğÇÏ¸é
//´Ù¸¥ defineµé°ú Ãæµ¹ÇÑ´Ù. ¶ÇÇÑ ¾Æ·¡¿Í °°Àº ÀÌÀ¯·Î¶óµµ
//Å¸ÀÔÀÇ Á¤ÀÇ´Â #define ´ë½Å typedefÀ» ÀÌ¿ëÇÏÀÚ.
/*
typedef int *t1;
#define t2 int *

t1 a, b; // a is 'int*' and b is 'int*'
t2 c, d; // c is 'int*' and d is 'int'
*/

#define		TO_STRING(a) #a
#define		ID2String(ID, sID) sID.Format( "%s", #ID )	

#define		Swap32BE(x) ((((x) & 0xff000000) >> 24) | (((x) & 0xff0000) >> 8) | (((x) & 0xff00) << 8) | (((x) & 0xff) << 24))
#define		Swap16BE(x) ((((x) & 0xff00) >> 8)| (((x) & 0xff) << 8))

//¹è¿­ÀÇ °¹¼ö ¸®ÅÏ
#define		countof(array) (sizeof(array)/sizeof(array[0]))

#define		MESSAGE_DOWNLOAD_DATA	WM_APP + 1000

#define		SYSTEM_LOGOFF			0
#define		SYSTEM_REBOOT			1
#define		SYSTEM_POWEROFF			2

#define		IsShiftPressed() ( 0x8000 ==(GetKeyState(VK_SHIFT) & 0x8000   ))
#define		IsCtrlPressed()  ( 0x8000 ==(GetKeyState(VK_CONTROL) & 0x8000 ))

#define		CLIP(x) ((x) > 255 ? 255 : (x) < 0 ? 0 : x)
#define		check_range_return(x, lower, upper) {if ((x) < (lower) || (x) > (upper)) return;}



enum FILE_TYPE
{
	//FILE_TYPE_UNKNOWN = 0, //already defined in WinBase.h
	FILE_TYPE_SOUND = 1,
	FILE_TYPE_IMAGE,
	FILE_TYPE_SUBTITLE,
	FILE_TYPE_VIDEO,
	FILE_TYPE_VIDEO_BIN,
};

enum TEXT_ENCODING
{
	text_encoding_unknown = -1,
	text_encoding_ansi,
	text_encoding_utf8bom,
	text_encoding_unicode,
};

enum RATIO_RECT_ATTACH
{
	attach_hcenter	= 0x00000000,	//0000 0001
	attach_vcenter	= 0x00000000,	//0000 1000
	attach_left		= 0x00000001,	//0000 0000
	attach_right	= 0x00000002,	//0000 0010
	attach_top		= 0x00000004,	//0000 0100
	attach_bottom	= 0x00000008,	//0001 0000
};

#define		FILE_EXTENSION_VIDEO			_T("avi;mpg;mp4;mpeg;mkv;mov;wmv;wma;asf;ts;m2ts;3gp")
#define		FILE_EXTENSION_SOUND			_T("mp3;m4a;wav")
#define		FILE_EXTENSION_MEDIA			CString(FILE_EXTENSION_VIDEO) + _T(";") + CString(FILE_EXTENSION_SOUND)
#define		FILE_EXTENSION_IMAGE			_T("bmp;jpg;jpeg;png;webp;gif;yuv;raw")
#define		FILE_EXTENSION_SUBTITLE			_T("smi;srt")

#define		FILENAME_NOT_ALLOWED_CHAR		_T("\\:*?\"<>|")

#define		DOWNLOAD_CONNECT_STATS	0
#define		DOWNLOAD_SET_TOTAL_SIZE	1	//ÀüÃ¼ ´Ù¿î¹ŞÀ» ÆÄÀÏ Å©±â
#define		DOWNLOAD_SET_FILE_SIZE	2	//ÇöÀç ´Ù¿î¹ŞÀ» ÆÄÀÏ Å©±â
#define		DOWNLOAD_RECEIVED_SIZE	3
#define		DOWNLOAD_COMPLETE_ONE	4
#define		DOWNLOAD_COMPLETE_TOTAL	5

#ifndef		MIN
#define		MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef		MAX
#define		MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

#ifndef		DEFAULT_RANGE
#define		DEFAULT_RANGE( x, min, max, default_value ) ( ((x) > max || (x) < min) ? default_value : (x) )
#endif

//¼Ò¼öÁ¡ n+1 ÀÚ¸®¿¡¼­ ¹İ¿Ã¸²ÇÏ¿© ¼Ò¼öÁ¡ nÀÚ¸®·Î Ç¥ÇöÇÑ´Ù.
//ex. ROUND(0.545, 2) = 0.55
//ex. ROUND(0.545, 1) = 0.50
//ex. ROUND(0.545, 0) = 1.00
#define		ROUND(x, n) ( floor((x) * pow(float(10), (n)) + 0.5f) / pow(float(10), (n)) )
#define		SQR(x)		((x)*(x))
#define		PI				3.141592
#define		RADIAN(x)		(x) * PI / 180.0	//degree to radian
#define		DEGREE(x)		(x) * 180. / PI
#define		DISTANCE(x1,y1,x2,y2)	sqrt( (double)( ((x2)-(x1)) * ((x2)-(x1)) + ((y2)-(y1)) * ((y2)-(y1)) ) )

#define		IsHexaChar(c) ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))

#define		KByte		1024.0
#define		MByte		KByte * 1024.0
#define		GByte		MByte * 1024.0
#define		TByte		GByte * 1024.0

#ifndef WIDTHSTEP4
#define		WIDTHSTEP4(bits)	( ((bits) + 31) / 32 * 4 )	//bits is not width, but (width * bitCount)
#endif

//widthº¸´Ù Å« 4ÀÇ ¹è¼ö·Î ¸¸µé¾îÁØ´Ù.
#define		MAKE4WIDTH_U(width)		(((width) + 3) & ~3)
//widthº¸´Ù ÀÛÀº 4ÀÇ ¹è¼ö·Î ¸¸µé¾îÁØ´Ù.
#define		MAKE4WIDTH_D(width)		(((width) - 3) & ~3 | 4)

extern		int			g_nDaysOfMonth[12];

//serial port °ü·Ã
#define		MAX_BAUD_RATE	15
extern		int			g_nBaudRate[MAX_BAUD_RATE];

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS) 
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64 
#else 
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL 
#endif 

#define MakeArray2(TYPE,VARNAME,C1,C2)     TYPE (*VARNAME)[C2]     = (TYPE (*)[C2])     new TYPE[C1*C2];    AutoEraser < TYPE > Auto##VARNAME(VARNAME)
#define MakeArray3(TYPE,VARNAME,C1,C2,C3)  TYPE (*VARNAME)[C2][C3] = (TYPE (*)[C2][C3]) new TYPE[C1*C2*C3]; AutoEraser < TYPE > Auto##VARNAME(VARNAME)


//½Ó¿ÚÊÍ·Å
#define SAFE_RELEASE(pObject) { if (pObject!=NULL) { pObject->Release(); pObject=NULL; } }

//É¾³ıÖ¸Õë
#define SAFE_DELETE(pData) { try { delete pData; } catch (...) { ASSERT(FALSE); } pData=NULL; } 

//¹Ø±Õ¾ä±ú
#define SAFE_CLOSE_HANDLE(hHandle) { if (hHandle!=NULL) { CloseHandle(hHandle); hHandle=NULL; } }

//É¾³ıÊı×é
#define SAFE_DELETE_ARRAY(pData) { try { delete [] pData; } catch (...) { ASSERT(FALSE); } pData=NULL; } 

//////////////////////////////////////////////////////////////////////////////////
//´ò¿ªÁ´½Ó
#define SHELL_OPEN(String)	ShellExecute(NULL, TEXT("open"), String, NULL, NULL, SW_SHOWNORMAL);

#define MAP_STYLE(src, dest) if(dwStyle & (src)) dwText |= (dest)
#define NMAP_STYLE(src, dest) if(!(dwStyle & (src))) dwText |= (dest)

#define extract_bits(data, area, loc) (((data)>>(loc)) & (area))
#define make_area(area, length)\
{\
   int32_t i = 0;\
   int32_t bit = 1;\
   for ( i = 0; i < (length-1); i++ ){ \
      bit = ((bit<<1)|1);\
   }\
   area = bit;\
}

void debug_string( const char* psz, ... );

template < typename T > class AutoEraser
{
private:
	T * VarPtr;
public:
	template < typename T2 >
	AutoEraser( T2& ptr)    {    VarPtr = (T*)ptr;   }
	~AutoEraser()           {    delete[] VarPtr;    }
};
/*
void func()
{
	MakeArray2(int,ddd,2,3);
	MakeArray3(double,fff,2,3,4);
}

ex>

int array[2][3];À» ¸¸µé·Á°í ÇÏ¸é..
MakeArray2(int,array,2,3);

int array[2][3][4];´Â
MakeArray3(int,array,2,3,4);
*/

struct timezone  
{ 
  int  tz_minuteswest; /* minutes W of Greenwich */ 
  int  tz_dsttime;     /* type of dst correction */ 
}; 


class CMouseEvent
{
public:
	CMouseEvent();
	CMouseEvent( CPoint pt1, int msg1, short zDelta1 = 0 ) { pt = pt1; msg = msg1; zDelta = zDelta1; }

	CPoint	pt;
	int		msg;
	short	zDelta;
};

class vector2
{
public:
     double x, y;

   //»ı¼ºÀÚ
	vector2()
	{
		x = 0.0, y = 0.0;
	}

    vector2(double _x, double _y)
	{
        x = _x, y = _y;
    }
    //¿ÜÀû
    double cross(const vector2& other) const
	{
        return x*other.y-y*other.x;
    }

    /* ¿¬»êÀÚ ¿À¹ö·ÎµùÀ» ÅëÇØ ½ÇÁ¦ º¤ÅÍÀÇ ¿¬»êÀ» ±¸ÇöÇÕ´Ï´Ù. */

    //º¤ÅÍÀÇ ½Ç¼ö¹è
    vector2 operator * (double r) const
	{
        return vector2(x*r, y*r);
    }
    //º¤ÅÍÀÇ µ¡¼À
    vector2 operator + (vector2 other) const
	{
        return vector2(x + other.x, y + other.y);
    }
    //º¤ÅÍÀÇ »¬¼À
    vector2 operator - (vector2 other) const
	{
        return vector2(x - other.x, y - other.y);
    }
    //µÎ º¤ÅÍÀÇ ºñ±³
    bool operator == (vector2 other) const
	{
        return x == other.x && y == other.y;
    }
    bool operator < (vector2 other) const
	{
        return x < other.x && y < other.y;
    }
};


//timer °ü·Ãº¯¼ö
extern		double		g_dTime0;
extern		double		g_dTime1;
extern		CString		g_sForTimer;

//ÇÔ¼ö ¼öÇà ½Ã°£À» Ãâ·Â. a´Â ¹®ÀÚ¿­, x´Â ¼öÇà ÇÔ¼ö
#define		TIME(caption, x, b)	/*ptimer_start(0);*/\
								g_dTime0 = ptimer_get_time(0);\
								(x);\
								g_dTime1 = ptimer_get_time(0);\
								g_sForTimer.Format( _T("%s : %f\n"), (caption), g_dTime1 - g_dTime0 );\
								if ( b ) \
									AfxMessageBox( g_sForTimer ); \
								else \
									printf( g_sForTimer );

								//printf( g_sForTimer );

struct	NETWORK_INFO
{
	TCHAR		sDescription[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
	TCHAR		sIPAddress[16];
	TCHAR		sGateway[16];
	TCHAR		sSubnetMask[16];
	TCHAR		sMacAddress[16];
};

void		Trace(char* szFormat, ...);

//////////////////////////////////////////////////////////////////////////
//ÇÁ·Î¼¼½º °ü·Ã
CString		GetFileVersionInformation(CString strFileName, CString strFlag);
CString		GetExeDirectory(bool includeSlash = false);
CString		GetExeRootDirectory();
CString		GetExeFilename( bool bFullPath = FALSE );
CString		GetExeFileTitle();
CString		GetCurrentDirectory();
ULONG		GetPID( CString processname );
ULONG		ProcIDFromWnd(HWND hwnd);
HWND		GetHWNDbyPID(ULONG pid);
CString		GetProcessNameByPID(const DWORD pid);
bool		IsRunning(CString processname);
//bool		ProcessKill(CString processname);
HWND		GetWindowHandleFromProcessID(DWORD dwProcId);
bool		IsDuplicatedRun();

//PID, ÇÁ·Î¼¼½º ÀÌ¸§, À©µµ¿ì Å¸ÀÌÆ² ÀÌ¸§, À©µµ¿ì Å¬·¡½º ÀÌ¸§À¸·Î Å¬·¡½ºÀÇ »ıÁ¸ »óÅÂ¸¦ ±¸ÇÒ¼ö ÀÖ½À´Ï´Ù. from Devpia
bool		CheckProcessUsingPID(unsigned long pid);
bool		CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName);
bool		CheckProcessUsingProcessName(LPCTSTR processName);

//¸ğ´ÏÅÍ Á¤º¸
extern std::deque<CRect> g_dqMonitors;
BOOL		CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

//Å¬¸³º¸µå clipboard
	bool		copy_to_clipboard(HWND hWnd, CString str);

//////////////////////////////////////////////////////////////////////////
//¹®ÀÚ¿­
	bool		Compare_By_Case_Sensitive(CString str1, CString str2, bool bCase);
	int			find_string(CString target, CString find_string, bool case_sensitive = false);
	//dqSrc¿¡ dqFind°¡ ÀÖ´ÂÁö °Ë»ç. ÇöÀç´Â AND ¿¬»êÀÌ¹Ç·Î dqFindÀÇ ¸ğµç ¿ø¼Ò°¡ dqSrc¿¡ Æ÷ÇÔµÇ¾î ÀÖ¾î¾ß ÇÔ.
	bool		find_dqstring(std::deque<CString> dqSrc, std::deque<CString> dqFind, TCHAR op = '&', bool bWholeWord = false, bool bCaseSensitive = false);
	int			Find_Divide_Position_By_Punctuation( CString str );
	int			FindStringFromArray( CStringArray& ar, CString sTarget, bool bCaseSensitive = false, bool bWholeWord = false );
	//int			FindStringFromDeque( std::deque<CString> dq, CString sTarget, bool bCaseSensitive = false, bool bWholeWord = false );
	CString		GetCommaString( CString sString, CString sComma = _T(",") );
	bool		IsNumericString(const CString& strSource);
	//¾ç¼öÀÇ Á¤¼ö°ª ¹®ÀÚ¿­ÀÎÁö
	bool		IsNatural(LPCTSTR lpszValue);
	//¼Ò¼ıÁ¡±îÁö Çã¿ëÇÏ¿© ¼ıÀÚ°ªÀÎÁö
	bool		IsNumeric(LPCTSTR lpszValue);
	//À½¼ö¸¦ Çã¿ëÇÏ´Â Á¤¼ö°ª ¹®ÀÚ¿­ÀÎÁö
	bool		IsInteger(LPCTSTR lpszValue);
	//startºÎÅÍ ½ÃÀÛÇØ¼­ Ã³À½ ¸¸³ª´Â ¼ıÀÚ ¿µ¿ªÀ» ÃßÃâÇØ¼­ num¿¡ ³Ñ°ÜÁØ´Ù.
	//¼ıÀÚ ¿µ¿ªÀÌ ¾øÀ¸¸é false¸¦ ¸®ÅÏÇÑ´Ù.
	bool		get_number_from_string(CString str, int &num, int start = 0);
	//¹®ÀÚ¿­¿¡ Æ÷ÇÔµÈ ¼ıÀÚ¹®ÀÚ¸¦ ¼ıÀÚ·Î °£ÁÖÇÏ¿© ºñ±³ÇÑ´Ù. "a5"´Â "a12"º¸´Ù ÀÛ´Ù.
	bool		is_greater_with_numeric(CString str0, CString str1);
	//ÁÖ¾îÁø ¹®ÀÚ¿­ÀÌ ¾ËÆÄºª°ú ¼ıÀÚ·Î¸¸ ±¸¼ºµÈ ¹®ÀÚ¿­ÀÎÁö °Ë»çÇÑ´Ù.
	//excepts¿¡´Â Æ÷ÇÔµÇµµ µÇ´Â ¹®ÀÚ¿­µéÀÌ µé¾îÀÖ´Âµ¥ ÀÌµéÀº ;À¸·Î ±¸ºĞµÇ¾î ÀÖ°í
	//°Ë»çÇÏ±â Àü¿¡ ¹Ì¸® »èÁ¦ÇÑ ÈÄ °Ë»çÇÑ´Ù.
	bool		IsAlphaNumeric(CString str, CString excepts = _T(""));
	//ÇÑ±Û·Î¸¸ ±¸¼ºµÈ ¹®ÀÚ¿­ÀÎÁö
	bool		IsHangul(CString str);
	CString		ConvertInt2AZ( int n );	//nÀ» 26Áø¼ö ¿¢¼¿ ÄÃ·³ ÀÎµ¦½º·Î º¯È¯ÇÑ ¹®ÀÚ¿­À» ¸®ÅÏ
	CString		GetToken( CString& str, LPCTSTR c );
	CString		GetToken( CString src, CString separator, int n );
	std::deque<CString>	GetTokenString(CString src, CString separator);
	int			GetTokenString(CString src, std::deque<CString>& dqToken, TCHAR separator = ' ', bool allowEmpty = true, int nMaxToken = -1);
	int			GetTokenString(CString src, std::deque<CString>& dqToken, std::deque<TCHAR> separator, bool allowEmpty = true, int nMaxToken = -1);
	int			getTokenString( TCHAR *src, TCHAR *separator, CString *sToken, int nMaxToken );
	int			getTokenString( char *src, char *separator, char **sToken, int nMaxToken );
	//deque¿¡ ÀÖ´Â ¿ø¼ÒµéÀ» ±¸ºĞÀÚ·Î ÇÏ´Â ÇÏ³ªÀÇ ¹®ÀÚ¿­·Î ¸®ÅÏ
	CString		get_tokenized(std::deque<CString> dq, TCHAR separator = ';');
	CString		get_str(CString& buff, CString sep = _T("|"));
	int			get_int(CString& buff, CString sep = _T("|"));
	double		get_double(CString& buff, CString sep = _T("|"));
	//unit			: 0:bytes, 1:KB, 2:MB, 3:GB
	//unit_string	: ´ÜÀ§¸¦ Ç¥½ÃÇÒ Áö
	//comma			: Á¤¼ö ºÎºĞ¿¡ ÀÚ¸®¼ö ÄŞ¸¶¸¦ Ç¥½ÃÇÒ Áö
	//floats		: ¼Ò¼öÁ¡À» ¸î ÀÚ¸®±îÁö Ç¥½ÃÇÒÁö
	CString		GetUnitSizeString(int64_t size, int unit = 3, int floats = 0, bool unit_string = true, bool comma = false);

	//¸Ç ¸¶Áö¸· ÀÎÀÚ´Â ¹İµå½Ã NULLÀ» ³Ö¾îÁà¾ß ³¡À» ¾Ë ¼ö ÀÖ´Ù.
	bool		isOneOf( LPCTSTR src, ... );
	//src ¹®ÀÚ¿­¿¡ set_of_keyword¿¡ ³ª¿­µÈ ´Ü¾î°¡ ÀÖ´ÂÁö °Ë»ç.
	//set_of_keyword´Â ¼¼¹ÌÄİ·ĞÀ¸·Î ±¸ºĞÇØ¼­ ¿©·¯ ¹®ÀÚ ¶Ç´Â ¹®ÀÚ¿­À» ³ÖÀ» ¼ö ÀÖ´Ù.
	//ex. src = "abcd1234"ÀÏ ¶§ set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
	bool		is_exist_keyword(CString src, CString set_of_keyword, bool case_sensitive = false, bool whole_word = false);

	//dqList¿¡¼­ element°ª°ú ÀÏÄ¡ÇÏ´Â Ç×¸ñÀÇ index¸¦ ¸®ÅÏÇÑ´Ù. ¾øÀ¸¸é -1À» ¸®ÅÏ.
	template <typename T> int find_index(std::deque <T> *dqList, T element)
	{
		std::deque<T>::iterator it = std::find(dqList->begin(), dqList->end(), element);
		if (it != dqList->end())
		{
			return distance(dqList->begin(), it);
		}

		return -1;
	}

	//strÀÇ from À§Ä¡ ÀÌÈÄ¿¡ ÀÖ´Â ¼ıÀÚ ¿µ¿ª°ªÀ» num¿¡ ³Ö¾îÁÖ°í ¼ıÀÚ ½ÃÀÛÀ§Ä¡¸¦ returnÇÑ´Ù.
	int			extract_digit_number( char *str, int from, double *num );

	//http://yeobi27.tistory.com/280
	//A2W, A2T ¹× ±× ¹İ´ë ¸ÅÅ©·ÎµéÀº ½ºÅÃÀ» »ç¿ëÇÏ¹Ç·Î ¹®Á¦ ¼ÒÁö°¡ ÀÖ°í Å©±â Á¦ÇÑµµ ÀÖÀ¸¹Ç·Î
	//°¡±ŞÀû CA2W, CA2TµîÀ» »ç¿ëÇÑ´Ù. ´Ü ÀÌ ¸ÅÅ©·ÎµéÀº encodingÀ» º¯°æÇÒ ¼ö ¾ø´Ù.
	std::wstring CString2wstring( const char* str );
	std::string CString2string( CString cs );
	CString		string2CString( std::string s );
	LPCWSTR		CString2LPCWSTR(CString str);
	char*		CString2char(CString str);
	TCHAR*		CString2TCHAR(CString str);
	LPCSTR		CString2LPCSTR(CString str);
	LPCWSTR		LPCTSTR2LPCWSTR(LPCTSTR str, UINT codePage = CP_UTF8);

	//chStrÀÇ À¯È¿ÇÑ ±æÀÌ¸¦ ÀÌ¹Ì ¾Ë°í ÀÖ´Ù¸é length¸¦ ÁöÁ¤ÇØÁà¾ß Á¤È®ÇÏ´Ù.
	//±×·¸Áö ¾ÊÀ» °æ¿ì chStrÀÇ ³¡¿¡ '\0'°¡ ¾øÀ» °æ¿ì ¾²·¹±â ¹®ÀÚµé±îÁö Æ÷ÇÔµÉ ¼ö ÀÖ´Ù.
	CString		char2CString(char *chStr, int length = -1);
	CString		TCHAR2CString(TCHAR *str);
	VARIANT		CString2VARIANT( CString str );

	//°ø¹é, '\t', '\r', '\n' ¸ğµÎ Á¦°Å
	void		Trim(char* src);
	void		Trim(std::deque<CString>* dq);
	//src¹®ÀÚ¿­¿¡¼­ chars¸¦ ¸ğµÎ Á¦°ÅÇÑ´Ù.
	void		remove_chars(CString &src, CString chars);
	//src¹®ÀÚ¿­¿¡¼­ ÁöÁ¤µÈ ±¸°£ÀÇ ¹®ÀÚ¿­À» Á¦°ÅÇÑ´Ù.

	inline BYTE toHex(const BYTE &x);
	void		hex2byte(const char *in, int len, uint8_t *out);

	unsigned int dec2bcd(unsigned int num);
	CString		URLEncode( CString sIn );
	CString		Utf8ToStringT(LPSTR str);
	char*		UTF8ToANSI( char *pszCode );
	CString		UrlDecode(LPCTSTR url);

	//void		NewTypingMessageFromCodePage(TCHAR* Message, UINT CodePage=0);


	float		Similarity( char *str1, char *str2 );
	float		Similarity( CString str1, CString str2 );

	void		SortStringArray( CStringArray* pArray );
	bool		StringArrayCompareAndSwap( CStringArray* pArray, int pos ); //called at SortStringArray function.

	CString		FormattedString(LPCTSTR lpszFormat, ...);
	int			GetTrimLength( CString str );
	//str¿¡¼­ ¾ÕµÚ ¹®ÀÚ¿­ »çÀÌ¿¡ ÀÖ´Â ¼­ºê ¹®ÀÚ¿­À» ¸®ÅÏÇÑ´Ù. ¾øÀ¸¸é "" ¸®ÅÏ.
	CString		ExtractSubString( CString src, CString sPrev, CString sPost );	//sPrev(ÇÊµå ¾Õ¿¡ ¿À´Â ¹®ÀÚ¿­), sPost(ÇÊµå µÚ¿¡ ¿À´Â ¹®ÀÚ¿­)
	//std::string string_format(const std::string fmt, ...);
	std::string string_format(const std::string fmt_str, ...);	//A safer and more efficient(http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf)
	/*
	//format¿¡ ÁÙ¶§´Â %s°¡ ¾Æ´Ñ %S??
	template<typename ... Args> std::string string_format(const std::string& format, Args ... args)
	{
		int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
		if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
		auto size = static_cast<size_t>(size_s);
		std::unique_ptr<char[]> buf(new char[size]);
		std::snprintf(buf.get(), size, format.c_str(), args ...);
		return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
	}
	*/
	CString		loadResString(UINT nID);



//µ¥ÀÌÅÍ º¯È¯
	CString		i2S(int64_t nValue, bool bComma = false, bool fill_zero = false, int digits = 0 );
	CString		i2HS(int64_t nValue, bool bCapital = true );
	CString		d2S( double dValue, bool bComma = false, int nfDigit = -1 );	//nfDigit : ¼Ò¼öÁ¡ ÀÚ¸´¼ö. -1ÀÌ¸é ±×´ë·Î Ãâ·Â.
	char*		ushortToBinary(unsigned short i);
	char*		intToBinary(int i);
	char*		uintToBinary(unsigned int i);
	int			binaryToInt(char *s);
	unsigned int binaryToUint(char *s);
	//¾î¶² ¼öÀÇ x¹øÂ° ºñÆ®°ª ¸®ÅÏ.
	int			get_bit(int number, int x);
	//x°¡ 1ÀÌ¸é n¹øÂ° °ªÀ» 1·Î º¯°æ, x°¡ 0ÀÌ¸é n¹øÂ° °ªÀ» 0À¸·Î º¯°æ
	void		set_bit(int& number, int n, int x);
	int			HexaStringToInt( CString str );
	CString		GetByteString(uint8_t* bt, int n, bool upper = true, bool prefix = true );
	CString		BinaryToHexString(BYTE* pData, int length, TCHAR separator = _T(' '));
	int			getPrecision( double d, bool bExceptZero = true );	//¼Ò¼öÁ¡ ÀÚ¸´¼ö ¸®ÅÏ
	template<class T> CString getBinaryString( T number, bool blank = true )
	{
		int i;
		std::vector<char> result;
		CString str;

		for ( i = 0; number > 0; i++ )
		{
			result.push_back( '0' + number % 2 );
			number /= 2;
		}

		for ( i = 0; i < result.size(); i++ )
		{
			str = result.at(i) + str;
			if ( blank && (i % 4 == 3) )
				str = _T(" ") + str;
		}

		str.Trim();

		return str;
	}

	unsigned __int64 binaryStringToInt64(char *str, int len);
	unsigned int getBitsValueFromInt64( uint8_t* bt, int num_of_bytes, int startbit, int bit_length );
	unsigned int getBitsValueFromInt64( CString *btStr, int num_of_bytes, int startbit, int bit_length );
	unsigned int parse_can_data(unsigned char* data,unsigned int startbit,unsigned int length);				//from ADAS source

	int			getSignedFromUnsigned( unsigned int value, int bit_length );
	unsigned int	Crc16( unsigned char* rdata, unsigned int len );
	uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte);
	uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size);




//////////////////////////////////////////////////////////////////////////
//ÆÄÀÏ °ü·Ã
	CString		GetFileNameFromFullPath(CString sFullPath);
	CString		GetFolderNameFromFullPath(CString sFullPath, bool includeSlash = false);	//= PathRemoveFileSpec
	CString		GetFileTitle(CString sFullPath);
	CString		GetFileExtension(CString filename, bool dot = false);
	int			GetFileTypeFromFilename( CString filename );
	int			GetFileTypeFromExtension( CString sExt );
	bool		ChangeExtension(CString& filepath, CString newExt, bool applyRealFile);

	//Æú´õ¿¡ ÀÖ´Â ÆÄÀÏµé Áß filetitleÀÌ°í extension¿¡ ÇØ´çÇÏ´Â ÆÄÀÏ¸íÀ» ¸®ÅÏÇÑ´Ù.
	std::deque<CString>		get_filename_from_filetitle(CString folder, CString filetitle, CString extension);
	std::deque<CString>		get_filename_from_filetitle(CString filename, CString extension);

	uint64_t	GetFileSize( CString sfile );
	uint64_t	get_folder_size(CString path);
	//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
	//unit_string	: ´ÜÀ§¸¦ Ç¥½ÃÇÒ Áö (default = true)
	CString		GetFileSizeString(CString sfile, int unit_limit = 3, int floats = 0, bool unit_string = true);
	CTime		GetFileCreationTime( CString sfile );
	CTime		GetFileLastModifiedTime( CString sfile );
	CTime		GetFileLastAccessTime( CString sfile );
	CString		GetMostRecentFile( CString sFolder, CString sWildCard = _T("*.*"), int nReturnType = 1 );
	CString		GetMostRecentDateFile( CString sFolder, CString sWildCard = _T("*.*") );	//°¡Àå ÃÖ±Ù ³¯Â¥ ÆÄÀÏ¸í ¸®ÅÏ
	CString		GetNextIndexFile( CString sCurrentFile, bool bNext = TRUE );	//´ÙÀ½ ÀÎµ¦½ºÀÇ ÆÄÀÏ¸í ¸®ÅÏ
	int			GetNextFileIndex( CString sCurrentFile );						//´ÙÀ½ ÆÄÀÏÀÇ ÀÎµ¦½º ¸®ÅÏ
	int			GetNumberFromFile( CString sfile );			//ÆÄÀÏÀ» ÀĞ¾î¼­ ÇÑ°³ÀÇ ¼ıÀÚ°ªÀ» ¸®ÅÏÇÑ´Ù. (ÆÄÀÏÀÌ ¾øÀ¸¸é -99999 ¸¦ ¸®ÅÏÇÑ´Ù.)
	bool		WriteNumberToFile( CString sfile, int n );	//¼ıÀÚ°ªÀ» ÆÄÀÏ¿¡ ±â·ÏÇØÁØ´Ù.
	CString		GetFileProperty( CString sFilePath, CString sProperty );
	size_t		read_raw( CString sfile, uint8_t *dst, size_t size );
	bool		save2raw( CString sfile, uint8_t *data, size_t size );
	int			RenameFiles(CString folder, CString oldName, CString newName, bool overwrite = false, bool bWholename = true, bool bRecursive = false);
	bool		delete_file(CString fullpath, bool bTrashCan = false);
	int			get_text_encoding(CString sfile);

	//mp4 ÆÄÀÏÀÇ Æ¯Á¤ ÅÂ±× µ¥ÀÌÅÍ Áß ¿øÇÏ´Â À§Ä¡ÀÇ µ¥ÀÌÅÍ¸¦ ÃßÃâÇÑ´Ù.
	//MOBIS ÇÁ·ÎÁ§Æ® ÀúÀå MP4´Â mdat ÇÊµåÀÇ 0x40¹øÁöºÎÅÍ 28 bytes°¡
	//µ¿¿µ»óÀÌ »ı¼ºµÈ Àı´ë½Ã°£ÀÌ ÀúÀåµÇ¾î ÀÖ´Ù.
	//nÀº 0x3CºÎÅÍ 4¹ÙÀÌÆ®°¡ ±× Å©±âÀÌ´Ù.(28 bytes)
	char*		GetDataFromMP4File(char* sfile, char* sTag, uint8_t tagStart, int tagLength );

//////////////////////////////////////////////////////////////////////////
//ÀÎÅÍ³İ ÆÄÀÏ
	DWORD		GetURLFileSize( LPCTSTR pUrl );
	bool		DownloadFile( LPCTSTR pUrl, CString strFileName, bool bOverwrite = TRUE, HWND hWnd = NULL );
	CString		DownloadURLFile(CString sUrl, CString sLocalFileName, HWND hWnd = NULL );
	bool		CheckFileIsURL( CString sURL );
	bool		is_valid_url(CString url);
	void		GetURLFileInfo( CString sURL, bool &bInURL, bool &bFileType );
	bool		ReadURLFile( LPCTSTR pUrl, CString &strBuffer );
	void		ReadURLFileString( CString sURL, CString &sString );
	CString		GetDefaultBrowserPath();	//[ÃâÃ³] [VC++] Windows ±âº» À¥ ºê¶ó¿ìÀú ÆÄÀÏ °æ·Î ¾ò¾î¿À±â|ÀÛ¼ºÀÚ µ¥ºê¸Ó½Å


//////////////////////////////////////////////////////////////////////////
//Æú´õ °ü·Ã
	bool		IsFolder( CString sfile );				//Æú´õÀÎÁö ÆÄÀÏÀÎÁö
	bool		isFolder( char *sfile );
	//ÆÄÀÏ¸íÀÌ³ª Æú´õ¸í¿¡ '\\', '/' È¥¿ëÀÏ °æ¿ì°¡ ÀÖÀ¸¹Ç·Î CStringÀÇ '==' ¿¬»êÀÚ·Î ºñ±³ÇØ¼± ¾ÈµÈ´Ù. 
	bool IsFileFolderPathIsEqual(CString file0, CString file1, bool bCaseSensitive = false);
	CString		GetParentDirectory( CString sFolder );	//ÇöÀç Æú´õÀÇ »óÀ§ Æú´õ¸íÀ» ¸®ÅÏÇÑ´Ù.

	//compare_only_filename : fullpath·Î Á¤·ÄÇÒÁö, ÆÄÀÏ¸í¸¸ ÃßÃâÇØ¼­ Á¤·ÄÇÒÁö. default = false;
	void		sort_like_explorer(std::deque<CString> *dq, bool compare_only_filename = false);
	//ÁöÁ¤µÈ Æú´õÀÇ ÆÄÀÏ ¸ñ·ÏÀ» ¾ò¾î¿Â´Ù.
	//sNameFilterÀÇ ¿ÍÀÏµåÄ«µå´Â Á÷Á¢ Áà¼­ °Ë»öÇØ¾ß ÇÑ´Ù.
	//ÇÁ·ÒÇÁÆ® ¸í·É°ú µ¿ÀÏÇÏ°Ô ¹°À½Ç¥³ª º°Ç¥¿Í °°Àº ¿ÍÀÏµåÄ«µå¸¦ ÀÌ¿ëÇÒ ¼ö ÀÖ´Ù.
	//sNameFilter = "test*", sExtFilter = "jpg;bmp;" ¿Í °°ÀÌ ÀÔ·ÂÇÏ¸é
	//test·Î ½ÃÀÛÇÏ°í È®ÀåÀÚ°¡ jpg, bmpÀÎ ÆÄÀÏ ¸ñ·ÏÀ» ¾ò¾î¿Â´Ù.
	//sExceptStr = "test;temp;error" ¿Í °°ÀÌ ¼¼¹ÌÄİ·ĞÀ¸·Î ±¸ºĞÇÏ¿© °Ë»ö Á¦¿ÜÇÒ ÆÄÀÏ¸í ÁöÁ¤ °¡´É.
	//ÁÖÀÇ! dqFiles´Â ÀÌ ÇÔ¼ö¿¡ ÀÇÇØ ÃÊ±âÈ­µÇÁö ¾ÊÀ¸¹Ç·Î ÇÊ¿äÇÑ °æ¿ì ÃÊ±âÈ­ÇÏ¿© È£ÃâÇÒ °Í!
	void		FindAllFiles(	CString sFolder, std::deque<CString> *dqFiles,
								CString sNameFilter = _T("*"), CString sExtFilter = _T("*"),
								bool bRecursive = false, CString sExceptStr = _T(""),
								bool auto_sort = true);
	//stdc++17ÀÇ std::filesystemÀ» ÀÌ¿ëÇÑ ÇÔ¼ö·Î¼­
	//FindAllFiles°¡ recursive function¿¡´Ù°¡ ¿É¼ÇÀÌ ¸¹´Ù°í´Â ÇØµµ
	//debug mode¿¡¼­ 8700°³ÀÇ ÆÄÀÏÀ» Ã£´Âµ¥(D:\1.project\0.backup)
	//25,047ms VS 76msÀÇ ¼ÓµµÂ÷ÀÌ°¡ ³­´Ù.(+auto_sort=268ms)
	//32,105°³ÀÇ ÆÄÀÏ : 1,773,564ms VS 3,337ms
	//release mode¿¡¼­´Â
	//9,715ms VS 21ms.
#if (_MSVC_LANG >= _std_cpp17)	//__cplusplus ¸ÅÅ©·Î¸¦ »ç¿ëÇÏ·Á¸é C/C++ÀÇ °í±ŞÃ¢¿¡¼­ /Zc:__cplusplus¸¦ Ãß°¡½ÃÄÑ¾ß ÇÑ´Ù.
	std::deque<CString>	find_all_files(CString path, CString name_filter = _T(""), CString ext_filters = _T(""), CString except_str = _T(""), bool recursive = true, bool auto_sort = true);
#endif
	void save_dqlist(std::deque<CString>* dqlist, CString path);

	//À§ÀÇ FindAllFiles¿¡¼­´Â "ÆÄÀÏ¸í*"°ú °°ÀÌ Ã£°Ô µÇ´Âµ¥ ÀÌ·² °æ¿ì ½Ã¸®Áî ÀÌ¸§ÀÇ ´Ù¸¥ ÆÄÀÏµéµµ ¸ğµÎ Ã£¾ÆÁø´Ù.
	//µû¶ó¼­ ÆÄÀÏ¸íÀº È®Á¤µÇ°í È®ÀåÀÚ¸¸ ¿©·¯°¡ÁöÀÎ °æ¿ì´Â ¾Æ·¡ ÇÔ¼ö¸¦ ÀÌ¿ëÇØ¾ß ÇÑ´Ù.
	//(a.jpg, a.pngµîÀ» Ã£°í ½ÍÀºµ¥ a1.jpg°¡ ÀÖ¾îµµ true°¡ µÇ±â ¶§¹®¿¡)
	//(a 00.jpg ~ a 99.jpg = a ??.jpg·Î Ã£À» ¼ö ÀÖ´Ù. Áï, "Å¸ÀÌÆ² ??"ÀÌ°í ?¿¡ ¼ıÀÚ°¡ ¿À¸é ´Ù Ã£¾ÆÁà¾ßÇÒµíÇÑµ¥ ¿ì¼± ÆĞ½º)
	std::deque<CString>		FindFilesWithExtensions(CString folder, CString fileTitle, CString extensions);

	// Æú´õÀÇ ¸ğµç ÆÄÀÏÀ» Áö¿î´Ù.
	int			DeleteAllFiles( CString sFolder, CString sFilter, bool bRecursive = TRUE, bool bResetCount = TRUE );
	bool		DeleteFolder( LPCTSTR lpFolder );
	bool		SHDeleteFolder( CString sFolder );


	//Ç®ÆĞ½º¸¦ ÁÖ¸é Æú´õ¸¦ ÀÚµ¿À¸·Î ¸¸µé¾îÁØ´Ù.
	bool		recursive_make_full_directory(LPCTSTR sFolder);
	bool		make_full_directory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpsa = NULL);

	//Æú´õ³»ÀÇ Æ¯Á¤ ¹®ÀÚ¿­ÀÌ µé¾î°£ ÆÄÀÏµéÀ» Áö¿î´Ù. ÇÏÀ§Æú´õ Áö¿ø¾ÈÇÔ.
	void		DeleteFilesBySubString( CString sFolder, CString filenameSubStr, bool bMatchWholeWordOnly = FALSE, bool bMatchCase = FALSE );

	//void		DeleteAllFiles( CString sFolder );

	//°­Á¦ »èÁ¦ °ü·Ã ÇÔ¼öµé
	//bool		EnableDebugPrivilege( void );
	//bool		CloseRemoteFileHandles( LPCTSTR );
	//DWORD		CloseRemoteHandle( LPCTSTR lpProcessName, DWORD processID, HANDLE handle );
	//bool		DeleteTheFile( LPCTSTR lpFileName );

	//Æú´õ¼±ÅÃ ´ëÈ­»óÀÚ¸¦ ¿¬´Ù.
	//strSelectedFolder : ¼±ÅÃÇÑ Æú´õ
	//strStartFolder : ±âº» ¼±ÅÃµÉ Æú´õ. ½ÃÀÛ Æú´õ.
	bool BrowseForFolder(HWND hwndOwner, TCHAR* lpszTitle, CString& strSelectedFolder, const TCHAR* strStartFolder = NULL, bool bNewFolderButton = true );
	static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM lpData);

	//À©µµ¿ì ¿î¿µÃ¼Á¦¿¡¼­ Æ¯Á¤ Æú´õÀÇ ½ÇÁ¦ °æ·Î¸¦ ¸®ÅÏÇÑ´Ù.
	//nFolder = https://docs.microsoft.com/ko-kr/windows/win32/shell/csidl
	//http://blog.naver.com/PostView.nhn?blogId=sobakmt&logNo=60058711792&widgetTypeCall=true
	CString get_special_folder(int nFolder);

//////////////////////////////////////////////////////////////////////////
//³×Æ®¿öÅ©, ÀÎÅÍ³İ
	bool		GetNICAdapterList( IP_ADAPTER_INFO* pAdapters, int& nTotal, int nMax = 10 );
	void		GetNetworkInformation(TCHAR* sFindDescription, NETWORK_INFO* pInfo );
	bool		CheckInternetIsOnline();
	int			get_char_count( CString sStr, TCHAR ch );
	bool		IsAvailableEMail( CString sEMail );


//////////////////////////////////////////////////////////////////////////
//¾ÏÈ£È­
	//¹®ÀÚ¿­À» ¾ÏÈ£È­ÇÑ´Ù.
	void		EncryptString( CString& ToCode, TCHAR* key );
	//¾ÏÈ£È­µÈ ¹®ÀÚ¿­À» º¹È£È­ÇÑ´Ù.
	void		DecryptString( CString& ToCode, TCHAR* key );
	//¼ıÀÚ·Î ±¸¼ºµÈ ¹®ÀÚ¿­À» ÀÔ·Â¹Ş¾Æ Á¤ÇØÁø ¿¬»êÀ» ÇÑ ÈÄ µÚ¼¯ÀÎ ¼ıÀÚ¹®ÀÚ¿­À» ¸®ÅÏÇÑ´Ù.
	CString		ShuffleNumericString( CString sSrc, bool bSameLength = true );

	//ÆÄÀÏÀ» ¾ÏÈ£È­ÇÑ´Ù.
	bool		FileEncryption( CString sfile, bool bShowErrorMessage = TRUE );


//////////////////////////////////////////////////////////////////////////
//½©, À©µµ¿ì, ·¹Áö½ºÆ®¸®, ½Ã½ºÅÛ
	CString		GetComputerNameString();
	HWND		GetHWndByExeFilename( CString sExeFile, bool bCaseSensitive = false, bool bExceptThis = true );
	CWnd*		FindWindowByCaption(CString sCaption, bool bMatchWholeWord = FALSE );
	HINSTANCE	FindExecutableEx( LPCTSTR lpFile, LPCTSTR lpDir, LPTSTR lpResult );

	LONG		IsExistRegistryKey(HKEY hKeyRoot, CString sSubKey);
	LONG		GetRegistryValue(HKEY hKeyRoot, CString sSubKey, CString sEntry, int *value);
	LONG		GetRegistryString(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString *str);
	LONG		SetRegistryValue(HKEY hKeyRoot, CString sSubKey, CString sEntry, int value);
	LONG		SetRegistryString( HKEY hKeyRoot, CString sSubKey, CString sEntry, CString str);
	double		GetProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double default);
	bool		WriteProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double value);

	void		SystemShutdown( int nMode );		// 0:logoff  1:reboot  2:shutdown
	void		SystemShutdownNT( int nMode = 2 );	// 1:reboot  2:shutdown

	void		HideTaskBar( bool bHide = TRUE );	// FALSE => Show Taskbar
	BOOL		Is64BitWindows();
	void		ClickMouse( int x, int y );

	//appÀÇ À§Ä¡¿Í Å©±â¸¦ ·¹Áö½ºÆ®¸®¿¡ ÀúÀåÇÏ°í ½ÇÇà ½Ã ´Ù½Ã º¹¿ø½ÃÅ²´Ù.
	//resize°¡ ¾ÈµÇ´Â ´ÙÀÌ¾ó·Î±×ÀÇ °æ¿ì´Â resize½ÃÅ°Áö ¾Ê°í ¿ø·¡ Å©±â·Î Ç¥½ÃÇØ¾ß ÇÑ´Ù.
	void		RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""), bool use_maximize = true, bool resize_window = true);
	void		SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""));


	//CPU Usage °ü·Ã
	LPBYTE		GetPerformanceData(LPTSTR src);
	int			GetCounterValue(const int& objId, const int& counterId, const char* instanceName, PERF_DATA_BLOCK **dataBlock, LONGLONG &value);
	double		GetCpuUsage(const char* process);

	//¸Ş¸ğ¸®, memory
	SIZE_T		GetCurrentMemUsage();
	INT			IsAvailableMemory(LPVOID pMemoryAddr);


/*
src		: ¼Ò½º µ¥ÀÌÅÍ
srcx	: ¼Ò½º ½ÃÀÛ x ÁÂÇ¥
srcy	: ¼Ò½º ½ÃÀÛ y ÁÂÇ¥
srcw	: ¼Ò½º ÀüÃ¼ width(pixel)
srch	: ¼Ò½º ÀüÃ¼ height(pixel)
dst		: Å¸°Ù µ¥ÀÌÅÍ
dstx	: Å¸°Ù x ÁÂÇ¥
dsty	: Å¸°Ù y ÁÂÇ¥
dstw	: Å¸°Ù ÀüÃ¼ width(pixel)
dsth	: Å¸°Ù ÀüÃ¼ height(pixel)
w		: º¹»çÇÒ width Å©±â(pixel)
h		: º¹»çÇÒ height Å©±â(pixel)
*/
bool		memcpy_block( uint8_t *src, int srcx, int srcy, int srcw, int srch, uint8_t *dst, int dstx, int dsty, int dstw, int dsth, int w, int h, int ch );
bool		memcpy_block( uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, int ch, uint8_t *dst );

//src¿¡¼­ roi ¿µ¿ªÀ» Àß¶ó¼­ dst_width * dst_height Å©±â·Î resize ½ÃÅ²´Ù.
bool		resize_roi( uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, uint8_t *dst, int dst_width, int dst_height );

//HDD
LONGLONG	GetDiskFreeSize( CString sDrive );
LONGLONG	GetDiskTotalSize( CString sDrive );
CString		GetDiskSizeString( CString sDrive );	// "1.25G / 380.00G"
//CString		GetHDDSerialNumber( int nPhysicalDrive );
CString		GetHDDVolumeNumber( CString sDrive );


//ÆÄ¶ó¹ÌÅÍ·Î µé¾î¿Â ¿¬¼ÓµÈ ÆÄÀÏ¸íµéÀ» ºĞ¸®ÇÑ´Ù. ½ÇÇàÆÄÀÏ¸íÀº Á¦¿ÜµÊ.(ex. command line or shell command)
void		ParseCommandString( CString sParam, CStringArray& ar );

//ÁöÁ¤ÇÑ ÀÌ¹ÌÁö¸¦ ¹ÙÅÁÈ­¸é¿¡ Ç¥½ÃÇÑ´Ù.
void		SetWallPaper(CString sfile );

//´ÜÃà¾ÆÀÌÄÜÀ» ¸¸µé¾îÁØ´Ù.
HRESULT		MyCreateShortCut( LPCTSTR pszSrcFile, LPCOLESTR pszLnkFile,
							 LPTSTR pszWorkingDir = NULL, LPTSTR pszArgument = NULL, LPTSTR pszDesc = NULL );
HRESULT		CreateShortCut(LPCTSTR pszShortcutFile, LPCOLESTR pszLink,
							LPCTSTR pszWorkingDir = _T(""), LPCTSTR pszArgument = _T(""), LPCTSTR pszDesc = _T("") );

//printer
int			GetPrinterList( CStringArray *arPrinter );
CString		GetDefaultPrinterName();
CSize		GetPrinterPaperSize( CString sPrinterName );

CString		get_error_message(DWORD errorId, bool show_msgBox);

//////////////////////////////////////////////////////////////////////////
//½Ã°£
	CString		GetDateStringFromTime( CTime t, CString sMark = _T("-") );
	CString		GetDateStringFromTime( COleDateTime t, CString sMark = _T("-") );
	CString		GetDateStringFromTime( __timeb32 t, CString sMark = _T("-") );
	CString		GetTimeStringFromTime( CTime t, CString sMark = _T(":") );
	CString		GetTimeStringFromTime( COleDateTime t, CString sMark = _T(":") );
	CString		GetTimeStringFromTime( __timeb32 t, CString sMark = _T(":") );
	CString		GetTimeString( CTime t, bool bSeparator = true );
	CString		GetTimeString( COleDateTime t, bool bSeparator = true );
	CString		GetTimeString( __timeb32 t, bool bSeparator = true, bool bUnderline = false, bool bHasMilliSec = true );	//2003-04-16 18:01:00.120
	CString		GetCurrentTimeString( bool bSeparator = true, bool bUnderline = false, bool bHasMilliSec = true );	//2003-04-16 18:01:00.120
	CTime		GetTimeFromTimeString( CString sDate, CString sTime );
	CTimeSpan	GetTimeSpanFromTimeString(CString sTime);
	CString		GetDateTimeStringFromTime( CTime t, bool bSeparator = true );
	CString		GetDateTimeStringFromTime( COleDateTime t, bool bSeparator = true );
	CTime		GetTimeFromDateTimeString( CString sDateTime);
	CString		GetTimeStringFromSeconds( double dSecond, bool bHasHour = true, bool bHasMilliSec = false );
	CString		GetTimeStringFromMilliSeconds(int ms, bool bHasHour = true, bool bHasMilliSec = true);
	int			GetSecondsFromTimeString(CString timeString);
	int			GetMilliSecondsFromTimeString(CString timeString);
	void		GetTimeFromSeconds( int nTotalSeconds, int &nHours, int &nMinutes, int &nSeconds );
	//type 0(date), 1(time), 2(date+time), ³â-¿ù-ÀÏ ½Ã:ºĞ:ÃÊ Çü½ÄÀ¸·Î ÇöÀç ½Ã°£ ¸®ÅÏ. blank´Â ³¯Â¥¿Í ½Ã°£ »çÀÌ °ø¹é ¿©ºÎ
	CString		GetCurrentDateTimeString( int nType = 2, bool bSeparator = true, TCHAR mid_char = ' ');
	void		SetSystemTimeClock( WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMinute, WORD wSecond );
	double		GetElapsedTime( __timeb32 pOldTime );	//pOldTime°ú ÇöÀç ½Ã°£ÀÇ Â÷ÀÌ °è»ê
	//ts°ªÀ» ³Ñ°Ü ¹Ş¾Æ "aÀÏ b½Ã°£ cºĞ dÃÊ" ÇüÅÂ·Î Ç¥½Ã
	CString		GetDayTimeCountString( CTimeSpan ts, bool bShowZero, bool bIncludeSec );
	//ts°ªÀ» ³Ñ°Ü ¹Ş¾Æ "aÀÏ b½Ã°£ cºĞ dÃÊ" ÇüÅÂ·Î Ç¥½Ã
	CString		GetDayTimeCountString( COleDateTimeSpan ts, bool bShowZero, bool bIncludeSec );
	time_t		_mkgmtime(const struct tm *tm) ;
	bool		IsAM( CTime t = 0 );	//t=0ÀÌ¸é ÇöÀç½Ã°¢±âÁØ, 0º¸´Ù Å©¸é ±× ½Ã°£°ª ±âÁØ
	CString		GetDayOfWeekString(CTime t = NULL, bool short_str = false);
	int			GetDaysOfMonth( int nYear, int nMonth );	//ÇØ´ç ´ŞÀÇ ³¯Â¥¼ö ¸®ÅÏ
	//³¯Â¥ °ü·Ã
	int			GetSeasonIndex();	//º½=0, ¿©¸§=1...
	int			gettimeofday(struct timeval *tv, struct timezone *tz);

	//³¯Â¥ Ç¥½Ã Çü½Ä¿¡ ¸Â´ÂÁö °Ë»ç
	bool		is_valid_date(CString str);
	//½Ã°£ Ç¥½Ã Çü½Ä¿¡ ¸Â´ÂÁö °Ë»ç
	bool		is_valid_time(CString str);

//Å¸ÀÌ¸Ó °ü·Ã
	void		Wait( DWORD dwMillisecond );	//WM_TIMER ¸Ş½ÃÁö ÇÚµé·¯ ³»¿¡¼­´Â »ç¿ëÇÒ ¼ö ¾øÀ½.
	//void		usleep(int microSec);
	void		ProcessWindowMessage();			//¹İº¹¹®¿¡ ÀÇÇØ process°¡ ÀÀ´ä¾øÀ½ÀÌ µÇÁö ¾Êµµ·Ï ¹İº¹¹®¾È¿¡¼­ È£ÃâÇÏ¿© ¸Ş½ÃÁöÅ¥ÀÇ ³»¿ëÀ» ¹Ù·Î Ã³¸®½ÃÅ²´Ù.
	int			ptimer_start(int instance);		//reset high resolution time lcounter 
	double		ptimer_get_time(int instance);	//get time(double precision) elaspsed since timer reset in ms
	//unit : ms
	long		getClock();


/*
//////////////////////////////////////////////////////////////////////////
//»ö»ó
	COLORREF	get_color( CString sColor );
	COLORREF	get_color( COLORREF crOrigin, int nOffset );
	//0~9±îÁö ¹Ì¸® Á¤ÇØ³õÀº ±âº» »ö»óÀ» ¸®ÅÏÇÑ´Ù.
	COLORREF	GetDefaultColor( int idx );
	//random19937À» ÀÌ¿ëÇÏ¿© ·£´ı ÄÃ·¯¸¦ ¸®ÅÏÇÑ´Ù.
	COLORREF	get_random_color();
	bool		IsHexaColorString(CString str);
	COLORREF	get_color_from_hexa_string(CString str);
	COLORREF	GetComplementaryColor( COLORREF crColor, COLORREF crBack = RGB(255,255,255) );
	void		RGB2HSL(int r, int g, int b, int& h, int& s, int& l);

	//32ºñÆ®ÀÎ dw¿¡ µé¾îÀÖ´Â R, G, B¸¦ ÃßÃâÇÏ¿© 16ºñÆ®(5+6+5) ÄÃ·¯·Î ¸®ÅÏÇÑ´Ù.
	WORD		RGB24ToRGB565( DWORD dw );
	//µÎ DWORD¸¦ WORD·Î º¯È¯ÇÏ¿© ÇÏ³ªÀÇ DWORD·Î º¯È¯ÇÑ´Ù.
	DWORD		RGB24ToRGB565( DWORD rgb1, DWORD rgb2 );
	//565·Î º¯È¯µÈ WORD¸¦ ´Ù½Ã 24ºñÆ® RGB·Î »ç¿ëÇÏ±â À§ÇØ DWORD·Î º¹¿ø½ÃÅ²´Ù.
	DWORD		RGB565ToRGB24( WORD wd );

	//gray °è¿­ÀÎÁö
	bool		isGray(COLORREF cr, int tolerance = 0);
	//gray image¸¦ 3Ã¤³Î ¶Ç´Â 4Ã¤³Î ÀÌ¹ÌÁö Æ÷¸ËÀ¸·Î º¹»çÇØÁØ´Ù.
	//dst´Â ¹İµå½Ã ÇÒ´çµÈ ¸Ş¸ğ¸®ÁÖ¼ÒÀÌ¾î¾ß ÇÑ´Ù.
	void gray2color( uint8_t *gray, int gray_width, int gray_height, uint8_t *dst, int dst_width, int dst_height, int dst_ch, uint8_t alpha );

	//3Ã¤³Î ¶Ç´Â 4Ã¤³Î ÀÌ¹ÌÁö¸¦ ´ÜÀÏ Èæ¹é ÀÌ¹ÌÁö·Î º¹»çÇÑ´Ù.
	//dst´Â ¹İµå½Ã ÇÒ´çµÈ ¸Ş¸ğ¸®ÁÖ¼ÒÀÌ¾î¾ß ÇÑ´Ù.
	void color2gray( uint8_t *src, int src_width, int src_height, int src_ch, uint8_t *dst, int dst_width, int dst_height );
*/
	extern bool		initialized_YUV_lookup_table;
	void		init_YUV_lookup_table();
	void		yuv420_yv12_to_bgr( unsigned char *src, unsigned char *dst, int w, int h );
	//cv::Mat		yuv420_yv12_to_bgr( uchar *pBuffer,long bufferSize, int width,int height );
	void		yuv420_nv12_to_gray( unsigned char* src, unsigned char* dst, int width, int height);
	void		yuv420_nv12_to_bgr( unsigned char* src, unsigned char* dst, int width, int height);
	void		yuv422_uyvy_to_bgr( unsigned char *src, unsigned char *dst, int w, int h );
	void		yuv_yuyv_to_bgr( unsigned char* src, unsigned char* dst, int w, int h );
	void		bgr_to_yuv422( uint8_t* src, uint8_t* dst, int width, int height, bool yuyv );	//yuyv(true) or uyuv(false)
	//convert 1ch gray to yuv_plain_gray(YYYY....128 128..128 128..)
	void		gray_to_yuv_plain_gray( uint8_t *src, uint8_t *dst, int width, int height );
	void		bgr_to_hsv( uint8_t* src, uint8_t* dst, int width, int height );


//////////////////////////////////////////////////////////////////////////
//GDI
	void		TextOutShadow(CDC* pDC, int x, int y, CString sText, COLORREF crText = RGB(0,0,0), COLORREF crShadow = GRAY(64), UINT nFlag = TA_LEFT | TA_TOP );
	void		DrawTextShadow(CDC* pDC, CString sText, CRect r, UINT format, COLORREF crText = RGB(255,255,255), COLORREF crShadow = GRAY(64), int offsetX = 1, int offsetY = 1 );
	void		TextOutOutline(CDC* pDC, int x, int y, CString sText, COLORREF crText = RGB(0,0,0), COLORREF crBorder = RGB(255,255,255), UINT nFlag = TA_LEFT | TA_TOP );
	void		DrawTextOutline(CDC* pDC, CString sText, CRect r, UINT format, COLORREF crText = RGB(255, 255, 255), COLORREF crShadow = GRAY(64));
	void		draw_center_text(CDC* pdc, const CString& strText, CRect& rcRect);
	//20220914 DrawLine°ú DrawLinePt¸¦ °°Àº ÀÌ¸§À¸·Î ÇÏ´Ï ¸ğÈ£ÇÏ´Ù´Â ¿¡·¯°¡ ¹ß»ıÇÏ¿© DrawLinePt·Î º¯°æ.
	void		DrawLine(CDC* pDC, int x1, int y1, int x2, int y2, COLORREF crColor = 0, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN );
	void		DrawLinePt(CDC* pDC, CPoint pt1, CPoint pt2, COLORREF crColor = 0, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN );
	void		DrawRectangle( CDC*	pDC, CRect Rect, COLORREF crColor = RGB(0,0,0), COLORREF crFill = NULL_BRUSH, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN );
	void		DrawSunkenRect( CDC* pDC, CRect Rect, bool bSunken = TRUE, COLORREF cr1 = GRAY(96), COLORREF cr2 = GRAY(128), int nWidth = 1 );
	void		DrawEllipse(CDC* pDC, int cx, int cy, int rx, int ry, COLORREF crLine, COLORREF crFill, int nPenStyle = PS_SOLID, int nWidth = 1, int nDrawMode = R2_COPYPEN);
	void		drawCircle( CDC* pDC, int xMidPoint,  int yMidPoint,  int radius);
	void		draw_polygon(CDC* pDC, std::vector<CPoint> pts, bool closed = true, COLORREF crLine = 0, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		drawArc( CDC *pDC, double cx, double cy,double r1, double r2, double start, double end, int width = 1, int style = PS_SOLID, COLORREF cr = 0, int mode = R2_COPYPEN );
	bool		LoadBitmapFromFile( CBitmap &bmp, CString strFile );
	bool		SaveBitmapToTile( CBitmap* bmp, CString strFile, CWnd* pWnd );
	bool		SaveRawDataToBmp( CString sBmpFile, BYTE* pData, int w, int h, int ch );
	HANDLE		DDBToDIB( CBitmap* bitmap, DWORD dwCompression, CPalette* pPal );
	HICON		LoadIconEx(HINSTANCE hInstance, UINT nID, int cx, int cy = 0);

	//font size to LOGFONT::lfHeight
	LONG		get_logical_size_from_font_size(HDC hDC, int font_size);
	//LOGFONT::lfHeight to font size
	LONG		get_font_size_from_logical_size(HDC hDC, int logical_size);


//gradient_fillÀ» À§ÇØ¼­ ¼±¾ğµÈ ÀÌ ÇÚµéÀ» »ç¿ëÇÏ´Â ÇÁ·Î±×·¥ÀÌ¶ó¸é
//Á¾·áµÉ ¶§ ÇØÁ¦½ÃÄÑÁÖ´Â ÇÔ¼öµµ ¹İµå½Ã È£ÃâÇØÁà¾ß ÇÑ´Ù.
	typedef UINT (CALLBACK* LPFNDLLFUNC1)(HDC,CONST PTRIVERTEX,DWORD,CONST PVOID,DWORD,DWORD);
	extern HINSTANCE	g_hInst_msimg32;
	extern LPFNDLLFUNC1 g_dllfunc_GradientFill;
	void		gradient_rect(CDC* pDC, CRect &rect, std::deque<COLORREF> dqColor, bool vertical);
	enum GRADIENT_RECT_PRESET
	{
		gradient_rect_white_black_white = 0,
		gradient_rect_white_gray128_white,
		gradient_rect_black_white_black,
		gradient_rect_black_gray128_black,
	};
	void		gradient_rect(CDC* pDC, CRect &rect, int preset, bool vertical);
	void		safe_release_gradient_rect_handle();

//ÀÌ¹ÌÁö°¡ Ç¥½ÃµÇ°í ÀÖ´Â ¿µ¿ª Á¤º¸¿Í È­¸é»óÀÇ ÁÂÇ¥¸¦ ÁÖ¸é ÀÌ¹ÌÁö»óÀÇ ½ÇÁ¦ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
//´Ü, °è»êµÈ ÀÌ¹ÌÁö»óÀÇ ½ÇÁ¦ ÁÂÇ¥°¡ ÀÌ¹ÌÁö Å©±â¸¦ ¹ş¾î³ª¸é °á°ú º¯¼ö¿¡´Â -1°ªÀ» Ã¤¿ö¼­ ¸®ÅÏÇÑ´Ù.
	void		GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, double *x, double *y);
	void		GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, CPoint *pt);
	void		GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, CRect *r);

	//ÀÌ¹ÌÁö°¡ Ç¥½ÃµÇ°í ÀÖ´Â ¿µ¿ª Á¤º¸¿Í ÀÌ¹ÌÁö »óÀÇ ÁÂÇ¥¸¦ ÁÖ¸é È­¸é»óÀÇ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
	void		GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, double *x, double *y);
	void		GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, CPoint *pt);
	void		GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, CRect *r);


//Á÷¼±, Line °ü·Ã ÇÔ¼ö

	//¿øÁ¡¿¡¼­ º¤ÅÍ b°¡ º¤ÅÍ aÀÇ ¹İ½Ã°è ¹æÇâÀÌ¸é ¾ç¼ö, ½Ã°è¹æÇâÀÌ¸é À½¼ö, ÆòÇàÀÌ¸é 0À» ¹İÈ¯ ÇÑ´Ù.
	double ccw(vector2 a, vector2 b);

	//Á¡ p¸¦ ±âÁØÀ¸·Î º¤ÅÍ b°¡ º¤ÅÍ aÀÇ ¹İ½Ã°è ¹æÇâÀÌ¸é ¾ç¼ö, ½Ã°è¹æÇâÀÌ¸é À½¼ö, ÆòÇàÀÌ¸é 0À» ¹İÈ¯ ÇÑ´Ù.
	double ccw(vector2 p, vector2 a, vector2 b);

	// - Á¡ a, b¸¦ Áö³ª´Â Á÷¼±°ú Á¡ c, d¸¦ Áö³ª´Â Á÷¼±ÀÇ ±³Á¡À» x¿¡ ¹İÈ¯ÇÑ´Ù.
	// - µÎ Á÷¼±ÀÌ ÆòÇàÀÌ¸é(°ãÄ¡´Â °æ¿ì Æ÷ÇÔ) °ÅÁşÀ», ¾Æ´Ï¸é ÂüÀ» ¹İÈ¯ÇÑ´Ù.
	bool		GetIntersectionPoint( vector2 a, vector2 b, vector2 c, vector2 d, vector2& x );
	bool		GetIntersectionPoint( CPoint* pt, CPoint* ispt );
	bool		GetIntersectionPoint( int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int& isx, int& isy );

	//4Á¡À¸·Î ±¸¼ºµÈ µÎ ¼±ºĞÀÌ ¼­·Î ±³Â÷ÇÏ´ÂÁö ¿©ºÎ¸¦ ÆÇº°ÇÑ´Ù.
	//bCheckCrossOver = falseÀÌ¸é (0,1)°ú (2,3)¶óÀÎÀÇ ±³Â÷¿©ºÎ¸¸, trueÀÌ¸é (0,3)°ú (1,2)ÀÇ ±³Â÷¿©ºÎ±îÁö ÆÇº°ÇÑ´Ù.
	bool		IsIntersect2LineSegment( vector2 a, vector2 b, vector2 c, vector2 d );
	bool		IsIntersect2LineSegment( CPoint* pt, bool bCheckCrossOver );

	//Á¡ a, b¿Í Á¡ c, d°¡ ÆòÇàÇÑ µÎ ¼±ºĞ ÀÏ ¶§ ÀÌµéÀÌ ÇÑ Á¡¿¡¼­ °ãÄ¡´ÂÁö È®ÀÎÇÑ´Ù.
	bool		paralleSegments(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p);

	// - p°¡ µÎ Á¡ a, b¸¦ °¨½Î¸é¼­ °¢ º¯ÀÌ x, yÃà¿¡ ÆòÇàÇÑ ÃÖ¼Ò»ç°¢Çü ³»ºÎ¿¡ ÀÖ´ÂÁö È®ÀÎÇÑ´Ù.
	// a, b, p´Â ÀÏÁ÷¼± »ó¿¡ ÀÖ´Ù°í °¡Á¤ÇÑ´Ù.
	bool		inBoundingRectangle(vector2 p, vector2 a, vector2 b);

	// - µÎ Á¡ a, b¸¦ Áö³ª´Â ¼±ºĞ°ú µÎ Á¡ c, b¸¦ Áö³ª´Â ¼±ºĞÀ» p¿¡ ¹İÈ¯ÇÑ´Ù.
	// - ±³ÁüÀÌ ¿©·¯°³ÀÏ °æ¿ì ¾Æ¹«Á¡ÀÌ³ª ¹İÈ¯ÇÑ´Ù.
	bool		segmentIntersection(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p);



	//µµÇü, »ï°¢Çü
	//»ï°¢Çü ¿ÜÁ¢¿øÀÇ Áß½ÉÀ» ±âÇÏÇĞÀûÀ¸·Î ±¸ÇÑ´Ù.(http://kipl.tistory.com/113)
	int circumCenter(CPoint A, CPoint B, CPoint C, double *xc, double *yc);
	//»ï°¢Çü ¿ÜÁ¢¿øÀÇ Áß½ÉÀ» ´ë¼öÀûÀ¸·Î ±¸ÇÑ´Ù.
	int circumCenter2(CPoint P, CPoint Q, CPoint R, double *xc, double *yc);
	//»ï°¢Çü ¿ÜÁ¢¿øÀÇ ¹İÁö¸§À» ±¸ÇÑ´Ù.
	double circumRadius(CPoint A, CPoint B, CPoint C);


//»ç°¢Çü
	//»ç°¢Çü Á¤º¸¸¦ ¹®ÀÚ¿­·Î ¸®ÅÏÇÑ´Ù.
	//0 : "1 2 3 4"
	//1 : "(1,2) x (3,4)"
	//2 : "l = 1, t = 2, r = 3, b = 4"
	CString		GetRectInfoString( CRect r, int nFormat );

	void		make_rect(CRect &Rect, int x, int y, int w, int h );
	CRect		make_rect(int x, int y, int w, int h);
	CRect		makeCenterRect(int cx, int cy, int w, int h );
	Gdiplus::Rect makeCenterGpRect(int cx, int cy, int w, int h);
	CRect		GpRect2CRect(Gdiplus::Rect);
	Gdiplus::Rect	CRect2GpRect(CRect r);
	Gdiplus::RectF	CRect2GpRectF(CRect r);
	CRect		getCenterRect( int cx, int cy, int w, int h );
	CRect		get_zoom_rect(CRect rect, double zoom);
	//0:lt, 1:rt, 2:rb, 3:lb, rb_cutÀÌ trueÀÌ¸é ³¡Á¡-1ÀÎ °ªÀ» ¸®ÅÏÇÏ°í falseÀÌ¸é ³¡Á¡ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
	CPoint		vertex(CRect r, int index, bool rb_cut = false);	

	//ÁÖ¾îÁø »ç°¢Çü ¹üÀ§¸¦ ¹ş¾î³ªÁö ¾Êµµ·Ï º¸Á¤ÇØÁØ´Ù.
	void		adjustRectRange( int32_t *l, int32_t *t, int32_t *r, int32_t *b, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool retainSize );
	//ÀÌ¹ÌÁöÀÇ °æ¿ì includeBRÀº false·Î ÇØ¾ß ³¡Á¡ ÁÂÇ¥°¡ À¯È¿ÇÏ´Ù.
	void		AdjustRectRange( CRect& rect, CRect rLimit, bool bRetainSize = true, bool includeBR = false );
	void		AdjustRectRange( CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize = true );

	//¸ğ´ÏÅÍÀÇ ÇÑÂÊ¿¡ ºÙÀº »ç°¢ÇüÀ» »õ·Î¿î Å©±â·Î º¯°æÇÒ °æ¿ì ºÙÀº »óÅÂ¸¦ À¯ÁöÇÏ°í º¯°æÇÒ ÇÊ¿ä°¡ ÀÖÀ» °æ¿ì »ç¿ë.
	void		adjust_with_monitor_attached(CRect rOld, CRect &rNew);

	//rTarget¿¡ Á¢ÇÏ´Â dRatio¸¦ À¯ÁöÇÏ´Â ÃÖ´ë »ç°¢ÇüÀ» ±¸ÇÑ´Ù.
	CRect		GetRatioRect(CRect rTarget, double dRatio, int attach = attach_hcenter | attach_vcenter);
	CRect		GetRatioRect(CRect rTarget, int w, int h, int attach = attach_hcenter | attach_vcenter);
	//rSub°¡ rMain¿¡ ¿ÏÀüÈ÷ ¼ÓÇØÀÖÀ¸¸é true¸¦ ¸®ÅÏÇÑ´Ù.
	bool		RectInRect( CRect rMain, CRect rSub );
	//r¿¡¼­ except¿µ¿ªÀ» Á¦¿ÜÇÏ°í crÄÃ·¯·Î Ã¤¿î´Ù.
	void		fill_except_rect(CDC* pDC, CRect r, CRect except, COLORREF cr);
	//µÎ »ç°¢ÇüÀÇ °ãÄ¡´Â ¿µ¿ªÀ» ¸®ÅÏÇÑ´Ù.
	CRect		getIntersectionRect( CRect r1, CRect r2 );
	CRect		getIntersectionRect( int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2 );

	//µÎ »ç°¢ÇüÀÌ °ãÄ¡´Â Á¤µµ¸¦ r1À» ±âÁØÀ¸·Î °è»êÇØ¼­ ¸®ÅÏÇÑ´Ù.
	double		getOverlappedRatio( CRect r1, CRect r2 );
	double		getOverlappedRatio( int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2 );

//side ¹è¿­ÀÇ ÀÎµ¦½º´Â resizeÇÏ´Â ¿µ¿ª ÀÎµ¦½º·Î¼­
//DefWindowProcÀÇ µÎ¹øÂ° ÆÄ¶ó¹ÌÅÍ¿¡ (SC_SIZE + m_nSideIndex)·Î ¾²ÀÌ¹Ç·Î ±× Â÷·Ê¸¦ µû¸¥´Ù.
	enum CORNER_INDEX
	{
		corner_inside = 0,
		corner_left,
		corner_right,
		corner_top,
		corner_topleft,
		corner_topright,
		corner_bottom,
		corner_bottomleft,
		corner_bottomright,
	};

	void		GetSideRect( CRect src, CRect *side, int margin );
	//src»ç°¢ÇüÀÇ marginÅ©±âÀÇ Å×µÎ¸® ¿µ¿ª¿¡ ptÁ¡ÀÌ Á¸ÀçÇÏ´Â ¿µ¿ªÀÇ ÀÎµ¦½º¸¦ ¸®ÅÏÇÑ´Ù.
	//ÀÎµ¦½º´Â CORNER_INDEXÀÇ Â÷·ÊÀÌ¸ç ÀÌ´Â DefWindowProc¿¡¼­ »ç¿ëÇÏ´Â Â÷·Ê¿Í µ¿ÀÏÇÏ´Ù.
	int			get_corner_index(CRect src, CPoint pt, int margin);

	//½ÃÀÛÁ¡À» ÁÖ¸é Á¤»ç°¢ÇüÀ» ÀÌ·ç´Â ³¡Á¡ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
	void		getSquareEndPoint(int sx, int sy, int& ex, int& ey);

//´Ù°¢Çü polygon °ü·Ã
	//ÀÓÀÇ Á¡ÀÌ Æú¸®°ï ³»¿¡ Á¸ÀçÇÏ´ÂÁö ÆÇº°
	bool PtInPolygon( CPoint* ptPolygons, CPoint pt, int nCorners );
	bool PtInPolygon1( CPoint* ptPolygons, CPoint pt, int nCorners );
	bool PtInPolygon2( CPoint* ptPolygons, CPoint pt, int nCorners );
	bool PtInPolygon3( CPoint *ptPolygons, CPoint pt, int nCorners );
	//´Ù°¢ÇüÀÇ ³ĞÀÌ¸¦ ±¸ÇÑ´Ù. ´Ü, º¯ÀÌ ÇÏ³ª¶óµµ ±³Â÷µÇ¸é ¼º¸³ÇÏÁö ¾Ê´Â´Ù.
	double		GetPolygonAreaSize( CPoint *pt, int nPoints );
	//ÁÖ¾îÁø ´Ù°¢Çü Á¡µéÀ» Æ÷ÇÔÇÏ´Â ÃÖ´ë »ç°¢ÇüÀ» ±¸ÇÑ´Ù.
	CRect		get_max_rect(CPoint	*pt, int nPoints);
	CRect		get_max_rect(std::vector<CPoint> pt, int pt_max = -1);


//region °ü·Ã
	HRGN		BitmapToRegion (HBITMAP hBmp, COLORREF cTransparentColor/* = 0*/, COLORREF cTolerance/* = 0x101010*/);
	HRGN		BitmapRegion( HBITMAP hBitmap, COLORREF cTransparentColor, bool bIsTransparent);
	HRGN		CreateRgnFromBitmap(HBITMAP hBmp, COLORREF color);

//Ä¸ÃÄ ±â´É
	//rÀº À©µµ¿ì ÁÂÇ¥°è.
	CImage*		capture_window(CRect r, CString filename);
	HBITMAP		CaptureScreenToBitmap(LPRECT pRect);
	HBITMAP		CaptureWindowToBitmap(HWND hWnd, LPRECT pRect = NULL);
	HBITMAP		CaptureClientToBitmap(HWND hWnd, LPRECT pRect = NULL);
	void		WriteBMP(HBITMAP bitmap, HDC hDC, LPTSTR filename);



//Å°º¸µå ¾ğ¾î¸¦ ±× ³ª¶ó ±âº»¾ğ¾î·Î º¯°æÇÑ´Ù.
void		IME_Convert_To_NativeCode( HWND hWnd, bool bNative );


//¹®ÀÚÀÔ·ÂÃ¢À» ¼û±ä´Ù.
void		HideIMM( HWND hwnd );


bool		IsLeapYear( int nYear );	//À±³âÀÎÁö ÆÇ´Ü

//CRichEditCtrlEx¸¦ »ç¿ëÇÏ¿© AppendToLogÇÔ¼ö¸¦ ÀÌ¿ëÇÏ´Â ¾ÛÀº ·Î±×¸¦ UI¿¡ Ç¥½ÃÇÏ±â°¡ ÁÁÀ¸³ª
//CRichEditCtrlExÀ» ÀÌ¿ëÇÏÁö ¸øÇÏ´Â »óÈ²ÀÇ ¾ÛÀ¸·Î °ü·Ã ÄÚµåµéÀ» ÀçÀÌ¿ëÇÏ·Á¸é ÄÚµå ¼öÁ¤ÀÌ ÇÊ¿äÇÏ´Ù.
//µû¶ó¼­ ¾Æ·¡ ÇÔ¼ö¸¦ ÀÌ¿ëÇÑ´Ù.
//void		AppendToLog(CWnd* pWnd, )



//UI control °ü·Ã
UINT		getButtonStyle(HWND hWnd);	//buttonÀÇ Á¾·ù¸¦ ¸®ÅÏÇÑ´Ù.

//¿¬¼ÓµÈ ¹öÆ°µé¿¡ ´ëÇÑ ÀÏ°ı Ã³¸®¿ë ÇÔ¼ö
//±âº» CheckRadioButton°°Àº °æ¿ì´Â unselect ±â´ÉÀ» Á¦°øÇÏÁö ¾Ê±â ¶§¹®¿¡
//¾Æ·¡ ÇÔ¼ö¸¦ »õ·ÎÀÌ Á¤ÀÇÇØ¼­ »ç¿ëÇÑ´Ù.
//id_offsetÀÌ 0º¸´Ù ÀÛÀ¸¸é first ~ last±îÁö ¸ğµÎ Àû¿ë.
void		CheckRadioButtons(CWnd *pWnd, int idFirst, int idLast, int id_offset, int nCheck = BST_CHECKED);

//dialog based¿¡¼­ Å°ÀÔ·ÂÀ¸·Î µ¿ÀÛÀ» Á¤ÀÇÇÏ´Âµ¥ CEdit°ú °°Àº ÀÔ·ÂÃ¢¿¡ Æ÷Ä¿½º°¡ ÀÖÀ¸¸é
//PreTranslateMessage¿¡¼­ ¹æÇâÅ°³ª charÅ°¸¦ Ã³¸®ÇÏ±â°¡ °ï¶õÇÏ´Ù.
//µû¶ó¼­ ÇöÀç Æ÷Ä¿½º¸¦ °¡Áø ÄÁÆ®·ÑÀÌ CEditÀÌ°í enableÀÌ°í readonly°¡ ¾Æ´Ñ °æ¿ì¿¡´Â
//PreTranslateMessage¿¡¼­ ÀÔ·ÂµÈ Å°¸¦ Ã³¸®ÇÏµµ·Ï ÇÑ´Ù.
bool		IsEditCtrlAcceptKeyState( CWnd *pWnd );

//start	: ½ÃÀÛ ÀÎµ¦½º.
//end	: Á¤·ÄÀ» ¿øÇÏ´Â n¹øÂ° Ç×¸ñ
//ex. quicksort( data, 9, 2 );¸¦ È£ÃâÇÏ¸é
//2¹ø ÀÎµ¦½ººÎÅÍ 9¹øÂ° Ç×¸ñÀÎ data[2] ~ data[8]±îÁöÀÇ µ¥ÀÌÅÍ°¡ Á¤·ÄµÈ´Ù.
//º¸Åë n°³ÀÇ µ¥ÀÌÅÍ¸¦ Á¤·ÄÇÑ´Ù¸é quicksort(data, n); ÀÌ¶ó È£ÃâÇÏ¸é µÈ´Ù.
template<class T> void quicksort( T& v, int end, int start = 0, bool bAscending = true )
{
	while( end > start )
	{
		int i = start;
		int j = end;

		do
		{
			if ( bAscending )
			{
				while( (v[i] < v[start]) && (i < j) )
					i++;

				//ÇÔ¼ö ¿øÇüÀ» quicksort( T *v, ...)¿Í °°ÀÌ Á¤ÀÇÇØ¼­ »ç¿ëÇÏ¸é ¾Æ·¡ ¹®Àå¿¡¼­ µğ¹ö±ë ¿¡·¯°¡ ¹ß»ıÇÑ´Ù.
				while( v[--j] > v[start] )
					;
			}
			else
			{
				while( (v[i] > v[start]) && (i < j) )
					i++;
				while( v[--j] < v[start] )
					;
			}

			if ( i < j )
				swap( v[i], v[j] );
		} while( i < j );

		swap( v[start], v[j] );

		if ( j - start > end - (j + 1) )
		{
			quicksort( v, j - 1, start, bAscending );
			start = j + 1;
		}
		else
		{
			quicksort( v, end, j + 1, bAscending );
			end = j - 1;
		}
	}
}

/*
template<class T> void quickSort(T *a, const int& leftarg, const int& rightarg);
	{
  if (leftarg < rightarg) {

    T pivotvalue = a[leftarg];
    int left = leftarg - 1;
    int right = rightarg + 1;

  for(;;) {

    while (a[--right] > pivotvalue);
    while (a[++left] < pivotvalue);

    if (left >= right) break;

    T temp = a[right];
    a[right] = a[left];
    a[left] = temp;
  }

  int pivot = right;
  quickSort(a, leftarg, pivot);
  quickSort(a, pivot + 1, rightarg);
  }
}
*/

/* templateÇÔ¼öÀÇ Æ¯Â¡
- µ¥ÀÌÅÍ Å¸ÀÔ¿¡ ¹«°üÇÏ°Ô ÇÔ¼ö ÇÏ³ª·Î Ä¿¹ö°¡ µÇÁö¸¸
  µ¥ÀÌÅÍ Å¸ÀÔ¸¶´Ù ÇÔ¼ö ¹Ùµğ°¡ objÄÚµå·Î »ı¼ºµÇ´Â ´ÜÁ¡ÀÌ ÀÖ´Ù°í ¾Ë°íÀÖ´Ù.
  ¶ÇÇÑ vsÀÇ debug¸ğµå¿¡¼­´Â ¼Óµµ ÀúÇÏ°¡ ¹ß»ıÇÑ´Ù.
  release¸ğµå¿¡¼­´Â ¼Óµµ ÀúÇÏ°¡ ¹ß»ıÇÏÁö ¾Ê´Â´Ù.
*/

//Ä¡È¯ ÇÔ¼ö
template<class T> void SWAP( T& x, T& y )
{
	T temp	= x;
	x		= y;
	y		= temp;
}

//Å¬¸®ÇÎ ÇÔ¼ö. Å¬¸®ÇÎÀÌ ÀÏ¾î³ª¸é true¸¦ ¸®ÅÏÇÑ´Ù.
template<class T> bool Clamp( T &n, T min, T max )
{
	if (max < min)
		SWAP(min, max);

	if ( n < min )
	{
		n = min;
		return true;
	}
	else if ( n > max )
	{
		n = max;
		return true;
	}

	return false;
}

//¹üÀ§ ¼øÈ¯ ÇÔ¼ö
template<class T> void Cycle( T& n, T min, T max )
{
	if ( n < min ) n = max;
	else if ( n > max ) n = min;
}

//¹üÀ§¸¦ ¹ş¾î³ª¸é default°ªÀ¸·Î ¼¼ÆÃ
template<class T> void Validate( T& n, T min, T max, T default )
{
	if ( n < min || n > max )
		n = default;
}

//Ä¡È¯ ÇÔ¼ö
template<class T> void Swap( T& x, T& y )
{
	T temp	= x;
	x		= y;
	y		= temp;
}

int compareInteger (const void * a, const void * b);
int compareChar( const void *arg1, const void *arg2 );
int compareString (const void * a, const void * b);


//////////////////////////////////////////////////////////////////////////
//¼öÇĞ
double		tangentfunc(double x, double y);
//µÎ Á¡ÀÇ °¢µµ¸¦ ±¸ÇÑ´Ù. screencoordÀÏ¶§¿Í Cartesian coordinate(Á÷±³ÁÂÇ¥°è)ÀÏ¶§´Â y°¡ ¹İ´ëÀÓ¿¡ ÁÖÀÇ.
double		GetAngle( double vx, double vy, bool bScreenCoord = true );
double		GetAngle( double x1, double y1, double x2,  double y2, bool bScreenCoord = true );
double		GetAngle( CPoint pt0, CPoint pt1, bool bScreenCoord = true );
double		GetAngle( CPoint a, CPoint b, CPoint c );	//3Á¡ÀÌ ÀÌ·ç´Â °¢µµ
CPoint		GetCenterPoint( CPoint pt0, CPoint pt1 );
double		GetDistance( CPoint pt0, CPoint pt1 );

//µÎ Á¡À» Áö³ª´Â Á÷¼±»óÀÇ x3 ¶Ç´Â y3¸¦ ±¸ÇÑ´Ù.
double		getLinePointX( double x1, double y1, double x2, double y2, double y3 );
double		getLinePointY( double x1, double y1, double x2, double y2, double x3 );
double		GetManhattanDistance( double x1, double y1, double x2, double y2 );
//ptCenter¸¦ ±âÁØÀ¸·Î dAngle ¸¸Å­ È¸ÀüµÈ dDist°Å¸®ÀÇ Á¡ÀÇ ÁÂÇ¥¸¦ ±¸ÇÑ´Ù.
CPoint		GetRotatedPoint( CPoint ptCenter, double dAngle, double dDist );
//cx, cy¸¦ Áß½ÉÀ¸·Î tx, tyÁ¡ÀÌ degree¸¦ È¸ÀüÇÒ °æ¿ì tx, tyÁ¡ÀÇ º¯°æ ÁÂÇ¥
void		get_rotated(int cx, int cy, int* tx, int* ty, double degree);
std::vector<CPoint>	get_rotated(int cx, int cy, CRect* r, double degree);

//Áöµµ ÁÂÇ¥ <-> µµºĞÃÊ º¯È¯
double		convert_gps_coord(int d, int m, double s);
void		convert_gps_coord(double gps, int &d, int &m, double &s);

//http://www.gamedevforever.com/114 (2012³â Æ÷½ºÆ®)
//±âÁ¸ rand()¿¡ ºñÇØ ºĞÆ÷°¡ °í¸£°í ¼Óµµµµ ºü¸£´Ù°í ¼Ò°³µÇ¾úÀ¸³ª
//Á¤ÀÛ ½ÇÇèÇØº¸´Ï ºĞÆ÷´Â À¯»çÇÏ°í ¼Óµµ´Â rand()°¡ 2¹è ´õ »¡¶ú´Ù.
//release mode, 1000000°³ÀÇ SetPixel, GetTickCount()·Î ½Ã°£Ã¼Å©.
template<typename T> inline T random19937(T min, T max)
{
	std::random_device rd;
	std::mt19937 mt(rd());
	if (typeid(T) == typeid(float) ||
		typeid(T) == typeid(double) ||
		typeid(T) == typeid(long double))
	{
		std::uniform_real_distribution<double> dist((double)min, (double)max);
	return dist(mt);
}

	std::uniform_int_distribution<int> dist((int)min, (int)max);
	return dist(mt);
	/*
	std::mt19937 engine((unsigned int)time(NULL));       // MT19937 ³­¼ö ¿£Áø
	std::uniform_int<> distribution(min, max);                     // »ı¼º ¹üÀ§
	std::variate_generator<std::mt19937, std::uniform_int<>> generator(engine, distribution);
	return generator();
	*/
}
/*
template<typename T> T random19937(T min, T max)
{
	srand((unsigned int)time(NULL));
	return rand();
}
*/
template<typename T> double standardDeviation(std::deque<T> v) 
{
	T sum = std::accumulate(v.begin(), v.end(), 0.0);
	double mean = sum / v.size();

	double squareSum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
	return sqrt(squareSum / v.size() - mean * mean);
}

double		Rounding( double x, int digit );
//´ë°¢¼±ÀÇ ±æÀÌ·Î °¡·Î, ¼¼·Î Å©±â¸¦ ±¸ÇÑ´Ù.
void		get_HV_angle_from_diagonal( double diagonal, double *h, double *v, double width, double height );
void		get_HV_angle_from_diagonal( int diagonal, int *h, int *v, int width, int height );

//numlock, capslock, scrolllock
bool		GetLockKeyState( BYTE nLockKey );
void		SetLockKeyState( BYTE nLockKey, bool bOn );

HBITMAP		MakeDIBSection(CDC& dc, int width, int height);

//2D ´ÜÀÏ ¿µ»ó¿¡¼­ ÀÌ¹Ì ¾Ë·ÁÁø ¼³Á¤°ªÀ» ±âÁØÀ¸·Î ¿µ»ó³»ÀÇ ÇÑ Á¡°ú ·»Áî¿ÍÀÇ °Å¸®¸¦ °è»ê(by sucwon)
//cam_height	: Ä«¸Ş¶ó ¼³Ä¡ ³ôÀÌ. ´ÜÀ§ cm
//fl_x, fl_y	: focal length
//c_x, c_y		: ÁÖÁ¡
double		getObjectDistance(	int width, int height, int vanishing_y, int x, int y, int cam_height, double *dx, double *dy,
								int cali_width = 1920, int cali_height = 1080,
								double fl_x = 2361.130, double fl_y = 2357.436 );

//¿øº»º¸´Ù Å©°Ô resize´Â ºÒ°¡ÇÔ.
void		resize_image(uint8_t *source_ptr,
						int source_width, 
						int source_height, 
						uint8_t *destination_ptr, 
						int destination_width, 
						int destination_height);


//resize_bilinear´Â ncnn¿¡¼­ °¡Á®¿ÔÀ¸³ª ¹º°¡ ¸Ş¸ğ¸® ¿¡·¯°¡ ¹ß»ıÇÑ´Ù.
//ÀÏ´Ü »ç¿ëÇÏÁö ¾Ê´Â´Ù.
//void		resize_bilinear_c1(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h);
//void		resize_bilinear_c3(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h);
//void		resize_bilinear_c4(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h);
//void		resize_bilinear_image(uint8_t *src, int sw, int sh, uint8_t *dst, int w, int h);
void resize11(int* input, int* output, int sourceWidth, int sourceHeight, int targetWidth, int targetHeight);

void		gaussian_blur(uint8_t *image, int width, int height);

double		scv_image_mean( uint8_t *src, int width, int height );

//minValue : minimum value for thresholding.
void		scv_image_threshold( uint8_t *src, int width, int height, int threshold, int minValid = 0, bool invert = false );

void		scv_absdiff( uint8_t *src1, uint8_t *src2, uint8_t *dst, int w, int h );

bool		scv_subImage(uint8_t* src, uint8_t* dst, int source_width, int source_height, int startx,int starty, int cut_width, int cut_height, int ch);

int			scv_countNonZero(uint8_t *src, int w, int h);

//ÇöÀç´Â 1Ã¤³Î ¿µ»ó¸¸ Áö¿øµÈ´Ù.
//¿ì¼± ±ŞÇÏ°Ô -90µµ¸¸ ±¸ÇöÇÑ´Ù.
//dst´Â ¹İµå½Ã ¸Ş¸ğ¸®°¡ ÇÒ´çµÇ¾î ÀÖ¾î¾ß ÇÑ´Ù.
void		rotate90( uint8_t *src, int width, int height, uint8_t *dst, int degree );

//opencvÀÇ flip°ú °°Àº µ¿ÀÛÀÌÁö¸¸ ÀÌ¸§ Ãæµ¹À» ÇÇÇÏ±â À§ÇØ mirror¶ó´Â ÀÌ¸§À» »ç¿ëÇÔ.
//ÇöÀç´Â 1Ã¤³Î ¿µ»ó¸¸ Áö¿øµÈ´Ù.
//method : 0(flip vertical), +(flip horizontal), -(both)
//dst´Â ¹İµå½Ã ¸Ş¸ğ¸®°¡ ÇÒ´çµÇ¾î ÀÖ¾î¾ß ÇÑ´Ù.
void		mirror( uint8_t *src, int width, int height, uint8_t *dst, int method );




//Æ¯Á¤ ¸ñÀûÀÇ ÇÔ¼öµé
//Â÷·®Á¤º¸ÆÄÀÏ(xml) ÀúÀå °ü·Ã
bool		SavePlateInfoFile( char* sfile, char* sPlate, RECT* rect = NULL );

void		printMessage(std::string msg, uint8_t bNewLine = true);

int readFilenames(std::vector<std::string> &filenames, const std::string &directory);

class CPlateCode
{
public:
	char	sLocalCode[5];
	char	sTypeCode[3];
	char	sUseCode[3];
	char	sDigitCode[5];

	CPlateCode( char* sPlate )
	{
		strcpy_s( sLocalCode, "\0" );
		strcpy_s( sTypeCode, "\0" );
		strcpy_s( sUseCode, "\0" );
		strcpy_s( sDigitCode, "\0" );

		//plate = °æ±â86³ª9993
		//local	= °æ±â
		//type	= 86
		//use	= ³ª
		//digit	= 9993
		if ( strlen( sPlate ) < 8 )
			return;
		else if ( strlen( sPlate ) == 8 )
		{
			strcpy_s( sLocalCode, "\0" );
			strncpy_s( sTypeCode, sPlate, 2 );
			sTypeCode[2] = '\0';
		}
		else
		{
			strncpy_s( sLocalCode, sPlate, 4 );
			sLocalCode[4] = '\0';

			if ( strlen( sPlate ) == 11 )
			{
				strncpy_s( sTypeCode, &(*(sPlate + 4)), 1 );
				sTypeCode[1] = '\0';
			}
			else
			{
				strncpy_s( sTypeCode, &(*(sPlate + 4)), 2 );
				sTypeCode[2] = '\0';
			}
		}

		strncpy_s( sUseCode, &(*(sPlate + strlen(sPlate) - 6)), 2 );
		sUseCode[2] = '\0';
		strncpy_s( sDigitCode, &(*(sPlate + strlen(sPlate) - 4)), 4 );
		sDigitCode[4] = '\0';
	}
};
#endif