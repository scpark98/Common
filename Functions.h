#pragma once

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
#include <gdiplus.h>

#include "../Common/colors.h"

#include <WinInet.h>

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


#ifdef _MSC_VER
#define __class_func__ __function__
#endif

//20231101 opencv에 trace가 이미 정의되어 있어서 trace를 Trace로 변경함.
//매크로로 정의되어 그런지 간혹 비정상적으로 출력되는 현상이 있다.
//일단, thread 내부에서 사용하면 오류가 발생하므로 절대 thread 내부에서는 사용하지 말것.
//20240429 thread 내부가 아닌 곳에서도 assertion failed되므로 일단 사용 금지!
//https://stackoverflow.com/questions/3211463/what-is-the-most-efficient-way-to-make-this-code-thread-safe
#define Trace(fmt, ...) trace_output(false, __function__, __LINE__, false, fmt, ##__VA_ARGS__)
#define Traceln(fmt, ...) trace_output(false, __function__, __LINE__, true, fmt, ##__VA_ARGS__)
#define Trace_only(fmt, ...) trace_output(true, __function__, __LINE__, true, fmt, ##__VA_ARGS__)

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

#define		ENUM_TO_STRING(a) #a
#define		ENUM_TO_CSTRING(a) CString(#a)
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

typedef void (WINAPI* PGNSI)(LPSYSTEM_INFO);
typedef BOOL(WINAPI* PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);


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
	text_encoding_ansi = CP_ACP,
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

//num보다 큰 n의 배수로 만들어준다.
#define		MAKE_MULTIPLY_U(num, n)		(((num) + ((n)-1)) & ~((n)-1))
//num보다 작은 n의 배수로 만들어준다.
#define		MAKE_MULTIPLY_D(num, n)		(((num) - ((n)-1)) & ~((n)-1) | (n))

extern		int			g_nDaysOfMonth[12];

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


#define SAFE_RELEASE(pObject) { if (pObject!=NULL) { pObject->Release(); pObject=NULL; } }

#define SAFE_DELETE(pData) { try { delete pData; } catch (...) { ASSERT(FALSE); } pData = NULL; } 
#define SAFE_FREE(pData) { try { free(pData); } catch (...) { ASSERT(FALSE); } pData = NULL; } 

#define SAFE_CLOSE_HANDLE(hHandle) { if (hHandle!=NULL) { CloseHandle(hHandle); hHandle = NULL; } }

#define SAFE_DELETE_ARRAY(pData) { try { delete [] pData; } catch (...) { ASSERT(FALSE); } pData = NULL; } 

//////////////////////////////////////////////////////////////////////////////////
#define SHELL_OPEN(String)	ShellExecute(NULL, TEXT("open"), String, NULL, NULL, SW_SHOWNORMAL);

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
	//url의 시작이 http인지 https인지, port가 80인지 443인지등의 정보로 판단할 수 있지만 제대로 명시되지 않거나 임의 포트번호를 사용하는 경우도 많다.
	bool		is_https = true;
	CString		body;					//post data(json format)

	//token_header.Format(_T("token: %s"), ServiceSetting::strManagerToken);
	//각 항목의 끝에는 반드시 "\r\n"을 붙여줘야하는데 이는 requestAPI()에서 알아서 처리함.
	std::deque<CString> headers;

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
	//파일, 폴더의 속성창을 표시한다.
	void		show_file_property_window(CString fullpath);
	CString		get_exe_directory(bool includeSlash = false);
	CString		get_exe_parent_directory();
	CString		get_exe_filename(bool fullpath = false);
	CString		get_exe_file_title();
	CString		GetCurrentDirectory();
	ULONG		GetPID(CString processname);
	ULONG		ProcIDFromWnd(HWND hwnd);
	HWND		GetHWNDbyPID(ULONG pid);
#ifndef _USING_V110_SDK71_
	CString		GetProcessNameByPID(const DWORD pid);
#endif
	//해당 프로세스 파일이 실행중인 인스턴스 카운트를 리턴.
	//실행 파일명만 주면 파일명만 비교하지만 전체 경로를 주면 경로까지 맞아야 카운트 됨.
	//ex. 풀패스인 c:\test.exe를 주면 d:\test.exe는 실행중이라도 카운트되지 않는다.
	int			get_process_running_count(CString processname);
	
	bool		KillProcess(CString processname);
	//프로세스 강제 종료.
	//return value : 1 : killed, 0 : fail to kill, -1 : not found
	bool		ProcessKill(CString szProcessName);

	HWND		GetWindowHandleFromProcessID(DWORD dwProcId);
	bool		IsDuplicatedRun();

	//cmd 명령 실행 후 결과를 문자열로 리턴.
	//wait_until_process_exit : 실행 프로세스가 정상 종료될때까지 기다린다.
	//return_after_first_read : wait_until_process_exit를 false로 해도 장시간 끝나지 않는 경우가 있어(ex. telnet)
	//우선 이 값이 true이면 맨 처음 read후에 바로 종료시킨다.
	//주의! osk.exe라는 가상키보드 프로그램은 system폴더에만 있고 SysWow64 폴더에는 없는데
	//32bit 프로그램에서 ShellExecute() 또는 CreateProcess()로 실행하면 SysWow64폴더에서 해당 파일을 찾으므로 실패한다.
	//c:\\windows\\system32\\osk.exe로 실행해도 SysWOW64 폴더로 redirect되므로 역시 실행되지 않는다.
	//Wow64DisableWow64FsRedirection()를 이용해서 redirection을 disable시켜주고 실행 후 복원시켜줘야 한다.
	//"dir C:\\*.*"은 성공하나 "dir \"C:\\Program Files\\*.*\"" 명령은 실패한다.
	CString		run_process(CString cmd, bool wait_until_process_exit, bool return_after_first_read = false);

	//"dir \"C:\\Program Files\\*.*\"" 명령은 잘 동작하나 ping -t와 같이 끝나지 않는 도스명령어나 notepad.exe를 실행할 경우
	//도스창이 계속 남아있다.
	CString		run_process(CString cmd);
	extern		void* g_wow64_preset;
	void		Wow64Disable(bool disable = true);

	//서비스 상태가 무엇이든 종료, 제거시킨다. sc queryex -> taskkill /pid -> sc delete
	//process_name이 주어지면 좀 더 간단히 제거된다.
	//정상 제거(또는 서비스가 없을 경우) : true
	//제거 실패 : false
	bool		kill_service(CString service_name, CString process_name = _T(""));

	//Console 명령인지 GUI 윈도우 어플리케이션인지 구분
	bool		is_gui_application(CString fullPath);

	//PID, 프로세스 이름, 윈도우 타이틀 이름, 윈도우 클래스 이름으로 클래스의 생존 상태를 구할수 있습니다. from Devpia
	bool		CheckProcessUsingPID(unsigned long pid);
	bool		CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName);
	bool		CheckProcessUsingProcessName(LPCTSTR processName);

	//모니터 정보
	//main에서 EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); 를 실행하고
	//이 파일에 전역변수로 선언된 g_dqMonitor를 이용하면 된다.
	//단, Win32API인 EnumDisplayMonitors()를 호출할때는 반드시 g_dqMonitors.clear()를 해줘야 하므로
	//enum_display_monitors()함수로 대체한다.
	extern std::deque<CRect> g_dqMonitors;
	void		enum_display_monitors();
	BOOL		CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
	//r이 걸쳐있는 모니터 인덱스를 리턴. 겹쳐지는 영역이 어디에도 없다면 -1을 리턴.
	//entire_included가 true이면 어떤 모니터에 완전히 속해있는 경우에만 해당 인덱스를 리턴.
	int			get_monitor_index(CRect r, bool entire_included = false);
	//x, y가 속해있는 모니터 인덱스를 리턴
	int			get_monitor_index(int x, int y);
	//멀티모니터 전체 영역 사각형 리턴
	CRect		get_entire_monitor_rect();

	//::SetForegroundWindow()가 Win98이후부터는 지원되지 않아 수정된 코드.
	void		SetForegroundWindowForce(HWND hWnd, bool makeTopMost = false);
	bool		is_top_most(HWND hWnd);

//클립보드 clipboard
	bool		copy_to_clipboard(HWND hWnd, CString str);

//////////////////////////////////////////////////////////////////////////
//문자열
	bool		Compare_By_Case_Sensitive(CString str1, CString str2, bool bCase);
	int			find_string(CString target, CString find_string, bool case_sensitive = false);
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
	//숫자 영역이 없으면 false를 리턴한다.
	bool		get_number_from_string(CString str, int &num, int start = 0);
	//문자열에 포함된 숫자문자를 숫자로 간주하여 비교한다. "a5"는 "a12"보다 작다.
	bool		is_greater_with_numeric(CString str0, CString str1);
	//주어진 문자열이 알파벳과 숫자로만 구성된 문자열인지 검사한다.
	//excepts에는 포함되도 되는 문자열들이 들어있는데 이들은 ;으로 구분되어 있고
	//검사하기 전에 미리 삭제한 후 검사한다.
	bool		IsAlphaNumeric(CString str, CString excepts = _T(""));

	//ASCII 코드의 #33(0x21)(' ') ~ #126(0x7E)('~') 범위인지(읽을 수 있는 문자열인지)
	bool		is_readable_char(CString src);

	//'가'~'?'범위의 한글로만 구성된 문자열인지 검사한다.
	bool		is_hangul(CString str);

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
	int			get_token_string(CString src, std::deque<CString>& dqToken, CString separator = _T("|"), bool allowEmpty = true, int nMaxToken = -1, bool include_rest = false);
	int			get_token_string(TCHAR *src, TCHAR *separator, CString *sToken, int nMaxToken);
	int			get_token_string(char *src, char *separator, char **sToken, int nMaxToken);

	//간혹 \r, \n, \t, \\등의 문자를 그대로 확인할 필요가 있다.
	CString		get_unescape_string(CString src);

	// a_value : 1.1.24050
	// b_value : Normal
	// c_value : True
	// 위와 같이 속성이름 및 값으로 매핑되는 문자열을 파싱하여 std::map에 넣어준다.
	// lfrf는 라인분리문자열이고 보통 "\n"이거나 "\r\n" 등이 있고
	// separator는 ':' 이름과 값을 구분하는 구분자이다.
	// return value : 항목의 개수
	int			get_map_string(CString src, std::map<CString, CString>& map, CString lfrf = _T("\n"), CString separator = _T(":"));

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

	//unit			: -1:auto, 0:bytes, 1:KB, 2:MB, 3:GB ~
	//auto일 경우는 1000보다 작을떄까지 나누고 소수점은 2자리까지 표시한다.(ex 7.28TB)
	//floats		: 소수점을 몇 자리까지 표시할지
	//unit_string	: 단위를 표시할 지
	//comma			: 정수 부분에 자리수 콤마를 표시할 지
	CString		get_size_string(int64_t size, int unit = 1, int floats = 0, bool unit_string = true, bool comma = true);

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


	//src 문자열에 set_of_keyword에 나열된 단어가 있는지 검사.
	//set_of_keyword는 세미콜론으로 구분해서 여러 문자 또는 문자열을 넣을 수 있다.
	//ex. src = "abcd1234"일 때 set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
	bool		is_exist_keyword(CString src, CString set_of_keyword, bool case_sensitive = false, bool whole_word = false);

	//dqList에서 element값과 일치하는 항목의 index를 리턴한다. 없으면 -1을 리턴.
	template <typename T> int find_index(std::deque <T> &dqList, T element)
	{
		std::deque<T>::iterator it = std::find(dqList.begin(), dqList.end(), element);
		if (it != dqList.end())
		{
			return distance(dqList.begin(), it);
		}

		return -1;
	}

	//str의 from 위치 이후에 있는 숫자 영역값을 num에 넣어주고 숫자 시작위치를 return한다.
	int	extract_digit_number(char *str, int from, double *num);

	//version string valid check
	//digits : 자릿수(1.0.0.1일 경우는 자릿수 4)
	bool valid_version_string(CString versionStr, int digits);

	//버전 또는 IP주소등은 그냥 문자열로 비교하면 1.0.9.0이 1.0.10.0보다 더 크다고 나오므로
	//.을 없앤 숫자로 비교했으나 이 방법도 오류 발생(1.0.1.13 > 1.0.10.3보다 크다고 판단함)
	//결국 각 자릿수끼리 구분해야 한다.
	//리턴값은 strcmp와 동일한 규칙으로 판단한다.(+:str0가 큼, -:str1이 큼, 0:같음)
	int	compare_string(CString str0, CString str1, TCHAR separator = '.');

	//src를 n번 연결한 문자열 리턴. n개의 공백, 탭이 필요할 경우 사용
	CString make_string(CString src, int n);

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
	char*		UTF8toANSI(char* pszCode);
	char*		ANSItoUTF8(char* pszCode);
	CString		utf8ToCString(std::string inputtext);
	std::string	multibyteToUtf8(std::string inputtext);
	std::string	utf8ToMultibyte(std::string inputtext);


	std::string	CStringToUtf8(CString inputtext);

	//return받은 char*는 반드시 사용 후 free()해줘야 함.
	char*		replace(char* s, const char* olds, const char* news);

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

	//simple json parser. Common/json/rapid_json 추천.
	CString		json_value(CString json, CString key);

	int			get_char_count(CString sStr, TCHAR ch, bool stop_at_first_mismatch = false, bool forward = true);
	CString		get_mac_address_format(CString src, TCHAR separator = ':');

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
	std::string base64_encode(unsigned char const*, unsigned int len);
	std::string base64_encode(const std::string& in);
	std::string base64_decode(const std::string& in);
	CString base64_encode(CString in);
	CString base64_decode(CString in);

//////////////////////////////////////////////////////////////////////////
//파일 관련
	//_tsplitpath("c:\\abc/def\\123.txt", ...)를 실행하면
	//"c:", "\\abc/def\\", "123", ".txt" 과 같이 분리되는데 기존에 사용하던 기대값과 달라 보정한다.
	//"c:\\", "c:\\abc/def", "123", "txt", "123.txt와 같이 보정한다.
	//part : fn_drive(drive), fn_folder(drive+folder), fn_last_folder(folder name), fn_title(filetitle), fn_ext(ext), fn_name(filename)
	//만약 path가 "d:\\aaa\\b.abc"이고 b.abc가 파일이 아닌 폴더라면 문제된다.
	//파일인지 폴더인지를 구분해서 처리하는 코드는 필수다.(실제 존재하는 경우에만 검사가 가능하다)
	//단, path가 "연구소문서(\\192.168.1.103) (Y:)"과 같이 네트워크 경로를 포함한 드라이브 볼륨인 경우는
	//분리해서는 안되므로 그냥 리턴해야 한다.
	CString		get_part(CString path, int part);
	enum FILENAME_PART
	{
		fn_drive,
		fn_folder,
		fn_last_folder,
		fn_title,
		fn_ext,
		fn_name,
	};
#if 0
	CString		GetFileNameFromFullPath(CString fullpath);
	CString		GetFolderNameFromFullPath(CString fullpath, bool includeSlash = false);	//= PathRemoveFileSpec
	CString		GetFileTitle(CString fullpath);
	CString		GetFileExtension(CString filename, bool dot = false);
#endif
	int			GetFileTypeFromFilename(CString filename);
	int			GetFileTypeFromExtension(CString sExt);
	bool		change_extension(CString& filepath, CString newExt, bool applyRealFile);
	CString		normalize_path(CString& filepath);

	//확장자 집합 문자열로 파일열기 대화상자의 filter string을 리턴한다.
	//simple : "bmp;jpg;jpeg;png;webp;gif;yuv;raw => "JPG files|*.jpg|bmp|*.bmp|
	//extension_group = FILE_EXTENSION_VIDEO or FILE_EXTENSION_SOUND or FILE_EXTENSION_IMAGE or FILE_EXTENSION_MEDIA...
	//현재 미완성!
	//CString		get_filter_string(CString extension_group, bool simple = true);

	//폴더에 있는 파일들 중 filetitle이고 extension에 해당하는 파일명을 리턴한다.
	std::deque<CString>		get_filelist_from_filetitle(CString folder, CString filetitle, CString extension);
	std::deque<CString>		get_filelist_from_filetitle(CString filename, CString extension);

	uint64_t	get_file_size(CString sfile);
	uint64_t	get_folder_size(CString path);
	//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
	//unit_string	: 단위를 표시할 지 (default = true)
	//폴더인 경우는 ""를 리턴함.
	CString		get_file_size_string(CString sfile, int unit = 1, int floats = 0, bool unit_string = true);
	CTime		GetFileCreationTime(CString sfile);
	//탐색기에서 복사하면 last modified time이 유지되지만 web에서 다운받은 파일일 경우는 다운받아 생성된 시각으로 변경됨.
	CTime		GetFileLastModifiedTime(CString sfile);
	CTime		GetFileLastAccessTime(CString sfile);
	CString		GetMostRecentFile(CString sFolder, CString sWildCard = _T("*.*"), int nReturnType = 1);
	CString		GetMostRecentDateFile(CString sFolder, CString sWildCard = _T("*.*"));	//가장 최근 날짜 파일명 리턴
	CString		GetFileProperty(CString sFilePath, CString sProperty);

	//확인 필요
	//CString		set_file_property(CString sFilePath, CString sProperty, CString value);

	size_t		read_raw(CString sfile, uint8_t *dst, size_t size);
	bool		save2raw(CString sfile, uint8_t *data, size_t size);
	int			RenameFiles(CString folder, CString oldName, CString newName, bool overwrite = false, bool bWholename = true, bool bRecursive = false);
	bool		delete_file(CString fullpath, bool bTrashCan = false);
	int			get_text_encoding(CString sfile);
	bool		save(CString filepath, CString text, int code_page = CP_UTF8);
	bool		file_open(FILE** fp, CString mode, CString file);

	//text 파일을 열어서 dqList에 넣어준다.
	bool		read_file(CString filepath, std::deque<CString> *dqList, bool using_utf8);

	//mp4 파일의 특정 태그 데이터 중 원하는 위치의 데이터를 추출한다.
	//MOBIS 프로젝트 저장 MP4는 mdat 필드의 0x40번지부터 28 bytes가
	//동영상이 생성된 절대시간이 저장되어 있다.
	//n은 0x3C부터 4바이트가 그 크기이다.(28 bytes)
	char*		GetDataFromMP4File(char* sfile, char* sTag, uint8_t tagStart, int tagLength);

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
	void		request_url(CRequestUrlParams* params);


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
	CString		GetParentDirectory(CString sFolder);	//현재 폴더의 상위 폴더명을 리턴한다.

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
	void		FindAllFiles(	CString sFolder, std::deque<CString> *dqFiles,
								CString sNameFilter = _T("*"), CString sExtFilter = _T("*"),
								bool bRecursive = false, CString sExceptStr = _T(""),
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

	CString		normalized_path(CString& path);

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
	CString get_known_folder(KNOWNFOLDERID folderID);
	CString get_known_folder(int csidl);

//////////////////////////////////////////////////////////////////////////
//네트워크, 인터넷
	bool		GetNICAdapterList(IP_ADAPTER_INFO* pAdapters, int& nTotal, int nMax = 10);
	bool		GetNetworkInformation(CString sTargetDeviceDescription, NETWORK_INFO* pInfo);
	bool		CheckInternetIsOnline();
	bool		IsAvailableEMail(CString sEMail);
	CString		get_mac_addres(bool include_colon = true);
	CString		get_ip_error_string(DWORD error_code);
	bool		port_is_open(const std::string& address, int port);

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
	CString		get_computer_name_string();
	//버전정보를 구조체로 리턴
	OSVERSIONINFOEX	get_windows_version();
	//버전정보를 숫자와 '.'로 리턴. ex. "10.0.12345"
	CString		get_windows_version_number();
	DWORD		get_windows_major_version();
	//detail=true이면 edition 정보까지 포함
	CString		get_windows_version_string(bool detail = true);
	//
	//CString		get_windows_version_string(CString version);

	CString		get_system_label(int csidl, int *sysIconIndex = NULL);

	//이 값은 윈도우가 설치될 때 생성되고 재설치되지 않으면 유지된다.
	//단, HDD 복제시에도 그대로 복사되므로 머신에 따라 unique하다고 볼 수 없다.
	CString		read_windows_GUID();
	CString		create_GUID();

	//윈도우10이상은 auto_update가 항상 true.
	//(registry에서 특정값을 추가하여 설정할 경우는 false로도 리턴됨)
	//WinXP에서 자동 업데이트를 사용하지 않음으로 해도 true이며 아래 level=1이 리턴됨.
	//level은 AutomaticUpdatesNotificationLevel 참조.
	//(0:aunlNotConfigured, 1:aunlDisabled, 2:aunlNotifyBeforeDownload, 3:aunlNotifyBeforeInstallation, 4:aunlScheduledInstallation)
	bool		get_windows_update_setting(bool& auto_update, int& level);

	//SystemParametersInfo(SPI_GETSCREENSAVEACTIVE...)으로는 제대로 설정값을 얻어오지 못한다.
	bool		get_screensaver_setting(int *timeout = NULL, int* use_secure = NULL);

	//좀 더 테스트 필요!
	HWND		GetHWndByExeFilename(CString sExeFile, bool bWholeWordsOnly = false, bool bCaseSensitive = false, bool bExceptThis = true);
	HANDLE		GetProcessHandleByName(LPCTSTR szFilename);

	CWnd*		FindWindowByCaption(CString sCaption, bool bMatchWholeWord = FALSE);
	HINSTANCE	FindExecutableEx(LPCTSTR lpFile, LPCTSTR lpDir, LPTSTR lpResult);

	LONG		IsExistRegistryKey(HKEY hKeyRoot, CString sSubKey);
//#ifndef _USING_V110_SDK71_
	//HKEY_LOCAL_MACHINE\\SOFTWARE\\MyCompany 에서 읽어올 경우 x64이면 실제 그 경로에서 읽어오지만
	//32bit이면 HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\MyCompany 에서 읽어온다.
	//!!반드시 Linker->Manifest File에서 Admin으로 빌드할 것!!
	LONG		get_registry_int(HKEY hKeyRoot, CString sSubKey, CString sEntry, DWORD *value);
	//!!반드시 Linker->Manifest File에서 Admin으로 빌드할 것!!
	LONG		get_registry_string(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString *str);
	//!!반드시 Linker->Manifest File에서 Admin으로 빌드할 것!!
	LONG		set_registry_int(HKEY hKeyRoot, CString sSubKey, CString sEntry, DWORD value);
	//!!반드시 Linker->Manifest File에서 Admin으로 빌드할 것!!
	LONG		set_registry_string(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString str);
//#endif

	//Windows visual effect registry
	bool		set_windows_visual_effects();


	double		GetProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double default);
	bool		WriteProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double value);

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


	BOOL		IsWow64();
	BOOL		IsXpOr2000();
	BOOL		Is64BitWindows();
	void		ClickMouse(int x, int y);

	//app의 위치와 크기를 레지스트리에 저장하고 실행 시 다시 복원시킨다.
	//resize가 안되는 다이얼로그의 경우는 resize시키지 않고 원래 크기로 표시해야 한다.
	void		RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""), bool use_maximize = true, bool resize_window = true);
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
	bool		memcpy_block(uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, int ch, uint8_t *dst);

//src에서 roi 영역을 잘라서 dst_width * dst_height 크기로 resize 시킨다.
	bool		resize_roi(uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, uint8_t *dst, int dst_width, int dst_height);

//HDD
	uint64_t	get_disk_free_size(CString sDrive);
	uint64_t	get_disk_total_size(CString sDrive);
	//위의 함수로 크기를 구한 후 get_size_string()을 이용할 것
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
	void		get_drive_map(std::map<TCHAR, CString> *drive_map, bool include_legacy = false);
	CString		get_drive_volume(TCHAR drive_letter);
	//"로컬 디스크 (C:)" <-> "C:\\" //하위 폴더 포함 유무에 관계없이 변환
	//문서 -> "C:\\Documents", 그 외 일반 폴더는 그대로 리턴.
	CString		convert_special_folder_to_real_path(CString special_folder, std::map<int, CString>* csidl_map = NULL);
	CString		convert_real_path_to_special_folder(CString real_path, std::map<int, CString>*csidl_map = NULL);


//파라미터로 들어온 연속된 파일명들을 분리한다. 실행파일명은 제외됨.(ex. command line or shell command)
	void		ParseCommandString(CString sParam, CStringArray& ar);

//지정한 이미지를 바탕화면에 표시한다.
#ifndef _USING_V110_SDK71_
void		SetWallPaper(CString sfile);
#endif

//단축아이콘을 만들어준다.
	HRESULT		MyCreateShortCut(LPCTSTR pszSrcFile, LPCOLESTR pszLnkFile,
								 LPTSTR pszWorkingDir = NULL, LPTSTR pszArgument = NULL, LPTSTR pszDesc = NULL);
	HRESULT		CreateShortCut(LPCTSTR pszShortcutFile, LPCOLESTR pszLink,
								LPCTSTR pszWorkingDir = _T(""), LPCTSTR pszArgument = _T(""), LPCTSTR pszDesc = _T(""));

//printer
	int			GetPrinterList(CStringArray *arPrinter);
	CString		GetDefaultPrinterName();
	CSize		GetPrinterPaperSize(CString sPrinterName);

	CString		get_last_error_string(bool show_msgBox = false);
	CString		get_last_error_string(DWORD dwError, bool show_msgBox = false);

//////////////////////////////////////////////////////////////////////////
//date, time 날짜/시간
	CString		get_date_string(CTime t, CString sep = _T("-"));
	CString		get_date_string(COleDateTime t, CString sep = _T("-"));
	CString		get_date_string(__timeb32 tb, CString sep = _T("-"));
	CString		get_date_string(SYSTEMTIME st, CString sep = _T("-"));

	CString		get_time_string(CTime t, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_string(COleDateTime t, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_string(__timeb32 tb, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_string(SYSTEMTIME st, CString sep = _T(":"), bool h24 = true, bool sec = true, bool msec = true);

	//type 0(date), 1(time:24h), 2(date+time) 년-월-일 시:분:초 형식으로 현재 시간 리턴. mid는 날짜와 시간 사이 문자열
	CString		get_datetime_string(CTime t, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_string(COleDateTime t, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_string(__timeb32 tb, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_string(SYSTEMTIME st, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true, bool msec = true);

	//type 0(date), 1(time:24h), 2(date+time) 년-월-일 시:분:초 형식으로 현재 시간 리턴. mid는 날짜와 시간 사이 문자열
	CString		get_cur_datetime_string(int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true, bool msec = false);


	//2003-04-16 18:01:00.120
	CString		GetCurrentTimeString(bool bSeparator = true, bool msec = false);
	CTime		GetTimeFromTimeString(CString sDate, CString sTime);
	CTimeSpan	GetTimeSpanFromTimeString(CString sTime);
	//CString		GetDateTimeStringFromTime(CTime t, bool bSeparator = true, bool h24 = true, bool include_seconds = true, bool bHasMilliSec = false);
	//CString		GetDateTimeStringFromTime(SYSTEMTIME t, bool bSeparator = true, bool h24 = true, bool include_seconds = true, bool bHasMilliSec = false);
	//CString		GetDateTimeStringFromTime(COleDateTime t, bool bSeparator = true);
	CTime		GetTimeFromDateTimeString(CString sDateTime);
	CString		GetTimeStringFromSeconds(double dSecond, bool bHasHour = true, bool bHasMilliSec = false);
	CString		GetTimeStringFromMilliSeconds(int ms, bool bHasHour = true, bool bHasMilliSec = true);
	int			GetSecondsFromTimeString(CString timeString);
	int			GetMilliSecondsFromTimeString(CString timeString);
	void		GetTimeFromSeconds(int nTotalSeconds, int &nHours, int &nMinutes, int &nSeconds);
	void		SetSystemTimeClock(WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMinute, WORD wSecond);
	double		GetElapsedTime(__timeb32 pOldTime);	//pOldTime과 현재 시간의 차이 계산
	//ts값을 넘겨 받아 "a일 b시간 c분 d초" 형태로 표시(format 0 = "?일 ?시간 ?분 ?초", 1 = "00:00:00") 
	CString		GetDayTimeCountString(int format, CTimeSpan ts, bool bShowZero = true, bool bIncludeSec = true);
	//ts값을 넘겨 받아 "a일 b시간 c분 d초" 형태로 표시
	CString		GetDayTimeCountString(COleDateTimeSpan ts, bool bShowZero, bool bIncludeSec);
	time_t		_mkgmtime(const struct tm *tm) ;
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
	void		normalize_datetime(CString & src);

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
	void		draw_center_text(CDC* pdc, const CString& strText, CRect& rcRect);

	//Gdiplus
	//Gdiplus::MeasureString()의 오류를 대체.(http://www.soen.kr/lecture/library/gdiplus/1-7.htm)
	Gdiplus::RectF measure_string(Gdiplus::Graphics* g, Gdiplus::Font& font, LPCTSTR String, int length = -1);

	//Gdiplus를 이용한 텍스트 출력
	CRect		draw_text(Gdiplus::Graphics *g,
							int x, int y, int w, int h,
							CString text,
							float font_size,
							int font_style,
							int shadow_depth,
							float thickness,
							CString font_name = _T("맑은 고딕"),
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							UINT align = DT_CENTER | DT_VCENTER);
	CRect		draw_text(Gdiplus::Graphics* g,
							CRect rTarget,
							CString text,
							float font_size,
							int font_style,
							int shadow_depth,
							float thickness,
							CString font_name = _T("맑은 고딕"),
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							UINT align = DT_CENTER | DT_VCENTER);

	//text의 출력픽셀 너비가 max_width를 넘을 경우 ...와 함께 표시될 문자위치를 리턴.
	//이 함수는 DrawText시에 DT_END_ELLIPSIS를 줘서 사용하므로 우선 사용 보류!
	int			get_ellipsis_pos(CDC* pDC, CString text, int max_width);

	//20220914 DrawLine과 DrawLinePt를 같은 이름으로 하니 모호하다는 에러가 발생하여 DrawLinePt로 변경.
	void		draw_line(CDC* pDC, int x1, int y1, int x2, int y2, Gdiplus::Color cr = Gdiplus::Color::Black, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		draw_line_pt(CDC* pDC, CPoint pt1, CPoint pt2, Gdiplus::Color cr = 0, int width = 1, int pen_style = PS_SOLID, int draw_mode = R2_COPYPEN);
	void		draw_rectangle(CDC*	pDC, CRect r, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int width = 1);
	void		draw_sunken_rect(CDC* pDC, CRect rect, bool bSunken = true, COLORREF cr1 = GRAY(96), COLORREF cr2 = GRAY(128), int width = 1);
	void		draw_sunken_rect(CDC* pDC, CRect rect, bool bSunken = true, Gdiplus::Color cr1 = gGRAY(96), Gdiplus::Color cr2 = gGRAY(128), int width = 1);
	void		draw_ellipse(CDC* pDC, int cx, int cy, int rx, int ry, Gdiplus::Color cr_line = Gdiplus::Color::Transparent, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent, int pen_style = PS_SOLID, int width = 1, int draw_mode = R2_COPYPEN);
	void		draw_circle(CDC* pDC, int xMidPoint,  int yMidPoint,  int radius);
	void		draw_polygon(CDC* pDC, std::vector<CPoint> pts, bool closed = true, COLORREF crLine = 0, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		draw_arc(CDC *pDC, double cx, double cy,double r1, double r2, double start, double end, int width = 1, int style = PS_SOLID, COLORREF cr = 0, int mode = R2_COPYPEN);
	bool		LoadBitmapFromFile(CBitmap &bmp, CString strFile);
	bool		SaveBitmapToTile(CBitmap* bmp, CString strFile, CWnd* pWnd);
	bool		SaveRawDataToBmp(CString sBmpFile, BYTE* pData, int w, int h, int ch);
	HANDLE		DDBToDIB(CBitmap* bitmap, DWORD dwCompression, CPalette* pPal);
	
	//.ico 아이콘 파일을 크기를 지정해서 로딩이 가능하다. LoadIcon()으로는 안되며 PNG와 같은 이미지도 불가하다.
	HICON		load_icon(HINSTANCE hInstance, UINT nID, int cx, int cy = 0);
	//해당 DC에 그리고 아이콘의 실제 크기를 리턴한다.
	CSize		draw_icon(CDC* pDC, HICON hIcon, CRect r);

	//font size to LOGFONT::lfHeight
	LONG		get_logical_size_from_font_size(HWND hWnd, int font_size);
	//LOGFONT::lfHeight to font size
	LONG		get_font_size_from_logical_size(HWND hWnd, int logical_size);

	int			GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	bool		save(Gdiplus::Bitmap* bitmap, CString filepath);


//gradient_fill을 위해서 선언된 이 핸들을 사용하는 프로그램이라면
//종료될 때 해제시켜주는 함수도 반드시 호출해줘야 한다.
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
	//아직 미구현
	void		gradient_rect(CDC* pDC, CRect &rect, int preset, bool vertical);
	void		safe_release_gradient_rect_handle();

//이미지가 표시되고 있는 영역 정보와 화면상의 좌표를 주면 이미지상의 실제 좌표를 리턴한다.
//단, 계산된 이미지상의 실제 좌표가 이미지 크기를 벗어나면 결과 변수에는 -1값을 채워서 리턴한다.
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, double sx, double sy, double *dx, double *dy);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint *pt_dst);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect *r_dst);

	//이미지가 표시되고 있는 영역 정보와 이미지 상의 좌표를 주면 화면상의 좌표를 리턴한다.
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, double sx, double sy, double *dx, double *dy);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint *pt_dst);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect *r_dst);


//직선, Line 관련 함수

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
	int circumCenter(CPoint A, CPoint B, CPoint C, double *xc, double *yc);
	//삼각형 외접원의 중심을 대수적으로 구한다.
	int circumCenter2(CPoint P, CPoint Q, CPoint R, double *xc, double *yc);
	//삼각형 외접원의 반지름을 구한다.
	double circumRadius(CPoint A, CPoint B, CPoint C);


//사각형 Rectangle
	//사각형 정보를 문자열로 리턴한다. (default : 2)
	//0 : "1 2 3 4"
	//1 : "(1,2) ~ (4,8)"
	//2 : "(1,2) ~ (4,8) (2x6)"
	//3 : "l = 1, t = 2, r = 3, b = 4"
	CString		get_rect_info_string(CRect r, int nFormat = 2);

	void		make_rect(CRect &Rect, int x, int y, int w, int h);
	CRect		make_rect(int x, int y, int w, int h);
	CRect		makeCenterRect(int cx, int cy, int w, int h);
	Gdiplus::Rect makeCenterGpRect(int cx, int cy, int w, int h);
	CRect		GpRect2CRect(Gdiplus::Rect);
	Gdiplus::Rect	CRect2GpRect(CRect r);
	Gdiplus::RectF	CRect2GpRectF(CRect r);
	void		get_round_rect_path(Gdiplus::GraphicsPath* path, Gdiplus::Rect r, int radius);
	void		draw_round_rect(Gdiplus::Graphics* g, Gdiplus::Rect r, Gdiplus::Color gcr_stroke, Gdiplus::Color gcr_fill, int radius, int width = 1);
	CRect		getCenterRect(int cx, int cy, int w, int h);
	CRect		get_zoom_rect(CRect rect, double zoom);

	//0:lt, 1:rt, 2:rb, 3:lb, rb_cut이 true이면 끝점-1인 값을 리턴하고 false이면 끝점 좌표를 리턴한다.
	CPoint		vertex(CRect r, int index, bool rb_cut = false);	

	//주어진 사각형 범위를 벗어나지 않도록 보정해준다.
	void		adjust_rect_range(int32_t *l, int32_t *t, int32_t *r, int32_t *b, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool retainSize);
	//이미지의 경우 includeBottomRight은 false로 해야 끝점 좌표가 유효하다.
	void		adjust_rect_range(CRect& rect, CRect rLimit, bool bRetainSize = true, bool includeBottomRight = false);
	void		adjust_rect_range(CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize = true);

	//모니터의 한쪽에 붙은 사각형을 새로운 크기로 변경할 경우 붙은 상태를 유지하고 변경할 필요가 있을 경우 사용.
	void		adjust_with_monitor_attached(CRect rOld, CRect &rNew);

	//rTarget에 접하는 dRatio를 유지하는 최대 사각형을 구한다.
	CRect		get_ratio_max_rect(CRect rTarget, double dRatio, int attach = attach_hcenter | attach_vcenter);
	CRect		get_ratio_max_rect(CRect rTarget, int w, int h, int attach = attach_hcenter | attach_vcenter);
	//w x h 사각형을 target안에 넣을 때 중앙에 표시되게 하는 사각형 영역을 리턴한다.
	//w, h보다 target이 적을때는 target보다 큰 영역이 리턴될 것이다.
	CRect		get_center_rect(CRect target, int w, int h);

	//rSub가 rMain에 완전히 속해있으면 true를 리턴한다.
	bool		RectInRect(CRect rMain, CRect rSub);
	//r에서 except영역을 제외하고 cr컬러로 채운다.
	void		fill_except_rect(CDC* pDC, CRect r, CRect except, COLORREF cr);
	//두 사각형의 겹치는 영역을 리턴한다.
	CRect		getIntersectionRect(CRect r1, CRect r2);
	CRect		getIntersectionRect(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

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

	void		GetSideRect(CRect src, CRect *side, int margin);
	//src사각형의 margin크기의 테두리 영역에 pt점이 존재하는 영역의 인덱스를 리턴한다.
	//인덱스는 CORNER_INDEX의 차례이며 이는 DefWindowProc에서 사용하는 차례와 동일하다.
	int			get_corner_index(CRect src, CPoint pt, int margin);

	//시작점을 주면 정사각형을 이루는 끝점 좌표를 리턴한다.
	void		getSquareEndPoint(int sx, int sy, int& ex, int& ey);

//다각형 polygon 관련
	//임의 점이 다각형 내에 존재하는지 판별.
	bool PtInPolygon(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon0(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon1(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon2(CPoint *ptPolygons, CPoint pt, int nCorners);

	//다각형의 넓이를 구한다. 단, 변이 하나라도 교차되면 성립하지 않는다.
	double		GetPolygonAreaSize(CPoint *pt, int nPoints);
	//주어진 다각형 점들을 포함하는 최대 사각형을 구한다.
	CRect		get_max_rect(CPoint	*pt, int nPoints);
	CRect		get_max_rect(std::vector<CPoint> pt, int pt_max = -1);


//region 관련
	HRGN		BitmapToRegion (HBITMAP hBmp, COLORREF cTransparentColor/* = 0*/, COLORREF cTolerance/* = 0x101010*/);
	HRGN		BitmapRegion(HBITMAP hBitmap, COLORREF cTransparentColor, bool bIsTransparent);
	HRGN		CreateRgnFromBitmap(HBITMAP hBmp, COLORREF color);

//캡쳐 기능
	//r은 윈도우 좌표계.
	CImage*		capture_window(CRect r, CString filename);
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
	void		save_bitmap(HBITMAP bitmap, LPCTSTR filename);

//키보드 언어를 그 나라 기본언어로 변경한다.
void		IME_Convert_To_NativeCode(HWND hWnd, bool bNative);


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
void		CheckRadioButtons(CWnd *pWnd, int idFirst, int idLast, int id_offset, int nCheck = BST_CHECKED);

//dialog based에서 키입력으로 동작을 정의하는데 CEdit과 같은 입력창에 포커스가 있으면
//PreTranslateMessage에서 방향키나 char키를 처리하기가 곤란하다.
//따라서 현재 포커스를 가진 컨트롤이 CEdit이고 enable이고 readonly가 아닌 경우에는
//PreTranslateMessage에서 입력된 키를 처리하도록 한다.
bool		IsEditCtrlAcceptKeyState(CWnd *pWnd);

//start	: 시작 인덱스.
//end	: 정렬을 원하는 n번째 항목
//ex. quicksort(data, 9, 2);를 호출하면
//2번 인덱스부터 9번째 항목인 data[2] ~ data[8]까지의 데이터가 정렬된다.
//보통 n개의 데이터를 정렬한다면 quicksort(data, n); 이라 호출하면 된다.
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

				//함수 원형을 quicksort(T *v, ...)와 같이 정의해서 사용하면 아래 문장에서 디버깅 에러가 발생한다.
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
	T temp	= x;
	x		= y;
	y		= temp;
}
#endif

//클리핑 함수. 클리핑이 일어나면 true를 리턴한다.
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

//범위 순환 함수
template<class T> void Cycle(T& n, T min, T max)
{
	if (n < min) n = max;
	else if (n > max) n = min;
}

//범위를 벗어나면 default값으로 세팅
template<class T> void Validate(T& n, T min, T max, T default)
{
	if (n < min || n > max)
		n = default;
}

//치환 함수
template<class T> void Swap(T& x, T& y)
{
	T temp	= x;
	x		= y;
	y		= temp;
}

int compareInteger (const void * a, const void * b);
int compareChar(const void *arg1, const void *arg2);
int compareString (const void * a, const void * b);


//////////////////////////////////////////////////////////////////////////
//수학
double		tangentfunc(double x, double y);
//두 점의 각도를 구한다. screencoord일때와 Cartesian coordinate(직교좌표계)일때는 y가 반대임에 주의.
double		GetAngle(double vx, double vy, bool bScreenCoord = true);
double		GetAngle(double x1, double y1, double x2,  double y2, bool bScreenCoord = true);
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
double		convert_gps_coord(int d, int m, double s);
void		convert_gps_coord(double gps, int &d, int &m, double &s);

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
template<typename T> double standardDeviation(std::deque<T> v) 
{
	T sum = std::accumulate(v.begin(), v.end(), 0.0);
	double mean = sum / v.size();

	double squareSum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
	return sqrt(squareSum / v.size() - mean * mean);
}

double		Rounding(double x, int digit);
//대각선의 길이로 가로, 세로 크기를 구한다.
void		get_HV_angle_from_diagonal(double diagonal, double *h, double *v, double width, double height);
void		get_HV_angle_from_diagonal(int diagonal, int *h, int *v, int width, int height);

//numlock, capslock, scrolllock
bool		GetLockKeyState(BYTE nLockKey);
void		SetLockKeyState(BYTE nLockKey, bool bOn);

HBITMAP		MakeDIBSection(CDC& dc, int width, int height);

//2D 단일 영상에서 이미 알려진 설정값을 기준으로 영상내의 한 점과 렌즈와의 거리를 계산(by sucwon)
//cam_height	: 카메라 설치 높이. 단위 cm
//fl_x, fl_y	: focal length
//c_x, c_y		: 주점
double		getObjectDistance(	int width, int height, int vanishing_y, int x, int y, int cam_height, double *dx, double *dy,
								int cali_width = 1920, int cali_height = 1080,
								double fl_x = 2361.130, double fl_y = 2357.436);

//원본보다 크게 resize는 불가함.
void		resize_image(uint8_t *source_ptr,
						int source_width, 
						int source_height, 
						uint8_t *destination_ptr, 
						int destination_width, 
						int destination_height);


//resize_bilinear는 ncnn에서 가져왔으나 뭔가 메모리 에러가 발생한다.
//일단 사용하지 않는다.
//void		resize_bilinear_c1(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h);
//void		resize_bilinear_c3(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h);
//void		resize_bilinear_c4(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h);
//void		resize_bilinear_image(uint8_t *src, int sw, int sh, uint8_t *dst, int w, int h);
void resize11(int* input, int* output, int sourceWidth, int sourceHeight, int targetWidth, int targetHeight);

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

int readFilenames(std::vector<std::string> &filenames, const std::string &directory);

//CMenu
//HMENU에서 메뉴ID와 캡션을 얻어온다.
bool	get_menu_item_info(HMENU hMenu, UINT uItem, UINT *uID, CString *caption, BOOL fByPos = FALSE);
//#endif
