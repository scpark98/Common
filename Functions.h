#pragma once

#ifndef _SCPARK_FUNCTIONS_H
#define _SCPARK_FUNCTIONS_H

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

#define GDIPVER 0x0110

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

#define		IsShiftPressed() (0x8000 ==(GetKeyState(VK_SHIFT) & 0x8000  ))
#define		IsCtrlPressed()  (0x8000 ==(GetKeyState(VK_CONTROL) & 0x8000))

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
#define		ROUND(x, n) (floor((x) * pow(float(10), (n)) + 0.5f) / pow(float(10), (n)))
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

//width보다 큰 4의 배수로 만들어준다.
#define		MAKE4WIDTH_U(width)		(((width) + 3) & ~3)
//width보다 작은 4의 배수로 만들어준다.
#define		MAKE4WIDTH_D(width)		(((width) - 3) & ~3 | 4)

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
void trace(LPCTSTR format, ...);

template < typename T > class AutoEraser
{
private:
	T * VarPtr;
public:
	template < typename T2 >
	AutoEraser(T2& ptr)    {    VarPtr = (T*)ptr;   }
	~AutoEraser()           {    delete[] VarPtr;    }
};
/*
void func()
{
	MakeArray2(int,ddd,2,3);
	MakeArray3(double,fff,2,3,4);
}

ex>

int array[2][3];을 만들려고 하면..
MakeArray2(int,array,2,3);

int array[2][3][4];는
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

void		Trace(char* szFormat, ...);

//////////////////////////////////////////////////////////////////////////
//프로세스 관련
	CString		GetFileVersionInformation(CString strFileName, CString strFlag);
	CString		GetExeDirectory(bool includeSlash = false);
	CString		GetExeRootDirectory();
	CString		GetExeFilename(bool bFullPath = FALSE);
	CString		GetExeFileTitle();
	CString		GetCurrentDirectory();
	ULONG		GetPID(CString processname);
	ULONG		ProcIDFromWnd(HWND hwnd);
	HWND		GetHWNDbyPID(ULONG pid);
#ifndef _USING_V110_SDK71_
	CString		GetProcessNameByPID(const DWORD pid);
#endif
	bool		IsRunning(CString processname);
	
	bool		KillProcess(CString processname);
	//프로세스 강제 종료.
	//return value : 1 : killed, 0 : fail to kill, -1 : not found
	bool		ProcessKill(CString szProcessName);

	HWND		GetWindowHandleFromProcessID(DWORD dwProcId);
	bool		IsDuplicatedRun();
	CString		run_process(CString exePath, bool wait_process_exit);

	//PID, 프로세스 이름, 윈도우 타이틀 이름, 윈도우 클래스 이름으로 클래스의 생존 상태를 구할수 있습니다. from Devpia
	bool		CheckProcessUsingPID(unsigned long pid);
	bool		CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName);
	bool		CheckProcessUsingProcessName(LPCTSTR processName);

	//모니터 정보
	//main에서 EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); 를 실행하고
	//이 파일에 전역변수로 선언된 g_dqMonitor를 이용하면 된다.
	extern std::deque<CRect> g_dqMonitors;
	BOOL		CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
	//r이 걸쳐있는 모니터 인덱스를 리턴. 겹쳐지는 영역이 어디에도 없다면 -1을 리턴.
	//entire_included가 true이면 어떤 모니터에 완전히 속해있는 경우에만 해당 인덱스를 리턴.
	int			get_monitor_index(CRect r, bool entire_included = false);

	//::SetForegroundWindow()가 Win98이후부터는 지원되지 않아 수정된 코드.
	void		SetForegroundWindowForce(HWND hWnd, bool makeTopMost = false);

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
	CString		GetToken(CString src, CString separator, int n);
	std::deque<CString>	GetTokenString(CString src, CString separator);
	int			GetTokenString(CString src, std::deque<CString>& dqToken, TCHAR separator = ' ', bool allowEmpty = true, int nMaxToken = -1);
	int			GetTokenString(CString src, std::deque<CString>& dqToken, std::deque<TCHAR> separator, bool allowEmpty = true, int nMaxToken = -1);
	int			getTokenString(TCHAR *src, TCHAR *separator, CString *sToken, int nMaxToken);
	int			getTokenString(char *src, char *separator, char **sToken, int nMaxToken);

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
	CString		get_str(CString& buff, CString sep = _T("|"));
	int			get_int(CString& buff, CString sep = _T("|"));
	double		get_double(CString& buff, CString sep = _T("|"));
	//unit			: 0:bytes, 1:KB, 2:MB, 3:GB
	//floats		: 소수점을 몇 자리까지 표시할지
	//unit_string	: 단위를 표시할 지
	//comma			: 정수 부분에 자리수 콤마를 표시할 지
	CString		get_size_string(int64_t size, int unit = 1, int floats = 0, bool unit_string = true, bool comma = true);

	//src를 파싱해서 특정 길이 이상의 문자열들로 나눈다.
	std::deque<CString> parse_divide(CString src, int len);
	//src의 pos근처에서 구두점을 찾아 그 위치를 리턴한다.(구두점으로 문장을 나눌때 이용)
	int			find_punctuation(CString src, int pos);
	bool		is_punctuation(TCHAR ch);

	//맨 마지막 인자는 반드시 NULL을 넣어줘야 끝을 알 수 있다.
	bool		isOneOf(LPCTSTR src, ...);
	//src 문자열에 set_of_keyword에 나열된 단어가 있는지 검사.
	//set_of_keyword는 세미콜론으로 구분해서 여러 문자 또는 문자열을 넣을 수 있다.
	//ex. src = "abcd1234"일 때 set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
	bool		is_exist_keyword(CString src, CString set_of_keyword, bool case_sensitive = false, bool whole_word = false);

	//dqList에서 element값과 일치하는 항목의 index를 리턴한다. 없으면 -1을 리턴.
	template <typename T> int find_index(std::deque <T> *dqList, T element)
	{
		std::deque<T>::iterator it = std::find(dqList->begin(), dqList->end(), element);
		if (it != dqList->end())
		{
			return distance(dqList->begin(), it);
		}

		return -1;
	}

	//str의 from 위치 이후에 있는 숫자 영역값을 num에 넣어주고 숫자 시작위치를 return한다.
	int	extract_digit_number(char *str, int from, double *num);

	//version string valid check
	//digits : 자릿수(1.0.0.1일 경우는 자릿수 4)
	bool valid_version_string(CString versionStr, int digits);
	//그냥 문자열로 비교하면 1.0.9.0이 1.0.10.0보다 더 크다고 나오므로 .을 없앤 숫자로 비교한다.
	//리턴값은 strcmp와 동일한 규칙으로 판단한다.(+:ver0가 큼, -:ver1이 큼, 0:같음)
	int	compare_version_string(CString ver0, CString ver1, TCHAR separator = '.');

	//http://yeobi27.tistory.com/280
	//A2W, A2T 및 그 반대 매크로들은 스택을 사용하므로 문제 소지가 있고 크기 제한도 있으므로
	//가급적 CA2W, CA2T등을 사용한다. 단 이 매크로들은 encoding을 변경할 수 없다.
	std::wstring CString2wstring(const char* str);
	std::string CString2string(CString cs);
	CString		string2CString(std::string s);
	//멀티바이트 환경에서 이 함수를 호출해서 사용하면 간혹 비정상적으로 동작한다.
	//아마도 함수내에서 메모리가 할당된 후 호출한 곳에서 사용하려니 문제가 될 수 있다.
	//이 함수의 바디를 그대로 쓰면 문제가 없으므로 일단 바디 코드를 그대로 복사해서 사용한다.
	LPCWSTR		CString2LPCWSTR(CString str);
	char*		CString2char(CString str);
	TCHAR*		CString2TCHAR(CString str);
	LPCSTR		CString2LPCSTR(CString str);
	LPCWSTR		LPCTSTR2LPCWSTR(LPCTSTR str, UINT codePage = CP_UTF8);
	WCHAR*		CString2WCHAR(CString str); //{ return (WCHAR*)(const WCHAR*)CStringW(str); }

	//chStr의 유효한 길이를 이미 알고 있다면 length를 지정해줘야 정확하다.
	//그렇지 않을 경우 chStr의 끝에 '\0'가 없을 경우 쓰레기 문자들까지 포함될 수 있다.
	CString		char2CString(char *chStr, int length = -1);
	CString		TCHAR2CString(TCHAR *str);
	VARIANT		CString2VARIANT(CString str);

	CStringA	UTF16toUTF8(const CStringW& utf16);
	CStringW	UTF8toUTF16(const CStringA& utf8);
	CString		UTF8toCString(char* pszCode);
	CString		utf8ToCString(std::string inputtext);
	std::string	multibyteToUtf8(std::string inputtext);
	std::string	utf8ToMultibyte(std::string inputtext);


	std::string	CStringToUtf8(CString inputtext);



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
	std::string string_format(const std::string fmt_str, ...);	//A safer and more efficient(http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf)
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



//데이터 변환
	CString		i2S(int64_t nValue, bool bComma = false, bool fill_zero = false, int digits = 0);
	CString		i2HS(int64_t nValue, bool bCapital = true);
	CString		d2S(double dValue, bool bComma = false, int nfDigit = -1);	//nfDigit : 소수점 자릿수. -1이면 그대로 출력.
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
	template<class T> CString getBinaryString(T number, bool blank = true)
	{
		int i;
		std::vector<char> result;
		CString str;

		for (i = 0; number > 0; i++)
		{
			result.push_back('0' + number % 2);
			number /= 2;
		}

		for (i = 0; i < result.size(); i++)
		{
			str = result.at(i) + str;
			if (blank && (i % 4 == 3))
				str = _T(" ") + str;
		}

		str.Trim();

		return str;
	}

	unsigned __int64 binaryStringToInt64(char *str, int len);
	unsigned int getBitsValueFromInt64(uint8_t* bt, int num_of_bytes, int startbit, int bit_length);
	unsigned int getBitsValueFromInt64(CString *btStr, int num_of_bytes, int startbit, int bit_length);
	unsigned int parse_can_data(unsigned char* data,unsigned int startbit,unsigned int length);				//from ADAS source

	int			getSignedFromUnsigned(unsigned int value, int bit_length);
	unsigned int	Crc16(unsigned char* rdata, unsigned int len);
	uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte);
	uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size);




//////////////////////////////////////////////////////////////////////////
//파일 관련
	CString		GetFileNameFromFullPath(CString fullpath);
	CString		GetFolderNameFromFullPath(CString fullpath, bool includeSlash = false);	//= PathRemoveFileSpec
	CString		GetFileTitle(CString fullpath);
	CString		GetFileExtension(CString filename, bool dot = false);
	int			GetFileTypeFromFilename(CString filename);
	int			GetFileTypeFromExtension(CString sExt);
	bool		ChangeExtension(CString& filepath, CString newExt, bool applyRealFile);
	CString		normalize_path(CString& filepath);

	//폴더에 있는 파일들 중 filetitle이고 extension에 해당하는 파일명을 리턴한다.
	std::deque<CString>		get_filename_from_filetitle(CString folder, CString filetitle, CString extension);
	std::deque<CString>		get_filename_from_filetitle(CString filename, CString extension);

	uint64_t	get_file_size(CString sfile);
	uint64_t	get_folder_size(CString path);
	//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
	//unit_string	: 단위를 표시할 지 (default = true)
	//폴더인 경우는 ""를 리턴함.
	CString		get_file_size_string(CString sfile, int unit = 1, int floats = 0, bool unit_string = true);
	CTime		GetFileCreationTime(CString sfile);
	CTime		GetFileLastModifiedTime(CString sfile);
	CTime		GetFileLastAccessTime(CString sfile);
	CString		GetMostRecentFile(CString sFolder, CString sWildCard = _T("*.*"), int nReturnType = 1);
	CString		GetMostRecentDateFile(CString sFolder, CString sWildCard = _T("*.*"));	//가장 최근 날짜 파일명 리턴
	CString		GetNextIndexFile(CString sCurrentFile, bool bNext = TRUE);	//다음 인덱스의 파일명 리턴
	int			GetNextFileIndex(CString sCurrentFile);						//다음 파일의 인덱스 리턴
	int			GetNumberFromFile(CString sfile);			//파일을 읽어서 한개의 숫자값을 리턴한다. (파일이 없으면 -99999 를 리턴한다.)
	bool		WriteNumberToFile(CString sfile, int n);	//숫자값을 파일에 기록해준다.
	CString		GetFileProperty(CString sFilePath, CString sProperty);
	size_t		read_raw(CString sfile, uint8_t *dst, size_t size);
	bool		save2raw(CString sfile, uint8_t *data, size_t size);
	int			RenameFiles(CString folder, CString oldName, CString newName, bool overwrite = false, bool bWholename = true, bool bRecursive = false);
	bool		delete_file(CString fullpath, bool bTrashCan = false);
	int			get_text_encoding(CString sfile);
	bool		save(CString filepath, CString text);
	bool		file_open(FILE** fp, CString mode, CString file);

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
	bool		is_valid_url(CString url);
	void		GetURLFileInfo(CString sURL, bool &bInURL, bool &bFileType);
	bool		ReadURLFile(LPCTSTR pUrl, CString &strBuffer);
	void		ReadURLFileString(CString sURL, CString &sString);

	//url상의 파일의 내용을 읽거나 로컬 파일로 다운로드 한다.
	//local_path가 지정되어 있으면 파일로 다운받고(이때 리턴값은 "ok", 에러가 있을 경우는 에러 메시지)
	//없으면 문자열로 리턴받는다.
	CString		get_uri(CString full_remote_url, CString local_path = _T(""));
	CString		get_uri(CString ip, int port, CString remote_path, CString local_path = _T(""));

	CString		GetDefaultBrowserPath();	//[출처] [VC++] Windows 기본 웹 브라우저 파일 경로 얻어오기|작성자 데브머신
	//Content-Type: multipart/form-data 형식을 이용한 웹서버로의 파일 전송 함수
	bool		HttpUploadFile(CString url, CString filepath, int chatIndex);

	//서버의 한글명 파일에 대한 처리때문에 get_uri()함수 대신 추가하여 테스트 해봤으나
	//서버측의 문제인듯하여 우선 이 함수 사용은 보류중...
	bool		HttpDownloadFile(CString url, CString local_path = _T(""));


//webView2 Runtime
	bool		is_WebView2Runtime_installed();
	bool		install_WebView2Runtime(CString runtimeExePath, bool silentInstall);

//////////////////////////////////////////////////////////////////////////
//폴더 관련
	bool		IsFolder(CString sfile);				//폴더인지 파일인지
	bool		isFolder(char *sfile);
	//파일명이나 폴더명에 '\\', '/' 혼용일 경우가 있으므로 CString의 '==' 연산자로 비교해선 안된다. 
	bool IsFileFolderPathIsEqual(CString file0, CString file1, bool bCaseSensitive = false);
	CString		GetParentDirectory(CString sFolder);	//현재 폴더의 상위 폴더명을 리턴한다.

	//compare_only_filename : fullpath로 정렬할지, 파일명만 추출해서 정렬할지. default = false;
	void		sort_like_explorer(std::deque<CString> *dq, bool compare_only_filename = false);
	//지정된 폴더의 파일 목록을 얻어온다.
	//sNameFilter의 와일드카드는 직접 줘서 검색해야 한다.
	//프롬프트 명령과 동일하게 물음표나 별표와 같은 와일드카드를 이용할 수 있다.
	//sNameFilter = "test*", sExtFilter = "jpg;bmp;" 와 같이 입력하면
	//test로 시작하고 확장자가 jpg, bmp인 파일 목록을 얻어온다.
	//sExceptStr = "test;temp;error" 와 같이 세미콜론으로 구분하여 검색 제외할 파일명 지정 가능.
	//주의! dqFiles는 이 함수에 의해 초기화되지 않으므로 필요한 경우 초기화하여 호출할 것!
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
	int	get_sub_folders(CString root, std::deque<CString>* list = NULL, bool special_folders = false);

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

//////////////////////////////////////////////////////////////////////////
//네트워크, 인터넷
	bool		GetNICAdapterList(IP_ADAPTER_INFO* pAdapters, int& nTotal, int nMax = 10);
	void		GetNetworkInformation(TCHAR* sFindDescription, NETWORK_INFO* pInfo);
	bool		CheckInternetIsOnline();
	int			get_char_count(CString sStr, TCHAR ch);
	bool		IsAvailableEMail(CString sEMail);


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
//쉘, 윈도우, 레지스트리, 시스템
	CString		GetComputerNameString();
	bool		GetWindowsVersion(OSVERSIONINFO& osversioninfo);
	DWORD		GetWindowsVersion();
	bool		GetWindowsVersion(DWORD& dwMajor, DWORD& dwMinor);
	//bool		GetWindowsVersion(DWORD& dwMajor, DWORD& dwMinor, DWORD& dwServicePack);
	bool		GetWindowsVersion(DWORD& dwMajor, DWORD& dwMinor, DWORD& dwPlatform);

	//좀 더 테스트 필요!
	HWND		GetHWndByExeFilename(CString sExeFile, bool bCaseSensitive = false, bool bExceptThis = true);
	HANDLE		GetProcessHandleByName(LPCTSTR szFilename);

	CWnd*		FindWindowByCaption(CString sCaption, bool bMatchWholeWord = FALSE);
	HINSTANCE	FindExecutableEx(LPCTSTR lpFile, LPCTSTR lpDir, LPTSTR lpResult);

	LONG		IsExistRegistryKey(HKEY hKeyRoot, CString sSubKey);
#ifndef _USING_V110_SDK71_
	LONG		GetRegistryValue(HKEY hKeyRoot, CString sSubKey, CString sEntry, int *value);
	LONG		GetRegistryString(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString *str);
	LONG		SetRegistryValue(HKEY hKeyRoot, CString sSubKey, CString sEntry, int value);
	LONG		SetRegistryString(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString str);
#endif
	double		GetProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double default);
	bool		WriteProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double value);

	//int			GetSystemImageListIcon(CString szFile, BOOL bDrive);

	void		SystemShutdown(int nMode);		// 0:logoff  1:reboot  2:shutdown
	void		SystemShutdownNT(int nMode = 2);	// 1:reboot  2:shutdown

	void		HideTaskBar(bool bHide = TRUE);	// FALSE => Show Taskbar
	BOOL		Is64BitWindows();
	void		ClickMouse(int x, int y);

	//app의 위치와 크기를 레지스트리에 저장하고 실행 시 다시 복원시킨다.
	//resize가 안되는 다이얼로그의 경우는 resize시키지 않고 원래 크기로 표시해야 한다.
	void		RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""), bool use_maximize = true, bool resize_window = true);
	void		SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""));


	//CPU Usage 관련
	LPBYTE		GetPerformanceData(LPTSTR src);
	int			GetCounterValue(const int& objId, const int& counterId, const char* instanceName, PERF_DATA_BLOCK **dataBlock, LONGLONG &value);
	double		GetCpuUsage(const char* process);

	//메모리, memory
#ifndef _USING_V110_SDK71_
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
uint64_t	GetDiskFreeSize(CString sDrive);
uint64_t	GetDiskTotalSize(CString sDrive);
CString		GetDiskSizeString(CString sDrive);	// "1.25G / 380.00G"
//CString		GetHDDSerialNumber(int nPhysicalDrive);
CString		GetHDDVolumeNumber(CString sDrive);
void		get_drive_map(std::map<TCHAR, CString> *drive_map);
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

CString		get_last_error_message(bool show_msgBox);
CString		get_last_error_message(DWORD errorId, bool show_msgBox);

//////////////////////////////////////////////////////////////////////////
//시간
	CString		GetDateStringFromTime(CTime t, CString sMark = _T("-"));
	CString		GetDateStringFromTime(COleDateTime t, CString sMark = _T("-"));
	CString		GetDateStringFromTime(__timeb32 t, CString sMark = _T("-"));
	CString		GetTimeStringFromTime(CTime t, CString sMark = _T(":"), bool h24 = true, bool include_seconds = true);
	CString		GetTimeStringFromTime(COleDateTime t, CString sMark = _T(":"));
	CString		GetTimeStringFromTime(__timeb32 t, CString sMark = _T(":"));
	//type 0(date), 1(time:24h), 2(date+time) 년-월-일 시:분:초 형식으로 현재 시간 리턴. mid는 날짜와 시간 사이 문자열
	CString		GetCurrentDateTimeString(int nType = 2, bool bSeparator = true, CString mid = _T(" "), bool h24 = true);
	//type 0(date), 1(time:24h), 2(date+time) 년-월-일 시:분:초 형식으로 현재 시간 리턴. mid는 날짜와 시간 사이 문자열
	CString		GetDateTimeString(CTime t, int type = 2, bool separator = true, CString mid = _T(" "), bool h24 = true);
	CString		GetTimeString(CTime t, bool bSeparator = true);
	CString		GetTimeString(COleDateTime t, bool bSeparator = true);
	CString		GetTimeString(__timeb32 t, bool bSeparator = true, bool bUnderline = false, bool bHasMilliSec = true);	//2003-04-16 18:01:00.120
	//2003-04-16 18:01:00.120
	CString		GetCurrentTimeString(bool bSeparator = true, bool bUnderline = false, bool bHasMilliSec = true);
	CTime		GetTimeFromTimeString(CString sDate, CString sTime);
	CTimeSpan	GetTimeSpanFromTimeString(CString sTime);
	CString		GetDateTimeStringFromTime(CTime t, bool bSeparator = true, bool h24 = true, bool include_seconds = true);
	CString		GetDateTimeStringFromTime(SYSTEMTIME t, bool bSeparator = true, bool h24 = true, bool include_seconds = true);
	CString		GetDateTimeStringFromTime(COleDateTime t, bool bSeparator = true);
	CTime		GetTimeFromDateTimeString(CString sDateTime);
	CString		GetTimeStringFromSeconds(double dSecond, bool bHasHour = true, bool bHasMilliSec = false);
	CString		GetTimeStringFromMilliSeconds(int ms, bool bHasHour = true, bool bHasMilliSec = true);
	int			GetSecondsFromTimeString(CString timeString);
	int			GetMilliSecondsFromTimeString(CString timeString);
	void		GetTimeFromSeconds(int nTotalSeconds, int &nHours, int &nMinutes, int &nSeconds);
	void		SetSystemTimeClock(WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMinute, WORD wSecond);
	double		GetElapsedTime(__timeb32 pOldTime);	//pOldTime과 현재 시간의 차이 계산
	//ts값을 넘겨 받아 "a일 b시간 c분 d초" 형태로 표시
	CString		GetDayTimeCountString(CTimeSpan ts, bool bShowZero, bool bIncludeSec);
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


/*
//////////////////////////////////////////////////////////////////////////
//색상
	COLORREF	get_color(CString sColor);
	COLORREF	get_color(COLORREF crOrigin, int nOffset);
	//0~9까지 미리 정해놓은 기본 색상을 리턴한다.
	COLORREF	GetDefaultColor(int idx);
	//random19937을 이용하여 랜덤 컬러를 리턴한다.
	COLORREF	get_random_color();
	bool		IsHexaColorString(CString str);
	COLORREF	get_color_from_hexa_string(CString str);
	COLORREF	GetComplementaryColor(COLORREF crColor, COLORREF crBack = RGB(255,255,255));
	void		RGB2HSL(int r, int g, int b, int& h, int& s, int& l);

	//32비트인 dw에 들어있는 R, G, B를 추출하여 16비트(5+6+5) 컬러로 리턴한다.
	WORD		RGB24ToRGB565(DWORD dw);
	//두 DWORD를 WORD로 변환하여 하나의 DWORD로 변환한다.
	DWORD		RGB24ToRGB565(DWORD rgb1, DWORD rgb2);
	//565로 변환된 WORD를 다시 24비트 RGB로 사용하기 위해 DWORD로 복원시킨다.
	DWORD		RGB565ToRGB24(WORD wd);

	//gray 계열인지
	bool		isGray(COLORREF cr, int tolerance = 0);
	//gray image를 3채널 또는 4채널 이미지 포맷으로 복사해준다.
	//dst는 반드시 할당된 메모리주소이어야 한다.
	void gray2color(uint8_t *gray, int gray_width, int gray_height, uint8_t *dst, int dst_width, int dst_height, int dst_ch, uint8_t alpha);

	//3채널 또는 4채널 이미지를 단일 흑백 이미지로 복사한다.
	//dst는 반드시 할당된 메모리주소이어야 한다.
	void color2gray(uint8_t *src, int src_width, int src_height, int src_ch, uint8_t *dst, int dst_width, int dst_height);
*/
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
	void		TextOutShadow(CDC* pDC, int x, int y, CString sText, COLORREF crText = RGB(0,0,0), COLORREF crShadow = GRAY(64), UINT nFlag = TA_LEFT | TA_TOP);
	void		DrawTextShadow(CDC* pDC, CString sText, CRect r, UINT format, COLORREF crText = RGB(255,255,255), COLORREF crShadow = GRAY(64), int offsetX = 1, int offsetY = 1);
	void		TextOutOutline(CDC* pDC, int x, int y, CString sText, COLORREF crText = RGB(0,0,0), COLORREF crBorder = RGB(255,255,255), UINT nFlag = TA_LEFT | TA_TOP);
	void		DrawTextOutline(CDC* pDC, CString sText, CRect r, UINT format, COLORREF crText = RGB(255, 255, 255), COLORREF crShadow = GRAY(64));
	void		draw_center_text(CDC* pdc, const CString& strText, CRect& rcRect);

	//Gdiplus
	void		draw_outline_text(CDC* pDC, int x, int y, CString text, int font_size, int thick,
								CString font_name = _T("맑은 고딕"),
								Gdiplus::Color crOutline = Gdiplus::Color::White,
								Gdiplus::Color crFill = Gdiplus::Color::Black,
								UINT align = DT_LEFT | DT_TOP);
	void		draw_shadow_text(CDC* pDC, int x, int y, CString text, int font_size, int depth,
								CString font_name = _T("맑은 고딕"),
								Gdiplus::Color crShadow = Gdiplus::Color::Red);

	//text의 출력픽셀 너비가 max_width를 넘을 경우 ...와 함께 표시될 문자위치를 리턴.
	//이 함수는 DrawText시에 DT_END_ELLIPSIS를 줘서 사용하므로 우선 사용 보류!
	int			get_ellipsis_pos(CDC* pDC, CString text, int max_width);

	//20220914 DrawLine과 DrawLinePt를 같은 이름으로 하니 모호하다는 에러가 발생하여 DrawLinePt로 변경.
	void		DrawLine(CDC* pDC, int x1, int y1, int x2, int y2, COLORREF crColor = 0, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		DrawLinePt(CDC* pDC, CPoint pt1, CPoint pt2, COLORREF crColor = 0, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		DrawRectangle(CDC*	pDC, CRect Rect, COLORREF crColor = RGB(0,0,0), COLORREF crFill = NULL_BRUSH, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		DrawRectangle(CDC* pDC, int x1, int y1, int x2, int y2, COLORREF crColor = RGB(0, 0, 0), COLORREF crFill = NULL_BRUSH, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		DrawSunkenRect(CDC* pDC, CRect Rect, bool bSunken = TRUE, COLORREF cr1 = GRAY(96), COLORREF cr2 = GRAY(128), int nWidth = 1);
	void		DrawEllipse(CDC* pDC, int cx, int cy, int rx, int ry, COLORREF crLine, COLORREF crFill, int nPenStyle = PS_SOLID, int nWidth = 1, int nDrawMode = R2_COPYPEN);
	void		drawCircle(CDC* pDC, int xMidPoint,  int yMidPoint,  int radius);
	void		draw_polygon(CDC* pDC, std::vector<CPoint> pts, bool closed = true, COLORREF crLine = 0, int nWidth = 1, int nPenStyle = PS_SOLID, int nDrawMode = R2_COPYPEN);
	void		drawArc(CDC *pDC, double cx, double cy,double r1, double r2, double start, double end, int width = 1, int style = PS_SOLID, COLORREF cr = 0, int mode = R2_COPYPEN);
	bool		LoadBitmapFromFile(CBitmap &bmp, CString strFile);
	bool		SaveBitmapToTile(CBitmap* bmp, CString strFile, CWnd* pWnd);
	bool		SaveRawDataToBmp(CString sBmpFile, BYTE* pData, int w, int h, int ch);
	HANDLE		DDBToDIB(CBitmap* bitmap, DWORD dwCompression, CPalette* pPal);
	HICON		LoadIconEx(HINSTANCE hInstance, UINT nID, int cx, int cy = 0);

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

//이미지가 표시되고 있는 영역 정보와 화면상의 좌표를 주면 이미지상의 실제 좌표를 리턴한다.
//단, 계산된 이미지상의 실제 좌표가 이미지 크기를 벗어나면 결과 변수에는 -1값을 채워서 리턴한다.
	void		GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, double *x, double *y);
	void		GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, CPoint *pt);
	void		GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, CRect *r);

	//이미지가 표시되고 있는 영역 정보와 이미지 상의 좌표를 주면 화면상의 좌표를 리턴한다.
	void		GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, double *x, double *y);
	void		GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, CPoint *pt);
	void		GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, CRect *r);


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


//사각형
	//사각형 정보를 문자열로 리턴한다.
	//0 : "1 2 3 4"
	//1 : "(1,2) ~ (4,8)"
	//2 : "(1,2) ~ (4,8) (2x6)"
	//3 : "l = 1, t = 2, r = 3, b = 4"
	CString		GetRectInfoString(CRect r, int nFormat);

	void		make_rect(CRect &Rect, int x, int y, int w, int h);
	CRect		make_rect(int x, int y, int w, int h);
	CRect		makeCenterRect(int cx, int cy, int w, int h);
	Gdiplus::Rect makeCenterGpRect(int cx, int cy, int w, int h);
	CRect		GpRect2CRect(Gdiplus::Rect);
	Gdiplus::Rect	CRect2GpRect(CRect r);
	Gdiplus::RectF	CRect2GpRectF(CRect r);
	void		get_round_path(Gdiplus::GraphicsPath* path, Gdiplus::Rect r, int radius);
	CRect		getCenterRect(int cx, int cy, int w, int h);
	CRect		get_zoom_rect(CRect rect, double zoom);
	//0:lt, 1:rt, 2:rb, 3:lb, rb_cut이 true이면 끝점-1인 값을 리턴하고 false이면 끝점 좌표를 리턴한다.
	CPoint		vertex(CRect r, int index, bool rb_cut = false);	

	//주어진 사각형 범위를 벗어나지 않도록 보정해준다.
	void		adjustRectRange(int32_t *l, int32_t *t, int32_t *r, int32_t *b, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool retainSize);
	//이미지의 경우 includeBR은 false로 해야 끝점 좌표가 유효하다.
	void		AdjustRectRange(CRect& rect, CRect rLimit, bool bRetainSize = true, bool includeBR = false);
	void		AdjustRectRange(CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize = true);

	//모니터의 한쪽에 붙은 사각형을 새로운 크기로 변경할 경우 붙은 상태를 유지하고 변경할 필요가 있을 경우 사용.
	void		adjust_with_monitor_attached(CRect rOld, CRect &rNew);

	//rTarget에 접하는 dRatio를 유지하는 최대 사각형을 구한다.
	CRect		GetRatioRect(CRect rTarget, double dRatio, int attach = attach_hcenter | attach_vcenter);
	CRect		GetRatioRect(CRect rTarget, int w, int h, int attach = attach_hcenter | attach_vcenter);
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
	HBITMAP		CaptureScreenToBitmap(LPRECT pRect);
	HBITMAP		CaptureWindowToBitmap(HWND hWnd, LPRECT pRect = NULL);
	HBITMAP		CaptureClientToBitmap(HWND hWnd, LPRECT pRect = NULL);
	void		WriteBMP(HBITMAP bitmap, HDC hDC, LPTSTR filename);



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
UINT		getButtonStyle(HWND hWnd);	//button의 종류를 리턴한다.

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
template<class T> void SWAP(T& x, T& y)
{
	T temp	= x;
	x		= y;
	y		= temp;
}

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




//특정 목적의 함수들
//차량정보파일(xml) 저장 관련
bool		SavePlateInfoFile(char* sfile, char* sPlate, RECT* rect = NULL);

void		printMessage(std::string msg, uint8_t bNewLine = true);

int readFilenames(std::vector<std::string> &filenames, const std::string &directory);

class CPlateCode
{
public:
	char	sLocalCode[5];
	char	sTypeCode[3];
	char	sUseCode[3];
	char	sDigitCode[5];

	CPlateCode(char* sPlate)
	{
		strcpy_s(sLocalCode, "\0");
		strcpy_s(sTypeCode, "\0");
		strcpy_s(sUseCode, "\0");
		strcpy_s(sDigitCode, "\0");

		//plate = 경기86나9993
		//local	= 경기
		//type	= 86
		//use	= 나
		//digit	= 9993
		if (strlen(sPlate) < 8)
			return;
		else if (strlen(sPlate) == 8)
		{
			strcpy_s(sLocalCode, "\0");
			strncpy_s(sTypeCode, sPlate, 2);
			sTypeCode[2] = '\0';
		}
		else
		{
			strncpy_s(sLocalCode, sPlate, 4);
			sLocalCode[4] = '\0';

			if (strlen(sPlate) == 11)
			{
				strncpy_s(sTypeCode, &(*(sPlate + 4)), 1);
				sTypeCode[1] = '\0';
			}
			else
			{
				strncpy_s(sTypeCode, &(*(sPlate + 4)), 2);
				sTypeCode[2] = '\0';
			}
		}

		strncpy_s(sUseCode, &(*(sPlate + strlen(sPlate) - 6)), 2);
		sUseCode[2] = '\0';
		strncpy_s(sDigitCode, &(*(sPlate + strlen(sPlate) - 4)), 4);
		sDigitCode[4] = '\0';
	}
};
#endif