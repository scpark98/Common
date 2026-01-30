#pragma once

//#ifdef DPSAPI_VERSION
//#undef DPSAPI_VERSION
//#endif
//
//#define DPSAPI_VERSION 1

//#ifndef _SCPARK_FUNCTIONS_H
//#define _SCPARK_FUNCTIONS_H

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
#include <map>
#include <algorithm>
#include <set>
#include <ostream>
#include <sstream>

#ifndef _USING_V110_SDK71_
	#include <d2d1_1.h>
	#include <wrl/client.h>
	#include <d2d1effects_2.h>
#endif

#include "GdiplusBitmap.h"

#include "colors.h"
#include "data_structure/SCParagraph/SCParagraph.h"

#include <WinInet.h>
#include <specstrings.h>


#define _std_cpp11 201103L
#define _std_cpp14 201402L
#define _std_cpp17 201703L
#define _std_cpp20 202002L

//ÇÁ·ÎÁ§Æ®°¡ À¯´ÏÄÚµå ¹®ÀÚ ÁıÇÕÀ» »ç¿ëÇÒ ¶§ fopen()½Ã UTF-8·Î ÀúÀåÇÏ°í
//¸ÖÆ¼¹ÙÀÌÆ® ¹®ÀÚ ÁıÇÕÀÌ¸é ANSI·Î ÀúÀåµÇµµ·Ï ÇÑ´Ù.
//´Ü, UTF-8·Î ÀúÀå½Ã BOM ¹®ÀÚ°¡ Çì´õ¿¡ ±â·ÏµÇ¹Ç·Î ÆÄÀÏÀ» ¾²±â¿ëÀ¸·Î »ı¼ºÇÑ ÈÄ
//fseek¸¦ ÀÌ¿ëÇØ¼­ Çì´õ¸¦ ¹«½ÃÇØ¾ß¸¸ ÇØ´ç ÆÄÀÏÀÌ UTF-8 without BOMÀ¸·Î »ı¼ºµÈ´Ù.
//_tfopen_s(&fp, m_droppedFolder + _T("\\filelist.lst"), _T("wt")CHARSET);
//fseek(fp, 0L, SEEK_SET);
#ifdef UNICODE
#define CHARSET _T(",ccs=UTF-8")
#define __function__ __FUNCTIONW__
#else
#define CHARSET _T("")
#define __function__ __FUNCTION__
#endif

#ifndef _S
	#define _S load_string
#endif

#ifdef _MSC_VER
#define __class_func__ __function__
#endif

//20231101 opencv¿¡ trace°¡ ÀÌ¹Ì Á¤ÀÇµÇ¾î ÀÖ¾î¼­ trace¸¦ Trace·Î º¯°æÇÔ.
//¸ÅÅ©·Î·Î Á¤ÀÇµÇ¾î ±×·±Áö °£È¤ ºñÁ¤»óÀûÀ¸·Î Ãâ·ÂµÇ´Â Çö»óÀÌ ÀÖ´Ù.
//ÀÏ´Ü, thread ³»ºÎ¿¡¼­ »ç¿ëÇÏ´Â ÀÏºÎ ÇÁ·ÎÁ§Æ®¿¡¼­´Â ¿À·ù°¡ ¹ß»ıÇÏ¹Ç·Î ÁÖÀÇÇÒ °Í.
//https://stackoverflow.com/questions/3211463/what-is-the-most-efficient-way-to-make-this-code-thread-safe
#define Trace(fmt, ...) trace_output(false, __function__, __LINE__, false, fmt, ##__VA_ARGS__)
#define Traceln(fmt, ...) trace_output(false, __function__, __LINE__, true, fmt, ##__VA_ARGS__)
#define Trace_only(fmt, ...) trace_output(true, __function__, __LINE__, true, fmt, ##__VA_ARGS__)
#define Trace_func() trace_output(false, __function__, __LINE__, false, _T("%s\n"), __function__)

#define traceonly TRACE(_T("%s(%d) current clock = %ld\n"), __function__, __LINE__, GetTickCount64());

inline std::basic_ostream<TCHAR>& operator<<(std::basic_ostream<TCHAR>& os, const CString& s)
{
	return os << (LPCTSTR)s;
}

inline std::basic_ostream<TCHAR>& operator<<(std::basic_ostream<TCHAR>& os, LPCTSTR s)
{
	if (!s)
		return os << _T("(null)");
	return os.write(s, _tcslen(s));
}

inline std::basic_ostream<TCHAR>& operator<<(std::basic_ostream<TCHAR>& os, const CPoint& pt)
{
	return os << _T("(") << pt.x << L", " << pt.y << _T(")");
}

inline std::basic_ostream<TCHAR>& operator<<(std::basic_ostream<TCHAR>& os, const CRect& r)
{
	return os << _T("(") << r.left << _T(", ") << r.top << _T(", ") << r.right << _T(", ") << r.bottom << L")";
}

template<typename T> void trace_impl(const TCHAR* func, int line, const TCHAR* var_name, const T& value)
{
	std::basic_ostringstream<TCHAR> oss;

	oss << func << _T("(") << line << _T(")") << _T(" ") << var_name << L" = " << value;
	TRACE(_T("%s\n"), oss.str().c_str());
}

//trace(n);À¸·Î È£ÃâÇÏ¸é "n = %d\n"¸¦ Ãâ·ÂÇÑ´Ù. Å¸ÀÔ¿¡ µû¶ó %d, %f, %s µîÀÌ ÀÚµ¿À¸·Î °áÁ¤µÈ´Ù.
//Á¤¼ö, ½Ç¼ö, ¹®ÀÚ¿­, CPoint, CRect¸¦ Áö¿øÇÏ¸ç ±× ¿Ü Å¸ÀÔÀº << ¿¬»êÀÚ¸¦ ¿À¹ö·ÎµùÇØ¼­ Áö¿øÇÒ ¼ö ÀÖ´Ù.
#define trace(n) trace_impl(__function__, __LINE__, L#n, n)

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

#define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))

//À§ÀÇ µ¥ÀÌÅÍ Å¸ÀÔÀ» #define uchar unsigned char °ú °°ÀÌ ¼±¾ğÇÏ¸é
//´Ù¸¥ defineµé°ú Ãæµ¹ÇÑ´Ù. ¶ÇÇÑ ¾Æ·¡¿Í °°Àº ÀÌÀ¯·Î¶óµµ
//Å¸ÀÔÀÇ Á¤ÀÇ´Â #define ´ë½Å typedefÀ» ÀÌ¿ëÇÏÀÚ.
/*
typedef int *t1;
#define t2 int *

t1 a, b; // a is 'int*' and b is 'int*'
t2 c, d; // c is 'int*' and d is 'int'
*/

#ifdef GDIPVER
	#undef GDIPVER
#endif

#define		GDIPVER 0x0110

#define		VAR_TO_STRING(a) #a
#define		VAR_TO_CSTRING(a) CString(#a)
#define		ID2String(ID, sID) sID.Format("%s", #ID)	

#define		Swap32BE(x) ((((x) & 0xff000000) >> 24) | (((x) & 0xff0000) >> 8) | (((x) & 0xff00) << 8) | (((x) & 0xff) << 24))
#define		Swap16BE(x) ((((x) & 0xff00) >> 8)| (((x) & 0xff) << 8))

//¹è¿­ÀÇ °¹¼ö ¸®ÅÏ
#define		countof(array) (sizeof(array)/sizeof(array[0]))

#define		MESSAGE_DOWNLOAD_DATA	WM_APP + 1000

#define		SYSTEM_LOGOFF			0
#define		SYSTEM_REBOOT			1
#define		SYSTEM_POWEROFF			2

#define		IsShiftPressed() (0x8000 ==(GetKeyState(VK_SHIFT) & 0x8000))
#define		IsCtrlPressed()  (0x8000 ==(GetKeyState(VK_CONTROL) & 0x8000))

#define		CLIP(x) ((x) > 255 ? 255 : (x) < 0 ? 0 : x)
#define		check_range_return(x, lower, upper) {if ((x) < (lower) || (x) > (upper)) return;}

#define		RECT_RESIZE_HANDLE_COUNT	9
#define		RECT_RESIZE_HANDLE_SIZE		3

#define lengthof(rg) (sizeof(rg)/sizeof(*rg))

inline const TCHAR* StringFromError(TCHAR* szErr, long nSize, long nErr)
{
	_ASSERTE(szErr);
	*szErr = 0;
	DWORD cb = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, nErr, 0, szErr, nSize, 0);
	TCHAR szUnk[] = _T("<unknown>");
	if (!cb && nSize >= lengthof(szUnk)) lstrcpy(szErr, szUnk);
	return szErr;
}

inline HRESULT TraceHR(const TCHAR* pszFile, long nLine, HRESULT hr)
{
	if (FAILED(hr))
	{
		TCHAR szErr[128];
		TCHAR sz[_MAX_PATH + lengthof(szErr) + 64];
		wsprintf(sz, _T("error ==> %s(%d) : 0x%x: %s\n"), pszFile, nLine, hr,
			StringFromError(szErr, lengthof(szErr), hr));
		OutputDebugString(sz);
	}
	return hr;
}

#ifdef _DEBUG
#define TRACEHR(_hr) TraceHR(__function__, __LINE__, _hr)
#else
#define TRACEHR(_hr) _hr
#endif

struct CStringHash
{
	size_t operator() (const CString&s) const noexcept
	{
#ifdef _UNICODE
		return std::hash<std::wstring>{}(std::wstring(s.GetString()));
#else
		return std::hash<std::string>{}(std::string(s.GetString()));
#endif
	}
};



//HRESULT¸¦ °á°ú·Î ¸®ÅÏ¹Ş´Â ÇÔ¼öµé¿¡ »ç¿ë.
//#define		_M(exp) (([](HRESULT hr) { if (FAILED(hr)) /*_com_raise_error(hr);*/ return hr; })(exp));
//hr°ú ÇÔ¼ö½ÄÀ» ³Ñ°ÜÁÖ¸é hr¿¡ ±× Ã³¸®°á°ú°¡ µé¾î°¡°í ¿¡·¯ÀÏ °æ¿ì ¿¡·¯¸¦ Ç¥½ÃÇÑ´Ù.
#define		_M(hr, exp) { if (FAILED(hr = (exp))) TRACEHR(hr); }

typedef void (WINAPI* PGNSI)(LPSYSTEM_INFO);
typedef BOOL(WINAPI* PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

//±âÁ¸ g_monitors´Â ¸ğ´ÏÅÍ ¿µ¿ª¸¸ ÀúÀåÇßÀ¸³ª Á» ´õ ÇÊ¿äÇÑ Á¤º¸°¡ ÀÖ¾î ÀÌ¸¦ class·Î È®ÀåÇÔ
class CSCMonitorInfo
{
public:
	CSCMonitorInfo(MONITORINFOEX *_mi, HMONITOR _handle)
	{
		memcpy(&mi, _mi, sizeof(MONITORINFOEX));
		handle = _handle;
		rMonitor = mi.rcMonitor;
		rWork = mi.rcWork;
	}

	MONITORINFOEX	mi;
	HMONITOR		handle;
	CRect			rMonitor;	//mi.rcMonitor°¡ LPRECT¶ó¼­ »ç¿ë ÆíÀÇ¼ºÀ» À§ÇØ Ãß°¡
	CRect			rWork;
};

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
	text_encoding_ansi = CP_ACP,	//= EUC-KR
	text_encoding_utf7 = CP_UTF7,
	text_encoding_utf8 = CP_UTF8,
	text_encoding_utf8_bom,
	text_encoding_utf16be,
	text_encoding_utf16be_bom,
	text_encoding_utf16le,
	text_encoding_utf16le_bom,
	text_encoding_utf32be,
	text_encoding_utf32be_bom,
	text_encoding_utf32le,
	text_encoding_utf32le_bom,
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

#define		FILE_EXTENSION_IMAGE			_T("bmp;jpg;jpeg;png;webp;gif;yuv;jfif;avif")
#define		FILE_EXTENSION_SOUND			_T("mp3;m4a;wav")
#define		FILE_EXTENSION_VIDEO			_T("avi;mpg;mp4;mpeg;mkv;mov;wmv;wma;asf;ts;m2ts;3gp")
#define		FILE_EXTENSION_MEDIA			CString(FILE_EXTENSION_VIDEO) + _T(";") + CString(FILE_EXTENSION_IMAGE) + _T(";") + CString(FILE_EXTENSION_SOUND)
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
#define		DEFAULT_RANGE(x, min, max, default_value) (((x) > max || (x) < min) ? default_value : (x))
#endif

//¼Ò¼öÁ¡ n+1 ÀÚ¸®¿¡¼­ ¹İ¿Ã¸²ÇÏ¿© ¼Ò¼öÁ¡ nÀÚ¸®·Î Ç¥ÇöÇÑ´Ù.
//ex. ROUND(0.545, 2) = 0.55
//ex. ROUND(0.545, 1) = 0.50
//ex. ROUND(0.545, 0) = 1.00
#define		ROUND(x, n) (floor((x) * pow(10.0, (n)) + 0.5f) / pow(10.0, (n)))
#define		SQR(x)		((x)*(x))
#define		PI				3.141592
#define		RADIAN(x)		(x) * PI / 180.0	//degree to radian
#define		DEGREE(x)		(x) * 180. / PI
#define		DISTANCE(x1,y1,x2,y2)	sqrt((double)(((x2)-(x1)) * ((x2)-(x1)) + ((y2)-(y1)) * ((y2)-(y1))))

#define		IsHexaChar(c) ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))

#define		KByte		1024.0
#define		MByte		KByte * 1024.0
#define		GByte		MByte * 1024.0
#define		TByte		GByte * 1024.0

#ifndef WIDTHSTEP4	
#define		WIDTHSTEP4(bits)	(((bits) + 31) / 32 * 4)	//bits is not width, but (width * bitCount)
#endif

//numº¸´Ù Å« nÀÇ ¹è¼ö·Î ¸¸µé¾îÁØ´Ù.	//old : MAKE4WIDTH_D()µµ MAKE_MULTIPLY_U()¸¦ ÀÌ¿ëÇÒ °Í!
#define		MAKE_MULTIPLY_UP(num, n)		(((num) + ((n)-1)) & ~((n)-1))
//numº¸´Ù ÀÛÀº nÀÇ ¹è¼ö·Î ¸¸µé¾îÁØ´Ù.
#define		MAKE_MULTIPLY_DOWN(num, n)		(((num) - ((n)-1)) & ~((n)-1) | (n))

extern		int			g_nDaysOfMonth[12];

//°¢ ¾ğ¾î¿¡ »ç¿ëµÇ´Â ÁÖ¼®¸¶Å©
extern		std::deque<CString>		g_comment_mark;

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


#define SAFE_RELEASE(pObject) { if (pObject != NULL) { pObject->Release(); pObject = NULL; } }

#define SAFE_DELETE(pData) { try { delete pData; } catch (...) { ASSERT(FALSE); } pData = NULL; } 
#define SAFE_FREE(pData) { try { free(pData); } catch (...) { ASSERT(FALSE); } pData = NULL; } 

#define SAFE_CLOSE_HANDLE(hHandle) { if (hHandle!=NULL) { CloseHandle(hHandle); hHandle = NULL; } }

#define SAFE_DELETE_ARRAY(pData) { try { delete [] pData; } catch (...) { ASSERT(FALSE); } pData = NULL; } 

//////////////////////////////////////////////////////////////////////////////////
#define SHELL_OPEN(String)	ShellExecute(NULL, TEXT("open"), String, NULL, NULL, SW_SHOWNORMAL);

#define MAX_REG_KEY_LENGTH	255
#define MAX_REG_VALUE_NAME	16383

#define MAP_STYLE(src, dest) if(dwStyle & (src)) dwText |= (dest)
#define NMAP_STYLE(src, dest) if(!(dwStyle & (src))) dwText |= (dest)

#define extract_bits(data, area, loc) (((data)>>(loc)) & (area))
#define make_area(area, length)\
{\
   int32_t i = 0;\
   int32_t bit = 1;\
   for (i = 0; i < (length-1); i++){ \
      bit = ((bit<<1)|1);\
   }\
   area = bit;\
}

void printf_string(const char* psz, ...);
void trace_output(bool only_text, TCHAR* func, int line, bool linefeed, LPCTSTR format, ...);

//trace(m_input);À» È£ÃâÇÏ¸é TRACE(_T("m_input = %d\n"));°ú °°ÀÌ µ¿ÀÛÇÑ´Ù.
//template <typename T> void trace(T args)
//{
//	CString name;
//	//= VAR_TO_CSTRING(args);
//	ID2String(args, name);
//	TRACE(_T("%s = %d\n"), name, args);
//}

template < typename T > class AutoEraser
{
private:
	T * VarPtr;
public:
	template < typename T2 >
	AutoEraser(T2& ptr)    {    VarPtr = (T*)ptr;   }
	~AutoEraser()           {    delete[] VarPtr;    }
};

struct timezone  
{ 
  int  tz_minuteswest; /* minutes W of Greenwich */ 
  int  tz_dsttime;     /* type of dst correction */ 
}; 


//https://exiv2.org/tags.html
//https://learn.microsoft.com/en-us/windows/win32/wic/-wic-native-image-format-metadata-queries#exif-metadata
class CSCEXIFInfo
{
public:
	CString camera_make;
	CString camera_model;
	CString software;
	CString image_description;
	CString image_copyright;
	CString original_datetime;
	CString last_modified_datetime;
	double exposure_time = 1.0;
	double exposure_bias = 0.0;
	double f_number = 0.0;				//"
	unsigned short iso_speed = 0;
	char flash = 0;
	double focal_length = 0.0;
	double focal_length_in_35mm = 0.0;
	double gps_altitude;
	double gps_latitude = 0.0;
	double gps_longitude = 0.0;
	int orientation = 0;
	CString gps_latitude_str;
	CString gps_longitude_str;
	CString orientation_str;

	CString get_exif_str()
	{
		CString res;
		res.Format(_T("Ä«¸Ş¶ó Á¦Á¶»ç: %s\nÄ«¸Ş¶ó ¸ğµ¨¸í: %s\n¼ÒÇÁÆ®¿ş¾î: %s\nÃÔ¿µ ½Ã°¢: %s\nÇÃ·¡½Ã: %s\nÃÊÁ¡ °Å¸®: %.1f mm\n35mm È¯»ê: %.1f\n")\
			_T("³ëÃâ ½Ã°£ : 1/%d sec\n³ëÃâ º¸Á¤: %.2f EV\nÁ¶¸®°³ °ª: f/%.1f\nISO °¨µµ: %d\nÈ¸Àü Á¤º¸: %s\nGPS Á¤º¸: N %s, E %s, %.0fm"),
			camera_make,
			camera_model,
			software,
			original_datetime,
			flash ? _T("on") : _T("off"),
			focal_length,
			focal_length_in_35mm,
			(unsigned)round(1.0 / exposure_time),
			exposure_bias,
			f_number,
			iso_speed,
			orientation_str,
			gps_latitude_str,
			gps_longitude_str,
			gps_altitude);
		return res;
	}
};

/*
* 	¼­¹öÁ¤º¸°¡ full_urlÀÌ³ª ip, port·Î ³ª´²Á®ÀÖ¾îµµ ±×¿¡ ¸Â´Â »ı¼ºÀÚÇÔ¼ö¸¦ È£ÃâÇÏ¿© Ã¤¿ò.
	CRequestUrlParams params(m_server_ip, m_server_port, _T(""), _T("GET"));

	//¸¸¾à sub_urlÀÌ ÇÊ¿äÇÒ °æ¿ì ¾Æ·¡¿Í °°ÀÌ Ã¤¿öÁÖ°í
	params.sub_url.Format(_T("/lmm/api/v1.0/temp_envcheck/config-value-return?input_type=flag_windows_auto_update&mgrid=%s"), m_login_id);

	//½ÇÁ¦ request¸¦ È£ÃâÇÑ´Ù. thread¹æ½ÄÀÌ ¾Æ´Ï¸é ¹Ù·Î °á°ú°¡ params¿¡ Ã¤¿öÁ®¼­ ¸®ÅÏµÈ´Ù.
	request_url(&params);
	...
	if (params.status == HTTP_STATUS_OK)
	...
*/
class CRequestUrlParams
{
public:
	CRequestUrlParams() {}
	CRequestUrlParams(CString _ip, int _port, CString _sub_url = _T(""), CString _verb = _T("GET"), bool _is_https = true, std::deque<CString>*_headers = NULL, CString _body = _T(""), CString _local_file_path = _T(""))
	{
		ip = _ip;
		port = _port;
		sub_url = _sub_url;

		verb = _verb;
		is_https = _is_https;
		body = _body;
		local_file_path = _local_file_path;

		if (_headers)
		{
			for (size_t i = 0; i < _headers->size(); i++)
			{
				if (_headers->at(i).Right(2) != _T("\r\n"))
					_headers->at(i) += _T("\r\n");
				headers.push_back(_headers->at(i));
			}
		}
	}

	CRequestUrlParams(CString _full_url, CString _verb = _T("GET"), bool _is_https = true, std::deque<CString>* _headers = NULL, CString _body = _T(""), CString _local_file_path = _T(""));

	void		reset(bool sub_url_reset = false)
	{
		status = -1;
		full_url.Empty();	//full_url ¸â¹ö°ªÀÌ Ã¤¿öÁ®ÀÖÀ¸¸é ±× ¹®ÀÚ¿­À» ÆÄ½ÌÇÏ¿© ip, port, sub_url·Î ºĞ¸®ÇÏ¹Ç·Î reset()ÇÒ °æ¿ì´Â ¹İµå½Ã ºñ¿öÁà¾ß ÇÑ´Ù.
		body.Empty();
		result.Empty();
		elapsed = 0;
		local_file_path.Empty();
		file_size = 0;
		downloaded_size = 0;
		download_index = -1;

		if (sub_url_reset)
			sub_url.Empty();
	}

	//thread·Î º°µµ ½ÇÇàÇÒÁö(Æ¯È÷ ÆÄÀÏ ´Ù¿î·Îµå request), request °á°ú¸¦ ¹Ù·Î ¹Ş¾Æ¼­ Ã³¸®ÇÒÁö(´Ü¼ø request)
	bool		use_thread = false;

	//m_request_id·Î ÇØ´ç ÀÛ¾÷ÀÌ ¹«¾ùÀÎÁö ±¸ºĞÇÑ´Ù.
	int			request_id = -1;

	//200, 404...¿Í °°Àº HTTP_STATUS¸¦ ´ãÁö¸¸ invalid address µî°ú °°Àº ¿¡·¯ÄÚµåµµ ´ã±â À§ÇØ int·Î »ç¿ëÇÑ´Ù. 0º¸´Ù ÀÛÀ» °æ¿ì´Â result ¹®ÀÚ¿­¿¡ ¿¡·¯ ³»¿ëÀÌ ´ã°ÜÀÖ´Ù.
	int			status = -1;

	//Æ÷Æ®·Î http¿Í https¸¦ ±¸ºĞÇÏ´Â °ÍÀº À§ÇèÇÏ´Ù. m_isHttps=true ¶Ç´Â ip¿¡ "https://"°¡ Æ÷ÇÔµÇ¾î ÀÖÀ¸¸é m_isHttps°¡ ÀÚµ¿ true·Î ¼³Á¤µÈ´Ù.
	CString		ip = _T("");

	int			port = 0;
	CString		sub_url;				//domainÀ» Á¦¿ÜÇÑ ³ª¸ÓÁö ÁÖ¼Ò
	CString		verb = _T("GET");
	//urlÀÇ ½ÃÀÛÀÌ httpÀÎÁö httpsÀÎÁö, port°¡ 80ÀÎÁö 443ÀÎÁöµîÀÇ Á¤º¸·Î ÆÇ´ÜÇÒ ¼ö ÀÖÁö¸¸ Á¦´ë·Î ¸í½ÃµÇÁö ¾Ê°Å³ª ÀÓÀÇ Æ÷Æ®¹øÈ£¸¦ »ç¿ëÇÏ´Â °æ¿ìµµ ¸¹´Ù.
	bool		is_https = true;
	CString		body;					//post data(json format)
	int			timeout_ms = 30000;

	//token_header.Format(_T("token: %s"), ServiceSetting::strManagerToken);
	//°¢ Ç×¸ñÀÇ ³¡¿¡´Â ¹İµå½Ã "\r\n"À» ºÙ¿©Áà¾ßÇÏ´Âµ¥ ÀÌ´Â requestAPI()¿¡¼­ ¾Ë¾Æ¼­ Ã³¸®ÇÔ.
	std::deque<CString> headers;

	//proxy °èÁ¤ Á¤º¸°¡ ¾ø¾î¼­ ½ÇÆĞÇÏ¸é 407(Proxy Authentication Required) error°¡ ¹ß»ıÇÏ¹Ç·Î ÀÌ¶§´Â »ç¿ëÀÚ¿¡°Ô Á÷Á¢ ÀÔ·Â¹Ş°í
	//´Ù½Ã È£ÃâÇØÁà¾ß ÇÑ´Ù.
	CString		proxy_id;
	CString		proxy_pw;

	//ÇÑ¹ø È£ÃâÇØ¼­ ½ÇÆĞÇÑ ÈÄ port³ª ÁÖ¼Ò µî url°ü·Ã Á¤º¸¸¦ ¼öÁ¤ÇÏ¿© ´Ù½Ã request_url()À» È£ÃâÇÒ ¶§
	//full_urlÀ» ""·Î ¸¸µé¾îÁÖÁö ¾ÊÀ¸¸é ÀÌ °ªÀ» ¹Ù·Î »ç¿ëÇØ¼­ ´Ù½Ã requestÇÏ¹Ç·Î ¿ª½Ã ½ÇÆĞÇÏ°Ô µÈ´Ù.
	//¹İµå½Ã url °ü·Ã°ªÀ» ¼öÁ¤ÇÏ¿© ´Ù½Ã requestÇÒ °æ¿ì¿¡´Â ¹İµå½Ã full_url = _T("")·Î ¸¸µé¾îÁÖ°í È£ÃâÇØ¾ß ÇÑ´Ù.
	CString		full_url;				//[in][out] full_urlÀ» ÁÖ°í È£ÃâÇÏ¸é ÀÌ¸¦ ip, port, sub_url·Î ³ª´²¼­ Ã³¸®ÇÑ´Ù. ""·Î È£ÃâÇÏ¸é 
	CString		result;
	long		elapsed = 0;			//¼Ò¿ä½Ã°£. ms´ÜÀ§.

	//ÆÄÀÏ ´Ù¿î·Îµå °ü·Ã
	CString		local_file_path;		//urlÀÇ ÆÄÀÏÀ» ´Ù¿î¹ŞÀ» °æ¿ì ·ÎÄÃ ÆÄÀÏ full path ÁöÁ¤.
	uint64_t	file_size = 0;			//url ÆÄÀÏ Å©±â
	uint64_t	downloaded_size = 0;	//ÇöÀç±îÁö ¹ŞÀº Å©±â
	int			download_index = -1;	//n°³ÀÇ ÆÄÀÏ ´Ù¿î·Îµå½Ã ÇöÀç ÆÄÀÏÀÇ ÀÎµ¦½º. request_id¿Í´Â ´Ù¸§.
};

class CMouseEvent
{
public:
	CMouseEvent();
	CMouseEvent(CPoint pt1, int msg1, short zDelta1 = 0) { pt = pt1; msg = msg1; zDelta = zDelta1; }

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
								g_sForTimer.Format(_T("%s : %f\n"), (caption), g_dTime1 - g_dTime0);\
								if (b) \
									AfxMessageBox(g_sForTimer); \
								else \
									printf(g_sForTimer);

								//printf(g_sForTimer);

struct	NETWORK_INFO
{
	TCHAR		sDescription[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
	TCHAR		sIPAddress[16];
	TCHAR		sGateway[16];
	TCHAR		sSubnetMask[16];
	TCHAR		sMacAddress[16];
};

//////////////////////////////////////////////////////////////////////////
//ÇÁ·Î¼¼½º °ü·Ã
	//fullpath°¡ ""ÀÌ¸é ÇöÀç ½ÇÇàÆÄÀÏ·Î, strFlag´Â ±âº» ÆÄÀÏ¹öÀüÀ» ¾ò¾î¿Â´Ù.
	CString		get_file_property(CString fullpath = _T(""), CString strFlag = _T("FileVersion"));
	CString		get_exe_directory(bool includeSlash = false);
	CString		get_exe_parent_directory();
	CString		get_exe_filename(bool fullpath = false);
	CString		get_exe_file_title();
	CString		GetCurrentDirectory();
	ULONG		GetPID(CString processname);
	ULONG		ProcIDFromWnd(HWND hwnd);
	HWND		get_hwnd_by_pid(ULONG pid);
#ifndef _USING_V110_SDK71_
	//ÆÄÀÏ or Æú´õ or µå¶óÀÌºê or "³» PC"ÀÇ ¼Ó¼ºÃ¢À» Ç¥½ÃÇÑ´Ù.
	//´ÜÀÏ ÆÄÀÏÀ» ÁÖ°íÀÚ ÇÒ °æ¿ì´Â show_property_window(std::deque<CString> {path});¿Í °°ÀÌ È£ÃâÇÏ¸é µÈ´Ù.
	bool		show_property_window(std::deque<CString> fullpath);
	CString		GetProcessNameByPID(const DWORD pid);
#else
	bool		show_property_window(std::deque<CString> fullpath);
#endif
	//ÇØ´ç ÇÁ·Î¼¼½º ÆÄÀÏÀÌ ½ÇÇàÁßÀÎ ÀÎ½ºÅÏ½º Ä«¿îÆ®¸¦ ¸®ÅÏ.
	//½ÇÇà ÆÄÀÏ¸í¸¸ ÁÖ¸é ÆÄÀÏ¸í¸¸ ºñ±³ÇÏÁö¸¸ ÀüÃ¼ °æ·Î¸¦ ÁÖ¸é °æ·Î±îÁö ¸Â¾Æ¾ß Ä«¿îÆ® µÊ.
	//ex. Ç®ÆĞ½ºÀÎ c:\test.exe¸¦ ÁÖ¸é d:\test.exe´Â ½ÇÇàÁßÀÌ¶óµµ Ä«¿îÆ®µÇÁö ¾Ê´Â´Ù.
	int			get_process_running_count(CString processname);
	bool		is_running(CString processname);
	
	//return value : 1(killed), 0(fail to kill), -1(not found)
	int			kill_process_by_fullpath(CString fullpath);

	bool		kill_process(CString processname);
	//ÇÁ·Î¼¼½º °­Á¦ Á¾·á.
	//return value : 1 : killed, 0 : fail to kill, -1 : not found
	bool		ProcessKill(CString szProcessName);

	HWND		GetWindowHandleFromProcessID(DWORD dwProcId);
	bool		IsDuplicatedRun();

	int			find_parameter(CString target);


	//cmd ¸í·É ½ÇÇà ÈÄ °á°ú¸¦ ¹®ÀÚ¿­·Î ¸®ÅÏ.
	//wait_until_process_exit : ½ÇÇà ÇÁ·Î¼¼½º°¡ Á¤»ó Á¾·áµÉ¶§±îÁö ±â´Ù¸°´Ù.
	//return_after_first_read : wait_until_process_exit¸¦ false·Î ÇØµµ Àå½Ã°£ ³¡³ªÁö ¾Ê´Â °æ¿ì°¡ ÀÖ¾î(ex. telnet)
	//¿ì¼± ÀÌ °ªÀÌ trueÀÌ¸é ¸Ç Ã³À½ readÈÄ¿¡ ¹Ù·Î Á¾·á½ÃÅ²´Ù.
	//ÁÖÀÇ! osk.exe¶ó´Â °¡»óÅ°º¸µå ÇÁ·Î±×·¥Àº systemÆú´õ¿¡¸¸ ÀÖ°í SysWow64 Æú´õ¿¡´Â ¾ø´Âµ¥
	//32bit ÇÁ·Î±×·¥¿¡¼­ ShellExecute() ¶Ç´Â CreateProcess()·Î ½ÇÇàÇÏ¸é SysWow64Æú´õ¿¡¼­ ÇØ´ç ÆÄÀÏÀ» Ã£À¸¹Ç·Î ½ÇÆĞÇÑ´Ù.
	//c:\\windows\\system32\\osk.exe·Î ½ÇÇàÇØµµ SysWOW64 Æú´õ·Î redirectµÇ¹Ç·Î ¿ª½Ã ½ÇÇàµÇÁö ¾Ê´Â´Ù.
	//Wow64DisableWow64FsRedirection()¸¦ ÀÌ¿ëÇØ¼­ redirectionÀ» disable½ÃÄÑÁÖ°í ½ÇÇà ÈÄ º¹¿ø½ÃÄÑÁà¾ß ÇÑ´Ù.
	//"dir C:\\*.*"Àº ¼º°øÇÏ³ª "dir \"C:\\Program Files\\*.*\"" ¸í·ÉÀº ½ÇÆĞÇÑ´Ù.
	//"ipconfig /all"Àº wait_until_process_exit°¡ true or false ¸ğµÎ ¹®Á¦¾øÁö¸¸
	//"systeminfo"¿Í "tasklist"´Â wait_until_process_exit¸¦ true·Î ÁÖ¸é ¹«ÇÑ ´ë±âÇÏ´Â Çö»óÀÌ ÀÖ´Ù. false·Î Áàµµ Á¤º¸´Â ¸ğµÎ ¸®ÅÏµÈ´Ù.
	CString		run_process(CString cmd, bool wait_until_process_exit, bool return_after_first_read = false);

	//"dir \"C:\\Program Files\\*.*\"" ¸í·ÉÀº Àß µ¿ÀÛÇÏ³ª ping -t¿Í °°ÀÌ ³¡³ªÁö ¾Ê´Â µµ½º¸í·É¾î³ª notepad.exe¸¦ ½ÇÇàÇÒ °æ¿ì
	//µµ½ºÃ¢ÀÌ °è¼Ó ³²¾ÆÀÖ´Ù.
	//¶ÇÇÑ ÀÌ ¹æ½ÄÀº µµ½ºÃ¢ÀÌ Ç¥½ÃµÇ¾ú´Ù°¡ »ç¶óÁö´Â ºÎÀÛ¿ëÀÌ ÀÖ´Ù. µµ½ºÃ¢ Ç¥½Ã°¡ ºÒÇÊ¿äÇÏ´Ù¸é À§ÀÇ run_process()¸¦ »ç¿ëÇØ¾ß ÇÑ´Ù.
	CString		run_process(CString cmd);
	extern		void* g_wow64_preset;
	void		Wow64Disable(bool disable = true);

	//¼­ºñ½º °ü·Ã ¸í·ÉÀ» ½±°Ô Ã³¸®ÇÏ±â À§ÇØ ÀÛ¼º.
	//cmd´Â ´ÙÀ½°ú °°Àº Å°¿öµå¸¦ »ç¿ëÇÑ´Ù.
	//"query"	: status¸¦ ¸®ÅÏ
	//"stop"	: ¼­ºñ½º¸¦ ÁßÁö½ÃÅ°°í ÃÖÁ¾ status = "SERVICE_STOPPED"¸¦ ¸®ÅÏ, ±×·¸Áö ¾ÊÀ¸¸é detail ÂüÁ¶.
	//			: ¼­ºñ½º°¡ Á¸ÀçÇÏÁö ¾Ê°Å³ª ÀÌ¹Ì ÁßÁöµÈ °æ¿ì¿¡µµ "SERVICE_STOPPED"¸¦ ¸®ÅÏÇÔ.
	//"delete"	: ¼­ºñ½º »èÁ¦°¡ ¼º°øÇÏ¸é 0ÀÌ ¾Æ´Ñ °ªÀ» ¸®ÅÏ. ½ÇÆĞÇÏ¸é 0À» ¸®ÅÏÇÏ¹Ç·Î ÀÌ °æ¿ì´Â detail ÂüÁ¶.
	DWORD		service_command(CString service_name, CString cmd, CString *detail = NULL);

	//Console ¸í·ÉÀÎÁö GUI À©µµ¿ì ¾îÇÃ¸®ÄÉÀÌ¼ÇÀÎÁö ±¸ºĞ
	bool		is_gui_application(CString fullPath);

	//PID, ÇÁ·Î¼¼½º ÀÌ¸§, À©µµ¿ì Å¸ÀÌÆ² ÀÌ¸§, À©µµ¿ì Å¬·¡½º ÀÌ¸§À¸·Î Å¬·¡½ºÀÇ »ıÁ¸ »óÅÂ¸¦ ±¸ÇÒ¼ö ÀÖ½À´Ï´Ù. from Devpia
	bool		CheckProcessUsingPID(unsigned long pid);
	bool		CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName);
	bool		CheckProcessUsingProcessName(LPCTSTR processName);

	//¸ğ´ÏÅÍ Á¤º¸
	//main¿¡¼­ EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); ¸¦ ½ÇÇàÇÏ°í
	//ÀÌ ÆÄÀÏ¿¡ Àü¿ªº¯¼ö·Î ¼±¾ğµÈ g_monitors¸¦ ÀÌ¿ëÇÏ¸é µÈ´Ù.
	//´Ü, Win32APIÀÎ EnumDisplayMonitors()¸¦ È£ÃâÇÒ¶§´Â ¹İµå½Ã g_monitors.clear()¸¦ ÇØÁà¾ß ÇÏ¹Ç·Î
	//enum_display_monitors()ÇÔ¼ö¸¦ »ç¿ëÇÏµµ·Ï ÇÑ´Ù.
	extern std::deque<CSCMonitorInfo> g_monitors;
	void		enum_display_monitors();
	BOOL		CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
	//rÀÌ °ÉÃÄÀÖ´Â ¸ğ´ÏÅÍ ÀÎµ¦½º¸¦ ¸®ÅÏ. °ãÃÄÁö´Â ¿µ¿ªÀÌ ¾îµğ¿¡µµ ¾ø´Ù¸é -1À» ¸®ÅÏ.
	//entire_included°¡ trueÀÌ¸é ¾î¶² ¸ğ´ÏÅÍ¿¡ ¿ÏÀüÈ÷ ¼ÓÇØÀÖ´Â °æ¿ì¿¡¸¸ ÇØ´ç ÀÎµ¦½º¸¦ ¸®ÅÏ.
	int			get_monitor_index(CRect r, bool entire_included = false);
	//x, y°¡ ¼ÓÇØÀÖ´Â ¸ğ´ÏÅÍ ÀÎµ¦½º¸¦ ¸®ÅÏ
	int			get_monitor_index(int x, int y);
	//¸ÖÆ¼¸ğ´ÏÅÍ ÀüÃ¼ ¿µ¿ª »ç°¢Çü ¸®ÅÏ. -1ÀÌ¸é ÀüÃ¼ ¸ğ´ÏÅÍ ¿µ¿ªÀ» ¸®ÅÏ.
	CRect		get_monitor_rect(int index = -1);

	//::SetForegroundWindow()°¡ Win98ÀÌÈÄºÎÅÍ´Â Áö¿øµÇÁö ¾Ê¾Æ ¼öÁ¤µÈ ÄÚµå.
	void		SetForegroundWindowForce(HWND hWnd, bool makeTopMost = false);
	bool		is_top_most(HWND hWnd);

//Å¬¸³º¸µå clipboard
	bool		copy_to_clipboard(HWND hWnd, CString str);

//////////////////////////////////////////////////////////////////////////
//¹®ÀÚ¿­
	bool		Compare_By_Case_Sensitive(CString str1, CString str2, bool bCase);
	//target¿¡¼­ ¸Ç Ã³À½ ¸ÅÄªµÇ´Â Ç×¸ñ¸¸ Ã£¾Æ ±× ½ÃÀÛÀ§Ä¡¸¦ ¸®ÅÏÇÑ´Ù.
	//´Ü, case_sensitive°¡ falseÀÏ °æ¿ì textÀÇ ±æÀÌ°¡ ¸Å¿ì Å©´Ù¸é ¸Å¹ø MakeLower()¸¦ È£ÃâÇÏ¹Ç·Î ´À·ÁÁú ¼ö ÀÖ´Ù.
	//while¹® µî¿¡¼­ ÀÌ ÇÔ¼ö¸¦ Áö¼ÓÀûÀ¸·Î È£ÃâÇÒ °æ¿ì´Â ¹Ì¸® MakeLower()·Î º¯°æ ÈÄ È£ÃâÇØ¾ßÇÑ´Ù.
	//Áï, °£´ÜÇÑ ¹®ÀÚ¿­ÀÏ °æ¿ì¿¡¸¸ ÀÌ ÇÔ¼ö¸¦ »ç¿ëÇÏ°í ±æÀÌ°¡ ¸Å¿ì Å©´Ù¸é ¹®ÀÚ¿­À» °¡°øÇÑ ÈÄ CString::Find()¸¦ ÀÌ¿ëÇØ¾ß ÇÑ´Ù.
	int			find(CString target, CString find_string, int start = 0, bool case_sensitive = false, bool whole_word = false);
	//target¿¡¼­ ¸ÅÄªµÇ´Â ¸ğµç Ç×¸ñÀÇ ½ÃÀÛÀ§Ä¡¸¦ result deque¿¡ ´ã°í Ã¹¹øÂ° ¸ÅÄª À§Ä¡¸¦ ¸®ÅÏÇÑ´Ù.
	//´Ü, case_sensitive°¡ falseÀÏ °æ¿ì textÀÇ ±æÀÌ°¡ ¸Å¿ì Å©´Ù¸é ¸Å¹ø MakeLower()¸¦ È£ÃâÇÏ¹Ç·Î ´À·ÁÁú ¼ö ÀÖ´Ù.
	//while¹® µî¿¡¼­ ÀÌ ÇÔ¼ö¸¦ Áö¼ÓÀûÀ¸·Î È£ÃâÇÒ °æ¿ì´Â ¹Ì¸® MakeLower()·Î º¯°æ ÈÄ È£ÃâÇØ¾ßÇÑ´Ù.
	//Áï, °£´ÜÇÑ ¹®ÀÚ¿­ÀÏ °æ¿ì¿¡¸¸ ÀÌ ÇÔ¼ö¸¦ »ç¿ëÇÏ°í ±æÀÌ°¡ ¸Å¿ì Å©´Ù¸é ¹®ÀÚ¿­À» °¡°øÇÑ ÈÄ CString::Find()¸¦ ÀÌ¿ëÇØ¾ß ÇÑ´Ù.
	int			find_all(std::deque<int>& result, CString target, CString find_string, bool case_sensitive = false, bool whole_word = false);

	//dqSrc¿¡ dqFind°¡ ÀÖ´ÂÁö °Ë»çÇÏ¿© ÀÎµ¦½º¸¦ ¸®ÅÏ. ÇöÀç´Â AND ¿¬»êÀÌ¹Ç·Î dqFindÀÇ ¸ğµç ¿ø¼Ò°¡ dqSrc¿¡ Æ÷ÇÔµÇ¾î ÀÖ¾î¾ß ÇÔ.
	int			find_dqstring(std::deque<CString> dqSrc, CString strFind, bool bWholeWord = false, bool bCaseSensitive = false);
	int			find_dqstring(std::deque<CString> dqSrc, std::deque<CString> dqFind, TCHAR op = '&', bool bWholeWord = false, bool bCaseSensitive = false);
	int			Find_Divide_Position_By_Punctuation(CString str);
	int			FindStringFromArray(CStringArray& ar, CString sTarget, bool bCaseSensitive = false, bool bWholeWord = false);
	//int			FindStringFromDeque(std::deque<CString> dq, CString sTarget, bool bCaseSensitive = false, bool bWholeWord = false);
	CString		GetCommaString(CString sString, CString sComma = _T(","));
	bool		IsNumericString(const CString& strSource);
	//¾ç¼öÀÇ Á¤¼ö°ª ¹®ÀÚ¿­ÀÎÁö
	bool		IsNatural(LPCTSTR lpszValue);
	//¼Ò¼ıÁ¡±îÁö Çã¿ëÇÏ¿© ¼ıÀÚ°ªÀÎÁö
	bool		IsNumeric(LPCTSTR lpszValue);
	//À½¼ö¸¦ Çã¿ëÇÏ´Â Á¤¼ö°ª ¹®ÀÚ¿­ÀÎÁö
	bool		IsInteger(LPCTSTR lpszValue);
	//startºÎÅÍ ½ÃÀÛÇØ¼­ Ã³À½ ¸¸³ª´Â ¼ıÀÚ ¿µ¿ªÀ» ÃßÃâÇØ¼­ num¿¡ ³Ñ°ÜÁØ´Ù.
	//¼ıÀÚ ¿µ¿ªÀÇ ³¡ ÀÎµ¦½º¸¦ ¸®ÅÏÇÑ´Ù.
	//¼ıÀÚ ¿µ¿ªÀÌ ¾øÀ¸¸é false¸¦ ¸®ÅÏÇÑ´Ù.
	int			get_number_from_string(CString str, int &num, int start = 0);
	//¹®ÀÚ¿­¿¡ Æ÷ÇÔµÈ ¼ıÀÚ¹®ÀÚ¸¦ ¼ıÀÚ·Î °£ÁÖÇÏ¿© ºñ±³ÇÑ´Ù. "a5"´Â "a12"º¸´Ù ÀÛ´Ù.
	bool		is_greater_with_numeric(CString str0, CString str1);
	//ÁÖ¾îÁø ¹®ÀÚ¿­ÀÌ ¾ËÆÄºª°ú ¼ıÀÚ·Î¸¸ ±¸¼ºµÈ ¹®ÀÚ¿­ÀÎÁö °Ë»çÇÑ´Ù.
	//excepts¿¡´Â Æ÷ÇÔµÇµµ µÇ´Â ¹®ÀÚ¿­µéÀÌ µé¾îÀÖ´Âµ¥ ÀÌµéÀº ;À¸·Î ±¸ºĞµÇ¾î ÀÖ°í
	//°Ë»çÇÏ±â Àü¿¡ ¹Ì¸® »èÁ¦ÇÑ ÈÄ °Ë»çÇÑ´Ù.
	bool		IsAlphaNumeric(CString str, CString excepts = _T(""));

	//ASCII ÄÚµåÀÇ #33(0x21)(' ') ~ #126(0x7E)('~') ¹üÀ§ÀÎÁö(ÀĞÀ» ¼ö ÀÖ´Â ¹®ÀÚ¿­ÀÎÁö)
	bool		is_readable_char(CString src);

	//°¢ ¾ğ¾î¸¶´Ù ÁÖ¼®Ã³¸® ¹®ÀÚ¿­ÀÌ ´Ù¸£¹Ç·Î ÁÖ¼®Ã³¸®µÈ ¶óÀÎÀÎÁö ÆÇº°
	//¸®ÅÏ°ªÀº ÇØ´ç ÁÖ¼®Ã³¸® ¹®ÀÚ¿­
	CString		is_comment(CString src);

	//'°¡'~'ÆR'¹üÀ§ÀÇ ¿ÂÀüÇÑ ÇÑ±ÛÀÎÁö °Ë»çÇÑ´Ù.
	//'°¡' = true
	//'°­' = true
	//'°­¤§' = false
	//allow_ascii°¡ true¶ó¸é ¿µ¹®, ¼ıÀÚ, Æ¯¼ö¹®ÀÚ°¡ ÀÖ¾îµµ ÇÑ±Û¸¸ ¿ÂÀüÇÏ¸é trueÀÌ¸ç
	//allow_ascii°¡ false¶ó¸é ¿À·ÎÁö ÇÑ±Û·Î¸¸ ±¸¼ºµÇ¾ú´ÂÁö¸¦ ÆÇº°ÇÏ¿© ¸®ÅÏÇÑ´Ù.
	//allow_ascii°¡ trueÀÏ °æ¿ì ½ÇÁ¦ ÇÑ±Û¹®ÀÚ°¡ Æ÷ÇÔµÇ¾ú´ÂÁöµµ ÆÇº°ÇÒ °æ¿ì´Â int*¸¦ ³Ñ°Ü¹Ş¾Æ ÆÇ´ÜÇÏ¸é µÈ´Ù.
	bool		is_hangul(CString str, bool allow_ascii = false, int *hangul_count = NULL);

	//¹®ÀÚ¿­ÀÌ ¿ÂÀüÇÑÁö ±úÁø ¹®ÀÚÀÎÁö¸¦ ÆÇº°(Æ¯È÷ ÇÑ±Û ÀÎÄÚµù ±úÁü ÆÇº°)
	bool		is_valid_string(CString src, bool include_hangul);

	//¿Ï¼ºÇü ÇÑ±ÛÀÇ ÇÑ ±ÛÀÚ¸¦ ÃÊ¼º, Áß¼º, Á¾¼ºÀ¸·Î ºĞ¸®ÇÑ´Ù.
	bool		get_consonant(CString src, wchar_t* cho = 0, wchar_t* jung = 0, wchar_t* jong = 0);

	CString		ConvertInt2AZ(int n);	//nÀ» 26Áø¼ö ¿¢¼¿ ÄÃ·³ ÀÎµ¦½º·Î º¯È¯ÇÑ ¹®ÀÚ¿­À» ¸®ÅÏ
	CString		GetToken(CString& str, LPCTSTR c);
	CString		GetToken(CString src, CString separator, int index);

	//separator´Â ±âÁ¸ CString¿¡¼­ TCHAR·Î ÅëÀÏÇÑ´Ù.
	//2charÀÌ»óÀÇ ¹®ÀÚ¿­ÀÌ ÇÏ³ªÀÇ separator·Î »ç¿ëµÉ °æ¿ì´Â °ÅÀÇ ¾øÀ¸³ª
	//1°³ ¶Ç´Â ±× ÀÌ»óÀÇ ¼­·Î ´Ù¸¥ ¹®ÀÚ¸¦ separatorµé·Î »ç¿ëÇÒ °æ¿ì´Â ÀÖÀ» °ÍÀÌ´Ù.
	//20240426 separator°¡ ¿©·¯°³ÀÏ °æ¿ì´Â Æ¯Á¤ separator¿¡ ÀÇÇØ ¾ò¾îÁø tokenÀÌ ¶Ç ´Ù¸¥ separator¸¦ Æ÷ÇÔÇÒ ¼ö ÀÖÀ¸¹Ç·Î
	//±× Ã³¸®°¡ ¸Å¿ì º¹ÀâÇØÁø´Ù. ±×³É CString separator·Î Ã³¸®ÇÑ´Ù.
	//include_rest´Â nMaxTokenÀÌ ¸í½ÃµÈ °æ¿ì ¸¶Áö¸· Åä±ÙÀ» ¾îµğ±îÁö·Î ÇÏ´À³Ä¸¦ Á¤ÇÏ´Â ¿É¼ÇÀÌ´Ù.
	//¿¹¸¦ µé¾î "LastUpdated : 1601-01-01 9:00:00"°ú °°Àº ¹®ÀÚ¿­À» ':'·Î ÆÄ½ÌÇÒ °æ¿ì
	//nMaxTokenÀ» 2·Î ÁØ °æ¿ì dqToken[0] = "LastUpdated "°¡ µÇ°í
	//µÎ¹øÂ° ÅäÅ«Àº " 1601-01-01 9"°¡ µÇÁö¸¸(½ÇÁ¦ ±â´ë°ªÀº " 1601-01-01 9:00:00"ÀÏ °ÍÀÌ´Ù)
	//include_rest¸¦ true·Î ÁÖ¸é dqToken[1] = " 1601-01-01 9:00:00"ÀÌ µÈ´Ù.
	//Áï, ÃÖ´ë ÅäÅ« °³¼ö°¡ Á¤ÇØÁ® ÀÖÀ» ¶§ ¸¶Áö¸· ÅäÅ«À» ¾îµğ±îÁö·Î Ã³¸®ÇÒ °ÍÀÎ°¡¿¡ ´ëÇÑ ¿É¼ÇÀÌ´Ù.
	int			get_token_str(CString src, std::deque<CString>& dqToken, CString separator = _T("|"), bool allowEmpty = true, int nMaxToken = -1, bool include_rest = false);
	int			get_token_str(std::string src, std::deque<CString>& dqToken, CString separator = _T("|"), bool allowEmpty = true, int nMaxToken = -1, bool include_rest = false);
	int			get_token_str(TCHAR *src, TCHAR *separator, CString *sToken, int nMaxToken);
	int			get_token_str(char *src, char *separator, char **sToken, int nMaxToken);

	//´ëºÎºĞÀÇ °æ¿ì´Â get_token_str()À» »ç¿ëÇÏÁö¸¸ separator°¡ ¹®ÀÚ¿­ ³»¿¡ Æ÷ÇÔµÈ °æ¿ìµµ ÀÖÀ» ¼ö ÀÖ´Ù.
	//"³» PC\\¿¬±¸¼Ò¹®¼­2(\\\\192.168.1.103) (X:)" ¹®ÀÚ¿­À» '\\'·Î ±¸ºĞÇÒ °æ¿ì Àß¸ø ÃßÃâµÇ¹Ç·Î
	//ÀÌ °æ¿ì´Â get_exact_token_string()À» »ç¿ëÇØ¼­ ½ÇÁ¦ '\\'ÀÎ °æ¿ì¿¡¸¸ ÃßÃâÇÏµµ·Ï ÇØ¾ß ÇÑ´Ù.
	int			get_exact_token_str(CString src, std::deque<CString>& dqToken, CString separator = _T("|"));

	//"<b><cr=red>This</b></cr> is a <i>sample</i> <b>paragraph</b>."
	//À§¿Í °°Àº Çü½ÄÀÏ ¶§ ÅÂ±×¿Í ÅØ½ºÆ®¸¦ ºĞ¸®ÇÑ´Ù. ÅÂ±×³»ÀÇ °ø¹éÀº Á¦°ÅµÈ´Ù.
	void		get_tag_str(CString& src, std::deque<CString>& tags);

	//°£È¤ \r, \n, \t, \\µîÀÇ ¹®ÀÚ¸¦ ±×´ë·Î È®ÀÎÇÒ ÇÊ¿ä°¡ ÀÖ´Ù.
	CString		get_unescape_str(CString src);

	//¿¢¼¿ÀÇ ÄÃ·³°ú °°ÀÌ n=0ÀÌ¸é "A"¸¦, n=25ÀÌ¸é "Z"¸¦, n=26ÀÌ¸é "AA"¸¦, n=27ÀÌ¸é "AB"¸¦ ¸®ÅÏÇÑ´Ù.
	//CString		get_excel_column(int n);
	std::string	get_excel_column(int n);

	// a_value : 1.1.24050
	// b_value : Normal
	// c_value : True
	// À§¿Í °°ÀÌ ¼Ó¼ºÀÌ¸§ ¹× °ªÀ¸·Î ¸ÅÇÎµÇ´Â ¹®ÀÚ¿­À» ÆÄ½ÌÇÏ¿© std::map¿¡ ³Ö¾îÁØ´Ù.
	// lfrf´Â ¶óÀÎºĞ¸®¹®ÀÚ¿­ÀÌ°í º¸Åë "\n"ÀÌ°Å³ª "\r\n" µîÀÌ ÀÖ°í
	// separator´Â ':' ÀÌ¸§°ú °ªÀ» ±¸ºĞÇÏ´Â ±¸ºĞÀÚÀÌ´Ù.
	// return value : Ç×¸ñÀÇ °³¼ö
	int			get_map_str(CString src, std::map<CString, CString>& map, CString lfrf = _T("\n"), CString separator = _T(":"));

	//list Ç×¸ñµéÀ» ','·Î ³ª¿­ÇÑ ¹®ÀÚ¿­À» ¸®ÅÏÇÑ´Ù.
	template <class T> inline CString get_list_str(std::deque<T>& list)
	{
		CString res;

		for (size_t i = 0; i < list.size(); i++)
		{
			if (typeid(T) == typeid(CString))
			{
				if (i > 0)
					res += _T(",");

				//list°¡ std::deque<CString> Å¸ÀÔÀÌ¶óµµ ¾Æ·¡¿Í °°Àº ¹®ÀåÀº ¿À·ù°¡ ¹ß»ıÇÑ´Ù.
				//¹İµå½Ã strÀ» ¼±¾ğÇÏ°í Format()À» ÀÌ¿ëÇÏ¿© °ªÀ» ¼¼ÆÃÇÑ ÈÄ ´õÇØÁà¾ß ÇÑ´Ù.
				//res += list[i];	//error
				//res += CString(list[i]);	//error

				CString str;
				str.Format(_T("%s"), list[i]);
				res += str;
			}
			else if (typeid(T) == typeid(int))
			{
				if (i > 0)
					res += _T(",");

				CString str;
				str.Format(_T("%d"), list[i]);
				res += str;
			}
			else
			{
				ASSERT(FALSE); //Áö¿øÇÏÁö ¾Ê´Â Å¸ÀÔ
			}
		}

		return res;
	}

	//dqÇ×¸ñÀ» ÇÏ³ªÀÇ ¹®ÀÚ¿­·Î ÇÕÃÄÁØ´Ù.
	CString		get_concat_string(std::deque<CString> dq, CString separator = _T("|"));

	//[2023/1/1 22:1:29] [DBMS][NMS_LS_TERMINATE_SESSION_DATA][ID : tmax25][update livesupport_report set endtime = '2023-01-01 22:01:29', env_type = '5', viewer_type = '0' where accesscode = '31108355' and sptnum = '50400']
	//[]·Î ¹­¿©Áø ÅäÅ«À» ºĞ¸®ÇÑ´Ù.
	//°ıÈ£°¡ ½ÖÀÌ ¾È¸ÂÀ¸¸é false¸¦ ¸®ÅÏÇÑ´Ù.
	bool		get_bracket_token(CString src, std::deque<CString>* token, TCHAR sep);

	//update livesupport_report set endtime = '2023-01-01 22:01:29', env_type = '5', viewer_type = '0' where accesscode = '31108355' and sptnum = '50400'
	//À§¿Í °°Àº sql¿¡¼­ field¿Í value¸¦ ÃßÃâÇÑ´Ù.
	void		get_sql_token_from_assign_form(CString src, std::map<CString, CString> *map);
	//insert into neturo_server_info(userid, com_name, s_pub_ip, s_pri_ip) values('14533821', 'DESKTOP-0CN9VAK', '220.85.215.243', 'publicIP')
	//À§¿Í °°Àº sql¿¡¼­ field¿Í value¸¦ ÃßÃâÇÑ´Ù.
	void		get_sql_token_from_bracket_form(CString src, std::map<CString, CString>* map);
	CString		get_sql_cmd(CString src, CString* sql_cmd = NULL, CString *table_name = NULL);

	//deque¿¡ ÀÖ´Â ¿ø¼ÒµéÀ» ±¸ºĞÀÚ·Î ÇÏ´Â ÇÏ³ªÀÇ ¹®ÀÚ¿­·Î ¸®ÅÏ
	CString		get_tokenized(std::deque<CString> dq, TCHAR separator = ';');

	//src¿¡¼­ sep¸¦ ±¸ºĞÀÚ·Î ÇÏ³ª¾¿ »Ì¾Æ³»°í ±× ³ª¸ÓÁö¸¦ ´Ù½Ã src·Î Ä¡È¯ÇÑ´Ù.
	//src°¡ °è¼Ó º¯°æµÊ¿¡ ÁÖÀÇ.
	CString		get_str(CString& src, CString sep = _T("|"));
	int			get_int(CString& src, CString sep = _T("|"));
	double		get_double(CString& src, CString sep = _T("|"));

	//src¿¡¼­ prefix¿Í postfix »çÀÌ¿¡ ÀÖ´Â ¹®ÀÚ¿­À» ÃßÃâÇÑ´Ù.
	CString		extract_sub_str(CString src, CString prefix, CString postfix);

	//resource string tableÀÇ ¹®ÀÚ¿­À» ¸®ÅÏÇÑ´Ù.
	extern CString load_string(UINT nID);

	//unit			: -1:auto, 0:bytes, 1:KB, 2:MB, 3:GB ~
	//autoÀÏ °æ¿ì´Â 1000º¸´Ù ÀÛÀ»‹š±îÁö ³ª´©°í ¼Ò¼öÁ¡Àº 2ÀÚ¸®±îÁö Ç¥½ÃÇÑ´Ù.(ex 7.28TB)
	//floats		: ¼Ò¼öÁ¡À» ¸î ÀÚ¸®±îÁö Ç¥½ÃÇÒÁö
	//unit_string	: ´ÜÀ§¸¦ Ç¥½ÃÇÒ Áö
	//comma			: Á¤¼ö ºÎºĞ¿¡ ÀÚ¸®¼ö ÄŞ¸¶¸¦ Ç¥½ÃÇÒ Áö
	CString		get_size_str(ULONGLONG size, int unit = 1, int floats = 0, bool unit_string = true, bool comma = true);

	//src¸¦ ÆÄ½ÌÇØ¼­ Æ¯Á¤ ±æÀÌ ÀÌ»óÀÇ ¹®ÀÚ¿­µé·Î ³ª´«´Ù.
	std::deque<CString> parse_divide(CString src, int len);
	//srcÀÇ pos±ÙÃ³¿¡¼­ ±¸µÎÁ¡À» Ã£¾Æ ±× À§Ä¡¸¦ ¸®ÅÏÇÑ´Ù.(±¸µÎÁ¡À¸·Î ¹®ÀåÀ» ³ª´­¶§ ÀÌ¿ë)
	int			find_punctuation(CString src, int pos);
	bool		is_punctuation(TCHAR ch);

	//isOneOf()´Â is_one_of()·Î ´ëÃ¼ÇÔ.
	template <typename ... Types> bool is_one_of(CString src, Types... args)
	{
		if (src.IsEmpty())
			return false;

		int n = sizeof...(args);
		CString arg[] = { args... };

		for (auto element : arg)
		{
			if (src == element)
				return true;
		}

		return false;
	}

	template <typename ... Types> bool find_one_of(CString src, Types... args)
	{
		if (src.IsEmpty())
			return false;

		int n = sizeof...(args);
		CString arg[] = { args... };

		for (auto element : arg)
		{
			if (src.Find(element) >= 0)
				return true;
		}

		return false;
	}


	//src ¹®ÀÚ¿­¿¡ set_of_keyword¿¡ ³ª¿­µÈ ´Ü¾î°¡ ÀÖ´ÂÁö °Ë»ç.
	//set_of_keyword´Â ¼¼¹ÌÄİ·ĞÀ¸·Î ±¸ºĞÇØ¼­ ¿©·¯ ¹®ÀÚ ¶Ç´Â ¹®ÀÚ¿­À» ³ÖÀ» ¼ö ÀÖ´Ù.
	//ex. src = "abcd1234"ÀÏ ¶§ set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
	bool		is_exist_keyword(CString src, CString set_of_keyword, bool case_sensitive = false, bool whole_word = false);

	//dqList¿¡¼­ element°ª°ú ÀÏÄ¡ÇÏ´Â Ç×¸ñÀÇ index¸¦ ¸®ÅÏÇÑ´Ù. ¾øÀ¸¸é -1À» ¸®ÅÏ.
	template <typename T> int find_index(std::deque <T> &dqList, T element)
	{
		auto it = std::find(dqList.begin(), dqList.end(), element);
		//typename T::iterator it = std::find(dqList.begin(), dqList.end(), element);
		if (it != dqList.end())
		{
			return std::distance(dqList.begin(), it);
		}

		return -1;
	}

	//strÀÇ from À§Ä¡ ÀÌÈÄ¿¡ ÀÖ´Â ¼ıÀÚ ¿µ¿ª°ªÀ» num¿¡ ³Ö¾îÁÖ°í ¼ıÀÚ ½ÃÀÛÀ§Ä¡¸¦ returnÇÑ´Ù.
	int	extract_digit_number(char *str, int from, double *num);

	//version string valid check
	//digits : ÀÚ¸´¼ö(1.0.0.1ÀÏ °æ¿ì´Â ÀÚ¸´¼ö 4)
	bool valid_version_string(CString versionStr, int digits);

	//¹öÀü ¶Ç´Â IPÁÖ¼ÒµîÀº ±×³É ¹®ÀÚ¿­·Î ºñ±³ÇÏ¸é 1.0.9.0ÀÌ 1.0.10.0º¸´Ù ´õ Å©´Ù°í ³ª¿À¹Ç·Î
	//.À» ¾ø¾Ø ¼ıÀÚ·Î ºñ±³ÇßÀ¸³ª ÀÌ ¹æ¹ıµµ ¿À·ù ¹ß»ı(1.0.1.13 > 1.0.10.3º¸´Ù Å©´Ù°í ÆÇ´ÜÇÔ)
	//°á±¹ °¢ ÀÚ¸´¼ö³¢¸® ±¸ºĞÇØ¾ß ÇÑ´Ù.
	//¸®ÅÏ°ªÀº strcmp¿Í µ¿ÀÏÇÑ ±ÔÄ¢À¸·Î ÆÇ´ÜÇÑ´Ù.(+:str0°¡ Å­, -:str1ÀÌ Å­, 0:°°À½)
	int	compare_string(CString str0, CString str1, TCHAR separator = '.');

	//src¸¦ n¹ø ¿¬°áÇÑ ¹®ÀÚ¿­ ¸®ÅÏ. n°³ÀÇ °ø¹é, ÅÇÀÌ ÇÊ¿äÇÒ °æ¿ì »ç¿ë
	//20250829 ±âÁ¸ make_string() ÇÔ¼öÀÇ ÀÌ¸§À» duplicate_str()À¸·Î º¯°æÇÔ.
	CString duplicate_str(CString src, int n);

	//http://yeobi27.tistory.com/280
	//A2W, A2T ¹× ±× ¹İ´ë ¸ÅÅ©·ÎµéÀº ½ºÅÃÀ» »ç¿ëÇÏ¹Ç·Î ¹®Á¦ ¼ÒÁö°¡ ÀÖ°í Å©±â Á¦ÇÑµµ ÀÖÀ¸¹Ç·Î
	//°¡±ŞÀû CA2W, CA2TµîÀ» »ç¿ëÇÑ´Ù. ´Ü ÀÌ ¸ÅÅ©·ÎµéÀº encodingÀ» º¯°æÇÒ ¼ö ¾ø´Ù.
	std::wstring CString2wstring(const char* str);
	//std::string ss = CT2CA(CString(_T("test")); °ú °°ÀÌ CT2CA¸¦ »ç¿ëÇÏ¸é °£´ÜÇÔ.
	std::string CString2string(CString cs);
	//CString str(sstr.c_str());
	CString		string2CString(std::string sstr);
	//¸ÖÆ¼¹ÙÀÌÆ® È¯°æ¿¡¼­ ÀÌ ÇÔ¼ö¸¦ È£ÃâÇØ¼­ »ç¿ëÇÏ¸é °£È¤ ºñÁ¤»óÀûÀ¸·Î µ¿ÀÛÇÑ´Ù.
	//¾Æ¸¶µµ ÇÔ¼ö³»¿¡¼­ ¸Ş¸ğ¸®°¡ ÇÒ´çµÈ ÈÄ È£ÃâÇÑ °÷¿¡¼­ »ç¿ëÇÏ·Á´Ï ¹®Á¦°¡ µÉ ¼ö ÀÖ´Ù.
	//ÀÌ ÇÔ¼öÀÇ ¹Ùµğ¸¦ ±×´ë·Î ¾²¸é ¹®Á¦°¡ ¾øÀ¸¹Ç·Î ÀÏ´Ü ¹Ùµğ ÄÚµå¸¦ ±×´ë·Î º¹»çÇØ¼­ »ç¿ëÇÑ´Ù.
	LPCWSTR		CString2LPCWSTR(CString str);
	//char chStr[100] = { 0, };¿Í °°ÀÌ pointer º¯¼ö°¡ ¾Æ´Ñ ¹è¿­·Î ¼±¾ğµÈ °æ¿ì¶ó¸é
	//chStr = CString2char(str); ¹®ÀåÀº ¿À·ù°¡ ¹ß»ıÇÏ¹Ç·Î ¾Æ·¡¿Í °°ÀÌ »ç¿ëÇÒ °Í.
	//sprintf(chStr, "%s", (LPSTR)(LPCTSTR)str);	//MBCS : ok, UNICODE : fail
	//sprintf(chStr, "%s", CStringA(str));		//both : ok
	//¸®ÅÏ¹Ş¾Æ »ç¿ëÇÑ char* º¯¼ö°ªÀº »ç¿ë ÈÄ ¹İµå½Ã delete [] ÇØÁÙ°Í
	char*		CString2char(CString str);
	TCHAR*		CString2TCHAR(CString str);
	LPCSTR		CString2LPCSTR(CString str);
	LPCWSTR		LPCTSTR2LPCWSTR(LPCTSTR str, UINT codePage = CP_UTF8);
	WCHAR*		CString2WCHAR(CString str); //{ return (WCHAR*)(const WCHAR*)CStringW(str); }

	//cstrÀÇ À¯È¿ÇÑ ±æÀÌ¸¦ ÀÌ¹Ì ¾Ë°í ÀÖ´Ù¸é length¸¦ ÁöÁ¤ÇØÁà¾ß Á¤È®ÇÏ´Ù.
	//±×·¸Áö ¾ÊÀ» °æ¿ì cstrÀÇ ³¡¿¡ '\0'°¡ ¾øÀ» °æ¿ì ¾²·¹±â ¹®ÀÚµé±îÁö Æ÷ÇÔµÉ ¼ö ÀÖ´Ù.
	//cstrÀÌ '\0'·Î ³¡³­´Ù¸é À¯´ÏÄÚµå, ¸ÖÆ¼¹ÙÀÌÆ® È¯°æ¿¡¼­ CString str = cstr;·Î Á¤»ó Ã³¸®µÈ´Ù.
	CString		char2CString(char *cstr, int length = -1);
	CString		TCHAR2CString(TCHAR *str);
	VARIANT		CString2VARIANT(CString str);

	CStringA	UTF16toUTF8(const CStringW& utf16);
	CStringW	UTF8toUTF16(const CStringA& utf8);
	CString		UTF8toCString(char* pszCode);
	char*		UTF8toANSI(char* pszCode);
	char*		ANSItoUTF8(char* pszCode);
	CString		utf82CString(std::string inputtext);
	std::string	multibyteToUtf8(std::string inputtext);
	std::string	utf82Multibyte(std::string inputtext);
	std::wstring multibyte2Unicode(std::string inputtext);
	std::string unicode2Multibyte(std::wstring inputtext);
	std::wstring utf8ToUnicode(std::string inputtext);
	std::string unicode2Utf8(std::wstring inputtext);
	std::string multibyteToUtf8(std::string inputtext);
	std::string utf82Multibyte(std::string inputtext);

	std::string	CString2Utf8(CString inputtext);

	//[[nodiscard]] static CStringA W2UTF8(_In_NLS_string_(nLength) const wchar_t* pszText, _In_ int nLength);
	//[[nodiscard]] static CStringW UTF82W(_In_NLS_string_(nLength) const char* pszText, _In_ int nLength);

	//return¹ŞÀº char*´Â ¹İµå½Ã »ç¿ë ÈÄ free()ÇØÁà¾ß ÇÔ.
	TCHAR*		replace(TCHAR* src, const TCHAR* olds, const TCHAR* news);

	//srcÀÇ ³¡¿¡¼­ length ±æÀÌ ¸¸Å­ Àß¶ó³½´Ù.
	CString		truncate(CString &src, int length);
	//src³¡ÀÇ ¹®ÀÚ¿­ÀÌ sub¿Í ÀÏÄ¡ÇÏ¸é Àß¶ó³½´Ù.
	CString		truncate(CString &src, CString sub);

	//°ø¹é, '\t', '\r', '\n', '\0' ¸ğµÎ Á¦°Å
	void		trim(char* src);
	void		trim(std::string &str);
	void		trim_left(std::string& str);
	void		trim_right(std::string& str);
	void		trim(std::deque<CString>* dq);
	//src¹®ÀÚ¿­¿¡¼­ chars¸¦ ¸ğµÎ Á¦°ÅÇÑ´Ù.
	void		remove_chars(CString &src, CString chars);
	//src¹®ÀÚ¿­¿¡¼­ ÁöÁ¤µÈ ±¸°£ÀÇ ¹®ÀÚ¿­À» Á¦°ÅÇÑ´Ù.

	void		make_lower(std::string& str);
	void		make_upper(std::string& str);
	std::string lower(std::string str);
	std::string upper(std::string str);

	inline BYTE toHex(const BYTE &x);
	void		hex2byte(const char *in, int len, uint8_t *out);

	unsigned int dec2bcd(unsigned int num);
	CString		URLEncode(CString sIn);
	CString		Utf8ToStringT(LPSTR str);
	char*		UTF8ToANSI(char *pszCode);
	CString		UrlDecode(LPCTSTR url);

	//void		NewTypingMessageFromCodePage(TCHAR* Message, UINT CodePage=0);


	float		Similarity(char *str1, char *str2);
	float		Similarity(CString str1, CString str2);

	void		SortStringArray(CStringArray* pArray);
	bool		StringArrayCompareAndSwap(CStringArray* pArray, int pos); //called at SortStringArray function.

	template<typename T> CString format_str(T var)
	{
		CString str;
		if (typeid(T) == typeid(int))
			str.Format(_T("%d"), var);
		else if (typeid(T) == typeid(char))
			str.Format(_T("%c"), var);
		else if (typeid(T) == typeid(CString))
			str.Format(_T("%s"), var);
		else if (typeid(T) == typeid(void*))
			str.Format(_T("%p"), var);

		return str;
	}
	
	CString		FormattedString(LPCTSTR lpszFormat, ...);
	int			GetTrimLength(CString str);
	//str¿¡¼­ ¾ÕµÚ ¹®ÀÚ¿­ »çÀÌ¿¡ ÀÖ´Â ¼­ºê ¹®ÀÚ¿­À» ¸®ÅÏÇÑ´Ù. ¾øÀ¸¸é "" ¸®ÅÏ.
	CString		ExtractSubString(CString src, CString sPrev, CString sPost);	//sPrev(ÇÊµå ¾Õ¿¡ ¿À´Â ¹®ÀÚ¿­), sPost(ÇÊµå µÚ¿¡ ¿À´Â ¹®ÀÚ¿­)
	//std::string string_format(const std::string fmt, ...);
	//A safer and more efficient(http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf)
	std::string string_format(const std::string fmt_str, ...);
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

	//simple json parser.
	//ÀÌÀü Koino °³¹ßÀÚ°¡ °£´ÜÈ÷ ¸¸µç ÄÚµåÀÌ³ª ´ë°ıÈ£ Ã³¸® µî Ãë¾àÁ¡ÀÌ ¸¹À½.
	//¶ÇÇÑ ¼ıÀÚÇüÀÇ ÇÊµå°ªµéµµ ¸ğµÎ CStringÀ¸·Î¸¸ returnÇÏ´Â ´ÜÁ¡ÀÌ ÀÖ´Ù.
	//½ÉÇÃÇÑ json¿¡ ´ëÇØ¼­´Â Á¤»ó µ¿ÀÛÇÏ¹Ç·Î ±âÁ¸ ÄÚµå¿ÍÀÇ È£È¯¼º¶§¹®¿¡ ¿ì¼± »ç¿ëÇÏÁö¸¸
	//Common/json/rapid_json ·Î ´ëÃ¼µÇ¾î¾ß ÇÔ.
	//¿ì¼± ¼öÁ¤ÇÏ¿© ¹®ÀÚ¿­ ÇÊµå°ª¿¡ ',', '}' µîÀÌ Æ÷ÇÔµÇ¾îµµ Á¤»ó ÆÄ½ÌµÇµµ·Ï ¼öÁ¤Àº ÇÑ »óÅÂ.
	CString		json_str(CString json, CString key);

	int			get_char_count(CString sStr, TCHAR ch, bool stop_at_first_mismatch = false, bool forward = true);
	CString		get_mac_address_format(CString src, TCHAR separator = ':');

	// templated version of str_icmp so it could work with both char and wchar_t
	template<typename T>
	struct str_icmp {
		str_icmp(const std::locale& loc) : loc_(loc) {}
		bool operator()(T ch1, T ch2) {
			return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
		}
	private:
		const std::locale& loc_;
	};

	// find substring (case insensitive)
	template<typename T> int find_substr(const T& str1, const T& str2, const std::locale& loc = std::locale())
	{
		typename T::const_iterator it = std::search(str1.begin(), str1.end(), str2.begin(), str2.end(), str_icmp<typename T::value_type>(loc));
		if (it != str1.end())
			return it - str1.begin();
		return -1; // not found
	}

	struct std_stricmp
	{
		// case-independent (ci) compare_less binary function
		struct nocase_compare
		{
			bool operator() (const unsigned char& c1, const unsigned char& c2) const {
				return tolower(c1) < tolower(c2);
			}
		};
		bool operator() (const std::string& s1, const std::string& s2) const {
			return std::lexicographical_compare
			(s1.begin(), s1.end(),   // source range
				s2.begin(), s2.end(),   // dest range
				nocase_compare());  // comparison
		}
	};

//¸Ş½ÃÁö ¹Ú½º
	void		msgbox(LPCTSTR format, ...);

//µ¥ÀÌÅÍ º¯È¯
	CString		i2S(int64_t nValue, bool bComma = false, bool fill_zero = false, int digits = 0);
	CString		i2HS(int64_t nValue, bool bCapital = true);
	CString		d2S(double dValue, bool bComma = false, int nfDigit = -1);	//nfDigit : ¼Ò¼öÁ¡ ÀÚ¸´¼ö. -1ÀÌ¸é ±×´ë·Î Ãâ·Â.

	//IPv4 ¹®ÀÚ¿­À» ¼ıÀÚ·Î ¹Ù²Ù´Â ¹ü¿ë ÄÚµåÀÌ¹Ç·Î ¹öÀü ¹®ÀÚ¿­ µî ÀÏ¹İ ¹®ÀÚ¿­ ºñ±³¿¡ »ç¿ëÇÏÁö ¸»°Í.
	//¹öÀü ¹®ÀÚ¿­ µîÀº °¢ ÀÚ¸´¼ö ±¸¼ºÀÌ ´Ù¸¦ ¼ö ÀÖÀ¸¹Ç·Î »ç¿ëÇÒ ¼ö ¾øÀ½.
	//¹®ÀÚ¿­ ºñ±³°¡ ÇÊ¿äÇÏ´Ù¸é compare_string()À» »ç¿ëÇÒ °Í.
	uint32_t	IP2int(CString IP);

	char*		ushortToBinary(unsigned short i);
	char*		intToBinary(int i);
	char*		uintToBinary(unsigned int i);
	int			binaryToInt(char *s);
	unsigned int binaryToUint(char *s);
	//¾î¶² ¼öÀÇ x¹øÂ° ºñÆ®°ª ¸®ÅÏ.
	int			get_bit(int number, int x);
	//x°¡ 1ÀÌ¸é n¹øÂ° °ªÀ» 1·Î º¯°æ, x°¡ 0ÀÌ¸é n¹øÂ° °ªÀ» 0À¸·Î º¯°æ
	void		set_bit(int& number, int n, int x);
	int			HexaStringToInt(CString str);
	CString		GetByteString(uint8_t* bt, int n, bool upper = true, bool prefix = true);
	CString		BinaryToHexString(BYTE* pData, int length, TCHAR separator = _T(' '));
	int			getPrecision(double d, bool bExceptZero = true);	//¼Ò¼öÁ¡ ÀÚ¸´¼ö ¸®ÅÏ
	template<class T> CString get_binary_string(T n, bool separator = true)
	{
		CString res;

		while (n != 0)
		{
			res = (n % 2 == 0 ? '0' : '1') + res;
			n /= 2;
		}

		//4ÀÚ¸´¼ö·Î ¸ÂÃã
		int len = res.GetLength();
		if (len % 4 != 0)
		{
			int count = 4 - len % 4;
			while (count-- > 0)
			{
				res = '0' + res;
			}
		}

		//4ÀÚ¸´¼ö °ø¹é Ãß°¡
		if (separator)
		{
			int pos = 4;
			while (pos < res.GetLength())
			{
				res.Insert(pos, ' ');
				pos += 5;
			}
		}

		return res;
	}

	unsigned __int64 binaryStringToInt64(char *str, int len);
	unsigned int getBitsValueFromInt64(uint8_t* bt, int num_of_bytes, int startbit, int bit_length);
	unsigned int getBitsValueFromInt64(CString *btStr, int num_of_bytes, int startbit, int bit_length);
	unsigned int parse_can_data(unsigned char* data,unsigned int startbit,unsigned int length);				//from ADAS source

	int			getSignedFromUnsigned(unsigned int value, int bit_length);
	unsigned int	Crc16(unsigned char* rdata, unsigned int len);
	uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte);
	uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size);

	//https://ikcoo.tistory.com/213
#ifndef REMOTESDK
	std::string base64_decode(const std::string& in);
	std::string base64_encode(unsigned char const*, unsigned int len);
	std::string base64_encode(const std::string& in);
	CString base64_encode(CString in);
	CString base64_decode(CString in);
#endif

//////////////////////////////////////////////////////////////////////////
//ÆÄÀÏ °ü·Ã
//Æú´õ °ü·Ã
	//_tsplitpath("c:\\abc/def\\123.txt", ...)¸¦ ½ÇÇàÇÏ¸é
	//"c:", "\\abc/def\\", "123", ".txt" °ú °°ÀÌ ºĞ¸®µÇ´Âµ¥ ±âÁ¸¿¡ »ç¿ëÇÏ´ø ±â´ë°ª°ú ´Ş¶ó º¸Á¤ÇÑ´Ù.
	//"c:\\", "c:\\abc/def", "123", "txt", "123.txt¿Í °°ÀÌ º¸Á¤ÇÑ´Ù.
	//part : fn_drive(drive), fn_folder(drive+folder), fn_leaf_folder(folder name), fn_title(filetitle), fn_ext(ext), fn_name(filename)
	//fn_folder´Â path°¡ ÆÄÀÏÀÌµç Æú´õµç ÀüÃ¼ Æú´õ °æ·Î°¡ ÀúÀåµÈ´Ù.
	//¸¸¾à path°¡ "d:\\aaa\\bb"¶ó¸é bb°¡ ÆÄÀÏÀÎÁö Æú´õÀÎÁö ¾Ë ¼ö ¾ø´Ù.
	//remoteÀÇ ÆÄÀÏÀÌ¶ó¸é PathIsFolder()ÇÔ¼ö·Î °Ë»çÇÒ¼öµµ ¾øÀ¸¹Ç·Î
 	//path°¡ fileÀÌ ¾Æ´Ñ Æú´õ¸íÀÌ¶ó°í ÇÏ¸é ¹İµå½Ã È£ÃâÇÒ¶§ºÎÅÍ ¸Ç ³¡¿¡ '\\'¸¦ ºÙ¿©¼­ È£ÃâÇØ¾ß Á¤È®È÷ ºĞ¸®µÈ´Ù.
	CString		get_part(CString path, int part);
	enum FILENAME_PART
	{
		fn_drive,			//µå¶óÀÌºê¸íÀ¸·Î ¹İµå½Ã \·Î ³¡³­´Ù.
		fn_folder,			//fullpath Æú´õ¸íÀÌ¹Ç·Î µå¶óÀÌºê °æ·Î±îÁö ¸ğµÎ Æ÷ÇÔÇÑ´Ù.	
		fn_leaf_folder,		//fullpathÀÇ ¸¶Áö¸· Æú´õ¸í. fullpath°¡ "C:\"ÀÏ °æ¿ì drive, folder, leaf_folder´Â ¸ğµÎ µ¿ÀÏÇÑ °ªÀÎ "C:\"°¡ µÈ´Ù.
		fn_title,			//ÆÄÀÏ Å¸ÀÌÆ²(È®ÀåÀÚ Á¦¿Ü)
		fn_ext,				//ÆÄÀÏ È®ÀåÀÚ(dot Á¦¿Ü)
		fn_name,			//ÆÄÀÏ È®ÀåÀÚ¸¦ Æ÷ÇÔÇÑ ÆÄÀÏ¸í
	};
#if 0
	CString		GetFileNameFromFullPath(CString fullpath);
	CString		GetFolderNameFromFullPath(CString fullpath, bool includeSlash = false);	//= PathRemoveFileSpec
	CString		GetFileTitle(CString fullpath);
	CString		GetFileExtension(CString filename, bool dot = false);
#endif
	int			get_filetype_from_filename(CString filename);
	int			get_filetype_from_extension(CString sExt);

	//ÆÄÀÏ¸í¿¡¼­ È®ÀåÀÚ¸¦ new_ext·Î º¯°æÇÑ´Ù.
	//apply_real_fileÀÌ falseÀÌ¸é filepath ¹®ÀÚ¿­ÀÇ È®ÀåÀÚ¸¸ º¯°æÇÏÁö¸¸ trueÀÌ¸é ½ÇÁ¦ ÆÄÀÏ¸íµµ º¯°æ½ÃÅ²´Ù.
	bool		change_extension(CString& filepath, CString new_ext, bool apply_real_file);
	CString		normalize_path(CString& filepath);

	//"C:\\", "C:\\Temp"¿Í °°ÀÌ ·çÆ®ÀÏ¶§¿Í ÀÏ¹İ Æú´õÀÏ °æ¿ì ³¡¿¡ ¿ª½½·¡½Ã À¯¹«°¡ ´Ù¸£¹Ç·Î ÇÊ¿ä.
	bool		is_drive_root(CString path);
	//src Æú´õ °æ·Î¿¡ sub Æú´õ °æ·Î¸¦ ºÙ¿©ÁÖ´Â ´Ü¼øÇÑ ÇÔ¼öÁö¸¸ µå¶óÀÌºê ·çÆ®ÀÏ¶§¿Í ¾Æ´Ò¶§ µîÀÇ Ã³¸®¶§¹®¿¡ °Ë»çÇÏ¿© °áÇÕÇØÁÖ´Â ¸ñÀûÀ¸·Î Ãß°¡.
	CString		concat_path(CString src, CString sub, TCHAR path_sep = '\\');

	//»õ Æú´õ, »õ Æú´õ (2)¿Í °°ÀÌ Æú´õ³»¿¡ »õ Ç×¸ñÀ» ¸¸µé ¶§ »ç¿ë °¡´ÉÇÑ ÀÎµ¦½º¸¦ ¸®ÅÏÇÑ´Ù.
	//zero_prefix°¡ 2ÀÌ¸é 001, 002·Î µÈ ÀÎµ¦½º°¡ ºÙÀº ÆÄÀÏ/Æú´õµé¸¸ ´ë»óÀ¸·Î ÇÏ·Á ÇßÀ¸³ª ¾ÆÁ÷ ¹Ì±¸Çö.
	int			get_file_index(CString folder, CString title, int zero_prefix = 0);

	//È®ÀåÀÚ ÁıÇÕ ¹®ÀÚ¿­·Î ÆÄÀÏ¿­±â ´ëÈ­»óÀÚÀÇ filter stringÀ» ¸®ÅÏÇÑ´Ù.
	//simple : "bmp;jpg;jpeg;png;webp;gif;yuv;raw => "JPG files|*.jpg|bmp|*.bmp|
	//extension_group = FILE_EXTENSION_VIDEO or FILE_EXTENSION_SOUND or FILE_EXTENSION_IMAGE or FILE_EXTENSION_MEDIA...
	//ÇöÀç ¹Ì¿Ï¼º!
	//CString		get_filter_string(CString extension_group, bool simple = true);

	//Æú´õ¿¡ ÀÖ´Â ÆÄÀÏµé Áß filetitleÀÌ°í extension¿¡ ÇØ´çÇÏ´Â ÆÄÀÏ¸íÀ» ¸®ÅÏÇÑ´Ù.
	std::deque<CString>		get_filelist_from_filetitle(CString folder, CString filetitle, CString extension);
	std::deque<CString>		get_filelist_from_filetitle(CString filename, CString extension);

	uint64_t	get_file_size(CString sfile);
	bool		get_file_size(CString path, ULARGE_INTEGER* ulFileSize);
	ULONGLONG	get_file_size(WIN32_FIND_DATA data);

	uint64_t	get_folder_size(CString path);

	//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
	//unit_string	: ´ÜÀ§¸¦ Ç¥½ÃÇÒ Áö (default = true)
	//Æú´õÀÎ °æ¿ì´Â ""¸¦ ¸®ÅÏÇÔ.
	CString		get_file_size_str(CString sfile, int unit = 1, int floats = 0, bool unit_string = true);
	CString		get_file_size_str(WIN32_FIND_DATA data, int unit = 1, int floats = 0, bool unit_string = true);
	CTime		GetFileCreationTime(CString sfile);
	//Å½»ö±â¿¡¼­ º¹»çÇÏ¸é last modified timeÀÌ À¯ÁöµÇÁö¸¸ web¿¡¼­ ´Ù¿î¹ŞÀº ÆÄÀÏÀÏ °æ¿ì´Â ´Ù¿î¹Ş¾Æ »ı¼ºµÈ ½Ã°¢À¸·Î º¯°æµÊ.
	CTime		GetFileLastModifiedTime(CString sfile);
	CTime		GetFileLastAccessTime(CString sfile);
	CString		GetMostRecentFile(CString sFolder, CString sWildCard = _T("*.*"), int nReturnType = 1);
	CString		GetMostRecentDateFile(CString sFolder, CString sWildCard = _T("*.*"));	//°¡Àå ÃÖ±Ù ³¯Â¥ ÆÄÀÏ¸í ¸®ÅÏ
	CString		GetFileProperty(CString sFilePath, CString sProperty);

	CString		get_file_time_str(FILETIME filetime);


	//È®ÀÎ ÇÊ¿ä
	//CString		set_file_property(CString sFilePath, CString sProperty, CString value);

	//binary file data¸¦ ÀĞ¾îµéÀÎ´Ù. dst´Â Å©±â¸¸Å­ ¹Ì¸® ÇÒ´çµÈ »óÅÂ·Î È£ÃâµÇ¾î¾ß ÇÑ´Ù.
	size_t		read_raw(CString sfile, uint8_t *dst, size_t size);
	bool		save2raw(CString sfile, uint8_t *data, size_t size);
	int			RenameFiles(CString folder, CString oldName, CString newName, bool overwrite = false, bool bWholename = true, bool bRecursive = false);
	bool		delete_file(CString fullpath, bool bTrashCan = false);
	int			get_text_encoding(CString sfile);
	bool		is_utf8_encoding(CString filepath);
	bool		is_utf8_encoding_old(CString filepath);
	//ÆÄÀÏÀ» ÀĞ¾î¼­ CStringÀ¸·Î ¸®ÅÏÇÑ´Ù. max_length < 0ÀÌ¸é ÀüÃ¼ ÆÄÀÏÀ» ÀĞ¾î¼­ ¸®ÅÏÇÑ´Ù.
	//encoding < 0ÀÌ¸é encoding ¹æ½ÄÀ» ÀÚµ¿ ÆÇº°ÇÏ¿© ÀĞ¾î¿Â´Ù.
	CString		read(CString filepath, int max_length = -1, int encoding = -1);
	//text ÆÄÀÏ·Î ÀúÀåÇÑ´Ù.
	//encoding = CP_ACPÀÌ¸é ANSI ÆÄÀÏ·Î, CP_UTF8ÀÌ¸é utf8 Çü½ÄÀ¸·Î ÀúÀåµÈ´Ù.
	//¸¸¾à base64_encodeµÈ text¶ó¸é encoding = CP_ACP, is_binary_data = true·Î ÇØ¾ß ANSI·Î ÀúÀåµÈ´Ù.
	bool		save(CString filepath, CString text, int encoding = CP_UTF8, bool is_binary_data = false);

	//ÅØ½ºÆ® ÆÄÀÏÀ» ¿­ ¶§ ansi, utf-8 µîÀ» ÀÚµ¿À¸·Î ÆÇº°ÇÏ¿© ¿­¾îÁÖ°í encoding ¹æ½ÄÀ» ¸®ÅÏÇÑ´Ù.
	//return value : text_encoding_ansi / text_encoding_utf8 / ....
	int			file_open(FILE** fp, CString mode, CString file);

	//text ÆÄÀÏÀ» ¿­¾î¼­ ¶óÀÎº°·Î ÀĞÀº ÈÄ dqList¿¡ ³Ö¾îÁØ´Ù.
	bool		read_lines(CString filepath, std::deque<CString> *dqList);

	//mp4 ÆÄÀÏÀÇ Æ¯Á¤ ÅÂ±× µ¥ÀÌÅÍ Áß ¿øÇÏ´Â À§Ä¡ÀÇ µ¥ÀÌÅÍ¸¦ ÃßÃâÇÑ´Ù.
	//MOBIS ÇÁ·ÎÁ§Æ® ÀúÀå MP4´Â mdat ÇÊµåÀÇ 0x40¹øÁöºÎÅÍ 28 bytes°¡
	//µ¿¿µ»óÀÌ »ı¼ºµÈ Àı´ë½Ã°£ÀÌ ÀúÀåµÇ¾î ÀÖ´Ù.
	//nÀº 0x3CºÎÅÍ 4¹ÙÀÌÆ®°¡ ±× Å©±âÀÌ´Ù.(28 bytes)
	char*		GetDataFromMP4File(char* sfile, char* sTag, uint8_t tagStart, int tagLength);

	//text, binary¸¦ ±¸ºĞÇÏ°íÀÚ ÇßÀ¸³ª Á¤»ó µ¿ÀÛÇÏÁö ¾Ê°í ÀÖÀ½. ¼öÁ¤ ÇÊ¿ä!
	//sfile
	bool		is_binary(CString sfile);

	void		watch_file_system(CString fullpath);

//////////////////////////////////////////////////////////////////////////
//ÀÎÅÍ³İ ÆÄÀÏ
	DWORD		GetURLFileSize(LPCTSTR pUrl);
	bool		DownloadFile(LPCTSTR pUrl, CString strFileName, bool bOverwrite = TRUE, HWND hWnd = NULL);
	CString		DownloadURLFile(CString sUrl, CString sLocalFileName, HWND hWnd = NULL);
	bool		CheckFileIsURL(CString sURL);
	//check_prefix°¡ trueÀÌ¸é http, https±îÁö Ã¼Å©ÇÑ´Ù. ¹º°¡ Ãë¾àÁ¡ÀÌ ÀÖ´ÂµíÇÏ¿© ¿ì¼± »ç¿ë±İÁö.(https://mathiasbynens.be/demo/url-regex)
	bool		is_valid_url(CString url, bool check_prefix);
	void		GetURLFileInfo(CString sURL, bool &bInURL, bool &bFileType);
	bool		ReadURLFile(LPCTSTR pUrl, CString &strBuffer);
	void		ReadURLFileString(CString sURL, CString &sString);

	bool		parse_url(CString full_url, CString &ip, int &port, CString &sub_url, bool &is_https);

	//urlÀ» È£ÃâÇÏ¿© °á°ú°ªÀ» ¸®ÅÏÇÏ°Å³ª ÁöÁ¤µÈ ·ÎÄÃ ÆÄÀÏ·Î ´Ù¿î·Îµå ÇÑ´Ù.
	//local_file_path°¡ ""ÀÌ¸é °á°ú°ªÀ» ¹®ÀÚ¿­·Î ¸®ÅÏ¹Ş´Â´Ù.
	//local_file_path°¡ ÁöÁ¤µÇ¾î ÀÖÀ¸¸é ÆÄÀÏ·Î ´Ù¿î¹Ş´Â´Ù.
	//(ÀÌ¶§ ¸®ÅÏ°ªÀº "")
	//¸®ÅÏ°ªÀÌ 200ÀÌ ¾Æ´Ò °æ¿ì´Â ¸®ÅÏµÈ ¿¡·¯ÄÚµå¿Í result_str¿¡ ÀúÀåµÈ ¿¡·¯ ¸Ş½ÃÁö¸¦ Á¶ÇÕÇÏ¿© ¿¡·¯ Ã³¸®ÇÑ´Ù.
	//port¸¸ °¡Áö°í http¿Í https¸¦ ±¸ºĞÇÏ´Â °ÍÀº À§ÇèÇÏ¹Ç·Î ¸íÈ®ÇÑ Áö½ÃÀÚ·Î Á¢±ÙÇØ¾ß ÇÑ´Ù.
	//(¹İµå½Ã https·Î Á¢±ÙÇØ¾ß ÇÏ´Â °æ¿ì, port°¡ ±âº»°ªÀÎ 443ÀÌ ¾Æ´Ï¶ó¸é ÁÖ¼Ò¸¦ https://~·Î ¸í½ÃÇÏ¿© È£ÃâÇØ¾ß ÇÑ´Ù)
	//DWORD		request_url(CString &result_str, CString ip, int port, CString sub_url, CString verb = _T("GET"), std::vector<CString> *headers = NULL, CString jsonBody = _T(""), CString local_file_path = _T(""));
	//DWORD		request_url(CString& result_str, CString full_url, CString verb = _T("GET"), std::vector<CString>* headers = NULL, CString jsonBody = _T(""), CString local_file_path = _T(""));
	void		request_url(CRequestUrlParams* params);


	//±âº» ºê¶ó¿ìÀú·Î ¼³Á¤µÈ ºê¶ó¿ìÀú ÀÌ¸§À» ¸®ÅÏÇÏ°í ºÎ°¡ÀûÀ¸·Î °æ·Î, ¹öÀüÀ» ¾òÀ» ¼ö ÀÖ´Ù.
	CString		get_default_browser_info(CString* pPath = NULL, CString* pVersion = NULL);

	//Content-Type: multipart/form-data Çü½ÄÀ» ÀÌ¿ëÇÑ À¥¼­¹ö·ÎÀÇ ÆÄÀÏ Àü¼Û ÇÔ¼ö
	bool		HttpUploadFile(CString url, CString filepath, int chatIndex);

	//¼­¹öÀÇ ÇÑ±Û¸í ÆÄÀÏ¿¡ ´ëÇÑ Ã³¸®¶§¹®¿¡ request_url()ÇÔ¼ö ´ë½Å Ãß°¡ÇÏ¿© Å×½ºÆ® ÇØºÃÀ¸³ª
	//¼­¹öÃøÀÇ ¹®Á¦ÀÎµíÇÏ¿© ¿ì¼± ÀÌ ÇÔ¼ö »ç¿ëÀº º¸·ùÁß...
	bool		HttpDownloadFile(CString url, CString local_path = _T(""));


//webView2 Runtime
	bool		is_WebView2Runtime_installed();
	bool		install_WebView2Runtime(CString runtimeExePath, bool silentInstall);

//////////////////////////////////////////////////////////////////////////
//Æú´õ °ü·Ã
	//°¡´ÉÇÏ¸é PathIsDirectory() »ç¿ëÇÒ °Í
	bool		IsFolder(CString sfile);				//Æú´õÀÎÁö ÆÄÀÏÀÎÁö
	bool		isFolder(char *sfile);
	//ÆÄÀÏ¸íÀÌ³ª Æú´õ¸í¿¡ '\\', '/' È¥¿ëÀÏ °æ¿ì°¡ ÀÖÀ¸¹Ç·Î CStringÀÇ '==' ¿¬»êÀÚ·Î ºñ±³ÇØ¼± ¾ÈµÈ´Ù. 
	bool		IsFileFolderPathIsEqual(CString file0, CString file1, bool bCaseSensitive = false);
	CString		get_parent_dir(CString path, TCHAR path_sep = '\\');	//ÇöÀç Æú´õÀÇ »óÀ§ Æú´õ¸íÀ» ¸®ÅÏÇÑ´Ù.

	//compare_only_filename : fullpath·Î Á¤·ÄÇÒÁö, ÆÄÀÏ¸í¸¸ ÃßÃâÇØ¼­ Á¤·ÄÇÒÁö. default = false;
	void		sort_like_explorer(std::deque<CString> *dq, bool compare_only_filename = false);
	void		sort_like_explorer(std::deque<CString>::iterator _first, std::deque<CString>::iterator _last, bool compare_only_filename = false);

	//ÁöÁ¤µÈ Æú´õ³»ÀÇ ÆÄÀÏ ¸ñ·ÏÀ» ¾ò¾î¿Â´Ù.
	//sNameFilterÀÇ ¿ÍÀÏµåÄ«µå´Â Á÷Á¢ Áà¼­ °Ë»öÇØ¾ß ÇÑ´Ù.
	//ÇÁ·ÒÇÁÆ® ¸í·É°ú µ¿ÀÏÇÏ°Ô ¹°À½Ç¥³ª º°Ç¥¿Í °°Àº ¿ÍÀÏµåÄ«µå¸¦ ÀÌ¿ëÇÒ ¼ö ÀÖ´Ù.
	//sNameFilter = "test*", sExtFilter = "jpg;bmp;" ¿Í °°ÀÌ ÀÔ·ÂÇÏ¸é
	//test·Î ½ÃÀÛÇÏ°í È®ÀåÀÚ°¡ jpg, bmpÀÎ ÆÄÀÏ ¸ñ·ÏÀ» ¾ò¾î¿Â´Ù.
	//sExceptStr = "test;temp;error" ¿Í °°ÀÌ ¼¼¹ÌÄİ·ĞÀ¸·Î ±¸ºĞÇÏ¿© °Ë»ö Á¦¿ÜÇÒ ÆÄÀÏ¸í ÁöÁ¤ °¡´É.
	//ÁÖÀÇ! dqFiles´Â ÀÌ ÇÔ¼ö¿¡ ÀÇÇØ ÃÊ±âÈ­µÇÁö ¾ÊÀ¸¹Ç·Î ÇÊ¿äÇÑ °æ¿ì ÃÊ±âÈ­ÇÏ¿© È£ÃâÇÒ °Í!
	//bRecursiveÀÌ¸é ÇÏÀ§ Æú´õµé ³»ÀÇ ¸ğµç ÆÄÀÏµéµµ °Ë»öÇÑ´Ù. Æú´õ ÀÚÃ¼´Â ¸®½ºÆ®¿¡ Æ÷ÇÔµÇÁö ¾Ê´Â´Ù.
	void		FindAllFiles(	CString sFolder,
								std::deque<CString> *dqFiles,
								CString sNameFilter = _T("*"),
								CString sExtFilter = _T("*"),
								bool bRecursive = false,
								bool include_folder = false,
								CString sExceptStr = _T(""),
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

	//include_folder°¡ trueÀÌ¸é Æú´õµµ ÇÏ³ªÀÇ Ç×¸ñÀ¸·Î ¸®ÅÏÇÏ°í
	//include_folder°¡ falseÀÌ°í recursive°¡ trueÀÌ¸é sub folderµéÀÇ ¸ğµç ÆÄÀÏ¸ñ·ÏÀ» ¸®ÅÏÇÑ´Ù.
	//dqÀÇ cFileName¿¡´Â °Ë»öµÈ Æú´õ ¶Ç´Â ÆÄÀÏÀÇ fullpath°¡ ±â·ÏµÈ´Ù.
	void find_all_files(CString folder, std::deque<WIN32_FIND_DATA>* dq, CString filter = _T("*"), bool include_folder = false, bool recursive = false, bool include_hidden_files = false, bool include_system_files = false);
	void find_all_files(CString folder, std::deque<CString>* dq, CString filter = _T("*"), bool include_folder = false, bool recursive = false, bool include_hidden_files = false, bool include_system_files = false);

	//list¸¦ NULL·Î È£ÃâÇÏ¸é ´ÜÁö sub folderÀÇ °¹¼ö¸¸ ÂüÁ¶ÇÒ ¸ñÀûÀÌ´Ù.
	//root°¡ "³» PC"ÀÏ °æ¿ì special_folders°¡ trueÀÌ¸é ´Ù¿î·Îµå, ³» ¹®¼­, ¹ÙÅÁ È­¸é Ç×¸ñ±îÁö Ãß°¡ÇÑ´Ù.
	//include_files°¡ trueÀÌ¸é ÆÄÀÏµµ Æ÷ÇÔµÈ´Ù.
	int	get_sub_folders(CString root, std::deque<CString>* list = NULL, bool special_folders = false, bool include_files = false);
	//À§ ÇÔ¼ö´Â ÀüÃ¼ ¼­ºê Æú´õÀÇ ¸ñ·ÏÀÌ³ª °³¼ö±îÁö ¸ğµÎ ±¸ÇÏ±â ¶§¹®¿¡ Æ¯Á¤ Æú´õÀÏ °æ¿ì´Â ¼Óµµ°¡ ¸Å¿ì ´À¸®´Ù
	//°£´ÜÈ÷ ¼­ºê Æú´õ À¯¹«¸¸ Ã¼Å©ÇÏ´Â ÇÔ¼ö¸¦ Ãß°¡ÇÑ´Ù.
	bool has_sub_folders(CString root);

	void save_dqlist(std::deque<CString>* dqlist, CString output_text_file_path);

	//À§ÀÇ FindAllFiles¿¡¼­´Â "ÆÄÀÏ¸í*"°ú °°ÀÌ Ã£°Ô µÇ´Âµ¥ ÀÌ·² °æ¿ì ½Ã¸®Áî ÀÌ¸§ÀÇ ´Ù¸¥ ÆÄÀÏµéµµ ¸ğµÎ Ã£¾ÆÁø´Ù.
	//µû¶ó¼­ ÆÄÀÏ¸íÀº È®Á¤µÇ°í È®ÀåÀÚ¸¸ ¿©·¯°¡ÁöÀÎ °æ¿ì´Â ¾Æ·¡ ÇÔ¼ö¸¦ ÀÌ¿ëÇØ¾ß ÇÑ´Ù.
	//(a.jpg, a.pngµîÀ» Ã£°í ½ÍÀºµ¥ a1.jpg°¡ ÀÖ¾îµµ true°¡ µÇ±â ¶§¹®¿¡)
	//(a 00.jpg ~ a 99.jpg = a ??.jpg·Î Ã£À» ¼ö ÀÖ´Ù. Áï, "Å¸ÀÌÆ² ??"ÀÌ°í ?¿¡ ¼ıÀÚ°¡ ¿À¸é ´Ù Ã£¾ÆÁà¾ßÇÒµíÇÑµ¥ ¿ì¼± ÆĞ½º)
	std::deque<CString>		FindFilesWithExtensions(CString folder, CString fileTitle, CString extensions);

	// Æú´õÀÇ ¸ğµç ÆÄÀÏÀ» Áö¿î´Ù.
	int			delete_all_files(CString folder, CString name_filter, CString ext_filter, bool recursive = true, bool trash_can = false);
	bool		DeleteFolder(LPCTSTR lpFolder);
	bool		SHDeleteFolder(CString sFolder);


	//Ç®ÆĞ½º¸¦ ÁÖ¸é Æú´õ¸¦ ÀÚµ¿À¸·Î ¸¸µé¾îÁØ´Ù.
	//Æú´õ°¡ ÀÌ¹Ì Á¸ÀçÇØµµ true¸¦ ¸®ÅÏÇÔ.
	//lpPathNameÀº ¼³·É ±× Çü½ÄÀÌ ÆÄÀÏ¸íÀÌ¶óµµ
	//¹«Á¶°Ç ±× ÀÌ¸§°ú µ¿ÀÏÇÑ Æú´õ¸¦ »ı¼ºÇÏ°Ô µÇ¹Ç·Î
	//¹İµå½Ã »ı¼ºÇÏ°íÀÚ ÇÏ´Â Æú´õ¸íÀ» »ç¿ëÇÒ°Í.
	bool		make_full_directory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpsa = NULL);
	bool		recursive_make_full_directory(LPCTSTR sFolder);

	//Æú´õ³»ÀÇ Æ¯Á¤ ¹®ÀÚ¿­ÀÌ µé¾î°£ ÆÄÀÏµéÀ» Áö¿î´Ù. ÇÏÀ§Æú´õ Áö¿ø¾ÈÇÔ.
	void		DeleteFilesBySubString(CString sFolder, CString filenameSubStr, bool bMatchWholeWordOnly = FALSE, bool bMatchCase = FALSE);

	//void		DeleteAllFiles(CString sFolder);

	//°­Á¦ »èÁ¦ °ü·Ã ÇÔ¼öµé
	//bool		EnableDebugPrivilege(void);
	//bool		CloseRemoteFileHandles(LPCTSTR);
	//DWORD		CloseRemoteHandle(LPCTSTR lpProcessName, DWORD processID, HANDLE handle);
	//bool		DeleteTheFile(LPCTSTR lpFileName);

	//Æú´õ¼±ÅÃ ´ëÈ­»óÀÚ¸¦ ¿¬´Ù.
	//strSelectedFolder : ¼±ÅÃÇÑ Æú´õ
	//strStartFolder : ±âº» ¼±ÅÃµÉ Æú´õ. ½ÃÀÛ Æú´õ.
	bool BrowseForFolder(HWND hwndOwner, TCHAR* lpszTitle, CString& strSelectedFolder, const TCHAR* strStartFolder = NULL, bool bNewFolderButton = true);
	static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM lpData);

	//À©µµ¿ì ¿î¿µÃ¼Á¦¿¡¼­ Æ¯Á¤ Æú´õ(´Ù¿î·Îµå, ³» ¹®¼­ µî)ÀÇ ½ÇÁ¦ °æ·Î¸¦ ¸®ÅÏÇÑ´Ù.
	//FOLDERID_Downloads, FOLDERID_Documents, ...
	CString		get_known_folder(KNOWNFOLDERID folderID);
	CString		get_known_folder(int csidl);

	//»ç¿ëÀÚ ¹®¼­ Æú´õ´Â "C:\Users\user_id\Documents" ¿Í °°ÀÌ À©µµ¿ì ·Î±×ÀÎ °èÁ¤ÀÎ user_id¿¡ µû¶ó °æ·Î°¡ ´Ş¶óÁö¹Ç·Î
	//"__user_documents_folder__"¶ó´Â Å°¿öµå°¡ ÀÖ´Ù¸é ÀÌ¸¦ "C:\Users\user_id\Documents"·Î º¯°æÇØÁØ´Ù.
	CString		adjust_special_folder_path(CString path);

//////////////////////////////////////////////////////////////////////////
//³×Æ®¿öÅ©, ÀÎÅÍ³İ
	bool		GetNICAdapterList(IP_ADAPTER_INFO* pAdapters, int& nTotal, int nMax = 10);
	bool		GetNetworkInformation(CString sTargetDeviceDescription, NETWORK_INFO* pInfo);
	bool		CheckInternetIsOnline();
	bool		IsAvailableEMail(CString sEMail);

	//domain or ip ¹®ÀÚ¿­À» sockaddr_in.sin_addr.S_un.S_addr ¶Ç´Â in_addr.addr.S_un.S_addr °ªÀ¸·Î ¸®ÅÏÇÑ´Ù.
	//½ÇÆĞ ½Ã 0À» ¸®ÅÏÇÑ´Ù.
	ULONG		get_S_addr_from_domain_or_ip_str(CString domain_or_ip_str);
	CString		get_my_ip();
	CString		get_mac_addres(bool include_colon = true);
	CString		get_ip_error_string(DWORD error_code);
	bool		port_is_open(const std::string& address, int port);
	//ip ¶Ç´Â domainÀÇ ÀÏºÎ °ªÀ» '*'·Î Ä¡È¯ÇÑ´Ù.
	CString		get_asterisk_addr(CString ip);
#if 1
	//À©µµ¿ì ³×Æ®¿öÅ© Á¤º¸¿¡ ¼³Á¤µÈ proxy Á¤º¸¸¦ ÀĞ¾î¿Â´Ù. use proxyÀÌ¸é true ¸®ÅÏ.
	void		get_proxy_info(bool& proxy_enable, CString& ip, int& port, CString& bypass, CString& PAC_url);
#endif

//////////////////////////////////////////////////////////////////////////
//¾ÏÈ£È­
	//¹®ÀÚ¿­À» ¾ÏÈ£È­ÇÑ´Ù.
	void		EncryptString(CString& ToCode, TCHAR* key);
	//¾ÏÈ£È­µÈ ¹®ÀÚ¿­À» º¹È£È­ÇÑ´Ù.
	void		DecryptString(CString& ToCode, TCHAR* key);
	//¼ıÀÚ·Î ±¸¼ºµÈ ¹®ÀÚ¿­À» ÀÔ·Â¹Ş¾Æ Á¤ÇØÁø ¿¬»êÀ» ÇÑ ÈÄ µÚ¼¯ÀÎ ¼ıÀÚ¹®ÀÚ¿­À» ¸®ÅÏÇÑ´Ù.
	CString		ShuffleNumericString(CString sSrc, bool bSameLength = true);

	//ÆÄÀÏÀ» ¾ÏÈ£È­ÇÑ´Ù.
	bool		FileEncryption(CString sfile, bool bShowErrorMessage = TRUE);


//////////////////////////////////////////////////////////////////////////
//½©(shell), À©µµ¿ì(window), ·¹Áö½ºÆ®¸®(registry), ½Ã½ºÅÛ(system)
	//MAX_COMPUTERNAME_LENGTH(15) ±æÀÌ±îÁö¸¸ ¸®ÅÏµÊ¿¡ ÁÖÀÇ.
	//GetComputerName API ÇÔ¼ö´Â Ç×»ó ´ë¹®ÀÚ·Î ¸®ÅÏÇÑ´Ù.
	//±×³É È®½ÇÇÏ°Ô GetComputerNameString().MakeLower() µî°ú °°ÀÌ
	//´ë¼Ò¹®ÀÚ¸¦ ¸íÈ®È÷ ÇÏ¿© ºñ±³ÇÏ´Â °ÍÀÌ ÁÁ´Ù.
	//=>255±ÛÀÚ±îÁö ¸®ÅÏµÇ´Â GetComputerNameEx()·Î º¯°æÇÔ.
	CString		get_computer_name_string();
	//¹öÀüÁ¤º¸¸¦ ±¸Á¶Ã¼·Î ¸®ÅÏ
	OSVERSIONINFOEX	get_windows_version();
	//¹öÀüÁ¤º¸¸¦ ¼ıÀÚ¿Í '.'·Î ¸®ÅÏ. ex. "10.0.12345"
	CString		get_windows_version_number();
	DWORD		get_windows_major_version();
	//detail=trueÀÌ¸é edition Á¤º¸±îÁö Æ÷ÇÔ
	CString		get_windows_version_string(bool detail = true);
	//
	//CString		get_windows_version_string(CString version);

	CString		get_system_label(int csidl, int *sysIconIndex = NULL);

	//ÀÌ °ªÀº À©µµ¿ì°¡ ¼³Ä¡µÉ ¶§ »ı¼ºµÇ°í Àç¼³Ä¡µÇÁö ¾ÊÀ¸¸é À¯ÁöµÈ´Ù.
	//´Ü, HDD º¹Á¦½Ã¿¡µµ ±×´ë·Î º¹»çµÇ¹Ç·Î ¸Ó½Å¿¡ µû¶ó uniqueÇÏ´Ù°í º¼ ¼ö ¾ø´Ù.
	CString		read_windows_GUID();
	CString		create_GUID();

	bool		is_VMWare();

	//À©µµ¿ì10ÀÌ»óÀº auto_update°¡ Ç×»ó true.
	//(registry¿¡¼­ Æ¯Á¤°ªÀ» Ãß°¡ÇÏ¿© ¼³Á¤ÇÒ °æ¿ì´Â false·Îµµ ¸®ÅÏµÊ)
	//WinXP¿¡¼­ ÀÚµ¿ ¾÷µ¥ÀÌÆ®¸¦ »ç¿ëÇÏÁö ¾ÊÀ½À¸·Î ÇØµµ trueÀÌ¸ç ¾Æ·¡ level=1ÀÌ ¸®ÅÏµÊ.
	//levelÀº AutomaticUpdatesNotificationLevel ÂüÁ¶.
	//(0:aunlNotConfigured, 1:aunlDisabled, 2:aunlNotifyBeforeDownload, 3:aunlNotifyBeforeInstallation, 4:aunlScheduledInstallation)
	bool		get_windows_update_setting(bool& auto_update, int& level);

	//SystemParametersInfo(SPI_GETSCREENSAVEACTIVE...)À¸·Î´Â Á¦´ë·Î ¼³Á¤°ªÀ» ¾ò¾î¿ÀÁö ¸øÇÑ´Ù.
	bool		get_screensaver_setting(int *timeout = NULL, int* use_secure = NULL);

	//Á» ´õ Å×½ºÆ® ÇÊ¿ä!
	//½ÇÇàÆÄÀÏ¸íÀ¸·ÎºÎÅÍ À©µµ¿ì ÇÚµé ¸®ÅÏ. ½ÇÇàÆÄÀÏ¸í ¶Ç´Â fullpath·Î °Ë»ö.
	HWND		get_hwnd_by_exe_file(CString target_exe_file, DWORD except_pid = 0);
	HANDLE		GetProcessHandleByName(LPCTSTR szFilename);

	CWnd*		FindWindowByCaption(CString sCaption, bool bMatchWholeWord = FALSE);
	HINSTANCE	FindExecutableEx(LPCTSTR lpFile, LPCTSTR lpDir, LPTSTR lpResult);

	//Set Privilege
	bool		set_privilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);

	bool		is_exist_registry_key(HKEY hKeyRoot, CString sSubKey);

	//HKEY_LOCAL_MACHINE\\SOFTWARE\\MyCompany ¿¡¼­ ÀĞ¾î¿Ã °æ¿ì x64ÀÌ¸é ½ÇÁ¦ ±× °æ·Î¿¡¼­ ÀĞ¾î¿ÀÁö¸¸
	//32bitÀÌ¸é HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\MyCompany ¿¡¼­ ÀĞ¾î¿Â´Ù.
	//!!¹İµå½Ã Linker->Manifest File¿¡¼­ AdminÀ¸·Î ºôµåÇÒ °Í!!
	LONG		get_registry_int(HKEY hKeyRoot, CString sSubKey, CString sEntry, DWORD *value);
	//!!¹İµå½Ã Linker->Manifest File¿¡¼­ AdminÀ¸·Î ºôµåÇÒ °Í!!
	LONG		get_registry_str(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString *str);
	//!!¹İµå½Ã Linker->Manifest File¿¡¼­ AdminÀ¸·Î ºôµåÇÒ °Í!!
	LONG		set_registry_int(HKEY hKeyRoot, CString sSubKey, CString sEntry, DWORD value);
	//!!¹İµå½Ã Linker->Manifest File¿¡¼­ AdminÀ¸·Î ºôµåÇÒ °Í!!
	LONG		set_registry_str(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString str);

	//reg_path¿¡ ÇØ´ç value Ç×¸ñÀÌ Á¸ÀçÇÏÁö ¾ÊÀ¸¸é Ãß°¡ÇÑ´Ù.
	//·¹Áö½ºÆ®¸® ÇØ´ç °æ·Î¿¡´Â "count"¿¡ °¹¼ö°¡, ¼ıÀÚ ÀÎµ¦½º Ç×¸ñ¿¡ °¢ °ªÀÌ ÀúÀåµÇ´Â ±¸Á¶·Î ±¸¼ºµÈ´Ù.
	//Ãß°¡µÈ ÀÎµ¦½º¸¦ ¸®ÅÏÇÑ´Ù.
	//ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÏ±â Àü¿¡ ¹İµå½Ã "count"¸¦ ¸®¼Â½ÃÄÑ ÁØ ÈÄ add_registry_str()¸¦ È£ÃâÇØ¾ß ÇÑ´Ù.
	//ex. theApp.WriteProfileInt(_T("setting\\recent files"), _T("count"), 0);
	int			add_registry_str(CWinApp* pApp, CString reg_path, CString str);
	//"count"¿¡ Ç×¸ñÀÇ °¹¼ö°¡ ÀúÀåµÇ¾î ÀÖ°í °¢ ¼ıÀÚ ÀÎµ¦½º Ç×¸ñ¿¡ °ªÀÌ ÀúÀåµÇ¾î ÀÖ´Â ±¸Á¶ÀÎ °æ¿ì ±× ¸ñ·ÏÀ» ¸®ÅÏÇÑ´Ù.
	//Ç×¸ñÀ» Ãß°¡ÇÒ¶§´Â add_registry_str() ÇÔ¼ö¸¦ ÀÌ¿ëÇÑ´Ù.
	int			get_registry_str_list(CWinApp* pApp, CString reg_path, std::deque<CString>& dqlist);

	//Windows visual effect registry
	bool		set_windows_visual_effects();

	double		GetProfileDouble(CWinApp* pApp, CString section, CString entry, double default_value);
	bool		WriteProfileDouble(CWinApp* pApp, CString section, CString entry, double value);

	//ºñÀç±Í¹æ½Ä, key_rootÀÇ ÇÏÀ§¿Í ±× ÇÏÀ§±îÁö¸¸ °Ë»öµÈ´Ù.
	std::deque<CString> get_registry_subkeys(HKEY hKeyRoot, CString key_root);

	//Àç±Í¹æ½ÄÀ¸·Î ¸ğµç ÇÏÀ§ Å°Ç×¸ñÀÌ result·Î ¸®ÅÏµÈ´Ù.
	void		enum_registry_subkeys(HKEY hKeyRoot, CString key_root, std::deque<CString>& result);


#if (_MSVC_LANG >= _std_cpp17)	//__cplusplus ¸ÅÅ©·Î¸¦ »ç¿ëÇÏ·Á¸é C/C++ÀÇ °í±ŞÃ¢¿¡¼­ /Zc:__cplusplus¸¦ Ãß°¡½ÃÄÑ¾ß ÇÑ´Ù.
	//ÇÁ·ÎÁ§Æ® ¼Ó¼º¿¡¼­ std:c++14¸¦ ¼±ÅÃÇÏ°í ºôµåÇÏ¸é 'if constexpr'Àº C++17 ¾ğ¾î È®ÀåÀÔ´Ï´Ù.'¶ó´Â warningÀÌ ¹ß»ıÇÏ´Âµ¥
	//½ÇÁ¦ »ç¿ëÄÚµå¸¦ µ¹·Áº¸¸é Á¤»óÀûÀ¸·Î µ¿ÀÛÇÑ´Ù.
	template <class T> inline T get_profile_value(CString section, CString entry, T default_value)
	{
		CWinApp* pApp = AfxGetApp();

		if constexpr (std::is_same_v<T, int>)
		//if (typeid(T) == typeid(int))			//compile error. C++17 ÀÌÀü ¹öÀü¿¡¼­´Â if constexpr¸¦ »ç¿ëÇÒ ¼ö ¾øÀ¸¹Ç·Î typeid·Î Å¸ÀÔÀ» °Ë»çÇÑ´Ù.
		//if (std::is_integral<T>)				//compile error.
		//if (std::is_same_v<decltype(default_value), int>)		//compile error.
		{
			return pApp->GetProfileInt(section, entry, default_value);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return pApp->GetProfileInt(section, entry, default_value);
		}
		else if constexpr (std::is_same_v<T, double>)
		{
			double* d;
			double res;
			UINT size = sizeof(double);
			pApp->GetProfileBinary(section, entry, reinterpret_cast<LPBYTE*>(&d), &size);

			if (d == NULL || size != sizeof(double))
				res = default_value;
			else
				res = *d;

			delete d;
			return res;
		}
		else if constexpr (std::is_same_v<T, CString>)
		{
			return pApp->GetProfileString(section, entry, default_value);
		}
		else if constexpr (std::is_same_v<T, CRect>)
		{
			CRect* r;
			CRect res;
			UINT size = sizeof(CRect);
			pApp->GetProfileBinary(section, entry, reinterpret_cast<LPBYTE*>(&r), &size);

			if (r == NULL || size != sizeof(CRect))
				res = default_value;
			else
				res = *r;

			delete r;
			return res;
		}
		else if constexpr (std::is_same_v<T, Gdiplus::Rect>)
		{
			Gdiplus::Rect* r;
			Gdiplus::Rect res;
			UINT size = sizeof(Gdiplus::Rect);
			pApp->GetProfileBinary(section, entry, reinterpret_cast<LPBYTE*>(&r), &size);

			if (r == NULL || size != sizeof(Gdiplus::Rect))
				res = default_value;
			else
				res = *r;

			delete r;
			return res;
		}
		else if constexpr (std::is_same_v<T, Gdiplus::RectF>)
		{
			Gdiplus::RectF* r;
			Gdiplus::RectF res;
			UINT size = sizeof(Gdiplus::RectF);
			pApp->GetProfileBinary(section, entry, reinterpret_cast<LPBYTE*>(&r), &size);

			if (r == NULL || size != sizeof(Gdiplus::RectF))
				res = default_value;
			else
				res = *r;

			delete r;
			return res;
		}
		else if constexpr (std::is_same_v<T, SYSTEMTIME>)
		{
			SYSTEMTIME* t;
			SYSTEMTIME res;
			UINT size = sizeof(SYSTEMTIME);
			pApp->GetProfileBinary(section, entry, reinterpret_cast<LPBYTE*>(&t), &size);

			if (t == NULL || size != sizeof(SYSTEMTIME))
				res = default_value;
			else
				res = *t;

			delete t;
			return res;
		}

		return T();
	}


	template <class T> inline void write_profile_value(CString section, CString entry, T value)
	{
		CWinApp* pApp = AfxGetApp();

		CString str;

		if constexpr (std::is_same_v<T, int>)	//if constexprÀº C++17ºÎÅÍ Áö¿øµÇ´Â ±â´ÉÀ¸·Î TÀÇ Å¸ÀÔÀÌ °áÁ¤µÈ ½ÃÁ¡¿¡ µû¶ó ºĞ±âÃ³¸®ÇÒ ¼ö ÀÖ´Ù.
		//if (typeid(T) == typeid(int))			//TÀÇ Å¸ÀÔÀÌ int°¡ ¾Æ´Ò °æ¿ì´Â ÄÄÆÄÀÏ ¿¡·¯ ¹ß»ı. ±×·¯³ª get_list_str(std::deque<T>& list)¿¡¼­´Â ¿¡·¯·Î Ã³¸®µÇÁö ¾Ê´ÂÁ¡ÀÌ ÀÌ»óÇÏ´Ù.
		{
			pApp->WriteProfileInt(section, entry, value);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			pApp->WriteProfileBinary(section, entry, (LPBYTE)&value, sizeof(float));
		}
		else if constexpr (std::is_same_v<T, double>)
		{
			pApp->WriteProfileBinary(section, entry, (LPBYTE)&value, sizeof(double));
		}
		else if constexpr (std::is_same_v<T, CString>)
		{
			pApp->WriteProfileString(section, entry, value);
		}
		else if constexpr (std::is_same_v<T, Gdiplus::Rect>)
		{
			pApp->WriteProfileBinary(section, entry, (LPBYTE)&value, sizeof(Gdiplus::Rect));
		}
		else if constexpr (std::is_same_v<T, Gdiplus::RectF>)
		{
			pApp->WriteProfileBinary(section, entry, (LPBYTE)&value, sizeof(Gdiplus::RectF));
		}
		else if constexpr (std::is_same_v<T, CRect>)
		{
			pApp->WriteProfileBinary(section, entry, (LPBYTE)&value, sizeof(CRect));
		}
		else if constexpr (std::is_same_v<T, SYSTEMTIME>)
		{
			pApp->WriteProfileBinary(section, entry, (LPBYTE)&value, sizeof(SYSTEMTIME));
		}
		else
		{
			static_assert(false, "Unsupported type for write_profile_value");
		}
	}
#endif

	//int			GetSystemImageListIcon(CString szFile, BOOL bDrive);

	//CTreeCtrl, CListCtrlµî¿¡¼­ ¼±ÅÃµÈ Ç×¸ñ ÀÚÃ¼¸¦ ÀÌ¹ÌÁö·Î ¸®ÅÏ(drag½Ã¿¡ »ç¿ë)
	void		get_GdiplusBitmap_from_selected_item(CWnd * pWnd);


	void		SystemShutdown(int nMode);		// 0:logoff  1:reboot  2:shutdown
	void		SystemShutdownNT(int nMode = 2);	// 1:reboot  2:shutdown

	void		HideTaskBar(bool bHide = TRUE);	// FALSE => Show Taskbar

	//taskbarÀÇ Å©±â ¹× ÇöÀç show/hide »óÅÂ¸¦ ¸®ÅÏÇÑ´Ù. (¿É¼Ç ¼³Á¤ »óÅÂ´Â get_taskbar_state()À¸·Î ÆÇº°ÇØ¾ß ÇÔ)
	bool		get_taskbar_size(CSize *sz = NULL);
	//¼³Á¤°ªÀÌ auto hideÀÎÁö´Â get_taskbar_state(ABS_AUTOHIDE)¸¦ È£ÃâÇÏ¿© ÆÇº°.
	//sz¸¦ ÁÖ¸é Å©±âÁ¤º¸µµ ¾Ë ¼ö ÀÖ´Ù.
	//À©7ºÎÅÍ´Â ABS_ALWAYSONTOPÀº Ç×»ó trueÀÌ¹Ç·Î ABS_ALWAYSONTOP¸¦ ÆÇº°ÇÏ´Â °ÍÀº ÀÇ¹Ì¾ø´Ù.
	bool		get_taskbar_state(UINT state, CSize *sz = NULL);

//trayIcon °ü·Ã
#if 1
	struct TRAYDATA
	{
		HWND hwnd;
		UINT uID;
		UINT uCallbackMessage;
		DWORD Reserved[2];
		HICON hIcon;
	};
	bool		refresh_tray_icon();
	HWND		find_tray_toolbar_window();
#else
	//Æ®·¹ÀÌ¿¡ ¾ÆÀÌÄÜ ÀÜ»óÀÌ ³²¾Æ ÀÖ´Â °æ¿ì°¡ ÀÖ´Âµ¥, ÀÌ¸¦ Á¦°ÅÇÏ±â À§ÇØ¼­´Â ¾Æ·¡ÀÇ ÄÚµå·Î ÀÛ¾÷ÇÏ´Â °ÍÀÌ È¿°úÀûÀÌ´Ù.
	//ÀÎÅÍ³İ »ó¿¡¼­ µ¹°í ÀÖ´Â Æ®·¹ÀÌ¾ÆÀÌÄÜÀÇ °¹¼ö¸¦ ±¸ÇØ¼­ Æ®·¹ÀÌ¾ÆÀÌÄÜ¿¡ »èÁ¦ ¸Ş½ÃÁö¸¦ ³¯¸®´Â ¹æ¹ıÀº
	//ÇÁ·Î¼¼½º°¡ ÀÌ¹Ì Á¾·áµÈ »óÅÂ¿¡¼­´Â ÀÛµ¿µÇÁö ¾Ê´Â ¹®Á¦°¡ ÀÖ±â ¶§¹®ÀÌ´Ù.
	void		refresh_tray_icon();
	void		process_notification_area(HWND hWnd);
#endif

	BOOL		IsWow64();
	BOOL		IsXpOr2000();
	BOOL		Is64BitWindows();

	//MOUSEEVENTF¿¡´Â DBLCLKÀÌ ¾ø°í
	//À©µµ¿ì ±âº» ¸Ş½ÃÁö¿¡´Â DBLCLKÀº ÀÖÀ¸³ª CLICKÀº ¾ø´Ù.
	//µû¶ó¼­ event¸¦ »õ·Î Á¤ÀÇÇÏ¿© »ç¿ëÇÑ´Ù.
	enum SC_MOUSE_EVENT
	{
		mouse_event_ldown,
		mouse_event_lup,
		mouse_event_lclick,
		mouse_event_ldbclick,
		mouse_event_rdown,
		mouse_event_rup,
		mouse_event_rclick,
		mouse_event_rdbclick,
		mouse_event_mdown,
		mouse_event_mup,
		mouse_event_mclick,
		mouse_event_mdbclick,
	};
	void		sc_mouse_event(DWORD event, int x = -1, int y = -1);

	//appÀÇ À§Ä¡¿Í Å©±â¸¦ ·¹Áö½ºÆ®¸®¿¡ ÀúÀåÇÏ°í ½ÇÇà ½Ã ´Ù½Ã º¹¿ø½ÃÅ²´Ù.
	//resize°¡ ¾ÈµÇ´Â ´ÙÀÌ¾ó·Î±×ÀÇ °æ¿ì´Â resize½ÃÅ°Áö ¾Ê°í ¿ø·¡ Å©±â·Î Ç¥½ÃÇØ¾ß ÇÑ´Ù.
	//°£È¤ ¸ğ´ÏÅÍ¸¦ ²ø °æ¿ì ¾ÛµéÀÇ À§Ä¡°¡ ¸ÖÆ¼¸ğ´ÏÅÍ À§Ä¡·Î ÀÌµ¿µÇ´Â °æ¿ì°¡ ÀÖ´Âµ¥
	//ÀÌ·² °æ¿ì ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÒ ¶§ force_primary_monitor = true·Î ÇÏ¸é
	//¹«Á¶°Ç primary monitor¿¡ Ç¥½ÃµÇµµ·Ï º¸Á¤µÈ´Ù.
	void		RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""), bool use_maximize = true, bool resize_window = true, bool force_primary_monitor = false);
	void		SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""));


	//CPU Usage °ü·Ã
	LPBYTE		GetPerformanceData(LPTSTR src);
	int			GetCounterValue(const int& objId, const int& counterId, const char* instanceName, PERF_DATA_BLOCK **dataBlock, LONGLONG &value);
	double		get_cpu_usage(const char* process);
	HRESULT		get_cpu_temperature(LPLONG pTemperature);

	//¸Ş¸ğ¸®, memory
	//ÇöÀç °¡¿ë ¸Ş¸ğ¸®¸¦ ¸®ÅÏÇÑ´Ù. (total_memory : ÀüÃ¼ ¸Ş¸ğ¸® ¿ë·®)
	uint64_t	get_available_memory(uint64_t *total_memory = NULL);

#ifndef _USING_V110_SDK71_
	//ÇöÀç ÇÁ·Î¼¼½ºÀÇ ¸Ş¸ğ¸® »ç¿ë·®À» ±¸ÇÑ´Ù.
	SIZE_T		GetCurrentMemUsage();
#endif
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
	bool		memcpy_block(uint8_t *src, int srcx, int srcy, int srcw, int srch, uint8_t *dst, int dstx, int dsty, int dstw, int dsth, int w, int h, int ch);
	bool		memcpy_block(uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, int ch, uint8_t *dst);

//src¿¡¼­ roi ¿µ¿ªÀ» Àß¶ó¼­ dst_width * dst_height Å©±â·Î resize ½ÃÅ²´Ù.
	bool		resize_roi(uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, uint8_t *dst, int dst_width, int dst_height);

//HDD
	ULONGLONG	get_disk_total_size(CString sDrive);
	ULONGLONG	get_disk_free_size(CString sDrive);
	//À§ÀÇ ÇÔ¼ö·Î Å©±â¸¦ ±¸ÇÑ ÈÄ get_size_str()À» ÀÌ¿ëÇÒ °Í
	//CString		GetDiskSizeString(CString sDrive, int unit = 3, int nfDigit = 0);	// "1.25G / 380.00G", nfDigitÀº ¼Ò¼öÁ¡ ÀÚ¸®¼ö
	CString		GetHDDSerialNumber(int nPhysicalDrive);
	CString		GetHDDVolumeNumber(CString sDrive);

	//HDD serialÀº HardwareInfo.exe°¡ ¾Ë·ÁÁÖ´Â´ë·Î S4EVNM0T230338R 15ÀÚ¸® ÇüÅÂ,
	//¶Ç´Â 0025_3852_2190_FE03 °°Àº ÇüÅÂ·Î ¸®ÅÏµÈ´Ù.
	//¾î¶² PC´Â ÅøÀÌ ¾Ë·ÁÁÖ´Â °ª°ú ÀÌ ÇÁ·Î±×·¥¿¡¼­ ±¸ÇÑ °ªÀÌ °°Áö¸¸(mwj, sdh)
	//¾î¶² PC´Â ÅøÀÌ ¾Ë·ÁÁÖ´Â °ª°ú ´Ù¸¥ Æ÷¸ËÀ¸·Î ¸®ÅÏÇÑ´Ù.(scpark)
	//¿Ö ÀÌ·± Â÷ÀÌ°¡ ¹ß»ıÇÏ´ÂÁö´Â ¾ÆÁ÷ ¾Ë ¼ö ¾øÀ¸³ª
	//uniqueÇÑ Å°°ªÀ» »ç¿ëÇÏ°íÀÚ ÇÔÀÌ¹Ç·Î ¿ì¼± 16ÀÚ¸®·Î ¸ÂÃç¼­ »ç¿ëÇÑ´Ù.
	//unify16 = trueÀÌ¸é 16ÀÚ¸®·Î ÅëÀÏ½ÃÄÑ ¸®ÅÏÇÑ´Ù.
	CString		get_HDD_serial_number(int index, bool unify16 = true);

	//µğ½ºÅ© µå¶óÀÌºê ¸ñ·ÏÀ» ¾ò¾î¿Â´Ù. include_legacy = trueÀÌ¸é floppy, cdrom±îÁö ³Ö´Â´Ù.
	void		get_drive_list(std::deque<CString> *drive_list, bool include_legacy = false);
	CString		get_drive_volume(TCHAR drive_letter);

	//µå¶óÀÌºê ÆĞ½º´Â "C:\\"¿Í °°ÀÌ 3°³ ¹®ÀÚ·Î ±¸¼ºµÇ°í Ã¹¹®ÀÚ´Â ´ë¹®ÀÚ·Î Ç¥½ÃÇÏ´Â °ÍÀÌ ÀÏ¹İÀûÀÌ´Ù.
	CString		normalize_drive_path(CString drive_path);

	//"·ÎÄÃ µğ½ºÅ© (C:)" -> "C:\\"
	//"¹®¼­" -> "C:\\Documents"
	//"¹®¼­\\AnySupport" -> "C:\\Documents\\AnySupport"
	//"Seagate(\\192.168.0.52) (X:)" -> "X:"	(³×Æ®¿öÅ© µå¶óÀÌºê)
	//ÇÏÀ§ Æú´õ Æ÷ÇÔ À¯¹«¿¡ °ü°è¾øÀÌ º¯È¯.
	//CString		convert_special_folder_to_real_path(CString special_folder, CShellImageList* plist = NULL, int index = 0);
	//"c:\\abc\\def"				=> "·ÎÄÃ µğ½ºÅ© (C:)\\abc\\def"
	//"C:\Users\scpark\Desktop"		=> "¹ÙÅÁ È­¸é"
	//"C:\Users\scpark\Documents"	=> "¹®¼­"
	//CString		convert_real_path_to_special_folder(CString real_path, CShellImageList * plist = NULL, int index = 0);

	//"c:\windows"¸¦ ÀÔ·ÂÇÏ¸é "C:\Windows"¿Í °°ÀÌ ½ÇÁ¦ ÆÄÀÏ½Ã½ºÅÛ¿¡ ÀúÀåµÈ °æ·Î¸í ¸®ÅÏ.
	CString		get_original_path(CString path);

//ÆÄ¶ó¹ÌÅÍ·Î µé¾î¿Â ¿¬¼ÓµÈ ÆÄÀÏ¸íµéÀ» ºĞ¸®ÇÑ´Ù. ½ÇÇàÆÄÀÏ¸íÀº Á¦¿ÜµÊ.(ex. command line or shell command)
	void		ParseCommandString(CString sParam, CStringArray& ar);

//ÁöÁ¤ÇÑ ÀÌ¹ÌÁö¸¦ ¹ÙÅÁÈ­¸é¿¡ Ç¥½ÃÇÑ´Ù.
#ifndef _USING_V110_SDK71_
	void		set_wallpaper(CString sfile);
#endif

//´ÜÃà¾ÆÀÌÄÜÀ» ¸¸µé¾îÁØ´Ù.
	HRESULT		MyCreateShortCut(LPCTSTR pszSrcFile, LPCOLESTR pszLnkFile,
								 LPTSTR pszWorkingDir = NULL, LPTSTR pszArgument = NULL, LPTSTR pszDesc = NULL);
	HRESULT		CreateShortCut(LPCTSTR pszShortcutFile, LPCOLESTR pszLink,
								LPCTSTR pszWorkingDir = _T(""), LPCTSTR pszArgument = _T(""), LPCTSTR pszDesc = _T(""));

//printer
	int			GetPrinterList(CStringArray *arPrinter);
	CString		GetDefaultPrinterName();
	CSize		GetPrinterPaperSize(CString sPrinterName);

	//system error code¸¦ ¹®ÀÚ¿­·Î ¸®ÅÏ.
	CString		get_error_str(DWORD dwError);

//////////////////////////////////////////////////////////////////////////
//³¯Â¥/½Ã°£ date, time 
	CString		get_date_str(CTime t, CString sep = _T("-"));
	CString		get_date_str(COleDateTime t, CString sep = _T("-"));
	CString		get_date_str(__timeb32 tb, CString sep = _T("-"));
	CString		get_date_str(SYSTEMTIME st, CString sep = _T("-"));

	CString		get_time_str(CTime t, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_str(COleDateTime t, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_str(__timeb32 tb, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_str(SYSTEMTIME st, CString sep = _T(":"), bool h24 = true, bool sec = true, bool msec = true);
	CString		get_time_str(double dSecond, bool bHasHour = true, bool bHasMilliSec = false);

	//185ÃÊÀÏ °æ¿ì "3m 5s"¸¦ ¸®ÅÏÇÑ´Ù.
	CString		get_remain_time_str(int seconds, bool include_sec = true);

	//type 0(date), 1(time:24h), 2(date+time) ³â-¿ù-ÀÏ ½Ã:ºĞ:ÃÊ Çü½ÄÀ¸·Î ÇöÀç ½Ã°£ ¸®ÅÏ. mid´Â ³¯Â¥¿Í ½Ã°£ »çÀÌ ¹®ÀÚ¿­
	CString		get_datetime_str(CTime t, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_str(COleDateTime t, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_str(__timeb32 tb, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_str(SYSTEMTIME st, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true, bool msec = true);

	//type 0(date), 1(time:24h), 2(date+time) ³â-¿ù-ÀÏ ½Ã:ºĞ:ÃÊ Çü½ÄÀ¸·Î ÇöÀç ½Ã°£ ¸®ÅÏ. mid´Â ³¯Â¥¿Í ½Ã°£ »çÀÌ ¹®ÀÚ¿­
	CString		get_cur_datetime_str(int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true, bool msec = false);


	//2003-04-16 18:01:00.120
	CString		GetCurrentTimeString(bool bSeparator = true, bool msec = false);
	CTime		get_CTime_from_datetime_str(CString date, CString time = _T("00:00:00"));
	SYSTEMTIME	get_SYSTEMTIME_from_datetime_str(CString datetime, CString separator = _T(" "));
	SYSTEMTIME	diff_SYSTEMTIME(SYSTEMTIME stStart, SYSTEMTIME stEnd);
	SYSTEMTIME operator-(const SYSTEMTIME& pSr, const SYSTEMTIME& pSl);
	SYSTEMTIME operator+(const SYSTEMTIME& pSr, const SYSTEMTIME& pSl);
	CTimeSpan	GetTimeSpanFromTimeString(CString sTime);
	//CString		GetDateTimeStringFromTime(CTime t, bool bSeparator = true, bool h24 = true, bool include_seconds = true, bool bHasMilliSec = false);
	//CString		GetDateTimeStringFromTime(SYSTEMTIME t, bool bSeparator = true, bool h24 = true, bool include_seconds = true, bool bHasMilliSec = false);
	//CString		GetDateTimeStringFromTime(COleDateTime t, bool bSeparator = true);
	CTime		GetTimeFromDateTimeString(CString sDateTime);
	CString		GetTimeStringFromMilliSeconds(int ms, bool bHasHour = true, bool bHasMilliSec = true);
	int			GetSecondsFromTimeString(CString timeString);
	int			GetMilliSecondsFromTimeString(CString timeString);
	void		GetTimeFromSeconds(int nTotalSeconds, int &nHours, int &nMinutes, int &nSeconds);
	void		SetSystemTimeClock(WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMinute, WORD wSecond);
	double		GetElapsedTime(__timeb32 pOldTime);	//pOldTime°ú ÇöÀç ½Ã°£ÀÇ Â÷ÀÌ °è»ê
	//ts°ªÀ» ³Ñ°Ü ¹Ş¾Æ "aÀÏ b½Ã°£ cºĞ dÃÊ" ÇüÅÂ·Î Ç¥½Ã(format 0 = "?ÀÏ ?½Ã°£ ?ºĞ ?ÃÊ", 1 = "00:00:00") 
	CString		GetDayTimeCountString(int format, CTimeSpan ts, bool bShowZero = true, bool bIncludeSec = true);
	//ts°ªÀ» ³Ñ°Ü ¹Ş¾Æ "aÀÏ b½Ã°£ cºĞ dÃÊ" ÇüÅÂ·Î Ç¥½Ã
	CString		GetDayTimeCountString(COleDateTimeSpan ts, bool bShowZero, bool bIncludeSec);
	time_t		_mkgmtime(const struct tm *tm) ;
	time_t		_mkgmtime(const struct tm* tm);
	bool		IsAM(CTime t = 0);	//t=0ÀÌ¸é ÇöÀç½Ã°¢±âÁØ, 0º¸´Ù Å©¸é ±× ½Ã°£°ª ±âÁØ
	CString		GetDayOfWeekString(CTime t = NULL, bool short_str = false);
	int			GetDaysOfMonth(int nYear, int nMonth);	//ÇØ´ç ´ŞÀÇ ³¯Â¥¼ö ¸®ÅÏ
	//³¯Â¥ °ü·Ã
	int			GetSeasonIndex();	//º½=0, ¿©¸§=1...
	int			gettimeofday(struct timeval* tv, struct timezone* tz);

	//³¯Â¥ Ç¥½Ã Çü½Ä¿¡ ¸Â´ÂÁö °Ë»ç
	bool		is_valid_date(CString str);
	//½Ã°£ Ç¥½Ã Çü½Ä¿¡ ¸Â´ÂÁö °Ë»ç
	bool		is_valid_time(CString str);

	//³¯Â¥½Ã°¢ Çü½ÄÀ» yyyy/mm/dd hh:mm:ss Æ÷¸ËÀ¸·Î ¸ÂÃá´Ù.
	void		normalize_datetime(CString & src);

//Å¸ÀÌ¸Ó °ü·Ã
	void		Wait(DWORD dwMillisecond);		//¿¹Àü¿¡´Â OnTimer() ³»¿¡¼­´Â µ¿ÀÛµÇÁö ¾Ê¾Ò¾ú´Âµ¥ ÇöÀç´Â °¡´ÉÇÏ´Ù.
	//void		usleep(int microSec);
	void		ProcessWindowMessage();			//¹İº¹¹®¿¡ ÀÇÇØ process°¡ ÀÀ´ä¾øÀ½ÀÌ µÇÁö ¾Êµµ·Ï ¹İº¹¹®¾È¿¡¼­ È£ÃâÇÏ¿© ¸Ş½ÃÁöÅ¥ÀÇ ³»¿ëÀ» ¹Ù·Î Ã³¸®½ÃÅ²´Ù.
	int			ptimer_start(int instance);		//reset high resolution time lcounter 
	double		ptimer_get_time(int instance);	//get time(double precision) elaspsed since timer reset in ms
	//unit : ms
	long		getClock();

	extern bool		initialized_YUV_lookup_table;
	void		init_YUV_lookup_table();
	void		yuv420_yv12_to_bgr(unsigned char *src, unsigned char *dst, int w, int h);
	//cv::Mat		yuv420_yv12_to_bgr(uchar *pBuffer,long bufferSize, int width,int height);
	void		yuv420_nv12_to_gray(unsigned char* src, unsigned char* dst, int width, int height);
	void		yuv420_nv12_to_bgr(unsigned char* src, unsigned char* dst, int width, int height);
	void		yuv422_uyvy_to_bgr(unsigned char *src, unsigned char *dst, int w, int h);
	void		yuv_yuyv_to_bgr(unsigned char* src, unsigned char* dst, int w, int h);
	void		bgr_to_yuv422(uint8_t* src, uint8_t* dst, int width, int height, bool yuyv);	//yuyv(true) or uyuv(false)
	//convert 1ch gray to yuv_plain_gray(YYYY....128 128..128 128..)
	void		gray_to_yuv_plain_gray(uint8_t *src, uint8_t *dst, int width, int height);
	void		bgr_to_hsv(uint8_t* src, uint8_t* dst, int width, int height);


//////////////////////////////////////////////////////////////////////////
//GDI
	void		draw_text(CDC* pDC, int x, int y, CString text, COLORREF cr_text);
	void		draw_center_text(CDC* pdc, const CString& strText, CRect& rcRect);

	//Gdiplus
	//Gdiplus::MeasureString()ÀÇ ¿À·ù¸¦ ´ëÃ¼.(http://www.soen.kr/lecture/library/gdiplus/1-7.htm)
	Gdiplus::RectF measure_string(Gdiplus::Graphics* g, Gdiplus::Font& font, LPCTSTR String, int length = -1);

	//Gdiplus¸¦ ÀÌ¿ëÇÑ ÅØ½ºÆ® Ãâ·Â
	CRect		draw_text(Gdiplus::Graphics &g,
							int x, int y, int w, int h,
							CString text,
							float font_size,
							int font_style = Gdiplus::FontStyleRegular,
							int shadow_depth = 0,
							float thickness = 0.0f,
							CString font_name = _T("¸¼Àº °íµñ"),
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							Gdiplus::Color cr_back = Gdiplus::Color::Transparent,
							UINT align = DT_CENTER | DT_VCENTER);

	CRect		draw_text(Gdiplus::Graphics &g,
							CRect rTarget,
							CString text,
							float font_size,
							int font_style = Gdiplus::FontStyleRegular,
							int shadow_depth = 0,
							float thickness = 0.0f,
							CString font_name = _T("¸¼Àº °íµñ"),
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							Gdiplus::Color cr_back = Gdiplus::Color::Transparent,
							UINT align = DT_CENTER | DT_VCENTER);

#ifndef _USING_V110_SDK71_
	CRect		draw_text(ID2D1DeviceContext* d2dc,
							CRect rTarget,
							CString text,
							IDWriteTextFormat* dWriteTextFormat,
							ID2D1Brush* brush,
							int shadow_depth = 0,
							float thickness = 0.0f
							);

	CRect		draw_text(ID2D1DeviceContext* d2dc,
							CRect rTarget,
							CString text,
							CString font_name = _T("¸¼Àº °íµñ"),
							float font_size = 12.0f,
							int font_weight = DWRITE_FONT_WEIGHT_NORMAL,
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							UINT align = DT_CENTER | DT_VCENTER);

	CRect		draw_text(ID2D1DeviceContext* d2dc,
							Gdiplus::RectF rTarget,
							CString text,
							CString font_name = _T("¸¼Àº °íµñ"),
							float font_size = 12.0f,
							int font_weight = DWRITE_FONT_WEIGHT_NORMAL,
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							UINT align = DT_CENTER | DT_VCENTER);
#endif

	//textÀÇ Ãâ·ÂÇÈ¼¿ ³Êºñ°¡ max_width¸¦ ³ÑÀ» °æ¿ì ...¿Í ÇÔ²² Ç¥½ÃµÉ ¹®ÀÚÀ§Ä¡¸¦ ¸®ÅÏ.
	//ÀÌ ÇÔ¼ö´Â DrawText½Ã¿¡ DT_END_ELLIPSIS¸¦ Áà¼­ »ç¿ëÇÏ¹Ç·Î ¿ì¼± »ç¿ë º¸·ù!
	int			get_ellipsis_pos(CDC* pDC, CString text, int max_width);

	//20220914 DrawLine°ú DrawLinePt¸¦ °°Àº ÀÌ¸§À¸·Î ÇÏ´Ï ¸ğÈ£ÇÏ´Ù´Â ¿¡·¯°¡ ¹ß»ıÇÏ¿© DrawLinePt·Î º¯°æ.
	void		draw_line(CDC* pDC, int x1, int y1, int x2, int y2, COLORREF cr, int thick = 1, int style = PS_SOLID, int draw_mode = R2_COPYPEN);
	void		draw_line(CDC* pDC, CPoint pt1, CPoint pt2, COLORREF cr, int thick = 1, int style = PS_SOLID, int draw_mode = R2_COPYPEN);
	void		draw_line(Gdiplus::Graphics& g, int x1, int y1, int x2, int y2, Gdiplus::Color cr, float thick = 1.0f, Gdiplus::DashStyle pen_style = Gdiplus::DashStyleSolid, int nDrawMode = R2_COPYPEN);
	void		draw_line_pt(CDC* pDC, CPoint pt1, CPoint pt2, Gdiplus::Color cr = 0, int width = 1, Gdiplus::DashStyle pen_style = Gdiplus::DashStyleSolid, int draw_mode = R2_COPYPEN);
	void		draw_rect(CDC* pDC, CRect r, COLORREF crColor = RGB(0, 0, 0), COLORREF crFill = NULL_BRUSH, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		draw_rect(CDC*	pDC, CRect r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int width = 1, int pen_align = Gdiplus::PenAlignmentInset, int pen_style = Gdiplus::DashStyleSolid);
	void		draw_rect(Gdiplus::Graphics &g, CRect r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int width = 1, int pen_align = Gdiplus::PenAlignmentInset, int pen_style = Gdiplus::DashStyleSolid);
	void		draw_rect(Gdiplus::Graphics& g, Gdiplus::RectF r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int width = 1, int pen_align = Gdiplus::PenAlignmentInset, int pen_style = Gdiplus::DashStyleSolid);
#ifndef _USING_V110_SDK71_
	void		draw_line(ID2D1DeviceContext* d2dc, int x1, int y1, int x2, int y2, Gdiplus::Color cr, float thick = 1.0f);
	void		draw_rect(ID2D1DeviceContext* d2dc, CRect r, Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, float thick = 1.0f, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);
	void		draw_rect(ID2D1DeviceContext* d2dc, Gdiplus::Rect r, Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, float thick = 1.0f, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);
	void		draw_rect(ID2D1DeviceContext* d2dc, Gdiplus::RectF r, Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, float thick = 1.0f, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);

	//lt, rt, lb, rb ÀÇ round¸¦ °¢°¢ ÁÙ ¼ö ÀÖ´Âµ¥ ltÀÌ¿ÜÀÇ °ªµé Áß ±× °ªÀÌ À½¼öÀÌ¸é lt¿Í µ¿ÀÏÇÑ °ªÀ¸·Î ±×·ÁÁø´Ù.
	ID2D1PathGeometry* draw_rect(ID2D1DeviceContext* d2dc, D2D1_RECT_F r, Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, float thick = 1.0f, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);
	ID2D1PathGeometry* create_round_path(ID2D1DeviceContext* d2dc, float x, float y, float right, float bottom, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);
#endif

	void		draw_sunken_rect(CDC* pDC, CRect rect, bool bSunken = true, COLORREF cr1 = GRAY(96), COLORREF cr2 = GRAY(128), int width = 1);
	void		draw_sunken_rect(CDC* pDC, CRect rect, bool bSunken = true, Gdiplus::Color cr1 = gGRAY(96), Gdiplus::Color cr2 = gGRAY(128), int width = 1);
	void		draw_ellipse(CDC* pDC, CRect r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int pen_style = PS_SOLID, int width = 1, int draw_mode = R2_COPYPEN);
	void		draw_ellipse(Gdiplus::Graphics& g, CRect r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int pen_style = PS_SOLID, int width = 1, int draw_mode = R2_COPYPEN);
	void		draw_ellipse(Gdiplus::Graphics& g, float cx, float cy, float radius, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int pen_style = PS_SOLID, int width = 1, int draw_mode = R2_COPYPEN);
	void		draw_circle(CDC* pDC, int xMidPoint,  int yMidPoint,  int radius);
	void		draw_polygon(CDC* pDC, std::vector<CPoint> pts, bool closed = true, COLORREF crLine = 0, int nWidth = 1, int pen_style = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		draw_arc(CDC *pDC, double cx, double cy,double r1, double r2, double start, double end, int width = 1, int pen_style = PS_SOLID, COLORREF cr = 0, int mode = R2_COPYPEN);
	bool		LoadBitmapFromFile(CBitmap &bmp, CString strFile);
	bool		SaveBitmapToTile(CBitmap* bmp, CString strFile, CWnd* pWnd);
	bool		SaveRawDataToBmp(CString sBmpFile, BYTE* pData, int w, int h, int ch);
	HANDLE		DDBToDIB(CBitmap* bitmap, DWORD dwCompression, CPalette* pPal);
	
	//.ico ¾ÆÀÌÄÜ ÆÄÀÏÀ» Å©±â¸¦ ÁöÁ¤ÇØ¼­ ·ÎµùÀÌ °¡´ÉÇÏ´Ù. LoadIcon()À¸·Î´Â ¾ÈµÇ¸ç PNG¿Í °°Àº ÀÌ¹ÌÁöµµ ºÒ°¡ÇÏ´Ù.
	HICON		load_icon(HINSTANCE hInstance, UINT nID, int cx, int cy = 0);
	//ÇØ´ç DC¿¡ ±×¸®°í ¾ÆÀÌÄÜÀÇ ½ÇÁ¦ Å©±â¸¦ ¸®ÅÏÇÑ´Ù.
	CSize		draw_icon(CDC* pDC, HICON hIcon, CRect r);

	//font size to LOGFONT::lfHeight
	LONG		get_pixel_size_from_font_size(HWND hWnd, int font_size);
	//LOGFONT::lfHeight to font size
	LONG		get_font_size_from_pixel_size(HWND hWnd, int logical_size);

	int			GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	//jpg·Î ÀúÀåÇÒ °æ¿ì quality¸¦ ÁöÁ¤ÇÒ ¼ö ÀÖ´Ù. 0~100 »çÀÌÀÇ °ªÀ¸·Î ÁöÁ¤ÇÏ¸ç, 100ÀÌ °¡Àå Ç°ÁúÀÌ ³ô´Ù.
	//bool		save(Gdiplus::Bitmap* bitmap, CString filepath, int quality = 0);
	//void		save(ID2D1DeviceContext* deviceContext, CString filepath);

//gradient_fillÀ» À§ÇØ¼­ ¼±¾ğµÈ ÀÌ ÇÚµéÀ» »ç¿ëÇÏ´Â ÇÁ·Î±×·¥ÀÌ¶ó¸é
//Á¾·áµÉ ¶§ ÇØÁ¦½ÃÄÑÁÖ´Â ÇÔ¼öµµ ¹İµå½Ã È£ÃâÇØÁà¾ß ÇÑ´Ù.
	typedef UINT (CALLBACK* LPFNDLLFUNC1)(HDC,CONST PTRIVERTEX,DWORD,CONST PVOID,DWORD,DWORD);
	extern HINSTANCE	g_hInst_msimg32;
	extern LPFNDLLFUNC1 g_dllfunc_GradientFill;
	void gradient_rect(CDC* pDC, CRect &rect, std::deque<Gdiplus::Color> dqColor, bool vertical = false);
	enum GRADIENT_RECT_PRESET
	{
		gradient_rect_white_black_white = 0,
		gradient_rect_white_gray128_white,
		gradient_rect_black_white_black,
		gradient_rect_black_gray128_black,
	};
	//¾ÆÁ÷ ¹Ì±¸Çö
	void		gradient_rect(CDC* pDC, CRect &rect, int preset, bool vertical);
	void		safe_release_gradient_rect_handle();

//ÀÌ¹ÌÁö°¡ Ç¥½ÃµÇ°í ÀÖ´Â ¿µ¿ª Á¤º¸¿Í È­¸é»óÀÇ ÁÂÇ¥¸¦ ÁÖ¸é ÀÌ¹ÌÁö»óÀÇ ½ÇÁ¦ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
//´Ü, °è»êµÈ ÀÌ¹ÌÁö»óÀÇ ½ÇÁ¦ ÁÂÇ¥°¡ ÀÌ¹ÌÁö Å©±â¸¦ ¹ş¾î³ª¸é °á°ú º¯¼ö¿¡´Â -1°ªÀ» Ã¤¿ö¼­ ¸®ÅÏÇÑ´Ù.
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, float sx, float sy, float*dx, float*dy);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint *pt_dst);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect *r_dst);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, Gdiplus::RectF r_src, Gdiplus::RectF* r_dst);

	//ÀÌ¹ÌÁö°¡ Ç¥½ÃµÇ°í ÀÖ´Â ¿µ¿ª Á¤º¸¿Í ÀÌ¹ÌÁö »óÀÇ ÁÂÇ¥¸¦ ÁÖ¸é È­¸é»óÀÇ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, float sx, float sy, float*dx, float*dy);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint *pt_dst);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect *r_dst);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, Gdiplus::RectF r_src, Gdiplus::RectF* r_dst);


//Á÷¼±, Line °ü·Ã ÇÔ¼ö
	//lower >= src <= upper ÀÎÁö ÆÇº°
	template <class T> bool is_in_range(T src, T lower, T upper)
	{
		if (src >= lower && src <= upper)
			return true;

		return false;
	}

	//¿øÁ¡¿¡¼­ º¤ÅÍ b°¡ º¤ÅÍ aÀÇ ¹İ½Ã°è ¹æÇâÀÌ¸é ¾ç¼ö, ½Ã°è¹æÇâÀÌ¸é À½¼ö, ÆòÇàÀÌ¸é 0À» ¹İÈ¯ ÇÑ´Ù.
	double ccw(vector2 a, vector2 b);

	//Á¡ p¸¦ ±âÁØÀ¸·Î º¤ÅÍ b°¡ º¤ÅÍ aÀÇ ¹İ½Ã°è ¹æÇâÀÌ¸é ¾ç¼ö, ½Ã°è¹æÇâÀÌ¸é À½¼ö, ÆòÇàÀÌ¸é 0À» ¹İÈ¯ ÇÑ´Ù.
	double ccw(vector2 p, vector2 a, vector2 b);

	// - Á¡ a, b¸¦ Áö³ª´Â Á÷¼±°ú Á¡ c, d¸¦ Áö³ª´Â Á÷¼±ÀÇ ±³Á¡À» x¿¡ ¹İÈ¯ÇÑ´Ù.
	// - µÎ Á÷¼±ÀÌ ÆòÇàÀÌ¸é(°ãÄ¡´Â °æ¿ì Æ÷ÇÔ) °ÅÁşÀ», ¾Æ´Ï¸é ÂüÀ» ¹İÈ¯ÇÑ´Ù.
	bool		GetIntersectionPoint(vector2 a, vector2 b, vector2 c, vector2 d, vector2& x);
	bool		GetIntersectionPoint(CPoint* pt, CPoint* ispt);
	bool		GetIntersectionPoint(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int& isx, int& isy);

	//4Á¡À¸·Î ±¸¼ºµÈ µÎ ¼±ºĞÀÌ ¼­·Î ±³Â÷ÇÏ´ÂÁö ¿©ºÎ¸¦ ÆÇº°ÇÑ´Ù.
	//bCheckCrossOver = falseÀÌ¸é (0,1)°ú (2,3)¶óÀÎÀÇ ±³Â÷¿©ºÎ¸¸, trueÀÌ¸é (0,3)°ú (1,2)ÀÇ ±³Â÷¿©ºÎ±îÁö ÆÇº°ÇÑ´Ù.
	bool		IsIntersect2LineSegment(vector2 a, vector2 b, vector2 c, vector2 d);
	bool		IsIntersect2LineSegment(CPoint* pt, bool bCheckCrossOver);

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


//»ç°¢Çü Rectangle
	//»ç°¢Çü Á¤º¸¸¦ ¹®ÀÚ¿­·Î ¸®ÅÏÇÑ´Ù. (default : 2)
	//0 : "1 2 3 4"
	//1 : "(1,2) ~ (4,8)"
	//2 : "(1,2) ~ (4,8) (2x6)"
	//3 : "1, 2, 3, 4"
	enum RECT_INFO_FORMAT
	{
		rect_info_format_comma = -1,
		rect_info_format_simple,
		rect_info_format_point,
		rect_info_format_point_size,
		rect_info_format_ltrb,
	};
	//CString		get_rect_info_str(CRect r, int nFormat = rect_info_format_point_size);
	//CString		get_rect_info_str(Gdiplus::Rect r, int nFormat = rect_info_format_point_size);
	//CString		get_rect_info_str(Gdiplus::RectF r, int nFormat = rect_info_format_point_size);

	template <typename T> CString get_rect_info_str(T rr, int format = rect_info_format_point_size)
	{
		CString str;
		//CString fs = _T("%%d");

		if (typeid(T) == typeid(CRect))
		{
			CRect* r = reinterpret_cast<CRect*>(&rr);
			if (format == rect_info_format_point)
				str.Format(_T("(%d, %d)~(%d, %d)"), r->left, r->top, r->right, r->bottom);
			else if (format == rect_info_format_point_size)
				str.Format(_T("(%d, %d)~(%d, %d) (%d x %d)"), r->left, r->top, r->right, r->bottom, r->Width(), r->Height());
			else if (format == rect_info_format_ltrb)
				str.Format(_T("l = %d, t = %d, r = %d, b = %d"), r->left, r->top, r->right, r->bottom);
			else
				str.Format(_T("%d, %d, %d, %d"), r->left, r->top, r->right, r->bottom);
		}
		else if (typeid(T) == typeid(Gdiplus::Rect))
		{
			Gdiplus::Rect* r = reinterpret_cast<Gdiplus::Rect*>(&rr);
			if (format == rect_info_format_point)
				str.Format(_T("(%d, %d)~(%d, %d)"), r->X, r->Y, r->X + r->Width, r->Y + r->Height);
			else if (format == rect_info_format_point_size)
				str.Format(_T("(%d, %d)~(%d, %d) (%d x %d)"), r->X, r->Y, r->X + r->Width, r->Y + r->Height, r->Width, r->Height);
			else if (format == rect_info_format_ltrb)
				str.Format(_T("l = %d, t = %d, r = %d, b = %d"), r->X, r->Y, r->X + r->Width, r->Y + r->Height);
			else
				str.Format(_T("%d, %d, %d, %d"), r->X, r->Y, r->X + r->Width, r->Y + r->Height);
		}
		else if (typeid(T) == typeid(Gdiplus::RectF))
		{
			Gdiplus::RectF* r = reinterpret_cast<Gdiplus::RectF*>(&rr);
			if (format == rect_info_format_point)
				str.Format(_T("(%f, %f)~(%f, %f)"), r->X, r->Y, r->X + r->Width, r->Y + r->Height);
			else if (format == rect_info_format_point_size)
				str.Format(_T("(%f, %f)~(%f, %f) (%f x %f)"), r->X, r->Y, r->X + r->Width, r->Y + r->Height, r->Width, r->Height);
			else if (format == rect_info_format_ltrb)
				str.Format(_T("l = %f, t = %f, r = %f, b = %f"), r->X, r->Y, r->X + r->Width, r->Y + r->Height);
			else
				str.Format(_T("%f, %f, %f, %f"), r->X, r->Y, r->X + r->Width, r->Y + r->Height);
		}
		else if (typeid(T) == typeid(D2D1_RECT_F))
		{
			//D2D1::RectF* r = reinterpret_cast<D2D1::RectF*>(&rr);
			D2D1_RECT_F* r = reinterpret_cast<D2D1_RECT_F*>(&rr);
			if (format == rect_info_format_point)
				str.Format(_T("(%f, %f)~(%f, %f)"), r->left, r->top, r->right, r->bottom);
			else if (format == rect_info_format_point_size)
				str.Format(_T("(%f, %f)~(%f, %f) (%f x %f)"), r->left, r->top, r->right, r->bottom, r->right - r->left, r->bottom - r->top);
			else if (format == rect_info_format_ltrb)
				str.Format(_T("l = %f, t = %f, r = %f, b = %f"), r->left, r->top, r->right, r->bottom);
			else
				str.Format(_T("%f, %f, %f, %f"), r->left, r->top, r->right, r->bottom);
		}

		return str;
	}

	void			make_rect(CRect &Rect, int x, int y, int w, int h);
	CRect			make_rect(int x, int y, int w, int h);
	CRect			make_center_rect(int cx, int cy, int w, int h);
	Gdiplus::Rect	make_center_gprect(int cx, int cy, int w, int h);
	D2D1_RECT_F		make_center_d2rect(float cx, float cy, float w, float h);
	CRect			gpRect_to_CRect(Gdiplus::Rect r);
	CRect			gpRectF_to_CRect(Gdiplus::RectF r);
	Gdiplus::Rect	CRect_to_gpRect(CRect r);
	Gdiplus::RectF	CRect_to_gpRectF(CRect r);
	D2D1_RECT_F		CRect_to_d2Rect(CRect r);
	CRect			d2RectF_to_CRect(D2D1_RECT_F r);
	D2D1_RECT_F		gpRectF_to_d2Rect(Gdiplus::RectF r);


	//Gdiplus::RectF´Â right ¶Ç´Â x2°¡ ¾ø°í x(left)¿Í Width ¸â¹öº¯¼ö¸¸ Á¸ÀçÈù´Ù.
	//µû¶ó¼­ left¸¸ ¹Ù²Ù°í ½Í¾îµµ Width±îÁö °°ÀÌ º¯°æÇØÁà¾ß ÇÑ´Ù. ÀÌ·¯ÇÑ ÀÌÀ¯·Î set_left(), set_top() ÇÔ¼ö¸¦ Ãß°¡ÇÔ.
	//CRect´Â left¸¦ º¯°æÇÏ¸é Width()°¡ º¯°æµÇÁö¸¸ Gdiplus::Rect´Â X¸¦ º¯°æÇØµµ Width´Â º¯°æµÇÁö ¾Ê´Â´Ù.
	void		set_left(Gdiplus::RectF& r, Gdiplus::REAL left);
	//CRect´Â topÀ» º¯°æÇÏ¸é Height()°¡ º¯°æµÇÁö¸¸ Gdiplus::Rect´Â Y¸¦ º¯°æÇØµµ Height´Â º¯°æµÇÁö ¾Ê´Â´Ù.
	void		set_top(Gdiplus::RectF& r, Gdiplus::REAL top);
	Gdiplus::PointF center(Gdiplus::RectF& r);

	//stroke thick¸¦ ¹«½ÃÇÏ°í r¿¡ ²ËÂù path¸¦ ±¸ÇÏ¸é right, bottomÀÌ Àß¸®¹Ç·Î stroke_thick¸¦ °í·ÁÇØ¼­ ±¸ÇØ¾ß ÇÑ´Ù.
	void		get_round_rect_path(Gdiplus::GraphicsPath* path, Gdiplus::Rect r, float radius, int stroke_thick = 1);
	void		get_bowl_rect_path(Gdiplus::GraphicsPath* path, Gdiplus::Rect r, float top_radius, float bottom_radius = -1.0f);
	//radius = -1ÀÌ¸é height/2Å©±â·Î ¼³Á¤µÇ°í ÀÌ´Â ¾ç ³¡ÀÌ µÕ±Ù ¿îµ¿Àå Æ®·¢ ¸ğ¾çÀ¸·Î ±×·ÁÁø´Ù.
	Gdiplus::GraphicsPath* draw_round_rect(Gdiplus::Graphics* g, Gdiplus::Rect r, Gdiplus::Color cr_stroke, Gdiplus::Color cr_fill, int radius = -1, int width = 1);
	CRect		getCenterRect(int cx, int cy, int w, int h);
	CRect		get_zoom_rect(CRect rect, double zoom);

	//0:lt, 1:rt, 2:rb, 3:lb, rb_cutÀÌ trueÀÌ¸é ³¡Á¡-1ÀÎ °ªÀ» ¸®ÅÏÇÏ°í falseÀÌ¸é ³¡Á¡ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
	CPoint		vertex(CRect r, int index, bool rb_cut = false);	

	//ÁÖ¾îÁø »ç°¢Çü ¹üÀ§¸¦ ¹ş¾î³ªÁö ¾Êµµ·Ï º¸Á¤ÇØÁØ´Ù.
	void		adjust_rect_range(float *l, float*t, float*r, float*b, float minx, float miny, float maxx, float maxy, bool retainSize, bool includeBottomRight = false);
	//ÀÌ¹ÌÁöÀÇ °æ¿ì includeBottomRightÀº false·Î ÇØ¾ß ³¡Á¡ ÁÂÇ¥°¡ À¯È¿ÇÏ´Ù.(µµÇüÀÏ °æ¿ì´Â true)
	void		adjust_rect_range(CRect& rect, CRect rLimit, bool bRetainSize = true, bool includeBottomRight = false);
	void		adjust_rect_range(CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize = true, bool includeBottomRight = false);
	void		adjust_rect_range(Gdiplus::RectF& rect, Gdiplus::RectF rLimit, bool bRetainSize = true, bool includeBottomRight = false);

	void		normalize_rect(Gdiplus::RectF& r);

	//¸ğ´ÏÅÍÀÇ ÇÑÂÊ¿¡ ºÙÀº »ç°¢ÇüÀ» »õ·Î¿î Å©±â·Î º¯°æÇÒ °æ¿ì ºÙÀº »óÅÂ¸¦ À¯ÁöÇÏ°í º¯°æÇÒ ÇÊ¿ä°¡ ÀÖÀ» °æ¿ì »ç¿ë.
	void		adjust_with_monitor_attached(CRect rOld, CRect &rNew);

	//rTarget¿¡ Á¢ÇÏ´Â dRatio¸¦ À¯ÁöÇÏ´Â ÃÖ´ë »ç°¢ÇüÀ» ±¸ÇÑ´Ù.
	//stretch = false·Î ÁÖ¸é È®´ëÇÏÁö ¾ÊÁö¸¸ Å« °æ¿ì¿¡´Â Ãà¼ÒÇÑ´Ù.
	//CRect		get_ratio_rect(CRect rTarget, double dRatio, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
	CRect		get_ratio_rect(CRect rTarget, int w, int h, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
	//w x h »ç°¢ÇüÀ» target¾È¿¡ ³ÖÀ» ¶§ Áß¾Ó¿¡ Ç¥½ÃµÇ°Ô ÇÏ´Â »ç°¢Çü ¿µ¿ªÀ» ¸®ÅÏÇÑ´Ù.
	//w, hº¸´Ù targetÀÌ ÀûÀ»¶§´Â targetº¸´Ù Å« ¿µ¿ªÀÌ ¸®ÅÏµÉ °ÍÀÌ´Ù.
	CRect		get_center_rect(CRect target, int w, int h);

	CRect		get_rc(HWND hWnd);

	bool		pt_in_rect(CRect r, CPoint pt);
	bool		pt_in_rect(Gdiplus::RectF r, CPoint pt);
	bool		pt_in_rect_border(CRect r, CPoint pt, int sz = 0);
	bool		pt_in_rect_border(Gdiplus::RectF r, CPoint pt, int sz = 0);

	//rSub°¡ rMain¿¡ ¿ÏÀüÈ÷ ¼ÓÇØÀÖÀ¸¸é true¸¦ ¸®ÅÏÇÑ´Ù.
	bool		rect_in_rect(CRect main, CRect sub);
	//r¿¡¼­ except¿µ¿ªÀ» Á¦¿ÜÇÏ°í crÄÃ·¯·Î Ã¤¿î´Ù.
	void		fill_except_rect(CDC* pDC, CRect r, CRect except, COLORREF cr);
	//µÎ »ç°¢ÇüÀÇ °ãÄ¡´Â ¿µ¿ªÀ» ¸®ÅÏÇÑ´Ù.
	CRect		get_intersect_rect(CRect r1, CRect r2);
	CRect		get_intersect_rect(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

	//µÎ »ç°¢ÇüÀÌ °ãÄ¡´Â Á¤µµ¸¦ r1À» ±âÁØÀ¸·Î °è»êÇØ¼­ ¸®ÅÏÇÑ´Ù.
	double		getOverlappedRatio(CRect r1, CRect r2);
	double		getOverlappedRatio(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

	CRect		subtract(CRect r0, CRect r1);

	//½ºÅ©¸°¿¡ Ç¥½ÃµÈ ÀÌ¹ÌÁö¿¡ ±×·ÁÁø »ç°¢ÇüÀÇ ½ÇÁ¦ ÀÌ¹ÌÁö»óÀÇ »ç°¢Çü ÁÂÇ¥
	//sr : ÀÌ¹ÌÁö¿¡ ±×·ÁÁø »ç°¢Çü
	//displayed : ÀÌ¹ÌÁö°¡ Ç¥½ÃµÇ°í ÀÖ´Â »ç°¢Çü ¿µ¿ª
	//real : ½ÇÁ¦ ÀÌ¹ÌÁöÀÇ Å©±â
	//resized : zoom in/out¿¡ ÀÇÇØ º¯°æµÈ Å©±â
	CRect		get_real_from_screen_coord(CRect sr, CRect displayed, CSize real, CSize resized);

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

	void		GetSideRect(CRect src, CRect *side, int margin);
	//src»ç°¢ÇüÀÇ marginÅ©±âÀÇ Å×µÎ¸® ¿µ¿ª¿¡ ptÁ¡ÀÌ Á¸ÀçÇÏ´Â ¿µ¿ªÀÇ ÀÎµ¦½º¸¦ ¸®ÅÏÇÑ´Ù.
	//ÀÎµ¦½º´Â CORNER_INDEXÀÇ Â÷·ÊÀÌ¸ç ÀÌ´Â DefWindowProc¿¡¼­ »ç¿ëÇÏ´Â Â÷·Ê¿Í µ¿ÀÏÇÏ´Ù.
	int			get_corner_index(CRect src, CPoint pt, int margin);

	//½ÃÀÛÁ¡À» ÁÖ¸é Á¤»ç°¢ÇüÀ» ÀÌ·ç´Â ³¡Á¡ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
	void		getSquareEndPoint(int sx, int sy, int& ex, int& ey);

	//src »ç°¢ÇüÀÇ Å©±âÁ¶Á¤ ¹× ÀÌµ¿À» À§ÇÑ 9°³ÀÇ »ç°¢Çü °ªÀ» ¸®ÅÏÇÑ´Ù.
	//handle[]Àº CRect handle[9] º¯¼ö¸¦ ³Ñ°Ü¹Ş´Â´Ù.
	//sz´Â ÇÚµé Å©±â ÇÑ º¯ÀÇ ±æÀÌ°¡ ¾Æ´Ñ 1/2À» ÀÇ¹ÌÇÑ´Ù.
	void		get_resizable_handle(CRect src, CRect handle[], int sz = RECT_RESIZE_HANDLE_SIZE);
	void		get_resizable_handle(Gdiplus::RectF src, CRect handle[], int sz = RECT_RESIZE_HANDLE_SIZE);
	void		get_resizable_handle(Gdiplus::RectF src, std::vector<CRect>* handle, int sz = RECT_RESIZE_HANDLE_SIZE);
	//src »ç°¢ÇüÀÇ Å©±âÁ¶Á¤ ¹× ÀÌµ¿À» À§ÇÑ 9°³ÀÇ »ç°¢Çü Áß pt°¡ À§Ä¡ÇÑ »ç°¢ÇüÀÇ ÀÎµ¦½º¸¦ ¸®ÅÏÇÑ´Ù.
	//ÀÎµ¦½º Á¤ÀÇ´Â enum CORNER_INDEX Á¤ÀÇ¸¦ °øÅëÀ¸·Î »ç¿ëÇÑ´Ù.
	int			get_handle_index(CRect src, CPoint pt, int sz);

//´Ù°¢Çü polygon °ü·Ã
	//ÀÓÀÇ Á¡ÀÌ ´Ù°¢Çü ³»¿¡ Á¸ÀçÇÏ´ÂÁö ÆÇº°.
	bool PtInPolygon(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon0(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon1(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon2(CPoint *ptPolygons, CPoint pt, int nCorners);

	//´Ù°¢ÇüÀÇ ³ĞÀÌ¸¦ ±¸ÇÑ´Ù. ´Ü, º¯ÀÌ ÇÏ³ª¶óµµ ±³Â÷µÇ¸é ¼º¸³ÇÏÁö ¾Ê´Â´Ù.
	double		GetPolygonAreaSize(CPoint *pt, int nPoints);
	//ÁÖ¾îÁø ´Ù°¢Çü Á¡µéÀ» Æ÷ÇÔÇÏ´Â ÃÖ´ë »ç°¢ÇüÀ» ±¸ÇÑ´Ù.
	CRect		get_max_rect(CPoint	*pt, int nPoints);
	CRect		get_max_rect(std::vector<CPoint> pt, int pt_max = -1);


//region °ü·Ã
	HRGN		BitmapToRegion (HBITMAP hBmp, COLORREF cTransparentColor/* = 0*/, COLORREF cTolerance/* = 0x101010*/);
	HRGN		BitmapRegion(HBITMAP hBitmap, COLORREF cTransparentColor, bool bIsTransparent);
	HRGN		CreateRgnFromBitmap(HBITMAP hBmp, COLORREF color);

//Ä¸ÃÄ ±â´É
	//rÀº À©µµ¿ì ÁÂÇ¥°è. jpg, png, bmp¸¸ ÇöÀç Áö¿ø.
	bool		capture_window(CRect r, CString filename);
	//Æ¯Á¤ ¿µ¿ªÀ» Ä¸Ã³ÇÏ¿© HBITMAPÀ¸·Î ¸®ÅÏÇÑ´Ù.
	//resourceID¸¦ ÁÖ¸é ÇØ´ç ÀÌ¹ÌÁö¸¦ overlayÇÏ¿© ¸®ÅÏÇÑ´Ù.(watermark¿Í °°Àº ¿ëµµ·Î »ç¿ë½Ã)
	HBITMAP		capture_screen_to_bitmap(LPRECT pRect, UINT id = 0, int dx = 0, int dy = 0, bool show_cursor = false);
	HBITMAP		CaptureWindowToBitmap(HWND hWnd, LPRECT pRect = NULL);
	HBITMAP		CaptureClientToBitmap(HWND hWnd, LPRECT pRect = NULL);
	//hwnd¸¸ ÁÖ¸é ÇØ´ç À©µµ¿ì ¿µ¿ªÀ» Ä¸Ã³ÇÏÁö¸¸ À©µµ¿ì¿¡¼­´Â ºÒÇÊ¿äÇÑ ¿©¹é±îÁö ¿µ¿ªÀ¸·Î Ã³¸®ÇÏ¹Ç·Î
	//pRect¸¦ Áà¼­ Á¤ÇØÁø ¿µ¿ª¸¸ Ä¸Ã³½ÃÅ²´Ù.
	HBITMAP		PrintWindowToBitmap(HWND hTargetWnd, LPRECT pRect = NULL);

//HBITMAP
	void		draw_bitmap(HDC hdc, int x, int y, HBITMAP hBitmap);
	HRESULT		save_bitmap(HBITMAP bitmap, LPCTSTR filename);

//Å°º¸µå ¾ğ¾î¸¦ ±× ³ª¶ó ±âº»¾ğ¾î·Î º¯°æÇÑ´Ù.
void		ime_convert(HWND hWnd, bool bNative);


//¹®ÀÚÀÔ·ÂÃ¢À» ¼û±ä´Ù.
void		HideIMM(HWND hwnd);


bool		IsLeapYear(int nYear);	//À±³âÀÎÁö ÆÇ´Ü

//CRichEditCtrlEx¸¦ »ç¿ëÇÏ¿© AppendToLogÇÔ¼ö¸¦ ÀÌ¿ëÇÏ´Â ¾ÛÀº ·Î±×¸¦ UI¿¡ Ç¥½ÃÇÏ±â°¡ ÁÁÀ¸³ª
//CRichEditCtrlExÀ» ÀÌ¿ëÇÏÁö ¸øÇÏ´Â »óÈ²ÀÇ ¾ÛÀ¸·Î °ü·Ã ÄÚµåµéÀ» ÀçÀÌ¿ëÇÏ·Á¸é ÄÚµå ¼öÁ¤ÀÌ ÇÊ¿äÇÏ´Ù.
//µû¶ó¼­ ¾Æ·¡ ÇÔ¼ö¸¦ ÀÌ¿ëÇÑ´Ù.
//void		AppendToLog(CWnd* pWnd,)



//UI control °ü·Ã
DWORD		getButtonStyle(HWND hWnd);	//buttonÀÇ Á¾·ù¸¦ ¸®ÅÏÇÑ´Ù.

//¿¬¼ÓµÈ ¹öÆ°µé¿¡ ´ëÇÑ ÀÏ°ı Ã³¸®¿ë ÇÔ¼ö
//±âº» CheckRadioButton°°Àº °æ¿ì´Â unselect ±â´ÉÀ» Á¦°øÇÏÁö ¾Ê±â ¶§¹®¿¡
//¾Æ·¡ ÇÔ¼ö¸¦ »õ·ÎÀÌ Á¤ÀÇÇØ¼­ »ç¿ëÇÑ´Ù.
//id_offsetÀÌ 0º¸´Ù ÀÛÀ¸¸é first ~ last±îÁö ¸ğµÎ Àû¿ë.
void		CheckRadioButtons(CWnd *pWnd, int idFirst, int idLast, int id_offset, int nCheck = BST_CHECKED);

//dialog based¿¡¼­ Å°ÀÔ·ÂÀ¸·Î µ¿ÀÛÀ» Á¤ÀÇÇÏ´Âµ¥ CEdit°ú °°Àº ÀÔ·ÂÃ¢¿¡ Æ÷Ä¿½º°¡ ÀÖÀ¸¸é
//PreTranslateMessage¿¡¼­ ¹æÇâÅ°³ª charÅ°¸¦ Ã³¸®ÇÏ±â°¡ °ï¶õÇÏ´Ù.
//µû¶ó¼­ ÇöÀç Æ÷Ä¿½º¸¦ °¡Áø ÄÁÆ®·ÑÀÌ CEditÀÌ°í enableÀÌ°í readonly°¡ ¾Æ´Ñ °æ¿ì¿¡´Â
//PreTranslateMessage¿¡¼­ ÀÔ·ÂµÈ Å°¸¦ Ã³¸®ÇÏµµ·Ï ÇÑ´Ù.
bool		IsEditCtrlAcceptKeyState(CWnd *pWnd);

//start	: ½ÃÀÛ ÀÎµ¦½º.
//end	: Á¤·ÄÀ» ¿øÇÏ´Â n¹øÂ° Ç×¸ñ
//ex. quicksort(data, 9, 2);¸¦ È£ÃâÇÏ¸é
//2¹ø ÀÎµ¦½ººÎÅÍ 9¹øÂ° Ç×¸ñÀÎ data[2] ~ data[8]±îÁöÀÇ µ¥ÀÌÅÍ°¡ Á¤·ÄµÈ´Ù.
//º¸Åë n°³ÀÇ µ¥ÀÌÅÍ¸¦ Á¤·ÄÇÑ´Ù¸é quicksort(data, n); ÀÌ¶ó È£ÃâÇÏ¸é µÈ´Ù.
template<class T> void quicksort(T& v, int end, int start = 0, bool bAscending = true)
{
	while(end > start)
	{
		int i = start;
		int j = end;

		do
		{
			if (bAscending)
			{
				while((v[i] < v[start]) && (i < j))
					i++;

				//ÇÔ¼ö ¿øÇüÀ» quicksort(T *v, ...)¿Í °°ÀÌ Á¤ÀÇÇØ¼­ »ç¿ëÇÏ¸é ¾Æ·¡ ¹®Àå¿¡¼­ µğ¹ö±ë ¿¡·¯°¡ ¹ß»ıÇÑ´Ù.
				while(v[--j] > v[start])
					;
			}
			else
			{
				while((v[i] > v[start]) && (i < j))
					i++;
				while(v[--j] < v[start])
					;
			}

			if (i < j)
				swap(v[i], v[j]);
		} while(i < j);

		swap(v[start], v[j]);

		if (j - start > end - (j + 1))
		{
			quicksort(v, j - 1, start, bAscending);
			start = j + 1;
		}
		else
		{
			quicksort(v, end, j + 1, bAscending);
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
#ifndef SWAP
template<class T> void SWAP(T& x, T& y)
{
	T temp	= x;
	x		= y;
	y		= temp;
}
#endif

//Å¬¸®ÇÎ ÇÔ¼ö. Å¬¸®ÇÎÀÌ ÀÏ¾î³ª¸é true¸¦ ¸®ÅÏÇÑ´Ù. std::clamp() ±ÇÀå.
template<class T> bool Clamp(T &n, T min, T max)
{
	if (max < min)
		SWAP(min, max);

	if (n < min)
	{
		n = min;
		return true;
	}
	else if (n > max)
	{
		n = max;
		return true;
	}

	return false;
}

//¹üÀ§ ¼øÈ¯ ÇÔ¼ö
template<class T> void Cycle(T& n, T min, T max)
{
	if (n < min) n = max;
	else if (n > max) n = min;
}

//¹üÀ§¸¦ ¹ş¾î³ª¸é default°ªÀ¸·Î ¼¼ÆÃ
template<class T> void Validate(T& n, T min, T max, T default_value)
{
	if (n < min || n > max)
		n = default_value;
}

//Ä¡È¯ ÇÔ¼ö
template<class T> void Swap(T& x, T& y)
{
	T temp	= x;
	x		= y;
	y		= temp;
}

//template<typename ... T> inline T get_max(T ... args)
//{
//	int n = sizeof...(args);
//	int arg[] = { args... };
//}

int compareInteger (const void * a, const void * b);
int compareChar(const void *arg1, const void *arg2);
int compareString (const void * a, const void * b);

template<typename T> void move_item(std::deque<T>& dq, size_t from, size_t to)
{
	if (from == to || from >= dq.size() || to >= dq.size())
		return;

	auto value = std::move(dq[from]);
	dq.erase(dq.begin() + from);
	dq.insert(dq.begin() + to, std::move(value));
}

//////////////////////////////////////////////////////////////////////////
//¼öÇĞ
double		tangentfunc(double x, double y);
//µÎ Á¡ÀÇ °¢µµ¸¦ ±¸ÇÑ´Ù. screencoordÀÏ¶§¿Í Cartesian coordinate(Á÷±³ÁÂÇ¥°è)ÀÏ¶§´Â y°¡ ¹İ´ëÀÓ¿¡ ÁÖÀÇ.
double		GetAngle(double vx, double vy, bool bScreenCoord = true);
double		GetAngle(double x1, double y1, double x2,  double y2, bool bScreenCoord = true);
double		GetAngle(CPoint pt0, CPoint pt1, bool bScreenCoord = true);
double		GetAngle(CPoint a, CPoint b, CPoint c);	//3Á¡ÀÌ ÀÌ·ç´Â °¢µµ
CPoint		GetCenterPoint(CPoint pt0, CPoint pt1);
double		GetDistance(CPoint pt0, CPoint pt1);

//µÎ Á¡À» Áö³ª´Â Á÷¼±»óÀÇ x3 ¶Ç´Â y3¸¦ ±¸ÇÑ´Ù.
double		getLinePointX(double x1, double y1, double x2, double y2, double y3);
double		getLinePointY(double x1, double y1, double x2, double y2, double x3);
double		GetManhattanDistance(double x1, double y1, double x2, double y2);
//ptCenter¸¦ ±âÁØÀ¸·Î dAngle ¸¸Å­ È¸ÀüµÈ dDist°Å¸®ÀÇ Á¡ÀÇ ÁÂÇ¥¸¦ ±¸ÇÑ´Ù.
CPoint		GetRotatedPoint(CPoint ptCenter, double dAngle, double dDist);
//cx, cy¸¦ Áß½ÉÀ¸·Î tx, tyÁ¡ÀÌ degree¸¦ È¸ÀüÇÒ °æ¿ì tx, tyÁ¡ÀÇ º¯°æ ÁÂÇ¥
void		get_rotated(int cx, int cy, int* tx, int* ty, double degree);
std::vector<CPoint>	get_rotated(int cx, int cy, CRect* r, double degree);

//Áöµµ ÁÂÇ¥ <-> µµºĞÃÊ º¯È¯
double		gps_to_double(int d, int m, double s);
void		double_to_gps(double gps, int &d, int &m, double &s);

//src³»ÀÇ ¸ğµç ¹®ÀÚ¿¡ ´ëÇØ digitsÀÚ¸´¼öÀÇ Á¶ÇÕ »ı¼º
void		combination(std::vector<TCHAR> src, CString temp, std::vector<CString>& result, int depth);

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

#include <numeric>
template<typename T> double standardDeviation(std::deque<T> v) 
{
	T sum = std::accumulate(v.begin(), v.end(), 0.0);
	double mean = sum / v.size();

	double squareSum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
	return sqrt(squareSum / v.size() - mean * mean);
}

double		Rounding(double x, int digit);
//´ë°¢¼±ÀÇ ±æÀÌ·Î °¡·Î, ¼¼·Î Å©±â¸¦ ±¸ÇÑ´Ù.
void		get_HV_angle_from_diagonal(double diagonal, double *h, double *v, double width, double height);
void		get_HV_angle_from_diagonal(int diagonal, int *h, int *v, int width, int height);

//numlock, capslock, scrolllock
bool		GetLockKeyState(BYTE nLockKey);
void		SetLockKeyState(BYTE nLockKey, bool bOn);

HBITMAP		MakeDIBSection(CDC& dc, int width, int height);

//2D ´ÜÀÏ ¿µ»ó¿¡¼­ ÀÌ¹Ì ¾Ë·ÁÁø ¼³Á¤°ªÀ» ±âÁØÀ¸·Î ¿µ»ó³»ÀÇ ÇÑ Á¡°ú ·»Áî¿ÍÀÇ °Å¸®¸¦ °è»ê(by sucwon)
//cam_height	: Ä«¸Ş¶ó ¼³Ä¡ ³ôÀÌ. ´ÜÀ§ cm
//fl_x, fl_y	: focal length
//c_x, c_y		: ÁÖÁ¡
double		getObjectDistance(	int width, int height, int vanishing_y, int x, int y, int cam_height, double *dx, double *dy,
								int cali_width = 1920, int cali_height = 1080,
								double fl_x = 2361.130, double fl_y = 2357.436);

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
void		resize11(int* input, int* output, int sourceWidth, int sourceHeight, int targetWidth, int targetHeight);

void		gaussian_blur(uint8_t *image, int width, int height);

double		scv_image_mean(uint8_t *src, int width, int height);

//minValue : minimum value for thresholding.
void		scv_image_threshold(uint8_t *src, int width, int height, int threshold, int minValid = 0, bool invert = false);

void		scv_absdiff(uint8_t *src1, uint8_t *src2, uint8_t *dst, int w, int h);

bool		scv_subImage(uint8_t* src, uint8_t* dst, int source_width, int source_height, int startx,int starty, int cut_width, int cut_height, int ch);

int			scv_countNonZero(uint8_t *src, int w, int h);

//ÇöÀç´Â 1Ã¤³Î ¿µ»ó¸¸ Áö¿øµÈ´Ù.
//¿ì¼± ±ŞÇÏ°Ô -90µµ¸¸ ±¸ÇöÇÑ´Ù.
//dst´Â ¹İµå½Ã ¸Ş¸ğ¸®°¡ ÇÒ´çµÇ¾î ÀÖ¾î¾ß ÇÑ´Ù.
void		rotate90(uint8_t *src, int width, int height, uint8_t *dst, int degree);

//opencvÀÇ flip°ú °°Àº µ¿ÀÛÀÌÁö¸¸ ÀÌ¸§ Ãæµ¹À» ÇÇÇÏ±â À§ÇØ mirror¶ó´Â ÀÌ¸§À» »ç¿ëÇÔ.
//ÇöÀç´Â 1Ã¤³Î ¿µ»ó¸¸ Áö¿øµÈ´Ù.
//method : 0(flip vertical), +(flip horizontal), -(both)
//dst´Â ¹İµå½Ã ¸Ş¸ğ¸®°¡ ÇÒ´çµÇ¾î ÀÖ¾î¾ß ÇÑ´Ù.
void		mirror(uint8_t *src, int width, int height, uint8_t *dst, int method);


//MFC Common Controls
BOOL		recreate_combobox(CComboBox* pCombo, LPVOID lpParam = NULL);



//Æ¯Á¤ ¸ñÀûÀÇ ÇÔ¼öµé
//Â÷·®Á¤º¸ÆÄÀÏ(xml) ÀúÀå °ü·Ã
bool		SavePlateInfoFile(char* sfile, char* sPlate, RECT* rect = NULL);

void		printMessage(std::string msg, uint8_t bNewLine = true);

int readFilenames(std::vector<std::string> &filenames, const std::string &directory);

//CMenu
//HMENU¿¡¼­ ¸Ş´ºID¿Í Ä¸¼ÇÀ» ¾ò¾î¿Â´Ù.
bool	get_menu_item_info(HMENU hMenu, UINT uItem, UINT *uID, CString *caption, BOOL fByPos = FALSE);
//#endif

template <class CharT>
constexpr auto GetFormatMessageFunction()
{
	if constexpr (std::is_same_v<CharT, char>)
	{
		return &FormatMessageA;
	}
	else
	{
		return &FormatMessageW;
	}
}

template <class CharT = char>
std::basic_string<CharT> GetSystemErrorMesssage(const DWORD errorCode)
{
	const auto formatMessageFunction = GetFormatMessageFunction<CharT>();

	DWORD langId;
	if (0 == GetLocaleInfoEx(LOCALE_NAME_SYSTEM_DEFAULT,
		LOCALE_ILANGUAGE |
		LOCALE_RETURN_NUMBER,
		reinterpret_cast<LPWSTR>(&langId),
		sizeof(langId) / sizeof(wchar_t)))
	{
		langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
	}

	std::basic_string<CharT> result;
	for (;;)
	{
		CharT* msg;
		const auto msgLen = formatMessageFunction(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			errorCode,
			langId,
			reinterpret_cast<CharT*>(&msg),
			0,
			nullptr);
		if (msgLen != 0)
		{
			try
			{
				result.assign(msg, msgLen);
			}
			catch (...)
			{
				LocalFree(msg);
				throw;
			}
			LocalFree(msg);
			break;
		}
		const auto primaryLangId = PRIMARYLANGID(langId);
		if (primaryLangId == LANG_NEUTRAL)
		{
			break;
		}
		const auto subLangId = SUBLANGID(langId);
		if (subLangId == SUBLANG_NEUTRAL)
		{
			langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
		}
		else if (subLangId == SUBLANG_DEFAULT)
		{
			langId = MAKELANGID(primaryLangId, SUBLANG_NEUTRAL);
		}
		else
		{
			langId = MAKELANGID(primaryLangId, SUBLANG_DEFAULT);
		}
	}
	for (auto charIndex = result.size() - 1; charIndex != MAXSIZE_T; charIndex--)
	{
		if (result[charIndex] != '\n' && result[charIndex] != '\r')
		{
			break;
		}
		result.resize(charIndex);
	}
	return result;
}

D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float ratio, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float width, float height, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
