#pragma once

//#ifdef DPSAPI_VERSION
//#undef DPSAPI_VERSION
//#endif
//
//#define DPSAPI_VERSION 1

//#ifndef _SCPARK_FUNCTIONS_H
//#define _SCPARK_FUNCTIONS_H

/*
MFC의 어떤 클래스를 상속받아 만든 다른 cpp, h파일과는 달리
Common 폴더에 존재하는 일반 cpp, h파일을 만들었다면
MFC의 클래스 등을 사용하기 위해서는 Afxwin.h 등 몇 개의 h파일들을 include 시켜줘야 한다.
예전에는 그냥 cpp 파일에 #include "stdafx.h"라고만 해도 h, cpp파일에서
MFC 클래스들을 사용할 수 있었으나 Common 폴더에서 프로젝트 폴더에 있는 저 파일을 인식할 리 없다.
프로젝트 세팅에서 "Precompiled Header"가 "Use"로 되어 있어서 동작했다고 생각되지만
IntelliSense에 의해 #include "stdafx.h" 문구에 빨간색 에러 표시가 뜨고
다른 코드들에도 빨간색 에러로 표시되면서 툴팁 정보가 표시되지 않는 불편함이 생겼다.
이를 정석적으로 해결하는 방법은 MFC 클래스 및 확장 클래스들을 사용하기 위해서는
h파일에 아래와 같이 필요한 h파일들을 include하고
cpp파일은 프로젝트 세팅에서 "Precompiled Header"를 사용안함으로 설정하는 것이라고 생각된다.
[MFC 헤더와 라이브러리 설명 참조]
http://www.devpia.com/MAEUL/Contents/Detail.aspx?BoardID=51&MAEULNo=20&no=567
*/
#include <Afxwin.h>
#include <Afxdisp.h>

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string>
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
#include <atomic>
#include <chrono>
#include <mutex>

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

//프로젝트가 유니코드 문자 집합을 사용할 때 fopen()시 UTF-8로 저장하고
//멀티바이트 문자 집합이면 ANSI로 저장되도록 한다.
//단, UTF-8로 저장시 BOM 문자가 헤더에 기록되므로 파일을 쓰기용으로 생성한 후
//fseek를 이용해서 헤더를 무시해야만 해당 파일이 UTF-8 without BOM으로 생성된다.
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

//20231101 opencv에 trace가 이미 정의되어 있어서 trace를 Trace로 변경함.
//매크로로 정의되어 그런지 간혹 비정상적으로 출력되는 현상이 있다.
//일단, thread 내부에서 사용하는 일부 프로젝트에서는 오류가 발생하므로 주의할 것.
//https://stackoverflow.com/questions/3211463/what-is-the-most-efficient-way-to-make-this-code-thread-safe
#define Trace(fmt, ...) trace_output(false, __function__, __LINE__, false, fmt, ##__VA_ARGS__)
#define Traceln(fmt, ...) trace_output(false, __function__, __LINE__, true, fmt, ##__VA_ARGS__)
#define Trace_only(fmt, ...) trace_output(true, __function__, __LINE__, true, fmt, ##__VA_ARGS__)
#define Trace_func() trace_output(false, __function__, __LINE__, false, _T("%s\n"), __function__)

#ifndef _USING_V110_SDK71_
#define traceonly TRACE(_T("%s(%d) current clock = %ld\n"), __function__, __LINE__, GetTickCount64());
#endif

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
inline std::basic_ostream<TCHAR>& operator<<(std::basic_ostream<TCHAR>& os, const Gdiplus::Rect& r)
{
	return os << _T("(") << r.X << _T(", ") << r.Y << _T(", ") << r.GetRight() << _T(", ") << r.GetBottom() << L")";
}
inline std::basic_ostream<TCHAR>& operator<<(std::basic_ostream<TCHAR>& os, const Gdiplus::RectF& r)
{
	return os << _T("(") << r.X << _T(", ") << r.Y << _T(", ") << r.GetRight() << _T(", ") << r.GetBottom() << L")";
}

template<typename T> void trace_impl(const TCHAR* func, int line, const TCHAR* var_name, const T& value)
{
	std::basic_ostringstream<TCHAR> oss;

	oss << func << _T("(") << line << _T(")") << _T(" ") << var_name << _T(" = ") << value;
	TRACE(_T("%s\n"), oss.str().c_str());
}

//trace(n);으로 호출하면 "n = %d\n"를 출력한다. 타입에 따라 %d, %f, %s 등이 자동으로 결정된다.
//정수, 실수, 문자열, CPoint, CRect를 지원하며 그 외 타입은 << 연산자를 오버로딩해서 지원할 수 있다.
#define trace(n) trace_impl(__function__, __LINE__, _T(#n), n)

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

//위의 데이터 타입을 #define uchar unsigned char 과 같이 선언하면
//다른 define들과 충돌한다. 또한 아래와 같은 이유로라도
//타입의 정의는 #define 대신 typedef을 이용하자.
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

//배열의 갯수 리턴
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



//HRESULT를 결과로 리턴받는 함수들에 사용.
//#define		_M(exp) (([](HRESULT hr) { if (FAILED(hr)) /*_com_raise_error(hr);*/ return hr; })(exp));
//hr과 함수식을 넘겨주면 hr에 그 처리결과가 들어가고 에러일 경우 에러를 표시한다.
#define		_M(hr, exp) {{ if (FAILED(hr = (exp))) TRACEHR(hr); }}

typedef void (WINAPI* PGNSI)(LPSYSTEM_INFO);
typedef BOOL(WINAPI* PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

//기존 g_monitors는 모니터 영역만 저장했으나 좀 더 필요한 정보가 있어 이를 class로 확장함
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
	CRect			rMonitor;	//mi.rcMonitor가 LPRECT라서 사용 편의성을 위해 추가
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
#define		DOWNLOAD_SET_TOTAL_SIZE	1	//전체 다운받을 파일 크기
#define		DOWNLOAD_SET_FILE_SIZE	2	//현재 다운받을 파일 크기
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

//소수점 n+1 자리에서 반올림하여 소수점 n자리로 표현한다.
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

//num보다 큰 n의 배수로 만들어준다.	//old : MAKE4WIDTH_D()도 MAKE_MULTIPLY_U()를 이용할 것!
#define		MAKE_MULTIPLY_UP(num, n)		(((num) + ((n)-1)) & ~((n)-1))
//num보다 작은 n의 배수로 만들어준다.
#define		MAKE_MULTIPLY_DOWN(num, n)		(((num) - ((n)-1)) & ~((n)-1) | (n))

extern		int			g_nDaysOfMonth[12];

//각 언어에 사용되는 주석마크
extern		std::deque<CString>		g_comment_mark;

//serial port 관련
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

//trace(m_input);을 호출하면 TRACE(_T("m_input = %d\n"));과 같이 동작한다.
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
		res.Format(_T("카메라 제조사: %s\n카메라 모델명: %s\n소프트웨어: %s\n촬영 시각: %s\n플래시: %s\n초점 거리: %.1f mm\n35mm 환산: %.1f\n")\
			_T("노출 시간: 1/%d sec\n노출 보정: %.2f EV\n조리개 값: f/%.1f\nISO 감도: %d"),
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
			iso_speed);

		CString orientation_info;
		if (!orientation_str.IsEmpty())
		{
			orientation_info.Format(_T("\n회전 정보: %s"), orientation_str);
			res += orientation_info;
		}

		CString gps_info;
		if (!gps_latitude_str.IsEmpty() || !gps_longitude_str.IsEmpty())
		{
			gps_info.Format(_T("\nGPS 정보: %s, %s, %.0fm"), gps_latitude_str, gps_longitude_str, gps_altitude);
			res += gps_info;
		}

		return res;
	}
};

/*
* 	서버정보가 full_url이나 ip, port로 나눠져있어도 그에 맞는 생성자함수를 호출하여 채움.
	CRequestUrlParams params(m_server_ip, m_server_port, _T(""), _T("GET"));

	//만약 sub_url이 필요할 경우 아래와 같이 채워주고
	params.sub_url.Format(_T("/lmm/api/v1.0/temp_envcheck/config-value-return?input_type=flag_windows_auto_update&mgrid=%s"), m_login_id);

	//실제 request를 호출한다. thread방식이 아니면 바로 결과가 params에 채워져서 리턴된다.
	request_url(&params);
	...
	if (params.status == HTTP_STATUS_OK)
	...
*/

class CRequestUrlParams;

//request_url() 내부에서 또는 단독적으로 서버가 현재 기동중인지를 빠르게 판별하기 위한 함수
bool		is_server_reachable(CString ip, int port, int timeout_ms = 1000);

// 서버 도달 가능 여부를 공유 캐시로 관리하는 클래스
// 529행 그대로
class CRequestUrlParams;

class CServerReachabilityCache
{
public:
	CServerReachabilityCache(int check_interval_ms = 10000)
		: m_check_interval_ms(check_interval_ms)
		, m_reachable(true)
		, m_last_check_time(0)
		, m_checking(false)
	{
	}

	bool is_reachable(CString ip, int port, int timeout_ms = 1000)
	{
		auto now = std::chrono::steady_clock::now().time_since_epoch();
		long long now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
		long long last = m_last_check_time.load();

		if (now_ms - last >= m_check_interval_ms)
		{
			bool expected = false;
			if (m_checking.compare_exchange_strong(expected, true))
			{
				bool result = is_server_reachable(ip, port, timeout_ms);
				bool was_reachable = m_reachable.exchange(result);

				if (was_reachable && !result)
					cancel_all_pending();

				m_last_check_time.store(now_ms);
				m_checking.store(false);
			}
		}

		return m_reachable.load();
	}

	bool is_reachable_cached() const
	{
		return m_reachable.load();
	}

	void update(bool success);		// ★ 선언만 (구현은 CRequestUrlParams 정의 이후)

	void register_request(CRequestUrlParams* param)
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		m_pending.insert(param);
	}

	void unregister_request(CRequestUrlParams* param)
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		m_pending.erase(param);
	}

	void invalidate()
	{
		m_last_check_time.store(0);
	}

private:
	void cancel_all_pending();		// ★ 선언만

	int						m_check_interval_ms;
	std::atomic<bool>		m_reachable;
	std::atomic<long long>	m_last_check_time;
	std::atomic<bool>		m_checking;

	std::mutex				m_mtx;
	std::set<CRequestUrlParams*> m_pending;
};

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

	// 복사 생성자
	CRequestUrlParams(const CRequestUrlParams& other)
		: use_thread(other.use_thread)
		, request_id(other.request_id)
		, status(other.status)
		, ip(other.ip)
		, port(other.port)
		, sub_url(other.sub_url)
		, verb(other.verb)
		, is_https(other.is_https)
		, body(other.body)
		, connect_timeout_ms(other.connect_timeout_ms)
		, transfer_timeout_ms(other.transfer_timeout_ms)
		, headers(other.headers)
		, proxy_id(other.proxy_id)
		, proxy_pw(other.proxy_pw)
		, full_url(other.full_url)
		, result(other.result)
		, elapsed(other.elapsed)
		, local_file_path(other.local_file_path)
		, file_size(other.file_size)
		, downloaded_size(other.downloaded_size)
		, download_index(other.download_index)
	{
		h_cancel_root.store(nullptr);
	}

	// 복사 대입 연산자
	CRequestUrlParams& operator=(const CRequestUrlParams& other)
	{
		if (this == &other) return *this;

		use_thread = other.use_thread;
		request_id = other.request_id;
		status = other.status;
		ip = other.ip;
		port = other.port;
		sub_url = other.sub_url;
		verb = other.verb;
		is_https = other.is_https;
		body = other.body;
		connect_timeout_ms = other.connect_timeout_ms;
		transfer_timeout_ms = other.transfer_timeout_ms;
		headers = other.headers;
		proxy_id = other.proxy_id;
		proxy_pw = other.proxy_pw;
		full_url = other.full_url;
		result = other.result;
		elapsed = other.elapsed;
		local_file_path = other.local_file_path;
		file_size = other.file_size;
		downloaded_size = other.downloaded_size;
		download_index = other.download_index;

		h_cancel_root.store(nullptr);

		return *this;
	}

	CRequestUrlParams(CString _full_url, CString _verb = _T("GET"), bool _is_https = true, std::deque<CString>* _headers = NULL, CString _body = _T(""), CString _local_file_path = _T(""));

	void		reset(bool sub_url_reset = false)
	{
		status = -1;
		full_url.Empty();	//full_url 멤버값이 채워져있으면 그 문자열을 파싱하여 ip, port, sub_url로 분리하므로 reset()할 경우는 반드시 비워줘야 한다.
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

	// 취소 지원: request_url 내부에서 InternetOpen 핸들을 저장하고,
	// 외부에서 cancel()을 호출하면 핸들을 닫아 블로킹 WinInet 작업을 즉시 취소한다.
	std::atomic<HINTERNET> h_cancel_root{ nullptr };
	void cancel()
	{
		HINTERNET h = h_cancel_root.exchange(nullptr);
		if (h) InternetCloseHandle(h);
		// MSDN: 부모 핸들을 닫으면 자식 핸들(hConnect, hRequest)도 무효화됨
	}

	//thread로 별도 실행할지(특히 파일 다운로드 request), request 결과를 바로 받아서 처리할지(단순 request)
	bool		use_thread = false;

	//m_request_id로 해당 작업이 무엇인지 구분한다.
	int			request_id = -1;

	//200, 404...와 같은 HTTP_STATUS를 담지만 invalid address 등과 같은 에러코드도 담기 위해 int로 사용한다. 0보다 작을 경우는 result 문자열에 에러 내용이 담겨있다.
	int			status = -1;

	//포트로 http와 https를 구분하는 것은 위험하다. m_isHttps=true 또는 ip에 "https://"가 포함되어 있으면 m_isHttps가 자동 true로 설정된다.
	CString		ip = _T("");

	int			port = 0;
	CString		sub_url;				//domain을 제외한 나머지 주소
	CString		verb = _T("GET");
	//url의 시작이 http인지 https인지, port가 80 또는 443일 경우는 자동 설정되지만
	//임의 포트번호를 사용하는 경우에는 반드시 명시해줘야 한다.
	bool		is_https = true;
	CString		body;					//post data(json format)

	//default = 30초
	//연결 타임아웃과 전송 타임아웃 분리.
	int			connect_timeout_ms = 5000;
	int			transfer_timeout_ms = 30000;

	//token_header.Format(_T("token: %s"), ServiceSetting::strManagerToken);
	//각 항목의 끝에는 반드시 "\r\n"을 붙여줘야하는데 이는 requestAPI()에서 알아서 처리함.
	std::deque<CString> headers;

	//proxy 계정 정보가 없어서 실패하면 407(Proxy Authentication Required) error가 발생하므로 이때는 사용자에게 직접 입력받고
	//다시 호출해줘야 한다.
	CString		proxy_id;
	CString		proxy_pw;

	//한번 호출해서 실패한 후 port나 주소 등 url관련 정보를 수정하여 다시 request_url()을 호출할 때
	//full_url을 ""로 만들어주지 않으면 이 값을 바로 사용해서 다시 request하므로 역시 실패하게 된다.
	//반드시 url 관련값을 수정하여 다시 request할 경우에는 반드시 full_url = _T("")로 만들어주고 호출해야 한다.
	CString		full_url;				//[in][out] full_url을 주고 호출하면 이를 ip, port, sub_url로 나눠서 처리한다. ""로 호출하면 
	CString		result;
	long		elapsed = 0;			//소요시간. ms단위.

	//파일 다운로드 관련
	CString		local_file_path;		//url의 파일을 다운받을 경우 로컬 파일 full path 지정.
	uint64_t	file_size = 0;			//url 파일 크기
	uint64_t	downloaded_size = 0;	//현재까지 받은 크기
	int			download_index = -1;	//n개의 파일 다운로드시 현재 파일의 인덱스. request_id와는 다름.
};

// ★ CRequestUrlParams 완전 정의 이후에 구현
inline void CServerReachabilityCache::cancel_all_pending()
{
	std::lock_guard<std::mutex> lock(m_mtx);
	for (auto* p : m_pending)
		p->cancel();
}

inline void CServerReachabilityCache::update(bool success)
{
	bool was_reachable = m_reachable.exchange(success);

	if (was_reachable && !success)
		cancel_all_pending();

	auto now = std::chrono::steady_clock::now().time_since_epoch();
	long long now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
	m_last_check_time.store(now_ms);
}

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

   //생성자
	vector2()
	{
		x = 0.0, y = 0.0;
	}

    vector2(double _x, double _y)
	{
        x = _x, y = _y;
    }
    //외적
    double cross(const vector2& other) const
	{
        return x*other.y-y*other.x;
    }

    /* 연산자 오버로딩을 통해 실제 벡터의 연산을 구현합니다. */

    //벡터의 실수배
    vector2 operator * (double r) const
	{
        return vector2(x*r, y*r);
    }
    //벡터의 덧셈
    vector2 operator + (vector2 other) const
	{
        return vector2(x + other.x, y + other.y);
    }
    //벡터의 뺄셈
    vector2 operator - (vector2 other) const
	{
        return vector2(x - other.x, y - other.y);
    }
    //두 벡터의 비교
    bool operator == (vector2 other) const
	{
        return x == other.x && y == other.y;
    }
    bool operator < (vector2 other) const
	{
        return x < other.x && y < other.y;
    }
};


//timer 관련변수
extern		double		g_dTime0;
extern		double		g_dTime1;
extern		CString		g_sForTimer;

//함수 수행 시간을 출력. a는 문자열, x는 수행 함수
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
//프로세스 관련
	//fullpath가 ""이면 현재 실행파일로, strFlag는 기본 파일버전을 얻어온다.
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
	//파일 or 폴더 or 드라이브 or "내 PC"의 속성창을 표시한다.
	//단일 파일을 주고자 할 경우는 show_property_window(std::deque<CString> {path});와 같이 호출하면 된다.
	bool		show_property_window(std::deque<CString> fullpath);
	CString		GetProcessNameByPID(const DWORD pid);
#else
	bool		show_property_window(std::deque<CString> fullpath);
#endif
	//해당 프로세스 파일이 실행중인 인스턴스 카운트를 리턴.
	//실행 파일명만 주면 파일명만 비교하지만 전체 경로를 주면 경로까지 맞아야 카운트 됨.
	//ex. 풀패스인 c:\test.exe를 주면 d:\test.exe는 실행중이라도 카운트되지 않는다.
	int			get_process_running_count(CString processname);
	bool		is_running(CString processname);
	
	//return value : 1(killed), 0(fail to kill), -1(not found)
	int			kill_process_by_fullpath(CString fullpath);

	bool		kill_process(CString processname);
	//프로세스 강제 종료.
	//return value : 1 : killed, 0 : fail to kill, -1 : not found
	bool		ProcessKill(CString szProcessName);

	HWND		GetWindowHandleFromProcessID(DWORD dwProcId);
	bool		IsDuplicatedRun();

	int			find_parameter(CString target);


	//cmd 명령 실행 후 결과를 문자열로 리턴.
	//wait_until_process_exit : 실행 프로세스가 정상 종료될때까지 기다린다.
	//return_after_first_read : wait_until_process_exit를 false로 해도 장시간 끝나지 않는 경우가 있어(ex. telnet)
	//우선 이 값이 true이면 맨 처음 read후에 바로 종료시킨다.
	//주의! osk.exe라는 가상키보드 프로그램은 system폴더에만 있고 SysWow64 폴더에는 없는데
	//32bit 프로그램에서 ShellExecute() 또는 CreateProcess()로 실행하면 SysWow64폴더에서 해당 파일을 찾으므로 실패한다.
	//c:\\windows\\system32\\osk.exe로 실행해도 SysWOW64 폴더로 redirect되므로 역시 실행되지 않는다.
	//Wow64DisableWow64FsRedirection()를 이용해서 redirection을 disable시켜주고 실행 후 복원시켜줘야 한다.
	//"dir C:\\*.*"은 성공하나 "dir \"C:\\Program Files\\*.*\"" 명령은 실패한다.
	//"ipconfig /all"은 wait_until_process_exit가 true or false 모두 문제없지만
	//"systeminfo"와 "tasklist"는 wait_until_process_exit를 true로 주면 무한 대기하는 현상이 있다. false로 줘도 정보는 모두 리턴된다.
	CString		run_process(CString cmd, bool wait_until_process_exit, bool return_after_first_read = false);

	//"dir \"C:\\Program Files\\*.*\"" 명령은 잘 동작하나 ping -t와 같이 끝나지 않는 도스명령어나 notepad.exe를 실행할 경우
	//도스창이 계속 남아있다.
	//또한 이 방식은 도스창이 표시되었다가 사라지는 부작용이 있다. 도스창 표시가 불필요하다면 위의 run_process()를 사용해야 한다.
	CString		run_process(CString cmd);
	extern		void* g_wow64_preset;
	void		Wow64Disable(bool disable = true);

	//서비스 관련 명령을 쉽게 처리하기 위해 작성.
	//cmd는 다음과 같은 키워드를 사용한다.
	//"query"	: status를 리턴
	//"stop"	: 서비스를 중지시키고 최종 status = "SERVICE_STOPPED"를 리턴, 그렇지 않으면 detail 참조.
	//			: 서비스가 존재하지 않거나 이미 중지된 경우에도 "SERVICE_STOPPED"를 리턴함.
	//"delete"	: 서비스 삭제가 성공하면 0이 아닌 값을 리턴. 실패하면 0을 리턴하므로 이 경우는 detail 참조.
	DWORD		service_command(CString service_name, CString cmd, DWORD& error_code, CString *detail = NULL);
	//status값에 해당하는 상태 스트링 리턴
	CString		get_service_status_str(DWORD status);

	//Console 명령인지 GUI 윈도우 어플리케이션인지 구분
	bool		is_gui_application(CString fullPath);

	//PID, 프로세스 이름, 윈도우 타이틀 이름, 윈도우 클래스 이름으로 클래스의 생존 상태를 구할수 있습니다. from Devpia
	bool		CheckProcessUsingPID(unsigned long pid);
	bool		CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName);
	bool		CheckProcessUsingProcessName(LPCTSTR processName);

	//모니터 정보
	//main에서 EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); 를 실행하고
	//이 파일에 전역변수로 선언된 g_monitors를 이용하면 된다.
	//단, Win32API인 EnumDisplayMonitors()를 호출할때는 반드시 g_monitors.clear()를 해줘야 하므로
	//enum_display_monitors()함수를 사용하도록 한다.
	extern std::deque<CSCMonitorInfo> g_monitors;
	void		enum_display_monitors();
	BOOL		CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
	//r이 걸쳐있는 모니터 인덱스를 리턴. 겹쳐지는 영역이 어디에도 없다면 -1을 리턴.
	//entire_included가 true이면 어떤 모니터에 완전히 속해있는 경우에만 해당 인덱스를 리턴.
	int			get_monitor_index(CRect r, bool entire_included = false);
	//x, y가 속해있는 모니터 인덱스를 리턴
	int			get_monitor_index(int x, int y);
	//특정 윈도우가 속해있는 모니터 인덱스를 리턴
	int			get_monitor_index(HWND hWnd);
	//멀티모니터 전체 영역 사각형 리턴. -1이면 전체 모니터 영역을 리턴.
	CRect		get_monitor_rect(int index = -1);

	//::SetForegroundWindow()가 Win98이후부터는 지원되지 않아 수정된 코드.
	void		SetForegroundWindowForce(HWND hWnd, bool makeTopMost = false);
	bool		is_top_most(HWND hWnd);

//클립보드 clipboard
	bool		copy_to_clipboard(HWND hWnd, CString str);

//////////////////////////////////////////////////////////////////////////
//문자열
	bool		Compare_By_Case_Sensitive(CString str1, CString str2, bool bCase);
	//target에서 맨 처음 매칭되는 항목만 찾아 그 시작위치를 리턴한다.
	//단, case_sensitive가 false일 경우 text의 길이가 매우 크다면 매번 MakeLower()를 호출하므로 느려질 수 있다.
	//while문 등에서 이 함수를 지속적으로 호출할 경우는 미리 MakeLower()로 변경 후 호출해야한다.
	//즉, 간단한 문자열일 경우에만 이 함수를 사용하고 길이가 매우 크다면 문자열을 가공한 후 CString::Find()를 이용해야 한다.
	int			find(CString target, CString find_string, int start = 0, bool case_sensitive = false, bool whole_word = false);
	//target에서 매칭되는 모든 항목의 시작위치를 result deque에 담고 첫번째 매칭 위치를 리턴한다.
	//단, case_sensitive가 false일 경우 text의 길이가 매우 크다면 매번 MakeLower()를 호출하므로 느려질 수 있다.
	//while문 등에서 이 함수를 지속적으로 호출할 경우는 미리 MakeLower()로 변경 후 호출해야한다.
	//즉, 간단한 문자열일 경우에만 이 함수를 사용하고 길이가 매우 크다면 문자열을 가공한 후 CString::Find()를 이용해야 한다.
	int			find_all(std::deque<int>& result, CString target, CString find_string, bool case_sensitive = false, bool whole_word = false);

	//dqSrc에 dqFind가 있는지 검사하여 인덱스를 리턴. 현재는 AND 연산이므로 dqFind의 모든 원소가 dqSrc에 포함되어 있어야 함.
	int			find_dqstring(std::deque<CString> dqSrc, CString strFind, bool bWholeWord = false, bool bCaseSensitive = false);
	int			find_dqstring(std::deque<CString> dqSrc, std::deque<CString> dqFind, TCHAR op = '&', bool bWholeWord = false, bool bCaseSensitive = false);
	int			Find_Divide_Position_By_Punctuation(CString str);
	int			FindStringFromArray(CStringArray& ar, CString sTarget, bool bCaseSensitive = false, bool bWholeWord = false);
	//int			FindStringFromDeque(std::deque<CString> dq, CString sTarget, bool bCaseSensitive = false, bool bWholeWord = false);
	CString		GetCommaString(CString sString, CString sComma = _T(","));
	bool		IsNumericString(const CString& strSource);
	//양수의 정수값 문자열인지
	bool		IsNatural(LPCTSTR lpszValue);
	//소숫점까지 허용하여 숫자값인지
	bool		IsNumeric(LPCTSTR lpszValue);
	//음수를 허용하는 정수값 문자열인지
	bool		IsInteger(LPCTSTR lpszValue);
	//start부터 시작해서 처음 만나는 숫자 영역을 추출해서 num에 넘겨준다.
	//숫자 영역의 끝 인덱스를 리턴한다.
	//숫자 영역이 없으면 false를 리턴한다.
	int			get_number_from_string(CString str, int &num, int start = 0);
	//문자열에 포함된 숫자문자를 숫자로 간주하여 비교한다. "a5"는 "a12"보다 작다.
	bool		is_greater_with_numeric(CString str0, CString str1);
	//주어진 문자열이 알파벳과 숫자로만 구성된 문자열인지 검사한다.
	//excepts에는 포함되도 되는 문자열들이 들어있는데 이들은 ;으로 구분되어 있고
	//검사하기 전에 미리 삭제한 후 검사한다.
	bool		IsAlphaNumeric(CString str, CString excepts = _T(""));

	//ASCII 코드의 #33(0x21)(' ') ~ #126(0x7E)('~') 범위인지(읽을 수 있는 문자열인지)
	bool		is_readable_char(CString src);

	//각 언어마다 주석처리 문자열이 다르므로 주석처리된 라인인지 판별
	//리턴값은 해당 주석처리 문자열
	CString		is_comment(CString src);

	//'가'~'힣'범위의 온전한 한글인지 검사한다.
	//'가' = true
	//'강' = true
	//'강ㄷ' = false
	//allow_ascii가 true라면 영문, 숫자, 특수문자가 있어도 한글만 온전하면 true이며
	//allow_ascii가 false라면 오로지 한글로만 구성되었는지를 판별하여 리턴한다.
	//allow_ascii가 true일 경우 실제 한글문자가 포함되었는지도 판별할 경우는 int*를 넘겨받아 판단하면 된다.
	bool		is_hangul(CString str, bool allow_ascii = false, int *hangul_count = NULL);

	//문자열이 온전한지 깨진 문자인지를 판별(특히 한글 인코딩 깨짐 판별)
	bool		is_valid_string(CString src, bool include_hangul);

	//완성형 한글의 한 글자를 초성, 중성, 종성으로 분리한다.
	bool		get_consonant(CString src, wchar_t* cho = 0, wchar_t* jung = 0, wchar_t* jong = 0);

	CString		ConvertInt2AZ(int n);	//n을 26진수 엑셀 컬럼 인덱스로 변환한 문자열을 리턴
	CString		GetToken(CString& str, LPCTSTR c);
	CString		GetToken(CString src, CString separator, int index);

	//separator는 기존 CString에서 TCHAR로 통일한다.
	//2char이상의 문자열이 하나의 separator로 사용될 경우는 거의 없으나
	//1개 또는 그 이상의 서로 다른 문자를 separator들로 사용할 경우는 있을 것이다.
	//20240426 separator가 여러개일 경우는 특정 separator에 의해 얻어진 token이 또 다른 separator를 포함할 수 있으므로
	//그 처리가 매우 복잡해진다. 그냥 CString separator로 처리한다.
	//include_rest는 nMaxToken이 명시된 경우 마지막 토근을 어디까지로 하느냐를 정하는 옵션이다.
	//예를 들어 "LastUpdated : 1601-01-01 9:00:00"과 같은 문자열을 ':'로 파싱할 경우
	//nMaxToken을 2로 준 경우 dqToken[0] = "LastUpdated "가 되고
	//두번째 토큰은 " 1601-01-01 9"가 되지만(실제 기대값은 " 1601-01-01 9:00:00"일 것이다)
	//include_rest를 true로 주면 dqToken[1] = " 1601-01-01 9:00:00"이 된다.
	//즉, 최대 토큰 개수가 정해져 있을 때 마지막 토큰을 어디까지로 처리할 것인가에 대한 옵션이다.
	int			get_token_str(CString src, std::deque<CString>& dqToken, CString separator = _T("|"), bool allowEmpty = true, int nMaxToken = -1, bool include_rest = false);
	int			get_token_str(std::string src, std::deque<CString>& dqToken, CString separator = _T("|"), bool allowEmpty = true, int nMaxToken = -1, bool include_rest = false);
	int			get_token_str(TCHAR *src, TCHAR *separator, CString *sToken, int nMaxToken);
	int			get_token_str(char *src, char *separator, char **sToken, int nMaxToken);

	//대부분의 경우는 get_token_str()을 사용하지만 separator가 문자열 내에 포함된 경우도 있을 수 있다.
	//"내 PC\\연구소문서2(\\\\192.168.1.103) (X:)" 문자열을 '\\'로 구분할 경우 잘못 추출되므로
	//이 경우는 get_exact_token_string()을 사용해서 실제 '\\'인 경우에만 추출하도록 해야 한다.
	int			get_exact_token_str(CString src, std::deque<CString>& dqToken, CString separator = _T("|"));

	//"<b><cr=red>This</b></cr> is a <i>sample</i> <b>paragraph</b>."
	//위와 같은 형식일 때 태그와 텍스트를 분리한다. 태그내의 공백은 제거된다.
	void		get_tag_str(CString& src, std::deque<CString>& tags);

	//간혹 \r, \n, \t, \\등의 문자를 그대로 확인할 필요가 있다.
	CString		get_unescape_str(CString src);

	//엑셀의 컬럼과 같이 n=0이면 "A"를, n=25이면 "Z"를, n=26이면 "AA"를, n=27이면 "AB"를 리턴한다.
	//CString		get_excel_column(int n);
	std::string	get_excel_column(int n);

	// a_value : 1.1.24050
	// b_value : Normal
	// c_value : True
	// 위와 같이 속성이름 및 값으로 매핑되는 문자열을 파싱하여 std::map에 넣어준다.
	// lfrf는 라인분리문자열이고 보통 "\n"이거나 "\r\n" 등이 있고
	// separator는 ':' 이름과 값을 구분하는 구분자이다.
	// return value : 항목의 개수
	int			get_map_str(CString src, std::map<CString, CString>& map, CString lfrf = _T("\n"), CString separator = _T(":"));

	//list 항목들을 ','로 나열한 문자열을 리턴한다.
	template <class T> inline CString get_list_str(std::deque<T>& list)
	{
		CString res;

		for (size_t i = 0; i < list.size(); i++)
		{
			if (typeid(T) == typeid(CString))
			{
				if (i > 0)
					res += _T(",");

				//list가 std::deque<CString> 타입이라도 아래와 같은 문장은 오류가 발생한다.
				//반드시 str을 선언하고 Format()을 이용하여 값을 세팅한 후 더해줘야 한다.
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
				ASSERT(FALSE); //지원하지 않는 타입
			}
		}

		return res;
	}

	//dq항목을 하나의 문자열로 합쳐준다.
	CString		get_concat_string(std::deque<CString> dq, CString separator = _T("|"));

	//[2023/1/1 22:1:29] [DBMS][NMS_LS_TERMINATE_SESSION_DATA][ID : tmax25][update livesupport_report set endtime = '2023-01-01 22:01:29', env_type = '5', viewer_type = '0' where accesscode = '31108355' and sptnum = '50400']
	//[]로 묶여진 토큰을 분리한다.
	//괄호가 쌍이 안맞으면 false를 리턴한다.
	bool		get_bracket_token(CString src, std::deque<CString>* token, TCHAR sep);

	//update livesupport_report set endtime = '2023-01-01 22:01:29', env_type = '5', viewer_type = '0' where accesscode = '31108355' and sptnum = '50400'
	//위와 같은 sql에서 field와 value를 추출한다.
	void		get_sql_token_from_assign_form(CString src, std::map<CString, CString> *map);
	//insert into neturo_server_info(userid, com_name, s_pub_ip, s_pri_ip) values('14533821', 'DESKTOP-0CN9VAK', '220.85.215.243', 'publicIP')
	//위와 같은 sql에서 field와 value를 추출한다.
	void		get_sql_token_from_bracket_form(CString src, std::map<CString, CString>* map);
	CString		get_sql_cmd(CString src, CString* sql_cmd = NULL, CString *table_name = NULL);

	//deque에 있는 원소들을 구분자로 하는 하나의 문자열로 리턴
	CString		get_tokenized(std::deque<CString> dq, TCHAR separator = ';');

	//src에서 sep를 구분자로 하나씩 뽑아내고 그 나머지를 다시 src로 치환한다.
	//src가 계속 변경됨에 주의.
	CString		get_str(CString& src, CString sep = _T("|"));
	int			get_int(CString& src, CString sep = _T("|"));
	double		get_double(CString& src, CString sep = _T("|"));

	//src에서 prefix와 postfix 사이에 있는 문자열을 추출한다.
	CString		extract_sub_str(CString src, CString prefix, CString postfix);

	//resource string table의 문자열을 리턴한다.
	extern CString load_string(UINT nID);

	//unit			: -1:auto, 0:bytes, 1:KB, 2:MB, 3:GB ~
	//auto일 경우는 1000보다 작을떄까지 나누고 소수점은 2자리까지 표시한다.(ex 7.28TB)
	//floats		: 소수점을 몇 자리까지 표시할지
	//unit_string	: 단위를 표시할 지
	//comma			: 정수 부분에 자리수 콤마를 표시할 지
	CString		get_size_str(ULONGLONG size, int unit = 1, int floats = 0, bool unit_string = true, bool comma = true);

	//src를 파싱해서 특정 길이 이상의 문자열들로 나눈다.
	std::deque<CString> parse_divide(CString src, int len);
	//src의 pos근처에서 구두점을 찾아 그 위치를 리턴한다.(구두점으로 문장을 나눌때 이용)
	int			find_punctuation(CString src, int pos);
	bool		is_punctuation(TCHAR ch);

	//isOneOf()는 is_one_of()로 대체함.
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


	//src 문자열에 set_of_keyword에 나열된 단어가 있는지 검사.
	//set_of_keyword는 세미콜론으로 구분해서 여러 문자 또는 문자열을 넣을 수 있다.
	//ex. src = "abcd1234"일 때 set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
	bool		is_exist_keyword(CString src, CString set_of_keyword, bool case_sensitive = false, bool whole_word = false);

	//dqList에서 element값과 일치하는 항목의 index를 리턴한다. 없으면 -1을 리턴.
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

	//str의 from 위치 이후에 있는 숫자 영역값을 num에 넣어주고 숫자 시작위치를 return한다.
	//음수인 경우도 있으므로 unsigned로 하면 안됨.
	int	extract_digit_number(char *str, int from, double *num);

	//version string valid check
	//digits : 자릿수(1.0.0.1일 경우는 자릿수 4)
	bool valid_version_str(CString versionStr, int digits);

	//버전 또는 IP주소등은 그냥 문자열로 비교하면 1.0.9.0이 1.0.10.0보다 더 크다고 나오므로
	//.을 없앤 숫자로 비교했으나 이 방법도 오류 발생(1.0.1.13 > 1.0.10.3보다 크다고 판단함)
	//결국 각 자릿수끼리 구분해야 한다.
	//리턴값은 strcmp와 동일한 규칙으로 판단한다.(+:str0가 큼, -:str1이 큼, 0:같음)
	//이 함수는 자릿수는 틀려도 무관하나 separator가 숫자라든지, 숫자가 아닌 문자가 섞여있을 경우에 대한 처리는 없다.
	//버전이든 IP주소이든 정석적인 문자열이 들어왔다라는 가정하에 정확히 판단한다.
	int compare_str(const CString& str0, const CString& str1, TCHAR separator = '.');

	//src를 n번 연결한 문자열 리턴. n개의 공백, 탭이 필요할 경우 사용
	//20250829 기존 make_string() 함수의 이름을 duplicate_str()으로 변경함.
	CString duplicate_str(CString src, int n);

	//http://yeobi27.tistory.com/280
	//A2W, A2T 및 그 반대 매크로들은 스택을 사용하므로 문제 소지가 있고 크기 제한도 있으므로
	//가급적 CA2W, CA2T등을 사용한다. 단 이 매크로들은 encoding을 변경할 수 없다.
	std::wstring CString2wstring(const char* str);
	//std::string ss = CT2CA(CString(_T("test")); 과 같이 CT2CA를 사용하면 간단함.
	std::string CString2string(CString cs);
	//CString str(sstr.c_str());
	CString		string2CString(std::string sstr);
	//멀티바이트 환경에서 이 함수를 호출해서 사용하면 간혹 비정상적으로 동작한다.
	//아마도 함수내에서 메모리가 할당된 후 호출한 곳에서 사용하려니 문제가 될 수 있다.
	//이 함수의 바디를 그대로 쓰면 문제가 없으므로 일단 바디 코드를 그대로 복사해서 사용한다.
	LPCWSTR		CString2LPCWSTR(CString str);
	//char chStr[100] = { 0, };와 같이 pointer 변수가 아닌 배열로 선언된 경우라면
	//chStr = CString2char(str); 문장은 오류가 발생하므로 아래와 같이 사용할 것.
	//sprintf(chStr, "%s", (LPSTR)(LPCTSTR)str);	//MBCS : ok, UNICODE : fail
	//sprintf(chStr, "%s", CStringA(str));		//both : ok
	//리턴받아 사용한 char* 변수값은 사용 후 반드시 delete [] 해줄것
	char*		CString2char(CString str);
	TCHAR*		CString2TCHAR(CString str);
	LPCSTR		CString2LPCSTR(CString str);
	LPCWSTR		LPCTSTR2LPCWSTR(LPCTSTR str, UINT codePage = CP_UTF8);
	WCHAR*		CString2WCHAR(CString str); //{ return (WCHAR*)(const WCHAR*)CStringW(str); }

	//cstr의 유효한 길이를 이미 알고 있다면 length를 지정해줘야 정확하다.
	//그렇지 않을 경우 cstr의 끝에 '\0'가 없을 경우 쓰레기 문자들까지 포함될 수 있다.
	//cstr이 '\0'로 끝난다면 유니코드, 멀티바이트 환경에서 CString str = cstr;로 정상 처리된다.
	CString		char2CString(char *cstr, int length = -1);
	CString		TCHAR2CString(TCHAR *str);
	VARIANT		CString2VARIANT(CString str);

	CStringA	UTF16toUTF8(const CStringW& utf16);
	CStringW	UTF8toUTF16(const CStringA& utf8);
	CString		UTF8toCString(char* pszCode);
	CString		utf8toCString(std::string inputtext);
	char*		UTF8toANSI(char* pszCode);
	char*		ANSItoUTF8(char* pszCode);
	std::string	multibyteToUtf8(std::string inputtext);
	std::string	utf82Multibyte(std::string inputtext);
	std::wstring multibyte2Unicode(std::string inputtext);
	std::string	unicode2Multibyte(std::wstring inputtext);
	std::wstring utf8ToUnicode(std::string inputtext);
	std::string	unicode2Utf8(std::wstring inputtext);
	std::string multibyteToUtf8(std::string inputtext);
	std::string utf82Multibyte(std::string inputtext);

	std::string	CString2Utf8(CString inputtext);

	//[[nodiscard]] static CStringA W2UTF8(_In_NLS_string_(nLength) const wchar_t* pszText, _In_ int nLength);
	//[[nodiscard]] static CStringW UTF82W(_In_NLS_string_(nLength) const char* pszText, _In_ int nLength);

	//return받은 char*는 반드시 사용 후 free()해줘야 함.
	TCHAR*		replace(TCHAR* src, const TCHAR* olds, const TCHAR* news);

	//src의 끝에서 length 길이 만큼 잘라낸다.
	CString		truncate(CString &src, int length);
	//src끝의 문자열이 sub와 일치하면 잘라낸다.
	CString		truncate(CString &src, CString sub);

	//공백, '\t', '\r', '\n', '\0' 모두 제거
	void		trim(char* src);
	void		trim(std::string &str);
	void		trim_left(std::string& str);
	void		trim_right(std::string& str);
	void		trim(std::deque<CString>* dq);
	//src문자열에서 chars를 모두 제거한다.
	void		remove_chars(CString &src, CString chars);
	//src문자열에서 지정된 구간의 문자열을 제거한다.

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
	//str에서 앞뒤 문자열 사이에 있는 서브 문자열을 리턴한다. 없으면 "" 리턴.
	CString		ExtractSubString(CString src, CString sPrev, CString sPost);	//sPrev(필드 앞에 오는 문자열), sPost(필드 뒤에 오는 문자열)
	//std::string string_format(const std::string fmt, ...);
	//A safer and more efficient(http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf)
	std::string string_format(const std::string fmt_str, ...);
	/*
	//format에 줄때는 %s가 아닌 %S??
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
	//이전 Koino 개발자가 간단히 만든 코드이나 대괄호 처리 등 취약점이 많음.
	//또한 숫자형의 필드값들도 모두 CString으로만 return하는 단점이 있다.
	//심플한 json에 대해서는 정상 동작하므로 기존 코드와의 호환성때문에 우선 사용하지만
	//Common/json/rapid_json 로 대체되어야 함.
	//우선 수정하여 문자열 필드값에 ',', '}' 등이 포함되어도 정상 파싱되도록 수정은 한 상태.
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

//메시지 박스
	void		msgbox(LPCTSTR format, ...);

//데이터 변환
	CString		i2S(int64_t nValue, bool bComma = false, bool fill_zero = false, int digits = 0);
	CString		i2HS(int64_t nValue, bool bCapital = true);
	CString		d2S(double dValue, bool bComma = false, int nfDigit = -1);	//nfDigit : 소수점 자릿수. -1이면 그대로 출력.

	//IPv4 문자열을 숫자로 바꾸는 범용 코드이므로 버전 문자열 등 일반 문자열 비교에 사용하지 말것.
	//버전 문자열 등은 각 자릿수 구성이 다를 수 있으므로 사용할 수 없음.
	//문자열 비교가 필요하다면 compare_string()을 사용할 것.
	uint32_t	IP2int(CString IP);

	char*		ushortToBinary(unsigned short i);
	char*		intToBinary(int i);
	char*		uintToBinary(unsigned int i);
	int			binaryToInt(char *s);
	unsigned int binaryToUint(char *s);
	//어떤 수의 x번째 비트값 리턴.
	int			get_bit(int number, int x);
	//x가 1이면 n번째 값을 1로 변경, x가 0이면 n번째 값을 0으로 변경
	void		set_bit(int& number, int n, int x);
	int			HexaStringToInt(CString str);
	CString		GetByteString(uint8_t* bt, int n, bool upper = true, bool prefix = true);
	CString		BinaryToHexString(BYTE* pData, int length, TCHAR separator = _T(' '));
	int			getPrecision(double d, bool bExceptZero = true);	//소수점 자릿수 리턴
	template<class T> CString get_binary_string(T n, bool separator = true)
	{
		CString res;

		while (n != 0)
		{
			res = (n % 2 == 0 ? '0' : '1') + res;
			n /= 2;
		}

		//4자릿수로 맞춤
		int len = res.GetLength();
		if (len % 4 != 0)
		{
			int count = 4 - len % 4;
			while (count-- > 0)
			{
				res = '0' + res;
			}
		}

		//4자릿수 공백 추가
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
	//URL or filepath safe base64 encoding
	CString base64_encode_url(CString plain_src, bool remove_padding = false);
	std::string base64_encode_url(const std::string& plain_src, bool remove_padding = false);
	//URL or filepath safe base64 decoding
	CString base64_decode_url(CString safe_base64_src);
	std::string base64_decode_url(std::string safe_base64_src);
#endif

//////////////////////////////////////////////////////////////////////////
//파일 관련
//폴더 관련
	//_tsplitpath("c:\\abc/def\\123.txt", ...)를 실행하면
	//"c:", "\\abc/def\\", "123", ".txt" 과 같이 분리되는데 기존에 사용하던 기대값과 달라 보정한다.
	//"c:\\", "c:\\abc/def", "123", "txt", "123.txt와 같이 보정한다.
	//part : fn_drive(drive), fn_folder(drive+folder), fn_leaf_folder(folder name), fn_title(filetitle), fn_ext(ext), fn_name(filename)
	//fn_folder는 path가 파일이든 폴더든 전체 폴더 경로가 저장된다.
	//만약 path가 "d:\\aaa\\bb"라면 bb가 파일인지 폴더인지 알 수 없다.
	//remote의 파일이라면 PathIsFolder()함수로 검사할수도 없으므로
 	//path가 file이 아닌 폴더명이라고 하면 반드시 호출할때부터 맨 끝에 '\\'를 붙여서 호출해야 정확히 분리된다.
	CString		get_part(CString path, int part);
	enum FILENAME_PART
	{
		fn_drive,			//드라이브명으로 반드시 \로 끝난다.
		fn_folder,			//fullpath 폴더명이므로 드라이브 경로까지 모두 포함된다.	
		fn_leaf_folder,		//fullpath의 마지막 폴더명. fullpath가 "C:\"일 경우 drive, folder, leaf_folder는 모두 동일한 값인 "C:\"가 된다.
		fn_title,			//파일 타이틀(확장자 제외)
		fn_ext,				//파일 확장자(dot 제외)
		fn_name,			//파일 확장자를 포함한 파일명
	};
#if 0
	CString		GetFileNameFromFullPath(CString fullpath);
	CString		GetFolderNameFromFullPath(CString fullpath, bool includeSlash = false);	//= PathRemoveFileSpec
	CString		GetFileTitle(CString fullpath);
	CString		GetFileExtension(CString filename, bool dot = false);
#endif
	int			get_filetype_from_filename(CString filename);
	int			get_filetype_from_extension(CString sExt);

	//파일명에서 확장자를 new_ext로 변경한다.
	//apply_real_file이 false이면 filepath 문자열의 확장자만 변경하지만 true이면 실제 파일명도 변경시킨다.
	bool		change_extension(CString& filepath, CString new_ext, bool apply_real_file);
	CString		normalize_path(CString& filepath);

	//"C:\\", "C:\\Temp"와 같이 루트일때와 일반 폴더일 경우 끝에 역슬래시 유무가 다르므로 필요.
	bool		is_drive_root(CString path);
	//src 폴더 경로에 sub 폴더 경로를 붙여주는 단순한 함수지만 드라이브 루트일때와 아닐때 등의 처리때문에 검사하여 결합해주는 목적으로 추가.
	CString		concat_path(CString src, CString sub, TCHAR path_sep = '\\');

	//새 폴더, 새 폴더 (2)와 같이 폴더내에 새 항목을 만들 때 사용 가능한 인덱스를 리턴한다.
	//zero_prefix가 2이면 001, 002로 된 인덱스가 붙은 파일/폴더들만 대상으로 하려 했으나 아직 미구현.
	int			get_file_index(CString folder, CString title, int zero_prefix = 0);

	//확장자 집합 문자열로 파일열기 대화상자의 filter string을 리턴한다.
	//simple : "bmp;jpg;jpeg;png;webp;gif;yuv;raw => "JPG files|*.jpg|bmp|*.bmp|
	//extension_group = FILE_EXTENSION_VIDEO or FILE_EXTENSION_SOUND or FILE_EXTENSION_IMAGE or FILE_EXTENSION_MEDIA...
	//현재 미완성!
	//CString		get_filter_string(CString extension_group, bool simple = true);

	//폴더에 있는 파일들 중 filetitle이고 extension에 해당하는 파일명을 리턴한다.
	std::deque<CString>		get_filelist_from_filetitle(CString folder, CString filetitle, CString extension);
	std::deque<CString>		get_filelist_from_filetitle(CString filename, CString extension);

	uint64_t	get_file_size(CString sfile);
	bool		get_file_size(CString path, ULARGE_INTEGER* ulFileSize);
	ULONGLONG	get_file_size(WIN32_FIND_DATA data);

	uint64_t	get_folder_size(CString path);

	//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
	//unit_string	: 단위를 표시할 지 (default = true)
	//폴더인 경우는 ""를 리턴함.
	CString		get_file_size_str(CString sfile, int unit = 1, int floats = 0, bool unit_string = true);
	CString		get_file_size_str(WIN32_FIND_DATA data, int unit = 1, int floats = 0, bool unit_string = true);
	CTime		GetFileCreationTime(CString sfile);
	//탐색기에서 복사하면 last modified time이 유지되지만 web에서 다운받은 파일일 경우는 다운받아 생성된 시각으로 변경됨.
	CTime		GetFileLastModifiedTime(CString sfile);
	CTime		GetFileLastAccessTime(CString sfile);
	CString		GetMostRecentFile(CString sFolder, CString sWildCard = _T("*.*"), int nReturnType = 1);
	CString		GetMostRecentDateFile(CString sFolder, CString sWildCard = _T("*.*"));	//가장 최근 날짜 파일명 리턴
	CString		GetFileProperty(CString sFilePath, CString sProperty);

	CString		get_file_time_str(FILETIME filetime);


	//확인 필요
	//CString		set_file_property(CString sFilePath, CString sProperty, CString value);

	//binary file data를 읽어들인다. dst는 크기만큼 미리 할당된 상태로 호출되어야 한다.
	size_t		read_raw(CString sfile, uint8_t *dst, size_t size);
	bool		save2raw(CString sfile, uint8_t *data, size_t size);
	int			RenameFiles(CString folder, CString oldName, CString newName, bool overwrite = false, bool bWholename = true, bool bRecursive = false);
	bool		delete_file(CString fullpath, bool bTrashCan = false);
	int			get_text_encoding(CString sfile);
	bool		is_utf8_encoding(CString filepath);
	bool		is_utf8_encoding_old(CString filepath);
	//파일을 읽어서 CString으로 리턴한다. max_length < 0이면 전체 파일을 읽어서 리턴한다.
	//encoding < 0이면 encoding 방식을 자동 판별하여 읽어온다.
	CString		read(CString filepath, int max_length = -1, int encoding = -1);
	//text 파일로 저장한다.
	//encoding = CP_ACP이면 ANSI 파일로, CP_UTF8이면 utf8 형식으로 저장된다.
	//만약 base64_encode된 text라면 encoding = CP_ACP, is_binary_data = true로 해야 ANSI로 저장된다.
	bool		save(CString filepath, CString text, int encoding = CP_UTF8, bool is_binary_data = false);

	//텍스트 파일을 열 때 ansi, utf-8 등을 자동으로 판별하여 열어주고 encoding 방식을 리턴한다.
	//return value : text_encoding_ansi / text_encoding_utf8 / ....
	int			file_open(FILE** fp, CString mode, CString file);

	//text 파일을 열어서 라인별로 읽은 후 dqList에 넣어준다.
	//각 라인의 끝에 있는 '\n' 기호는 구분자이므로 deque에 들어갈 때는 포함되지 않음에 주의.
	bool		read_lines(CString filepath, std::deque<CString> *dqList);

	//mp4 파일의 특정 태그 데이터 중 원하는 위치의 데이터를 추출한다.
	//MOBIS 프로젝트 저장 MP4는 mdat 필드의 0x40번지부터 28 bytes가
	//동영상이 생성된 절대시간이 저장되어 있다.
	//n은 0x3C부터 4바이트가 그 크기이다.(28 bytes)
	char*		GetDataFromMP4File(char* sfile, char* sTag, uint8_t tagStart, int tagLength);

	//text, binary를 구분하고자 했으나 정상 동작하지 않고 있음. 수정 필요!
	//sfile
	bool		is_binary(CString sfile);

	void		watch_file_system(CString fullpath);

//////////////////////////////////////////////////////////////////////////
//인터넷 파일
	DWORD		GetURLFileSize(LPCTSTR pUrl);
	bool		DownloadFile(LPCTSTR pUrl, CString strFileName, bool bOverwrite = TRUE, HWND hWnd = NULL);
	CString		DownloadURLFile(CString sUrl, CString sLocalFileName, HWND hWnd = NULL);
	bool		CheckFileIsURL(CString sURL);
	//check_prefix가 true이면 http, https까지 체크한다. 뭔가 취약점이 있는듯하여 우선 사용금지.(https://mathiasbynens.be/demo/url-regex)
	bool		is_valid_url(CString url, bool check_prefix);
	void		GetURLFileInfo(CString sURL, bool &bInURL, bool &bFileType);
	bool		ReadURLFile(LPCTSTR pUrl, CString &strBuffer);
	void		ReadURLFileString(CString sURL, CString &sString);

	bool		parse_url(CString full_url, CString &ip, int &port, CString &sub_url, bool &is_https);

	//url을 호출하여 결과값을 리턴하거나 지정된 로컬 파일로 다운로드 한다.
	//local_file_path가 ""이면 결과값을 문자열로 리턴받는다.
	//local_file_path가 지정되어 있으면 파일로 다운받는다.
	//(이때 리턴값은 "")
	//리턴값이 200이 아닐 경우는 리턴된 에러코드와 result_str에 저장된 에러 메시지를 조합하여 에러 처리한다.
	//port만 가지고 http와 https를 구분하는 것은 위험하므로 명확한 지시자로 접근해야 한다.
	//(반드시 https로 접근해야 하는 경우, port가 기본값인 443이 아니라면 주소를 https://~로 명시하여 호출해야 한다)
	//DWORD		request_url(CString &result_str, CString ip, int port, CString sub_url, CString verb = _T("GET"), std::vector<CString> *headers = NULL, CString jsonBody = _T(""), CString local_file_path = _T(""));
	//DWORD		request_url(CString& result_str, CString full_url, CString verb = _T("GET"), std::vector<CString>* headers = NULL, CString jsonBody = _T(""), CString local_file_path = _T(""));

	//기본 30초 타임아웃으로 요청하는데 간혹 서버가 구동중인지 빠르게 확인하고자 한다면 check_server_reachable을 true로 하면 3초이내 판별된다.
	//단, 이 요청은 별도의 TCP 소켓을 생성하므로 다수의 요청이 발생하는 상황에서는 절대 사용하면 안된다.
	void		request_url(CRequestUrlParams* params, bool check_server_reachable = false);


	//기본 브라우저로 설정된 브라우저 이름을 리턴하고 부가적으로 경로, 버전을 얻을 수 있다.
	CString		get_default_browser_info(CString* pPath = NULL, CString* pVersion = NULL);

	//Content-Type: multipart/form-data 형식을 이용한 웹서버로의 파일 전송 함수
	bool		HttpUploadFile(CString url, CString filepath, int chatIndex);

	//서버의 한글명 파일에 대한 처리때문에 request_url()함수 대신 추가하여 테스트 해봤으나
	//서버측의 문제인듯하여 우선 이 함수 사용은 보류중...
	bool		HttpDownloadFile(CString url, CString local_path = _T(""));


//webView2 Runtime
	bool		is_WebView2Runtime_installed();
	bool		install_WebView2Runtime(CString runtimeExePath, bool silentInstall);

//////////////////////////////////////////////////////////////////////////
//폴더 관련
	//가능하면 PathIsDirectory() 사용할 것
	bool		IsFolder(CString sfile);				//폴더인지 파일인지
	bool		isFolder(char *sfile);
	//파일명이나 폴더명에 '\\', '/' 혼용일 경우가 있으므로 CString의 '==' 연산자로 비교해선 안된다. 
	bool		IsFileFolderPathIsEqual(CString file0, CString file1, bool bCaseSensitive = false);
	CString		get_parent_dir(CString path, TCHAR path_sep = '\\');	//현재 폴더의 상위 폴더명을 리턴한다.

	//compare_only_filename : fullpath로 정렬할지, 파일명만 추출해서 정렬할지. default = false;
	void		sort_like_explorer(std::deque<CString> *dq, bool compare_only_filename = false);
	void		sort_like_explorer(std::deque<CString>::iterator _first, std::deque<CString>::iterator _last, bool compare_only_filename = false);

	//지정된 폴더내의 파일 목록을 얻어온다.
	//sNameFilter의 와일드카드는 직접 줘서 검색해야 한다.
	//프롬프트 명령과 동일하게 물음표나 별표와 같은 와일드카드를 이용할 수 있다.
	//sNameFilter = "test*", sExtFilter = "jpg;bmp;" 와 같이 입력하면
	//test로 시작하고 확장자가 jpg, bmp인 파일 목록을 얻어온다.
	//sExceptStr = "test;temp;error" 와 같이 세미콜론으로 구분하여 검색 제외할 파일명 지정 가능.
	//주의! dqFiles는 이 함수에 의해 초기화되지 않으므로 필요한 경우 초기화하여 호출할 것!
	//bRecursive이면 하위 폴더들 내의 모든 파일들도 검색한다. 폴더 자체는 리스트에 포함되지 않는다.
	void		FindAllFiles(	CString sFolder,
								std::deque<CString> *dqFiles,
								CString sNameFilter = _T("*"),
								CString sExtFilter = _T("*"),
								bool bRecursive = false,
								bool include_folder = false,
								CString sExceptStr = _T(""),
								bool auto_sort = true);
	//stdc++17의 std::filesystem을 이용한 함수로서
	//FindAllFiles가 recursive function에다가 옵션이 많다고는 해도
	//debug mode에서 8700개의 파일을 찾는데(D:\1.project\0.backup)
	//25,047ms VS 76ms의 속도차이가 난다.(+auto_sort=268ms)
	//32,105개의 파일 : 1,773,564ms VS 3,337ms
	//release mode에서는
	//9,715ms VS 21ms.
#if (_MSVC_LANG >= _std_cpp17)	//__cplusplus 매크로를 사용하려면 C/C++의 고급창에서 /Zc:__cplusplus를 추가시켜야 한다.
	std::deque<CString>	find_all_files(CString path, CString name_filter = _T(""), CString ext_filters = _T(""), CString except_str = _T(""), bool recursive = true, bool auto_sort = true);
#endif

	//include_folder가 true이면 폴더도 하나의 항목으로 리턴하고
	//include_folder가 false이고 recursive가 true이면 sub folder들의 모든 파일목록을 리턴한다.
	//dq의 cFileName에는 검색된 폴더 또는 파일의 fullpath가 기록된다.
	void find_all_files(CString folder, std::deque<WIN32_FIND_DATA>* dq, CString filter = _T("*"), bool include_folder = false, bool recursive = false, bool include_hidden_files = false, bool include_system_files = false);
	void find_all_files(CString folder, std::deque<CString>* dq, CString filter = _T("*"), bool include_folder = false, bool recursive = false, bool include_hidden_files = false, bool include_system_files = false);

	//list를 NULL로 호출하면 단지 sub folder의 갯수만 참조할 목적이다.
	//root가 "내 PC"일 경우 special_folders가 true이면 다운로드, 내 문서, 바탕 화면 항목까지 추가한다.
	//include_files가 true이면 파일도 포함된다.
	int	get_sub_folders(CString root, std::deque<CString>* list = NULL, bool special_folders = false, bool include_files = false);
	//위 함수는 전체 서브 폴더의 목록이나 개수까지 모두 구하기 때문에 특정 폴더일 경우는 속도가 매우 느리다
	//간단히 서브 폴더 유무만 체크하는 함수를 추가한다.
	bool has_sub_folders(CString root);

	void save_dqlist(std::deque<CString>* dqlist, CString output_text_file_path);

	//위의 FindAllFiles에서는 "파일명*"과 같이 찾게 되는데 이럴 경우 시리즈 이름의 다른 파일들도 모두 찾아진다.
	//따라서 파일명은 확정되고 확장자만 여러가지인 경우는 아래 함수를 이용해야 한다.
	//(a.jpg, a.png등을 찾고 싶은데 a1.jpg가 있어도 true가 되기 때문에)
	//(a 00.jpg ~ a 99.jpg = a ??.jpg로 찾을 수 있다. 즉, "타이틀 ??"이고 ?에 숫자가 오면 다 찾아줘야할듯한데 우선 패스)
	std::deque<CString>		FindFilesWithExtensions(CString folder, CString fileTitle, CString extensions);

	// 폴더의 모든 파일을 지운다.
	int			delete_all_files(CString folder, CString name_filter, CString ext_filter, bool recursive = true, bool trash_can = false);
	bool		DeleteFolder(LPCTSTR lpFolder);
	bool		SHDeleteFolder(CString sFolder);


	//풀패스를 주면 폴더를 자동으로 만들어준다.
	//폴더가 이미 존재해도 true를 리턴함.
	//lpPathName은 설령 그 형식이 파일명이라도
	//무조건 그 이름과 동일한 폴더를 생성하게 되므로
	//반드시 생성하고자 하는 폴더명을 사용할것.
	bool		make_full_directory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpsa = NULL);
	bool		recursive_make_full_directory(LPCTSTR sFolder);

	//폴더내의 특정 문자열이 들어간 파일들을 지운다. 하위폴더 지원안함.
	void		DeleteFilesBySubString(CString sFolder, CString filenameSubStr, bool bMatchWholeWordOnly = FALSE, bool bMatchCase = FALSE);

	//void		DeleteAllFiles(CString sFolder);

	//강제 삭제 관련 함수들
	//bool		EnableDebugPrivilege(void);
	//bool		CloseRemoteFileHandles(LPCTSTR);
	//DWORD		CloseRemoteHandle(LPCTSTR lpProcessName, DWORD processID, HANDLE handle);
	//bool		DeleteTheFile(LPCTSTR lpFileName);

	//폴더선택 대화상자를 연다.
	//strSelectedFolder : 선택한 폴더
	//strStartFolder : 기본 선택될 폴더. 시작 폴더.
	bool BrowseForFolder(HWND hwndOwner, TCHAR* lpszTitle, CString& strSelectedFolder, const TCHAR* strStartFolder = NULL, bool bNewFolderButton = true);
	static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM lpData);

	//윈도우 운영체제에서 특정 폴더(다운로드, 내 문서 등)의 실제 경로를 리턴한다.
	//FOLDERID_Downloads, FOLDERID_Documents, ...
	CString		get_known_folder(KNOWNFOLDERID folderID);
	CString		get_known_folder(int csidl);

	//사용자 문서 폴더는 "C:\Users\user_id\Documents" 와 같이 윈도우 로그인 계정인 user_id에 따라 경로가 달라지므로
	//"__user_documents_folder__"라는 키워드가 있다면 이를 "C:\Users\user_id\Documents"로 변경해준다.
	CString		adjust_special_folder_path(CString path);

//////////////////////////////////////////////////////////////////////////
//네트워크, 인터넷
	bool		GetNICAdapterList(IP_ADAPTER_INFO* pAdapters, int& nTotal, int nMax = 10);
	bool		GetNetworkInformation(CString sTargetDeviceDescription, NETWORK_INFO* pInfo);
	bool		CheckInternetIsOnline();
	bool		IsAvailableEMail(CString sEMail);

	//domain or ip 문자열을 sockaddr_in.sin_addr.S_un.S_addr 또는 in_addr.addr.S_un.S_addr 값으로 리턴한다.
	//실패 시 0을 리턴한다.
	ULONG		get_S_addr_from_domain_or_ip_str(CString domain_or_ip_str);
	CString		get_my_ip();
	CString		get_mac_addres(bool include_colon = true);
#ifndef _USING_V110_SDK71_
	CString		get_ip_error_string(DWORD error_code);
#endif
	bool		port_is_open(const std::string& address, int port);
	//ip 또는 domain의 일부 값을 '*'로 치환한다.
	CString		get_asterisk_addr(CString ip);
#if 1
	//윈도우 네트워크 정보에 설정된 proxy 정보를 읽어온다. use proxy이면 true 리턴.
	void		get_proxy_info(bool& proxy_enable, CString& ip, int& port, CString& bypass, CString& PAC_url);
#endif

//////////////////////////////////////////////////////////////////////////
//암호화
	//문자열을 암호화한다.
	void		EncryptString(CString& ToCode, TCHAR* key);
	//암호화된 문자열을 복호화한다.
	void		DecryptString(CString& ToCode, TCHAR* key);
	//숫자로 구성된 문자열을 입력받아 정해진 연산을 한 후 뒤섞인 숫자문자열을 리턴한다.
	CString		ShuffleNumericString(CString sSrc, bool bSameLength = true);

	//파일을 암호화한다.
	bool		FileEncryption(CString sfile, bool bShowErrorMessage = TRUE);


//////////////////////////////////////////////////////////////////////////
//쉘(shell), 윈도우(window), 레지스트리(registry), 시스템(system)
	//MAX_COMPUTERNAME_LENGTH(15) 길이까지만 리턴됨에 주의.
	//GetComputerName API 함수는 항상 대문자로 리턴한다.
	//그냥 확실하게 GetComputerNameString().MakeLower() 등과 같이
	//대소문자를 명확히 하여 비교하는 것이 좋다.
	//=>255글자까지 리턴되는 GetComputerNameEx()로 변경함.
	CString		get_computer_name();
	//버전정보를 구조체로 리턴
	OSVERSIONINFOEX	get_windows_version();
	//버전정보를 숫자와 '.'로 리턴. ex. "10.0.12345"
	CString		get_windows_version_number();
	DWORD		get_windows_major_version();
	//detail=true이면 edition 정보까지 포함
	CString		get_windows_version_string(bool detail = true);

	bool		open_with_explorer(CString path);

	CString		get_system_label(int csidl, int *sysIconIndex = NULL);

	bool		run_self_update_batch();

	//이 값은 윈도우가 설치될 때 생성되고 재설치되지 않으면 유지된다.
	//단, HDD 복제시에도 그대로 복사되므로 머신에 따라 unique하다고 볼 수 없다.
	CString		read_windows_GUID();
	CString		create_GUID();

	bool		is_VMWare();

	//윈도우10이상은 auto_update가 항상 true.
	//(registry에서 특정값을 추가하여 설정할 경우는 false로도 리턴됨)
	//WinXP에서 자동 업데이트를 사용하지 않음으로 해도 true이며 아래 level=1이 리턴됨.
	//level은 AutomaticUpdatesNotificationLevel 참조.
	//(0:aunlNotConfigured, 1:aunlDisabled, 2:aunlNotifyBeforeDownload, 3:aunlNotifyBeforeInstallation, 4:aunlScheduledInstallation)
	bool		get_windows_update_setting(bool& auto_update, int& level);

#ifndef _USING_V110_SDK71_
	//SystemParametersInfo(SPI_GETSCREENSAVEACTIVE...)으로는 제대로 설정값을 얻어오지 못한다.
	bool		get_screensaver_setting(int *timeout = NULL, int* use_secure = NULL);
#endif

	//좀 더 테스트 필요!
	//실행파일명으로 윈도우 핸들 리턴. 실행파일명 또는 fullpath로 검색.
	HWND		get_hwnd_by_exe_file(CString target_exe_file, DWORD except_pid = 0);
#ifndef _USING_V110_SDK71_
	HANDLE		GetProcessHandleByName(LPCTSTR szFilename);
#endif

	CWnd*		FindWindowByCaption(CString sCaption, bool bMatchWholeWord = FALSE);
	HINSTANCE	FindExecutableEx(LPCTSTR lpFile, LPCTSTR lpDir, LPTSTR lpResult);

	//Set Privilege
	bool		set_privilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);

	bool		is_exist_registry_key(HKEY hKeyRoot, CString sSubKey);

	//HKEY_LOCAL_MACHINE\\SOFTWARE\\MyCompany 에서 읽어올 경우 x64이면 실제 그 경로에서 읽어오지만
	//32bit이면 HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\MyCompany 에서 읽어온다.
	//!!반드시 Linker->Manifest File에서 Admin으로 빌드할 것!!
	LONG		get_registry_int(HKEY hKeyRoot, CString sSubKey, CString sEntry, DWORD *value);
	//!!반드시 Linker->Manifest File에서 Admin으로 빌드할 것!!
	LONG		get_registry_str(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString *str);
	//!!반드시 Linker->Manifest File에서 Admin으로 빌드할 것!!
	LONG		set_registry_int(HKEY hKeyRoot, CString sSubKey, CString sEntry, DWORD value);
	//!!반드시 Linker->Manifest File에서 Admin으로 빌드할 것!!
	LONG		set_registry_str(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString str);

	//reg_path에 해당 value 항목이 존재하지 않으면 추가한다.
	//레지스트리 해당 경로에는 "count"에 갯수가, 숫자 인덱스 항목에 각 값이 저장되는 구조로 구성된다.
	//추가된 인덱스를 리턴한다.
	//이 함수를 호출하기 전에 반드시 "count"를 리셋시켜 준 후 add_registry_str()를 호출해야 한다.
	//ex. theApp.WriteProfileInt(_T("setting\\recent files"), _T("count"), 0);
	int			add_registry_str(CWinApp* pApp, CString reg_path, CString str);
	//"count"에 항목의 갯수가 저장되어 있고 각 숫자 인덱스 항목에 값이 저장되어 있는 구조인 경우 그 목록을 리턴한다.
	//항목을 추가할때는 add_registry_str() 함수를 이용한다.
	int			get_registry_str_list(CWinApp* pApp, CString reg_path, std::deque<CString>& dqlist);

	//Windows visual effect registry
	bool		set_windows_visual_effects();

	double		GetProfileDouble(CWinApp* pApp, CString section, CString entry, double default_value);
	bool		WriteProfileDouble(CWinApp* pApp, CString section, CString entry, double value);

	//비재귀방식, key_root의 하위와 그 하위까지만 검색된다.
	std::deque<CString> get_registry_subkeys(HKEY hKeyRoot, CString key_root);

	//재귀방식으로 모든 하위 키항목이 result로 리턴된다.
	void		enum_registry_subkeys(HKEY hKeyRoot, CString key_root, std::deque<CString>& result);


#if (_MSVC_LANG >= _std_cpp17)	//__cplusplus 매크로를 사용하려면 C/C++의 고급창에서 /Zc:__cplusplus를 추가시켜야 한다.
	//프로젝트 속성에서 std:c++14를 선택하고 빌드하면 'if constexpr'은 C++17 언어 확장입니다.'라는 warning이 발생하는데
	//실제 사용코드를 돌려보면 정상적으로 동작한다.
	template <class T> inline T get_profile_value(CString section, CString entry, T default_value)
	{
		CWinApp* pApp = AfxGetApp();

		if constexpr (std::is_same_v<T, int>)
		//if (typeid(T) == typeid(int))			//compile error. C++17 이전 버전에서는 if constexpr를 사용할 수 없으므로 typeid로 타입을 검사한다.
		//if (std::is_integral<T>)				//compile error.
		//if (std::is_same_v<decltype(default_value), int>)		//compile error.
		{
			return pApp->GetProfileInt(section, entry, default_value);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			float* f;
			float res;
			UINT size = sizeof(float);
			pApp->GetProfileBinary(section, entry, reinterpret_cast<LPBYTE*>(&f), &size);

			if (f == NULL || size != sizeof(float))
				res = default_value;
			else
				res = *f;

			delete f;
			return res;
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

		if constexpr (std::is_same_v<T, int>)	//if constexpr은 C++17부터 지원되는 기능으로 T의 타입이 결정된 시점에 따라 분기처리할 수 있다.
		//if (typeid(T) == typeid(int))			//T의 타입이 int가 아닐 경우는 컴파일 에러 발생. 그러나 get_list_str(std::deque<T>& list)에서는 에러로 처리되지 않는점이 이상하다.
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

	//CTreeCtrl, CListCtrl등에서 선택된 항목 자체를 이미지로 리턴(drag시에 사용)
	void		get_GdiplusBitmap_from_selected_item(CWnd * pWnd);


	void		SystemShutdown(int nMode);		// 0:logoff  1:reboot  2:shutdown
	void		SystemShutdownNT(int nMode = 2);	// 1:reboot  2:shutdown

	void		HideTaskBar(bool bHide = TRUE);	// FALSE => Show Taskbar

	//taskbar의 크기 및 현재 show/hide 상태를 리턴한다. (옵션 설정 상태는 get_taskbar_state()으로 판별해야 함)
	bool		get_taskbar_size(CSize *sz = NULL);
	//설정값이 auto hide인지는 get_taskbar_state(ABS_AUTOHIDE)를 호출하여 판별.
	//sz를 주면 크기정보도 알 수 있다.
	//윈7부터는 ABS_ALWAYSONTOP은 항상 true이므로 ABS_ALWAYSONTOP를 판별하는 것은 의미없다.
	bool		get_taskbar_state(UINT state, CSize *sz = NULL);

//trayIcon 관련
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
	//트레이에 아이콘 잔상이 남아 있는 경우가 있는데, 이를 제거하기 위해서는 아래의 코드로 작업하는 것이 효과적이다.
	//인터넷 상에서 돌고 있는 트레이아이콘의 갯수를 구해서 트레이아이콘에 삭제 메시지를 날리는 방법은
	//프로세스가 이미 종료된 상태에서는 작동되지 않는 문제가 있기 때문이다.
	void		refresh_tray_icon();
	void		process_notification_area(HWND hWnd);
#endif

	BOOL		IsWow64();
	BOOL		IsXpOr2000();
	BOOL		Is64BitWindows();

	//MOUSEEVENTF에는 DBLCLK이 없고
	//윈도우 기본 메시지에는 DBLCLK은 있으나 CLICK은 없다.
	//따라서 event를 새로 정의하여 사용한다.
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

	//app의 위치와 크기를 레지스트리에 저장하고 실행 시 다시 복원시킨다.
	//resize가 안되는 다이얼로그의 경우는 resize시키지 않고 원래 크기로 표시해야 한다.
	//간혹 모니터를 끌 경우 앱들의 위치가 멀티모니터 위치로 이동되는 경우가 있는데
	//이럴 경우 이 함수를 호출할 때 force_primary_monitor = true로 하면
	//무조건 primary monitor에 표시되도록 보정된다.
	void		RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""), bool use_maximize = true, bool resize_window = true, bool force_primary_monitor = false);
	void		SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""));


	//CPU Usage 관련
	LPBYTE		GetPerformanceData(LPTSTR src);
	int			GetCounterValue(const int& objId, const int& counterId, const char* instanceName, PERF_DATA_BLOCK **dataBlock, LONGLONG &value);
	double		get_cpu_usage(const char* process);
	HRESULT		get_cpu_temperature(LPLONG pTemperature);

	//메모리, memory
	//현재 가용 메모리를 리턴한다. (total_memory : 전체 메모리 용량)
	uint64_t	get_available_memory(uint64_t *total_memory = NULL);

#ifndef _USING_V110_SDK71_
	//현재 프로세스의 메모리 사용량을 구한다.
	SIZE_T		GetCurrentMemUsage();
#endif
	INT			IsAvailableMemory(LPVOID pMemoryAddr);


/*
src		: 소스 데이터
srcx	: 소스 시작 x 좌표
srcy	: 소스 시작 y 좌표
srcw	: 소스 전체 width(pixel)
srch	: 소스 전체 height(pixel)
dst		: 타겟 데이터
dstx	: 타겟 x 좌표
dsty	: 타겟 y 좌표
dstw	: 타겟 전체 width(pixel)
dsth	: 타겟 전체 height(pixel)
w		: 복사할 width 크기(pixel)
h		: 복사할 height 크기(pixel)
*/
	bool		memcpy_block(uint8_t *src, int srcx, int srcy, int srcw, int srch, uint8_t *dst, int dstx, int dsty, int dstw, int dsth, int w, int h, int ch);
	ULONGLONG	get_disk_total_size(CString sDrive);
	ULONGLONG	get_disk_free_size(CString sDrive);
	//위의 함수로 크기를 구한 후 get_size_str()을 이용할 것
	//CString		GetDiskSizeString(CString sDrive, int unit = 3, int nfDigit = 0);	// "1.25G / 380.00G", nfDigit은 소수점 자리수
	CString		GetHDDSerialNumber(int nPhysicalDrive);
	CString		GetHDDVolumeNumber(CString sDrive);

	//HDD serial은 HardwareInfo.exe가 알려주는대로 S4EVNM0T230338R 15자리 형태,
	//또는 0025_3852_2190_FE03 같은 형태로 리턴된다.
	//어떤 PC는 툴이 알려주는 값과 이 프로그램에서 구한 값이 같지만(mwj, sdh)
	//어떤 PC는 툴이 알려주는 값과 다른 포맷으로 리턴한다.(scpark)
	//왜 이런 차이가 발생하는지는 아직 알 수 없으나
	//unique한 키값을 사용하고자 함이므로 우선 16자리로 맞춰서 사용한다.
	//unify16 = true이면 16자리로 통일시켜 리턴한다.
	CString		get_HDD_serial_number(int index, bool unify16 = true);

	//디스크 드라이브 목록을 얻어온다. include_legacy = true이면 floppy, cdrom까지 넣는다.
	void		get_drive_list(std::deque<CString>* drive_list, bool include_legacy = false);
	CString		get_drive_volume(TCHAR drive_letter);

	//드라이브 패스는 "C:\\"와 같이 3개 문자로 구성되고 첫문자는 대문자로 표시하는 것이 일반적이다.
	CString		normalize_drive_path(CString drive_path);

	//"로컬 디스크 (C:)" -> "C:\\"
	//"문서" -> "C:\\Documents"
	//"문서\\AnySupport" -> "C:\\Documents\\AnySupport"
	//"Seagate(\\192.168.0.52) (X:)" -> "X:"	(네트워크 드라이브)
	//하위 폴더 포함 유무에 관계없이 변환.
	//CString		convert_special_folder_to_real_path(CString special_folder, CShellImageList* plist = NULL, int index = 0);
	//"c:\\abc\\def"				=> "로컬 디스크 (C:)\\abc\\def"
	//"C:\Users\scpark\Desktop"		=> "바탕 화면"
	//"C:\Users\scpark\Documents"	=> "문서"
	//CString		convert_real_path_to_special_folder(CString real_path, CShellImageList * plist = NULL, int index = 0);

	//"c:\windows"를 입력하면 "C:\Windows"와 같이 실제 파일시스템에 저장된 경로명 리턴.
	CString		get_original_path(CString path);

	//파라미터로 들어온 연속된 파일명들을 분리한다. 실행파일명은 제외됨.(ex. command line or shell command)
	void		ParseCommandString(CString sParam, CStringArray& ar);

	//지정한 이미지를 바탕화면에 표시한다.
#ifndef _USING_V110_SDK71_
	void		set_wallpaper(CString sfile);
#endif

	//단축아이콘을 만들어준다.
	HRESULT		MyCreateShortCut(LPCTSTR pszSrcFile, LPCOLESTR pszLnkFile,
		LPTSTR pszWorkingDir = NULL, LPTSTR pszArgument = NULL, LPTSTR pszDesc = NULL);
	HRESULT		CreateShortCut(LPCTSTR pszShortcutFile, LPCOLESTR pszLink,
		LPCTSTR pszWorkingDir = _T(""), LPCTSTR pszArgument = _T(""), LPCTSTR pszDesc = _T(""));

	//printer
	int			GetPrinterList(CStringArray* arPrinter);
	CString		GetDefaultPrinterName();
	CSize		GetPrinterPaperSize(CString sPrinterName);

	//system error code를 문자열로 리턴.
	CString		get_error_str(DWORD dwError);

	//////////////////////////////////////////////////////////////////////////
	//날짜/시간 date, time 
	CString		get_date_str(CTime t, CString sep = _T("-"));
	CString		get_date_str(COleDateTime t, CString sep = _T("-"));
	CString		get_date_str(__timeb32 tb, CString sep = _T("-"));
	CString		get_date_str(SYSTEMTIME st, CString sep = _T("-"));

	CString		get_time_str(CTime t, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_str(CTimeSpan ts, bool bHasHour = true);
	CString		get_time_str(COleDateTime t, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_str(__timeb32 tb, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_str(SYSTEMTIME st, CString sep = _T(":"), bool h24 = true, bool sec = true, bool msec = true);
	CString		get_time_str(double dSecond, bool bHasHour = true, bool bHasMilliSec = false);

	//185초일 경우 "3m 5s"를 리턴한다.
	CString		get_remain_time_str(int seconds, bool include_sec = true);

	//type 0(date), 1(time:24h), 2(date+time) 년-월-일 시:분:초 형식으로 현재 시간 리턴. mid는 날짜와 시간 사이 문자열
	CString		get_datetime_str(CTime t, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_str(COleDateTime t, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_str(__timeb32 tb, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_str(SYSTEMTIME st, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true, bool msec = true);

	//type 0(date), 1(time:24h), 2(date+time) 년-월-일 시:분:초 형식으로 현재 시간 리턴. mid는 날짜와 시간 사이 문자열
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
	void		GetTimeFromSeconds(int nTotalSeconds, int& nHours, int& nMinutes, int& nSeconds);
	void		SetSystemTimeClock(WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMinute, WORD wSecond);
	double		GetElapsedTime(__timeb32 pOldTime);	//pOldTime과 현재 시간의 차이 계산
	//ts값을 넘겨 받아 "a일 b시간 c분 d초" 형태로 표시(format 0 = "?일 ?시간 ?분 ?초", 1 = "00:00:00") 
	CString		GetDayTimeCountString(int format, CTimeSpan ts, bool bShowZero = true, bool bIncludeSec = true);
	//ts값을 넘겨 받아 "a일 b시간 c분 d초" 형태로 표시
	CString		GetDayTimeCountString(COleDateTimeSpan ts, bool bShowZero, bool bIncludeSec);
	time_t		_mkgmtime(const struct tm* tm);
	time_t		_mkgmtime(const struct tm* tm);
	bool		IsAM(CTime t = 0);	//t=0이면 현재시각기준, 0보다 크면 그 시간값 기준
	CString		GetDayOfWeekString(CTime t = NULL, bool short_str = false);
	int			GetDaysOfMonth(int nYear, int nMonth);	//해당 달의 날짜수 리턴
	//날짜 관련
	int			GetSeasonIndex();	//봄=0, 여름=1...
	int			gettimeofday(struct timeval* tv, struct timezone* tz);

	//날짜 표시 형식에 맞는지 검사
	bool		is_valid_date(CString str);
	//시간 표시 형식에 맞는지 검사
	bool		is_valid_time(CString str);

	//날짜시각 형식을 yyyy/mm/dd hh:mm:ss 포맷으로 맞춘다.
	void		normalize_datetime(CString& src);

	//타이머 관련
	void		Wait(DWORD dwMillisecond);		//예전에는 OnTimer() 내에서는 동작되지 않았었는데 현재는 가능하다.
	//void		usleep(int microSec);
	void		ProcessWindowMessage();			//반복문에 의해 process가 응답없음이 되지 않도록 반복문안에서 호출하여 메시지큐의 내용을 바로 처리시킨다.
	int			ptimer_start(int instance);		//reset high resolution time lcounter 
	double		ptimer_get_time(int instance);	//get time(double precision) elaspsed since timer reset in ms
	//unit : ms
	long		getClock();

	extern bool		initialized_YUV_lookup_table;
	void		init_YUV_lookup_table();
	void		yuv420_yv12_to_bgr(unsigned char* src, unsigned char* dst, int w, int h);
	//cv::Mat		yuv420_yv12_to_bgr(uchar *pBuffer,long bufferSize, int width,int height);
	void		yuv420_nv12_to_gray(unsigned char* src, unsigned char* dst, int width, int height);
	void		yuv420_nv12_to_bgr(unsigned char* src, unsigned char* dst, int width, int height);
	void		yuv422_uyvy_to_bgr(unsigned char* src, unsigned char* dst, int w, int h);
	void		yuv_yuyv_to_bgr(unsigned char* src, unsigned char* dst, int w, int h);
	void		bgr_to_yuv422(uint8_t* src, uint8_t* dst, int width, int height, bool yuyv);	//yuyv(true) or uyuv(false)
	//convert 1ch gray to yuv_plain_gray(YYYY....128 128..128 128..)
	void		gray_to_yuv_plain_gray(uint8_t* src, uint8_t* dst, int width, int height);
	void		bgr_to_hsv(uint8_t* src, uint8_t* dst, int width, int height);


	//////////////////////////////////////////////////////////////////////////
	//GDI
	void		draw_text(CDC* pDC, int x, int y, CString text, COLORREF cr_text);
	void		draw_center_text(CDC* pdc, const CString& strText, CRect& rcRect);

	//Gdiplus
	//Gdiplus::MeasureString()의 오류를 대체.(http://www.soen.kr/lecture/library/gdiplus/1-7.htm)
	Gdiplus::RectF measure_string(Gdiplus::Graphics* g, Gdiplus::Font& font, LPCTSTR String, int length = -1);

	//Gdiplus를 이용한 텍스트 출력
	CRect		draw_text(Gdiplus::Graphics& g,
		int x, int y, int w, int h,
		CString text,
		float font_size,
		int font_style = Gdiplus::FontStyleRegular,
		int shadow_depth = 0,
		float thickness = 0.0f,
		CString font_name = _T("맑은 고딕"),
		Gdiplus::Color cr_text = Gdiplus::Color::Black,
		Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
		Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
		Gdiplus::Color cr_back = Gdiplus::Color::Transparent,
		UINT align = DT_CENTER | DT_VCENTER);

	CRect		draw_text(Gdiplus::Graphics& g,
		CRect rTarget,
		CString text,
		float font_size,
		int font_style = Gdiplus::FontStyleRegular,
		int shadow_depth = 0,
		float thickness = 0.0f,
		CString font_name = _T("맑은 고딕"),
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
		CString font_name = _T("맑은 고딕"),
		float font_size = 12.0f,
		int font_weight = DWRITE_FONT_WEIGHT_NORMAL,
		Gdiplus::Color cr_text = Gdiplus::Color::Black,
		Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
		UINT align = DT_CENTER | DT_VCENTER,
		bool show_text = true,
		bool show_shadow = true);

	CRect		draw_text(ID2D1DeviceContext* d2dc,
		Gdiplus::RectF rTarget,
		CString text,
		CString font_name = _T("맑은 고딕"),
		float font_size = 12.0f,
		int font_weight = DWRITE_FONT_WEIGHT_NORMAL,
		Gdiplus::Color cr_text = Gdiplus::Color::Black,
		Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
		UINT align = DT_CENTER | DT_VCENTER,
		bool show_text = true,
		bool show_shadow = true);
	CRect		draw_text(ID2D1DeviceContext* d2dc,
		D2D1_RECT_F rTarget,
		CString text,
		CString font_name = _T("맑은 고딕"),
		float font_size = 12.0f,
		int font_weight = DWRITE_FONT_WEIGHT_NORMAL,
		Gdiplus::Color cr_text = Gdiplus::Color::Black,
		Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
		UINT align = DT_CENTER | DT_VCENTER,
		bool show_text = true,
		bool show_shadow = true);
#endif

	void		unpremultiply(BYTE* p, UINT pixelCount);

	//text의 출력픽셀 너비가 max_width를 넘을 경우 ...와 함께 표시될 문자위치를 리턴.
	//이 함수는 DrawText시에 DT_END_ELLIPSIS를 줘서 사용하므로 우선 사용 보류!
	int			get_ellipsis_pos(CDC* pDC, CString text, int max_width);

	//20220914 DrawLine과 DrawLinePt를 같은 이름으로 하니 모호하다는 에러가 발생하여 DrawLinePt로 변경.
	void		draw_line(CDC* pDC, int x1, int y1, int x2, int y2, COLORREF cr, int thick = 1, int style = PS_SOLID, int draw_mode = R2_COPYPEN);
	void		draw_line(CDC* pDC, CPoint pt1, CPoint pt2, COLORREF cr, int thick = 1, int style = PS_SOLID, int draw_mode = R2_COPYPEN);
	void		draw_line(Gdiplus::Graphics& g, int x1, int y1, int x2, int y2, Gdiplus::Color cr, float thick = 1.0f, Gdiplus::DashStyle pen_style = Gdiplus::DashStyleSolid, int nDrawMode = R2_COPYPEN);
	void		draw_line_pt(CDC* pDC, CPoint pt1, CPoint pt2, Gdiplus::Color cr = 0, int width = 1, Gdiplus::DashStyle pen_style = Gdiplus::DashStyleSolid, int draw_mode = R2_COPYPEN);
	void		draw_rect(CDC* pDC, CRect r, COLORREF crColor = RGB(0, 0, 0), COLORREF crFill = NULL_BRUSH, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		draw_rect(CDC* pDC, CRect r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int width = 1, int pen_align = Gdiplus::PenAlignmentInset, int pen_style = Gdiplus::DashStyleSolid);
	void		draw_rect(Gdiplus::Graphics& g, CRect r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int width = 1, int pen_align = Gdiplus::PenAlignmentInset, int pen_style = Gdiplus::DashStyleSolid);
	void		draw_rect(Gdiplus::Graphics& g, Gdiplus::RectF r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int width = 1, int pen_align = Gdiplus::PenAlignmentInset, int pen_style = Gdiplus::DashStyleSolid);
#ifndef _USING_V110_SDK71_
	void		draw_line(ID2D1DeviceContext* d2dc, int x1, int y1, int x2, int y2, Gdiplus::Color cr, float thick = 1.0f);
	void		draw_rect(ID2D1DeviceContext* d2dc, CRect r, Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, float thick = 1.0f, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);
	void		draw_rect(ID2D1DeviceContext* d2dc, Gdiplus::Rect r, Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, float thick = 1.0f, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);
	void		draw_rect(ID2D1DeviceContext* d2dc, Gdiplus::RectF r, Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, float thick = 1.0f, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);

	//lt, rt, lb, rb 의 round를 각각 줄 수 있는데 lt이외의 값들 중 그 값이 음수이면 lt와 동일한 값으로 그려진다.
	ID2D1PathGeometry* draw_rect(ID2D1DeviceContext* d2dc, D2D1_RECT_F r, Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, float thick = 1.0f, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);
	ID2D1PathGeometry* create_round_path(ID2D1DeviceContext* d2dc, float x, float y, float right, float bottom, float round_lt = 0.0f, float round_rt = -1.0f, float round_lb = -1.0f, float round_rb = -1.0f);
#endif

	void		draw_sunken_rect(CDC* pDC, CRect rect, bool bSunken = true, COLORREF cr1 = GRAY(96), COLORREF cr2 = GRAY(128), int width = 1);
	void		draw_sunken_rect(CDC* pDC, CRect rect, bool bSunken = true, Gdiplus::Color cr1 = gGRAY(96), Gdiplus::Color cr2 = gGRAY(128), int width = 1);
	void		draw_ellipse(CDC* pDC, CRect r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int pen_style = PS_SOLID, int width = 1, int draw_mode = R2_COPYPEN);
	void		draw_ellipse(Gdiplus::Graphics& g, CRect r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int pen_style = PS_SOLID, int width = 1, int draw_mode = R2_COPYPEN);
	void		draw_ellipse(Gdiplus::Graphics& g, float cx, float cy, float radius, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int pen_style = PS_SOLID, int width = 1, int draw_mode = R2_COPYPEN);
	void		draw_circle(CDC* pDC, int xMidPoint, int yMidPoint, int radius);
	void		draw_polygon(CDC* pDC, std::vector<CPoint> pts, bool closed = true, COLORREF crLine = 0, int nWidth = 1, int pen_style = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		draw_arc(CDC* pDC, double cx, double cy, double r1, double r2, double start, double end, int width = 1, int pen_style = PS_SOLID, COLORREF cr = 0, int mode = R2_COPYPEN);
	bool		LoadBitmapFromFile(CBitmap& bmp, CString strFile);
	bool		SaveBitmapToTile(CBitmap* bmp, CString strFile, CWnd* pWnd);
	bool		SaveRawDataToBmp(CString sBmpFile, BYTE* pData, int w, int h, int ch);
	HANDLE		DDBToDIB(CBitmap* bitmap, DWORD dwCompression, CPalette* pPal);

	//.ico 아이콘 파일을 크기를 지정해서 로딩이 가능하다. LoadIcon()으로는 안되며 PNG와 같은 이미지도 불가하다.
	HICON		load_icon(HINSTANCE hInstance, UINT nID, int cx, int cy = 0);
	//해당 DC에 그리고 아이콘의 실제 크기를 리턴한다.
	CSize		draw_icon(CDC* pDC, HICON hIcon, CRect r);

	//font size to LOGFONT::lfHeight
	LONG		get_pixel_size_from_font_size(HWND hWnd, int font_size);
	//LOGFONT::lfHeight to font size
	LONG		get_font_size_from_pixel_size(HWND hWnd, int logical_size);

	int			GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	//jpg로 저장할 경우 quality를 지정할 수 있다. 0~100 사이의 값으로 지정하며, 100이 가장 품질이 높다.
	//bool		save(Gdiplus::Bitmap* bitmap, CString filepath, int quality = 0);
	//void		save(ID2D1DeviceContext* deviceContext, CString filepath);

//gradient_fill을 위해서 선언된 이 핸들을 사용하는 프로그램이라면
//종료될 때 해제시켜주는 함수도 반드시 호출해줘야 한다.
	typedef UINT(CALLBACK* LPFNDLLFUNC1)(HDC, CONST PTRIVERTEX, DWORD, CONST PVOID, DWORD, DWORD);
	extern HINSTANCE	g_hInst_msimg32;
	extern LPFNDLLFUNC1 g_dllfunc_GradientFill;
	void gradient_rect(CDC* pDC, CRect& rect, std::deque<Gdiplus::Color> dqColor, bool vertical = false);
	enum GRADIENT_RECT_PRESET
	{
		gradient_rect_white_black_white = 0,
		gradient_rect_white_gray128_white,
		gradient_rect_black_white_black,
		gradient_rect_black_gray128_black,
	};
	//아직 미구현
	void		gradient_rect(CDC* pDC, CRect& rect, int preset, bool vertical);
	void		safe_release_gradient_rect_handle();

	//이미지가 표시되고 있는 영역 정보와 화면상의 좌표를 주면 이미지상의 실제 좌표를 리턴한다.
	//단, 계산된 이미지상의 실제 좌표가 이미지 크기를 벗어나면 결과 변수에는 -1값을 채워서 리턴한다.
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, float sx, float sy, float* dx, float* dy);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint* pt_dst);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect* r_dst);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, Gdiplus::RectF r_src, Gdiplus::RectF* r_dst);

	//이미지가 표시되고 있는 영역 정보와 이미지 상의 좌표를 주면 화면상의 좌표를 리턴한다.
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, float sx, float sy, float* dx, float* dy);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint* pt_dst);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect* r_dst);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, Gdiplus::RectF r_src, Gdiplus::RectF* r_dst);


	//직선, Line 관련 함수
		//lower >= src <= upper 인지 판별
	template <class T> bool is_in_range(T src, T lower, T upper)
	{
		if (src >= lower && src <= upper)
			return true;

		return false;
	}

	//원점에서 벡터 b가 벡터 a의 반시계 방향이면 양수, 시계방향이면 음수, 평행이면 0을 반환 한다.
	double ccw(vector2 a, vector2 b);

	//점 p를 기준으로 벡터 b가 벡터 a의 반시계 방향이면 양수, 시계방향이면 음수, 평행이면 0을 반환 한다.
	double ccw(vector2 p, vector2 a, vector2 b);

	// - 점 a, b를 지나는 직선과 점 c, d를 지나는 직선의 교점을 x에 반환한다.
	// - 두 직선이 평행이면(겹치는 경우 포함) 거짓을, 아니면 참을 반환한다.
	bool		GetIntersectionPoint(vector2 a, vector2 b, vector2 c, vector2 d, vector2& x);
	bool		GetIntersectionPoint(CPoint* pt, CPoint* ispt);
	bool		GetIntersectionPoint(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int& isx, int& isy);

	//4점으로 구성된 두 선분이 서로 교차하는지 여부를 판별한다.
	//bCheckCrossOver = false이면 (0,1)과 (2,3)라인의 교차여부만, true이면 (0,3)과 (1,2)의 교차여부까지 판별한다.
	bool		IsIntersect2LineSegment(vector2 a, vector2 b, vector2 c, vector2 d);
	bool		IsIntersect2LineSegment(CPoint* pt, bool bCheckCrossOver);

	//점 a, b와 점 c, d가 평행한 두 선분 일 때 이들이 한 점에서 겹치는지 확인한다.
	bool		paralleSegments(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p);

	// - p가 두 점 a, b를 감싸면서 각 변이 x, y축에 평행한 최소사각형 내부에 있는지 확인한다.
	// a, b, p는 일직선 상에 있다고 가정한다.
	bool		inBoundingRectangle(vector2 p, vector2 a, vector2 b);

	// - 두 점 a, b를 지나는 선분과 두 점 c, b를 지나는 선분을 p에 반환한다.
	// - 교짐이 여러개일 경우 아무점이나 반환한다.
	bool		segmentIntersection(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p);



	//도형, 삼각형
	//삼각형 외접원의 중심을 기하학적으로 구한다.(http://kipl.tistory.com/113)
	int circumCenter(CPoint A, CPoint B, CPoint C, double* xc, double* yc);
	//삼각형 외접원의 중심을 대수적으로 구한다.
	int circumCenter2(CPoint P, CPoint Q, CPoint R, double* xc, double* yc);
	//삼각형 외접원의 반지름을 구한다.
	double circumRadius(CPoint A, CPoint B, CPoint C);


	//사각형 Rectangle
		//사각형 정보를 문자열로 리턴한다. (default : 2)
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
#ifndef _USING_V110_SDK71_
		else if (typeid(T) == typeid(D2D1_RECT_F))
		{
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
#endif
		return str;
	}

	void			make_rect(CRect& Rect, int x, int y, int w, int h);
	CRect			make_rect(int x, int y, int w, int h);
	CRect			make_center_rect(int cx, int cy, int w, int h);
	Gdiplus::Rect	make_center_gprect(int cx, int cy, int w, int h);
	CRect			gpRect_to_CRect(Gdiplus::Rect r);
	CRect			gpRectF_to_CRect(Gdiplus::RectF r);
	Gdiplus::Rect	CRect_to_gpRect(CRect r);
	Gdiplus::RectF	CRect_to_gpRectF(CRect r);

#ifndef _USING_V110_SDK71_
	D2D1_RECT_F		make_center_d2rect(float cx, float cy, float w, float h);
	void			inflate_rect(D2D1_RECT_F& r, float x, float y);
	D2D1_RECT_F		CRect_to_d2Rect(CRect r);
	D2D1_RECT_F		gpRectF_to_d2Rect(Gdiplus::RectF r);
	CRect			d2RectF_to_CRect(D2D1_RECT_F r);
	Gdiplus::RectF	d2RectF_to_gpRectF(D2D1_RECT_F r);
	D2D1_RECT_F		get_ratio_rect(D2D1_RECT_F target, float ratio, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
	D2D1_RECT_F		get_ratio_rect(D2D1_RECT_F target, float width, float height, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
#endif


	//Gdiplus::RectF는 right 또는 x2가 없고 x(left)와 Width 멤버변수만 존재힌다.
	//따라서 left만 바꾸고 싶어도 Width까지 같이 변경해줘야 한다. 이러한 이유로 set_left(), set_top() 함수를 추가함.
	//CRect는 left를 변경하면 Width()가 변경되지만 Gdiplus::Rect는 X를 변경해도 Width는 변경되지 않는다.
	void		set_left(Gdiplus::RectF& r, Gdiplus::REAL left);
	//CRect는 top을 변경하면 Height()가 변경되지만 Gdiplus::Rect는 Y를 변경해도 Height는 변경되지 않는다.
	void		set_top(Gdiplus::RectF& r, Gdiplus::REAL top);
	Gdiplus::PointF center(Gdiplus::RectF& r);

	//stroke thick를 무시하고 r에 꽉찬 path를 구하면 right, bottom이 잘리므로 stroke_thick를 고려해서 구해야 한다.
	void		get_round_rect_path(Gdiplus::GraphicsPath* path, Gdiplus::Rect r, float radius, int stroke_thick = 1);
	void		get_bowl_rect_path(Gdiplus::GraphicsPath* path, Gdiplus::Rect r, float top_radius, float bottom_radius = -1.0f);
	//radius = -1이면 height/2크기로 설정되고 이는 양 끝이 둥근 운동장 트랙 모양으로 그려진다.
	Gdiplus::GraphicsPath* draw_round_rect(Gdiplus::Graphics* g, Gdiplus::Rect r, Gdiplus::Color cr_stroke, Gdiplus::Color cr_fill, int radius = -1, int width = 1);
	CRect		getCenterRect(int cx, int cy, int w, int h);
	CRect		get_zoom_rect(CRect rect, double zoom);

	//0:lt, 1:rt, 2:rb, 3:lb, rb_cut이 true이면 끝점-1인 값을 리턴하고 false이면 끝점 좌표를 리턴한다.
	CPoint		vertex(CRect r, int index, bool rb_cut = false);

	//주어진 사각형 범위를 벗어나지 않도록 보정해준다.
	void		adjust_rect_range(float* l, float* t, float* r, float* b, float minx, float miny, float maxx, float maxy, bool retainSize, bool includeBottomRight = false);
	//이미지의 경우 includeBottomRight은 false로 해야 끝점 좌표가 유효하다.(도형일 경우는 true)
	void		adjust_rect_range(CRect& rect, CRect rLimit, bool bRetainSize = true, bool includeBottomRight = false);
	void		adjust_rect_range(CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize = true, bool includeBottomRight = false);
	void		adjust_rect_range(Gdiplus::RectF& rect, Gdiplus::RectF rLimit, bool bRetainSize = true, bool includeBottomRight = false);

	void		normalize_rect(Gdiplus::RectF& r);

	//모니터의 한쪽에 붙은 사각형을 새로운 크기로 변경할 경우 붙은 상태를 유지하고 변경할 필요가 있을 경우 사용.
	void		adjust_with_monitor_attached(CRect rOld, CRect& rNew);

	//rTarget에 접하는 dRatio를 유지하는 최대 사각형을 구한다.
	//stretch = false로 주면 확대하지 않지만 큰 경우에는 축소한다.
	//CRect		get_ratio_rect(CRect rTarget, double dRatio, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
	CRect		get_ratio_rect(CRect rTarget, int w, int h, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
	//w x h 사각형을 target안에 넣을 때 중앙에 표시되게 하는 사각형 영역을 리턴한다.
	//w, h보다 target이 적을때는 target보다 큰 영역이 리턴될 것이다.
	CRect		get_center_rect(CRect target, int w, int h);

	CRect		get_client_rect(HWND hWnd);
	//GetWindowRect()는 실제 보여지는 윈도우의 리얼 좌표가 아닌 DWM/Aero 기반의 resize border까지 포함하므로 보통 8씩 확장된 영역을 리턴한다.
	//특히 전체화면일 경우 lt는 0,0이 아닌 -8,-8로 리턴된다. 실제 보여지는 영역을 구하기 위한 함수
	CRect		get_window_real_rect(CWnd* pWnd);

	bool		pt_in_rect(CRect r, CPoint pt);
	bool		pt_in_rect(Gdiplus::RectF r, CPoint pt);
	bool		pt_in_rect_border(CRect r, CPoint pt, int sz = 0);
	bool		pt_in_rect_border(Gdiplus::RectF r, CPoint pt, int sz = 0);

	//rSub가 rMain에 완전히 속해있으면 true를 리턴한다.
	bool		rect_in_rect(CRect main, CRect sub);
	//r에서 except영역을 제외하고 cr컬러로 채운다.
	void		fill_except_rect(CDC* pDC, CRect r, CRect except, COLORREF cr);
	//두 사각형의 겹치는 영역을 리턴한다.
	CRect		get_intersect_rect(CRect r1, CRect r2);
	CRect		get_intersect_rect(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

	//두 사각형이 겹치는 정도를 r1을 기준으로 계산해서 리턴한다.
	double		getOverlappedRatio(CRect r1, CRect r2);
	double		getOverlappedRatio(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

	CRect		subtract(CRect r0, CRect r1);

	//스크린에 표시된 이미지에 그려진 사각형의 실제 이미지상의 사각형 좌표
	//sr : 이미지에 그려진 사각형
	//displayed : 이미지가 표시되고 있는 사각형 영역
	//real : 실제 이미지의 크기
	//resized : zoom in/out에 의해 변경된 크기
	CRect		get_real_from_screen_coord(CRect sr, CRect displayed, CSize real, CSize resized);

	//side 배열의 인덱스는 resize하는 영역 인덱스로서
	//DefWindowProc의 두번째 파라미터에 (SC_SIZE + m_nSideIndex)로 쓰이므로 그 차례를 따른다.
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

	void		GetSideRect(CRect src, CRect* side, int margin);
	//src사각형의 margin크기의 테두리 영역에 pt점이 존재하는 영역의 인덱스를 리턴한다.
	//인덱스는 CORNER_INDEX의 차례이며 이는 DefWindowProc에서 사용하는 차례와 동일하다.
	int			get_corner_index(CRect src, CPoint pt, int margin);

	//시작점을 주면 정사각형을 이루는 끝점 좌표를 리턴한다.
	void		getSquareEndPoint(int sx, int sy, int& ex, int& ey);

	//src 사각형의 크기조정 및 이동을 위한 9개의 사각형 값을 리턴한다.
	//handle[]은 CRect handle[9] 변수를 넘겨받는다.
	//sz는 핸들 크기 한 변의 길이가 아닌 1/2을 의미한다.
	void		get_resizable_handle(CRect src, CRect handle[], int sz = RECT_RESIZE_HANDLE_SIZE);
	void		get_resizable_handle(Gdiplus::RectF src, CRect handle[], int sz = RECT_RESIZE_HANDLE_SIZE);
	void		get_resizable_handle(Gdiplus::RectF src, std::vector<CRect>* handle, int sz = RECT_RESIZE_HANDLE_SIZE);
	//src 사각형의 크기조정 및 이동을 위한 9개의 사각형 중 pt가 위치한 사각형의 인덱스를 리턴한다.
	//인덱스 정의는 enum CORNER_INDEX 정의를 공통으로 사용한다.
	int			get_handle_index(CRect src, CPoint pt, int sz);

	//다각형 polygon 관련
		//임의 점이 다각형 내에 존재하는지 판별.
	bool PtInPolygon(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon0(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon1(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon2(CPoint* ptPolygons, CPoint pt, int nCorners);

	//다각형의 넓이를 구한다. 단, 변이 하나라도 교차되면 성립하지 않는다.
	double		GetPolygonAreaSize(CPoint* pt, int nPoints);
	//주어진 다각형 점들을 포함하는 최대 사각형을 구한다.
	CRect		get_max_rect(CPoint* pt, int nPoints);
	CRect		get_max_rect(std::vector<CPoint> pt, int pt_max = -1);


	//region 관련
	HRGN		BitmapToRegion(HBITMAP hBmp, COLORREF cTransparentColor/* = 0*/, COLORREF cTolerance/* = 0x101010*/);
	HRGN		BitmapRegion(HBITMAP hBitmap, COLORREF cTransparentColor, bool bIsTransparent);
	HRGN		CreateRgnFromBitmap(HBITMAP hBmp, COLORREF color);

	//캡쳐 기능
		//r은 윈도우 좌표계. jpg, png, bmp만 현재 지원.
	bool		capture_window(CRect r, CString filename);
	//특정 영역을 캡처하여 HBITMAP으로 리턴한다.
	//resourceID를 주면 해당 이미지를 overlay하여 리턴한다.(watermark와 같은 용도로 사용시)
	HBITMAP		capture_screen_to_bitmap(LPRECT pRect, UINT id = 0, int dx = 0, int dy = 0, bool show_cursor = false);
	HBITMAP		CaptureWindowToBitmap(HWND hWnd, LPRECT pRect = NULL);
	HBITMAP		CaptureClientToBitmap(HWND hWnd, LPRECT pRect = NULL);
	//hwnd만 주면 해당 윈도우 영역을 캡처하지만 윈도우에서는 불필요한 여백까지 영역으로 처리하므로
	//pRect를 줘서 정해진 영역만 캡처시킨다.
	HBITMAP		PrintWindowToBitmap(HWND hTargetWnd, LPRECT pRect = NULL);

	//HBITMAP
	void		draw_bitmap(HDC hdc, int x, int y, HBITMAP hBitmap);
	HRESULT		save_bitmap(HBITMAP bitmap, LPCTSTR filename);

	//키보드 언어를 그 나라 기본언어로 변경한다.
	void		ime_convert(HWND hWnd, bool bNative);
	//현재 한글 입력중인지
	bool		is_ime_composing(HWND hWnd);

	//문자입력창을 숨긴다.
	void		HideIMM(HWND hwnd);


	bool		IsLeapYear(int nYear);	//윤년인지 판단

	//CRichEditCtrlEx를 사용하여 AppendToLog함수를 이용하는 앱은 로그를 UI에 표시하기가 좋으나
	//CRichEditCtrlEx을 이용하지 못하는 상황의 앱으로 관련 코드들을 재이용하려면 코드 수정이 필요하다.
	//따라서 아래 함수를 이용한다.
	//void		AppendToLog(CWnd* pWnd,)



	//UI control 관련
	DWORD		getButtonStyle(HWND hWnd);	//button의 종류를 리턴한다.

	//연속된 버튼들에 대한 일괄 처리용 함수
	//기본 CheckRadioButton같은 경우는 unselect 기능을 제공하지 않기 때문에
	//아래 함수를 새로이 정의해서 사용한다.
	//id_offset이 0보다 작으면 first ~ last까지 모두 적용.
	void		CheckRadioButtons(CWnd* pWnd, int idFirst, int idLast, int id_offset, int nCheck = BST_CHECKED);

	//dialog based에서 키입력으로 동작을 정의하는데 CEdit과 같은 입력창에 포커스가 있으면
	//PreTranslateMessage에서 방향키나 char키를 처리하기가 곤란하다.
	//따라서 현재 포커스를 가진 컨트롤이 CEdit이고 enable이고 readonly가 아닌 경우에는
	//PreTranslateMessage에서 입력된 키를 처리하도록 한다.
	bool		IsEditCtrlAcceptKeyState(CWnd* pWnd);

	//start	: 시작 인덱스.
	//end	: 정렬을 원하는 n번째 항목
	//ex. quicksort(data, 9, 2);를 호출하면
	//2번 인덱스부터 9번째 항목인 data[2] ~ data[8]까지의 데이터가 정렬된다.
	//보통 n개의 데이터를 정렬한다면 quicksort(data, n); 이라 호출하면 된다.
	template<class T> void quicksort(T& v, int end, int start = 0, bool bAscending = true)
	{
		while (end > start)
		{
			int i = start;
			int j = end;

			do
			{
				if (bAscending)
				{
					while ((v[i] < v[start]) && (i < j))
						i++;

					//함수 원형을 quicksort(T *v, ...)와 같이 정의해서 사용하면 아래 문장에서 디버깅 에러가 발생한다.
					while (v[--j] > v[start])
						;
				}
				else
				{
					while ((v[i] > v[start]) && (i < j))
						i++;
					while (v[--j] < v[start])
						;
				}

				if (i < j)
					swap(v[i], v[j]);
			} while (i < j);

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

	/* template함수의 특징
	- 데이터 타입에 무관하게 함수 하나로 커버가 되지만
	  데이터 타입마다 함수 바디가 obj코드로 생성되는 단점이 있다고 알고있다.
	  또한 vs의 debug모드에서는 속도 저하가 발생한다.
	  release모드에서는 속도 저하가 발생하지 않는다.
	*/

	//치환 함수
#ifndef SWAP
	template<class T> void SWAP(T& x, T& y)
	{
		T temp = x;
		x = y;
		y = temp;
	}
#endif

	//클리핑 함수. 클리핑이 일어나면 true를 리턴한다. std::clamp() 권장.
	template<class T> bool Clamp(T& n, T min, T max)
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

	//범위 순환 함수
	template<class T> void Cycle(T& n, T min, T max)
	{
		if (n < min) n = max;
		else if (n > max) n = min;
	}

	//범위를 벗어나면 default값으로 세팅
	template<class T> void Validate(T& n, T min, T max, T default_value)
	{
		if (n < min || n > max)
			n = default_value;
	}

	//치환 함수
	template<class T> void Swap(T& x, T& y)
	{
		T temp = x;
		x = y;
		y = temp;
	}

	//template<typename ... T> inline T get_max(T ... args)
	//{
	//	int n = sizeof...(args);
	//	int arg[] = { args... };
	//}

	int compareInteger(const void* a, const void* b);
	int compareChar(const void* arg1, const void* arg2);
	int compareString(const void* a, const void* b);

	template<typename T> void move_item(std::deque<T>& dq, size_t from, size_t to)
	{
		if (from == to || from >= dq.size() || to >= dq.size())
			return;

		auto value = std::move(dq[from]);
		dq.erase(dq.begin() + from);
		dq.insert(dq.begin() + to, std::move(value));
	}

	//////////////////////////////////////////////////////////////////////////
	//수학
	double		tangentfunc(double x, double y);
	//두 점의 각도를 구한다. screencoord일때와 Cartesian coordinate(직교좌표계)일때는 y가 반대임에 주의.
	double		GetAngle(double vx, double vy, bool bScreenCoord = true);
	double		GetAngle(double x1, double y1, double x2, double y2, bool bScreenCoord = true);
	double		GetAngle(CPoint pt0, CPoint pt1, bool bScreenCoord = true);
	double		GetAngle(CPoint a, CPoint b, CPoint c);	//3점이 이루는 각도
	CPoint		GetCenterPoint(CPoint pt0, CPoint pt1);
	double		GetDistance(CPoint pt0, CPoint pt1);

	//두 점을 지나는 직선상의 x3 또는 y3를 구한다.
	double		getLinePointX(double x1, double y1, double x2, double y2, double y3);
	double		getLinePointY(double x1, double y1, double x2, double y2, double x3);
	double		GetManhattanDistance(double x1, double y1, double x2, double y2);
	//ptCenter를 기준으로 dAngle 만큼 회전된 dDist거리의 점의 좌표를 구한다.
	CPoint		GetRotatedPoint(CPoint ptCenter, double dAngle, double dDist);
	//cx, cy를 중심으로 tx, ty점이 degree를 회전할 경우 tx, ty점의 변경 좌표
	void		get_rotated(int cx, int cy, int* tx, int* ty, double degree);
	std::vector<CPoint>	get_rotated(int cx, int cy, CRect* r, double degree);

	//지도 좌표 <-> 도분초 변환
	double		gps_to_double(int d, int m, double s);
	CString		double_to_gps(double gps, bool is_latitude, int* d = nullptr, int* m = nullptr, double* s = nullptr);

	//src내의 모든 문자에 대해 digits자릿수의 조합 생성
	void		combination(std::vector<TCHAR> src, CString temp, std::vector<CString>& result, int depth);

	//http://www.gamedevforever.com/114 (2012년 포스트)
	//기존 rand()에 비해 분포가 고르고 속도도 빠르다고 소개되었으나
	//정작 실험해보니 분포는 유사하고 속도는 rand()가 2배 더 빨랐다.
	//release mode, 1000000개의 SetPixel, GetTickCount()로 시간체크.
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
		std::mt19937 engine((unsigned int)time(NULL));       // MT19937 난수 엔진
		std::uniform_int<> distribution(min, max);                     // 생성 범위
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
	//대각선의 길이로 가로, 세로 크기를 구한다.
	void		get_HV_angle_from_diagonal(double diagonal, double* h, double* v, double width, double height);
	void		get_HV_angle_from_diagonal(int diagonal, int* h, int* v, int width, int height);

	//numlock, capslock, scrolllock
	bool		GetLockKeyState(BYTE nLockKey);
	void		SetLockKeyState(BYTE nLockKey, bool bOn);

	HBITMAP		MakeDIBSection(CDC& dc, int width, int height);

	//2D 단일 영상에서 이미 알려진 설정값을 기준으로 영상내의 한 점과 렌즈와의 거리를 계산(by sucwon)
	//cam_height	: 카메라 설치 높이. 단위 cm
	//fl_x, fl_y	: focal length
	//c_x, c_y		: 주점
	double		getObjectDistance(int width, int height, int vanishing_y, int x, int y, int cam_height, double* dx, double* dy,
		int cali_width = 1920, int cali_height = 1080,
		double fl_x = 2361.130, double fl_y = 2357.436);

	//원본보다 크게 resize는 불가함.
	void		resize_image(uint8_t* source_ptr,
		int source_width,
		int source_height,
		uint8_t* destination_ptr,
		int destination_width,
		int destination_height);

//resize_bilinear는 ncnn에서 가져왔으나 뭔가 메모리 에러가 발생한다.
//일단 사용하지 않는다.
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

//현재는 1채널 영상만 지원된다.
//우선 급하게 -90도만 구현한다.
//dst는 반드시 메모리가 할당되어 있어야 한다.
void		rotate90(uint8_t *src, int width, int height, uint8_t *dst, int degree);

//opencv의 flip과 같은 동작이지만 이름 충돌을 피하기 위해 mirror라는 이름을 사용함.
//현재는 1채널 영상만 지원된다.
//method : 0(flip vertical), +(flip horizontal), -(both)
//dst는 반드시 메모리가 할당되어 있어야 한다.
void		mirror(uint8_t *src, int width, int height, uint8_t *dst, int method);


//MFC Common Controls
BOOL		recreate_combobox(CComboBox* pCombo, LPVOID lpParam = NULL);



//특정 목적의 함수들
//차량정보파일(xml) 저장 관련
bool		SavePlateInfoFile(char* sfile, char* sPlate, RECT* rect = NULL);

void		printMessage(std::string msg, uint8_t bNewLine = true);

int readFilenames(std::vector<std::string>& filenames, const std::string& directory);

//CMenu
//HMENU에서 메뉴ID와 캡션을 얻어온다.
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

