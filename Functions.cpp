
#include <afxinet.h>	// for Internet
#include <string>
#include <utility>
#include <fstream>
#include <filesystem>
#include <regex>		//require c++11

#include <imm.h>
#include <comutil.h>	//for _bstr_t
#include <TlHelp32.h>	//for CreateToolhelp32Snapshot
#include "Psapi.h"		//for GetCurrentMemUsage()
#include <wuapi.h>		//for windows update option check

#include "Functions.h"

//아래 두 라인은 GetWindowsVersion()함수를 위해 포함되었는데
//Functions.h에 include할 경우
//NH의 LiveWebClient에서 사용하는 opencv와 뭔가 충돌이 생긴다.
//가능하면 .h에 include를 최소화 할 필요가 있다.
#include <LM.h>
#pragma comment(lib, "netapi32.lib")

//get_proxy_info()를 위해 추가했으나 HTTP_VERSION_INFO 재정의 등 충돌 발생하여 우선 주석처리함. (XP호환이 원인)
//#include <winhttp.h>
//#pragma comment(lib, "Winhttp.lib")


#include <limits>

//#include "SystemInfo.h"
#pragma comment(lib, "Psapi.lib")

//for GUID
#pragma comment(lib, "Rpcrt4.lib")
#include <Rpcdce.h>

//for file property api
#include <propvarutil.h>
#pragma comment(lib, "propsys.lib")

#pragma warning(disable : 4018)		// '<': signed/unsigned mismatch
#pragma warning(disable : 4127)		// conditional expression is constant
#pragma warning(disable : 4244)		// possible loss of data
#pragma warning(disable : 4477)		// CString
#pragma warning(disable : 4800)		// 'BOOL': forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable : 4996)		// disable bogus deprecation warning

#undef min
#undef max

#define PTIMER_INSTANCE_NUM	100
__int64 i64_freq[PTIMER_INSTANCE_NUM];
__int64 i64_start[PTIMER_INSTANCE_NUM];
__int64 i64_end;

double		g_dTime0;
double		g_dTime1;
CString		g_sForTimer;

int			g_nDaysOfMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int			g_nBaudRate[MAX_BAUD_RATE] = { 110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 460800, 921600 };

bool		initialized_YUV_lookup_table = false;

std::deque<CSCMonitorInfo>	g_monitors;
std::deque<CString>			g_comment_mark = {
											_T("//"),	// single : C / C++ / C# / JAVA / Kotlin / Swift / Go / R / Javascript / Objective-C
											_T("/*"),	// block  : C / C++ / C# / JAVA / Kotlin / Swift / Go / R / Javascript / Objective-C
											_T("<!--"),	// html / xml
											_T("#"),	// single : Python / Ruby
											_T("..."),	// block  : Python / Ruby
										};

void*						g_wow64_preset;

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "version.lib")		//for VerQueryValue
#pragma comment(lib, "comsuppw.lib ")	//for _bstr_t

CString	get_date_str(CTime t, CString sep)
{
	CString str;

	if (GetUserDefaultUILanguage() == 1042)
	{
		str.Format(_T("%d%s%02d%s%02d"), t.GetYear(), sep, t.GetMonth(), sep, t.GetDay());
	}
	else
	{
		sep = _T("/");
		str.Format(_T("%d%s%d%s%d"), t.GetMonth(), sep, t.GetDay(), sep, t.GetYear());
	}

	return str;
}

CString get_date_str(COleDateTime t, CString sep)
{
	CString str;

	if (GetUserDefaultUILanguage() == 1042)
	{
		str.Format(_T("%d%s%02d%s%02d"), t.GetYear(), sep, t.GetMonth(), sep, t.GetDay());
	}
	else
	{
		sep = _T("/");
		str.Format(_T("%d%s%d%s%d"), t.GetMonth(), sep, t.GetDay(), sep, t.GetYear());
	}

	return str;
}

CString get_date_str(__timeb32 tb, CString sep)
{
	CTime	t(tb.time);
	return get_date_str(t, sep);
}

CString get_date_str(SYSTEMTIME st, CString sep)
{
	CTime	t(st);
	return get_date_str(t, sep);
}

CString get_time_str(CTime t, CString sep, bool h24, bool sec)
{
	return get_time_str(COleDateTime(t.GetTime()), sep, h24, sec);
}

CString get_time_str(COleDateTime t, CString sep, bool h24, bool sec)
{
	CString str;

	int h = t.GetHour();
	int m = t.GetMinute();
	int s = t.GetSecond();
	bool am = (h < 12);

	if (h24)
	{
		if (GetUserDefaultUILanguage() == 1042)
		{
			str.Format(_T("%02d%s%02d%s%02d"), h, sep, m, sep, s);
		}
		else
		{
			str.Format(_T("%02d%s%02d%s%02d"), h, sep, m, sep, s);
		}
	}
	else
	{
		if (GetUserDefaultUILanguage() == 1042)
		{
			str.Format(_T("%s %2d%s%02d%s%02d"), (am ? _T("오전") : _T("오후")), (h >= 13 ? h - 12 : h), sep, m, sep, s);
		}
		else
		{
			if (sec)
			{
				str.Format(_T("%2d%s%02d%s%02d %s"), (h >= 13 ? h - 12 : h), sep, m, sep, s, (am ? _T("AM") : _T("PM")));
			}
			else
			{
				str.Format(_T("%2d%s%02d %s"), (h >= 13 ? h - 12 : h), sep, m, (am ? _T("AM") : _T("PM")));
				return str;
			}
		}
	}

	//초 표시가 false일 경우는 위에서 2자리와 sep길이만큼 잘라준다.
	//단, !h24이고 한글윈도우인 경우에만 아래 처리를 해야 한다.
	if (!sec)
		str = str.Left(str.GetLength() - 2 - sep.GetLength());

	return str;
}

CString get_time_str(__timeb32 tb, CString sep, bool h24, bool sec)
{
	CTime	t(tb.time);
	return get_time_str(t, sep, h24, sec);
}

CString	get_time_str(SYSTEMTIME st, CString sep, bool h24, bool sec, bool msec)
{
	CString str;
	CTime t(st);
	str.Format(_T("%s%s%s"), get_time_str(t, sep, h24, sec), (msec ? _T(".") : _T("")), (msec ? i2S(st.wMilliseconds, false, true, 3) : _T("")));
	return str;
}

CString	get_datetime_str(CTime t, int type, bool sep, CString mid, bool h24, bool sec)
{
	return get_datetime_str(COleDateTime(t.GetTime()), type, sep, mid, h24, sec);
}

CString	get_datetime_str(COleDateTime t, int type, bool sep, CString mid, bool h24, bool sec)
{
	CString sDate = (sep ? get_date_str(t) : get_date_str(t, _T("")));
	CString sTime = (sep ? get_time_str(t, _T(":"), h24, sec) : get_time_str(t, _T(""), h24, sec));

	if (type == 0)
		return sDate;
	else if (type == 1)
		return sTime;

	return sDate + mid + sTime;
}

CString	get_datetime_str(__timeb32 tb, int type, bool sep, CString mid, bool h24, bool sec)
{
	CTime	t(tb.time);
	return get_datetime_str(t, type, sep, mid, h24, sec);
}

CString	get_datetime_str(SYSTEMTIME st, int type, bool sep, CString mid, bool h24, bool sec, bool msec)
{
	CString sDate = (sep ? get_date_str(st) : get_date_str(st, _T("")));
	CString sTime = (sep ? get_time_str(st, _T(":"), h24, sec, msec) : get_time_str(st, _T(""), h24, sec, msec));

	if (type == 0)
		return sDate;
	else if (type == 1)
		return sTime;

	return sDate + mid + sTime;
}


CString	get_cur_datetime_str(int type, bool sep, CString mid, bool h24, bool sec, bool msec)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	return get_datetime_str(st, type, sep, mid, h24, sec, msec);
}

#if 0
//type 0(date), 1(time), 2(date+time), 년-월-일 시:분:초 형식으로 현재 시간 리턴. mid_char는 날짜와 시간 사이 문자
CString GetCurrentDateTimeString(int type, bool separator /*= true*/, CString mid /*= _T(" ")*/, bool h24 /*= true*/)
{
	return GetDateTimeString(CTime::GetCurrentTime(), type, separator, mid, h24);
}

CString	GetTimeString(CTime t, bool bSeparator /*= true*/)
{
	CString str;

	str.Format(_T("%d-%02d-%02d %02d:%02d:%02d"), t.GetYear(), t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute(), t.GetSecond());

	if (bSeparator == false)
	{
		str.Remove('-');
		str.Remove(':');
	}

	return str;
}

CString GetDateTimeString(COleDateTime t, bool bSeparator /*= true*/)
{
	if (t.GetStatus() != COleDateTime::valid)
		return _T("");

	CString str;

	str.Format(_T("%d-%02d-%02d %02d:%02d:%02d"), t.GetYear(), t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute(), t.GetSecond());

	if (bSeparator == false)
	{
		str.Remove('-');
		str.Remove(':');
	}

	return str;
}

//bSeparator = true : 2003-04-16 18:01:00.123
//bSeparator = false: 20030416 180100.123
CString	GetDateTimeString(__timeb32 t, bool bSeparator /*= true*/, bool bUnderline /*= false*/, bool bHasMilliSec /*= true*/)
{
	int		i;
	CString	str;
	CTime	ct(t.time);

	if (bHasMilliSec)
	{
		str.Format(_T("%d-%02d-%02d%s%02d:%02d:%02d%s%03d"),
						ct.GetYear(), ct.GetMonth(), ct.GetDay(), (bUnderline ? _T("_") : _T("")),
						ct.GetHour(), ct.GetMinute(), ct.GetSecond(), (bUnderline ? _T("_") : _T("")), t.millitm);
	}
	else
	{
		str.Format(_T("%d-%02d-%02d%s%02d:%02d:%02d"),
			ct.GetYear(), ct.GetMonth(), ct.GetDay(), (bUnderline ? _T("_") : _T("")),
			ct.GetHour(), ct.GetMinute(), ct.GetSecond());
	}

	//2003-04-19 09:26:00.123 => 19 + 4 - 23 = 0
	//2003-04-19 09:26:00.12  => 19 + 4 - 23 = 1
	//2003-04-19 09:26:00.1   => 19 + 4 - 23 = 2
	// 0.123  0.12  0.1
	int		nEndZero = str.ReverseFind('.') + 4 - str.GetLength();
	//printf("str=%s, endzero = %d\n", str, nEndZero);

	for (i = 0; i < nEndZero; i++)
		str += "0";

	if (bSeparator == false)
	{
		str.Remove('-');
		str.Remove(':');
	}

	return str;
}

//bSeparator = true : 2003-04-16 18:01:00.123
//bSeparator = false: 20030416 180100.123
CString	GetCurrentTimeString(bool bSeparator, bool bHasMilliSec)
{
	__timeb32	tCur;

	_ftime32(&tCur);
	return GetTimeString(tCur, bSeparator, bUnderline, bHasMilliSec);
}

CString GetDateTimeStringFromTime(CTime t, bool bSeparator, bool h24, bool bHasMilliSec, bool include_seconds)
{
	CString str;

	str.Format(_T("%s %s"), GetDateStringFromTime(t), GetTimeStringFromTime(t, _T(":"), h24, include_seconds));

	if (!bSeparator)
	{
		str.Remove('-');
		str.Remove('/');
		str.Remove(':');
	}

	return str;
}

CString GetDateTimeStringFromTime(SYSTEMTIME t, bool bSeparator /*= true*/, bool h24 /*= true*/, bool include_seconds /*= true*/)
{
	CTime tTime(t);
	return GetDateTimeStringFromTime(tTime, bSeparator, h24, include_seconds);
}

#endif

//date = "20140403"
//time = "123456"
//CTime t = 
CTime get_CTime_from_datetime_str(CString date, CString time)
{
	if (date.IsEmpty())
		date = get_cur_datetime_str(0);

	date.Remove('-');
	date.Remove('/');
	date.Remove(' ');

	time.Remove(':');
	time.Remove(' ');

	ASSERT(time.GetLength() == 6);

	CTime t(_tstoi((TCHAR*)(LPCTSTR)(date.Left(4))), _tstoi((TCHAR*)(LPCTSTR)(date.Mid(4, 2))), _tstoi((TCHAR*)(LPCTSTR)(date.Right(2))),
			 _tstoi((TCHAR*)(LPCTSTR)(time.Left(2))), _tstoi((TCHAR*)(LPCTSTR)(time.Mid(2, 2))), _tstoi((TCHAR*)(LPCTSTR)(time.Right(2))));

	return t;
}

CTimeSpan GetTimeSpanFromTimeString(CString time)
{
	ASSERT(time.GetLength() == 6 || time.GetLength() == 8);
	time.Remove(':');
	return CTimeSpan(0, _ttoi(time.Left(2)), _ttoi(time.Mid(2,2)), _ttoi(time.Right(2)));
}

void GetTimeFromSeconds(int nTotalSeconds, int &nHours, int &nMinutes, int &nSeconds)
{
	nHours		= (nTotalSeconds / 60 / 60);
	nMinutes	= (nTotalSeconds / 60) % 60;
	nSeconds	= (nTotalSeconds % 60);
}

time_t _mkgmtime(const struct tm *tm) 
{
    // Month-to-day offset for non-leap-years.
    static const int month_day[12] =
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    // Most of the calculation is easy; leap years are the main difficulty.
    int month = tm->tm_mon % 12;
    int year = tm->tm_year + tm->tm_mon / 12;
    if (month < 0) {   // Negative values % 12 are still negative.
        month += 12;
        --year;
    }

    // This is the number of Februaries since 1900.
    const int year_for_leap = (month > 1) ? year + 1 : year;

    time_t rt = tm->tm_sec                             // Seconds
        + 60 * (tm->tm_min                          // Minute = 60 seconds
        + 60 * (tm->tm_hour                         // Hour = 60 minutes
        + 24 * (month_day[month] + tm->tm_mday - 1  // Day = 24 hours
        + 365 * (year - 70)                         // Year = 365 days
        + (year_for_leap - 69) / 4                  // Every 4 years is     leap...
        - (year_for_leap - 1) / 100                 // Except centuries...
        + (year_for_leap + 299) / 400)));           // Except 400s.
    return rt < 0 ? -1 : rt;
}

int	find(CString target, CString find_string, bool case_sensitive)
{
	if (!case_sensitive)
	{
		target.MakeLower();
		find_string.MakeLower();
	}
	
	return target.Find(find_string);
}

int find_dqstring(std::deque<CString> dqSrc, CString strFind, bool bWholeWord, bool bCaseSensitive)
{
	TCHAR op = '|';
	std::deque<CString> dqFind;

	if (strFind.Find(_T("&")) >= 0)
	{
		op = '&';
		get_token_string(strFind, dqFind, '&');
	}
	else if (strFind.Find(_T("|")) >= 0)
	{
		op = '|';
		get_token_string(strFind, dqFind, '|');
	}
	else
	{
		dqFind.push_back(strFind);
	}

	return find_dqstring(dqSrc, dqFind, op, bWholeWord, bCaseSensitive);
}

//dqSrc에 dqFind가 있는지 검사. 현재는 AND 연산이므로 dqFind의 모든 원소가 dqSrc에 포함되어 있어야 함.
int find_dqstring(std::deque<CString> dqSrc, std::deque<CString> dqFind, TCHAR op, bool bWholeWord, bool bCaseSensitive)
{
	int i, j;
	int found_index = -1;

	if (!bCaseSensitive)
	{
		for (i = 0; i < dqSrc.size(); i++)
			dqSrc[i].MakeLower();
		for (i = 0; i < dqFind.size(); i++)
			dqFind[i].MakeLower();
	}

	int found_count = 0;

	for (i = 0; i < dqSrc.size(); i++)
	{
		found_index = -1;
		found_count = 0;

		for (j = 0; j < dqFind.size(); j++)
		{
			if (bWholeWord)
			{
				if (dqFind[j] == dqSrc[i])
				{
					//만약 OR 연산이면 여기서 true 리턴
					found_count++;

					if (found_index < 0)
						found_index = i;

					if (op == '|')
						break;
				}
			}
			else
			{
				if (dqSrc[i].Find(dqFind[j]) >= 0)
				{
					//만약 OR 연산이면 여기서 true 리턴
					found_count++;

					if (found_index < 0)
						found_index = i;

					if (op == '|')
						break;
				}
			}
		}

		//AND이면 찾는 문자열이 모두 존재해야 하고
		if (op == '&')
		{
			if (found_count == dqFind.size())
				return found_index;
		}
		//OR이면 하나만 존재해도 true
		else
		{
			if (found_count > 0)
				return found_index;
		}

		found_index = -1;
	}

	return found_index;
}

//클립보드 clipboard
bool copy_to_clipboard(HWND hWnd, CString str)
{
	if (str.IsEmpty())
		return false;

	char	*pString = NULL;
	int len = WideCharToMultiByte(CP_ACP, 0, (CStringW)str, -1, NULL, 0, NULL, NULL);
	pString = (char*)new char[len];
	WideCharToMultiByte(CP_ACP, 0, (CStringW)str, -1, pString, len, NULL, NULL);

	// 저장할 문자열의 길이를 구한다. ('\0'까지 포함한 크기)
	int string_length = strlen(pString) + 1;

	// 클립보드로 문자열을 복사하기 위하여 메모리를 할당한다. 
	// 클립보드에는 핸들을 넣는 형식이라서 HeapAlloc 함수 사용이 블가능하다. 
	HANDLE h_data = ::GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, string_length);

	// 할당된 메모리에 문자열을 복사하기 위해서 사용 가능한 주소를 얻는다. 
	char *p_data = (char *)::GlobalLock(h_data);

	if (NULL != p_data)
	{
		// 할당된 메모리 영역에 삽입할 문자열을 복사한다. 
		memcpy(p_data, pString, string_length);

		// 문자열을 복사하기 위해서 Lock 했던 메모리를 해제한다.
		::GlobalUnlock(h_data);

		if (::OpenClipboard(hWnd))
		{
			::EmptyClipboard(); // 클립보드를 연다.
			::SetClipboardData(CF_TEXT, h_data);  // 클립보드에 저장된 기존 문자열을 삭제한다.
												  // 클립보드로 문자열을 복사한다.
			::CloseClipboard(); // 클립보드를 닫는다.
			delete[] pString;
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool Compare_By_Case_Sensitive(CString str1, CString str2, bool bCase)
{
	if (!bCase)
	{
		str1.MakeLower();
		str2.MakeLower();
	}
	
	return (str1 == str2);
}
#if 0
COLORREF get_color(COLORREF crOrigin, int nOffset)
{
	int r	= GetRValue(crOrigin) + nOffset;
	int g	= GetGValue(crOrigin) + nOffset;
	int b	= GetBValue(crOrigin) + nOffset;
	
	if (r < 0) r = 0; else if (r > 255) r = 255;
	if (g < 0) g = 0; else if (g > 255) g = 255;
	if (b < 0) b = 0; else if (b > 255) b = 255;
	
	return RGB(r, g, b);
}

COLORREF	GetDefaultColor(int idx)
{
	idx %= 10;

	switch (idx)
	{
		case 0 : return RGB(237, 125,  49);
		case 1 : return RGB(91, 155, 213);
		case 2 : return RGB(165, 255, 165);
		case 3 : return RGB(255, 192,   0);
		case 4 : return RGB(68, 114, 196);
		case 5 : return RGB(112, 173,  71);
		case 6 : return RGB(37,  94, 255);
		case 7 : return RGB(158,  72,  14);
		case 8 : return RGB(99,  99, 199);
		case 9 : return RGB(153, 115,   0);
	}
}

//random19937을 이용하여 랜덤 컬러를 리턴한다.
COLORREF	get_random_color()
{
	return RGB(random19937(0, 255), random19937(0, 255), random19937(0, 255));
}

COLORREF	GetComplementaryColor(COLORREF crColor, COLORREF crBack)
{
	int	r = abs(GetRValue(crBack) - GetRValue(crColor));
	int	g = abs(GetGValue(crBack) - GetGValue(crColor));
	int	b = abs(GetBValue(crBack) - GetBValue(crColor));

	Clamp(r, 0, 255);
	Clamp(g, 0, 255);
	Clamp(b, 0, 255);

	return RGB(r, g, b);
}
#endif

CString	get_exe_directory(bool includeSlash)
{
	TCHAR	sFilePath[1024];
	
	//GetModuleFileName(AfxGetInstanceHandle(), sFilePath, MAX_PATH);
	GetModuleFileName(NULL, sFilePath, MAX_PATH);
	
	CString exe_directory = get_part(sFilePath, fn_folder);
	if (includeSlash && exe_directory.Right(1) != _T("\\"))
		exe_directory += _T("\\");

	return exe_directory;
}

CString get_exe_parent_directory()
{
	CString sExePath = get_exe_directory();
	return sExePath.Left(sExePath.ReverseFind('\\'));
}

CString		get_exe_filename(bool fullpath /*= false*/)
{
	TCHAR	sFilePath[1024];
	CString sExeFile;
	
	//GetModuleFileName(AfxGetInstanceHandle(), sFilePath, MAX_PATH);
	GetModuleFileName(NULL, sFilePath, MAX_PATH);
	
	if (fullpath)
		sExeFile = sFilePath;
	else
		sExeFile = get_part(sFilePath, fn_name);
	
	return sExeFile;
}

CString		get_exe_file_title()
{
	return get_part(get_exe_filename(false), fn_title);
}

CString		GetCurrentDirectory()
{
	TCHAR	sFilePath[1024];
	CString sExeFolder;
	
	GetCurrentDirectory(MAX_PATH, sFilePath);
	
	return sFilePath;
}

//_tsplitpath("c:\\abc/def\\123.txt", ...)를 실행하면
//"c:", "\\abc/def\\", "123", ".txt" 과 같이 분리되는데 기존에 사용하던 기대값과 달라 보정한다.
//"c:\\", "c:\\abc/def", "123", "txt", "123.txt와 같이 보정한다.
//part : fn_drive(drive), fn_folder(drive+folder), fn_leaf_folder(folder name), fn_title(filetitle), fn_ext(ext), fn_name(filename)
//만약 path가 "d:\\aaa\\b.abc"이고 b.abc가 파일이 아닌 폴더라면 문제된다.
//remote의 파일이라면 PathIsFolder()함수로 검사할수도 없으므로
//path가 file이 아닌 폴더명이라고 하면 반드시 호출할때부터 맨 끝에 '\\'를 붙여서 호출해야 정확히 분리된다.
CString		get_part(CString path, int part)
{
	if (path.Right(2) == _T(":)"))
		return path;

	TCHAR tDrive[_MAX_DRIVE] = { 0, };
	TCHAR tDir[_MAX_DIR] = { 0, };
	TCHAR tFname[_MAX_FNAME] = { 0, };
	TCHAR tExt[_MAX_EXT] = { 0, };
	_tsplitpath(path, tDrive, tDir, tFname, tExt);

	CString parts[6] = { tDrive, tDir, _T(""), tFname, tExt, };

	//넘어온 path와 추출된 tDir이 동일하다면 path가 폴더이므로 별도 처리.
	if (path == parts[fn_folder])
	{

	}

	//확장자를 포함한 파일명
	parts[fn_name] = parts[fn_title] + parts[fn_ext];

	//폴더명의 끝에 '/' 또는 '\\'라면 제거.
	if (parts[fn_folder].Right(1) == '/' || parts[fn_folder].Right(1) == '\\')
		parts[fn_folder].Truncate(parts[fn_folder].GetLength() - 1);

	//폴더명은 드라이브 경로까지 모두 포함. "\\abc/def" => "c:\\abc/def"
	parts[fn_folder] = parts[fn_drive] + parts[fn_folder];
	parts[fn_leaf_folder] = parts[fn_folder].Mid(parts[fn_folder].ReverseFind('\\') + 1);

	//드라이브 루트라면 끝에 '\'를 붙여줘야 한다. "C:" => "C:\\"
	if (parts[fn_folder].GetLength() == 2 && parts[fn_folder].Right(1) == _T(":"))
	{
		parts[fn_folder] += _T("\\");
		//드라이브 루트인 경우라면 leaf_folder = folder와 동일하다.
		parts[fn_leaf_folder] = parts[fn_folder];
	}

	//확장자는 .을 제외시킨다. parts[3]가 ""이어도 .Mid(1)은 에러가 발생하지는 않는다.
	parts[fn_ext] = parts[fn_ext].Mid(1);

	//path가 b.abc와 같이 파일명처럼 되어 있지만 파일이 아닌 folder일 경우의 처리(실제 존재할 경우에만 가능)
	//이 처리는 취약하여 삭제하고 처음부터 path를 명확히 줘서 사용하도록 한다.
	//path가 file이 아닌 폴더명이라고 하면 호출할때부터 맨 끝에 '\\'를 붙여서 호출해야 한다.
	/*
	if (PathFileExists(path))
	{
		if (PathIsDirectory(path))
		{
			//해당 경로가 디렉토리이고 part == fn_name이라고 넘어왔다면
			//이는 실제로는 fn_leaf_folder를 요청하는 것으로 봐야 한다.
			//또는 호출하는 곳에서 미리 판별하여 요청할 수도 있다.
			//파일이면 fn_name을 요청하고 폴더이면 fn_leaf_folder를 요청해야 한다.
			part = fn_leaf_folder;
			parts[fn_leaf_folder] = parts[fn_name];
			parts[fn_folder] = parts[fn_folder] + _T("\\") + parts[fn_name];
			parts[fn_title] = parts[fn_ext] = parts[fn_name] = _T("");
		}
	}
	else
	*/
	{
		//"바탕 화면"과 같이 가상 경로일 경우는 fn_title과 fn_name만 이 값으로 채워지므로
		//폴더이름에도 이 값을 넣어 사용한다.
		if (parts[fn_drive].IsEmpty() && parts[fn_folder].IsEmpty() && !parts[fn_name].IsEmpty())
			parts[fn_folder] = parts[fn_leaf_folder] = parts[fn_name];
	}

	return parts[part];
}

#if 0
CString		GetFileNameFromFullPath(CString fullpath)
{
	fullpath.Replace(_T("/"), _T("\\"));

	if (fullpath.Find(_T("\\")) > 0)
		return fullpath.Right(fullpath.GetLength() - fullpath.ReverseFind('\\') - 1);
	
	return fullpath;
}

CString		GetFolderNameFromFullPath(CString sFullPath, bool includeSlash)
{
	PathStripPath((LPTSTR)(LPCTSTR)sFullPath);
	return sFullPath;

	TCHAR sep = '/';

	sFullPath.Replace(_T("/"), _T("\\"));

	//sFullPath가 폴더라면 그냥 그 값을 리턴한다.
	if (PathIsDirectory(sFullPath))
		return sFullPath;

	if (sFullPath.Find(_T("\\")) > 0)
	{
		sFullPath = sFullPath.Left(sFullPath.ReverseFind('\\') + (includeSlash ? 1 : 0));

		if (sFullPath.GetLength() == 2)	//ex. C:
			sFullPath += _T("\\");

		return sFullPath;
	}

	return _T("");
}

CString		GetFileTitle(CString sFullPath)
{
	CString	filename = GetFileNameFromFullPath(sFullPath);
	
	//	AfxMessageBox(filename);
	// 확장명이 있는 경우와 없는 경우를 구분한다.
	if (filename.Find('.') > 0)
		return filename.Left(filename.ReverseFind('.'));
	else
		return filename;
}

CString	GetFileExtension(CString sFullPath, bool dot)
{
	CString sString = GetFileNameFromFullPath(sFullPath);

	// 확장명이 있는 경우와 없는 경우를 구분한다.
	if (sString.Find('.') > 0)
		return sString.Mid(sString.ReverseFind('.') + (dot ? 0 : 1));
	else
		return _T("");
}
#endif

int	GetFileTypeFromFilename(CString filename)
{
	return GetFileTypeFromExtension(get_part(filename, fn_ext));
}

int	GetFileTypeFromExtension(CString sExt)
{
	sExt.MakeLower();

	if (is_exist_keyword(sExt, FILE_EXTENSION_VIDEO))
		return FILE_TYPE_VIDEO;
	else if (is_exist_keyword(sExt, _T("bin")))
		return FILE_TYPE_VIDEO_BIN;
	else if (is_exist_keyword(sExt, FILE_EXTENSION_IMAGE))
		return FILE_TYPE_IMAGE;
	else if (is_exist_keyword(sExt, FILE_EXTENSION_SOUND))
		return FILE_TYPE_SOUND;
	else if (is_exist_keyword(sExt, FILE_EXTENSION_SUBTITLE))
		return FILE_TYPE_SUBTITLE;
	else
		return FILE_TYPE_UNKNOWN;
}

//파일명에서 확장자를 newExt로 변경한다.
//applyRealFile이 true이면 실제 파일명도 변경시킨다.
bool change_extension(CString& filepath, CString newExt, bool applyRealFile)
{
	CString sOldExt = get_part(filepath, fn_ext);
	CString sNewFullPath = filepath.Left(filepath.GetLength() - sOldExt.GetLength()) + newExt;

	bool changeSuccess = false;

	if (applyRealFile)
	{
		//실제 파일 확장자를 변경했을때만 filepath도 변경시켜준다.
		changeSuccess = MoveFile(filepath, sNewFullPath);
		if (changeSuccess)
			filepath = sNewFullPath;
		return changeSuccess;
	}

	filepath = sNewFullPath;
	return true;
}

//경로상에 "\\.." 또는 "/.."이 들어있으면 실제 경로로 재구성해준다.
//c:/abc/def/../../test.txt => c://test.txt
CString	normalize_path(CString& filepath)
{
	DWORD  retval = 0;
	TCHAR buffer[MAX_PATH] = _T("");

	retval = GetFullPathName(filepath, MAX_PATH, buffer, NULL);

	if (retval == 0)
	{
		TRACE(_T("GetFullPathName() error"));
		return filepath;
	}

	filepath = buffer;

	return filepath;
}

//"C:\\", "C:\\Temp"와 같이 루트일때와 일반 폴더일 경우 끝에 역슬래시 유무가 다르므로 필요.
bool is_drive_root(CString path)
{
	if (path.GetLength() == 3 && path.Right(1) == '\\')
		return true;

	return false;
}

//src 폴더 경로에 sub 폴더 경로를 붙여주는 단순한 함수지만 드라이브 루트일때와 아닐때 등의 처리때문에 검사하여 결합해주는 목적으로 추가.
CString	concat_path(CString src, CString sub, TCHAR path_sep)
{
	if (src.IsEmpty())
		return sub;

	if (sub.IsEmpty())
		return src;

	if (is_drive_root(src) || (src.Right(1) == path_sep))
		return src + sub;

	return src + path_sep + sub;
}

//새 폴더, 새 폴더 (2)와 같이 폴더내에 새 항목을 만들 때 사용 가능한 인덱스를 리턴한다.
//zero_prefix가 2이면 001, 002로 된 인덱스가 붙은 파일/폴더들만 대상으로 하려 했으나 아직 미구현.
int	get_file_index(CString folder, CString title, int zero_prefix)
{
	std::deque<CString> dq;
	find_all_files(folder, &dq, title + _T("*"), true, false);

	int max_index = -1;
	std::set<int> idx_set;

	if (is_drive_root(folder))
		truncate(folder, 1);

	for (auto item : dq)
	{
		item.Replace(folder + _T("\\"), _T(""));
		if (item.MakeLower() == title.MakeLower())
			idx_set.insert(1);

		//끝 ')'를 찾고
		int start_paren = -1;
		int end_paren = item.ReverseFind(')');
		int found_index = -1;

		if (end_paren > 0)
		{
			//시작 '('를 찾아서 그 사이의 숫자를 추출
			item = item.Left(end_paren);
			start_paren = item.ReverseFind('(');

			if (start_paren > 0)
			{
				item = item.Mid(start_paren + 1);
				found_index = _ttoi(item);
			}
		}

		if (found_index > 0)
			idx_set.insert(found_index);
	}

	//set 항목 중 비어있는 인덱스를 리턴해준다.
	int index = 0;
	bool found = false;

	for (int elem : idx_set)
	{
		index++;
		if (elem != index)
		{
			found = true;
			break;
		}
	}

	//만약 1 ~ n까지 모든 순번이 순차적으로 들어있다면 1 증가된 값을 리턴해주면 된다.
	if (!found)
		index++;

	return index;
}

//확장자 집합 문자열로 파일열기 대화상자의 filter string을 리턴한다.
//simple : "bmp;jpg;jpeg;png;webp;gif;yuv;raw => "JPG files|*.jpg|bmp|*.bmp|
//extension_group = FILE_EXTENSION_VIDEO or FILE_EXTENSION_SOUND or FILE_EXTENSION_IMAGE or FILE_EXTENSION_MEDIA...
//현재 미완성!
CString	get_filter_string(CString extension_group, bool simple)
{
	CString filter;
	std::deque<CString> token;
	get_token_string(extension_group, token, _T(";"), false);

	return filter;
}

//폴더에 있는 파일들 중 filetitle이고 extension에 해당하는 파일명을 리턴한다.
std::deque<CString>	get_filelist_from_filetitle(CString folder, CString filetitle, CString extension)
{
	return FindFilesWithExtensions(folder, filetitle, extension);
}

std::deque<CString> get_filelist_from_filetitle(CString filename, CString extension)
{
	return get_filelist_from_filetitle(get_part(filename, fn_folder), get_part(filename, fn_title), extension);
}

ULONGLONG get_file_size(CString sfile)
{
	CFileStatus		status;
	
	//디스크 드라이브인 경우
	if (sfile.Mid(1) == _T(":") || sfile.Mid(2) == _T(":\\"))
	{
		return (get_disk_total_size(sfile) - get_disk_free_size(sfile));
	}

	if(CFile::GetStatus(sfile, status))
	{
		return status.m_size;
	}
	
	return 0;
}

bool get_file_size(CString path, ULARGE_INTEGER* ulFileSize)
{
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	ulFileSize->LowPart = ::GetFileSize(hFile, &(ulFileSize->HighPart));

	CloseHandle(hFile);

	return true;
}

ULONGLONG get_file_size(WIN32_FIND_DATA data)
{
	ULARGE_INTEGER ul;
	ul.HighPart = data.nFileSizeHigh;
	ul.LowPart = data.nFileSizeLow;
	return ul.QuadPart;
}

ULONGLONG get_folder_size(CString path)
{
	WIN32_FIND_DATA data;
	ULONGLONG size = 0;
	CString fname = path + "\\*.*";
	HANDLE h = FindFirstFile(fname, &data);
	if (h != INVALID_HANDLE_VALUE)
	{
		do {
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				// make sure we skip "." and "..".  Have to use strcmp here because
				// some file names can start with a dot, so just testing for the 
				// first dot is not suffient.
				if (_tcscmp(data.cFileName, _T(".")) != 0 && _tcscmp(data.cFileName, _T("..")) != 0)
				{
					// We found a sub-directory, so get the files in it too
					fname = path + "\\" + data.cFileName;
					// recurrsion here!
					size += get_folder_size(fname);
				}

			}
			else
			{
				LARGE_INTEGER sz;
				// All we want here is the file size.  Since file sizes can be larger
				// than 2 gig, the size is reported as two DWORD objects.  Below we
				// combine them to make one 64-bit integer.
				sz.LowPart = data.nFileSizeLow;
				sz.HighPart = data.nFileSizeHigh;
				size += sz.QuadPart;

			}
		} while (FindNextFile(h, &data) != 0);
		FindClose(h);
	}

	return size;
}

//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
//unit_string	: 단위를 표시할 지 (default = true)
CString		get_file_size_str(CString sfile, int unit, int floats, bool unit_string)
{
	return get_size_str(get_file_size(sfile), unit, floats, unit_string);
}

CString		get_file_size_str(WIN32_FIND_DATA data, int unit, int floats, bool unit_string)
{
	ULARGE_INTEGER filesize;
	filesize.HighPart = data.nFileSizeHigh;
	filesize.LowPart = data.nFileSizeLow;
	return get_size_str(filesize.QuadPart, unit, floats, unit_string);
}

//unit			: 0:bytes, 1:KB, 2:MB, 3:GB ~
//auto일 경우는 1000보다 작을떄까지 나누고 소수점은 2자리까지 표시한다.(ex 7.28TB 32.1TB 123.5TB)
//floats		: 소수점을 몇 자리까지 표시할지
//unit_string	: 단위를 표시할 지
//comma			: 정수 부분에 자리수 콤마를 표시할 지
CString		get_size_str(ULONGLONG size, int unit, int floats, bool unit_string, bool comma)
{
	double dsize = (double)size;
	CString size_str;
	CString unit_str[9] = { _T(" Bytes"), _T("KB"), _T("MB"), _T("GB"), _T("TB"), _T("PB"), _T("EB"), _T("ZB"), _T("YB") };

	if (unit >= 0)
	{
		for (int i = 0; i < unit; i++)
			dsize /= 1024.0;
	}
	else
	{
		unit = 0;

		//1024가 아니라 1000인 점에 주의!
		while (dsize >= 1000.0)
		{
			dsize /= 1024.0;
			unit++;
		}

		//정수가 3자리면 소숫점 자리는 0,
		//정수가 2자리면 소숫점 자리는 1,
		//정수가 1자리면 소숫점 자리는 2,
		floats = 3 - d2S(dsize, false, 0).GetLength();

		//단, bytes 단위라면 소숫점 자리수는 0이다.
		if (unit == 0)
			floats = 0;
		TRACE(_T("floats = %d\n"), floats);
		//if (dsize < 10)
		//	floats = 2;
		//else
		//	floats = 1;
	}

	//dsize = 0.01234;
	//floats = 2;
	double multiply = pow(10.0, floats);
	dsize = ceil(dsize * multiply) / multiply;// ROUND(dsize, floats);
	//0.045KB일 때 floats가 0이면 0KB으로 리턴될 것이다.
	//이 경우라면 1KB로 리턴하는 것이 맞다.
	//실제 파일의 크기가 0byte일때만 0을 리턴하자.
	//0.045KB일 때 floats가 1이면 역시 0.0KB로 리턴될 것이다.
	//이 경우라면 0.1KB로 리턴되는게 맞다.
	//floats가 1이면 0.1이고 이 0.1보다 dsize가 작다면 그냥 0.1라고 해주자.
	double dmin = 1.0 / pow(10.0, floats);
	if (dsize > 0.0 && dsize < dmin)
		dsize = dmin;

	//dsize = ROUND(dsize, floats + 1);
	size_str = d2S(dsize, comma, floats);
	if (unit_string)
		size_str += unit_str[unit];

	return size_str;
}


//src를 파싱해서 특정 길이 이상의 문자열들로 나눈다.
std::deque<CString> parse_divide(CString src, int len)
{
	std::deque<CString> result;

	while (src.GetLength() >= len)
	{
		int punc_pos = find_punctuation(src, len);
		if (punc_pos <= 0)
		{
			break;
		}
		else
		{
			result.push_back(src.Left(punc_pos));
			src = src.Mid(punc_pos);
		}
	}

	if (src.GetLength())
		result.push_back(src);

	return result;
}

//src의 pos근처에서 구두점을 찾아 그 위치를 리턴한다.(구두점으로 문장을 나눌때 이용)
int	find_punctuation(CString src, int pos)
{
	int idx = 0;
	while (!is_punctuation(src[pos + idx]))
	{
		if (idx >= 0)
		{
			idx++;
			idx *= -1;
			if (pos + idx <= 1)
				return -1;
		}
		else
		{
			idx--;
			idx *= -1;
			if (pos + idx >= src.GetLength() - 1)
				return -1;
		}

		if (abs(idx) >= 20)
			return -1;
	}

	return (pos + idx);
}


bool is_punctuation(TCHAR ch)
{
	const wchar_t start_ch = L'가';
	const wchar_t end_ch = L'힣';

	if ((ch >= '0' && ch <= '9') ||
		(ch >= 'a' && ch <= 'z') ||
		(ch >= 'A' && ch <= 'Z') ||
		(ch <= 0))
		return false;

	return true;
}

CTime		GetFileCreationTime(CString sfile)
{
	CFileStatus		status;
	
	if(CFile::GetStatus(sfile, status))
		return status.m_ctime;
	
	return NULL;
}

CTime		GetFileLastModifiedTime(CString sfile)
{
	CFileStatus		status;
	
	if(CFile::GetStatus(sfile, status))
		return status.m_mtime;
	
	return NULL;
}

CTime		GetFileLastAccessTime(CString sfile)
{
	CFileStatus		status;
	
	if(CFile::GetStatus(sfile, status))
		return status.m_atime;
	
	return NULL;
}

bool CheckFileIsURL(CString sURL)
{
	if (sURL.Find(_T("://")) > 0)
		return TRUE;
	else
		return FALSE;
}

//check_prefix가 true이면 http, https까지 체크한다. 뭔가 취약점이 있는듯하여 우선 사용금지.(https://mathiasbynens.be/demo/url-regex)
bool is_valid_url(CString url, bool check_prefix)
{
	std::string pattern;
	
	if (check_prefix)
		pattern = "https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{2,256}\.[a-z]{2,4}\b([-a-zA-Z0-9@:%_\+.~#?&//=]*)";
	else //확인 필요!!
		pattern = "[-a-zA-Z0-9@:%._\+~#=]{2,256}\.[a-z]{2,4}\b([-a-zA-Z0-9@:%_\+.~#?&//=]*)";

	std::regex url_regex(pattern);
	std::string stdurl = std::string(CT2CA(url));

	return std::regex_match(stdurl, url_regex);
}

bool IsFolder(CString sfile)
{
	if (PathIsRoot(sfile))
		return true;

	WIN32_FIND_DATA info; 
	HANDLE h_dir_info = ::FindFirstFile(sfile,&info); 
	bool bIsFolder = false;

	if (h_dir_info != INVALID_HANDLE_VALUE)
	{ 
		if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			bIsFolder = true; 
		else
			bIsFolder = false; 
	}

	::FindClose(h_dir_info); 

	return bIsFolder;
}

#include <sys/types.h>
#include <sys/stat.h>

bool isFolder(char *sfile)
{
	//struct stat stats;
	//stat(sfile, &stats);
	//if (S_ISDIR(stats.st_mode))
	//	return true;
	return false;
}

bool IsFileFolderPathIsEqual(CString file0, CString file1, bool bCaseSensitive)
{
	if (bCaseSensitive)
	{
		file0.MakeLower();
		file1.MakeLower();
	}

	file0.Replace(_T('\\'), _T('/'));
	file1.Replace (_T('\\'), _T('/'));
	return (file0 == file1);
}

void GetURLFileInfo(CString sURL, bool &bInURL, bool &bFileType)
{
	bInURL		= FALSE;
	bFileType	= FALSE;
	
	if (sURL.Find(_T("://")) > 0)
	{
		bInURL = TRUE;
		
		CString filename = get_part(sURL, fn_name);
		if (filename == "")
			bFileType = FALSE;
		else
			bFileType = TRUE;
	}
}

int	get_char_count(CString sStr, TCHAR ch, bool stop_at_first_mismatch, bool forward)
{
	int	count = 0;
	
	if (forward)
	{
		for (int i = 0; i < sStr.GetLength(); i++)
		{
			if (sStr.GetAt(i) == ch)
				count++;
			else if (stop_at_first_mismatch)
				break;
		}

		return count;
	}
	else
	{
		for (int i = sStr.GetLength() - 1; i >= 0; i--)
		{
			if (sStr.GetAt(i) == ch)
				count++;
			else if (stop_at_first_mismatch)
				break;
		}

		return count;
	}
}

CString	get_mac_address_format(CString src, TCHAR separator)
{
	ASSERT(src.GetLength() == 12);
	ASSERT(get_char_count(src, separator) == 0);

	if (src.GetLength() != 12)
		return src;

	//2 5 8 11 14
	for (int i = 0; i < 5; i++)
		src.Insert(i * 3 + 2, CString(separator));

	return src;
}

int	Find_Divide_Position_By_Punctuation(CString str)
{
	if (str == "")
		return 0;
	
	int		nIndex = 0;
	int		nCenter = str.GetLength() / 2;
	TCHAR	ch;
	
	while (TRUE)
	{
		ch = str.GetAt(nCenter + nIndex);
		if (ch == ' ' || ch == '.' || ch == ',' || ch == ')' || ch == ';')
		{
			return (nCenter + nIndex);
		}
		else if (ch == '/' || ch == '(')
		{
			return (nCenter + nIndex - 1);
		}
		else
		{
			if (nIndex < 0)
				nIndex *= -1;
			else
			{
				nIndex++;
				nIndex *= -1;
			}
		}
		
		if (nCenter+nIndex <= 0 || abs(nIndex) > nCenter/2)
		{
			// DoubleByteCharacterSet이면...
			if (IsDBCSLeadByte((BYTE)(str.GetAt(nCenter))))
				return nCenter-1;
			else
				return nCenter;
		}
	}
}

//주어진 array에서 원하는 항목을 찾는다.
int FindStringFromArray(CStringArray& ar, CString sTarget, bool bCaseSensitive, bool bWholeWord)
{
	int		i;
	CString sArray;

	if (bCaseSensitive)
		sTarget.MakeLower();

	for (i = 0; i < ar.GetCount(); i++)
	{
		sArray = ar.GetAt(i);

		if (bCaseSensitive)
			sArray.MakeLower();

		if (bWholeWord)
		{
			if (sArray == sTarget)
			{
				return i;
			}
		}
		else if (sArray.Find(sTarget) >= 0)
		{
			return i;
		}
	}

	return -1;
}

int	FindStringFromDeque(std::deque<CString> dq, CString sTarget, bool bCaseSensitive, bool bWholeWord)
{
	int		i;
	CString sArray;

	if (bCaseSensitive)
		sTarget.MakeLower();

	for (i = 0; i < dq.size(); i++)
	{
		sArray = dq[i];

		if (bCaseSensitive)
			sArray.MakeLower();

		if (bWholeWord)
		{
			if (sArray == sTarget)
			{
				return i;
			}
		}
		else if (sArray.Find(sTarget) >= 0)
		{
			return i;
		}
	}

	return -1;
}


//sTime = "2018-01-29 12:34:56" => "20180129 123456"
CTime GetTimeFromDateTimeString(CString sDateTime)
{
	sDateTime.TrimLeft();
	sDateTime.TrimRight();
	sDateTime.Remove(_T('-'));
	sDateTime.Remove(_T(':'));

	ASSERT(sDateTime.GetLength() == 15);
	ASSERT(sDateTime.GetAt(8) == _T(' '));
	
	// 2007-07-22 09-10-24
	return  CTime(	_tstoi((TCHAR*)(LPCTSTR)sDateTime.Left(4)),
							_tstoi((TCHAR*)(LPCTSTR)sDateTime.Mid(4, 2)),
							_tstoi((TCHAR*)(LPCTSTR)sDateTime.Mid(6, 2)),
							_tstoi((TCHAR*)(LPCTSTR)sDateTime.Mid(9, 2)),
							_tstoi((TCHAR*)(LPCTSTR)sDateTime.Mid(11, 2)),
							_tstoi((TCHAR*)(LPCTSTR)sDateTime.Mid(13, 2)));
}

CString	get_time_str(double dSec, bool bHasHour /*= true*/, bool bMilliSec /*=false*/)
{
	bool minus = false;

	if (dSec < 0.0)
	{
		dSec *= -1.0;
		minus = true;
	}

	double	dMilli	= (dSec - (int)dSec) * 1000.0;
	int		nSec	= (int)dSec;
	int		nHour	= (nSec / 60 / 60);
	int		nMinute	= (nSec / 60) % 60;
	int		nSecond	= (nSec % 60);
	
	CString	str;
	CString sMilli;

	//sMilli.Format("%03d", (int)(dMilli));
	sMilli.Format(_T("%03d"), (int)(ROUND(dMilli,0)));
	
	if (bHasHour)
		str.Format(_T("%02d:%02d:%02d"), nHour, nMinute, nSecond);
	else
		str.Format(_T("%02d:%02d"), nMinute, nSecond);

	if (bMilliSec)
		str = str + "." + sMilli;
	
	if (minus)
		str = '-' + str;
	return str;
}

//185초일 경우 "3m 5s"를 리턴한다.
CString	get_remain_time_str(int seconds, bool include_sec)
{
	int		hour = (seconds / 60 / 60);
	int		minute = (seconds / 60) % 60;
	int		second = (seconds % 60);
	CString	str;

	if (include_sec)
		str.Format(_T("%dh %dm %ds"), hour, minute, second);
	else
		str.Format(_T("%dh %dm"), hour, minute);

	return str;
}

//원래 double dSec를 사용하여 위의 함수를 사용했으나 소수점 표시 방식의 오류가 존재하므로
//1.0이어야 할 값이 0.999999999998 과 같이 표현되어 위의 함수를 사용할 수 없을 경우가 있다.
//그래서 아예 millisecond값도 int로 간주하여 처리한 아래 함수를 사용한다.
CString	GetTimeStringFromMilliSeconds(int ms, bool bHasHour, bool bHasMilliSec)
{
	bool minus = false;

	if (ms < 0)
	{
		ms *= -1;
		minus = true;
	}

	int		nMilli	= ms % 1000;
	int		nSec	= ms / 1000;
	int		nHour	= (nSec / 60 / 60);
	int		nMinute	= (nSec / 60) % 60;
	int		nSecond	= (nSec % 60);
	
	CString	str;
	CString sMilli;
	CString sTimeString;

	sMilli.Format(_T("%03d"), nMilli);
	
	if (bHasHour)
		str.Format(_T("%02d:%02d:%02d"), nHour, nMinute, nSecond);
	else
		str.Format(_T("%02d:%02d"), nMinute, nSecond);

	if (bHasMilliSec)
		sTimeString.Format(_T("%s.%s"), str, sMilli);
	else
		sTimeString.Format(_T("%s"), str);
	
	if (minus)
		sTimeString = _T("-") + sTimeString;
	return sTimeString;
}

int	GetSecondsFromTimeString(CString timeString)
{
	bool minus = false;

	if (timeString[0] == '-')
	{
		minus = true;
		timeString = timeString.Mid(1);
	}

	std::deque<CString> dqToken;
	get_token_string(timeString, dqToken, ':');
	int hour = _ttoi(dqToken[0]);
	int minute = _ttoi(dqToken[1]);
	int second = _ttoi(dqToken[2]);

	int seconds = (hour * 3600 + minute * 60 + second);
	if (minus)
		seconds *= -1;
	return seconds;
}

//"00:01:23.456" => 83456
//srt자막의 경우는 .대신 ,를 사용하므로 둘 다 고려해준다.
int	GetMilliSecondsFromTimeString(CString timeString)
{
	timeString.Trim();

	int time = 0;
	int dotPos = -1;
	bool minus = false;

	if (timeString[0] == '-')
	{
		minus = true;
		timeString = timeString.Mid(1);
	}

	CString sTime = timeString;

	if (timeString.Find('.') > 0)
		dotPos = timeString.ReverseFind('.');
	else if (timeString.Find(',') > 0)
		dotPos = timeString.ReverseFind(',');

	if (dotPos >= 0)
	{
		time = _ttoi(timeString.Mid(dotPos + 1));
		sTime = timeString.Left(dotPos);
	}

	time += (GetSecondsFromTimeString(sTime) * 1000);

	if (minus)
		time *= -1;

	return time;
}

bool IsHexaColorString(CString str)
{
	bool result = true;

	for (int i = 0; i < str.GetLength(); i++)
	{
		if (IsHexaChar(str[i]) == false)
			return false;
	}

	return true;
}

#if 0
// 기본적으로 16진수 스트링이 넘어온다고 가정...
COLORREF get_color_from_hexa_string(CString str)
{
	if (IsHexaColorString(str) == false || str.GetLength() != 6)
		return deeppink;
	
	int r = _tcstol(str.Left(2), NULL, 16);
	int g = _tcstol(str.Mid(2, 2), NULL, 16);
	int b = _tcstol(str.Right(2), NULL, 16);
	
	return RGB(r, g, b);
}

COLORREF get_color(CString sColor)
{
	if (sColor == "")
		return RGB(255,255,255);
	
	sColor.MakeLower();
	sColor.Replace(_T(" "), _T(""));
	sColor.TrimLeft();
	sColor.TrimRight();
	
	if (IsHexaColorString(sColor))
	{
		return get_color_from_hexa_string(sColor);
	}

	sColor.MakeLower();

	if		(sColor == "black")				return RGB(0x00,0x00,0x00);
	else if (sColor ==	"snow")				return RGB(0xff,0xfa,0xfa);
	else if (sColor ==	"ghostwhite")			return RGB(0xf8,0xf8,0xff);
	else if (sColor ==	"whitesmoke")			return RGB(0xf5,0xf5,0xf5);
	else if (sColor ==	"floralwhite")			return RGB(0xff,0xfa,0xf0);
	else if (sColor ==	"papayawhip")			return RGB(0xff,0xef,0xd5);
	else if (sColor ==	"moccasin")			return RGB(0xff,0xe4,0xb5);
	else if (sColor ==	"cornsilk")			return RGB(0xff,0xf8,0xdc);
	else if (sColor ==	"ivory")				return RGB(0xff,0xff,0xf0);
	else if (sColor ==	"bisque")				return RGB(0xFF,0xE4,0xC4);
	else if (sColor ==	"lemonchiffon")		return RGB(0xff,0xfa,0xcd);
	else if (sColor ==	"seashell")			return RGB(0xff,0xf5,0xee);
	else if (sColor ==	"honeydew")			return RGB(0xf0,0xff,0xf0);
	else if (sColor ==	"mintcream")			return RGB(0xf5,0xff,0xfa);
	else if (sColor ==	"azure")				return RGB(0xf0,0xff,0xff);
	else if (sColor ==	"aliceblue")			return RGB(0xf0,0xf8,0xff);
	else if (sColor ==	"lavender")			return RGB(0xe6,0xe6,0xfa);
	else if (sColor ==	"lavenderblush")		return RGB(0xff,0xf0,0xf5);
	else if (sColor ==	"mistyrose")			return RGB(0xff,0xe4,0xe1);
	else if (sColor ==	"white")				return RGB(0xff,0xff,0xff);
	else if (sColor ==	"black")				return RGB(0x00,0x00,0x00);
	else if (sColor ==	"darkslategrey")		return RGB(0x2f,0x4f,0x4f);
	else if (sColor ==	"dimgrey")				return RGB(0x69,0x69,0x69);
	else if (sColor ==	"slategrey")			return RGB(0x70,0x80,0x90);
	else if (sColor ==	"lightslategrey")		return RGB(0x77,0x88,0x99);
	else if (sColor ==	"grey")				return RGB(0xbe,0xbe,0xbe);
	else if (sColor ==	"lightgrey")			return RGB(0xd3,0xd3,0xd3);
	else if (sColor ==	"midnightblue")		return RGB(0x19,0x19,0x70);
	else if (sColor ==	"navyblue")			return RGB(0x00,0x00,0x80);
	else if (sColor ==	"navy")				return RGB(0x00,0x00,0x80);
	else if (sColor ==	"lightnavy")			return RGB(0x80,0x80,0xff);
	else if (sColor ==	"cornflowerblue")		return RGB(0x64,0x95,0xed);
	else if (sColor ==	"darkslateblue")		return RGB(0x48,0x3d,0x8b);
	else if (sColor ==	"slateblue")			return RGB(0x6a,0x5a,0xcd);
	else if (sColor ==	"mediumslateblue")		return RGB(0x7b,0x68,0xee);
	else if (sColor ==	"lightslateblue")		return RGB(0x84,0x70,0xff);
	else if (sColor ==	"mediumblue")			return RGB(0x00,0x00,0xcd);
	else if (sColor ==	"royalblue")			return RGB(0x41,0x69,0xe1);
	else if (sColor ==	"blue")				return RGB(0x00,0x00,0xff);
	else if (sColor ==	"dodgerblue")			return RGB(0x1e,0x90,0xff);
	else if (sColor ==	"deepskyblue")			return RGB(0x00,0xbf,0xff);
	else if (sColor ==	"skyblue")				return RGB(0x87,0xce,0xeb);
	else if (sColor ==	"lightskyblue")		return RGB(0x87,0xce,0xfa);
	else if (sColor ==	"steelblue")			return RGB(0x46,0x82,0xb4);
	else if (sColor ==	"lightsteelblue")		return RGB(0xb0,0xc4,0xde);
	else if (sColor ==	"lightblue")			return RGB(0xad,0xd8,0xe6);
	else if (sColor ==	"powderblue")			return RGB(0xb0,0xe0,0xe6);
	else if (sColor ==	"paleturquoise")		return RGB(0xaf,0xee,0xee);
	else if (sColor ==	"darkturquoise")		return RGB(0x00,0xce,0xd1);
	else if (sColor ==	"mediumturquoise")		return RGB(0x48,0xd1,0xcc);
	else if (sColor ==	"turquoise")			return RGB(0x40,0xe0,0xd0);
	else if (sColor ==	"cyan")				return RGB(0x00,0xff,0xff);
	else if (sColor ==	"lightcyan")			return RGB(0xe0,0xff,0xff);
	else if (sColor ==	"cadetblue")			return RGB(0x5f,0x9e,0xa0);
	else if (sColor ==	"mediumaquamarine")	return RGB(0x66,0xcd,0xaa);
	else if (sColor ==	"aquamarine")			return RGB(0x7f,0xff,0xd4);
	else if (sColor ==	"darkgreen")			return RGB(0x00,0x64,0x00);
	else if (sColor ==	"darkolivegreen")		return RGB(0x55,0x6b,0x2f);
	else if (sColor ==	"darkseagreen")		return RGB(0x8f,0xbc,0x8f);
	else if (sColor ==	"seagreen")			return RGB(0x2e,0x8b,0x57);
	else if (sColor ==	"mediumseagreen")		return RGB(0x3c,0xb3,0x71);
	else if (sColor ==	"lightgreen")			return RGB(0x20,0xb2,0xaa);
	else if (sColor ==	"palegreen")			return RGB(0x98,0xfb,0x98);
	else if (sColor ==	"springgreen")			return RGB(0x00,0xff,0x7f);
	else if (sColor ==	"lawngreen")			return RGB(0x7c,0xfc,0x00);
	else if (sColor ==	"green")				return RGB(0x00,0xff,0x00);
	else if (sColor ==	"chartreuse")			return RGB(0x7f,0xff,0x00);
	else if (sColor ==	"mediumspringgreen")	return RGB(0x00,0xfa,0x9a);
	else if (sColor ==	"greenyellow")			return RGB(0xad,0xff,0x2f);
	else if (sColor ==	"limegreen")			return RGB(0x32,0xcd,0x32);
	else if (sColor ==	"yellowgreen")			return RGB(0x9a,0xcd,0x32);
	else if (sColor ==	"forestgreen")			return RGB(0x22,0x8b,0x22);
	else if (sColor ==	"olivedrab")			return RGB(0x6b,0x8e,0x23);
	else if (sColor ==	"darkkhaki")			return RGB(0xbd,0xb7,0x6b);
	else if (sColor ==	"khaki")				return RGB(0xf0,0xe6,0x8c);
	else if (sColor ==	"palegoldenrod")		return RGB(0xee,0xe8,0xaa);
	else if (sColor ==	"lightyellow")			return RGB(0xff,0xff,0xe0);
	else if (sColor ==	"yellow")				return RGB(0xff,0xff,0x00);
	else if (sColor ==	"gold")				return RGB(0xff,0xd7,0x00);
	else if (sColor ==	"lightgoldenrod")		return RGB(0xee,0xdd,0x82);
	else if (sColor ==	"goldenrod")			return RGB(0xda,0xa5,0x20);
	else if (sColor ==	"darkgoldenrod")		return RGB(0xb8,0x86,0x0b);
	else if (sColor ==	"rosybrown")			return RGB(0xbc,0x8f,0x8f);
	else if (sColor ==	"indianred")			return RGB(0xcd,0x5c,0x5c);
	else if (sColor ==	"saddlebrown")			return RGB(0x8b,0x45,0x13);
	else if (sColor ==	"sienna")				return RGB(0xa0,0x52,0x2d);
	else if (sColor ==	"peru")				return RGB(0xcd,0x85,0x3f);
	else if (sColor ==	"burlywood")			return RGB(0xde,0xb8,0x87);
	else if (sColor ==	"beige")				return RGB(0xf5,0xf5,0xdc);
	else if (sColor ==	"wheat")				return RGB(0xf5,0xde,0xb3);
	else if (sColor ==	"sandybrown")			return RGB(0xf4,0xa4,0x60);
	else if (sColor ==	"tan")					return RGB(0xd2,0xb4,0x8c);
	else if (sColor ==	"chocolate")			return RGB(0xd2,0x69,0x1e);
	else if (sColor ==	"firebrick")			return RGB(0xb2,0x22,0x22);
	else if (sColor ==	"brown")				return RGB(0xa5,0x2a,0x2a);
	else if (sColor ==	"darksalmon")			return RGB(0xe9,0x96,0x7a);
	else if (sColor ==	"salmon")				return RGB(0xfa,0x80,0x72);
	else if (sColor ==	"lightsalmon")			return RGB(0xff,0xa0,0x7a);
	else if (sColor ==	"orange")				return RGB(0xff,0xa5,0x00);
	else if (sColor ==	"darkorange")			return RGB(0xff,0x8c,0x00);
	else if (sColor ==	"coral")				return RGB(0xff,0x7f,0x50);
	else if (sColor ==	"lightcoral")			return RGB(0xf0,0x80,0x80);
	else if (sColor ==	"tomato")				return RGB(0xff,0x63,0x47);
	else if (sColor ==	"orangered")			return RGB(0xff,0x45,0x00);
	else if (sColor ==	"red")					return RGB(0xff,0x00,0x00);
	else if (sColor ==	"hotpink")				return RGB(0xff,0x69,0xb4);
	else if (sColor ==	"deeppink")			return RGB(0xff,0x14,0x93);
	else if (sColor ==	"pink")				return RGB(0xff,0xc0,0xcb);
	else if (sColor ==	"lightpink")			return RGB(0xff,0xb6,0xc1);
	else if (sColor ==	"palevioletred")		return RGB(0xdb,0x70,0x93);
	else if (sColor ==	"maroon")				return RGB(0xb0,0x30,0x60);
	else if (sColor ==	"mediumvioletred")		return RGB(0xc7,0x15,0x85);
	else if (sColor ==	"violetred")			return RGB(0xd0,0x20,0x90);
	else if (sColor ==	"magenta")				return RGB(0xff,0x00,0xff);
	else if (sColor ==	"violet")				return RGB(0xee,0x82,0xee);
	else if (sColor ==	"mediumorchid")		return RGB(0xba,0x55,0xd3);
	else if (sColor ==	"darkorchid")			return RGB(0x99,0x32,0xcc);
	else if (sColor ==	"darkviolet")			return RGB(0x94,0x00,0xd3);
	else if (sColor ==	"blueviolet")			return RGB(0x8a,0x2b,0xe2);
	else if (sColor ==	"purple")				return RGB(0xa0,0x20,0xf0);
	else if (sColor ==	"mediumpurple")		return RGB(0x93,0x70,0xdb);
	else if (sColor ==	"thistle")				return RGB(0xd8,0xbf,0xd8);
	else if (sColor ==	"aqua")				return RGB(0, 255, 255);
	else return -1;
}
#endif

void	SystemShutdown(int nMode)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES TokenPrivileges;
	
	OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL,
		SE_SHUTDOWN_NAME,
		&TokenPrivileges.Privileges[0].Luid);
	
	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	
	AdjustTokenPrivileges(	hToken,
		FALSE,
		&TokenPrivileges,
		0,
		(PTOKEN_PRIVILEGES)NULL,
		0);
	
	if (nMode == SYSTEM_LOGOFF)
		ExitWindowsEx(EWX_LOGOFF | EWX_FORCE, 0);
	else if (nMode == SYSTEM_REBOOT)
		ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
	else if (nMode == SYSTEM_POWEROFF)
		ExitWindowsEx(EWX_POWEROFF | EWX_FORCE, 0);

	CloseHandle(hToken);
}

void SystemShutdownNT(int nMode /* = 2 */)
{
	HANDLE				hToken;		// handle to process token 
	TOKEN_PRIVILEGES	tkp;		// pointer to token structure 
	bool				fResult;	// system shutdown flag 
	bool				bReboot;
	
	if (nMode == 1)
		bReboot = TRUE;
	else
		bReboot = FALSE;
	
	// Get the current process token handle so we can get shutdown 
	// privilege. 
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		TRACE("OpenProcessToken failed.");
	
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
	
	tkp.PrivilegeCount              = 1;                                    // one privilege to set    
	tkp.Privileges[0].Attributes    = SE_PRIVILEGE_ENABLED; 
	
	// Get shutdown privilege for this process. 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0); 
	
	// Cannot test the return value of AdjustTokenPrivileges. 
	if (GetLastError() != ERROR_SUCCESS) 
	{
		TRACE("AdjustTokenPrivileges enable failed.\n"); 
		CloseHandle(hToken);
		return;
	}
	
	// Display the shutdown dialog box and start the time-out countdown. 
	fResult = InitiateSystemShutdown(
		NULL,					// shut down local computer 
		_T("System rebooting."),	// message to user 
		0,						// time-out period (<- 여기를 20 이라고 쓰면 20초 후 에 리부팅한다.)
		FALSE,					// ask user to close apps 
		bReboot);				// reboot after shutdown 
	
	if (!fResult) 
	{ 
		TRACE("InitiateSystemShutdown failed."); 
		return;
	} 
	
	// error string..
	// PrintCSBackupAPIErrorMessage(GetLastError());
	// Disable shutdown privilege. 
	
	tkp.Privileges[0].Attributes = 0; 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0); 
	
	if (GetLastError() != ERROR_SUCCESS) 
		TRACE("AdjustTokenPrivileges disable failed."); 
}
/*
bool	IsNumericString(CString str)
{
	bool	bIsNumber = TRUE;
	
	for (int i = 0; i < str.GetLength(); i++)
	{
		if (str[i] < '0' || str[i] > '9')
			return FALSE;
	}
	
	return TRUE;
}
*/
bool IsNumericString(const CString& strSource)
{
	try 
	{
		CString strData=strSource;
		strData=strData.Trim();

		if(strData.IsEmpty())
			return FALSE;

		//소숫점이 있는 경우
		int iDotPos = strData.Find(_T(".")); 
		if (0 <= iDotPos)
		{
			if (0<= strData.Find(_T("."), iDotPos + 1))    // 두번째 "."
			{
				return FALSE;
			}
		} 
		int iCountE=0;
		bool bNextSign=FALSE;
		bool bNextNum=FALSE;
		for (int i = 0; i < strData.GetLength() ; i++) {
			CString sChar =strData.Mid(i, 1);
			if (sChar.FindOneOf(_T("1234567890")) < 0)
			{
				if(sChar.FindOneOf(_T("-+.eE")) <0)
				{
					return FALSE;
				}
				bool bHasE = (0 <= sChar.FindOneOf(_T("eE"))) ;
				if(bHasE)
				{
					//bool bDotNothing = (iDotPos<0);//.이 없으면 Error
					//if(bDotNothing)
					//{
					// return FALSE;
					//}
					bool bDotPosLater = (i<iDotPos);//.이 E보다 뒤에 있으면 Error
					if(bDotPosLater)
					{
						return FALSE;
					}
					iCountE++;
					if(1<iCountE){//E가 두개 있으면 Error
						return FALSE;
					}
					bNextSign=TRUE;
					continue;
				}

				//E뒤에는 +나 -가 있어야하고 뒤에는 숫자가 있어야 함
				if(bNextSign)
				{
					if(sChar.FindOneOf(_T("-+")) <0)
					{
						return FALSE;
					}
					bNextSign=FALSE;
					bNextNum=TRUE;
					continue;
				}
				if(bNextNum)
				{
					return FALSE;
				}
				bool bSign= (0 <= sChar.FindOneOf(_T("+-"))) ;
				bool bSignError = bSign && (0!=i); //처음이 아니면 Error
				if(bSignError)
				{
					return FALSE;
				}
			}
			else //숫자인 경우
			{
				if(bNextSign)
				{
					return FALSE;
				}

				bNextNum=FALSE;
			}
		}



		if(bNextSign || bNextNum)
		{
			return FALSE;
		}
		return TRUE;

	}
	catch(...)
	{
		return FALSE;
	}
}

//주어진 문자열이 알파벳과 숫자로만 구성된 문자열인지 검사한다.
//excepts에는 포함되도 되는 문자열들이 들어있는데 이들은 ;으로 구분되어 있고
//검사하기 전에 미리 삭제한 후 검사한다.
bool IsAlphaNumeric(CString str, CString excepts)
{
	int i;
	std::deque<CString> dqExcepts;

	if (excepts.IsEmpty() == false)
	{
		get_token_string(excepts, dqExcepts, ';');
		for (i = 0; i < dqExcepts.size(); i++)
			str.Replace(dqExcepts[i], _T(""));
	}

	for (int i = 0; i < str.GetLength(); i++)
	{
		if ((str[i] >= '0' && str[i] <= '9') ||
			(str[i] >= 'a' && str[i] <= 'z') ||
			(str[i] >= 'A' && str[i] <= 'Z'))
		{
		}
		else
			return false;
	}
	
	return true;
}

//ASCII 코드의 #33(0x21)(' ') ~ #126(0x7E)('~') 범위인지(읽을 수 있는 문자열인지)
bool is_readable_char(CString src)
{
	for (int i = 0; i < src.GetLength(); i++)
	{
		if (src[i] < 33 || src[i] > '~')
			return false;
	}

	return true;
}

//각 언어마다 주석처리 문자열이 다르므로 주석처리된 라인인지 판별
//리턴값은 해당 주석처리 문자열
CString	is_comment(CString src)
{
	src.Trim();

	if (src.IsEmpty())
		return _T("");

	std::deque<CString>::iterator it;
	
	for (it = g_comment_mark.begin(); it != g_comment_mark.end(); it++)
	{
		if (src.Find(*it) == 0)
			return *it;
	}

	return _T("");
}


//'가'~'힣'범위의 온전한 한글인지 검사한다.
//'가' = true
//'강' = true
//'강ㄷ' = false
//allow_ascii가 true라면 영문, 숫자, 특수문자가 있어도 한글만 온전하면 true이며
//allow_ascii가 false라면 오로지 정상적인 한글로만 구성되었는지를 판별하여 리턴한다.
bool is_hangul(CString str, bool allow_ascii, int* hangul_count)
{
	const wchar_t start_ch = L'가';
	const wchar_t end_ch = L'힣';

	if (hangul_count)
		*hangul_count = 0;

	for (int i = 0; i < str.GetLength(); i++)
	{
		int code = str[i];

		if (allow_ascii)
		{
			if (code >= 0 && code <= 255)
				continue;
		}

		if (str[i] < start_ch || str[i] > end_ch)
			return false;

		if (hangul_count)
			(*hangul_count)++;
	}

	return true;
}

static const char* const initial_array[] = {
	"ㄱ", "ㄲ", "ㄴ", "ㄷ", "ㄸ", "ㄹ", "ㅁ", "ㅂ", "ㅃ", "ㅅ",
	"ㅆ", "ㅇ", "ㅈ", "ㅉ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ"
};
static const char* const medial_array[] = {
	"ㅏ", "ㅐ", "ㅑ", "ㅒ", "ㅓ", "ㅔ", "ㅕ", "ㅖ", "ㅗ", "ㅘ",
	"ㅙ", "ㅚ", "ㅛ", "ㅜ", "ㅝ", "ㅞ", "ㅟ", "ㅠ", "ㅡ", "ㅢ",
	"ㅣ"
};
static const char* const final_array[] = {
	  "", "ㄱ", "ㄲ", "ㄳ", "ㄴ", "ㄵ", "ㄶ", "ㄷ", "ㄹ", "ㄺ",
	"ㄻ", "ㄼ", "ㄽ", "ㄾ", "ㄿ", "ㅀ", "ㅁ", "ㅂ", "ㅄ", "ㅅ",
	"ㅆ", "ㅇ", "ㅈ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ"
};

//문자열이 온전한지 깨진 문자인지를 판별(특히 한글 인코딩 깨짐 판별)
//"한글'??쒗넻??ъ뿰而李⑦듃?댁옱??"
bool is_valid_string(CString src, bool include_hangul)
{
	int i;
	TCHAR ch;

	for (i = 0; i < src.GetLength(); i++)
	{
		ch = src[i];
		if ((ch < ' ' || ch > '~') && (!include_hangul || !is_hangul(ch)))
			return false;
	}

	return true;
}

//완성형 한글의 한 글자를 초성, 중성, 종성으로 분리한다.
bool get_consonant(CString src, wchar_t* cho, wchar_t* jung, wchar_t* jong)
{
	const wchar_t *tsrc = CString2WCHAR(src);
	//wchar_t *asdf = L"한";
	wchar_t uniValue = *tsrc - 0xAC00;
	*jong = uniValue % 28;
	*jung = ((uniValue - *jong) / 28) % 21;
	*cho = ((uniValue - *jong) / 28) / 21;

	//CString c(initial_array[cho]);
	//CString j(jung + 0x1161);
	//CString o(jong + 0x11a7);
	return true;
}


bool IsNatural(LPCTSTR lpszValue)
{
	int nLen = (int)_tcslen(lpszValue);

	if (nLen < 1)
		return false;

	for (int i = 0; i < nLen; ++i)
	{
		if (lpszValue[i] < '0' || lpszValue[i] > '9')
			return false;
	}
	return true;
}

bool IsNumeric(LPCTSTR lpszValue)
{
	CString str = lpszValue;
	str.Replace(_T("."), _T(""));

	if (IsNatural(str) == false || str.GetLength() == 0)
		return false;

	int nNumberOfSeparators = 0;
	int nLen = (int)_tcslen(lpszValue);
	for (int i = 0; i < nLen; ++i)
	{
		if (lpszValue[i] == '.')
			nNumberOfSeparators++;
	}

	if (nNumberOfSeparators > 1)
		return false;

	return true;
}

bool IsInteger(LPCTSTR lpszValue)
{
	int nLen = (int)_tcslen(lpszValue);
	for (int i = 0; i < nLen; ++i)
	{
		if (lpszValue[i] < '0' || lpszValue[i] > '9')
		{
			if (0 != i || lpszValue[i] != '-')
				return false;
		}
	}
	return true;
}

//start부터 시작해서 처음 만나는 숫자 영역을 추출해서 숫자로 리턴한다.
bool get_number_from_string(CString str, int &num, int start)
{
	int i = start;

	while (true)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			i++;
			if (i == str.GetLength())
			{
				//i--;
				break;
			}
			continue;
		}
		else
		{
			break;
		}
	}

	if (i == start)
		return false;

	CString num_str = str.Mid(start, i - start);
	num = _ttoi(num_str);

	return true;
}

//문자열에 포함된 숫자문자를 숫자로 간주하여 비교한다.
//"a5"는 "a12345"보다 작다.
//단, "a5"는 "a-22"와 같이 "-"기호는 부호로도, 특수문자로도 간주하지 않으므로 "a5"가 작다고 처리된다.
bool is_greater_with_numeric(CString str0, CString str1)
{
	//StrCmpLogicalW만으로는 탐색기와 같은 차례로 정렬되지 않아서
	//아래 코드를 넣었으나 지금 다시 돌려보면
	//StrCmpLogicalW만으로도 탐색기와 같은 차례로 정렬된다.
	//분명히 StrCmpLogicalW만으로는 같지 않았는데...
	/*
	int i = 0;

	while (true)
	{
		//서로 다른 문자를 만날때까지 스킵한다.
		if (str0[i] == str1[i])
		{
			i++;
			if (i >= str0.GetLength() || i >= str1.GetLength())
				break;
			continue;
		}
		else
		{
			break;
		}
	}

	if (i == str0.GetLength())
		return false;
	if (i == str1.GetLength())
		return true;

	int num0;
	int num1;

	//만약 파일명 처음 ~ i바로 전까지 숫자였고 i부터 숫자가 아니면
	//파일명 시작부터 숫자로 된 파일명인 경우다.
	
	if ((i > 0) && (str0[i] < '0' || str0[i] > '9' || str1[i] < '0' || str1[i] > '9') &&
		IsNumeric(str0.Left(i)))
	{
		//아래 if문은 항상 true일 수 밖에 없을 것이다.
		if (get_number_from_string(str0, num0, 0) &&
			get_number_from_string(str1, num1, 0))
			return (num0 > num1);
	}
	*/

	LPCWSTR s0, s1;
#ifdef UNICODE
	s0 = str0;
	s1 = str1;
#else
	USES_CONVERSION;
	s0 = A2W(str0);
	s1 = A2W(str1);
#endif

	return (StrCmpLogicalW((CStringW)s0, (CStringW)s1) == 1);
}


//n을 26진수 엑셀 컬럼 인덱스로 변환한 문자열을 리턴
CString	ConvertInt2AZ(int n)
{
	int		div, mod;
	CString	str;
	CString sResult = _T("");

	while (true)
	{
		div = n / 26;
		mod = n % 26;

		if (div > 0 && mod == 0)
		{
			div--;
			mod = 26;
		}

		str.Format(_T("%c"), 'A' - 1 + mod);
		sResult = str + sResult;

		n = div;
		if (n <= 0)
			break;
		else if (n < 26)
		{
			str.Format(_T("%c"), 'A'- 1 + n);
			sResult = str + sResult;
			break;
		}
	}

	return sResult;
}

#pragma comment(lib, "WinInet.lib")
bool CheckInternetIsOnline()
{
	DWORD	dwFlags;
	bool	bChkInternet = InternetGetConnectedState(&dwFlags,0);
	
	//	if (dwFlags == INTERNET_CONNECTION_LAN);
	//INTERNET_CONNECTION_PROXY 
	//INTERNET_CONNECTION_MODEM_BUSY
	return bChkInternet;
}

#include <Afxsock.h>
bool IsNetwork(CString szSite, UINT nPort) 
{
	CSocket socket;
	
	socket.Create();
	
	if (socket.Connect(szSite, nPort) == FALSE)
		return FALSE;
	
	return TRUE;
}

bool IsAvailableEMail(CString sEMail)
{
	if (sEMail == "")
		return FALSE;  // 이메일이 없으면 false

	int iAtCount = 0;   //@ 위치
	int iDotCount = 0;  // . 위치
	int i;

	for(i = 0; i < sEMail.GetLength(); i++)
	{
		if (i > 0 && sEMail.GetAt(i) == '@')
			iAtCount = i+1;    // ①

		if(iAtCount > 0 && i > iAtCount && sEMail.GetAt(i) == '.')
			iDotCount = i+1;   // ②
	}

	if (i > iDotCount && iAtCount > 0 && iDotCount > 0)
		return TRUE;     // ③
	
	return FALSE;
}



void ReadURLFileString(CString sURL, CString &sString)
{
	TCHAR	sWinDir[MAX_PATH];
	CString sTempDir;
	GetWindowsDirectory(sWinDir, MAX_PATH);
	
	CString sTempFile = sWinDir;
	
	sTempFile = sTempFile.Left(3) + _T("Temp");
	CreateDirectory(sTempFile, NULL);
	
	sTempFile = sTempFile + _T("\\_temp.tmp");
	
	URLDownloadToFile(NULL, sURL, sTempFile, 0, NULL);
	
	TCHAR	s[1000];
	FILE*	fp;

	if ((fp = _tfopen(sTempFile, _T("r"))) != 0)
		return;
	
	_fgetts(s, 1000, fp);
	
	sString = s;
	
	fclose(fp);
	
	DeleteFile(sTempFile);
}

bool ReadURLFile(LPCTSTR pUrl, CString &strBuffer) 
{
	TCHAR g_szHead[] = _T("Accept: */*\r\n\r\n");
	TCHAR g_pBuffer[2049]; 
	
	CHttpFile *pDownFile = NULL; 
	CInternetSession m_DownSession(_T("INIPayDownload"), 1, INTERNET_OPEN_TYPE_DIRECT); 
	
	TRY 
	{ 
		pDownFile = (CHttpFile*)m_DownSession.OpenURL(pUrl, 1, 
			INTERNET_FLAG_RELOAD | INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_DONT_CACHE
			, g_szHead, -1L); 
	} 
	CATCH(CException, e) 
	{ 
		//e->ReportError(); 
		e->Delete();
		return FALSE; 
	} 
	END_CATCH 
		
		if(pDownFile == NULL)
			return FALSE; 
		
		int nSize = 2048; 
		int nPush = 0; 
		int nLength = 2048; 
		int nSum = 0; 
		
		pDownFile->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, g_pBuffer, (unsigned long*)&nSize, NULL); 
		nSize = _tstoi((TCHAR*)g_pBuffer); 
		
		if(nSize <= 0) 
		{ 
			pDownFile->Close(); 
			delete pDownFile; 
			
			return FALSE; 
		} 
		
		strBuffer.Empty(); 
		
		for(;nLength == 2048;) 
		{ 
			TRY 
			{ 
				nLength = pDownFile->Read(g_pBuffer, 2048); 
			} 
			CATCH(CException, e) 
			{ 
				pDownFile->Close(); 
				
				delete pDownFile; 
				
				//e->ReportError(); 
				e->Delete(); 
				
				return FALSE; 
			} 
			END_CATCH 
				
				g_pBuffer[nLength] = 0x00; 
			strBuffer += g_pBuffer; 
			
			nSum += nLength; 
		} 
		
		ASSERT(nSum == nSize); 
		
		pDownFile->Close(); 
		m_DownSession.Close(); 
		
		delete pDownFile; 
		
		return TRUE; 
} 

std::wstring multibyteToUnicode(std::string inputtext)
{
	int length = MultiByteToWideChar(CP_ACP, 0, &inputtext[0], inputtext.size(), NULL, NULL);
	std::wstring outputtext(length + 1, 0);
	MultiByteToWideChar(CP_ACP, 0, &inputtext[0], inputtext.size(), &outputtext[0], length);
	return outputtext;
}

std::string unicodeToMultibyte(std::wstring inputtext)
{
	int length = WideCharToMultiByte(CP_ACP, 0, &inputtext[0], -1, NULL, 0, NULL, NULL);
	std::string outputtext(length + 1, 0);
	WideCharToMultiByte(CP_ACP, 0, &inputtext[0], -1, &outputtext[0], length, NULL, NULL);
	return outputtext;
}

std::wstring utf8ToUnicode(std::string inputtext)
{
	int length = MultiByteToWideChar(CP_UTF8, 0, &inputtext[0], -1, NULL, NULL);
	std::wstring outputtext(length + 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, &inputtext[0], -1, &outputtext[0], length);
	return outputtext;
}

std::string unicodeToUtf8(std::wstring inputtext)
{
	int length = WideCharToMultiByte(CP_UTF8, 0, &inputtext[0], -1, NULL, 0, NULL, NULL);
	std::string outputtext(length + 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, &inputtext[0], -1, &outputtext[0], length, NULL, NULL);
	return outputtext;
}

std::string multibyteToUtf8(std::string inputtext)
{
	return (unicodeToUtf8(multibyteToUnicode(inputtext)));
}

std::string utf8ToMultibyte(std::string inputtext)
{
	return (unicodeToMultibyte(utf8ToUnicode(inputtext)));
}

//http://localhost:4300/test_doc_favorite/test.avi
bool parse_url(CString full_url, CString& ip, int& port, CString& sub_url, bool &is_https)
{
	DWORD dwServiceType;
	INTERNET_PORT nPort;

	//AfxParseURL()을 사용하기 위해서는 url에 반드시 http:// 또는 https:// 등과 같은 서비스 종류가 표시되어야 한다.
	if (full_url.Left(7) != _T("http://") && full_url.Left(8) != _T("https://"))
		full_url = (is_https ? _T("https://") : _T("http://")) + full_url;
	else if (full_url.Left(7) == _T("http://"))
		is_https = false;
	else if (full_url.Left(8) == _T("https://"))
		is_https = true;

	bool ret = AfxParseURL(full_url, dwServiceType, ip, sub_url, nPort);
	port = (int)nPort;
	return ret;
}

//void request_url(CRequestUrlParams* params)
//{
//	params->status = request_url(params->result, params->ip, params->port, params->sub_url, params->verb, &params->headers, params->body, params->local_file_path);
//}
/*
DWORD request_url(CString& result_str, CString full_url, CString verb, std::vector<CString> *headers, CString jsonBody, CString local_file_path)
{
	DWORD statusCode = HTTP_STATUS_BAD_REQUEST;

	CString ip, sub_url;
	int port;

	parse_url(full_url, ip, port, sub_url);

	return request_url(result_str, ip, port, sub_url, verb, headers, jsonBody, local_file_path);
}
*/
//url을 호출하여 결과값을 리턴하거나 지정된 로컬 파일로 다운로드 한다.
//local_file_path가 ""이면 결과값을 문자열로 리턴받는다.
//local_file_path가 지정되어 있으면 파일로 다운받는다. (이때 result_str은 "")
//리턴값이 200이 아닐 경우는 리턴된 에러코드와 result_str에 저장된 에러 메시지를 조합하여 에러 처리한다.
//DWORD request_url(CString &result_str, CString ip, int port, CString sub_url, CString verb, std::vector<CString> *headers, CString jsonBody, CString local_file_path)
//https://m.blog.naver.com/nawoo/80132924296
//https://blog.naver.com/tija98/120037685173
void request_url(CRequestUrlParams* params)
{
	long t0 = clock();

	//ip에 http://인지 https://인지가 명시되어 있다면 이는 명확하므로
	//이를 판단하여 params->is_https값을 재설정한다.
	//포트번호로 https를 판별하는 것은 한계가 있으므로 ip에 명시하든, params->is_https에 정확히 명시하여 사용한다.
	//또한 ip에 http:// 또는 https:// 가 붙어 있으면 InternetConnect()은 실패한다. 제거하고 호출해줘야 한다.
	if (params->ip.Left(7) == _T("http://") || params->port == 80)
	{
		params->is_https = false;
		params->ip.Replace(_T("http://"), _T(""));
	}
	else if (params->ip.Left(8) == _T("https://") || params->port == 443)
	{
		params->is_https = true;
		params->ip.Replace(_T("https://"), _T(""));
	}


	if (params->full_url.IsEmpty() == false)
	{
		parse_url(params->full_url, params->ip, params->port, params->sub_url, params->is_https);
	}


	//sub_url의 맨 앞에는 반드시 '/'가 붙어있어야 한다.
	if (params->sub_url[0] != '/')
		params->sub_url = _T("/") + params->sub_url;

	params->full_url.Format(_T("%s%s:%d%s"),
			(params->is_https ? _T("https://") : _T("http://")),
			params->ip, params->port, params->sub_url);

	bool ret;
	CString str;
	CString remoteURL;
	TCHAR szHead[] = _T("Accept: */*\r\n\r\n");

	if (params->ip.GetLength() < 7)
	{
		params->result = _T("Invalid IP address = ") + params->ip;
		params->status = -1;
		TRACE(_T("result = %s\n"), params->result);
		return;
	}


	//포트가 0보다 작으면 기본 포트를 사용한다.
	if (params->port <= 0)
	{
		if (params->is_https)
			params->port = 443;
		else
			params->port = 80;
	}

	if (params->verb.IsEmpty())
		params->verb = _T("GET");

	params->verb.MakeUpper();
	if (!is_one_of(params->verb, _T("GET"), _T("PUT"), _T("POST"), _T("DELETE")))
	{
		params->status = HTTP_STATUS_BAD_METHOD;
		params->result = _T("Unknown HTTP Request method(\"") + params->verb + _T("\")");
		TRACE(_T("result = %s\n"), params->result);
		return;
	}

	//INTERNET_OPEN_TYPE_PRECONFIG만 사용하면 Proxy 설정 유무와 무관하게 정상 동작한다.
	//단, 어떤 request때문에 INTERNET_OPEN_TYPE_DIRECT 옵션을 넣었었으나 이 옵션이 추가되면 Proxy 설정이 있을 경우 실패하게 된다.
	//또한 Proxy 설정정보가 변경된 후에는 407 에러가 발생하는데
	//윈도우에서 Proxy 설정정보를 다시 입력받는 창이 나오고 정상 입력된 후에는 이 API도 정상적으로 동작하게 된다.
	//만약 407 에러가 발생하면 
	HINTERNET hInternetRoot = InternetOpen(_T("request_url"), INTERNET_OPEN_TYPE_PRECONFIG/* | INTERNET_OPEN_TYPE_DIRECT*/, NULL, NULL, 0);
	if (hInternetRoot == NULL)
	{
		params->status = GetLastError();
		params->result = _T("InternetOpen() failed.");
		TRACE(_T("result = %s\n"), params->result);
		return;
	}


	HINTERNET hInternetConnect = InternetConnect(hInternetRoot,
		params->ip,
		params->port,
		_T(""),
		_T(""),
		INTERNET_SERVICE_HTTP,
		0,
		0);

	if (hInternetConnect == NULL)
	{
		params->result = _T("hInternetConnect() failed.");
		params->status = GetLastError();
		TRACE(_T("result = %s\n"), params->result);
		return;
	}


	//HINTERNET hURL = InternetOpenUrl(hInternetRoot, remoteURL, szHead, -1L, secureFlags, 0);
	//if (hURL == NULL) {
	//	InternetCloseHandle(hInternetRoot);
	//	return _T("error=InternetOpenUrl() failed.");
	//}

	int secureFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_TRANSFER_BINARY; // http
	if (params->is_https)
	{
		secureFlags |= INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID; // https
	}

	HINTERNET hOpenRequest = HttpOpenRequest(hInternetConnect,
		params->verb,
		params->sub_url,
		HTTP_VERSION,
		_T(""),
		NULL,
		secureFlags,
		0);

	//2009년 블로그에는 INTERNET_OPTION_RECEIVE_TIMEOUT외에 나머지 2개의 timeout은
	//버그라고 되어 있는데 현재도 그러한지는 확인되지 않고 동작도 되지 않는듯함.
	//https://blog.naver.com/che5886/20061092638
	//20250117 30초 timeout됨을 확인 완료.
	DWORD dwTimeout = params->timeout_ms;
	InternetSetOption(hOpenRequest, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(DWORD));
	InternetSetOption(hOpenRequest, INTERNET_OPTION_SEND_TIMEOUT, &dwTimeout, sizeof(DWORD));
	InternetSetOption(hOpenRequest, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTimeout, sizeof(DWORD));

	//Proxy 계정관련 코드를 추가했으나 확인 필요.
	if (params->proxy_id.IsEmpty() == false || params->proxy_pw.IsEmpty() == false)
	{
		char* proxy_id = CString2char(params->proxy_id);
		char* proxy_pw = CString2char(params->proxy_pw);
		InternetSetOption(hOpenRequest, INTERNET_OPTION_PROXY_USERNAME, proxy_id, strlen(proxy_id));
		InternetSetOption(hOpenRequest, INTERNET_OPTION_PROXY_PASSWORD, proxy_pw, strlen(proxy_pw));
		delete[] proxy_id;
		delete[] proxy_pw;
	}

	if (params->is_https)
	{
		DWORD dwFlags = 0;
		DWORD dwBuffLen = sizeof(dwFlags);

		dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
			SECURITY_FLAG_IGNORE_REVOCATION |
			SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP |
			SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS |
			SECURITY_FLAG_IGNORE_WRONG_USAGE |
			SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
			SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
		InternetSetOption(hOpenRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
	}

	//기본 헤더를 추가하고
	params->headers.push_front(_T("Content-Type: application/json; charset=utf-8\r\n"));

	//헤더의 끝문자열 검사 및 추가
	for (int i = 0; i < params->headers.size(); i++)
	{
		if (params->headers[i].Right(2) != _T("\r\n"))
			params->headers[i] += _T("\r\n");
		HttpAddRequestHeaders(hOpenRequest, params->headers[i], -1, HTTP_ADDREQ_FLAG_ADD);
	}

	BOOL res = FALSE;
#ifdef _UNICODE
	int char_str_len = WideCharToMultiByte(CP_UTF8, 0, params->body, -1, NULL, NULL, NULL, NULL);
	char* jsonData = new char[char_str_len];
	ZeroMemory(jsonData, char_str_len);
	WideCharToMultiByte(CP_UTF8, 0, params->body, -1, jsonData, char_str_len, 0, 0);

	//body = ""일 경우 아래 if문이 의미없다고 판단되나 backend의 .py에서 ""와 null string을 처리하는 방식이
	//postman과는 다르다는 GPT의 설명이 있다. 우선 이 if문을 그대로 둔다.
	if (strlen(jsonData) == 0)
		res = HttpSendRequest(hOpenRequest, NULL, 0, 0, 0);
	else
		res = HttpSendRequest(hOpenRequest, NULL, 0, jsonData, strlen(jsonData));
#else
	int char_str_len = WideCharToMultiByte(CP_UTF8, 0, CStringW(params->body), -1, NULL, 0, NULL, NULL);
	char* jsonData = new char[char_str_len];
	char* charMsg = params->body.GetBuffer(params->body.GetLength());
	//int char_str_len = WideCharToMultiByte(CP_UTF8, 0, CT2CA(jsonBody), -1, NULL, 0, NULL, NULL);
	//sprintf(jsonData, "%s", jsonBody);

	WideCharToMultiByte(CP_UTF8, 0, (CStringW)params->body, -1, jsonData, char_str_len, 0, 0);
	//CString2char()
	res = HttpSendRequest(hOpenRequest, NULL, 0, jsonData, strlen(jsonData));
#endif

	if (!res)
	{
		DWORD dwError = GetLastError();
		params->status = dwError;
		params->result.Format(_T("HttpSendRequest failed. elapsed = %ldms. error code = %d(%s)"), clock() - t0, dwError, get_error_str(dwError));
		TRACE(_T("result = %s\n"), params->result);

		SAFE_DELETE_ARRAY(jsonData);

		InternetCloseHandle(hOpenRequest);
		InternetCloseHandle(hInternetConnect);
		InternetCloseHandle(hInternetRoot);

		return;
	}

	DWORD buffer_size = 4096;
	DWORD dwRead, dwWritten, dwTotalSize = 0;
	char* buffer = new char[buffer_size];
	TCHAR query_buffer[32] = { 0, };
	DWORD query_buffer_size = sizeof(query_buffer);

	memset(buffer, 0, buffer_size);

	if (params->local_file_path.IsEmpty())
	{
		//total_buffer = new char[buffer_size * 10];
		//memset(total_buffer, 0, buffer_size * 10);
	}

	//size_buffer가 char면 파일크기를 못얻어온다.
	//파일이 존재하지 않아도 에러 내용이 포함된 html이 넘어오므로 항상 그 값이 0보다 크다.
	// 연결정보 확인

	ret = HttpQueryInfo(hOpenRequest, HTTP_QUERY_STATUS_CODE, (LPVOID)&query_buffer, &query_buffer_size, NULL);
	params->status = _ttol(query_buffer);

	if (!ret)
	{
		params->result = _T("HttpQueryInfo(HTTP_QUERY_STATUS_CODE) failed.");
		//params->status = GetLastError();
		TRACE(_T("result = %s\n"), params->result);

		SAFE_DELETE_ARRAY(jsonData);
		SAFE_DELETE_ARRAY(buffer);

		InternetCloseHandle(hOpenRequest);
		InternetCloseHandle(hInternetConnect);
		InternetCloseHandle(hInternetRoot);

		return;
	}

	if (params->status != HTTP_STATUS_OK)
	{
		SAFE_DELETE_ARRAY(jsonData);
		SAFE_DELETE_ARRAY(buffer);

		InternetCloseHandle(hOpenRequest);
		InternetCloseHandle(hInternetConnect);
		InternetCloseHandle(hInternetRoot);

		return;
	}

	HANDLE hFile = NULL;

	if (!params->local_file_path.IsEmpty())
	{
		CString folder = get_part(params->local_file_path, fn_folder);
		make_full_directory(folder);
	}

	params->result.Empty();

	//0바이트의 파일은 다운받지 않아도 될 듯 하지만
	//서버의 파일과 다운받은 로컬의 파일의 수가 같은지 등을 비교할 수도 있으므로 생성하자.
	//HTTP_QUERY_FLAG_NUMBER을 넣지 않으면 HttpQueryInfo()에서 오류가 발생한다.
	DWORD dwBufLen = sizeof(dwTotalSize);
	uint64_t total_read = 0;
	char* total_result = NULL;
	ret = HttpQueryInfo(hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwTotalSize, &dwBufLen, NULL);

	if (!ret)
		dwTotalSize = 4 * 10240;

	total_result = new char[dwTotalSize + 1];
	memset(total_result, 0, sizeof(char) * (dwTotalSize + 1));

	do
	{
		//InternetQueryDataAvailable(hOpenRequest, &dwSize, 0, 0); //이 함수는 웹페이지의 크기를 리턴하는 듯하다.
		long t2 = clock();
		//1.48GB, buffer_size에 따른 시간 비교. 1K:133s, 4K:65s, 1M:63s, 4M:64s
		//버퍼 크기가 1K면 너무 빈번한 read가 발생하여 느리지만 4K이상이면 큰 차이는 발생하지 않는다.
		InternetReadFile(hOpenRequest, buffer, buffer_size, &dwRead);

		total_read += dwRead;

		if (dwRead == 0)
			break;

		if (params->local_file_path.IsEmpty())
		{
			//UTF-8 with BOM으로 작성된 txt 파일을 읽어오면 파일 헤더에 EF BB BF 라는 3 char가 붙어온다.
			//이는 윈도우에서 fopen으로 읽어올때는 문제되지 않으나
			//InternetReadFile()로 읽어와서 저장하면 맨 앞 글자가 깨져 표시되는 문제가 발생한다. 날려준다.
			if (byte(buffer[0]) == 0xEF &&
				byte(buffer[1]) == 0xBB &&
				byte(buffer[2]) == 0xBF)
				memcpy(buffer, buffer + 3, dwRead - 3);
			//TRACE(_T("%X, %X, %X\n"), buffer[0], buffer[1], buffer[2]);
			//buffer[dwRead] = '\0';
			strncat(total_result, buffer, dwRead);
			//TRACE(_T("total_result len = %d\n"), strlen(total_result));

			//buffer_size씩 읽어와서 UTF8toCString()하여 result에 넣어주면 반쪽짜리 한글이 생겨서 깨지게 된다.
			//모두 읽어온 후 한번에 UTF8toCString()을 돌려야 한다.
			//params->result += UTF8toCString(buffer);
		}
		else
		{
			//remote file이 존재하지 않을 경우 로컬에 파일을 만들지 않기 위해 여기서 체크.
			if (hFile == NULL)
			{
				if (PathFileExists(params->local_file_path) && !DeleteFile(params->local_file_path))
				{
					SAFE_DELETE_ARRAY(buffer);
					SAFE_DELETE_ARRAY(total_result);

					InternetCloseHandle(hOpenRequest);
					InternetCloseHandle(hInternetConnect);
					InternetCloseHandle(hInternetRoot);

					params->result = _T("error=") + params->local_file_path + _T("\n\nfail to DeleteFile().");
					params->status = -1;
					TRACE(_T("result = %s\n"), params->result);
					return;
				}

				hFile = CreateFile(params->local_file_path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					SAFE_DELETE_ARRAY(buffer);
					SAFE_DELETE_ARRAY(total_result);

					InternetCloseHandle(hOpenRequest);
					InternetCloseHandle(hInternetConnect);
					InternetCloseHandle(hInternetRoot);

					params->result = _T("error=") + params->local_file_path + _T("\n\nfail to CreateFile().");
					params->status = -1;
					TRACE(_T("result = %s\n"), params->result);
					return;
				}
			}

			WriteFile(hFile, buffer, dwRead, &dwWritten, NULL);
			params->downloaded_size += dwWritten;

			//TRACE(_T("id = %d, downloaded_size = %d\n"), params->request_id, params->downloaded_size);
		}
	} while (dwRead != 0);

	total_result[dwTotalSize] = '\0';
	params->result = UTF8toCString(total_result);

	if (params->local_file_path.IsEmpty())
	{
		//UTF-8 with BOM으로 인코딩 된 파일의 경우 헤더에 EF BB BF가 붙는다. 제거하자.
		if (params->result.GetLength() >= 3 &&
			params->result[0] == 0xEF &&
			params->result[1] == 0xBB &&
			params->result[2] == 0xBF)
			params->result = params->result.Mid(3);
	}
	else
	{
		CloseHandle(hFile);
		params->result = _T("");
	}

	SAFE_DELETE_ARRAY(jsonData);
	SAFE_DELETE_ARRAY(buffer);
	SAFE_DELETE_ARRAY(total_result);

	InternetCloseHandle(hOpenRequest);
	InternetCloseHandle(hInternetConnect);
	InternetCloseHandle(hInternetRoot);

	params->elapsed = clock() - t0;
	//TRACE(_T("elapsed = %ld\n"), params->elapsed);
}

DWORD	GetURLFileSize(LPCTSTR pUrl)
{
	int			nFileSize = -1; 
	TCHAR		g_szHead[] = _T("Accept: */*\r\n\r\n");
	TCHAR		g_pBuffer[1025]; 
	CHttpFile	*pDownFile = NULL; 
	CInternetSession m_DownSession(_T("INIPayDownload"), 1, INTERNET_OPEN_TYPE_DIRECT); 
	
	TRY 
	{ 
		pDownFile = (CHttpFile*)m_DownSession.OpenURL(pUrl, 1, 
			INTERNET_FLAG_RELOAD | INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_DONT_CACHE, 
			g_szHead, -1L); 
	} 
	CATCH(CException, e) 
	{
		e->Delete(); 
		return nFileSize; 
	} 
	END_CATCH 
		

 	pDownFile->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, g_pBuffer, (unsigned long*)&nFileSize, NULL); 
 	nFileSize = _tstoi((TCHAR*)g_pBuffer); 

	pDownFile->Close();
	delete pDownFile;

	return nFileSize;
}

bool	DownloadFile(LPCTSTR pUrl, CString strFileName, bool bOverwrite /*= TRUE*/, HWND hWnd/*=NULL*/)
{
	// 인터넷 연결이 안된 경우 직접 접속을 시도하면 오류가 발생하므로 우선 체크해야 한다.
	//	if (!CheckInternetIsOnline())
	//		return FALSE;
	if (!IsNetwork(_T("www.naver.com"), 80))
		return FALSE;
	
	DWORD	dwSize = GetURLFileSize(pUrl);
	if (dwSize < 0)
		return FALSE;

	if (hWnd)
	{
		::SendMessage(hWnd, MESSAGE_DOWNLOAD_DATA, DOWNLOAD_SET_FILE_SIZE, dwSize);
	}


	FILE*	pFile = _tfopen(strFileName, _T("rb")); 
	
	// 파일이 존재하면...
	if (pFile)
	{ 
		fclose(pFile);
		
		if (bOverwrite)
			DeleteFile(strFileName);
		else
			return TRUE; 
	} 
	
	TCHAR		g_szHead[] = _T("Accept: */*\r\n\r\n");
	TCHAR*		sReadBuffer;
	CHttpFile	*pDownFile; 
	CInternetSession m_DownSession(_T("INIPayDownload"), 1, INTERNET_OPEN_TYPE_DIRECT); 
	
	TRY 
	{ 
		pDownFile = (CHttpFile*)m_DownSession.OpenURL(pUrl, 1, 
			INTERNET_FLAG_RELOAD | INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_DONT_CACHE, 
			g_szHead, -1L); 
	} 
	CATCH(CException, e) 
	{ 
		//e->ReportError(); 
		e->Delete(); 
		return FALSE; 
	} 
	END_CATCH 
		
	if (pDownFile == NULL)
		return FALSE; 
	
	int nPush = 0; 
	int nLength; 
	int nSum = 0; 
	
	pFile = _tfopen(strFileName, _T("wb"));

	if(pFile == NULL) 
	{ 
		pDownFile->Close(); 
		delete pDownFile; 
		
		return FALSE; 
	} 
	
#define READ_SIZE 1024 * 4

	sReadBuffer = new TCHAR[READ_SIZE];

	nLength = READ_SIZE;
	
	for(;nLength == READ_SIZE;) 
	{ 
		TRY 
		{ 
			nLength = pDownFile->Read(sReadBuffer, READ_SIZE); 
			//nLength = pDownFile->ReadHuge(sReadBuffer, READ_SIZE); 
		} 
		CATCH(CException, e) 
		{ 
			pDownFile->Close(); 
			
			delete pDownFile; 
			delete[] sReadBuffer;
			
			fclose(pFile); 
			
			//e->ReportError(); 
			e->Delete();
			return FALSE;
		} 
		END_CATCH 
			
			fwrite(sReadBuffer, 1, nLength, pFile); 
		
		nSum += nLength; 

		if (hWnd != NULL)
			::SendMessage(hWnd, MESSAGE_DOWNLOAD_DATA, DOWNLOAD_RECEIVED_SIZE, nSum);

	} 
	
	ASSERT(nSum == dwSize);
	
	pDownFile->Close(); 
	
	delete pDownFile; 
	delete[] sReadBuffer;
	
	fclose(pFile); 
	
	::SendMessage(hWnd, MESSAGE_DOWNLOAD_DATA, DOWNLOAD_COMPLETE_ONE, 0);

	return TRUE; 
} 

CString DownloadURLFile(CString sUrl, CString sLocalFileName, HWND hWnd/*=NULL*/)
{
	if (!IsNetwork(_T("www.naver.com"), 80))
		return _T("네트워크에 접속할 수 없습니다.");

#define HTTPBUFLEN    1024 * 1024 // Size of HTTP Buffer...
	TCHAR		*httpbuff;
	TCHAR		szCause[255];
	CString		Cause;

	Cause.Format(_T("TRUE"));

	TRY
	{
		CInternetSession mysession;
	CHttpFile* remotefile = (CHttpFile*)mysession.OpenURL(sUrl,1,INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD);

	ULONGLONG dwSize = remotefile->GetLength();
	if (hWnd != NULL)
	{
		::SendMessage(hWnd, MESSAGE_DOWNLOAD_DATA, (WPARAM)0, (LPARAM)dwSize);
		return _T("");
	}

	CFile myfile(sLocalFileName, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary);
	int numbytes;

	httpbuff = new TCHAR[HTTPBUFLEN];

	while (numbytes = remotefile->Read(httpbuff, HTTPBUFLEN))
		//while (numbytes = remotefile->ReadHuge(httpbuff, HTTPBUFLEN))
	{
		myfile.Write(httpbuff, numbytes);
		//myfile.WriteHuge(httpbuff, numbytes);

		if (hWnd != NULL)
			::SendMessage(hWnd, MESSAGE_DOWNLOAD_DATA, 1, numbytes);
	}

	delete[] httpbuff;
	}

		CATCH_ALL(error)
	{
		error->GetErrorMessage(szCause, 254, NULL);
		Cause.Format(_T("%s"), szCause);
	}
	END_CATCH_ALL;

	return (Cause);
}

//기본 브라우저로 설정된 브라우저 이름을 리턴하고 부가적으로 경로, 버전을 얻을 수 있다.
CString	get_default_browser_info(CString* pPath, CString* pVersion)
{
	CString ProgId;
	CString browser;
	CString path;
	CString version;

	get_registry_str(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\https\\UserChoice"), _T("ProgId"), &ProgId);

	if (ProgId.Find(_T("Chrome")) >= 0)			//ChromeHTML
		browser = _T("Google Chrome");
	else if (ProgId.Find(_T("Edge")) >= 0)		//FirefoxURL
		browser = _T("Microsoft Edge");
	else if (ProgId.Find(_T("Firefox")) >= 0)	//MSEdgeHTM
		browser = _T("Mozilla Firefox");
	else if (ProgId.Find(_T("IE.HTTP")) >= 0)	//IE.HTTP
		browser = _T("Internet Explorer");
	else if (ProgId.Find(_T("WhaleHTM")) >= 0)	//Naver Whale
		browser = _T("Naver Whale");
	else
		browser = ProgId;

	//"C:\Program Files\Google\Chrome\Application\chrome.exe" --single-argument %1
	//"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe" --single-argument %1
	//"C:\Program Files\Internet Explorer\iexplore.exe" %1

	CString section;
	section.Format(_T("SOFTWARE\\Classes\\%s\\shell\\open\\command"), ProgId);
	get_registry_str(HKEY_LOCAL_MACHINE, section, _T(""), &path);

	int pos = path.Find(_T(".exe\""));
	if (pos > 0)
	{
		path = path.Left(pos) + _T(".exe");
		path.Remove('\"');
	}
	if (pPath)
		*pPath = path;

	if (pVersion)
		*pVersion = get_file_property(path);

	return browser;
}

LONG IsExistRegistryKey(HKEY hKeyRoot, CString sSubKey)
{
	HKEY hkey = NULL;
	LONG nError = RegOpenKeyEx(hKeyRoot, sSubKey, 0, KEY_READ, &hkey);
	RegCloseKey(hkey);

	return nError;
}

//#ifndef _USING_V110_SDK71_
LONG get_registry_int(HKEY hKeyRoot, CString sSubKey, CString sEntry, DWORD *value)
{
	HKEY	hkey = NULL;
	LONG 	nError = RegOpenKeyEx(hKeyRoot, sSubKey, 0, KEY_ALL_ACCESS, &hkey);
	DWORD	buf_size = 256;
	TCHAR	buffer[256] = { 0, };
	LPVOID	lpMsgBuf;
	DWORD	dwType = REG_DWORD;

	if (nError == ERROR_SUCCESS)
	{
		if (hkey)
		{
			//WinXP SP3에서 RegGetValue()가 지원되지 않아 RegQueryValueEx()로 변경함.
			//nError = RegGetValue(hKeyRoot, sSubKey, sEntry, RRF_RT_DWORD, &dwType, value, &cbData);

			//dwType을 주지 않으면 실제 그 값이 0인 REG_DWORD 값임에도 불구하고 '0'이 되어 48이라는 값을 리턴하게 된다.
			nError = RegQueryValueEx(hkey, sEntry, NULL, &dwType, reinterpret_cast<LPBYTE>(value), &buf_size);

			if (nError == ERROR_SUCCESS)
			{
			}
			else
			{
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								nError,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL);
				//여기서는 굳이 메시지박스를 표시하지 않는다.
				//실제 해당 entry가 없을 수 있으며 없을 경우 "지정된 파일을 찾을 수 없습니다."와 같은 시스템 에러 메시지가 표시된다.
				//AfxMessageBox((LPCTSTR)lpMsgBuf, MB_ICONERROR);
				LocalFree(lpMsgBuf);
			}
		}
	}
	else
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						nError,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL);
		//AfxMessageBox((LPCTSTR)lpMsgBuf, MB_ICONERROR);
		TRACE(_T("GetRegistryString() error = %s\n"), (LPCTSTR)lpMsgBuf);
		LocalFree(lpMsgBuf);
	}
	
	RegCloseKey(hkey);
	
	return nError;
}

LONG get_registry_str(HKEY hKeyRoot, CString sSubKey, CString entry, CString *str)
{
	HKEY	hkey = NULL;
	DWORD	dwType = REG_SZ;
	TCHAR	buffer[1024] = { 0, };
	DWORD	dwBytes = sizeof(buffer);
	LPVOID	lpMsgBuf;
	
	LONG nError = RegOpenKeyEx(hKeyRoot, sSubKey, 0, KEY_ALL_ACCESS, &hkey);

	if (nError == ERROR_SUCCESS)
	{
		if (hkey)
		{
			//nError = RegQueryValueEx(hkey, entry, NULL, &dwType, (LPBYTE)buffer, &dwBytes);
			nError = RegQueryValueEx(hkey, entry, NULL, &dwType, (LPBYTE)buffer, &dwBytes);
			
			if (nError == ERROR_SUCCESS)
			{
			}
			else
			{
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								nError,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL);
				//AfxMessageBox((LPCTSTR)lpMsgBuf, MB_ICONERROR);
				TRACE(_T("GetRegistryString() error = %s\n"), (LPCTSTR)lpMsgBuf);
				LocalFree(lpMsgBuf);
			}
		}
	}
	else
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						nError,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL);
		//AfxMessageBox((LPCTSTR)lpMsgBuf, MB_ICONERROR);
		LocalFree(lpMsgBuf);
	}
	
	RegCloseKey(hkey);
	
	(*str) = CString(buffer);
	return nError;
}

LONG set_registry_int(HKEY hKeyRoot, CString sSubKey, CString entry, DWORD value)
{
	HKEY	hkey;
	DWORD	dwDesc;
	DWORD	lResult = 0;

	RegOpenKeyEx(hKeyRoot, sSubKey, 0, KEY_WRITE, &hkey);

	lResult = RegCreateKeyEx(hKeyRoot, sSubKey, 0,
		NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwDesc);

	if (lResult == ERROR_SUCCESS)
		lResult = RegSetValueEx(hkey, entry, NULL, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));

	RegCloseKey(hkey);

	return lResult;
}

LONG set_registry_str(HKEY hKeyRoot, CString sSubKey, CString entry, CString str)
{
	HKEY	hkey;
	LONG	lResult = ERROR_SUCCESS;
	DWORD	dwDesc;
	TCHAR	buffer[1000] = { 0, };
	
	ZeroMemory(buffer, sizeof(buffer));

	_stprintf(buffer, _T("%s\0"), str);
	int len = _tcslen(buffer);

	RegOpenKeyEx(hKeyRoot, sSubKey, 0, KEY_WRITE, &hkey);
	
	lResult = RegCreateKeyEx(hKeyRoot, sSubKey, 0,
		buffer, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwDesc);
	
	if (lResult == ERROR_SUCCESS)
		lResult = RegSetValueEx(hkey, entry, NULL, REG_SZ, (BYTE*)buffer, _tcslen(buffer) * sizeof(TCHAR));
	
	RegCloseKey(hkey);

	return lResult;
}
//#endif

//reg_path에 해당 value 항목이 존재하지 않으면 추가한다.
//"count"에 갯수가, 숫자 인덱스 항목들에 각 항목이 저장된 구조에만 사용 가능하다.
//추가된 인덱스를 리턴한다.
int add_registry(CWinApp* pApp, CString reg_path, CString value)
{
	CString item;
	int count = pApp->GetProfileInt(reg_path, _T("count"), 0);

	for (int i = 0; i < count; i++)
	{
		item = pApp->GetProfileString(reg_path, i2S(i), _T(""));
		if (value.CompareNoCase(item) == 0)
			return -1;
	}

	//존재하지 않는다면 추가한다.
	pApp->WriteProfileString(reg_path, i2S(count), value);
	pApp->WriteProfileInt(reg_path, _T("count"), count + 1);

	return count;
}

bool LoadBitmapFromFile(CBitmap &bmp, CString strFile)
{
	bmp.Detach();
	
	HANDLE handle = ::LoadImage(NULL,// AfxGetInstanceHandle(),
		strFile,
		IMAGE_BITMAP,
		0, 0,
		LR_LOADFROMFILE);
	
	if (!handle)
		return FALSE;
	
	bmp.Attach((HBITMAP)handle);
	
	return TRUE;
}

bool	SaveBitmapToTile(CBitmap* bmp, CString strFile, CWnd*	pWnd)
{
	bool				bResult = TRUE;
	CString				sMessage;
	BITMAPINFOHEADER    *pBMI ;
	BITMAPFILEHEADER	hdr;
	DWORD				dwTotal;              // total count of bytes 
	DWORD				cb;                   // incremental count of bytes 
	CPalette			pal;
	DWORD				dwWritten = 0;
	LPBYTE				hp;
	CClientDC			dc(pWnd);
	
	// Prepare for converting DDB to DIB
	UINT        nSize   = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 256);
	LOGPALETTE* pLP     = (LOGPALETTE*)new BYTE[nSize];
	pLP->palVersion     = 0x300;
	pLP->palNumEntries = (unsigned short)GetSystemPaletteEntries(
		dc.GetSafeHdc(), 0, 255,
		pLP->palPalEntry);
	
	// Create the palette
	pal.CreatePalette(pLP);
	
	// Free memory
	delete[] pLP;
	
	// Do convert DDB to DIB
	HANDLE hDib = DDBToDIB(bmp, BI_RGB, &pal);
	
	// Prepare Bitmap Info Header
	pBMI = (BITMAPINFOHEADER*)GlobalLock(hDib) ;
	int nColors = 0;
	if (pBMI->biBitCount <= 8)
	{
		nColors = (1 << pBMI->biBitCount);
	}
	
	// Prepare FileHeader
	hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 
	hdr.bfReserved1 = 0; 
	hdr.bfReserved2 = 0; 
	
	// Compute the size of the entire file. 
	hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
		pBMI->biSize + pBMI->biClrUsed
		* sizeof(RGBQUAD) + pBMI->biSizeImage); 
	
	// Compute the offset to the array of color indices. 
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
		pBMI->biSize + pBMI->biClrUsed 
		* sizeof (RGBQUAD); 
	
	// Create the .BMP file. 
	HANDLE hf = CreateFile(strFile, 
		GENERIC_READ | GENERIC_WRITE, 
		(DWORD) 0, 
		NULL, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		(HANDLE) NULL); 
	if (hf == INVALID_HANDLE_VALUE) 
	{
		sMessage = "Cannot create file";
		bResult = FALSE;
		goto CLEANUP;
	}
	
	// Copy the BITMAPFILEHEADER into the .BMP file. 
	if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
		(LPDWORD) &dwWritten,  NULL)) 
	{
		sMessage = "Cannot write to file";
		bResult = FALSE;
		goto CLEANUP;
	}
	
	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
	if (!WriteFile(hf, (LPVOID) pBMI, sizeof(BITMAPINFOHEADER) 
		+ pBMI->biClrUsed * sizeof (RGBQUAD), 
		(LPDWORD) &dwWritten, (NULL)))
	{
		sMessage = "Cannot write to file";
		bResult = FALSE;
		goto CLEANUP;
	}
	
	// Copy the array of color indices into the .BMP file. 
	dwTotal = cb = pBMI->biSizeImage; 
	hp = (LPBYTE)pBMI + (pBMI->biSize + nColors * sizeof(RGBQUAD)); 
	if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwWritten,NULL)) 
	{
		sMessage = "Cannot write to file";
		bResult = FALSE;
		goto CLEANUP;
	}
	
CLEANUP:;
		
		// free resources
		GlobalUnlock(hDib) ;
		GlobalFree(hDib) ;
		
		// Close the .BMP file. 
		if (hf != INVALID_HANDLE_VALUE)
		{
			if (!CloseHandle(hf))
			{
				sMessage = "Cannot close file";
				return FALSE;
			}
		}
		
		return bResult;
}

bool SaveRawDataToBmp(CString sBmpFile, BYTE* pData, int w, int h, int ch)
{
	CFile file;

	if (file.Open(sBmpFile, CFile::modeCreate | CFile::modeWrite) == false)
	{
		AfxMessageBox(_T("create bmp failed"));
		return false;
	}

	BYTE	zeros[8];
	DWORD	dw;

	file.Write("BM", 2);

	memset(zeros, 0, 8);
	file.Write(zeros, 8);

	dw = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	file.Write(&dw, 4);

	BITMAPINFO	bmpInfo;

	// Initialize permanent data in the bitmapinfo header.
	bmpInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biPlanes        = 1;
	bmpInfo.bmiHeader.biCompression   = BI_RGB;
	bmpInfo.bmiHeader.biXPelsPerMeter = 100;
	bmpInfo.bmiHeader.biYPelsPerMeter = 100;
	bmpInfo.bmiHeader.biClrUsed       = 0;
	bmpInfo.bmiHeader.biClrImportant  = 0;

	// Set a default window size.
	bmpInfo.bmiHeader.biWidth			= MAKE_MULTIPLY_U(w, 4);
	bmpInfo.bmiHeader.biHeight			= -h;
	bmpInfo.bmiHeader.biBitCount		= 8 * ch;
	bmpInfo.bmiHeader.biSizeImage		= sizeof(BYTE) * w * h * ch;

	file.Write(&bmpInfo, sizeof(BITMAPINFOHEADER));


	DWORD	width4;

	width4 = WIDTHSTEP4(w * bmpInfo.bmiHeader.biBitCount);
	file.Write(pData, sizeof(BYTE) * width4 * h);

	/////////////////////////////////////////////////////////////////////
	// BITMAPFILEHEADER에 bfSize를 기록한다.
	dw = (DWORD)(file.GetLength());
	file.Seek(2, CFile::begin);
	file.Write(&dw, sizeof(dw));
	file.Close();

	return true;
}

HANDLE DDBToDIB(CBitmap* bitmap, DWORD dwCompression, CPalette* pPal) 
{
	BITMAP            bm;
	BITMAPINFOHEADER    bi;
	LPBITMAPINFOHEADER     lpbi;
	DWORD            dwLen;
	HANDLE            hDIB;
	HANDLE            handle;
	HDC             hDC;
	HPALETTE        hPal;
	
	
	ASSERT(bitmap->GetSafeHandle());
	
	// The function has no arg for bitfields
	if(dwCompression == BI_BITFIELDS)
		return NULL;
	
	// If a palette has not been supplied use defaul palette
	hPal = (HPALETTE) pPal->GetSafeHandle();
	if (hPal==NULL)
		hPal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);
	
	// Get bitmap information
	bitmap->GetObject(sizeof(bm),(LPSTR)&bm);
	
	// Initialize the bitmapinfoheader
	bi.biSize        = sizeof(BITMAPINFOHEADER);
	bi.biWidth        = bm.bmWidth;
	bi.biHeight         = bm.bmHeight;
	bi.biPlanes         = 1;
	bi.biBitCount        = (unsigned short)(bm.bmPlanes * bm.bmBitsPixel) ;
	bi.biCompression    = dwCompression;
	bi.biSizeImage        = 0;
	bi.biXPelsPerMeter    = 0;
	bi.biYPelsPerMeter    = 0;
	bi.biClrUsed        = 0;
	bi.biClrImportant    = 0;
	
	// Compute the size of the  infoheader and the color table
	int nColors = 0;
	if(bi.biBitCount <= 8)
	{
		nColors = (1 << bi.biBitCount);
	}
	dwLen  = bi.biSize + nColors * sizeof(RGBQUAD);
	
	// We need a device context to get the DIB from
	hDC = ::GetDC(NULL);
	hPal = SelectPalette(hDC,hPal,FALSE);
	RealizePalette(hDC);
	
	// Allocate enough memory to hold bitmapinfoheader and color table
	hDIB = GlobalAlloc(GMEM_FIXED,dwLen);
	
	if (!hDIB){
		SelectPalette(hDC,hPal,FALSE);
		::ReleaseDC(NULL,hDC);
		return NULL;
	}
	
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	
	*lpbi = bi;
	
	// Call GetDIBits with a NULL lpBits param, so the device driver 
	// will calculate the biSizeImage field 
	GetDIBits(hDC, (HBITMAP)bitmap->GetSafeHandle(), 0L, (DWORD)bi.biHeight,
		(LPBYTE)NULL, (LPBITMAPINFO)lpbi, (DWORD)DIB_RGB_COLORS);
	
	bi = *lpbi;
	
	// If the driver did not fill in the biSizeImage field, then compute it
	// Each scan line of the image is aligned on a DWORD (32bit) boundary
	if (bi.biSizeImage == 0){
		bi.biSizeImage = ((((bi.biWidth * bi.biBitCount) + 31) & ~31) / 8) 
			* bi.biHeight;
		
		// If a compression scheme is used the result may infact be larger
		// Increase the size to account for this.
		if (dwCompression != BI_RGB)
			bi.biSizeImage = (bi.biSizeImage * 3) / 2;
	}
	
	// Realloc the buffer so that it can hold all the bits
	dwLen += bi.biSizeImage;
	handle = GlobalReAlloc(hDIB, dwLen, GMEM_MOVEABLE) ;
	if (handle != NULL)
		hDIB = handle;
	else
	{
		GlobalFree(hDIB);
		
		// Reselect the original palette
		SelectPalette(hDC,hPal,FALSE);
		::ReleaseDC(NULL,hDC);
		return NULL;
	}
	
	// Get the bitmap bits
	lpbi = (LPBITMAPINFOHEADER)hDIB;
	
	// FINALLY get the DIB
	bool bGotBits = GetDIBits(hDC, (HBITMAP)bitmap->GetSafeHandle(),
		0L,                      // Start scan line
		(DWORD)bi.biHeight,      // # of scan lines
		(LPBYTE)lpbi             // address for bitmap bits
		+ (bi.biSize + nColors * sizeof(RGBQUAD)),
		(LPBITMAPINFO)lpbi,      // address of bitmapinfo
		(DWORD)DIB_RGB_COLORS);  // Use RGB for color table
	
	if(!bGotBits)
	{
		GlobalFree(hDIB);
		
		SelectPalette(hDC,hPal,FALSE);
		::ReleaseDC(NULL,hDC);
		return NULL;
	}
	
	SelectPalette(hDC,hPal,FALSE);
	::ReleaseDC(NULL,hDC);
	return hDIB;
}

void	SetSystemTimeClock(WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMinute, WORD wSecond)
{
	SYSTEMTIME	lpSysTime;
	
	GetLocalTime(&lpSysTime);
	
	lpSysTime.wYear		= wYear;
	lpSysTime.wMonth	= wMonth;
	lpSysTime.wDay		= wDay;

// 	TIME_ZONE_INFORMATION TimeZoneInformation;
// 	GetTimeZoneInformation(&TimeZoneInformation);
// 	TimeZoneInformation.Bias;
// 	
// 	if (wHour >= 9)
// 		wHour -= 9;
// 	else
// 	{
// 		wHour = wHour - 9 + 24;
// 		lpSysTime.wDay	-= 1;
// 	}
	
	lpSysTime.wHour		= wHour;
	lpSysTime.wMinute	= wMinute;
	lpSysTime.wSecond	= wSecond;
	
	SetLocalTime(&lpSysTime);
}



//다음 파일의 인덱스 리턴
//sCurrentFile = camera02.jpg, then this function will return 3
int			GetNextFileIndex(CString sCurrentFile)
{
	TCHAR	ch;
	CString fileTitle = get_part(sCurrentFile, fn_title);
	CString sIndex = _T("");

	for (int i = fileTitle.GetLength() - 1; i >= 0; i--)
	{
		ch = fileTitle.GetAt(i);
		if (ch >= '0' && ch <= '9')
			sIndex = ch + sIndex;
		else
			break;
	}

	if (sIndex == "")
		return 0;

	int nIndex = _ttoi((TCHAR*)(LPCTSTR)sIndex);
	return nIndex + 1;
}


// nReturnType
// 0 : Full Path Name	(c:\abc.txt)
// 1 : Only File Name	(abc.txt)
// 2 : Only File Title	(abc)
CString		GetMostRecentFile(CString sFolder, CString sWildCard /*= "*.*" */, int nReturnType /* = 2 */)
{
	CFileFind	FileFind;
	bool		bWorking;
	CString		sMostRecentFile = _T("");
	
	if (sFolder.Right(1) != "\\")
		sFolder += "\\";
	
	bWorking = FileFind.FindFile(sFolder + sWildCard);
	
	while (bWorking)
	{
		bWorking = FileFind.FindNextFile();
		
		if (!FileFind.IsDots() && !FileFind.IsDirectory())
		{
			if (sMostRecentFile == "")
				sMostRecentFile = FileFind.GetFilePath();
			if (FileFind.GetFilePath() > sMostRecentFile)
				sMostRecentFile = FileFind.GetFilePath();
		}
	}
	
	if (nReturnType == 1)
		return get_part(sMostRecentFile, fn_name);
	else if (nReturnType == 2)
		return get_part(sMostRecentFile, fn_title);
	
	return sMostRecentFile;
}

CString		GetMostRecentDateFile(CString sFolder, CString sWildCard /*= "*.*"*/)	//가장 최근 날짜 파일명 리턴
{
	CFileFind	FileFind;
	bool		bWorking;
	CString		sMostRecentDateFile = _T("");
	CString		sfile;
	CTime		tFile;
	
	//과거 시간 최대는 1970년 1월 1일 0시가 아닌 9시다.
	//왜냐면 우리나라가 GMP+9이므로. 1970-1-1 8:59:59 이것도 assert fail 난다.
	CTime		tRecent(1970,1,1,9,0,0);


	
	if (sFolder.Right(1) != "\\")
		sFolder += "\\";
	
	bWorking = FileFind.FindFile(sFolder + sWildCard);
	
	while (bWorking)
	{
		bWorking = FileFind.FindNextFile();
		
		if (!FileFind.IsDots() && !FileFind.IsDirectory())
		{
			FileFind.GetCreationTime(tFile);
			
			if (tFile > tRecent)
			{
				tRecent = tFile;
				sMostRecentDateFile = FileFind.GetFilePath();
			}
		}
	}
	
	return sMostRecentDateFile;
}

#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

bool GetNetworkInformation(CString sTargetDeviceDescription, NETWORK_INFO* pInfo)
{
	bool				result = false;
	IP_ADAPTER_INFO*	pAdapterInfo = NULL;
	IP_ADAPTER_INFO*	pAdapter = NULL;
	ULONG				ulOutBufLen = 0;
	TCHAR				zeroIP[] = _T("0.0.0.0");

	USES_CONVERSION;

	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
	if (pAdapterInfo == NULL)
	{
		TRACE(_T("Error allocating memory needed to call GetAdaptersinfo\n"));
		return result;
	}

	if (GetAdaptersInfo(NULL, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);

		if (pAdapterInfo == NULL)
		{
			TRACE(_T("Error allocating memory needed to call GetAdaptersinfo\n"));
			return result;
		}
	}

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;

 		while (pAdapter)
		{
			_tcscpy(pInfo->sDescription, CString(pAdapter->Description));

			//if (pAdapter->Type == MIB_IF_TYPE_ETHERNET && CString(pInfo->sDescription).Find(sTargetDeviceDescription) >= 0)
			{
				_tcscpy(pInfo->sIPAddress, CString(pAdapter->IpAddressList.IpAddress.String));
				_tcscpy(pInfo->sGateway, CString(pAdapter->GatewayList.IpAddress.String));
				_tcscpy(pInfo->sSubnetMask, CString(pAdapter->IpAddressList.IpMask.String));

				//TCHAR sMacAddress[16] 이므로 구분자를 넣어주면 안된다.
				//main에서 Functions.h의 get_mac_address_format()함수로 구분자를 넣어서 표시할 것.

				_stprintf(pInfo->sMacAddress, _T("%02X%02X%02X%02X%02X%02X"),
					pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2], pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);

				TRACE(_T("pInfo->sDescription = %s, sIPAddress = %s, sGateway = %s, sSubnetMask = %s, sMacAddress = %s\n"),
					pInfo->sDescription,
					pInfo->sIPAddress,
					pInfo->sGateway,
					pInfo->sSubnetMask,
					pInfo->sMacAddress);

#ifdef LMM_SERVICE_SAMSUNGLIFE
				result = true;
				break;
#else
				//20241223 scpark 여러개의 네트워크 카드중에서 실제 연결된 네트워크 카드의 정보를 얻어야하므로
				//아래의 4개 값이 0이 아닌 경우가 실제 연결된 네트워크 정보임.
				//단, 삼성생명 LMM에서는 mgrid를 사원번호_MAC 으로 정했으므로
				//어떤 사원이 이더넷을 쓰다가 와이파이로 변경하면 아래 코드에 의해 mgrid 값이 달라질 수 있다.
				//단, 삼성생명 마지막 패치에 반입된 LMM_SSL_Launcher.exe는 20240905이므로 아래 코드가 추가된 20241223보다 훨씬 이전이므로
				//LMM_SSL_Launcher.exe는 네트워크 연결 방식과 관계없이 무조건 0번 카드의 정보를 리턴하게 되어 있다.
				if (_tcscmp(pInfo->sIPAddress, zeroIP) != 0 &&
					_tcscmp(pInfo->sGateway, zeroIP) != 0 &&
					_tcscmp(pInfo->sSubnetMask, zeroIP) != 0 &&
					_tcscmp(pInfo->sMacAddress, _T("")) != 0)
				{
					result = true;
					break;
				}
#endif
			}

			pAdapter = pAdapter->Next;
		}
	}

	if (pAdapterInfo)
		free(pAdapterInfo);

	return result;
}

CString get_my_ip()
{
	NETWORK_INFO ni;
	GetNetworkInformation(_T(""), &ni);
	return ni.sIPAddress;
}

CString	get_mac_addres(bool include_colon)
{
	NETWORK_INFO netInfo;
	CString mac;

	if (GetNetworkInformation(_T(""), &netInfo))
	{
		mac = CString(netInfo.sMacAddress);

		if (include_colon)
			mac = get_mac_address_format(mac);
	}

	return mac;
}

CString get_ip_error_string(DWORD error_code)
{
	//auto e = GetLastError();
	DWORD buf_size = 1024;
	WCHAR buf[1024] = { 0, };
	GetIpErrorString(error_code, buf, &buf_size);

	return CString(buf);
}

bool port_is_open(const std::string& address, int port)
{
	return false;
}

bool GetNICAdapterList(IP_ADAPTER_INFO* pAdapters, int& nTotal, int nMax /*= 10*/)
{
	PIP_ADAPTER_INFO	AdapterInfo;
	DWORD	dwBufLen = sizeof(AdapterInfo);
	CString sDesc, sIP, sMac;
	CString str;

	nTotal = 0;

	AdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));

	if (AdapterInfo == NULL)
	{
		//printf("Error allocating memory needed to call GetAdaptersinfo\n");
		return false;
	}

	// Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen     variable
	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		AdapterInfo = (IP_ADAPTER_INFO *) malloc(dwBufLen);
		if (AdapterInfo == NULL)
		{
			//printf("Error allocating memory needed to call GetAdaptersinfo\n");
			return false;
		}
	}

	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR)
	{
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;// Contains pointer to current adapter info
		
		do
		{
			memcpy(&pAdapters[nTotal++], pAdapterInfo, sizeof(IP_ADAPTER_INFO));
/*
			sMac.Format("%02X-%02X-%02X-%02X-%02X-%02X",
				pAdapterInfo->Address[0], pAdapterInfo->Address[1],
				pAdapterInfo->Address[2], pAdapterInfo->Address[3],
				pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
			str.Format("%s\nAddr: %s, MAC: %s\n", pAdapterInfo->Description, pAdapterInfo->IpAddressList.IpAddress.String, sMac);
			AfxMessageBox(str);

			printf("\n");
*/
			if (nTotal >= nMax)
				break;

			pAdapterInfo = pAdapterInfo->Next;        
		}
		while(pAdapterInfo);                        
	}

	free(AdapterInfo);
	
	return true;
/*
	for (int i = 0; i < nTotal; i++)
	{
		sDesc	= Adapters[i].Description;
		sIP		= m_Adapters[i].IpAddressList.IpAddress.String;
		sMac.Format("%02X-%02X-%02X-%02X-%02X-%02X",
			m_Adapters[i].Address[0], m_Adapters[i].Address[1],
			m_Adapters[i].Address[2], m_Adapters[i].Address[3],
			m_Adapters[i].Address[4], m_Adapters[i].Address[5]);


		//if (sIP != "0.0.0.0")

		//if (sDesc.Find(""))

		m_List.AddItem(I2S(i), sDesc, sIP, sMac, NULL);

	}
*/
}

void EncryptString(CString& ToCode, TCHAR* key)
{
	//TCHAR key[] = "ai^j$l*m@u^"; // change this to be your key
	// Take the supplied string and add subsequent letters of the key (in ascii value) to each letter
	unsigned int j = 0;
	int len = _tcslen(key);

	for(int i = 0; i < ToCode.GetLength(); i++)
	{
		ToCode.SetAt(i, (ToCode.GetAt(i) + key[j++]));
		
		if(j == _tcslen(key))
		{
			j = 0;
		}
	}

/*	CString RetStr,AddStr;
	int i,max;
	unsigned short asc;
	unsigned TCHAR c;
	max = (unsigned int)ToCode.GetLength();
	for(i=0;i<max;i++)
	{
		c = ToCode[i];
		asc = c;//(unsigned int)c;
		if(asc>47 && asc<58)
		{
			RetStr+=c;//Interim[(int)i];
		}
		else if(asc>64 && asc<91)
		{
			RetStr+=c;//Interim[(int)i];
		}
		else if(asc>96 && asc<123)
		{
			RetStr+=c;//Interim[(int)i];
		}
		else if(asc==32)
		{
			RetStr+="+";
		}
		else
		{
			AddStr.Format("%%%2x",asc);
			int iv = (int)AddStr.GetAt(1);
			if((int)AddStr.GetAt(1)==32)
			{
				AddStr.SetAt(1,'0');
			}
			RetStr+=AddStr;
		}
	}
	return RetStr;
*/
}

void DecryptString(CString& ToCode, TCHAR* key)
{
	//TCHAR key[] = "ai^j$l*m@u^"; // change this to be your key
	// Take the supplied string and subtract subsequent letters of the key (in ascii value) from each letter
	
	unsigned int j = 0;
	
	for(int i = 0; i < ToCode.GetLength(); i++)
	{
		ToCode.SetAt(i, (ToCode.GetAt(i) - key[j++]));
		
		if(j == _tcslen(key))
		{
			j = 0;
		}
	}

/*	CString RetStr,AddStr;
	int i,max;
	unsigned short asc;
	unsigned TCHAR c;
	max = (unsigned int)ToCode.GetLength();
	for(i=0;i<max;)
	{
		c = ToCode[i];
		asc = c;//(unsigned int)c;
		if(asc==37)
		{
			AddStr=ToCode.Mid(i+1,2);
			i+=3;
			sscanf((LPCTSTR)AddStr,"%2x",&asc);
			RetStr+=(TCHAR)asc;
		}
		else if(asc==43)
		{
			RetStr += ' ';
			i++;
		}
		else
		{
			RetStr += c;
			i++;
		}
	}
	return RetStr;
*/
}

bool FileEncryption(CString sSource, bool bShowErrorMessage /*= TRUE*/)
{
	HANDLE	hfile,hmapObj;
	DWORD filesz;
	
	TCHAR *c;

	hfile=CreateFile(sSource, GENERIC_READ|GENERIC_WRITE, 0, NULL,OPEN_EXISTING, 0, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return FALSE;

	hmapObj=CreateFileMapping(hfile,NULL,PAGE_READWRITE,0,0,NULL);
	if (hmapObj == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hfile);
		return FALSE;
	}
	
	c=(TCHAR*)MapViewOfFile(hmapObj,FILE_MAP_READ|FILE_MAP_WRITE,0,0,0);
	
	filesz = GetFileSize(hfile,NULL);
	if (filesz == 0)
	{
		CloseHandle(hfile);
		CloseHandle(hmapObj);
		return FALSE;
	}

	if (filesz == 0xFFFFFFFF)
	{
		DWORD dwError = GetLastError();
		if (bShowErrorMessage)
		{
			CString sMsg;
			sMsg.Format(_T("파일 열기 실패 : \n\n암호화를 위해 %s 파일을 열 수 없습니다."), sSource);
			AfxMessageBox(sMsg);
		}

		UnmapViewOfFile((void*)c);
		CloseHandle(hmapObj);
		CloseHandle(hfile);

		return FALSE;
	}
	
	for(DWORD i(0);i<filesz;i++){
		
		if (int(c[i])>=128)
			c[i]=TCHAR(int(c[i])-128);
		else
			c[i]=TCHAR(int(c[i])+128);
		
	}	
	
	UnmapViewOfFile((void*)c);
	CloseHandle(hmapObj);
	CloseHandle(hfile);
	
	return TRUE;
}

CWnd* FindWindowByCaption(CString sCaption, bool bMatchWholeWord/* = FALSE*/)
{
	CWnd*	pWnd;
	CString	sText;
	CString	sCaptionString;
	
	pWnd = CWnd::GetDesktopWindow()->GetTopWindow();
	
	while ((pWnd = pWnd->GetNextWindow()) != NULL)
	{
		//pWnd->IsKindOf(RUNTIME_CLASS(CDialog))
		pWnd->GetWindowText(sText);
		
		sCaptionString = sText;
		sCaptionString.TrimLeft();
		sCaptionString.TrimRight();
		//sCaptionString.MakeLower();

		TRACE(_T("caption = %s\n"), sText);
		
		if (bMatchWholeWord)
		{
			if (sCaptionString == sCaption)
				return pWnd;
		}
		else
		{
			//TRACE(_T("%s\n"), sCaptionString);
			if (sCaptionString.Find(sCaption) >= 0)
				return pWnd;
		}
	}
	
	return NULL;
}
/*
HINSTANCE FindExecutableEx(LPCTSTR lpFile, LPCTSTR lpDirectory, LPTSTR lpResult)   
{   
    TCHAR drive[_MAX_DRIVE];   
    TCHAR dir[_MAX_DIR];   
    TCHAR dir1[_MAX_DIR];   
    TCHAR file[_MAX_FNAME];   
    TCHAR ext[_MAX_EXT];   
   
    ZeroMemory(lpResult, MAX_PATH);   
    HINSTANCE hinstance = FindExecutable(lpFile, lpDirectory, lpResult);   
    lpResult[lstrlen(lpResult)] = ' ';   
   
    _tsplitpath_s(lpResult, drive, _MAX_DRIVE, dir, _MAX_DIR, file, _MAX_FNAME, ext, _MAX_EXT);   
       
    //search for : in dir name   
    LPSTR p = _tcschr(dir, ':');   
    if(p != NULL)   
    {   
        --p;   
        dir[p-dir] = 0;   
		_tsplitpath_s(dir, NULL, _MAX_DRIVE, dir1, _MAX_DIR, file, _MAX_FNAME, ext, _MAX_EXT);
        p = strchr(ext, ' ');   
        if(p)   
            ext[p-ext] = 0;   
        _tmakepath_s(lpResult, _MAX_PATH, drive, dir1, file, ext);   
    }   
   
    return hinstance;   
}   
*/
ULONGLONG get_disk_free_size(CString sDrive)
{
	TCHAR Drive[10];
	ULARGE_INTEGER	m_lFreeBytesAvailableToCaller;    
	ULARGE_INTEGER	m_lTotalNumberOfBytes;
	ULARGE_INTEGER	m_lTotalNumberOfFreeBytes;
	
	if (sDrive.IsEmpty())
		sDrive = GetCurrentDirectory();

	if (sDrive.GetLength() == 1)
		_stprintf(Drive, _T("%s:\\"), sDrive);
	else
		_stprintf(Drive, _T("%s"), sDrive);

	if (GetDriveType(Drive) == DRIVE_CDROM)
		return 0;
	
	int res =  GetDiskFreeSpaceEx(Drive, 
		&m_lFreeBytesAvailableToCaller, 
		&m_lTotalNumberOfBytes, 
		&m_lTotalNumberOfFreeBytes);

	return m_lTotalNumberOfFreeBytes.QuadPart;
}

ULONGLONG get_disk_total_size(CString sDrive)
{
	TCHAR			Drive[10];
	ULARGE_INTEGER	m_lFreeBytesAvailableToCaller;    
	ULARGE_INTEGER	m_lTotalNumberOfBytes;
	ULARGE_INTEGER	m_lTotalNumberOfFreeBytes;
	
	if (sDrive.GetLength() == 1)
		_stprintf(Drive, _T("%s:\\"), sDrive);
	else
		_stprintf(Drive, _T("%s"), sDrive);
	
	if (GetDriveType(Drive) == DRIVE_CDROM)
		return 0;

	int res =  GetDiskFreeSpaceEx(Drive, 
		&m_lFreeBytesAvailableToCaller, 
		&m_lTotalNumberOfBytes, 
		&m_lTotalNumberOfFreeBytes);

	return m_lTotalNumberOfBytes.QuadPart;
}

//
/*
CString	GetDiskSizeString(CString sDrive, int nfDigit)
{
	CString str;

	str.Format(_T("%.1f GB / %.1f GB"),
				 GetDiskFreeSize(sDrive[0]) / 1024.0 / 1024.0 / 1024.0,
				GetDiskTotalSize(sDrive[0]) / 1024.0 / 1024.0 / 1024.0);
	return str;
}
*/

CString	GetCommaString(CString sString, CString sComma)
{
	// 1,231,014.958 GB
	int		nDotPos = sString.Find(_T("."));
	CString sIntPart;
	CString	sFinal = _T("");
	
	if (nDotPos > 0)	// 실수인 경우
	{
		sFinal		= sString.Mid(nDotPos);
		sIntPart	= sString.Left(nDotPos);
	}
	else
	{
		sIntPart	= sString;
	}
	
	//맨 끝 숫자부터 하나씩 새 문자열에 추가시킨다.
	//3자리 될 때마다 콤마를 넣어준다.
	for (int i = 0; i < sIntPart.GetLength(); i++)
	{
		sFinal.Insert(0, sIntPart.GetAt(sIntPart.GetLength() - 1 - i));
		
		if (((i + 1) % 3 == 0) &&				//3째 자리마다
			 (i != sIntPart.GetLength() - 1))
			sFinal.Insert(0, sComma);
	}

//	if (sFinal.Right(1) == sComma)
//		sFinal = sFinal.Left(sFinal.GetLength() - 1);
	
	
	return sFinal;
	/*
	TCHAR	sSeparator[1];
	
	  sprintf(sSeparator, "%s", sComma);
	  
		static	NUMBERFMT nFmt = { 0, 0, 3, ".", sSeparator, 0 };           
		TCHAR	szOut[100];
		
		  GetNumberFormat (NULL, NULL, (LPCTSTR)sString, &nFmt, szOut, 100);
		  
			return szOut;
	*/
}

//value가 3자리이고 total_digits가 7이면 4개의 0을 앞에 붙여서 리턴한다.
CString i2S(int64_t nValue, bool bComma, bool fill_zero, int total_digits)
{
	CString str;
	CString str_fill;

	if (fill_zero)
	{
		str.Format(_T("%I64d"), nValue);
		str_fill.Format(_T("%%0%dI64d"), MAX(0, total_digits));
		str.Format(str_fill, nValue);
	}
	else
	{
		str.Format(_T("%I64d"), nValue);
	}

	if (bComma)
		return GetCommaString(str);

	return str;
}

//convert to hexa-decimal
CString i2HS(int64_t nValue, bool bCapital)
{
	CString str;

	str.Format((bCapital ? _T("0x%08X") : _T("0x%08x")), nValue);

	return str;
}

CString d2S(double dValue, bool bComma, int nfDigit)
{
	CString str;
	CString sPrecision;

	if (nfDigit >= 0)
	{
		sPrecision.Format(_T("%%0.%df"), nfDigit);
		str.Format(sPrecision, dValue);
	}
	else
	{
		str.Format(_T("%f"), dValue);
	}

	if (bComma)
		return GetCommaString(str);

	return str;
}

//IPv4 문자열을 숫자로 바꾸는 범용 코드이므로 버전 문자열 등 일반 문자열 비교에 사용하지 말것.
//버전 문자열 등은 각 자릿수 구성이 다를 수 있으므로 사용할 수 없음.
//문자열 비교가 필요하다면 compare_string()을 사용할 것.
uint32_t IP2int(CString IP)
{
	std::deque<CString> token;
	get_token_string(IP, token, '.', false);

	if (token.size() != 4)
	{
		ASSERT(token.size() == 4);
		return 0;
	}

	uint32_t token_int[4];

	for (int i = 0; i < 4; i++)
		token_int[i] = _ttoi(token[i]);

	return (uint32_t)((token_int[0] << 24) + (token_int[1] << 16) + (token_int[2] << 8) + token_int[3]);
}

int HexaStringToInt(CString str)
{
	if (str.Left(2) == _T("0x"))
		return (int)(_tcstol(str, NULL, 0));

	return (int)(_tcstol(str, NULL, 16));
}

CString	GetByteString(uint8_t* bt, int n, bool upper /*= true*/, bool prefix /*= true*/)
{
	int		i;
	CString str = _T("");
	CString sBt;

	for (i = 0; i < n; i++)
	{
		sBt.Format((upper ? _T("%s%02X") : _T("%s%02x")), (prefix ? "0x" : ""), bt[i]);
		str = str + sBt + _T(" ");
	}

	str.TrimRight();
	return str;
}

// 사용법
//char data[6] = {0x30,0x41,0x00,0x00,0x00,0xE5};
//CString str = BinaryToHexString((BYTE*)data, 6);
CString BinaryToHexString(BYTE* pData, int length, TCHAR separator)
{
	ASSERT(pData);
	ASSERT(length >= 0);
	ASSERT(!isalpha(separator));

	if (0 == length)
		return _T("");

	CString result;
	for (int i = 0; i < length; ++i)
	{
		if (0 == i)
		{
			result.AppendFormat(_T("%02X"), pData[i]);
		}
		else
		{
			result.AppendFormat(_T("%c%02X"), separator, pData[i]);
		}
	}

	return result;
}


//소수점 자릿수 리턴.
//0.123456789 을 넘겨줘도 d값은 그대로 넘겨받으나
//sprintf에서 0.123457로 변경된다.(소수점 6자리로만 처리된다.)
//우선 이점을 주의하고 많은 자리수를 굳이 정확히 리턴해야 한다면
//코드를 좀 더 보완하자.
int	getPrecision(double d, bool bExceptZero)
{
	int i;
	char str[128] = {0};
	int	nPrecision = 0;

	//12.305600
	sprintf(str, "%f\0", d);
	for (i = strlen(str) - 1; i >= 0; i--)
	{
		if (str[i] == '.')
		{
			return nPrecision;
		}
		else if (bExceptZero && str[i] == '0')
		{
			continue;
		}
		else
		{
			nPrecision++;
			//0이 아닌 자릿수를 한번 만나면 그 후로는 유효 0이므로 카운트해야 한다.
			bExceptZero = false;
		}
	}

	return nPrecision;
}
/*
template<class T> CString getBinaryString(T number, bool blank)
{
	int i;
	std::vector<TCHAR> result;
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
			str = " " + str;
	}

	str.Trim();

	return str;
}
*/
unsigned __int64 binaryStringToInt64(char *str, int len)
{
  unsigned __int64 i = 0;
  int count = 0;

  while (count < len)
    i = (i << 1) | (str[count++] - '0');

  return i;
}

unsigned int getBitsValueFromInt64(uint8_t* bt, int num_of_bytes, int startbit, int bit_length)
{
	unsigned __int64 reverse = 0;
	unsigned __int64 btdata;
	unsigned __int64 mult;

	int		i = 0;
	CString	str, btstr;

	_tprintf(_T("source  = "));
	while (i < num_of_bytes)
	{
		_tprintf(_T("%02X "), bt[i]);

		if (i == 0)
			reverse = bt[i];
		else
		{
			btdata = bt[i];
			mult = btdata << (8*i);
			reverse = reverse + mult;
		}
		i++;
	}

	_tprintf(_T("\nreverse = %I64X\n"), reverse);

	int nbits = 64 - startbit;
	char *mask = new char[nbits];
	memset(mask, '0', nbits);
	
	for (i = 0; i < bit_length; i++)
		mask[nbits-1-i] = '1';

	_tprintf(_T("reverse origin  = %s\n"), get_binary_string(reverse, true));
	reverse = reverse >> startbit;
	_tprintf(_T("reverse shifted = %s\n"), get_binary_string(reverse, true));

	unsigned __int64 imask = binaryStringToInt64(mask, nbits);
	delete [] mask;

	return (reverse & imask);
}

unsigned int parse_can_data(unsigned char* data,unsigned int startbit,unsigned int length)
{
	unsigned int area;
	unsigned long long value = ((unsigned long long)data[0]<<56) | ((unsigned long long)data[1]<<48) | ((unsigned long long)data[2]<<40) | ((unsigned long long)data[3]<<32) | ((unsigned long long)data[4]<<24) | ((unsigned long long)data[5]<<16) | ((unsigned long long)data[6]<<8) | ((unsigned long long)data[7]);
	if(length == 0) return 0;
	make_area(area,length);
	return (unsigned int)extract_bits(value, area, startbit);
}

//기능 : unsigned 값을 signed 범위로 변경한다.
//ex1. 0 ~ 99 범위를 가진 unsigned 데이터 타입을 signed로 변경할 경우
//signed의 범위는 -50 ~ +49일 것이다.
//53이라는 값은 53 - 100 = -47이 된다.
//signed의 max인 49보다 작거나 같은 값은 그냥 리턴
//ex2. 0 ~ 3 범위를 가진 2bit 데이터 타입을 signed로 변경할 경우
//signed의 범위는 -2 ~ +1일 것이다.
//3이라는 값은 3 - 4 = -1이 된다.
//signed의 max인 1보다 작거나 같은 값은 그냥 리턴
int getSignedFromUnsigned(unsigned value, int bit_length)
{
	int range_max = pow(2.0, bit_length);
	int signed_max = range_max / 2 - 1;

	if (value <= signed_max)
		return value;

	return (value - range_max);
}

//dlg의 OnInitDialog()에서 이 함수를 호출할 경우
//예전에는 잘 적용되었으나 윈10의 IME 방식이 변경되었는지
//OnInitDialog()에 넣으면 적용되지 않는다.
//SetTimer나 UI 버튼을 추가하여 변경하면 잘 동작함.
//단, OnInitDialog()에서 타이머를 100ms와 같이 너무 짧게 주면 변경 안됨. 넉넉히 500ms 이상 줄 것.
void IME_Convert_To_NativeCode(HWND hWnd, bool bNative)
{
	//아래 주석처리된 블럭은 이전 방식이고
	//맨 아래 3줄은 간략히 수정된 방식. 둘 다 정상 동작함.
	/*
	HIMC hIMC = ImmGetContext(hWnd);
	DWORD dwConv, dwSent;
	DWORD dwTemp;
	
	ImmGetConversionStatus(hIMC,&dwConv,&dwSent);
	
	dwTemp = dwConv & ~IME_CMODE_LANGUAGE;
	
	if (bNative)
		dwTemp |= IME_CMODE_NATIVE;
	else
		dwTemp |= IME_CMODE_ALPHANUMERIC;
	
	dwConv = dwTemp;
	ImmSetConversionStatus(hIMC,dwConv,dwSent);
	
	ImmReleaseContext(hWnd, hIMC);
	*/
	HIMC himc=ImmGetContext(hWnd);
    ImmSetConversionStatus(himc, (bNative ? IME_CMODE_NATIVE : IME_CMODE_ALPHANUMERIC), IME_SMODE_CONVERSATION);
    ImmReleaseContext(hWnd,himc);    
	
}

void HideIMM(HWND hwnd)
{
	HWND hwndIMM;
//	if (_bHideIMM) 
	{
		hwndIMM = (HWND)ImmGetDefaultIMEWnd(hwnd);
		if (hwndIMM == NULL) return;
		//::ShowWindow(hwndIMM, SW_HIDE);
		PostMessage(hwndIMM, WM_CLOSE, 0, 0);
		//DestroyWindow(hwndIMM);
	}
}

bool IsLeapYear(int nYear)
{
	// 윤년이되려면
	// 1. 4로 나누어 나머지가 없어야 하고
	// 2. 100으로 나누어떨어지지 않거나,
	// 400으로도 나누어떨어져야 함
	
	if ((nYear % 4 == 0) && ((nYear % 100 != 0) || (nYear % 400 == 0)))
		return TRUE;
	
	return FALSE;
}

void sort_like_explorer(std::deque<CString>* dq, bool compare_only_filename)
{
	sort_like_explorer(dq->begin(), dq->end(), compare_only_filename);
}

void sort_like_explorer(std::deque<CString>::iterator _first, std::deque<CString>::iterator _last, bool compare_only_filename)
{
	bool only_filename = compare_only_filename;
	std::sort(_first, _last,
		[only_filename](CString a, CString b)
	{
		if (only_filename)
		{
			a = get_part(a, fn_name);
			b = get_part(b, fn_name);
		}
		return !is_greater_with_numeric(a, b);
	}
	);
}

void FindAllFiles(CString sFolder, std::deque<CString> *dqFiles, CString sNameFilter, CString sExtFilter,
				  bool bRecursive, bool include_folder, CString sExceptStr, bool auto_sort)
{
	int				i;
	CTime			tTime;
	CString			sfile;
	CString			sFilename;
	CFileFind		finder;

	if (sNameFilter == "")
		sNameFilter = _T("*");

	//필터 관련
	std::deque<CString>	dqExtFilter;

	//제외 항목을 위한 변수들
	std::deque<CString>	dqExcepts;
	CString			sToken;
	int				curPos;

	if (sExtFilter == "")
		sExtFilter = _T("*");

	//필터와 제외 문자열은 대소문자를 구분하지 않는다.
	curPos = 0;
	sToken = sExtFilter;
	sExtFilter.MakeLower();
	dqExtFilter.clear();

	//확장자는 "jpg"와 같이 dot가 없이 비교한다.
	//확장자에 * 또는 dot가 있다면 제거해준다.
	sExtFilter.Replace(_T("*"), _T(""));
	sExtFilter.Replace(_T("."), _T(""));
	while (true)
	{
		sToken = sExtFilter.Tokenize(_T(";"), curPos);
		if (sToken == "")
			break;
		dqExtFilter.push_back(sToken);
	}

	curPos = 0;
	sToken = sExceptStr;
	sExceptStr.MakeLower();
	dqExcepts.clear();
	if (sExceptStr != "")
	{
		while (true)
		{
			sToken = sExceptStr.Tokenize(_T(";"), curPos);
			if (sToken == "")
				break;

			dqExcepts.push_back(sToken);
		}
	}

	if (sFolder.Right(1) != "\\")
		sFolder += _T("\\");

	//recursive까지 고려하여 우선은 모든 파일을 대상으로 해야 한다.
	bool bWorking = finder.FindFile(sFolder + sNameFilter);

	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		sfile = finder.GetFilePath();
		sFilename = get_part(sfile, fn_name).MakeLower();
		
		if (finder.IsDots())
			continue;

		//검색 대상인 확장자 파일인지 검사한다.
		bool ext_matched = false;

		if (dqExtFilter.size() == 0 || find_index(dqExtFilter, get_part(sFilename, fn_ext).MakeLower()) >= 0)
			ext_matched = true;

		//제외할 문자열이 포함되어 있지 않은 파일들만 리스트에 추가한다.
		bool excepted = false;

		for (i = 0; i < dqExcepts.size(); i++)
		{
			if (sFilename.Find(dqExcepts[i]) >= 0)
			{
				excepted = true;
				break;
			}
		}

		if (finder.IsDirectory())
		{
			if (bRecursive)
				FindAllFiles(sfile, dqFiles, sNameFilter, sExtFilter, bRecursive, include_folder, sExceptStr, auto_sort);
			else if (include_folder)
			{
				dqFiles->push_back(sfile);
			}
		}
		else
		{
			if (ext_matched && !excepted)
			{
				dqFiles->push_back(sfile);
			}
		}
	}

	//sort looks like windows10 explorer
	if (dqFiles->size() && auto_sort)
	{
//#ifdef _UNICODE
		sort_like_explorer(dqFiles);
//#endif
	}
}


void find_all_files(CString folder, std::deque<CString>* dq_path, CString filter, bool include_folder, bool recursive, bool include_hidden_files, bool include_system_files)
{
	dq_path->clear();
	std::deque<WIN32_FIND_DATA> dq;
	find_all_files(folder, &dq, filter, include_folder, recursive, include_hidden_files, include_system_files);

	for (auto file : dq)
		dq_path->push_back(file.cFileName);
}

void find_all_files(CString folder, std::deque<WIN32_FIND_DATA>* dq, CString filter, bool include_folder, bool recursive, bool include_hidden_files, bool include_system_files)
{
	HANDLE hFind;
	WIN32_FIND_DATA data;

	if (filter.IsEmpty())
		filter = _T("*");

	hFind = FindFirstFile(folder + _T("\\") + filter, &data);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		return;
	}

	do
	{
		if (!include_hidden_files && (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
		{
			continue;
		}

		if (!include_system_files && (data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
		{
			continue;
		}

		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if ((_tcscmp(data.cFileName, _T(".")) != 0) && (_tcscmp(data.cFileName, _T("..")) != 0))
			{
				if (include_folder)
				{
					TCHAR temp[1024] = { 0, };

					_stprintf(temp, _T("%s%s%s"), folder, folder.Right(1) == '\\' ? _T("") : _T("\\"), data.cFileName);
					if (_tcslen(temp) < MAX_PATH)
					{
						//V3 제품군을 설치하면 랜섬웨어 방지를 위한 Decoy로 인해 각 드라이브 루트에 ")GTFE0E"와 같은 폴더가 생기고
						//그 안에는 오피스 파일들이 생긴다. 이 경로는 스킵한다.
						//if (_tcslen(data.cAlternateFileName) == 0)
						{
							_tcscpy(data.cFileName, temp);
							dq->push_back(data);
						}
					}
					else
					{
						TRACE(_T("exceed filename length : %d. (%s)\n"), _tcslen(temp), temp);
					}
				}

				if (recursive)
					find_all_files(CString(data.cFileName), dq, filter, include_folder, recursive);
			}
		}
		else
		{
			TCHAR temp[1024] = { 0, };
			_stprintf(temp, _T("%s%s%s"), folder, folder.Right(1) == '\\' ? _T("") : _T("\\"), data.cFileName);
			if (_tcslen(temp) < MAX_PATH)
			{
				//if (_tcslen(data.cAlternateFileName) == 0)
				{
					_tcscpy(data.cFileName, temp);
					dq->push_back(data);
				}
			}
			else
			{
				TRACE(_T("exceed filename length : %d. (%s)\n"), _tcslen(temp), temp);
			}
		}
	} while (FindNextFile(hFind, &data));

	FindClose(hFind);
	bool res;
	//sort looks like windows10 explorer
	if (dq->size())// && auto_sort)
	{
		std::sort(dq->begin(), dq->end(),
			[&res](WIN32_FIND_DATA a, WIN32_FIND_DATA b)
			{
				res = (StrCmpLogicalW((CStringW)a.cFileName, (CStringW)b.cFileName) == -1);
				//res = (_tcsicmp(a.cFileName, b.cFileName) < 0);
				//TRACE(_T("%s < %s = %d\n"), a.cFileName, b.cFileName, res);
				return res;
				//return (_tcsicmp(a.cFileName, b.cFileName) == 1);
				//CString aa = get_part(a.cFileName, fn_name);
				//CString bb = get_part(b.cFileName, fn_name);
				//return (aa < bb);
			}
		);
	}
}

//list를 NULL로 호출하면 단지 sub folder의 갯수만 참조할 목적이다.
//recursive는 제공하지 않는다.
//root가 "내 PC"일 경우 special_folders가 true이면 다운로드, 내 문서, 바탕 화면 항목까지 추가한다.
int get_sub_folders(CString root, std::deque<CString>* list, bool special_folders, bool include_files)
{
	if (list)
		list->clear();

	std::deque<CString> folders;

	CString file;
	CFileFind finder;

	//"로컬 디스크 (C:)"
	//root = convert_special_folder_to_real_path(root);

	if (PathIsDirectory(root))
	{
		if (root.Right(1) == _T("\\"))
			root += _T("*");
		else
			root += _T("\\*");
	}

	if (root == _T(""))
	{
		folders.push_front(get_system_label(CSIDL_DESKTOP));
		folders.push_front(get_system_label(CSIDL_MYDOCUMENTS));
		//folders.push_front(get_system_label(CSIDL_DOWNLOAD)_T("다운로드"));
	}
	else if (root == get_system_label(CSIDL_DRIVES))
	{
		std::deque<CString> drive_list;
		get_drive_list(&drive_list);
		for (int i = 0; i < drive_list.size(); i++)
			folders.push_back(drive_list[i]);

		if (special_folders)
		{
			folders.push_front(get_system_label(CSIDL_DESKTOP));
			folders.push_front(get_system_label(CSIDL_MYDOCUMENTS));
			//folders.push_front(_T("다운로드"));
		}
	}
	else
	{
		bool bWorking = finder.FindFile(root);

		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			file = finder.GetFilePath();

			if (finder.IsDots())
			{
				continue;
			}
			else if ((include_files || finder.IsDirectory()) && !finder.IsHidden() && !finder.IsSystem())
			{
				folders.push_back(file);
			}
		}
	}

	if (list && root != get_system_label(CSIDL_DRIVES))
		sort_like_explorer(&folders);

	if (list)
		list->assign(folders.begin(), folders.end());

	return folders.size();
}

bool has_sub_folders(CString path)
{
	if (path.GetLength() > 2 && path.Right(1) == _T("\\"))
		truncate(path, 1);

	CString file;
	CFileFind filefind;
	bool bWorking = filefind.FindFile(path + _T("\\*"));

	while (bWorking)
	{
		bWorking = filefind.FindNextFile();
		file = filefind.GetFilePath();

		if (!filefind.IsDots() && filefind.IsDirectory() && !filefind.IsHidden())
			return true;
	}

	return false;
}

#if (_MSVC_LANG >= _std_cpp17)
std::deque<CString> find_all_files(CString path, CString name_filter, CString ext_filters, CString except_str, bool recursive, bool auto_sort)
{
	int i;
	std::deque<CString> list;

	if (IsFolder(path) == false)
		return list;

	std::deque<CString>	dq_ext;
	std::deque<CString>	dq_except;
	//필터와 제외 문자열은 대소문자를 구분하지 않는다.
	int curPos = 0;
	CString sToken = ext_filters;
	CString file;
	CString filetitle;
	CString name_field = name_filter.MakeLower();

	//name_filter에서 wildcard를 제거한 부분
	name_field.Replace(_T("*"), _T(""));
	name_field.Replace(_T("?"), _T(""));

	//name_filter는 일단 *만 허용한다.
	//abc* : 
	//*abc
	//*abc*
	if (name_filter == "*")
		name_filter = _T("");

	if (ext_filters == "")
		ext_filters = _T("*");

	ext_filters.MakeLower();

	//확장자는 "jpg"와 같이 dot가 없이 비교한다.
	//확장자에 * 또는 dot가 있다면 제거해준다.
	ext_filters.Replace(_T("*"), _T(""));
	//ext_filters.Replace(_T("."), _T(""));
	while (true)
	{
		sToken = ext_filters.Tokenize(_T(";"), curPos);
		if (sToken == "")
			break;
		dq_ext.push_back(sToken);
	}

	curPos = 0;
	sToken = except_str;
	except_str.MakeLower();
#ifdef _WINDOWS
	except_str.Replace(_T("/"), _T("\\"));
#endif
	dq_except.clear();
	if (except_str != "")
	{
		while (true)
		{
			sToken = except_str.Tokenize(_T(";"), curPos);
			if (sToken == "")
				break;

			dq_except.push_back(sToken);
		}
	}

	//const boost::regex filter()
	if (recursive)
	{
		for (const std::filesystem::directory_entry& entry :
			std::filesystem::recursive_directory_iterator(CString2string(path)))
		{
			file = entry.path().c_str();

			if (IsFolder(file))
				continue;

			bool found = false;
			filetitle = get_part(file, fn_title).MakeLower();

			if (name_filter.IsEmpty())
			{
				found = true;
			}
			else
			{
				//abc
				if (name_filter.Find('*') < 0)
				{
					if (filetitle == name_filter)
						found = true;
				}
				//abc*
				else if (name_filter.Find('*') == name_filter.GetLength() - 1)
				{
					if (filetitle.Find(name_field) == 0)
						found = true;
				}
				//*abc
				else if (name_filter.Find('*') == 0)
				{
					if (filetitle.Right(name_field.GetLength()) == name_filter.Right(name_filter.GetLength() - 1))
						found = true;
				}
				//*abc*
				else if (name_filter.Find('*') == name_filter.GetLength() - 1)
				{
					if (filetitle.Find(name_field) > 0)
						found = true;
				}
			}

			if (!found)
				continue;

			found = false;
			if (dq_ext.size() > 0)
			{
				for (i = 0; i < dq_ext.size(); i++)
				{
					if (get_part(file, fn_ext).MakeLower() == dq_ext[i])
					{
						found = true;
						break;
					}
				}
			}
			else
			{
				found = true;
			}

			if (!found)
				continue;

			bool except = false;
			//제외할 문자열이 포함되어 있지 않은 파일들만 리스트에 추가한다.
			for (i = 0; i < dq_except.size(); i++)
			{
				if (CString(file).MakeLower().Find(dq_except[i]) >= 0)
				{
					except = true;
					break;
				}
			}

			if (!except)
				list.push_back(file);
		}
	}
	else
	{
		for (const std::filesystem::directory_entry& entry :
			std::filesystem::directory_iterator(CString2string(path)))
		{
			file = entry.path().c_str();

			if (IsFolder(file))
				continue;

			bool found = false;
			filetitle = get_part(file, fn_title).MakeLower();

			if (name_filter.IsEmpty())
			{
				found = true;
			}
			else
			{
				//abc
				if (name_filter.Find('*') < 0)
				{
					if (filetitle == name_filter)
						found = true;
				}
				//abc*
				else if (name_filter.Find('*') == name_filter.GetLength() - 1)
				{
					if (filetitle.Find(name_field) == 0)
						found = true;
				}
				//*abc
				else if (name_filter.Find('*') == 0)
				{
					if (filetitle.Right(name_field.GetLength()) == name_filter.Right(name_filter.GetLength() - 1))
						found = true;
				}
				//*abc*
				else if (name_filter.Find('*') == name_filter.GetLength() - 1)
				{
					if (filetitle.Find(name_field) > 0)
						found = true;
				}
			}

			if (!found)
				continue;

			found = false;
			if (dq_ext.size() > 0)
			{
				for (i = 0; i < dq_ext.size(); i++)
				{
					if (get_part(file, fn_ext).MakeLower() == dq_ext[i])
					{
						found = true;
						break;
					}
				}
			}
			else
			{
				found = true;
			}

			if (!found)
				continue;

			bool except = false;
			//제외할 문자열이 포함되어 있지 않은 파일들만 리스트에 추가한다.
			for (i = 0; i < dq_except.size(); i++)
			{
				if (CString(file).MakeLower().Find(dq_except[i]) >= 0)
				{
					except = true;
					break;
				}
			}

			if (!except)
				list.push_back(file);
		}
	}

	//std::deque<CString> list1;
	//list1.assign(list.begin(), list.end());

	//sort looks like windows10 explorer
	if (list.size() && auto_sort)
	{
		sort_like_explorer(&list);
	}

	//if (list == list1)
	//	;
	//if (std::equal(list.begin(), list.end(), list1.begin(), list1.end()))
	//	;

	return list;
}
#endif

static BOOL wildcmp(const wchar_t* pat, const wchar_t* str)
{
	wchar_t* s;
	wchar_t* p;
	BOOL star = FALSE;

loopStart:
	for (s = const_cast<wchar_t*>(str), p = const_cast<wchar_t*>(pat); *s; ++s, ++p) {
		switch (*p) {
		case L'?':
			if (*s == L'.') goto starCheck;
			break;
		case L'*':
			star = TRUE;
			str = s, pat = p;
			do { ++pat; } while (*pat == L'*');
			if (!*pat) return TRUE;
			goto loopStart;
		default:
			// if (_totupper(*s) != _totupper(*p))
			if (towupper(*s) != towupper(*p))
				goto starCheck;
			break;
		} /* endswitch */
	} /* endfor */
	while (*p == L'*') ++p;
	return (!*p);

starCheck:
	if (!star) return FALSE;
	str++;
	goto loopStart;
}

void save_dqlist(std::deque<CString>* dqlist, CString output_text_file_path)
{
	int i;
	FILE* fp;

	if ((fp = _tfopen(output_text_file_path, _T("wt")CHARSET)) == NULL)
		return;

	//  UniCode BOM 기록
	//WORD wMark = 0xFEFF;
	//fwrite(&wMark, sizeof(WORD), 1, fp);

	for (i = 0; i < dqlist->size(); i++)
	{
		_ftprintf(fp, _T("%s\n"), dqlist->at(i));
	}

	fclose(fp);
}

std::deque<CString>	FindFilesWithExtensions(CString folder, CString fileTitle, CString extensions)
{
	CString token;
	CString fullpath;
	std::deque<CString> dqToken;
	std::deque<CString> result;

	//확장자는 "jpg"와 같이 dot가 없이 비교한다.
	//확장자에 * 또는 dot가 있다면 제거해준다.
	extensions.Replace(_T("*"), _T(""));
	extensions.Replace(_T("."), _T(""));

	get_token_string(extensions, dqToken, ';');

	//폴더내의 파일들을 찾아서 비교하는게 아니라
	//조건에 맞는 파일이 존재하는지로 검사한다.
	for (int i = 0; i < dqToken.size(); i++)
	{
		fullpath.Format(_T("%s\\%s.%s"), folder, fileTitle, dqToken[i]);
		if (PathFileExists(fullpath))
			result.push_back(fullpath);
	}

	return result;
}

bool delete_file(CString fullpath, bool bTrashCan)
{
	if (!bTrashCan)
		return DeleteFile(fullpath);

	SHFILEOPSTRUCT FileOp = {0};
	TCHAR szTemp[MAX_PATH];

	_stprintf(szTemp, _T("%s\0"), fullpath);
	szTemp[fullpath.GetLength() + 1] = NULL;

	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_DELETE; // 삭제 속성 설정
	FileOp.pFrom = NULL;
	FileOp.pTo = NULL;
	//확인메시지가 안뜨도록 설정
	FileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI;
	FileOp.fAnyOperationsAborted = false;
	FileOp.hNameMappings = NULL;
	FileOp.lpszProgressTitle = NULL;
	FileOp.pFrom = szTemp;

	int res = SHFileOperation(&FileOp); // 삭제 작업
	if (res == 0)
		return true;

	return false;
}

int	get_text_encoding(CString sfile)
{
	int text_encoding = text_encoding_ansi;

	unsigned char buf[4096] = { 0, };

	FILE* fp = _tfopen(sfile, _T("rb"));
	if (fp == NULL)
		return text_encoding_unknown;

	size_t dwRead = fread(buf, 1, 4096, fp);
	fclose(fp);

	if (dwRead >= 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
		text_encoding = text_encoding_utf8bom;
	else if (dwRead >= 2 && buf[0] == 0xfe && buf[1] == 0xff)
		text_encoding = text_encoding_utf16be;
	else if (dwRead >= 2 && buf[0] == 0xff && buf[1] == 0xfe)
		text_encoding = text_encoding_utf16be;
	else if (dwRead >= 4 && buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0xfe && buf[3] == 0xff)
		text_encoding = text_encoding_utf32be;
	else if (dwRead >= 4 && buf[0] == 0xff && buf[1] == 0xfe && buf[2] == 0x00 && buf[3] == 0x00)
		text_encoding = text_encoding_utf32le;
	else if (dwRead >= 4 && buf[0] == 0x2b && buf[1] == 0x2f && buf[2] == 0x76 && buf[3] == 0x38)
		text_encoding = text_encoding_utf7;
	else if (dwRead >= 4 && buf[0] == 0x2b && buf[1] == 0x2f && buf[2] == 0x76 && buf[3] == 0x39)
		text_encoding = text_encoding_utf7;
	{
		//BOM이 없는 일반 파일의 경우 아래와 같이 utf8 문자 범위를 검사하여 utf8인지 euc-kr인지 판별한다.
		//출처: https://dev-drive.tistory.com/10 [Dev Drive:티스토리]
		//단, 용량이 0바이트이면 판별이 불가하므로 utf8로 간주한다.
		bool is_utf8 = true;

		unsigned char* start = (unsigned char*)buf;
		unsigned char* end = (unsigned char*)buf + dwRead;

		while (start < end)
		{
			if (*start < 0x80) // (10000000)[output][/output]
			{
				start++;
			}
			else if (*start < (0xC0)) // (11000000)
			{
				is_utf8 = false;
				break;
			}
			else if (*start < (0xE0)) // (11100000)
			{
				if (start >= end - 1)
					break;
				if ((start[1] & (0xC0)) != 0x80)
				{
					is_utf8 = false;
					break;
				}
				start += 2;
			}
			else if (*start < (0xF0)) // (11110000)
			{
				if (start >= end - 2)
					break;
				if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80)
				{
					is_utf8 = false;
					break;
				}
				start += 3;
			}
			else
			{
				is_utf8 = false;
				break;
			}
		}

		if (is_utf8)
			text_encoding = text_encoding_utf8;
		else
			text_encoding = text_encoding_ansi;
	}

	return text_encoding;
}

//파일을 읽어서 CString으로 리턴한다. max_length < 0이면 전체 파일을 읽어서 리턴한다.
//encoding < 0이면 encoding 방식을 자동 판별하여 읽어온다.
CString read(CString filepath, int max_length, int encoding)
{
	CString result;

	if (PathFileExists(filepath) == false)
		return result;

	int text_encoding = get_text_encoding(filepath);
	uint64_t filesize = get_file_size(filepath);

	if (max_length > 0)
		max_length = MIN(filesize, max_length);
	else
		max_length = filesize;

	if (filesize == 0)
		return result;

	if (encoding < 0)
	{
		if (text_encoding == text_encoding_ansi)
			encoding = CP_ACP;
		else
			encoding = CP_UTF8;
	}
	else if (encoding != CP_ACP && encoding != CP_UTF8)
	{
		encoding = CP_UTF8;
	}

	FILE* fp = NULL;
	
	if (encoding == CP_UTF8)
	{
		TCHAR* data = new TCHAR[max_length * 2 + 1];
		memset(data, 0, sizeof(TCHAR) * (max_length * 2 + 1));
		fp = _tfopen(filepath, _T("rt")CHARSET);
		fread(data, 1, max_length * 2, fp);
		result = CString(data);
		delete[] data;
	}
	else
	{
		char* data = new char[max_length + 1];
		memset(data, 0, sizeof(char) * (max_length + 1));
		_tfopen_s(&fp, filepath, _T("rt"));
		fread(data, 1, max_length, fp);
		result = CString(data);
		delete[] data;
	}

	fclose(fp);

	return result;
}

//text 파일로 저장한다. 만약 base64_encode된 text라면 encoding = CP_ACP, is_binary_data = true로 해야 한다.
bool save(CString filepath, CString text, int code_page, bool is_binary_data)
{
	if (code_page != CP_ACP && code_page != CP_UTF8)
		code_page = CP_UTF8;

	FILE* fp = NULL;

	if (code_page == CP_UTF8)
		_tfopen_s(&fp, filepath, _T("wt")CHARSET);
	else
		_tfopen_s(&fp, filepath, _T("wt"));

	if (fp == NULL)
		return false;

	if (code_page == CP_ACP)
	{
		char* chText = CString2char(text);

		if (is_binary_data)
		{
			fwrite(chText, sizeof(char), strlen(chText), fp);

			//맨 마지막에 null char를 써주지 않으면 여전히 UTF-8로 인식된다.
			char null_ch = '\0';
			fwrite(&null_ch, 1, 1, fp);
		}
		else
		{
			_ftprintf(fp, _T("%S"), chText);
		}

		delete[] chText;
	}
	else
	{
		_ftprintf(fp, _T("%s"), text);
	}

	fclose(fp);

	return true;
}

int file_open(FILE** fp, CString mode, CString file)
{
	//binary open이면 그냥 열어주고
	if (mode.Find(_T("b")) > 0)
	{
		_tfopen_s(fp, file, mode);
		return text_encoding_unknown;
	}

	//encording 방식을 읽어온다.
	int	text_encoding = get_text_encoding(file);

	if (text_encoding <= text_encoding_ansi)
		_tfopen_s(fp, file, mode);
	else if (text_encoding != text_encoding_unknown)
		_tfopen_s(fp, file, mode + CHARSET);

	return text_encoding;
}

//text 파일을 열어서 dqList에 넣어준다.
bool read_file(CString filepath, std::deque<CString> *dqList, bool using_utf8)
{
	dqList->clear();

	FILE* fp;
	TCHAR tline[2048];
	CString sline;

	_tfopen_s(&fp, filepath, (using_utf8 ? _T("rt")CHARSET : _T("rt")));
	if (!fp)
		return false;

	while (_fgetts(tline, sizeof(tline), fp) != NULL)
	{
		sline = tline;
		sline.Trim();
		dqList->push_back(sline);
	}

	fclose(fp);
	return true;
}

//시작폴더 및 하위 폴더들은 여전히 남아있다.
//폴더 통째로 다 지우려면 코드의 수정이 필요하다.
int	delete_all_files(CString folder, CString name_filter, CString ext_filter, bool recursive /*= true*/, bool trash_can /*= false*/)
{
	std::deque<CString> files;
	FindAllFiles(folder, &files, name_filter, ext_filter, recursive, false);

	if (files.size() == 0)
		return 0;

	for (int i = 0; i < files.size(); i++)
		delete_file(files[i], trash_can);

	return files.size();
}

bool RecursiveCreateDirectory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpsa/* = NULL*/)
{
	ASSERT(lpPathName);

	return (ERROR_SUCCESS == SHCreateDirectoryEx(NULL, lpPathName, lpsa));
}

bool RecursiveRemoveDirectory(LPCTSTR lpPathName, bool bDeletePermanent/* = TRUE*/)
{
	ASSERT(lpPathName);

	TCHAR szDirectory[MAX_PATH + 1];
	SHFILEOPSTRUCT fos = {0};

	_tcscpy_s(szDirectory, MAX_PATH, lpPathName);
	szDirectory[_tcslen(szDirectory) + 1] = 0;

	fos.wFunc = FO_DELETE;
	fos.fFlags = FOF_NO_UI | (bDeletePermanent ? 0 : FOF_ALLOWUNDO);
	fos.pFrom = szDirectory;

	return (ERROR_SUCCESS == SHFileOperation(&fos));
}

//인자로 넘어온 폴더를 포함하여 모든 하위폴더, 파일을 지운다.
bool SHDeleteFolder(CString sFolder)
{
    SHFILEOPSTRUCT FileOp = {0};
    TCHAR szTemp[MAX_PATH];
 
	_stprintf(szTemp, _T("%s"), sFolder);
    szTemp[_tcslen(szTemp) + 1] = NULL;
 
    FileOp.hwnd = NULL;
    FileOp.wFunc = FO_DELETE;       // 삭제 속성 설정
    FileOp.pTo = NULL;
    FileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SIMPLEPROGRESS; // 확인메시지가 안뜨도록 설정
    FileOp.fAnyOperationsAborted = false;
    FileOp.hNameMappings = NULL;
    FileOp.lpszProgressTitle = sFolder;
    FileOp.pFrom = szTemp;
 
    if (SHFileOperation(&FileOp) == 0)       // 삭제 작업
        return TRUE;

	return FALSE;
}
 
//------------------------------------------------------------
//하위폴더 모두 삭제
//하위 디렉토리 모두 삭제
bool DeleteFolder(LPCTSTR lpFolder)
{
    WIN32_FIND_DATA FindData;
    HANDLE			FindHandle;
    TCHAR			temp1[MAX_PATH];
    int				lpFolder_Len;

    if (lpFolder == NULL)
        return FALSE;

    if (*lpFolder == '\0')
        return FALSE;

    lpFolder_Len = _tcslen(lpFolder);
    if(lpFolder[lpFolder_Len-1]=='\\')
    {
        _stprintf(temp1, _T("%s*.*"), lpFolder);
    }
    else
    {
		_stprintf(temp1, _T("%s\\*.*"), lpFolder);
    }
    FindHandle=FindFirstFile(temp1, &FindData);
    if(INVALID_HANDLE_VALUE!=FindHandle)
    {
        while(1)
        {
            if(lpFolder[lpFolder_Len-1]=='\\')
            {	
				_stprintf(temp1, _T("%s%s"), lpFolder, FindData.cFileName);
            }
            else
            {
				_stprintf(temp1, _T("%s\\%s"), lpFolder, FindData.cFileName);
            }
            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //Dir
                if(_tcscmp(FindData.cFileName, _T(".")) == 0)
                {
                    //.    -->> skip
                }
                else if(_tcscmp(FindData.cFileName, _T("..")) == 0)
                {
                    //..   -->> skip
                }
                else
                {
                    DeleteFolder(temp1);
                    if((FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)||
                        (FindData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
                    {
                        SetFileAttributes(temp1, FILE_ATTRIBUTE_NORMAL);
                    }
                    RemoveDirectory(temp1);
                }
            }
            else
            {
                //File
                if((FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)||
                    (FindData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
                {
                    SetFileAttributes(temp1, FILE_ATTRIBUTE_NORMAL);
                }
                delete_file(temp1, true);
            }
            if(!FindNextFile(FindHandle, &FindData))
            {
                break;
            }
        }
    }

    FindClose(FindHandle);	//do not use CloseHandle();

    SetFileAttributes(lpFolder, FILE_ATTRIBUTE_NORMAL);
    return RemoveDirectory(lpFolder);
}

//폴더내의 특정 문자열이 들어간 파일들을 지운다. 하위폴더 지원안함.
void DeleteFilesBySubString(CString sFolder, CString filenameSubStr, bool bMatchWholeWordOnly/* = FALSE*/, bool bMatchCase/* = FALSE*/)
{
	CFileFind	finder;
	CString		sfile, filename;
	bool		bWorking;
	
	if (sFolder.Right(4) != "\\*.*")
		sFolder += "\\*.*";

	bWorking = finder.FindFile(sFolder);

	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		sfile = finder.GetFilePath();
		filename = get_part(sfile, fn_name);

		if (!bMatchCase)
		{
			filename.MakeLower();
			filenameSubStr.MakeLower();
		}
		
		if (finder.IsDots() || finder.IsDirectory())
			continue;
		else
		{
			if (bMatchWholeWordOnly)
			{
				if (filename == filenameSubStr)
					delete_file(sfile, true);
			}
			else
			{
				if (filename.Find(filenameSubStr) >= 0)
					delete_file(sfile, true);
			}
		}
	}
}

#include "Shlwapi.h"
#include <regex>
#pragma comment(lib, "shlwapi") 

//usage : sFolder include folder names only except file name.
//c:\test\00\1.bmp	(x)	=> 1.bmp folder will be created.(not intended)
//c:\test\00		(o)
bool recursive_make_full_directory(LPCTSTR sFolder)
{
	if (PathFileExists(sFolder) && ::PathIsDirectory(sFolder))
		return true;
	
	TCHAR parent[MAX_PATH] = _T("");

	_tcscpy(parent, sFolder);
	::PathRemoveFileSpec(parent);
	
	if (recursive_make_full_directory(parent))
		return (::CreateDirectory(sFolder, NULL) != false);

	return false;
} 

bool make_full_directory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpsa/* = NULL*/)
{
	if (PathFileExists(lpPathName))
		return true;
	CString folder(lpPathName);
	return (ERROR_SUCCESS == SHCreateDirectoryEx(NULL, (LPCTSTR)folder, lpsa));
}

void draw_center_text(CDC* pdc, const CString& strText, CRect& rcRect)
{
	CRect rcSavedRect = rcRect;
	CRect rcNewRect = rcRect;
	CRgn rgn;

	rgn.CreateRectRgn(rcRect.left, rcRect.top, rcRect.right, rcRect.bottom);

	// Call DrawText with DT_WORDBREAK | DT_CALCRECT flags, this will adjust the rectagle size
	// after breaking it into lines but without drawing

	pdc->DrawText(strText, &rcSavedRect, DT_CALCRECT | DT_WORDBREAK | DT_CENTER | DT_VCENTER | DT_NOCLIP);

	// Modify the caption rectagle according to the new coordinates
	rcNewRect.top = rcRect.top + (rcRect.Height() - rcSavedRect.Height()) / 2;
	rcNewRect.bottom = rcRect.top + rcSavedRect.Height();

	HRGN hrgnOld = NULL;
	::GetClipRgn(pdc->m_hDC, hrgnOld);

	pdc->SelectClipRgn(&rgn, RGN_COPY);

	// Draw the caption
	pdc->DrawText(strText, &rcNewRect, DT_WORDBREAK | DT_CENTER | DT_VCENTER | DT_NOCLIP);

	::SelectClipRgn(pdc->m_hDC, hrgnOld);

	DeleteObject(rgn);
}

Gdiplus::RectF measure_string(Gdiplus::Graphics* g, Gdiplus::Font& font, LPCTSTR String, int length)
{
	Gdiplus::RectF layout(0, 0, 65536, 65536);
	Gdiplus::CharacterRange chrange(0, length == -1 ? lstrlen(String) : length);
	Gdiplus::StringFormat sf;
	Gdiplus::Region rgn;
	Gdiplus::RectF rt;

	sf.SetMeasurableCharacterRanges(1, &chrange);
	g->MeasureCharacterRanges((CStringW)String, length, &font, layout, &sf, 1, &rgn);
	rgn.GetBounds(&rt, g);

	return rt;
}

//출력할 글자를 작게 출력한 후 이를 다시 원래 크기로 늘려
//blur가 생기게 하고 이를 shadow로 사용하는 방식인데 뭔가 어색하다.
//ApplyEffect의 blur를 적용해서 구현하는 것이 나을 듯 하다.
CRect draw_text(Gdiplus::Graphics &g,
				int x, int y, int w, int h,
				CString text,
				float font_size,
				int font_style,
				int shadow_depth,
				float thickness,
				CString font_name,
				Gdiplus::Color cr_text,
				Gdiplus::Color cr_stroke,
				Gdiplus::Color cr_shadow,
				Gdiplus::Color cr_back,
				UINT align)
{
	return draw_text(g,
					CRect(x, y, x + w, y + h),
					text,
					font_size,
					font_style,
					shadow_depth,
					thickness,
					font_name,
					cr_text,
					cr_stroke,
					cr_shadow,
					cr_back,
					align);
}

CRect draw_text(Gdiplus::Graphics &g,
				CRect rTarget,
				CString text,
				float font_size,
				int font_style,
				int shadow_depth,
				float thickness,
				CString font_name,
				Gdiplus::Color cr_text,
				Gdiplus::Color cr_stroke,
				Gdiplus::Color cr_shadow,
				Gdiplus::Color cr_back,
				UINT align)
{
	bool calcRect = false;
	HDC hDC = ::GetDC(AfxGetMainWnd()->m_hWnd);

	//배경색을 rTarget 크기로 그려서는 안된다. 실제 텍스트가 그려질 boundRect 영역만 그려져야 한다.
	draw_rectangle(g, rTarget, Gdiplus::Color::Transparent, cr_back);

	//큰 글씨는 AntiAlias를 해주는게 좋지만 작은 글씨는 오히려 뭉개지므로 안하는게 좋다.
	//파라미터로 처리해야 한다.
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	Gdiplus::Unit unit = g.GetPageUnit();
	float fDpiX = g.GetDpiX();
	float fDpiY = g.GetDpiY();

	int logPixelsY = ::GetDeviceCaps(hDC, LOGPIXELSY);
	//Gdiplus::REAL emSize = (Gdiplus::REAL)MulDiv(font_size, 96, logPixelsY);
	float emSize = fDpiY * font_size / 96.0;

	Gdiplus::FontFamily *fontFamily = new Gdiplus::FontFamily((WCHAR*)(const WCHAR*)CStringW(font_name));
	if (!fontFamily->IsAvailable())
	{
		delete fontFamily;
		fontFamily = new Gdiplus::FontFamily(CStringW("Arial"));
	}

	Gdiplus::Font font(fontFamily, emSize, font_style);

	Gdiplus::SolidBrush shadow_brush(cr_shadow);
	Gdiplus::SolidBrush brush2(cr_text);

	Gdiplus::StringFormat sf;


	Gdiplus::RectF boundRect;
	g.MeasureString(CStringW(text), -1, &font, Gdiplus::PointF(rTarget.left, rTarget.top), &boundRect);
	//boundRect = measure_string(g, font, text);

	boundRect.Width += shadow_depth;
	boundRect.Height += shadow_depth;

	if (rTarget.Width() == 0)
		rTarget.right = rTarget.left + boundRect.Width;
	if (rTarget.Height() == 0)
		rTarget.bottom = rTarget.top + boundRect.Height;

	if (align & DT_CENTER)
		sf.SetAlignment(Gdiplus::StringAlignmentCenter);
		//x = rTarget.CenterPoint().x - boundRect.Width / 2;
	else if (align & DT_RIGHT)
		sf.SetAlignment(Gdiplus::StringAlignmentFar);
		//x = rTarget.right - boundRect.Width;
	else
		sf.SetAlignment(Gdiplus::StringAlignmentNear);

	if (align & DT_VCENTER)
		//y = rTarget.CenterPoint().y - boundRect.Height / 2;
		sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
	else if (align & DT_BOTTOM)
		//y = rTarget.bottom - boundRect.Height;
		sf.SetLineAlignment(Gdiplus::StringAlignmentFar);
	else
		sf.SetLineAlignment(Gdiplus::StringAlignmentNear);


	if (calcRect)
	{
		switch (unit)
		{
		case Gdiplus::UnitWorld:
			break;
		case Gdiplus::UnitPoint:
			break;
		case Gdiplus::UnitInch:
			break;
		case Gdiplus::UnitDocument:
			break;
		case Gdiplus::UnitMillimeter:
			break;
		}

		delete fontFamily;
		//delete g;
		::DeleteDC(hDC);
		//TRACE(_T("%f, %f, %f x %f\n"), boundRect.X, boundRect.Y, boundRect.Width, boundRect.Height);
		return CRect(rTarget.left, rTarget.top, rTarget.left + boundRect.Width, rTarget.top + boundRect.Height);
	}

	//g_shadow.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	if (shadow_depth > 0)
	{
		//blur를 이용한 방법. 간단하지만 속도가 매우 느림
		/*
		CGdiplusBitmap shadow(rTarget.Width(), rTarget.Height());
		Gdiplus::Graphics g_shadow(shadow.m_pBitmap);

		g_shadow.DrawString(CStringW(text), -1, &font, Gdiplus::RectF(rTarget.left, rTarget.top, rTarget.Width(), rTarget.Height()), &sf, &shadow_brush);
		shadow.apply_effect_blur(20.0f, FALSE);
		//g->DrawImage(shadow.m_pBitmap, (Gdiplus::REAL)shadow_depth, (Gdiplus::REAL)shadow_depth);
		g->DrawImage(shadow.m_pBitmap, (Gdiplus::REAL)shadow_depth/2, (Gdiplus::REAL)shadow_depth/2);
		*/

		//작게 이미지를 만든 후 늘리는 방식은 quality도 떨어지고 그림자 방향을 정하는 것도 다소 문제있다.
		//shadow는 (+n, +n)에 그림자를 그리고 실제 텍스트를 그리는 방식도 있지만 text를 둘러싸는 그림자도 있으므로
		//좀 복잡하다. 우선 간단한 방법으로의 shadow만 고려한다.
		Gdiplus::Bitmap shadow_bitmap(rTarget.Width(), rTarget.Height());
		Gdiplus::Graphics g_shadow(&shadow_bitmap);
		g_shadow.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		g_shadow.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		g_shadow.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

		//그림자의 흐릿한 정도. 0.0f(그림자 없음), 0.1f(많이 흐림), 0.4f(권장), 1.0f에 가까울수록 선명함.
		//축소한 글자를 원본크기로 늘려서 흐릿한 이미지로 만듬.
		float ratio = 0.4f;
		Gdiplus::Matrix mx(ratio, 0, 0, ratio, 0.0f, 0.0f);
		g_shadow.SetTransform(&mx);
		g_shadow.DrawString(CStringW(text), -1, &font, CRect2GpRectF(rTarget), &sf, &shadow_brush);

		g.DrawImage(&shadow_bitmap, (Gdiplus::REAL)shadow_depth, (Gdiplus::REAL)shadow_depth,
			(Gdiplus::REAL)(shadow_bitmap.GetWidth()) / ratio, (Gdiplus::REAL)(shadow_bitmap.GetHeight()) / ratio);
	}

	//return CRect(rTarget.left, rTarget.top, rTarget.left + boundRect.Width, rTarget.top + boundRect.Height);

	if (thickness == 0.0)
	{
		g.DrawString(CStringW(text), -1, &font, Gdiplus::RectF(rTarget.left, rTarget.top, rTarget.Width(), rTarget.Height()), &sf, &brush2);
	}
	else
	{
		float emSize = fDpiY * font_size / 72.0;
		//float emSize = fDpiY * font.GetSize() / 72.0;
		Gdiplus::GraphicsPath str_path;

		str_path.AddString(CStringW(text), -1, fontFamily,
			font_style, emSize, Gdiplus::RectF(rTarget.left, rTarget.top, rTarget.Width(), rTarget.Height()), &sf);

		Gdiplus::Pen   gp(cr_stroke, thickness);
		//gp.SetLineJoin(Gdiplus::LineJoinMiter);
		Gdiplus::SolidBrush gb(cr_text);

		g.DrawPath(&gp, &str_path);
		g.FillPath(&gb, &str_path);
	}

	delete fontFamily;
	::DeleteDC(hDC);

	return CRect(rTarget.left, rTarget.top, rTarget.left + boundRect.Width, rTarget.top + boundRect.Height);
}

//text의 출력픽셀 너비가 max_width를 넘을 경우 ...와 함께 표시될 문자위치를 리턴.
//이 함수는 DrawText시에 DT_END_ELLIPSIS를 줘서 사용하므로 우선 사용 보류!
int	get_ellipsis_pos(CDC* pDC, CString text, int max_width)
{
	CRect rt;
	int dot_width;
	bool dot = false;
	CString sub_str;

	pDC->DrawText(_T("..."), &rt, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER);
	dot_width = rt.Width();

	for (int i = text.GetLength(); i >= 1; i--)
	{
		sub_str = text.Left(i);
		TRACE(_T("sub_str = %s\n"), sub_str);

		pDC->DrawText(sub_str, &rt, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		if (rt.Width() + (dot ? dot_width : 0) >= max_width)
		{
			dot = true;
		}
		else
		{
			return i;
		}
	}

	return 0;
}

void draw_line_pt(CDC* pDC, CPoint pt1, CPoint pt2, Gdiplus::Color cr, int width, Gdiplus::DashStyle pen_style, int draw_mode)
{
	draw_line(pDC, pt1.x, pt1.y, pt2.x, pt2.y, cr, width, pen_style, draw_mode);
}

void draw_line(CDC* pDC, int x1, int y1, int x2, int y2, Gdiplus::Color cr, float thick, Gdiplus::DashStyle pen_style, int draw_mode)
{
	Gdiplus::Graphics g(pDC->m_hDC);
	Gdiplus::Pen pen(cr, thick);
	pen.SetDashStyle(pen_style);
	g.DrawLine(&pen, x1, y1, x2, y2);
}

void draw_rectangle(CDC* pDC, CRect Rect, COLORREF crColor/* = RGB(0,0,0)*/, COLORREF crFill, int nWidth, int nPenStyle, int nDrawMode)
{
	LOGBRUSH lb;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = crColor;

	CPen	Pen(PS_GEOMETRIC | nPenStyle, nWidth, &lb);
	CPen*	pOldPen = (CPen*)pDC->SelectObject(&Pen);
	int		nOldDrawMode = pDC->SetROP2(nDrawMode);
	CBrush	brBrush(crFill);
	CBrush* pOldBrush;

	if (crFill == NULL_BRUSH)
		pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	else
		pOldBrush = (CBrush*)pDC->SelectObject(&brBrush);

	//Rect.NormalizeRect();
	pDC->Rectangle(Rect);

	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
	Pen.DeleteObject();
	brBrush.DeleteObject();

	pDC->SetROP2(nOldDrawMode);
}

void draw_rectangle(CDC* pDC, CRect r, Gdiplus::Color cr_line, Gdiplus::Color cr_fill, int width)
{
	Gdiplus::Graphics g(pDC->m_hDC);
	draw_rectangle(g, r, cr_line, cr_fill, width);
}

void draw_rectangle(Gdiplus::Graphics &g, CRect r, Gdiplus::Color cr_line, Gdiplus::Color cr_fill, int width)
{
	Gdiplus::Pen pen(cr_line, width);
	Gdiplus::SolidBrush br(cr_fill);

	if (cr_fill.GetValue() != Gdiplus::Color::Transparent)
		g.FillRectangle(&br, CRect2GpRect(r));

	//DrawRectangle()로 그리면 right, bottom까지 그리는데 영역을 벗어나게 된다.
	//즉, (left, top) ~ (right - 1, bottom - 1)까지 그려줘야 영역을 벗어나지 않게 된다.
	r.DeflateRect(0, 0, 1, 1);
	g.DrawRectangle(&pen, CRect2GpRect(r));
}

void draw_sunken_rect(CDC* pDC, CRect r, bool bSunken, COLORREF cr1, COLORREF cr2, int width)
{
	draw_sunken_rect(pDC, r, bSunken, RGB2gpColor(cr1), RGB2gpColor(cr2), width);
}

void draw_sunken_rect(CDC* pDC, CRect r, bool sunken, Gdiplus::Color cr1, Gdiplus::Color cr2, int width)
{
	Gdiplus::Graphics g(pDC->m_hDC);
	Gdiplus::Pen pen1(cr1, width);
	Gdiplus::Pen pen2(cr2, width);

	g.DrawLine(sunken ? &pen1 : &pen2, r.left, r.bottom - 1, r.left, r.top);
	g.DrawLine(sunken ? &pen1 : &pen2, r.left, r.top, r.right - 1, r.top);

	g.DrawLine(sunken ? &pen2 : &pen1, r.right - 1, r.top, r.right - 1, r.bottom - 1);
	g.DrawLine(sunken ? &pen2 : &pen1, r.right - 1, r.bottom - 1, r.left, r.bottom - 1);
}

void draw_ellipse(CDC* pDC, CRect r, Gdiplus::Color cr_line, Gdiplus::Color cr_fill, int pen_style, int width, int draw_mode)
{
	Gdiplus::Graphics g(pDC->m_hDC);
	Gdiplus::Pen pen(cr_line, width);
	Gdiplus::SolidBrush br(cr_fill);

	g.FillEllipse(&br, CRect2GpRect(r));
	g.DrawEllipse(&pen, CRect2GpRect(r));
}

void draw_polygon(CDC* pDC, std::vector<CPoint> pts, bool closed, COLORREF crLine, int nWidth, int nPenStyle, int nDrawMode)
{
	LOGBRUSH lb;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = crLine;

	CPen	Pen(PS_GEOMETRIC | nPenStyle, nWidth, &lb);
	CPen* pOldPen = (CPen*)pDC->SelectObject(&Pen);
	int		nOldDrawMode = pDC->SetROP2(nDrawMode);

	pDC->MoveTo(pts[0]);

	for (int i = 1; i < pts.size(); i++)
	{
		pDC->LineTo(pts[i]);
	}

	if (closed)
		pDC->LineTo(pts[0]);

	pDC->SelectObject(pOldPen);
	Pen.DeleteObject();

	pDC->SetROP2(nOldDrawMode);
}

/*
void Clamp(int& nValue, int nMin, int nMax)
{
	if (nValue < nMin)
		nValue = nMin;
	else if (nValue > nMax)
		nValue = nMax;
}

void Clamp(long& nValue, long nMin, long nMax)
{
	if (nValue < nMin)
		nValue = nMin;
	else if (nValue > nMax)
		nValue = nMax;
}

void Clamp(double& dValue, double dMin, double dMax)
{
	if (dValue < dMin)
		dValue = dMin;
	else if (dValue > dMax)
		dValue = dMax;
}
*/


int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	TCHAR szDir[MAX_PATH];

	switch (uMsg)
	{
		case BFFM_INITIALIZED :	SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)lpData);
								break;
		case BFFM_SELCHANGED :	if (SHGetPathFromIDList((LPITEMIDLIST)lParam , szDir))
									SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szDir);
								break;
	}
         
	return 0;
}

bool BrowseForFolder(	HWND hwndOwner, TCHAR* lpszTitle, CString& strSelectedFolder, const TCHAR* strStartFolder /*= NULL*/, bool bNewFolderButton)
{
	bool		bSuccess = false;
	BROWSEINFO	bi;

	ZeroMemory(&bi, sizeof(BROWSEINFO));

	bi.hwndOwner	= hwndOwner;
	bi.lpszTitle	= lpszTitle;
	bi.ulFlags		= BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	if (bNewFolderButton == false)
		bi.ulFlags	|= BIF_NONEWFOLDERBUTTON;
	bi.lParam		= (LPARAM)strStartFolder;
	bi.lpfn			= BrowseCallbackProc;

	::OleInitialize(NULL);

	TCHAR	sBuffer[MAX_PATH];
	LPITEMIDLIST pIDL = ::SHBrowseForFolder(&bi);

	if (pIDL != NULL)
	{
		if (::SHGetPathFromIDList(pIDL, sBuffer))
		{
			bSuccess = true;
			strSelectedFolder.Format(_T("%s"), sBuffer);
		}

		::OleUninitialize();
	}

	return bSuccess;
}

//윈도우 운영체제에서 특정 폴더(다운로드, 내 문서 등)의 실제 경로를 리턴한다.
//FOLDERID_Downloads, FOLDERID_Documents, ...
CString get_known_folder(KNOWNFOLDERID folderID)
{
	PWSTR path = NULL;

#ifndef _USING_V110_SDK71_
	SHGetKnownFolderPath(folderID, 0, NULL, &path);
#else
	if (folderID == FOLDERID_Downloads)
	{
		CString download_folder = get_known_folder(CSIDL_PROFILE) + _T("\\Downloads");
		return download_folder;
	}

#endif

	return path;
}

CString get_known_folder(int csidl)
{
	TCHAR buf[MAX_PATH] = { 0, };
	SHGetSpecialFolderPath(NULL, buf, csidl, FALSE);
	return buf;
}

//사용자 문서 폴더는 "C:\Users\user_id\Documents" 와 같이 윈도우 로그인 계정인 user_id에 따라 경로가 달라지므로
//"__user_documents_folder__"라는 키워드가 있다면 이를 "C:\Users\user_id\Documents"로 변경해준다.
CString	adjust_special_folder_path(CString path)
{
	if (path.Find(_T("__user_documents_folder__")) == 0)
		path.Replace(_T("__user_documents_folder__"), get_known_folder(CSIDL_MYDOCUMENTS));

	return path;
}

void ParseCommandString(CString sParam, CStringArray& ar)
{
//	AfxMessageBox("sParam = [" + sParam + "]");

	sParam = sParam.Right(sParam.GetLength() - sParam.Find(_T("\""), 1) - 2);

	sParam.TrimLeft();
	sParam.TrimRight();

	if (sParam == "")
		return;

	sParam.Replace(_T("\""), _T(""));
//	AfxMessageBox("sParam = [" + sParam + "]");

	// d:\1 2.png d:\3 4.png
	CString sSub = _T("");

	int		nIndex;
	
	while (TRUE)
	{
		nIndex = sParam.Find(_T(":\\"), 3);

		if (nIndex < 0)
		{
			ar.Add(sParam);
			break;
		}

		sSub = sParam.Left(nIndex - 2);
		ar.Add(sSub);
//		AfxMessageBox("sSub = " + sSub);
		sParam = sParam.Right(sParam.GetLength() - sSub.GetLength() - 1);
	}

}

//타이머에 의해 호출된 함수내에서 사용할 경우에는 제대로 동작하지 않음.
void Wait(DWORD dwMillisecond)
{
    MSG		msg;
    DWORD	dwStart;
	
    dwStart = GetTickCount();

    while (GetTickCount() - dwStart < dwMillisecond)
    {
		//TRACE("tick = %d\n", GetTickCount());
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

//반복문에 의해 process가 응답없음이 되지 않도록 반복문안에서 호출하여 메시지큐의 내용을 바로 처리시킨다.
void ProcessWindowMessage()
{
   MSG msg;

   while(::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
   {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
   }
}

// 16비트 정수를 2진수 문자열로 변환 함수
char*	ushortToBinary(unsigned short i)
{
	static char s[16 + 1] = { '0', };
	int count = 16;
	
	do { s[--count] = '0' + (char) (i & 1);
	i = i >> 1;
	} while (count);
	
	return s;
}

char *intToBinary(int i) {
  static char s[32 + 1] = { '0', };
  int count = 32;

  do { s[--count] = '0' + (char) (i & 1);
       i = i >> 1;
  } while (count);

  return s;
}


char *uintToBinary(unsigned int i) {
  static char s[32 + 1] = { '0', };
  int count = 32;

  do { s[--count] = '0' + (char) (i & 1);
       i = i >> 1;
  } while (count);

  return s;
}



int binaryToInt(char *s) {
  int i = 0;
  int count = 0;

  while (s[count])
    i = (i << 1) | (s[count++] - '0');

  return i;
}


unsigned int binaryToUint(char *s) {
  unsigned int i = 0;
  int count = 0;

  while (s[count])
    i = (i << 1) | (s[count++] - '0');

  return i;
}

//어떤 수의 x 인덱스 위치의 비트값을 리턴.
int get_bit(int number, int x)
{
	int bit = (number >> x) & 1;
	return bit;
}

//x가 1이면 n번째 값을 1로 변경, x가 0이면 n번째 값을 0으로 변경
void set_bit(int& number, int n, int x)
{
	number ^= (-x ^ number) & (1 << n);
}

void make_rect(CRect &Rect, int x, int y, int w, int h)
{
	Rect.left	= x;
	Rect.top	= y;
	Rect.right	= x + w;
	Rect.bottom	= y + h;
}

CRect make_rect(int x, int y, int w, int h)
{
	return CRect(x, y, x+w, y+h);
}

CRect make_center_rect(int cx, int cy, int w, int h)
{
	CRect result;
	result.left		= cx - w/2;
	result.top		= cy - h/2;
	result.right	= result.left + w;
	result.bottom	= result.top + h;
	return result;
}

Gdiplus::Rect makeCenterGpRect(int cx, int cy, int w, int h)
{
	Gdiplus::Rect result;
	result.X = cx - w / 2;
	result.Y = cy - h / 2;
	result.Width = w;
	result.Height = h;
	return result;
}

CRect getCenterRect(int cx, int cy, int w, int h)
{
	CRect r;
	r.left = cx - w/2;
	r.right = r.left + w;
	r.top = cy - h/2;
	r.bottom = r.top + h;

	return r;
}

CRect gpRectToCRect(Gdiplus::Rect r)
{
	return CRect(r.X, r.Y, r.X + r.Width, r.Y + r.Height);
}

CRect GpRectF2CRect(Gdiplus::RectF r)
{
	return CRect(r.X, r.Y, r.X + r.Width, r.Y + r.Height);
}

Gdiplus::Rect CRect2GpRect(CRect r)
{
	return Gdiplus::Rect(r.left, r.top, r.Width(), r.Height());
}

Gdiplus::RectF CRect2GpRectF(CRect r)
{
	return Gdiplus::RectF(r.left, r.top, r.Width(), r.Height());
}

//Gdiplus::RectF는 right 또는 x2가 없고 x(left)와 Width 멤버변수만 존재힌다.
//따라서 left만 바꾸고 싶어도 Width까지 같이 변경해줘야 한다. 이러한 이유로 set_left(), set_top() 함수를 추가함.
//CRect는 left를 변경하면 Width()가 변경되지만 Gdiplus::Rect는 X를 변경해도 Width는 변경되지 않는다.
void set_left(Gdiplus::RectF& r, Gdiplus::REAL left)
{
	Gdiplus::REAL right = r.X + r.Width;
	r.X = left;
	r.Width = right - left;
}

void set_top(Gdiplus::RectF& r, Gdiplus::REAL top)
{
	Gdiplus::REAL bottom = r.Y + r.Height;
	r.Y = top;
	r.Height = bottom - top;
}

Gdiplus::PointF center(Gdiplus::RectF& r)
{
	return Gdiplus::PointF(r.X + r.Width / 2.0, r.Y + r.Height / 2.0);
}

void get_round_rect_path(Gdiplus::GraphicsPath* path, Gdiplus::Rect r, int radius)
{
	//path->SetFillMode(Gdiplus::FillModeWinding);

	if (radius <= 0)
	{
		path->AddRectangle(r);
		path->CloseFigure();
		return;
	}

	radius = MIN(radius, MIN(r.Width, r.Height) / 2.0);

	float diameter = radius;// *2.0F;

	Gdiplus::RectF arc(r.X, r.Y, diameter, diameter);

	//https://www.codeproject.com/Articles/1010822/RoundedButton-Control-Demystifying-DrawArc
	//Arc를 그리는 차례는 tl-bl-br-tr가 아니면 round rect가 그려지지 않는다.
	//top-left round corder. 180도 위치에서 시계방향으로 90도 만큼의 호를 그린다.
	if (true)
	{
		path->AddArc(arc, 180.0, 90.0);
	}
	else
	{
		//path->AddLine(r.X + );
	}

	//bottom-left round corder. 270도 위치에서 시계방향으로 90도 만큼의 호를 그린다.
	arc.X = r.GetRight() - diameter - 1;
	path->AddArc(arc, 270.0, 90.0);

	//bottom-right round corder. 0도 위치에서 시계방향으로 90도 만큼의 호를 그린다.
	arc.Y = r.GetBottom() - diameter - 1;
	path->AddArc(arc, 0.0, 90.0);

	//top-right round corder. 90도 위치에서 시계방향으로 90도 만큼의 호를 그린다.
	arc.X = r.GetLeft();
	path->AddArc(arc, 90.0, 90.0);

	//그려진 네귀퉁이의 호를 연결시킨다.
	path->CloseFigure();
}

void draw_round_rect(Gdiplus::Graphics* g, Gdiplus::Rect r, Gdiplus::Color gcr_stroke, Gdiplus::Color gcr_fill, int radius, int width)
{
	int dia = 2 * radius;

	// set to pixel mode
	int oldPageUnit = g->SetPageUnit(Gdiplus::UnitPixel);

	// define the pen
	Gdiplus::Pen pen(gcr_stroke, 1);
	Gdiplus::SolidBrush br(gcr_fill);

	pen.SetAlignment(Gdiplus::PenAlignmentCenter);

	// get the corner path
	Gdiplus::GraphicsPath path;

	// get path
	get_round_rect_path(&path, r, dia);

	//fill the round rect
	g->FillPath(&br, &path);

	// draw the round rect
	g->DrawPath(&pen, &path);

	// if width > 1
	for (int i = 1; i < width; i++)
	{
		// left stroke
		r.Inflate(-1, 0);
		// get the path
		get_round_rect_path(&path, r, dia);

		// draw the round rect
		g->DrawPath(&pen, &path);

		// up stroke
		r.Inflate(0, -1);

		// get the path
		get_round_rect_path(&path, r, dia);

		// draw the round rect
		g->DrawPath(&pen, &path);
	}

	// restore page unit
	g->SetPageUnit((Gdiplus::Unit)oldPageUnit);
}

CRect get_zoom_rect(CRect rect, double zoom)
{
	double l, t, r, b;
	l = (double)rect.left * zoom;
	t = (double)rect.top * zoom;
	r = (double)rect.right * zoom;
	b = (double)rect.bottom * zoom;
	return CRect(l, t, r, b);
}

//0:lt, 1:rt, 2:rb, 3:lb, rb_cut이 true이면 끝점-1인 값을 리턴하고 false이면 끝점 좌표를 리턴한다.
//실제 사각형은 rb_cut이 false이지만
//영역 또는 픽셀을 본다면 true로 해야 맞다.
CPoint vertex(CRect r, int index, bool rb_cut)
{
	switch (index)
	{
		case 0: return CPoint(r.left, r.top);
		case 1: return CPoint(r.right - rb_cut, r.top);
		case 2: return CPoint(r.right - rb_cut, r.bottom - rb_cut);
		case 3: return CPoint(r.left, r.bottom - rb_cut);
	}

	return CPoint();
}


// 파일에 대한 쇼트컷을 만든다.
// 아래의 함수와 거의 동일하다.
// pszSrcFile:숏컷 대상 파일의 풀 패스
// pszLnkFile:숏컷 파일의 풀 패스
// pszArgument:쇼트컷에 전달될 인수 문자열
// pszDesc:쇼트컷 설명 문자열
HRESULT	MyCreateShortCut(LPCTSTR pszSrcFile, LPCOLESTR pszLnkFile,
						  LPTSTR pszWorkingDir/* = NULL*/, LPTSTR pszArgument/* = NULL*/, LPTSTR pszDesc/* = NULL*/)
{
	HRESULT			hr;
	IShellLink		*pSl;
	IPersistFile	*pPf;
	TCHAR wszLnkFile[MAX_PATH]={0,};
	
	// IShellLink 객체를 생성하고 포인터를 구한다.
	CoInitialize(NULL);

	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		 IID_IShellLink, (void **)&pSl);
	
	if (FAILED(hr))
		return E_FAIL;
	
	// 쇼트컷의 대상체와 설명을 설정한다.
	pSl->SetPath(pszSrcFile);
	pSl->SetArguments(pszArgument);
	pSl->SetDescription(pszDesc);
	pSl->SetWorkingDirectory(pszWorkingDir);
	
	// 저장하기 위해 IPersistFile 객체를 생성한다.
	hr = pSl->QueryInterface(IID_IPersistFile, (void **)&pPf);

	if (FAILED(hr))
	{
		pSl->Release();
		return E_FAIL;
	}
	
	// 유니코드로 파일 패스를 변경한 후 저장한다.
	//MultiByteToWideChar(CP_ACP, 0, pszLnkFile, -1, wszLnkFile, MAX_PATH);
	hr=pPf->Save(pszLnkFile, TRUE);
	
	// 객체 해제
	pPf->Release();
	pSl->Release();

	CoUninitialize();
	return hr;
}

//
//
//
HRESULT	CreateShortCut(LPCTSTR pszShortcutFile, LPCOLESTR pszLink,
						LPCTSTR pszWorkingDir/* = ""*/, LPCTSTR pszArgument/* = ""*/, LPCTSTR pszDesc/* = ""*/)
{
	HRESULT hres;
	IShellLink *psl;
	
	//unlink(pszLink);
	// Create an IShellLink object and get a pointer to the IShellLink
	// interface (returned from CoCreateInstance).
	CoInitialize(NULL);

	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		IID_IShellLink, (void **)&psl);

	if (SUCCEEDED(hres))
	{
		IPersistFile *ppf;
		
		// Query IShellLink for the IPersistFile interface for
		// saving the shortcut in persistent storage.
		hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
		if (SUCCEEDED(hres))
		{   
			// Set the path to the shortcut target.
			hres = psl->SetPath(pszShortcutFile);
			hres = psl->SetArguments(pszArgument);
			// Set the description of the shortcut.
			hres = psl->SetDescription(pszDesc);
			hres = psl->SetWorkingDirectory(pszWorkingDir);
			
			// Ensure that the string consists of ANSI characters.
			//MultiByteToWideChar(CP_ACP, 0, pszLink, -1, (LPWSTR)wsz, MAX_PATH);
			
			// Save the shortcut via the IPersistFile::Save member function.
			hres = ppf->Save(pszLink, TRUE);
			
			// Release the pointer to IPersistFile.
			ppf->Release();
		}
		// Release the pointer to IShellLink.
		psl->Release();
	}

	CoUninitialize();
	
	return hres;
}


void HideTaskBar(bool bHide /*=TRUE*/)
{
	CRect rectWorkArea = CRect(0,0,0,0);
	CRect rectTaskBar = CRect(0,0,0,0);
	
	CWnd* pWnd = CWnd::FindWindow(_T("Shell_TrayWnd"), _T(""));
	
	if(bHide)
	{
		// Code to Hide the System Task Bar
/*		SystemParametersInfo(SPI_GETWORKAREA,
			0,
			(LPVOID)&rectWorkArea,
			0);
		
		if(pWnd)
		{
			pWnd->GetWindowRect(rectTaskBar);
			rectWorkArea.bottom += rectTaskBar.Height();
			SystemParametersInfo(SPI_SETWORKAREA,
				0,
				(LPVOID)&rectWorkArea,
				0);
			
			pWnd->ShowWindow(SW_HIDE);
		}
*/
		pWnd->ShowWindow(SW_HIDE);
	}
	else
	{
		// Code to Show the System Task Bar
/*		SystemParametersInfo(SPI_GETWORKAREA,
			0,
			(LPVOID)&rectWorkArea,
			0);
		if(pWnd)
		{
			pWnd->GetWindowRect(rectTaskBar);
			rectWorkArea.bottom -= rectTaskBar.Height();
			SystemParametersInfo(SPI_SETWORKAREA,
				0,
				(LPVOID)&rectWorkArea,
				0);
			
			pWnd->ShowWindow(SW_SHOW);
		}
*/
		pWnd->ShowWindow(SW_SHOW);
	}
}

//현재 실행파일의 버전관련 정보 문자열을 얻어온다.
//rc->Version->VS_VERSION_INFO 항목
//ex. m_sVersion = GetFileVersionInformation(sExeFile, "FileVersion");
//아래의 GetFileProperty 함수도 동일한 기능을 수행하는 코드이나 각 항목별로 확인 필요!
CString get_file_property(CString fullpath, CString strFlag)
{
	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	if (fullpath.IsEmpty())
		fullpath = get_exe_filename(true);

	//.ico 파일과 같은 파일들은 dwSize가 0이므로 더 이상 실행은 의미없다.
	DWORD dwSize = GetFileVersionInfoSize(fullpath, 0);
	if (dwSize <= 0)
		return _T("");

	TCHAR * buffer = new TCHAR[dwSize];
	memset(buffer, 0, dwSize);

	GetFileVersionInfo(fullpath, 0, dwSize, buffer);

	UINT cbTranslate;
	UINT dwBytes;

	LPBYTE lpBuffer = NULL;

	VerQueryValue(buffer, _T("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate);

	if(cbTranslate != 0) // 버전 정보가 없을 경우 "
	{
		CString strSub;
		strSub.Format(_T("\\StringFileInfo\\%04x%04x\\%s"), lpTranslate[0].wLanguage, lpTranslate[0].wCodePage, strFlag);
		VerQueryValue(buffer, (LPTSTR)(LPCTSTR)strSub, (LPVOID*)&lpBuffer, &dwBytes); 
	}

	CString strReturn;
	strReturn.Format(_T("%s"), lpBuffer);

	delete [] buffer;

	return strReturn;
} 

#ifndef _USING_V110_SDK71_
bool show_property_window(std::deque<CString> fullpath)
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		AfxMessageBox(_T("fail to CoInitialize() to show property window."));
		return false;
	}

	//"내 PC"를 선택한 경우 시스템 속성 창을 열어준다.
	if (fullpath.size() == 1)
	{
		if (fullpath[0] == get_system_label(CSIDL_DRIVES))
		{
			//탐색기에서 "내 PC" 우클릭하여 속성창을 열 때와 동일한 코드.
			ShellExecute(NULL, _T("properties"), _T("shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), NULL, NULL, SW_SHOWNORMAL);

			//5개의 탭으로 구성된 간단한 시스템 속성창을 표시한다.
			//ShellExecute(NULL, _T("open"), _T("SystemPropertiesComputerName.exe"), NULL, NULL, SW_SHOW);

			//탐색기에서 "내 PC" 우클릭하여 속성을 선택하면 보여지는 창은 작업관리자로 보면 다음 명령어와 파라미터로 실행되나
			//정작 cmd창에서는 실행되지 않고 아래와 같은 명령으로도 실행되지 않는다.
			//ShellExecute(NULL, _T("open"), _T("C:\\Windows\\ImmersiveControlPanel\\SystemSettings.exe"), _T("-ServerName:microsoft.windows.immersivecontrolpanel"), NULL, SW_SHOWNORMAL);
		}
		else
		{
			//처음 실행후에는 SHMultiFileProperties()가 잘 동작하나 한두번 파일전송후에는
			//local에서 SHMultiFileProperties()가 에러를 표시하며 제대로 동작하지 않는다.
			//그럼에도 1개 파일에 대해서는 정상 표시되므로 우선 1개일때는 아래 함수를 호출한다.
			//SHMultiFileProperties() 호출후에 인위적인 딜레이를 줘서 아직까지는 에러가 발생하는 현상이 없으므로
			//더 이상 에러창이 뜨지 않는다면 1개 항목일때의 처리를 별도로 하지 않아도 됨.
			SHObjectProperties(NULL, SHOP_FILEPATH, (CStringW)fullpath[0], NULL);
		}

		CoUninitialize();
		return true;
	}

	int i;
	//LPITEMIDLIST* pidl = (LPITEMIDLIST*)malloc(sizeof(LPITEMIDLIST) * fullpath.size());
	LPITEMIDLIST* pidl = new LPITEMIDLIST[fullpath.size()];
	IShellFolder* pDesktop;
	//IDataObject* pDataObject;

	if (!pidl)
	{
		AfxMessageBox(_T("pidl is null."));
		CoUninitialize();
		return false;
	}

	hr = SHGetDesktopFolder(&pDesktop);
	if (FAILED(hr))
	{
#ifdef _DEBUG
		AfxMessageBox(_T("SHGetDesktopFolder() fail"));
#endif
		CoUninitialize();
		return false;
	}

	for (int i = 0; i < fullpath.size(); i++)
	{
		//hr = pDesktop->ParseDisplayName(/*AfxGetApp()->GetMainWnd()->GetSafeHwnd()*/NULL, NULL, (LPWSTR)(LPCTSTR)fullpath[i], NULL, (LPITEMIDLIST*)&pidl[i], NULL);
		//hr = pDesktop->ParseDisplayName(GetDesktopWindow(), NULL, (LPWSTR)(LPCTSTR)fullpath[i], NULL, &pidl[i], NULL);
		SFGAOF sfgao;
		hr = SHParseDisplayName((CStringW)fullpath[i], NULL, &pidl[i], 0, &sfgao);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			AfxMessageBox(_T("pDesktop->ParseDisplayName() fail"));
#endif
			pDesktop->Release();
			CoUninitialize();
			return false;
		}
	}

	// 4. PIDL 배열을 이용하여 IShellItemArray 생성
	IShellItemArray* psia = nullptr;
	hr = SHCreateShellItemArrayFromIDLists(fullpath.size(), (LPCITEMIDLIST*)pidl, &psia);
	if (FAILED(hr) || !psia)
	{
		AfxMessageBox(_T("SHCreateShellItemArrayFromIDLists 실패"));
		for (int i = 0; i < fullpath.size(); i++)
		{
			if (pidl[i])
				CoTaskMemFree(pidl[i]);
		}
		delete[] pidl;
		CoUninitialize();
		return 1;
	}

	// 5. IShellItemArray에서 IDataObject 얻기
	IDataObject* pdtobj = nullptr;
	hr = psia->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARGS(&pdtobj));
	if (FAILED(hr) || !pdtobj)
	{
		AfxMessageBox(_T("IShellItemArray로부터 IDataObject 획득 실패"));
		psia->Release();
		for (int i = 0; i < fullpath.size(); i++)
		{
			if (pidl[i])
				CoTaskMemFree(pidl[i]);
		}
		delete[] pidl;
		CoUninitialize();
		return 1;
	}

	//hr = pDesktop->GetUIObjectOf(GetDesktopWindow(), fullpath.size(), (LPCITEMIDLIST*)pidl, IID_IDataObject, NULL, (void**)&pDataObject);
	//hr = pDesktop->GetUIObjectOf(GetDesktopWindow(), fullpath.size(), (LPCITEMIDLIST*)pidl, IID_IDataObject, NULL, (void**)&pDataObject);
	//pDesktop->Release();
	// alternatively, you can also use SHCreateDataObject() or CIDLData_CreateFromIDArray() to create the IDataObject

	if (SUCCEEDED(hr))
	{
		//hr = SHMultiFileProperties(pDataObject, 0);
		hr = SHMultiFileProperties(pdtobj, 0);
		//SHMultiFileProperties()을 호출한 후 OS에서 표시할 시간을 약간 준다는 정보를 어디선가 본 듯하다.
		//간혹 "속성창을 열 수 없다"는 오류가 표시되는 현상이 있었는데 인위적인 딜레이를 주니 아직까지는 발생하지 않았다.
		//좀 더 테스트가 필요하다.
		Wait(500);
		pdtobj->Release();
		psia->Release();

		if (SUCCEEDED(hr))
		{
			//MessageBox(0, _T("Dummy message box"), 0, 0);
			//Sleep(2500); // Give the system time to show the dialog before exiting
		}

	}

	for (i = 0; i < fullpath.size(); i++)
		//ILFree(pidl[i]);
		//SHFree(pidl[i]);
		CoTaskMemFree(pidl[i]);
	delete[] pidl;

	CoUninitialize();

	return true;
}
#endif

//명시된 FileVersion 또는 ProductVersion을 얻어온다.
CString	GetFileProperty(CString sFilePath, CString sProperty)
{
	DWORD cbVerInfo, dummy;
	CString sResult = _T("");

	// How big is the version info? 
	cbVerInfo = GetFileVersionInfoSize(sFilePath, &dummy); 

	if (!cbVerInfo) 
		return _T("");

	// Allocate space to hold the info 
	PBYTE pVerInfo = new BYTE[cbVerInfo]; 

	if (!pVerInfo)
		return _T("");

	//try 
	{ 
		if (!GetFileVersionInfo(sFilePath, 0, cbVerInfo, pVerInfo)) 
		{
			delete [] pVerInfo; 
			return _T("");
		}

		TCHAR szQueryStr[ 0x100 ]; 

		// Format the string with the 1200 codepage (Unicode) 
		wsprintf(szQueryStr, _T("\\StringFileInfo\\%04X%04X\\%s"), GetUserDefaultLangID(), 1200, sProperty); 

		PSTR pszVerRetVal; 
		UINT cbReturn; 
		bool fFound; 

		fFound = VerQueryValue(pVerInfo, szQueryStr, (LPVOID *)&pszVerRetVal, &cbReturn); 

		if (fFound && pszVerRetVal)
		{
			TCHAR sz[1024];
			wsprintf(sz, _T("%s"), pszVerRetVal); 
			sResult = sz;
		} 
	} 

	delete [] pVerInfo;

	return sResult;
}

CString get_file_time_str(FILETIME filetime)
{
	SYSTEMTIME st;
	FILETIME ftLocal;

	FileTimeToLocalFileTime(&(filetime), &ftLocal);
	FileTimeToSystemTime(&ftLocal, &st);

	return get_datetime_str(st, 2, true, _T(" "), false, false, false);
}

size_t read_raw(CString sfile, uint8_t *dst, size_t size)
{
	FILE *fp = _tfopen(sfile, _T("rb"));

	if (fp == NULL)
		return false;

	size_t read_size = fread(dst, size, 1, fp);
	fclose(fp);

	return read_size;
}

bool save2raw(CString sfile, uint8_t *data, size_t size)
{
	FILE *fp = _tfopen(sfile, _T("wb"));
	
	if (fp == NULL)
		return false;

	fwrite(data, size, 1, fp);
	fclose(fp);

	return true;
}

//해당 폴더의 이전 파일명 전체 또는 일부분을 새로운 이름으로 변경한다.
//단, 확장자는 포함되지 않는다.
//따라서 호출할 때 아예 oldName과 newName에는 확장자는 있어서는 안된다.
//주의!
//bWholename이 false라고 해도 "*%s*"와 같이 앞에도 *를 붙이면
//원치 않는 결과가 발생할 수 있으니 반드시 디버깅으로 확인한 후 사용할 필요가 있다.
//rename이 성공한 파일의 개수를 리턴한다.
int RenameFiles(CString folder, CString oldName, CString newName, bool overwrite, bool bWholename, bool bRecursive)
{
	std::deque<CString> files;
	CString names;
	CString newFilename;
	int success = 0;

	if (bWholename)
		names = oldName;
	else
		names.Format(_T("%s*"), oldName);

	FindAllFiles(folder, &files, names, _T("*"), bRecursive, false, _T(""), false);

	for (int i = 0; i < files.size(); i++)
	{
		//검색된 파일명에서 oldName 다음에 .과 _Snapshot 이 두개의 패턴은 같은 파일군으로 처리한다.
		if (files[i].Find(oldName + _T(".")) >= 0 ||
			files[i].Find(oldName + _T("_Snapshot")) >= 0)
		{
			newFilename = files[i];
			newFilename.Replace(oldName, newName);

			bool bSuccess = false;

			if (overwrite)
				bSuccess = MoveFileEx(files[i], newFilename, MOVEFILE_REPLACE_EXISTING);
			else
				bSuccess = MoveFile(files[i], newFilename);

			if (bSuccess)
				success++;
			else
				get_error_str(GetLastError());
		}
	}

	return success;
}

//mp4 파일의 특정 태그 데이터 중 원하는 위치의 데이터를 추출한다.
//MOBIS 프로젝트 저장 MP4는 mdat 필드의 0x40번지부터 n bytes가
//동영상이 생성된 절대시간이 저장되어 있다.
//n은 0x3C부터 4바이트가 그 크기이다.
char* GetDataFromMP4File(char* sfile, char* sTag, uint8_t tagStart, int tagLength)
{
	FILE	*fp = fopen(sfile, "rb");
	if (fp == NULL)
		return NULL;

	int		size;
	char	type[6][4];
	char	*pData = NULL;

	//4~7까지 4바이트가 "ftyp"이어야 mp4 파일이다.
	fseek(fp, 4, SEEK_SET);
	fread(type[0], 4, 1, fp);

	if (strncmp(type[0], "ftyp", 4) == 0)
	{
		fseek(fp, 0, SEEK_SET);
	}
	else
	{
		fclose(fp);
		return NULL;
	}

	while(!feof(fp))
	{
		fread(&size, sizeof(int32_t), 1, fp);
		fread(type[0], 4, 1, fp);
		size = Swap32BE(size);
		if (strncmp(type[0], sTag, 4) == 0)
		{
			fseek(fp, tagStart, SEEK_SET);
			fread(&size, 1, tagLength, fp);
			size = Swap32BE(size);

			//데이터의 크기는 28바이트이어야 한다.
			//다른 값이라면 timeStamp가 아니거나 없는 동영상이다.
			if (size != 28)
			{
				fclose(fp);
				return NULL;
			}

			pData = new char[size];
			fread(pData, 1, size, fp);

			fclose(fp);
			return pData;
		}
		else
		{
			fseek(fp, size - 8, SEEK_CUR);
		}

		continue;
	}

	fclose(fp);
	return NULL;
}

bool is_binary(CString sfile)
{
	int text_encoding = get_text_encoding(sfile);

	if (text_encoding == text_encoding_unknown)
		return true;

	return false;
}

void watch_file_system(CString fullpath)
{

}

CStringA UTF16toUTF8(const CStringW& utf16)
{
	CStringA utf8;
	int len = WideCharToMultiByte(CP_ACP, 0, utf16, -1, NULL, 0, 0, 0);
	if (len > 1)
	{
		char* ptr = utf8.GetBuffer(len - 1);
		if (ptr) WideCharToMultiByte(CP_ACP, 0, utf16, -1, ptr, len, 0, 0);
		utf8.ReleaseBuffer();
	}
	return utf8;
}

CStringW UTF8toUTF16(const CStringA& utf8)
{
	CStringW utf16;
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (len > 1)
	{
		wchar_t* ptr = utf16.GetBuffer(len - 1);
		if (ptr) MultiByteToWideChar(CP_UTF8, 0, utf8, -1, ptr, len);
		utf16.ReleaseBuffer();
	}
	return utf16;
}

CString UTF8toCString(char* pszCode)
{
	BSTR    bstrWide;
	char* pszAnsi;
	int     nLength;
	// Get nLength of the Wide Char buffer
	nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, NULL, NULL);
	bstrWide = SysAllocStringLen(NULL, nLength);
	// Change UTF-8 to Unicode (UTF-16)
	MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, bstrWide, nLength);
	// Get nLength of the multi byte buffer
	nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
	pszAnsi = new char[nLength];
	ZeroMemory(pszAnsi, nLength);
	// Change from unicode to mult byte
	WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
	SysFreeString(bstrWide);

	CString strResult = (CString)pszAnsi;
	delete[] pszAnsi;

	return strResult;
}

char* UTF8toANSI(char* pszCode)
{
	BSTR    bstrWide;
	char* pszAnsi;
	int     nLength;
	// Get nLength of the Wide Char buffer
	nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, NULL, NULL);
	bstrWide = SysAllocStringLen(NULL, nLength);
	// Change UTF-8 to Unicode (UTF-16)
	MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, bstrWide, nLength);
	// Get nLength of the multi byte buffer
	nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
	pszAnsi = new char[nLength];
	ZeroMemory(pszAnsi, nLength);
	// Change from unicode to mult byte
	WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
	SysFreeString(bstrWide);
	return pszAnsi;
}

char* ANSItoUTF8(char* pszCode)
{
	BSTR bstrCode;
	char* pszUTFCode = NULL;
	int  nLength, nLength2;
	nLength = MultiByteToWideChar(CP_ACP, 0, pszCode, strlen(pszCode), NULL, NULL);
	bstrCode = SysAllocStringLen(NULL, nLength);
	MultiByteToWideChar(CP_ACP, 0, pszCode, strlen(pszCode), bstrCode, nLength);
	nLength2 = WideCharToMultiByte(CP_UTF8, 0, bstrCode, -1, pszUTFCode, 0, NULL, NULL);
	pszUTFCode = new char[nLength2 + 1];
	ZeroMemory(pszUTFCode, nLength2 + 1);
	WideCharToMultiByte(CP_UTF8, 0, bstrCode, -1, pszUTFCode, nLength2, NULL, NULL);
	return pszUTFCode;
}

CString utf8ToCString(std::string inputtext)
{
#ifdef _UNICODE
	return CString(utf8ToUnicode(inputtext).c_str());
#else
	return CString(utf8ToMultibyte(inputtext).c_str());
#endif // _UNICODE
}

std::string CStringToUtf8(CString inputtext)
{
#ifdef _UNICODE
	return unicodeToUtf8(inputtext.operator LPCWSTR());
#else
	return multibyteToUtf8(inputtext.operator LPCSTR());
#endif
}

/*
char* Utf8Encode(IN LPCTSTR szText)
{
	USES_CONVERSION;
	// str이 Unicode인지 Ansi 인지 따질 필요없게 T2CW로 변환    
	const WCHAR* wStr = T2CW(szText);    
	
	// 길이는 -1로 주어 널NULL 문자도 변환되도록
	//WCHAR -> UTF-8
	
	int nUTF8codeSize = WideCharToMultiByte(CP_UTF8, 0, wStr, -1, NULL, 0, NULL, NULL); //wStr의 크기를 구함
	char *utf8Str = new char[nUTF8codeSize];
	ZeroMemory(utf8Str, nUTF8codeSize);
	
	WideCharToMultiByte(CP_UTF8, 0, wStr, -1, utf8Str, nUTF8codeSize, 0, 0);    
	
	return utf8Str;
}

void Utf8Decode(IN OUT BYTE* pbyData, IN OUT int& nSize)
{
	char* szMsg = new char[nSize + 1];
	CopyMemory(szMsg, pbyData, nSize);
	szMsg[nSize] = '\0';
	
	int size = MultiByteToWideChar(CP_UTF8, 0,  szMsg, -1, NULL, 0);
	LPWSTR wStr = new WCHAR[size];
	ZeroMemory(wStr, sizeof(WCHAR) * size);
	
	MultiByteToWideChar(CP_UTF8, 0,  szMsg, -1, wStr, size); 
	
	USES_CONVERSION;
	LPSTR str = W2A(wStr);
	
	nSize = strlen(str);
	CopyMemory(pbyData, str, nSize);
	
	delete[] szMsg;
	delete[] wStr;
}
*/


/*
//Enable DEBUG privilege
bool EnableDebugPrivilege(void)
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;

	// enable the SeDebugPrivilege
	if (! OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		_tprintf(_T("OpenProcessToken() failed, Error = %d SeDebugPrivilege is not available.\n"), GetLastError());
		return FALSE;
	}

	if (! LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue))
	{
		_tprintf(_T("LookupPrivilegeValue() failed, Error = %d SeDebugPrivilege is not available.\n"), GetLastError());
		CloseHandle(hToken);
		return FALSE;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (! AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof tkp, NULL, NULL))
		_tprintf(_T("AdjustTokenPrivileges() failed, Error = %d SeDebugPrivilege is not available.\n"), GetLastError());
		
	return CloseHandle(hToken);
}

//Closes the file handles in the processes which are using this file
bool CloseRemoteFileHandles(LPCTSTR lpFileName)
{
	CString deviceFileName;
	CString fsFilePath;
	CString name;
	CString processName;
	SystemHandleInformation hi;
	SystemProcessInformation pi;
	SystemProcessInformation::SYSTEM_PROCESS_INFORMATION* pPi;
	
	//Convert it to device file name
	if (!SystemInfoUtils::GetDeviceFileName(lpFileName, deviceFileName))
	{
		_tprintf(_T("GetDeviceFileName() failed.\n"));
		return FALSE;
	}
	
	//Query every file handle (system wide)
	if (!hi.SetFilter(_T("File"), TRUE))
	{
		_tprintf(_T("SystemHandleInformation::SetFilter() failed.\n"));
		return FALSE;
	}

	if (!pi.Refresh())
	{
		_tprintf(_T("SystemProcessInformation::Refresh() failed.\n"));
		return FALSE;
	}

	//Iterate through the found file handles
	for (POSITION pos = hi.m_HandleInfos.GetHeadPosition(); pos != NULL;)
	{
		SystemHandleInformation::SYSTEM_HANDLE& h = hi.m_HandleInfos.GetNext(pos);

		if (!pi.m_ProcessInfos.Lookup(h.ProcessID, pPi))
			continue;

		if (pPi == NULL)
			continue;

		//Get the process name
		SystemInfoUtils::Unicode2CString(&pPi->usName, processName);

		//NT4 Stupid thing if I query the name of a file in services.exe
		//Messengr service brings up a message dialog ??? :(
		if (INtDll::dwNTMajorVersion == 4 && _tcsicmp(processName, _T("services.exe")) == 0)
			continue;
		
		//what's the file name for this given handle?
		hi.GetName((HANDLE)h.HandleNumber, name, h.ProcessID);

		//This is what we want to delete, so close the handle
		if (_tcsicmp(name, deviceFileName) == 0)
			CloseRemoteHandle(processName, h.ProcessID, (HANDLE)h.HandleNumber);
	}

	return TRUE;
}

//Close a handle in a remote process
DWORD CloseRemoteHandle(LPCTSTR lpProcessName, DWORD processID, HANDLE handle)
{
	HANDLE ht = 0;
	DWORD rc = 0;
	
	_tprintf(_T("Closing handle in process #%d (%s) ... "), processID, lpProcessName);

	// open the process
	HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
			PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, processID);
	
	if (hProcess == NULL)
	{
		rc = GetLastError();
		_tprintf(_T("OpenProcess() failed\n"));
		return rc;
	}

	// load kernel32.dll
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));

	// CreateRemoteThread()
	ht = CreateRemoteThread(
			hProcess, 
			0, 
			0, 
			(DWORD (__stdcall *)(void *))GetProcAddress(hKernel32, "CloseHandle"),
			handle, 
			0, 
			&rc);
	
	if (ht == NULL)
	{
		//Something is wrong with the privileges, or the process doesn't like us
		rc = GetLastError();
		_tprintf(_T("CreateRemoteThread() failed\n"));
		goto cleanup;
	}

	switch (WaitForSingleObject(ht, 2000))
	{
	case WAIT_OBJECT_0:
		//Well done
		rc = 0;
		_tprintf(_T("Ok\n"), rc);
		
		break;
	
	default:
		//Oooops, shouldn't be here
		rc = GetLastError();
		_tprintf(_T("WaitForSingleObject() failed\n"));
		goto cleanup;

		break;
	}

cleanup:

	//Closes the remote thread handle
	CloseHandle(ht);

	//Free up the kernel32.dll
	if (hKernel32 != NULL)
		FreeLibrary(hKernel32);

	//Close the process handle
	CloseHandle(hProcess);
		
	return rc;
}

//Deletes the file
bool DeleteTheFile(LPCTSTR lpFileName)
{
	//Deletes the file
	bool rc = ::DeleteFile(lpFileName);
	
	if (rc)
		_tprintf(_T("Successfully deleted.\n"));
	else
		_tprintf(_T("Couldn't delete. Error = %d\n"), GetLastError());

	return rc;
}
*/





//
//	BitmapToRegion :	Create a region from the "non-transparent" pixels of a bitmap
//	Author :			Jean-Edouard Lachand-Robert (http://www.geocities.com/Paris/LeftBank/1160/resume.htm), June 1998.
//
//	hBmp :				Source bitmap
//	cTransparentColor :	Color base for the "transparent" pixels (default is black)
//	cTolerance :		Color tolerance for the "transparent" pixels.
//
//	A pixel is assumed to be transparent if the value of each of its 3 components (blue, green and red) is 
//	greater or equal to the corresponding value in cTransparentColor and is lower or equal to the 
//	corresponding value in cTransparentColor + cTolerance.
//
HRGN BitmapToRegion (HBITMAP hBmp, COLORREF cTransparentColor/* = 0*/, COLORREF cTolerance/* = 0x101010*/)
{
	HRGN hRgn = NULL;

	if (hBmp)
	{
		// Create a memory DC inside which we will scan the bitmap content
		HDC hMemDC = CreateCompatibleDC(NULL);
		if (hMemDC)
		{
			// Get bitmap size
			BITMAP bm;
			GetObject(hBmp, sizeof(bm), &bm);

			// Create a 32 bits depth bitmap and select it into the memory DC 
			BITMAPINFOHEADER RGB32BITSBITMAPINFO = {	
					sizeof(BITMAPINFOHEADER),	// biSize 
					bm.bmWidth,					// biWidth; 
					bm.bmHeight,				// biHeight; 
					1,							// biPlanes; 
					32,							// biBitCount 
					BI_RGB,						// biCompression; 
					0,							// biSizeImage; 
					0,							// biXPelsPerMeter; 
					0,							// biYPelsPerMeter; 
					0,							// biClrUsed; 
					0							// biClrImportant; 
			};
			VOID * pbits32; 
			HBITMAP hbm32 = CreateDIBSection(hMemDC, (BITMAPINFO *)&RGB32BITSBITMAPINFO, DIB_RGB_COLORS, &pbits32, NULL, 0);
			if (hbm32)
			{
				HBITMAP holdBmp = (HBITMAP)SelectObject(hMemDC, hbm32);

				// Create a DC just to copy the bitmap into the memory DC
				HDC hDC = CreateCompatibleDC(hMemDC);
				if (hDC)
				{
					// Get how many bytes per row we have for the bitmap bits (rounded up to 32 bits)
					BITMAP bm32;
					GetObject(hbm32, sizeof(bm32), &bm32);
					while (bm32.bmWidthBytes % 4)
						bm32.bmWidthBytes++;

					// Copy the bitmap into the memory DC
					HBITMAP holdBmp = (HBITMAP)SelectObject(hDC, hBmp);
					BitBlt(hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, hDC, 0, 0, SRCCOPY);

					// For better performances, we will use the ExtCreateRegion() function to create the
					// region. This function take a RGNDATA structure on entry. We will add rectangles by
					// amount of ALLOC_UNIT number in this structure.
					#define ALLOC_UNIT	100
					DWORD maxRects = ALLOC_UNIT;
					HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
					RGNDATA *pData = (RGNDATA *)GlobalLock(hData);
					pData->rdh.dwSize = sizeof(RGNDATAHEADER);
					pData->rdh.iType = RDH_RECTANGLES;
					pData->rdh.nCount = pData->rdh.nRgnSize = 0;
					SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);

					// Keep on hand highest and lowest values for the "transparent" pixels
					BYTE lr = GetRValue(cTransparentColor);
					BYTE lg = GetGValue(cTransparentColor);
					BYTE lb = GetBValue(cTransparentColor);
					BYTE hr = std::min(0xff, lr + GetRValue(cTolerance));
					BYTE hg = std::min(0xff, lg + GetGValue(cTolerance));
					BYTE hb = std::min(0xff, lb + GetBValue(cTolerance));

					// Scan each bitmap row from bottom to top (the bitmap is inverted vertically)
					BYTE *p32 = (BYTE *)bm32.bmBits + (bm32.bmHeight - 1) * bm32.bmWidthBytes;
					for (int y = 0; y < bm.bmHeight; y++)
					{
						// Scan each bitmap pixel from left to right
						for (int x = 0; x < bm.bmWidth; x++)
						{
							// Search for a continuous range of "non transparent pixels"
							int x0 = x;
							LONG *p = (LONG *)p32 + x;
							while (x < bm.bmWidth)
							{
								BYTE b = GetRValue(*p);
								if (b >= lr && b <= hr)
								{
									b = GetGValue(*p);
									if (b >= lg && b <= hg)
									{
										b = GetBValue(*p);
										if (b >= lb && b <= hb)
											// This pixel is "transparent"
											break;
									}
								}
								p++;
								x++;
							}

							if (x > x0)
							{
								// Add the pixels (x0, y) to (x, y+1) as a new rectangle in the region
								if (pData->rdh.nCount >= maxRects)
								{
									GlobalUnlock(hData);
									maxRects += ALLOC_UNIT;
									hData = GlobalReAlloc(hData, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), GMEM_MOVEABLE);
									pData = (RGNDATA *)GlobalLock(hData);
								}
								RECT *pr = (RECT *)&pData->Buffer;
								SetRect(&pr[pData->rdh.nCount], x0, y, x, y+1);
								if (x0 < pData->rdh.rcBound.left)
									pData->rdh.rcBound.left = x0;
								if (y < pData->rdh.rcBound.top)
									pData->rdh.rcBound.top = y;
								if (x > pData->rdh.rcBound.right)
									pData->rdh.rcBound.right = x;
								if (y+1 > pData->rdh.rcBound.bottom)
									pData->rdh.rcBound.bottom = y+1;
								pData->rdh.nCount++;

								// On Windows98, ExtCreateRegion() may fail if the number of rectangles is too
								// large (ie: > 4000). Therefore, we have to create the region by multiple steps.
								if (pData->rdh.nCount == 2000)
								{
									HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
									if (hRgn)
									{
										CombineRgn(hRgn, hRgn, h, RGN_OR);
										DeleteObject(h);
									}
									else
										hRgn = h;
									pData->rdh.nCount = 0;
									SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
								}
							}
						}

						// Go to next row (remember, the bitmap is inverted vertically)
						p32 -= bm32.bmWidthBytes;
					}

					// Create or extend the region with the remaining rectangles
					HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
					if (hRgn)
					{
						CombineRgn(hRgn, hRgn, h, RGN_OR);
						DeleteObject(h);
					}
					else
						hRgn = h;

					// Clean up
					SelectObject(hDC, holdBmp);
					DeleteDC(hDC);
				}

				DeleteObject(SelectObject(hMemDC, holdBmp));
			}

			DeleteDC(hMemDC);
		}	
	}

	return hRgn;
}

HRGN BitmapRegion(HBITMAP hBitmap, COLORREF cTransparentColor, bool bIsTransparent)
{
	// We create an empty region
	HRGN		hRegion=NULL;
	
	// If the passed bitmap is NULL, go away!
	if(!hBitmap) 
		return hRegion;
	
	// We create a memory context for working with the bitmap
	// The memory context is compatible with the display context (screen)
	HDC			hMemDC=CreateCompatibleDC(NULL);
	
	// If no context is created, go away, too!
	if(!hMemDC) 
		return hRegion;
		
	// Computation of the bitmap size
	BITMAP		bmBitmap;
	
	GetObject(hBitmap, sizeof(bmBitmap), &bmBitmap);
	
	// In order to make the space for the region, we
	// create a bitmap with 32bit depth color and with the
	// size of the loaded bitmap!
	BITMAPINFOHEADER RGB32BITSBITMAPINFO=
	{ 
		sizeof(BITMAPINFOHEADER), 
		bmBitmap.bmWidth, 
		bmBitmap.bmHeight, 
		1,32,BI_RGB,0,0,0,0,0 
	};
	
	// Here is the pointer to the bitmap data
	VOID		*pBits;
	
	// With the previous information, we create the new bitmap!
	HBITMAP		hNewBitmap;
	hNewBitmap=CreateDIBSection(hMemDC,
								(BITMAPINFO *)&RGB32BITSBITMAPINFO,
								DIB_RGB_COLORS,&pBits,NULL,0);

	// If the creation process succeded...
	if(hNewBitmap)	
	{
		// We select the bitmap onto the created memory context
		// and then we store the previosly selected bitmap on this context!
		HBITMAP		hPrevBmp=(HBITMAP) SelectObject(hMemDC,hNewBitmap);
		
		// We create another device context compatible with the first!
		HDC			hDC=CreateCompatibleDC(hMemDC);
		
		// If success...
		if(hDC) 
		{		
			// We compute the number of bytes per row that the bitmap contains, rounding to 32 bit-multiples
			BITMAP		bmNewBitmap;
			
			GetObject(hNewBitmap,sizeof(bmNewBitmap),&bmNewBitmap);
				
			while(bmNewBitmap.bmWidthBytes % 4) 
				bmNewBitmap.bmWidthBytes++;
			
			// Copy of the original bitmap on the memory context!
			HBITMAP		hPrevBmpOrg=(HBITMAP) SelectObject(hDC,hBitmap);
			BitBlt(hMemDC,0,0,bmBitmap.bmWidth,bmBitmap.bmHeight,hDC,0,0,SRCCOPY);

			// In order to optimize the code, we don't call the GDI each time we
			// find a transparent pixel. We use a RGN_DATA structure were we store
			// consecutive rectangles, until we have a large amount of them and then we crete
			// the composed region with ExtCreateRgn(), combining it with the main region.
			// Then we begin again initializing the RGN_DATA structure and doing another
			// iteration, until the entire bitmap is analyzed.

			// Also, in order to not saturate the Windows API with calls for reserving
			// memory, we wait until NUMRECT rectangles are stores in order to claim
			// for another NUMRECT memory space!
            #define NUMRECT	100			
			DWORD maxRect = NUMRECT;
			
			// We create the memory data
			HANDLE hData=GlobalAlloc(GMEM_MOVEABLE,sizeof(RGNDATAHEADER)+(sizeof(RECT)*maxRect));
			RGNDATA *pData=(RGNDATA*) GlobalLock(hData);
			pData->rdh.dwSize=sizeof(RGNDATAHEADER);
			pData->rdh.iType=RDH_RECTANGLES;
			pData->rdh.nCount=pData->rdh.nRgnSize=0;
			SetRect(&pData->rdh.rcBound,MAXLONG,MAXLONG,0,0);
			
			// We study each pixel on the bitmap...
			BYTE *Pixeles=(BYTE*) bmNewBitmap.bmBits+(bmNewBitmap.bmHeight-1)*bmNewBitmap.bmWidthBytes;
			
			// Main loop
			for(int Row=0;Row<bmBitmap.bmHeight;Row++) 
			{
				// Horizontal loop
				for(int Column=0;Column<bmBitmap.bmWidth;Column++)
				{		
					// We optimized searching for adyacent transparent pixels!
					int Xo=Column;
					LONG *Pixel=(LONG*) Pixeles+Column;

					while(Column<bmBitmap.bmWidth) 
					{
						bool bInRange=FALSE;

						BYTE	r = GetRValue(*Pixel);
						BYTE	g = GetGValue(*Pixel);
						BYTE	b = GetBValue(*Pixel);

						// If the color is that indicated as transparent...
						if(	GetRValue(*Pixel)==GetRValue(cTransparentColor) &&
							GetGValue(*Pixel)==GetGValue(cTransparentColor) &&
							GetBValue(*Pixel)==GetBValue(cTransparentColor))
							bInRange=TRUE;

						if((bIsTransparent) && (bInRange)) 
							break;

						if((!bIsTransparent) && (!bInRange)) 
							break;

						Pixel++;
						Column++;
					} // while (Column < bm.bmWidth) 		
					
					if(Column>Xo) 
					{
						// We add the rectangle (Xo,Row),(Column,Row+1) to the region

						// If the number of rectangles is greater then NUMRECT, we claim
						// another pack of NUMRECT memory places!
						if (pData->rdh.nCount>=maxRect)
						{
							GlobalUnlock(hData);
							maxRect+=NUMRECT;
							hData=GlobalReAlloc(hData,sizeof(RGNDATAHEADER)+(sizeof(RECT)*maxRect),GMEM_MOVEABLE);
							pData=(RGNDATA *)GlobalLock(hData);					
						}		
						
						RECT *pRect=(RECT*) &pData->Buffer;
						SetRect(&pRect[pData->rdh.nCount],Xo,Row,Column,Row+1);		
								
						if(Xo<pData->rdh.rcBound.left) 
							pData->rdh.rcBound.left=Xo;

						if(Row<pData->rdh.rcBound.top) 
							pData->rdh.rcBound.top=Row;

						if(Column>pData->rdh.rcBound.right) 
							pData->rdh.rcBound.right=Column;
						
						if(Row+1>pData->rdh.rcBound.bottom) 
							pData->rdh.rcBound.bottom=Row+1;
		
						pData->rdh.nCount++;																	

						// In Win95/08 there is a limitation on the maximum number of
						// rectangles a RGN_DATA can store (aprox. 4500), so we call
						// the API for a creation and combination with the main region
						// each 2000 rectangles. This is a good optimization, because
						// instead of calling the routines for combining for each new
						// rectangle found, we call them every 2000 rectangles!!!
						if(pData->rdh.nCount==2000)
						{						
							HRGN hNewRegion=ExtCreateRegion(NULL,sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRect),pData);
							if (hNewRegion) {
								// Si ya existe la regi? principal,sumamos la nueva,
								// si no,entonces de momento la principal coincide con
								// la nueva regi?.
								if (hRegion) {
									CombineRgn(hRegion,hRegion,hNewRegion,RGN_OR);									
									DeleteObject(hNewRegion);
								} else
									hRegion=hNewRegion;
								
								
							}
							// Volvemos a comenzar la suma de rect?gulos
							pData->rdh.nCount=0;
							SetRect(&pData->rdh.rcBound,MAXLONG,MAXLONG,0,0);
						}			
					
					} // if (Column > Xo)
				} // for (int  Column ...)

				// Nueva Row. Lo del negativo se debe a que el bitmap est?invertido
				// verticalmente.
				Pixeles -= bmNewBitmap.bmWidthBytes;
			
			} // for (int Row...)			

			// Una vez finalizado el proceso,procedemos a la fusi? de la
			// regi? remanente desde la ?tima fusi? hasta el final			
			HRGN hNewRegion=ExtCreateRegion(NULL,sizeof(RGNDATAHEADER)+(sizeof(RECT)*maxRect),pData);

			if(hNewRegion) 
			{
				// If the main region does already exist, we add the new one,
				if(hRegion)
				{														
					CombineRgn(hRegion,hRegion,hNewRegion,RGN_OR);
					DeleteObject(hNewRegion);
				}
				else
					// if not, we consider the new one to be the main region at first!
					hRegion=hNewRegion;				
			}						
			
			// We free the allocated memory and the rest of used ressources
			GlobalFree(hData);
			SelectObject(hDC,hPrevBmpOrg);
			DeleteDC(hDC);
			
		}// if (hDC) 

		SelectObject(hMemDC,hPrevBmp);
		DeleteDC(hMemDC);
	} //if (hNewBitmap)	

	return hRegion;
}

HRGN CreateRgnFromBitmap(HBITMAP hBmp, COLORREF color)
{
	if (!hBmp) return NULL;

	BITMAP bm;
	GetObject(hBmp, sizeof(BITMAP), &bm);	// get bitmap attributes

	CDC dcBmp;
	dcBmp.CreateCompatibleDC(NULL);	//Creates a memory device context for the bitmap
	dcBmp.SelectObject(hBmp);			//selects the bitmap in the device context

	const DWORD RDHDR = sizeof(RGNDATAHEADER);
	const DWORD MAXBUF = 40;		// size of one block in RECTs
									// (i.e. MAXBUF*sizeof(RECT) in bytes)
	LPRECT	pRects;								
	DWORD	cBlocks = 0;			// number of allocated blocks

	INT		i, j;					// current position in mask image
	INT		first = 0;				// left position of current scan line
									// where mask was found
	bool	wasfirst = false;		// set when if mask was found in current scan line
	bool	ismask;					// set when current color is mask color

	// allocate memory for region data
	RGNDATAHEADER* pRgnData = (RGNDATAHEADER*)new BYTE[ RDHDR + ++cBlocks * MAXBUF * sizeof(RECT) ];
	memset(pRgnData, 0, RDHDR + cBlocks * MAXBUF * sizeof(RECT));
	// fill it by default
	pRgnData->dwSize	= RDHDR;
	pRgnData->iType		= RDH_RECTANGLES;
	pRgnData->nCount	= 0;
	for (i = 0; i < bm.bmHeight; i++)
	for (j = 0; j < bm.bmWidth; j++){
		// get color
		ismask=(dcBmp.GetPixel(j,bm.bmHeight-i-1)!=color);
		// place part of scan line as RECT region if transparent color found after mask color or
		// mask color found at the end of mask image
		if (wasfirst && ((ismask && (j==(bm.bmWidth-1)))||(ismask ^ (j<bm.bmWidth)))){
			// get offset to RECT array if RGNDATA buffer
			pRects = (LPRECT)((LPBYTE)pRgnData + RDHDR);
			// save current RECT
			pRects[ pRgnData->nCount++ ] = CRect(first, bm.bmHeight - i - 1, j+(j==(bm.bmWidth-1)), bm.bmHeight - i);
			// if buffer full reallocate it
			if (pRgnData->nCount >= cBlocks * MAXBUF){
				LPBYTE pRgnDataNew = new BYTE[ RDHDR + ++cBlocks * MAXBUF * sizeof(RECT) ];
				memcpy(pRgnDataNew, pRgnData, RDHDR + (cBlocks - 1) * MAXBUF * sizeof(RECT));
				delete[] pRgnData;
				pRgnData = (RGNDATAHEADER*)pRgnDataNew;
			}
			wasfirst = false;
		} else if (!wasfirst && ismask){		// set wasfirst when mask is found
			first = j;
			wasfirst = true;
		}
	}
	dcBmp.DeleteDC();	//release the bitmap

	// create region
	HRGN hRgn=CreateRectRgn(0, 0, 0, 0);
	ASSERT(hRgn!=NULL);
	pRects = (LPRECT)((LPBYTE)pRgnData + RDHDR);
	for(i=0;i<(int)pRgnData->nCount;i++)
	{
		HRGN hr=CreateRectRgn(pRects[i].left, pRects[i].top, pRects[i].right, pRects[i].bottom);
		VERIFY(CombineRgn(hRgn, hRgn, hr, RGN_OR)!=ERROR);
		if (hr) DeleteObject(hr);
	}
	ASSERT(hRgn!=NULL);
// } ExtCreateRegion replacement

	delete[] pRgnData;
	return hRgn;
}

int GetSeasonIndex()
{
	CTime	t = CTime::GetCurrentTime();
	int		nMonth	= t.GetMonth();
	int		nDay	= t.GetDay();
	CString	str;

	str.Format(_T("%02d%02d"), nMonth, nDay);
	
	if (str >= _T("0225") && str <= _T("0524"))
		return 0;
	else if (str >= _T("0525") && str <= _T("0824"))
		return 1;
	else if (str >= _T("0825") && str <= _T("1124"))
		return 2;

	return 3;
}

// reset high resolution time lcounter 
int ptimer_start(int instance)
{
	if (instance < 0 || instance >= PTIMER_INSTANCE_NUM) return -1;
	if (!QueryPerformanceFrequency((LARGE_INTEGER*)&i64_freq[instance])) return -2;
	if (!QueryPerformanceCounter((LARGE_INTEGER*)&i64_start[instance])) return -3;
	if (i64_freq[instance] == 0) return -4;
	return 0;
}

// get time(double precision) elaspsed since timer reset in ms
double ptimer_get_time(int instance)
{
	QueryPerformanceCounter((LARGE_INTEGER*)&i64_end); 
	return (double)((i64_end - i64_start[instance])/(double)i64_freq[instance])*1000.0;
}

#ifndef _USING_V110_SDK71_
void set_wallpaper(CString sfile)
{
	::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	CComPtr<IDesktopWallpaper> pWallPaper;
	HRESULT hr = pWallPaper.CoCreateInstance(CLSID_DesktopWallpaper);
	if (FAILED(hr))
	{
		return;
	}

	hr = pWallPaper->Enable(FALSE);

	COLORREF color = 0x000000;
	hr = pWallPaper->SetBackgroundColor(color);
	hr = pWallPaper->GetBackgroundColor(&color);

	hr = pWallPaper->Enable(TRUE);

	UINT nCount = 0;
	hr = pWallPaper->GetMonitorDevicePathCount(&nCount);

	LPWSTR szPathAt = nullptr;
	pWallPaper->GetMonitorDevicePathAt(0, &szPathAt);

	LPWSTR szWallpaper = nullptr;
	pWallPaper->GetWallpaper(szPathAt, &szWallpaper);
	pWallPaper->SetWallpaper(szPathAt, CStringW(sfile));

	DESKTOP_WALLPAPER_POSITION dwp;
	hr = pWallPaper->GetPosition(&dwp);

	RECT rc;
	pWallPaper->GetMonitorRECT(szPathAt, &rc);

	::CoTaskMemFree(szPathAt);
	::CoTaskMemFree(szWallpaper);	

	pWallPaper.Release();

	::CoUninitialize();
}
#endif

//두 점의 각도를 구한다.
//원하는 결과값은 0~360도인데
//atan은 부호에 따른 각도를 구분할 수 없고
//atan2는 -PI ~ +PI로 리턴하므로 0~360 범위와 맞지 않으므로 아래와 같이 구해야 한다.
//ScreenCoord일때와 Cartesian coordinate(직교좌표계)일때는 y가 반대임에 주의.
double GetAngle(double vx, double vy, bool bScreenCoord)
{
	double radian = atan2((bScreenCoord ? -vy : vy), vx);
	double angle = radian * 180.0 / PI + (radian > 0 ? 0 : 360);
	if (angle >= 360.0)
		angle = fmod(angle, 360.0);
	return angle;
}

double GetAngle(double x1, double y1, double x2,  double y2, bool bScreenCoord)
{
	//return 90 - 180.0 * atan2(x2 - x1, y2 - y1) / 3.141592;
	return GetAngle(x2-x1, y2-y1, bScreenCoord);
}

double GetAngle(CPoint pt0, CPoint pt1, bool bScreenCoord)
{
	return GetAngle(pt0.x, pt0.y, pt1.x, pt1.y, bScreenCoord);
}

//3점이 이루는 각도
double GetAngle(CPoint a, CPoint b, CPoint c)
{
	CPoint ab(b.x - a.x, b.y - a.y);
	CPoint cb(b.x - c.x, b.y - c.y);

	float dot = (ab.x * cb.x + ab.y * cb.y); // dot product
	float cross = (ab.x * cb.y - ab.y * cb.x); // cross product

	float alpha = atan2(cross, dot);

	return (int)floor(alpha * 180.0 / PI + 0.5);
}

CPoint GetCenterPoint(CPoint pt0, CPoint pt1)
{
	return CPoint(pt0.x + (pt1.x - pt0.x) / 2.0, pt0.y + (pt1.y - pt0.y) / 2.0);
}

double GetDistance(CPoint pt0, CPoint pt1)
{
	return DISTANCE((double)pt0.x, (double)pt0.y, (double)pt1.x, (double)pt1.y);
}

//두 점을 지나는 직선상의 x3를 구한다.
double getLinePointX(double x1, double y1, double x2, double y2, double y3)
{
	if (x1 == x2)
		return x1;

	double a = (double)(y2 - y1) / (double)(x2 - x1);
	double b = y2 - a * x2;
	return ((y3 - b) / a);
}

//두 점을 지나는 직선상의 y3를 구한다.
double getLinePointY(double x1, double y1, double x2, double y2, double x3)
{
	if (x1 == x2)
		return x1;

	double a = (double)(y2 - y1) / (double)(x2 - x1);
	double b = y2 - a * x2;
	return (a * x3 + b);
}

double GetManhattanDistance(double x1, double y1, double x2, double y2)
{
	return (fabs(x1 - x2) + fabs(y1 - y2));
}

//ptCenter를 기준으로 dAngle 만큼 회전된 dDist거리의 점의 좌표를 구한다.
CPoint GetRotatedPoint(CPoint ptCenter, double dAngle, double dDist)
{
	CPoint	pt;

	pt.x = cos(RADIAN(dAngle)) * dDist;
	pt.y = -sin(RADIAN(dAngle)) * dDist;
	pt.Offset(ptCenter);

	return pt;
}

//cx, cy를 중심으로 tx, ty점이 degree를 회전할 경우 tx, ty점의 변경 좌표
void get_rotated(int cx, int cy, int* tx, int* ty, double degree)
{
	double radian = RADIAN(degree);
	int tx1 = ((double)(*tx - cx) * cos(radian) - (double)(*ty - cy) * sin(radian)) + cx;
	int ty1 = ((double)(*tx - cx) * sin(radian) + (double)(*ty - cy) * cos(radian)) + cy;
	*tx = tx1;
	*ty = ty1;
}

std::vector<CPoint> get_rotated(int cx, int cy, CRect* r, double degree)
{
	int x, y;
	std::vector<CPoint> pts;
	
	x = r->left;
	y = r->top;
	get_rotated(cx, cy, &x, &y, degree);
	pts.push_back(CPoint(x, y));

	x = r->right;
	y = r->top;
	get_rotated(cx, cy, &x, &y, degree);
	pts.push_back(CPoint(x, y));

	x = r->right;
	y = r->bottom;
	get_rotated(cx, cy, &x, &y, degree);
	pts.push_back(CPoint(x, y));

	x = r->left;
	y = r->bottom;
	get_rotated(cx, cy, &x, &y, degree);
	pts.push_back(CPoint(x, y));

	*r = get_max_rect(pts);
	return pts;
}

CString GetToken(CString& str, LPCTSTR c)
{
	int		pos;
	CString sToken;

	if (str.Left(1) == c)
		str = str.Right(str.GetLength() - 1);

	pos = str.Find(c);

	if (pos < 0)
	{
		sToken = str;
		str = "";
		return sToken;
	}

	sToken = str.Left(pos);
	str = str.Mid(pos + 1);

	return sToken;
}

//문자열에서 n번째 토큰을 리턴한다.
CString GetToken(CString src, CString separator, int index)
{
	int i = 0;
	CStringArray saItems;

	for (CString sItem = src.Tokenize(separator,i); i >= 0; sItem = src.Tokenize(separator,i))
	{
		saItems.Add(sItem);
	}

	if (index < 0 || index >= (int)saItems.GetSize())
		return _T("");

	return saItems.GetAt(index);
}

//Tokenize를 이용하면 공백인 토큰은 처리되지 않고 무시되므로 원하는 개수만큼 추출되지 않는다.
//따라서 아래 함수는 사용하지 않는다.
/*
std::deque<CString>	get_token_string(CString src, CString separator)
{
	int i = 0;
	std::deque<CString> dq;
	for (CString sItem = src.Tokenize(separator,i); i >= 0; sItem = src.Tokenize(separator,i))
		dq.push_back(sItem);

	return dq;
}
*/
int get_token_string(CString src, std::deque<CString> &dqToken, CString separator, bool allowEmpty, int nMaxToken, bool include_rest)
{
	CString token;

	dqToken.clear();

	int pos = 0;
	
	while (true)
	{
		pos = src.Find(separator, 0);

		if (pos >= 0)
		{
			token = src.Left(pos);
			src = src.Mid(pos + separator.GetLength());
		}
		else
		{
			//더 이상 separator가 없다면 맨 마지막 token이다.
			token = src;
			src.Empty();
		}

		if (!token.IsEmpty() || allowEmpty)
			dqToken.push_back(token);

		if (nMaxToken > 0 && dqToken.size() == nMaxToken)
		{
			//최대 토큰 개수가 정해져 있을 때 include_rest가 true이면
			//나머지 문자열도 모두 맨 마지막 토큰뒤에 붙여준다.
			if (include_rest && !src.IsEmpty())
			{
				dqToken[nMaxToken - 1] += (separator + src);
			}

			return dqToken.size();
		}

		if (pos < 0)
			break;
	}

	return dqToken.size();
}


//nMaxToken 숫자 만큼 토큰을 분리해서 sToken에 넣고
//실제 처리된 토큰 개수를 리턴한다.
//src는 토큰이 분리될 때마다 줄어든다.
//이는 CString::Tokenize() 또는 AfxExtractSubString()과는 다르다.
//(seps = " ,\t\n" 과 같이 분리기호들로 이루어진 스트링 데이터)
//strtok_s 함수는 연속 공백이나 연속 쉼표 등 중복된 분리자는 모두 하나의 구분자로 취급된다.
//ex. "1,, ,,2" = "1,2"
int get_token_string(TCHAR *src, TCHAR *seps, CString *sToken, int nMaxToken)
{
	int		nToken = 0;
	TCHAR	*token = NULL;
	TCHAR	*next_token = NULL;

	token = _tcstok(src, seps);

	while (token != NULL)
	{
		//printf("%s ", token);
		sToken[nToken++] = token;

		if (nToken >= nMaxToken)
		{
			return nToken;
		}

		token = _tcstok(NULL, seps);
	}

	return nToken;
}

int get_token_string(char *src, char *seps, char **sToken, int nMaxToken)
{
	int		i;
	int		nToken = 0;
	char	*token = NULL;
	char	*next_token = NULL;

	sToken = (char**)calloc(nMaxToken, sizeof(char *));
	for (i = 0; i < nMaxToken; i++)
		sToken[i] = (char*)calloc(100, sizeof(char));

	token = strtok(src, seps);

	while (token != NULL)
	{
		strcpy(sToken[nToken++], token);

		if (nToken >= nMaxToken)
		{
			return nToken;
		}

		token = strtok(NULL, seps);
	}

	return nToken;
}

//"<b><cr=red>This</b></cr> is a <i>sample</i> <b>paragraph</b>."
//위와 같은 형식일 때 태그와 텍스트를 분리한다. 태그내의 공백은 제거된다.
//"\r", "\n"과 같은 line break는 모두 <br>로 변경한다.
void get_tag_str(CString src, std::deque<CString>& tags)
{
	int		i;
	CString str;

	tags.clear();

	//line break는 모두 <br>로 변경한다.
	src.Replace(_T("\r\n"), _T("<br>"));
	src.Replace(_T("\r"), _T("<br>"));
	src.Replace(_T("\n"), _T("<br>"));

	//태그와 텍스트를 분리한다.
	for (i = 0; i < src.GetLength(); i++)
	{
		if (src[i] == '<')
		{
			//시작 태그를 만난 경우 그 전까지의 문자열을 저장한다.
			if (str.GetLength() > 0)
			{
				//태그가 아닌 일반 텍스트가 저장된다.
				tags.push_back(str);
				str.Empty();
			}

			str += src[i];

			while (src[i] != '>')
			{
				i++;

				if (i >= src.GetLength())
					break;
				str += src[i];
			}

			//태그가 저장된다. 공백, 하이픈, 언더바는 모두 제거된다.
			//컬러명은 Red로 주면 Gdiplus::Color::Red를 사용하므로 대소문자를 구분한다.
			str.Replace(_T(" "), _T(""));
			str.Replace(_T("_"), _T(""));
			str.Replace(_T("-"), _T(""));
			tags.push_back(str);
			str.Empty();
		}
		else
		{
			str += src[i];
		}
	}

	//맨 마지막까지 잔여 텍스트가 있다면 모두 넣어줘야 한다.
	if (str.GetLength() > 0)
		tags.push_back(str);
}

//간혹 \r, \n, \t, \\등의 문자를 그대로 확인할 필요가 있다.
CString	get_unescape_string(CString src)
{
	CString result = _T("");

	//result.Replace(_T("\r"), _T("\\r"));
	//result.Replace(_T("\n"), _T("\\n"));
	//result.Replace(_T("\t"), _T("\\t"));
	//result.Replace(_T("\\"), _T("\\\\"));
	//result.Replace(_T("&"))

	CString escape_set = _T("\r\n\t\\&");

	//Replace를 쓸 경우 "\\r"의 "\r"도 "\\r"로 변경시키므로 결과적으로는 "\\\r"이 된다.
	//escape char는 하나의 char이므로 원시적으로 처리한다.
	for (int i = 0; i < src.GetLength(); i++)
	{
		if (escape_set.Find(src[i]) < 0)
		{
			result += src[i];
		}
		else
		{
			if (src[i] == '&')
				result += _T("&&");
			else if (src[i] == '\\')
				result += _T("\\\\");
			else if (src[i] == '\r')
				result += _T("\\r");
			else if (src[i] == '\n')
				result += _T("\\n");
			else if (src[i] == '\t')
				result += _T("\\t");
		}
	}

	return result;
}

// a_value : 1.1.24050
// b_value : Normal
// c_value : True
// 위와 같이 속성이름 및 값으로 매핑되는 문자열을 파싱하여 std::map에 넣어준다.
// lfrf는 라인분리문자열이고 보통 "\n"이거나 "\r\n" 등이 있고
// separator는 ':' 이름과 값을 구분하는 구분자이다.
// return value : 항목의 개수
int	get_map_string(CString src, std::map<CString, CString>& map, CString lfrf, CString separator)
{
	map.clear();

	std::deque<CString> dqLines;
	get_token_string(src, dqLines, lfrf, false);

	std::deque<CString> dqToken;

	for (auto line : dqLines)
	{
		get_token_string(line, dqToken, separator, true, 2, true);
		
		for (int i = 0; i < dqToken.size(); i++)
			dqToken[i].Trim();
		
		if (dqToken.size() != 2 || dqToken[0].IsEmpty())
			continue;

		map.insert(std::pair<CString, CString>(dqToken[0], dqToken[1]));
	}

	return map.size();
}

//dq항목을 하나의 문자열로 합쳐준다.
CString	get_concat_string(std::deque<CString> dq, CString separator)
{
	int i;
	CString res;

	for (i = 0; i < dq.size(); i++)
	{
		if (i == dq.size() - 1)
			res = res + dq[i];
		else
			res = res + dq[i] + separator;
	}

	return res;
}

CString	get_tokenized(std::deque<CString> dq, TCHAR separator)
{
	CString res = _T("");

	for (int i = 0; i < dq.size(); i++)
		res = res + dq[i] + separator;

	return res;
}

bool get_bracket_token(CString src, std::deque<CString>* token, TCHAR bracket)
{
	CString sub;
	TCHAR end_bracket;
	int start = src.Find(bracket);
	int end;

	if (token)
		token->clear();

	if (bracket == '[')
		end_bracket = ']';
	else if (bracket == '(')
		end_bracket = ')';
	else if (bracket == '{')
		end_bracket = '}';
	else
		return false;

	if (start < 0)
		return false;

	while (true)
	{
		end = src.Find(end_bracket, start);
		//시작 괄호는 있는데 끝 괄호가 없다면 정상이 아니다.
		//정상인 데이터만 사용할지, 에러라면 무조건 버릴지는 사용하는 곳에서 판단한다.
		if (end < 0)
			return false;

		sub = src.Mid(start + 1, end - start - 1);
		token->push_back(sub);
		start = src.Find(bracket, end + 1);

		//더 이상 시작 괄호가 없다면 종료.
		if (start < 0)
			break;
	}

	return true;
}

//update livesupport_report set endtime = '2023-01-01 22:01:29', env_type = '5', viewer_type = '0' where accesscode = '31108355' and sptnum = '50400'
//위와 같은 형식에서 필드명과 값을 추출한다.
//fields에는 추출할 필드명을 넘겨준다. "endtime", "env_type"...
void get_sql_token_from_assign_form(CString src, std::map<CString, CString> *map)
{
	std::string sql;
	std::regex pattern("(\\w+)\\s*=\\s*'([^']*)'");

	sql = CString2string(src);
	map->clear();

	std::sregex_iterator iter(sql.begin(), sql.end(), pattern);
	std::sregex_iterator end;


	while (iter != end)
	{
		map->insert(std::make_pair(CString((*iter)[1].str().c_str()), CString((*iter)[2].str().c_str())));
		//fieldNames.push_back((*iter)[1].str());
		//values.push_back((*iter)[2].str());
		++iter;
	}
}

//insert into neturo_server_info(userid, com_name, s_pub_ip, s_pri_ip) values('14533821', 'DESKTOP-0CN9VAK', '220.85.215.243', 'publicIP')
//위와 같은 sql에서 field와 value를 추출한다.
void get_sql_token_from_bracket_form(CString src, std::map<CString, CString>* map)
{
	std::string sql;
	std::regex pattern("\\((.*)\\)\\s*values\\s*\\((.*)\\)");

	sql = CString2string(src);
	map->clear();

	std::vector<std::string> fieldNames;
	std::vector<std::string> values;

	std::smatch matches;
	if (std::regex_search(sql, matches, pattern))
	{
		std::string fieldNamesStr = matches[1].str();
		std::string valuesStr = matches[2].str();

		// Split the field names and values
		std::regex fieldPattern("(\\w+)(,\\s*|$)");
		std::sregex_iterator fieldIter(fieldNamesStr.begin(), fieldNamesStr.end(), fieldPattern);
		std::sregex_iterator fieldEnd;
		while (fieldIter != fieldEnd) {
			fieldNames.push_back((*fieldIter)[1].str());
			++fieldIter;
		}

		std::regex valuePattern("'(.*?)'(,\\s*|$)");
		std::sregex_iterator valueIter(valuesStr.begin(), valuesStr.end(), valuePattern);
		std::sregex_iterator valueEnd;
		while (valueIter != valueEnd) {
			values.push_back((*valueIter)[1].str());
			++valueIter;
		}

		for (int i = 0; i < std::min(fieldNames.size(), values.size()); i++)
			map->insert(std::make_pair(CString(fieldNames[i].c_str()), CString(values[i].c_str())));
	}
}

CString get_sql_cmd(CString sql, CString* sql_cmd, CString *table_name)
{
	sql.Trim();
	sql.MakeLower();

	std::deque<CString> sql_cmd_list = { "select", "insert into", "update", "delete", };
										 //"create", "alter", "drop", "rename", "truncate",
										 //"commit", "rollback", "savepoint",
										 //"grant", "revoke",};
	CString cmd = _T("");

	//src가 sql_cmd로 시작되는 문장이면 true
	for (int i = 0; i < sql_cmd_list.size(); i++)
	{
		if (sql.Find(sql_cmd_list[i]) == 0)
		{
			cmd = sql_cmd_list[i];
			break;
		}
	}

	if (sql_cmd)
		*sql_cmd = cmd;

	if (table_name)
	{
		sql = sql.Mid(cmd.GetLength() + 1);
		//table_name 다음에는 공백 또는 '('가 오므로 그 위치를 찾는다.
		int pos = sql.FindOneOf(_T(" ("));
		*table_name = sql.Left(pos);
	}

	return cmd;
}

CString get_str(CString& src, CString sep)
{
	src.TrimLeft();

	int pos = src.Find(sep);
	if(pos < 0)
	{
		pos = src.GetLength();
		if(pos < 1) throw 1;
	}

	CString ret = src.Left(pos);
	if (pos < src.GetLength())
		src = src.Mid(pos + 1);

	return(ret);
}

int get_int(CString& src, CString sep)
{
	CString str;

	str = get_str(src, sep);
	str.MakeLower();

	CString fmtstr = str.GetLength() > 2 && (str.Left(2) == _T("&h") || str.Left(2) == _T("0x"))
		? str = str.Mid(2), _T("%x")
		: _T("%d");

	int ret;
	if(_stscanf(str, fmtstr, &ret) != 1) throw 1;

	return(ret);
}

double get_double(CString& src, CString sep)
{
	CString str;

	str = get_str(src, sep);
	str.MakeLower();

	float ret;
	if(_stscanf(str, _T("%f"), &ret) != 1) throw 1;

	return((double)ret);
}

//resource string table의 문자열을 리턴한다.
CString	load_string(UINT nID)
{
	CString str;
	str.LoadString(nID);
	return str;
}


//src 문자열에 set_of_keyword에 나열된 단어가 있는지 검사.
//set_of_keyword는 세미콜론으로 구분해서 여러 문자 또는 문자열을 넣을 수 있다.
//ex. src = "abcd1234"일 때 set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
bool is_exist_keyword(CString src, CString set_of_keyword, bool case_sensitive, bool whole_word)
{
	std::deque<CString> dqKeyword;

	get_token_string(set_of_keyword, dqKeyword, ';');

	if (!case_sensitive)
		src.MakeLower();

	for (int i = 0; i < dqKeyword.size(); i++)
	{
		if (!case_sensitive)
			dqKeyword[i].MakeLower();

		if (whole_word)
		{
			if (src == dqKeyword[i])
				return true;
		}
		else if (src.Find(dqKeyword[i]) >= 0)
		{
			return true;
		}
	}

	return false;
}

#ifndef _USING_V110_SDK71_
SIZE_T GetCurrentMemUsage()
{
	DWORD dwpid = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |	PROCESS_VM_READ, FALSE, dwpid);
	PROCESS_MEMORY_COUNTERS_EX mc;

	if (NULL == hProcess)
		return 0;

	if (::GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&mc, sizeof(mc)))
	{
		CloseHandle(hProcess);
		return mc.WorkingSetSize;
	}

	return 0;
}
#endif

//경로에 '\\' 또는 '/'가 혼용되어 사용되는 경우가 있으므로 이를 감안해야 한다.
//c:\\folder1\\folder2\\	=> c:\\folder1
//c:\\folder1\\folder2		=> c:\\folder1
//c:\\folder1\\				=> c:\\
//c:\\folder1				=> c:\\

CString	get_parent_dir(CString path, TCHAR path_sep)
{
	//path 끝에 path 구분자가 붙었다면 제거해준다.
	if (path.Right(1) == path_sep)
		path = path.Left(path.GetLength() - 1);

	//만약 구해진 폴더가 "c:"와 같이 드라이브 루트라면 끝에 "\\"를 다시 붙여줘야한다.
	if (path.GetLength() == 2 && path.GetAt(1) == ':')
	{
		path += path_sep;
		return path;
	}

	//해당 폴더의 parent 폴더명을 구한다.
	path = path.Left(path.ReverseFind(path_sep));

	//다시 구해진 폴더가 "c:"와 같이 드라이브 루트라면 끝에 "\\"를 다시 붙여줘야한다.
	if (path.GetLength() == 2 && path.GetAt(1) == ':')
	{
		path += path_sep;
		return path;
	}

	return path;
}

//MAX_COMPUTERNAME_LENGTH(15) 길이까지만 리턴됨에 주의.
//GetComputerName API 함수는 항상 대문자로 리턴한다.
//그냥 확실하게 GetComputerNameString().MakeLower() 등과 같이
//대소문자를 명확히 하여 비교하는 것이 좋다.
//=>255글자까지 리턴되는 GetComputerNameEx()로 변경함.
CString get_computer_name_string()
{
	//TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
	TCHAR computerName[255] = { 0, };
	DWORD size = 255;// sizeof(computerName) / sizeof(computerName[0]);
	GetComputerNameEx(ComputerNamePhysicalDnsHostname, computerName, &size);
	return computerName;
}

OSVERSIONINFOEX get_windows_version()
{
	OSVERSIONINFOEX osInfo;
	NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEX);

	*(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

	if (NULL != RtlGetVersion)
	{
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);
		RtlGetVersion(&osInfo);
	}

	return osInfo;
}

CString get_windows_version_number()
{
	CString version;
	OSVERSIONINFOEX osvi = get_windows_version();

	version.Format(_T("%d.%d.%d"), osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
	return version;
}

DWORD get_windows_major_version()
{
	OSVERSIONINFOEX osvi = get_windows_version();;
	return osvi.dwMajorVersion;
}

CString	get_windows_version_string(bool detail)
{
	CString version;
	OSVERSIONINFOEX osvi = get_windows_version();
	SYSTEM_INFO si;

	if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
		version = _T("Microsoft Windows XP");
	else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
		version = _T("Microsoft Windows Vista");
	else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
		version = _T("Microsoft Windows 7");
	else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
		version = _T("Microsoft Windows 8");
	else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
		version = _T("Microsoft Windows 8.1");
	else if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0)
	{
		if (osvi.dwBuildNumber >= 22000)
			version = _T("Microsoft Windows 11");
		else
			version = _T("Microsoft Windows 10");
	}
	else
	{
		version = _T("Unknown OS version");
	}

	version.Format(_T("%s (Build %d)"), version, osvi.dwBuildNumber);

	if (detail)
	{
		PGNSI pGNSI;
		PGPI pGPI;
		DWORD dwType;
		CString sType;

		pGPI = (PGPI)GetProcAddress(
			GetModuleHandle(TEXT("kernel32.dll")),
			"GetProductInfo");

		pGPI(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

		switch (dwType)
		{
			case PRODUCT_ULTIMATE:
				sType = _T("Ultimate Edition");
				break;
			case PRODUCT_PROFESSIONAL:
				sType = _T("Professional");
				break;
			case PRODUCT_HOME_PREMIUM:
				sType = _T("Home Premium Edition");
				break;
			case PRODUCT_HOME_BASIC:
				sType = _T("Home Basic Edition");
				break;
			case PRODUCT_ENTERPRISE:
				sType = _T("Enterprise Edition");
				break;
			case PRODUCT_BUSINESS:
				sType = _T("Business Edition");
				break;
			case PRODUCT_STARTER:
				sType = _T("Starter Edition");
				break;
			case PRODUCT_CLUSTER_SERVER:
				sType = _T("Cluster Server Edition");
				break;
			case PRODUCT_DATACENTER_SERVER:
				sType = _T("Datacenter Edition");
				break;
			case PRODUCT_DATACENTER_SERVER_CORE:
				sType = _T("Datacenter Edition (core installation)");
				break;
			case PRODUCT_ENTERPRISE_SERVER:
				sType = _T("Enterprise Edition");
				break;
			case PRODUCT_ENTERPRISE_SERVER_CORE:
				sType = _T("Enterprise Edition (core installation)");
				break;
			case PRODUCT_ENTERPRISE_SERVER_IA64:
				sType = _T("Enterprise Edition for Itanium-based Systems");
				break;
			case PRODUCT_SMALLBUSINESS_SERVER:
				sType = _T("Small Business Server");
				break;
			case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
				sType = _T("Small Business Server Premium Edition");
				break;
			case PRODUCT_STANDARD_SERVER:
				sType = _T("Standard Edition");
				break;
			case PRODUCT_STANDARD_SERVER_CORE:
				sType = _T("Standard Edition (core installation)");
				break;
			case PRODUCT_WEB_SERVER:
				sType = _T("Web Server Edition");
				break;
		}

		if (sType.IsEmpty() == false)
		{
			version = version + _T(" ") + sType;
		}

		pGNSI = (PGNSI)GetProcAddress(
			GetModuleHandle(TEXT("kernel32.dll")),
			"GetNativeSystemInfo");
		if (NULL != pGNSI)
			pGNSI(&si);
		else
			GetSystemInfo(&si);

		if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			version = version + _T(", 64-bit");
		else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
			version = version + _T(", 32-bit");
	}

	return version;
}

CString	get_system_label(int csidl, int* sysIconIndex)
{
	//이걸 해주지 않을 경우 CSIDL_DRIVES (내 PC)는 레이블을 잘 가져오지만
	//CSIDL_DESKTOP, CSIDL_MYDOCUMENTS는 가져오지 못한다.
	CoInitialize(NULL);

	LPMALLOC pMalloc;
	SHGetMalloc(&pMalloc);
	SHFILEINFO sfi = { 0 };
	// 'My Computer' or '내 컴퓨터' text
	LPITEMIDLIST pidl;
	SHGetSpecialFolderLocation(NULL, csidl, &pidl);
	SHGetFileInfo((TCHAR*)pidl, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	pMalloc->Free(pidl);
	pMalloc->Release();

	if (sysIconIndex != NULL)
	{
		*sysIconIndex = sfi.iIcon;
	}

	CoUninitialize();

	return sfi.szDisplayName;
}

//이 값은 윈도우가 설치될 때 생성되고 재설치되지 않으면 유지된다.
//단, HDD 복제시에도 유지되므로 머신에 따라 unique하다고 볼 수 없다.
CString	read_windows_GUID()
{
	TCHAR value[1024] = { 0, };
	DWORD size = _countof(value);
	DWORD type = REG_SZ;
	HKEY key;
	LONG retKey = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Cryptography"), 0, KEY_READ | KEY_WOW64_64KEY, &key);
	LONG retVal = ::RegQueryValueEx(key, _T("MachineGuid"), nullptr, &type, (LPBYTE)value, &size);

	if (retKey == ERROR_SUCCESS && retVal == ERROR_SUCCESS) {
		//ret = value;
	}

	::RegCloseKey(key);

	return value;
}

CString	create_GUID()
{
	RPC_STATUS Status;
	UUID uuid;
	TCHAR* idStr = NULL;
	CString guid;

	Status = UuidCreate(&uuid);

	if (RPC_S_OK != Status)
	{
		uuid = GUID_NULL;
	}
	else
	{
#ifdef _UNICODE
		if (UuidToString(&uuid, (RPC_WSTR*)&idStr) == RPC_S_OK)
		{
			guid = idStr;
			RpcStringFree((RPC_WSTR*)&idStr);
		}
#else
		if (UuidToString(&uuid, (RPC_CSTR*)&idStr) == RPC_S_OK)
		{
			guid = idStr;
			RpcStringFree((RPC_CSTR*)&idStr);
		}
#endif
	}

	return guid;
}

bool get_windows_update_setting(bool& auto_update, int& level)
{
	auto_update = false;
	level = -1;

	// CoInitializeEx() 함수를 호출하여 COM 라이브러리를 초기화합니다.
	if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		IAutomaticUpdates* pAutomaticUpdates = NULL;
		// Automatic Updates 객체 생성
		HRESULT hr = CoCreateInstance(
			CLSID_AutomaticUpdates,
			NULL,
			CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
			IID_IAutomaticUpdates,
			(LPVOID*)&pAutomaticUpdates);

		if (SUCCEEDED(hr))
		{
			VARIANT_BOOL isAutoUpdateEnabled = VARIANT_FALSE;
			// 자동 업데이트 활성화 여부 확인
			hr = pAutomaticUpdates->get_ServiceEnabled(&isAutoUpdateEnabled);
			if (SUCCEEDED(hr))
			{
				if (isAutoUpdateEnabled == VARIANT_TRUE)
				{
					//AfxMessageBox(_T("자동 업데이트가 활성화되었습니다."));
					auto_update = true;
				}
				else
				{
					//AfxMessageBox(_T("자동 업데이트가 비활성화되었습니다."));
					auto_update = true;
				}
			}
			else
			{
				//AfxMessageBox(_T("자동 업데이트 상태를 가져오는 데 실패했습니다."));
				return false;
			}

			IAutomaticUpdatesSettings* settings = NULL;
			pAutomaticUpdates->get_Settings(&settings);

			AutomaticUpdatesNotificationLevel notification_level;
			if (S_OK == settings->get_NotificationLevel(&notification_level))
			{
				level = notification_level;
			}

			//AfxMessageBox(i2S(level));

			pAutomaticUpdates->Release();
		}
	}
	else
	{
		return false;
	}

	// COM 라이브러리 정리
	CoUninitialize();

	return true;
}

//SystemParametersInfo(SPI_GETSCREENSAVEACTIVE...)으로는 제대로 설정값을 얻어오지 못한다.
//SCRNSAVE.EXE라는 항목이 존재하면 설정된 것이고 없으면 해제된 것이다.
//설정시간과 잠금화면을 표시할지에 대한 설정값을 얻어올 수 있다.
bool get_screensaver_setting(int *timeout, int* use_secure)
{
	HKEY hKeySreenSaver = NULL;
	long lReturn = NULL;
	long lScreenSaver = NULL;
	long lScreenSaverActive = NULL;
	DWORD dwType = 0;
	DWORD dwDataSize = MAX_PATH;

	lReturn = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"), 0, KEY_ALL_ACCESS, &hKeySreenSaver);

	if (lReturn == ERROR_SUCCESS)
	{
		lScreenSaver = RegQueryValueEx(hKeySreenSaver, _T("SCRNSAVE.EXE"), NULL, &dwType, NULL, &dwDataSize);

		if (lScreenSaver == ERROR_SUCCESS)
		{
			if (timeout != NULL)
				SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, timeout, 0);

			if (use_secure != NULL)
				SystemParametersInfo(SPI_GETSCREENSAVESECURE, 0, use_secure, 0);

			return true;
		}
	}

	RegCloseKey(hKeySreenSaver);

	return false;
}

#include <WinIoCtl.h>

//HDD serial은 HardwareInfo.exe가 알려주는대로 S4EVNM0T230338R 15자리 형태,
//또는 0025_3852_2190_FE03 같은 형태로 리턴된다.
//어떤 PC는 툴이 알려주는 값과 이 프로그램에서 구한 값이 같지만(mwj, sdh)
//어떤 PC는 툴이 알려주는 값과 다른 포맷으로 리턴한다.(scpark)
//왜 이런 차이가 발생하는지는 아직 알 수 없으나
//unique한 키값을 사용하고자 함이므로 우선 16자리로 맞춰서 사용한다.
//unify16 = true이면 16자리로 통일시켜 리턴한다.

CString get_HDD_serial_number(int index, bool unify16)
{
	CString strDrive;

	strDrive.Format(_T("\\\\.\\PhysicalDrive%d"), index);

	//get a handle to the first physical drive
	HANDLE h = CreateFile(strDrive, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (h == INVALID_HANDLE_VALUE)
		return CString();

	//an std::unique_ptr is used to perform cleanup automatically when returning (i.e. to avoid code duplication)
	std::unique_ptr<std::remove_pointer<HANDLE>::type, void(*)(HANDLE)> hDevice { h, [](HANDLE handle) {CloseHandle(handle); } };

	//initialize a STORAGE_PROPERTY_QUERY data structure (to be used as input to DeviceIoControl)
	STORAGE_PROPERTY_QUERY storagePropertyQuery{};

	storagePropertyQuery.PropertyId = StorageDeviceProperty;
	storagePropertyQuery.QueryType = PropertyStandardQuery;

	//initialize a STORAGE_DESCRIPTOR_HEADER data structure (to be used as output from DeviceIoControl)
	STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader{};

	//the next call to DeviceIoControl retrieves necessary size (in order to allocate a suitable buffer)
	//call DeviceIoControl and return an empty std::string on failure
	DWORD dwBytesReturned = 0;
	if (!DeviceIoControl(hDevice.get(), IOCTL_STORAGE_QUERY_PROPERTY, &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		&storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER), &dwBytesReturned, NULL))
		return CString();

	//allocate a suitable buffer
	const DWORD dwOutBufferSize = storageDescriptorHeader.Size;
	std::unique_ptr<BYTE[]> pOutBuffer{ new BYTE[dwOutBufferSize]{} };

	//call DeviceIoControl with the allocated buffer
	if (!DeviceIoControl(hDevice.get(), IOCTL_STORAGE_QUERY_PROPERTY, &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		pOutBuffer.get(), dwOutBufferSize, &dwBytesReturned, NULL))
		return CString();

	//read and return the serial number out of the output buffer
	STORAGE_DEVICE_DESCRIPTOR* pDeviceDescriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(pOutBuffer.get());
	const DWORD dwSerialNumberOffset = pDeviceDescriptor->SerialNumberOffset;

	if (dwSerialNumberOffset == 0)
		return CString();

	CString serialNumber = CString(pOutBuffer.get() + dwSerialNumberOffset);

	if (unify16 && serialNumber.GetLength() != 16)
	{
		if (serialNumber.Right(1) == '.')
			serialNumber = serialNumber.Left(serialNumber.GetLength() - 1);

		serialNumber.Replace(_T("_"), _T(""));

		if (serialNumber.GetLength() == 15)
			serialNumber += _T("0");
	}

	return serialNumber;
}

//숫자로 구성된 문자열을 입력받아 정해진 연산을 한 후 뒤섞인 숫자문자열을 리턴한다.
CString	ShuffleNumericString(CString sSrc, bool bSameLength)
{
	if (!IsNumericString(sSrc))
		return _T("");

	double	dSource = _tstof(sSrc);
	double	dResult;
	CString	sResult;

	dResult = ((dSource * 529.0) - 529.0) / 79.0;
	dResult = sin(dResult) * 1000000.0;

	if (dResult < 0.0)
		dResult *= -1;

	sResult.Format(_T("%f"), dResult);
	sResult.Remove(_T('.'));

	if (bSameLength)
	{
		if (sResult.GetLength() < sSrc.GetLength())
		{
			CString sTemp = sResult;

			for (int i = 0; i < sSrc.GetLength() - sResult.GetLength(); i++)
			{
				sTemp += "0";
			}

			sResult = sTemp;
		}
		else if (sResult.GetLength() > sSrc.GetLength())
		{
			sResult = sResult.Left(sSrc.GetLength());
		}
	}

	return sResult;
}

CString GetHDDVolumeNumber(CString sDrive)
{
	DWORD	dwSerialNumber;
	CString	str;

	if (sDrive.GetLength() == 1)
		sDrive += ":\\";

	GetVolumeInformation(sDrive, NULL, 0, &dwSerialNumber, NULL, NULL, NULL, 0);
	str.Format(_T("%u"), dwSerialNumber);
	return str;
}

#pragma comment(lib, "mpr.lib")

CString	get_drive_volume(TCHAR drive_letter)
{
	TCHAR Label[MAX_PATH] = { 0, };
	memset(Label, 0, sizeof(Label));
	CString drive_root;

	if (drive_letter >= 'a' && drive_letter <= 'z')
		drive_letter = toupper(drive_letter);

	drive_root.Format(_T("%c:\\"), drive_letter);
	GetVolumeInformation(drive_root, Label, sizeof(Label), NULL, NULL, NULL, NULL, 0);

	CString sLabel = Label;

	/*
	LANGID langID = GetUserDefaultUILanguage();
	// Extract the primary language and sublanguage
	WORD primaryLanguage = PRIMARYLANGID(langID);
	WORD subLanguage = SUBLANGID(langID);

	// Optionally, convert to a human-readable name using LCIDToLocaleName
	TCHAR localeName[LOCALE_NAME_MAX_LENGTH];
	if (LCIDToLocaleName(MAKELCID(langID, SORT_DEFAULT), localeName, LOCALE_NAME_MAX_LENGTH, 0))
	{
		TRACE(_T("lang = %d(%s)\n"), langID, localeName);
	}
	*/

	if (sLabel.IsEmpty())
	{
		sLabel.Format(_T("%s (%c:)"), GetUserDefaultUILanguage() == 1042 ? _T("로컬 디스크") : _T("Local Disk"), drive_letter);
	}
	else
	{
		sLabel.Format(_T("%s (%c:)"), sLabel, drive_letter);
	}

	if (GetDriveType(drive_root.Left(2)) == DRIVE_REMOTE)
	{
		unsigned long bufferLength = MAX_PATH;
		WNetGetConnection(drive_root.Left(2), Label, &bufferLength);
		//네트워크 드라이브의 Label이 "\\\\192.168.1.103\\연구소문서" 와 같이 구해진다.
		//막상 탐색기에서는 "연구소문서(\\192.168.1.103) (Y:)" 와 같이 표현하고 있다. 보정해준다.
		sLabel = Label;
		sLabel.Format(_T("%s(%s) (%c:)"),
			sLabel.Mid(sLabel.ReverseFind('\\') + 1),
			sLabel.Left(sLabel.ReverseFind('\\')),
			drive_letter);
	}

	return sLabel;
}

//드라이브 패스는 "C:\\"와 같이 3개 문자로 구성되고 첫문자는 대문자로 표시하는 것이 일반적이다.
CString	normalize_drive_path(CString drive_path)
{
	if (drive_path.IsEmpty())
		return _T("");

	drive_path.MakeUpper();

	if (drive_path.GetLength() == 1)
		drive_path += _T(":\\");
	else if (drive_path.GetLength() == 2 && drive_path.Right(1) == ';')
		drive_path += _T("\\");
	else
		drive_path = drive_path.Left(3);

	return drive_path;
}

void get_drive_list(std::deque<CString> *drive_list, bool include_legacy)
{
	DWORD dwError = 0;
	TCHAR tzDriveString[MAX_PATH] = { 0, };
	CString strDrive;

	drive_list->clear();

	DWORD logicalDrives = GetLogicalDrives();
	unsigned int i = 64;

	do
	{
		i++;
		if ((logicalDrives & 1) != 0)
		{
			strDrive.Format(_T("%c:\\"), i);
			UINT driveType = GetDriveType(strDrive);

			// CD-ROM이나 floppy disk는 하위 폴더를 표시하지 않는다.
			if (!include_legacy && (driveType == DRIVE_REMOVABLE || driveType == DRIVE_CDROM))
				continue;

			drive_list->push_back(get_drive_volume(strDrive[0]));
		}
	} while ((logicalDrives >>= 1) != 0);
}


/*
//"내 PC\\로컬 디스크 (C:)" -> "C:\\"
//"로컬 디스크 (C:)" -> "C:\\"
//"문서" -> "C:\\Documents"
//"문서\\AnySupport" -> "C:\\Documents\\AnySupport"
//"Seagate(\\192.168.0.52) (X:)" -> "X:"	(네트워크 드라이브)
//하위 폴더 포함 유무에 관계없이 변환.
CString	convert_special_folder_to_real_path(CString special_folder, CShellImageList *plist, int index)
{
	//실제 존재한다고 판별되면 이는 real_path이므로 그대로 리턴.
	if (special_folder.IsEmpty() || PathFileExists(special_folder))
		return special_folder;

	//"내 PC"가 존재하면 날린다.
	CString myPC_label;
	
	if (plist && plist->m_volume.size() > index)
		myPC_label = plist->m_volume[index].get_label(CSIDL_DRIVES);
	else
		myPC_label = get_system_label(CSIDL_DRIVES);

	if (special_folder == myPC_label)
		return myPC_label;

	//"내 PC\\연구소문서(\\\\192.168.1.103) (Y:)"
	if (special_folder.Find(myPC_label) >= 0)
		special_folder.Replace(myPC_label, _T(""));

	//"\\연구소문서(\\\\192.168.1.103) (Y:)"와 같이 맨 앞에 '\\'가 붙어있다면 제거.
	if (special_folder.GetLength() > 1 && special_folder.Left(1) == '\\')
		special_folder = special_folder.Mid(1);

	CString real_path = special_folder;
	CString drive_prefix;

	//"바탕 화면\\", "문서", "문서\\abc"와 같이 넘어오므로 맨 처음 항목의 실제 경로를 구해야 한다.
	//"Seagate(\\192.168.0.52) (X:)"과 같은 네트워크 드라이브도 존재하므로 '\\'로만 판별해서는 안된다.
	bool is_network_drive = false;

	int pos1 = special_folder.Find(_T(":)"));
	int pos2 = special_folder.Find(_T("\\\\"));
	if (pos1 > 0 && pos2 > 0 && pos1 > pos2)
	{
		is_network_drive = true;
		drive_prefix = special_folder.Left(special_folder.Find(_T(":)")) + 2);
	}
	else
	{
		std::deque<CString> token;
		get_token_string(special_folder, token, _T("\\"), false);
		drive_prefix = token[0];
	}

	CString rest_path = special_folder;
	rest_path.Replace(drive_prefix, _T(""));

	//local일 경우는 직접 구해서 비교하고
	if (plist == NULL)
	{
		if (drive_prefix == ::get_system_label(CSIDL_DRIVES))
			real_path.Format(_T("%s%s"), ::get_system_label(CSIDL_DRIVES), rest_path);
		else if (drive_prefix == ::get_system_label(CSIDL_DESKTOP))
			real_path.Format(_T("%s%s"), get_known_folder(CSIDL_DESKTOP), rest_path);
		else if (drive_prefix == ::get_system_label(CSIDL_MYDOCUMENTS))
			real_path.Format(_T("%s%s"), get_known_folder(CSIDL_MYDOCUMENTS), rest_path);
		else
		{
			std::deque<CDiskDriveInfo> drive_list;
			get_drive_list(&drive_list);

			for (int i = 0; i < drive_list.size(); i++)
			{
				//"로컬 디스크 (C:)"
				if (_tcsicmp(drive_list[i].label, drive_prefix) == 0)
				{
					int pos = real_path.Find(_T(":)"));
					if (pos < 0)
						return real_path;

					CString rest = real_path.Mid(pos + 2);
					CString drive_letter = real_path.Mid(pos - 1, 1);

					if (rest.Left(1) == _T("\\"))
						rest = rest.Mid(1);

					real_path.Format(_T("%s:\\%s"), drive_letter, rest);
					break;
				}
			}
		}
	}
	//remote일 경우는 map을 이용해서 비교
	else
	{
		if (drive_prefix == plist->m_volume[index].get_label(CSIDL_DRIVES))
			real_path.Format(_T("%s%s"), plist->m_volume[index].get_label(CSIDL_DRIVES), rest_path);
		else if (drive_prefix == ::get_system_label(CSIDL_DESKTOP))
			real_path.Format(_T("%s%s"), plist->m_volume[index].get_path(CSIDL_DESKTOP), rest_path);
		else if (drive_prefix == ::get_system_label(CSIDL_MYDOCUMENTS))
			real_path.Format(_T("%s%s"), plist->m_volume[index].get_path(CSIDL_MYDOCUMENTS), rest_path);
		else
		{
			auto drive_list = plist->m_volume[index].get_drive_list();

			for (int i = 0; i < drive_list->size(); i++)
			{
				//"로컬 디스크 (C:)"
				if (_tcsicmp(drive_list->at(i).label, drive_prefix) == 0)
				{
					int pos = real_path.Find(_T(":)"));
					if (pos < 0)
						return real_path;

					CString rest = real_path.Mid(pos + 2);
					CString drive_letter = real_path.Mid(pos - 1, 1);

					if (rest.Left(1) == _T("\\"))
						rest = rest.Mid(1);

					real_path.Format(_T("%s:\\%s"), drive_letter, rest);
					break;
				}
			}
		}
	}

	return real_path;
}

//"c:\\abc\\def"				=> "로컬 디스크 (C:)\\abc\\def"
//"C:\Users\scpark\Desktop"		=> "바탕 화면"
//"C:\Users\scpark\Documents"	=> "문서"
CString	convert_real_path_to_special_folder(CString real_path, CShellImageList* plist, int index)
{
	if (plist == NULL)
	{
		if (real_path == get_known_folder(CSIDL_DESKTOP))
			return get_system_label(CSIDL_DESKTOP);
		else if (real_path == get_known_folder(CSIDL_MYDOCUMENTS))
			return get_system_label(CSIDL_MYDOCUMENTS);
	}
	else
	{
		if (real_path == plist->m_volume[index].get_path(CSIDL_DESKTOP))
			return plist->m_volume[index].get_label(CSIDL_DESKTOP);
		else if (real_path == plist->m_volume[index].get_path(CSIDL_MYDOCUMENTS))
			return plist->m_volume[index].get_label(CSIDL_MYDOCUMENTS);
	}

	CString volume_path = real_path;

	if (real_path.Mid(1, 2) != _T(":\\"))
		return real_path;

	CString volume = get_drive_volume(real_path[0]);

	if (plist)
		volume = plist->m_volume[index].get_drive_volume(real_path);

	volume_path.Replace(CString(real_path[0]) + _T(":"), volume);

	return volume_path;
}
*/

//"c:\windows"를 입력하면 "C:\Windows"와 같이 실제 파일시스템에 저장된 경로명 리턴.
CString	get_original_path(CString path)
{
	//존재하지 않는 경로가 넘어오면 아래 함수들에서는 ""이 리턴되지만
	//이 함수의 역할로 봐서는 넘어온 그대로 리턴하는 것이 맞는 듯 하다.
	//존재하지 않는 경로에 대한 처리가 필요하다면 main에서 먼저 처리한다.
	if (!PathFileExists(path))
		return path;

	TCHAR short_path[MAX_PATH] = { 0, };
	TCHAR long_path[MAX_PATH] = { 0, };
	GetShortPathName(path, short_path, MAX_PATH);
	GetLongPathName(short_path, long_path, MAX_PATH);

	//단, 드라이브명까지 자동 변경되지 않는다.
	//소문자로 넘어오면 소문자 그대로 리턴되므로 대문자로 변경하여 리턴한다.
	//_toupper 가 아닌 반드시 _totupper를 사용함에 주의할 것.
	//_toupper는 아스키 코드값을 shift시키는 단순 매크로임.
	if (_tcslen(long_path) > 0)
		long_path[0] = _totupper(long_path[0]);

	return CString(long_path);
}

void ClickMouse(int x, int y)
{
	SetCursorPos(x, y);
	mouse_event(MOUSEEVENTF_LEFTDOWN, x, y, 0, 0);
	mouse_event(MOUSEEVENTF_LEFTUP,   x, y, 0, 0);
}

std::wstring CString2wstring(const char* str)
{
	size_t cn;
	const std::size_t size = strlen(str);
	wchar_t* buff(new wchar_t[size]);
	memset(buff, 0, size);
	mbstowcs_s(&cn, buff, size, str, size);
	return std::wstring(buff);
}

//char chStr[100] = { 0, };와 같이 pointer 변수가 아닌 배열로 선언된 경우라면
//chStr = CString2char(str); 문장은 오류가 발생하므로 아래와 같이 사용할 것.
//sprintf(chStr, "%s", (LPSTR)(LPCTSTR)str);	//MBCS : ok, UNICODE : fail
//sprintf(chStr, "%s", CStringA(str));		//both : ok
//리턴받아 사용한 char* 변수값은 사용 후 반드시 delete [] 해줄것
char* CString2char(CString str)
{
	char *char_str = NULL;

#if defined(UNICODE) || defined(_UNICODE)
	wchar_t *wchar_str;
	int char_str_len;

	wchar_str = str.GetBuffer(str.GetLength());
	char_str_len = WideCharToMultiByte(CP_ACP, 0, wchar_str, -1, NULL, 0, NULL, NULL);
	char_str = new char[char_str_len];
	memset(char_str, 0, char_str_len);

	WideCharToMultiByte(CP_ACP, 0, wchar_str, -1, char_str, char_str_len, 0, 0);
#else
	int nLen = str.GetLength() + 1;
	char_str = new char[nLen];
	memset(char_str, 0, nLen);
	strcpy(char_str, str);
#endif
	return char_str;
}

//
// CString → TCHAR
//
TCHAR* CString2TCHAR(CString str)
{
	TCHAR* tszStr = NULL;
	int nLen = str.GetLength() + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	_tcscpy_s(tszStr, nLen, str);

	return tszStr;
}

//https://m.blog.naver.com/PostView.naver?isHttpsRedirect=true&blogId=ikariksj&logNo=140186998237
LPCSTR		CString2LPCSTR(CString str)
{
#if defined(UNICODE) || defined(_UNICODE)
	//unicode 환경에서 Casting은 가능하지만
	//실제 LPCSTR 인자로 넘겨서 사용 하니 문제 발생.
	CStringA strA(str);
	LPCSTR lpcstr = strA;
	return lpcstr;
#else
	//Multibyte 환경에서는 캐스팅으로 문제 없이 사용 가능
	LPCSTR lpcstr = (LPSTR)(LPCTSTR)str;
	return lpcstr;
#endif
}

LPCWSTR	LPCTSTR2LPCWSTR(LPCTSTR str, UINT codePage)
{
#ifdef UNICODE
	return str;
#else
	USES_CONVERSION;
	return A2W_CP(str, codePage);
#endif
}

WCHAR* CString2WCHAR(CString str)
{
	return (WCHAR*)(const WCHAR*)CStringW(str);
}

//
// CString → std::string
//
std::string CString2string(CString str)
{
	return std::string(CT2CA(str));
	/*
	std::string stdStr;
	char* szStr = CString2char(str);
	if (szStr)
	{
		stdStr = szStr;
		delete[] szStr;
	}

	return stdStr;
	*/
}

//cstr의 유효한 길이를 이미 알고 있다면 length를 지정해줘야 정확하다.
//그렇지 않을 경우 cstr의 끝에 '\0'가 없을 경우 쓰레기 문자들까지 포함될 수 있다.
//cstr이 '\0'로 끝난다면 유니코드, 멀티바이트 환경에서 CString str = cstr;로 정상 처리된다.
CString char2CString(char* cstr, int length)
{
	CString str;

#if defined(UNICODE) || defined(_UNICODE)
	//length가 -1인 경우, 즉 길이를 모르거나 특별히 지정하지 않고
	//cstr이 '\0'문자로 끝나는 온전한 값이라면
	//unicode에서 str = CString(cstr); 로도 정상 동작한다. 예외가 있는지 확인 필요!
	if (length < 0)
	{
		str = CA2CT(cstr);
		return str;
	}

	int len;
	TCHAR* buf;

	if (length < 0)
		len = MultiByteToWideChar(CP_ACP, 0, cstr, strlen(cstr), NULL, NULL);
	else
		len = length;

	buf = SysAllocStringLen(NULL, len);
	MultiByteToWideChar(CP_ACP, 0, cstr, len, buf, len);

	str.Format(_T("%s"), buf);
#else
	str.Format("%s", cstr);
#endif
	return str;
}

//
// Char → TCHAR
//
TCHAR* char2TCHAR(char* str)
{
	TCHAR* tszStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = strlen(str) + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	MultiByteToWideChar(CP_ACP, 0, str, -1, tszStr, nLen * sizeof(TCHAR));
#else
	int nLen = strlen(str) + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	_tcscpy(tszStr, str);
#endif
	return tszStr;
}

//
// TCHAR → CString
//
CString TCHAR2CString(TCHAR* str)
{
	CString cStr;
	cStr.Format(_T("%s"), str);
	return cStr;
}

//
// TCHAR → Char
//
char* TCHAR2char(TCHAR* str)
{
	char* szStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nSize = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, NULL, NULL, NULL);
	szStr = new char[nSize];
	memset(szStr, 0x00, nSize);
	WideCharToMultiByte(CP_ACP, 0, str, -1, szStr, nSize, NULL, NULL);
#else
	int nLen = strlen(str) + 1;
	szStr = new char[nLen];
	memset(szStr, 0x00, nLen);
	strcpy(szStr, str);
#endif
	return szStr;
}
/*
std::string CString2string(CString str)
{
#ifdef _UNICODE
	//CT2CA pszConvertedAnsiString(cs);
	return std::string(CT2CA(str));
#endif
	return std::string((LPCTSTR)str);
}
*/

LPCWSTR CString2LPCWSTR(CString str)
{
	LPCWSTR p = NULL;

#ifdef _UNICODE
	p = str;
#else
	USES_CONVERSION;
	p = A2W(str);
#endif
	return p;
}


//returned char* is must be free
/*
char* CString2char(CString str)
{
	int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	char* pString = (char*)new char[len];
	WideCharToMultiByte(CP_ACP, 0, str, -1, pString, len, NULL, NULL);
	return pString;
}
*/

int	GetNumberFromFile(TCHAR* sfile)		//파일을 읽어서 한개의 숫자값을 리턴한다. (파일이 없거나 내용이 없으면 -99999 를 리턴한다.)
{
	FILE*	fp = _tfopen(sfile, _T("r"));
	TCHAR	sLine[MAX_PATH] = _T("\0");
	CString str;

	if (fp == NULL)
		return -99999;

	_fgetts(sLine, MAX_PATH, fp);
	fclose(fp);
	str = sLine;

	if (str.GetLength() == 0)
		return -99999;
	
	return _tstoi(str);
}

bool WriteNumberToFile(TCHAR* sfile, int n)	//숫자값을 파일에 기록해준다.
{
	FILE*	fp = _tfopen(sfile, _T("w"));
	TCHAR	sLine[MAX_PATH] = _T("\0");

	if (fp == NULL)
	{
		return FALSE;
	}

	_ftprintf(fp, _T("%d"), n);
	fclose(fp);

	return TRUE;
}

inline BYTE toHex(const BYTE &x)
{
	return x > 9 ? x + 55: x + 48;
}

void hex2byte(const char *in, int len, byte *out)
{
	for (int i = 0; i < len; i += 2)
	{
		char c0 = in[i + 0];
		char c1 = in[i + 1];
		byte c = (
			((c0 & 0x40 ? (c0 & 0x20 ? c0 - 0x57 : c0 - 0x37) : c0 - 0x30) << 4) |
			((c1 & 0x40 ? (c1 & 0x20 ? c1 - 0x57 : c1 - 0x37) : c1 - 0x30))
			);
		out[i / 2] = c;
	}
}

unsigned int dec2bcd(unsigned int num)
{
	unsigned int ones = 0;
	unsigned int tens = 0;
	unsigned int temp = 0;

	ones = num%10;
	temp = num/10;
	tens = temp<<4;
	return (tens + ones);
}

CString URLEncode(CString sIn)
{
	CString sOut;
	const int nLen = sIn.GetLength() + 1;
	LPBYTE pOutTmp = NULL;
	LPBYTE pOutBuf = NULL;
	LPBYTE pInTmp = NULL;
	LPBYTE pInBuf =(LPBYTE)sIn.GetBuffer(nLen);
	BYTE b = 0;
	//alloc out buffer
	pOutBuf = (LPBYTE)sOut.GetBuffer(nLen*3 - 2);//new BYTE [nLen * 3];
	if(pOutBuf)
	{
		pInTmp   = pInBuf;
		pOutTmp = pOutBuf;
		// do encoding
		while (*pInTmp)
		{
			if(isalnum(*pInTmp))
				*pOutTmp++ = *pInTmp;
			else
				if(isspace(*pInTmp))
					*pOutTmp++ = '+';
				else
				{
					*pOutTmp++ = '%';
					*pOutTmp++ = toHex(*pInTmp>>4);
					*pOutTmp++ = toHex(*pInTmp%16);
				}
				pInTmp++;
		}
		*pOutTmp = '\0';
		//sOut=pOutBuf;
		//delete [] pOutBuf;
		sOut.ReleaseBuffer();
	}
	sIn.ReleaseBuffer();
	return sOut;
}

CString Utf8ToStringT(LPSTR str)
{
	_ASSERT(str);
	USES_CONVERSION;
	WCHAR *buf;
	int length = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	buf = new WCHAR[length+1];
	ZeroMemory(buf, (length+1) * sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, length);
	return (CString(W2T(buf)));
}

/*
char* UTF8toANSI(char *pszCode)
{
    BSTR    bstrWide;
    char*   pszAnsi;
    int     nLength;    
    // Get nLength of the Wide Char buffer
    nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, lstrlen(pszCode) + 1,NULL, NULL);
    bstrWide = SysAllocStringLen(NULL, nLength);
    // Change UTF-8 to Unicode (UTF-16)
    MultiByteToWideChar(CP_UTF8, 0, pszCode, lstrlen(pszCode) + 1, bstrWide,nLength);
    // Get nLength of the multi byte buffer<
    nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
    pszAnsi = new char[nLength];
    // Change from unicode to mult byte>
    WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
    SysFreeString(bstrWide);
    return pszAnsi;
}
*/

CString UrlDecode(LPCTSTR url)
{
	_ASSERT(url);
	USES_CONVERSION;
	LPSTR _url = T2A(const_cast<LPTSTR>(url));
	int i = 0;
	int length = (int)strlen(_url);
	char *buf = new char[length];
	ZeroMemory(buf, length);
	LPSTR p = buf;
	while(i < length)
	{
		if(i <= length -3 && _url[i] == '%' && IsHexaChar(_url[i+1]) && IsHexaChar(_url[i+2]))
		{
			_stscanf((TCHAR*)_url + i + 1, _T("%x"), p++);
			i += 3;
		}
		else
		{
			*(p++) = _url[i++];
		}
	}
	return Utf8ToStringT(buf);
}


//for cpu usage
#ifndef DEFAULT_PERF_BUFFER
#define DEFAULT_PERF_BUFFER (2048*10)
#endif

#define PERF_BUFFER_INCREMENT (1024*10)

LPBYTE GetPerformanceData(LPTSTR src)
{
	DWORD bufSize = DEFAULT_PERF_BUFFER;

	LPBYTE buf = (LPBYTE)malloc(bufSize);
	LPBYTE tmpBuf = NULL;

	if(NULL == buf)
		return NULL;

	LONG lRes;

	while (ERROR_MORE_DATA == (lRes = RegQueryValueEx(HKEY_PERFORMANCE_DATA, src, NULL, NULL, buf, &bufSize))) {
		// Get a more buffer
		bufSize += PERF_BUFFER_INCREMENT;

		tmpBuf = (LPBYTE)realloc(buf, bufSize);

		if(NULL != tmpBuf) {
			buf = tmpBuf;
		}
		else
		{
			AfxMessageBox(_T("Reallocation error\n"));
			RegCloseKey(HKEY_PERFORMANCE_DATA);
			free(buf);
			return NULL;
		}
	}


	if(ERROR_SUCCESS != lRes)
	{
		AfxMessageBox(_T("RegQueryValueEx failed with 0x%x.\n"), lRes);
		free(buf);
	}

	RegCloseKey(HKEY_PERFORMANCE_DATA);

	return buf;
}

int GetCounterValue(const int& objId, const int& counterId, const char* instanceName, PERF_DATA_BLOCK **dataBlock, LONGLONG &value)
{
	TCHAR key[256];
	_stprintf(key, _T("%d"), objId);

	PPERF_DATA_BLOCK   perfDataBlock = NULL;
	PPERF_OBJECT_TYPE perfObj = NULL;

	PPERF_COUNTER_DEFINITION perfCounterDef = NULL;
	PPERF_COUNTER_BLOCK perfCounterBlock = NULL;
	PPERF_INSTANCE_DEFINITION perfInstanceDef = NULL;

	perfDataBlock = (PPERF_DATA_BLOCK)GetPerformanceData(key); // process object

	if(NULL == perfDataBlock) return -1;

	*dataBlock = perfDataBlock;

	perfObj = (PPERF_OBJECT_TYPE)((PBYTE)perfDataBlock + perfDataBlock->HeaderLength); // first object

	for(DWORD i = 0; i < perfDataBlock->NumObjectTypes; ++i) {
		if(perfObj->ObjectNameTitleIndex == objId) {
			perfCounterDef = (PPERF_COUNTER_DEFINITION)((PBYTE)perfObj + perfObj->HeaderLength); // first counter definition

			for(DWORD j=0; j < perfObj->NumCounters; ++j){
				if(perfCounterDef->CounterNameTitleIndex == counterId) break;

				perfCounterDef = (PPERF_COUNTER_DEFINITION)((PBYTE)perfCounterDef + perfCounterDef->ByteLength); // next counter definition
			}

			if(perfObj->NumInstances == PERF_NO_INSTANCES) {
				perfCounterBlock = (PPERF_COUNTER_BLOCK)((PBYTE)perfObj + perfObj->DefinitionLength);
			} else {
				perfInstanceDef = (PPERF_INSTANCE_DEFINITION)((PBYTE)perfObj + perfObj->DefinitionLength); // first instance

				_bstr_t bstrInst;
				_bstr_t bstrInInst = instanceName;
				for(int k=0; k < perfObj->NumInstances; ++k) {
					bstrInst = (wchar_t*)((PBYTE)perfInstanceDef + perfInstanceDef->NameOffset);
					if(!_tcscmp((LPCTSTR)bstrInst, (LPCTSTR)bstrInInst)) {
						perfCounterBlock = (PPERF_COUNTER_BLOCK)((LPBYTE)perfInstanceDef + perfInstanceDef->ByteLength);
						break;
					}

					PPERF_COUNTER_BLOCK tmpblock = PPERF_COUNTER_BLOCK((PBYTE)perfInstanceDef + perfInstanceDef->ByteLength);
					perfInstanceDef = (PPERF_INSTANCE_DEFINITION)((PBYTE)tmpblock + tmpblock->ByteLength);
				}
			}
		}
		perfObj = (PPERF_OBJECT_TYPE)((PBYTE)perfObj + perfObj->TotalByteLength); // next object
	}

	if(NULL != perfCounterBlock) {
		value = *((LONGLONG*) ((LPBYTE)perfCounterBlock + perfCounterDef->CounterOffset));
		return 0;
	} else {
		return -1;
	}
}

double get_cpu_usage(const char* process)
{
	const int CPU_INDEX = 230; // Perf: Process
	const int CPU_COUNTER = 6;  

	static bool isCpuFirst = true;
	static LARGE_INTEGER oldCpuTime_100n = {0};
	static LONGLONG oldCpuVal = 0;

	LONGLONG newVal = 0;
	LARGE_INTEGER cpuTime_100n = {0};
	double cpuUsage = 0.0;

	PPERF_DATA_BLOCK cpuData = NULL;
	// Process 객체(CPU_INDEX)에서 해당 process 이름을 갖는 프로세스의 CPU 값을 얻는다.
	if (GetCounterValue(CPU_INDEX, CPU_COUNTER, process, &cpuData, newVal) < 0)
		return -1;
	// 획득한 성능 정보를 이용해서 CPU 사용율을 계산
	cpuTime_100n = cpuData->PerfTime100nSec;

	if (isCpuFirst)
	{
		isCpuFirst = false;
		oldCpuVal = newVal;
		oldCpuTime_100n = cpuTime_100n;
	}
	else
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);

		LONGLONG delta = newVal - oldCpuVal;
		double deltaTime = (double)cpuTime_100n.QuadPart - (double)oldCpuTime_100n.QuadPart;

		oldCpuVal = newVal;
		oldCpuTime_100n = cpuTime_100n;

		double a = (double)delta / deltaTime;

		cpuUsage = (a*100) / sysinfo.dwNumberOfProcessors;
	}

	if(NULL != cpuData){
		free(cpuData);
		cpuData = NULL;
	}

	return cpuUsage;
}

#define _WIN32_DCOM
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
HRESULT get_cpu_temperature(LPLONG pTemperature)
{
	if (pTemperature == NULL)
		return E_INVALIDARG;

	*pTemperature = -1;
	HRESULT ci = CoInitialize(NULL);
	HRESULT hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (SUCCEEDED(hr))
	{
		IWbemLocator* pLocator;
		hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator);
		if (SUCCEEDED(hr))
		{
			IWbemServices* pServices;
			BSTR ns = SysAllocString(L"root\\WMI");
			hr = pLocator->ConnectServer(ns, NULL, NULL, NULL, 0, NULL, NULL, &pServices);
			pLocator->Release();
			SysFreeString(ns);
			if (SUCCEEDED(hr))
			{
				BSTR query = SysAllocString(L"SELECT * FROM MSAcpi_ThermalZoneTemperature");
				BSTR wql = SysAllocString(L"WQL");
				IEnumWbemClassObject* pEnum;
				hr = pServices->ExecQuery(wql, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
				SysFreeString(wql);
				SysFreeString(query);
				pServices->Release();
				if (SUCCEEDED(hr))
				{
					IWbemClassObject* pObject;
					ULONG returned;
					hr = pEnum->Next(WBEM_INFINITE, 1, &pObject, &returned);
					pEnum->Release();
					if (SUCCEEDED(hr))
					{
						BSTR temp = SysAllocString(L"CurrentTemperature");
						VARIANT v;
						VariantInit(&v);
						hr = pObject->Get(temp, 0, &v, NULL, NULL);
						pObject->Release();
						SysFreeString(temp);
						if (SUCCEEDED(hr))
						{
							*pTemperature = V_I4(&v);
						}
						VariantClear(&v);
					}
				}
			}
			if (ci == S_OK)
			{
				CoUninitialize();
			}
		}
	}
	return hr;
}

//현재 가용 메모리를 리턴한다. (total_memory : 전체 메모리 용량)
uint64_t get_available_memory(uint64_t* total_memory)
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);

	if (total_memory)
		*total_memory = status.ullTotalPhys;

	return status.ullAvailPhys;
}

unsigned int Crc16(unsigned char* rdata, unsigned int len)
{
	unsigned int	i, j;
	unsigned int	wCrc = 0xFFFF;
	unsigned int	wCh;

	for (i = 0; i < len; i++)
	{
		wCh = (unsigned int) *(rdata + i);

		for (j = 0; j < 8; j++)
		{
			if ((wCh^wCrc) & 0x0001)
				wCrc = (wCrc >> 1) ^ 0xA001;
			else
				wCrc >>= 1;

			wCh >>= 1;
		}
	}

	return wCrc;	
}

/**
* @brief  Update CRC16 for input byte
* @param  crc_in input value
* @param  input byte
* @retval None
*/
uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte)
{
	uint32_t crc = crc_in;
	uint32_t in = byte | 0x100;

	do
	{
		crc <<= 1;
		in <<= 1;
		if(in & 0x100)
			++crc;
		if(crc & 0x10000)
			crc ^= 0x1021;
	}

	while(!(in & 0x10000));

	return crc & 0xffffu;
}

/**
* @brief  Cal CRC16 for YModem Packet
* @param  data
* @param  length
* @retval None
*/
uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size)
{
	uint32_t crc = 0;
	const uint8_t* dataEnd = p_data+size;

	while(p_data < dataEnd)
		crc = UpdateCRC16(crc, *p_data++);

	crc = UpdateCRC16(crc, 0);
	crc = UpdateCRC16(crc, 0);

	return crc&0xffffu;
}

double GetElapsedTime(__timeb32 pOldTime)	//pOldTime과 현재 시간의 차이 계산
{
	__timeb32	tCur;

	_ftime32(&tCur);

	time_t t0 = pOldTime.time * 1000 + pOldTime.millitm;
	time_t t1 = tCur.time * 1000 + tCur.millitm;

	return (double)(t1 - t0) / 1000.0 ;
}

CString	GetDayTimeCountString(int format, CTimeSpan ts, bool bShowZero, bool bIncludeSec)	//ts값을 넘겨 받아 "a일 b시간 c분 d초" 형태로 표시
{
	CString str;
	CString sResult = _T("");

	if (format == 1)
	{
		sResult.Format(_T("%02d:%02d:%02d"), ts.GetHours(), ts.GetMinutes(), ts.GetSeconds());
		return sResult;
	}

	if (ts.GetDays() || bShowZero)
	{
		str.Format(_T("%d일"), ts.GetDays());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetHours() || bShowZero)
	{
		str.Format(_T("%d시간"), ts.GetHours());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetMinutes() || bShowZero)
	{
		str.Format(_T("%d분"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	if (bIncludeSec && (sResult != "" || ts.GetMinutes() || bShowZero))
	{
		str.Format(_T("%d초"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	sResult.TrimLeft();

	if (sResult == "")
	{
		if (bIncludeSec)
			sResult = "0초";
		else
			sResult = "0분";
	}

	return sResult;
}

//ts값을 넘겨 받아 "a일 b시간 c분 d초" 형태로 표시
CString	GetDayTimeCountString(COleDateTimeSpan ts, bool bShowZero, bool bIncludeSec)
{
	CString str;
	CString sResult = _T("");

	if (ts.GetDays() || bShowZero)
	{
		str.Format(_T("%d일"), ts.GetDays());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetHours() || bShowZero)
	{
		str.Format(_T("%d시간"), ts.GetHours());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetMinutes() || bShowZero)
	{
		str.Format(_T("%d분"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	if (bIncludeSec && (sResult != "" || ts.GetMinutes() || bShowZero))
	{
		str.Format(_T("%d초"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	sResult.TrimLeft();

	if (sResult == "")
	{
		if (bIncludeSec)
			sResult = "0초";
		else
			sResult = "0분";
	}

	return sResult;
}

bool IsAM(CTime t /*= 0*/)
{
	//t값이 0인 경우는 현재 시각을 얻어와서 오전, 오후를 판단해준다.
	if (t == 0)
		t = CTime::GetCurrentTime();
	
	if (t.GetHour() >= 0 && t.GetHour() < 12)
		return true;

	return false;
}

CString	GetDayOfWeekString(CTime t, bool short_str)
{
	if (t == NULL)
		t = CTime::GetCurrentTime();

	CString sWeek;

	switch (t.GetDayOfWeek())
	{
			case 1	:	sWeek = "일요일";
						break;
			case 2	:	sWeek = "월요일";
						break;
			case 3	:	sWeek = "화요일";
						break;
			case 4	:	sWeek = "수요일";
						break;
			case 5	:	sWeek = "목요일";
						break;
			case 6	:	sWeek = "금요일";
						break;
			case 7	:	sWeek = "토요일";
						break;
	}

	if (short_str)
	{
#ifdef _UNICODE
		return sWeek.Left(1);
#else
		return sWeek.Left(2);
#endif
	}

	return sWeek;
}

//해당 달의 날짜수 리턴
int	GetDaysOfMonth(int nYear, int nMonth)
{
	if (IsLeapYear(nYear) && nMonth == 2)
		return g_nDaysOfMonth[nMonth - 1] + 1;

	return g_nDaysOfMonth[nMonth - 1];
}

void SortStringArray(CStringArray* pArray)
{
	bool bNotDone = TRUE;

	while (bNotDone)
	{
		bNotDone = FALSE;
		for(int pos = 0;pos < pArray->GetUpperBound();pos++)
			bNotDone |= StringArrayCompareAndSwap(pArray, pos);
	}
}

bool StringArrayCompareAndSwap(CStringArray* pArray, int pos)
{
	CString temp;
	int posFirst = pos;
	int posNext = pos + 1;

	if (pArray->GetAt(posFirst).CompareNoCase(pArray->GetAt(posNext)) > 0)
	{
		temp = pArray->GetAt(posFirst);
		pArray->SetAt(posFirst, pArray->GetAt(posNext));
		pArray->SetAt(posNext, temp);
		return TRUE;
	}

	return FALSE;
}

//#include <algorithm>
/*
template<class T> void quicksort(T *data, int end, int start)
{
	while(end>start)
	{
		int i=start;
		int j=end;
		do
		{
			while(data[i]<data[start]&&i<j)
				i++;
			while(data[--j]>data[start])
				;
			if(i<j)
				swap(data[i],data[j]);
		}while(i<j);
		swap(data[start],data[j]);

		if(j-start>end-(j+1)) {
			quicksort(data,j-1,start);
			start=j+1;
		}else{
			quicksort(data,end,j+1);
			end=j-1;
		}
	}
}
*/
/*
template<class T> void quickSort(T *a, const int& leftarg, const int& rightarg)
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
int compareInteger (const void * a, const void * b)
{
	return (*(int*)a - *(int*)b);
}

int compareChar(const void *arg1, const void *arg2)
{
	/* Compare all of both strings: */
	return _stricmp(* (char**) arg1, * (char**) arg2);
}

int compareString (const void * a, const void * b)
{
	CString str0 = *(CString*)a;
	CString str1 = *(CString*)b;

	return str0.Compare(str1);
}

CString FormattedString(LPCTSTR lpszFormat, ...)
{
	CString sResult;

	va_list args = NULL;
	va_start(args, lpszFormat);

	sResult.FormatV(lpszFormat, args);

	va_end(args);

	return sResult;
}
/*
int random(int minimum, int maximum)
{
	//여기에서 srand를 호출하면 원하는대로 동작하지 않는다. 메인에서 시작시 한번만 호출하자.
	//srand(time(NULL));
	if (minimum > maximum)
		Swap(minimum, maximum);
	return minimum + rand() % (maximum - minimum + 1);
}

double random(double minimum, double maximum)
{
	//여기에서 srand를 호출하면 원하는대로 동작하지 않는다. 메인에서 시작시 한번만 호출하자.
	//srand(time(NULL));
	if (minimum > maximum)
		Swap(minimum, maximum);
	return minimum + (maximum - minimum) * rand() / (RAND_MAX + 1.0);
}
*/
//지도 좌표 <-> 도분초 변환
double		convert_gps_coord(int d, int m, double s)
{
	return (d + (double)m/60.0 + s/3600.0);
}

void		convert_gps_coord(double gps, int &d, int &m, double &s)
{
	d = int(gps);
	m = int((gps-d)*60.0);
	s = ((gps-d)*60.0 - m) * 60.0;
}

double Rounding(double x, int digit)
{
    return (floor((x) * pow(float(10), digit) + 0.5f) / pow(float(10), digit));
}

//src내의 모든 문자에 대해 digits자릿수의 조합 생성
void combination(std::vector<TCHAR> src, CString temp, std::vector<CString> &result, int depth)
{
	if (depth == 0)//result.size())  // depth == n // 계속 안뽑다가 r 개를 채우지 못한 경우는 이 곳에 걸려야 한다.
	{
		result.push_back(temp);
		TRACE(_T("%s\n"), temp);
		return;
	}

	//for (int i = 0; i < src.size(); i++)
	//{
	//	temp.SetAt(depth-1, src[i]);
	//	combination(src, temp, result, depth - 1);
	//}
	for (auto& ch : src)//int i = 0; i < src.size(); i++)
	{
		temp.SetAt(depth - 1, ch);
		combination(src, temp, result, depth - 1);
	}

}

//대각 각도로 수평, 수직 화각을 수학적으로 계산한다.
//실제 렌즈 스펙과 반드시 일치하진 않는다.
void get_HV_angle_from_diagonal(int diagonal, int *h, int *v, int width, int height)
{
	double dh, dv;
	get_HV_angle_from_diagonal((double)diagonal, &dh, &dv, (double)width, (double)height);
	*h = (int)dh;
	*v = (int)dv;
}

void get_HV_angle_from_diagonal(double diagonal, double *h, double *v, double width, double height)
{
	*h = diagonal * cos(atan2(height, width));
	*v = diagonal * sin(atan2(height, width));
}

//<프로세스 이름으로 프로세스 ID 얻기>
ULONG GetPID(CString processname)
{
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 pe32 = { 0, };

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32))

	{
		OutputDebugString(_T("Error checking process"));
		CloseHandle(hProcessSnap);
		return false;
	}

	do
	{
		if (StrCmp(pe32.szExeFile, processname) == 0)
			return pe32.th32ProcessID;

	} while (Process32Next(hProcessSnap, &pe32));


	CloseHandle(hProcessSnap);

	return false;
}

// 윈도우 핸들로 프로세스 아이디 얻기   
ULONG ProcIDFromWnd(HWND hwnd)
{   
	ULONG idProc = 0;   
	::GetWindowThreadProcessId(hwnd, &idProc);   
	return idProc;   
}

// 프로세스 아이디로 윈도우 핸들 얻기   
HWND get_hwnd_by_pid(ULONG pid)
{   
	HWND tempHwnd = ::FindWindow(NULL,NULL); // 최상위 윈도우 핸들 찾기   

	while(tempHwnd != NULL)   
	{   
		TRACE("tempHwnd = %p, pid = %u\n", tempHwnd, ProcIDFromWnd(tempHwnd));

		if (::GetParent(tempHwnd) == NULL) // 최상위 핸들인지 체크, 버튼 등도 핸들을 가질 수 있으므로 무시하기 위해   
		{
			if (pid == ProcIDFromWnd(tempHwnd))
				return tempHwnd;
		}

		tempHwnd = ::GetWindow(tempHwnd, GW_HWNDNEXT); // 다음 윈도우 핸들 찾기   
	}   
	return NULL;
}

//출처: https://smok95.tistory.com/300?category=28201 [Only YOUng:티스토리]
#ifndef _USING_V110_SDK71_
CString GetProcessNameByPID(const DWORD pid)
{
	CString name = _T("unknown");

	DWORD error = 0;

	if (HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid))
	{
		TCHAR buf[512] = { 0, };
		DWORD bufLen = sizeof(buf);
		QueryFullProcessImageName(hProc, 0, buf, &bufLen);
		CloseHandle(hProc);
		name = buf;
	}

	return name;
}
#endif

HWND hWndToFind = NULL;

BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam)
{
	DWORD pid;
	GetWindowThreadProcessId(hwnd,&pid);

	if ((DWORD)lParam == pid)// && IsWindowVisible(hwnd))
	{
#if defined(_DEBUG)
		TCHAR name[255]={0,};
		GetWindowText(hwnd,name,255);
		TRACE("%s 0x%08x(%d) 0x%08x(%d)\n",name, hwnd,hwnd,pid,pid);
#endif
		hWndToFind = hwnd;
		return FALSE;
	}

	return TRUE;
}

HANDLE GetProcessHandleByName(LPCTSTR szFilename)
{
	HANDLE hProcessSnapshot;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD sessionID;

	sessionID = WTSGetActiveConsoleSessionId();
	hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);

	if (hProcessSnapshot == INVALID_HANDLE_VALUE)
	{
		return INVALID_HANDLE_VALUE;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hProcessSnapshot, &pe32);

	do
	{

		TCHAR sFilePath[MAX_PATH] = { 0, };
		DWORD bufLen = MAX_PATH;

		_tcscpy(sFilePath, pe32.szExeFile);

		//전체경로로 검색하는 경우
		if (PathFileExists(szFilename))
		{
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

			if (hProcess != NULL)
			{
				//QueryFullProcessImageName(hProcess, NULL, sFilePath, &bufLen);
				::GetModuleFileNameEx(hProcess, NULL, sFilePath, MAX_PATH);
				//TRACE(_T("%s\n"), sFilePath);
				CloseHandle(hProcess);
			}
		}

		if (_tcsicmp(sFilePath, szFilename) == 0)
		{
			get_hwnd_by_pid(pe32.th32ProcessID);
			CloseHandle(hProcessSnapshot);
		}
	} while (Process32Next(hProcessSnapshot, &pe32));

	CloseHandle(hProcessSnapshot);

	return INVALID_HANDLE_VALUE;
}

//실행파일명으로부터 윈도우 핸들 리턴. 실행파일명 또는 fullpath로 검색.
HWND get_hwnd_by_exe_file(CString target_exe_file)
{
	HANDLE			hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL); 
	PROCESSENTRY32	pe32 = { 0, };
	HWND			hWnd = NULL;

	if (!hSnapShot)
	{
		return NULL;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	CString exe_name;

	if (!Process32First(hSnapShot, &pe32))
	{
		CloseHandle(hSnapShot);
		return NULL;
	}

	do 
	{
		exe_name = pe32.szExeFile;
		exe_name.MakeLower();
		target_exe_file.MakeLower();

		TCHAR sFilePath[MAX_PATH] = { 0, };
		DWORD bufLen = MAX_PATH;

		//target_exe_file이 실행 파일명만 있다면 exe 파일명만 비교하고
		//전체 경로라면 fullpath를 구해서 비교한다.
		//단 hProcess가 NULL이라서 전체경로를 구하지 못하는 프로세스도 있다.
		if (PathFileExists(target_exe_file))
		{
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

			if (hProcess)
			{
				//QueryFullProcessImageName(hProcess, NULL, sFilePath, &bufLen);
				::GetModuleFileNameEx(hProcess, NULL, sFilePath, MAX_PATH);
				//TRACE(_T("%s\n"), sFilePath);
				CloseHandle(hProcess);
			}
		}

		if (_tcsicmp(sFilePath, target_exe_file) == 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			if (hProcess != NULL)
			{
				hWnd = get_hwnd_by_pid(pe32.th32ProcessID);
				CloseHandle(hProcess);
				break;
			}
		}
	}
	while (Process32Next(hSnapShot, &pe32)); 

	CloseHandle(hSnapShot);

	return hWnd;
}

//return value : 1(killed), 0(fail to kill), -1(not found)
int	kill_process_by_fullpath(CString fullpath)
{
	int res = -1;

	if (fullpath.IsEmpty() || !PathFileExists(fullpath) || get_process_running_count(fullpath) <= 0)
		return -1;

	HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pe32 = { 0, };
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnapshot, &pe32))
	{
		OutputDebugString(_T("Error while checking process"));
		CloseHandle(hProcessSnapshot);
		return 0;
	}

	do
	{
		if (pe32.th32ProcessID != 0)
		{
			TCHAR sFilePath[1024] = { 0, };
			DWORD bufLen = 1024;

			_tcscpy(sFilePath, pe32.szExeFile);

			//processname이 실행파일명만 있다면 exe 파일명만 비교하고
			//전체 경로라면 fullpath를 구해서 비교한다.
			//단 hProcess가 NULL이라서 전체경로를 구하지 못하는 프로세스도 있다.
			//HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

			if (hProcess)
			{
				//QueryFullProcessImageName(hProcess, NULL, sFilePath, &bufLen);
				::GetModuleFileNameEx(hProcess, NULL, sFilePath, MAX_PATH);
				//TRACE(_T("%s\n"), sFilePath);
				CloseHandle(hProcess);

				if (_tcsicmp(sFilePath, fullpath) == 0)
				{
					hProcess = OpenProcess(PROCESS_TERMINATE, false, pe32.th32ProcessID);
					TerminateProcess(hProcess, 1);
					CloseHandle(hProcess);
					res = 1;
					break;
				}
			}
			else
			{

			}
		}
	} while (Process32Next(hProcessSnapshot, &pe32));

	CloseHandle(hProcessSnapshot);

	return res;
}

bool is_running(CString processname)
{
	return (get_process_running_count(processname) > 0);
}

//해당 파일이 실행중인 카운트를 리턴하는 함수이며
//fullpath를 주면 경로까지 동일해야 카운트되도록 기능을 구현했으나
//권한문제인지 현재 PC에서는 잘 얻어오지만
//다른 PC에서는 아예 실행조차되지 않는다.
//우선 보류한다.
#if 1
int get_process_running_count(CString processname)
{
	if (processname.IsEmpty())
		return 0;

	HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pe32 = { 0, };
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnapshot, &pe32))
	{
		OutputDebugString(_T("Error while checking process"));
		CloseHandle(hProcessSnapshot);
		return 0;
	}

	int running_count = 0;

	do
	{
		//TRACE(_T("process = %s\n"), pe32.szExeFile);

		if (pe32.th32ProcessID != 0)
		{
			TCHAR sFilePath[1024] = { 0, };
			DWORD bufLen = 1024;

			_tcscpy(sFilePath, pe32.szExeFile);

			//processname이 실행파일명만 있다면 exe 파일명만 비교하고
			//전체 경로라면 fullpath를 구해서 비교한다.
			//단 hProcess가 NULL이라서 전체경로를 구하지 못하는 프로세스도 있다.
			if (PathFileExists(processname))
			{
				//HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

				if (hProcess)
				{
					//QueryFullProcessImageName(hProcess, NULL, sFilePath, &bufLen);
					::GetModuleFileNameEx(hProcess, NULL, sFilePath, MAX_PATH);
					//TRACE(_T("%s\n"), sFilePath);
					CloseHandle(hProcess);
				}
				else
				{
					TRACE(_T("fail to get OpenProcess(). %s\n"), get_error_str(false));
				}
			}

			if (_tcsicmp(sFilePath, processname) == 0)
				running_count++;
		}
	} while (Process32Next(hProcessSnapshot, &pe32));

	CloseHandle(hProcessSnapshot);

	return running_count;
} 
#else

int get_process_running_count(CString processname)
{
	DWORD dwSize = 250;
	HANDLE hSnapShot;
	PROCESSENTRY32 pEntry;
	BOOL bCrrent = FALSE;

	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);

	pEntry.dwSize = sizeof(pEntry);
	// 실행중인 프로세스들의 첫번재 정보를 가져온다.
	Process32First(hSnapShot, &pEntry);

	int procCount = 0;
	// Process 가 실행중인지 확인
	while (1)
	{
		// 다음번 프로세스의 정보를 가져온다.
		BOOL hRes = Process32Next(hSnapShot, &pEntry);

		if (hRes == FALSE)
			break;

		//권한이 없을 경우 특정 프로세스들은 OpenProcess()가 NULL을 리턴한다.
		//간단히 해당 프로세스 카운트만 얻어온다면 OpenProcess()를 호출하지 말자.
		//권한을 주고 얻어와야 한다면 AdjustTokenPrivileges 등을 이용해야 한다.
		//if (pEntry.th32ProcessID != 0)
		//{
		//	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pEntry.th32ProcessID);
		//	DbgOutA(_T("hProcess = %p, process = %s, id = %d"), hProcess, pEntry.szExeFile, pEntry.th32ProcessID);

		//}
		//else
		//{
		//}

		if (_tcscmp(pEntry.szExeFile, processname) == 0)
		{
			//CloseHandle(hSnapShot);
			//return TRUE;
			procCount++;
		}
	}

	CloseHandle(hSnapShot);
	return procCount;
}
#endif


bool KillProcess(CString szFilename)
{
	HANDLE hProcess = GetProcessHandleByName(szFilename);
	if (hProcess != INVALID_HANDLE_VALUE)
	{
		bool res = TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
		return res;
	}

	return false;
}

//프로세스 강제 종료.
//return value : 1 : killed, 0 : fail to kill, -1 : not found
bool ProcessKill(CString szProcessName)
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	HANDLE hProcess = 0; 
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof (pEntry);
	bool bRes = Process32First(hSnapShot, &pEntry);
	bool result;

	while (bRes)
	{
		if (_tcsicmp(pEntry.szExeFile, szProcessName) == 0)
		{
			hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD) pEntry.th32ProcessID);
			if (hProcess != NULL && pEntry.th32ProcessID != GetCurrentProcessId())
			{
				result = TerminateProcess(hProcess, 9);
				goto end;
			}
		}
		bRes = Process32Next(hSnapShot, &pEntry);
	}

	result = false;
end:
	CloseHandle(hProcess);
	CloseHandle(hSnapShot);

	return result;
}

static HWND shWndCurWnd;

BOOL CALLBACK cbfEnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	shWndCurWnd =0;
	DWORD PID=0, TID=0;
	TID = ::GetWindowThreadProcessId (hWnd, &PID);

	if((DWORD)lParam == PID){
		shWndCurWnd=hWnd;
		return FALSE;
	}

	return TRUE;
}

HWND GetWindowHandleFromProcessID(DWORD dwProcId)
{
	shWndCurWnd=0;

	BOOL b = EnumWindows(cbfEnumWindowsProc,  dwProcId);

	if((b==FALSE) &&(shWndCurWnd!=0)){
		return shWndCurWnd;
	}

	return 0;
}

bool IsDuplicatedRun()
{
	HANDLE hEvent;
	hEvent = CreateEvent(NULL, FALSE, TRUE, AfxGetAppName());

	if (GetLastError() == ERROR_ALREADY_EXISTS)
		return true;

	return false;
}

int find_parameter(CString target)
{
	for (int i = 1; i < __argc - 1; i++)
	{
		if (CString(__targv[i]) == target)
			return i;
	}

	return -1;
}

void Wow64Disable(bool disable)
{
#ifndef _USING_V110_SDK71_
	if (IsWow64())
	{
		//XP 64bit부터 지원되는 API이므로 32bit이면 직접 호출해줘야 한다.
		if (disable)
			Wow64DisableWow64FsRedirection(&g_wow64_preset);
		else
			Wow64RevertWow64FsRedirection(g_wow64_preset);
	}
	else
#endif
	{
		HMODULE hKernel = GetModuleHandle(_T("Kernel32"));
		typedef BOOL(WINAPI* tFSDisable)(PVOID*);
		typedef BOOL(WINAPI* tFSRevert)(PVOID);
		tFSDisable pDisableFunc = (tFSDisable)GetProcAddress(hKernel, "Wow64DisableWow64FsRedirection");
		tFSRevert pRevertFunc = (tFSRevert)GetProcAddress(hKernel, "Wow64RevertWow64FsRedirection");

		if ((pDisableFunc) && (pRevertFunc))
		{
			if (disable)
			{
				if (!pDisableFunc(&g_wow64_preset))  // Turn off the file system redirector
				{
					_tprintf(_T("\nFile System Redirection could not be turned off. Reason: %d"), GetLastError());
					return;
				}
			}
			else
			{
				if (!pRevertFunc(g_wow64_preset))  // Restore the file system redirector
				{
					_tprintf(_T("\nFile System Redirection could not be restored. Reason: %d"), GetLastError());
					return;
				}
			}
		}
	}
}

#include <array>
CString	run_process(CString cmd)
{
	void* prevSet = NULL;

	Wow64Disable(true);

	std::array<TCHAR, 10240> buffer;
	//std::string result;
	CString result;
	std::shared_ptr<FILE> pipe(_tpopen(cmd, _T("r")), _pclose);

	if (!pipe)
		throw std::runtime_error("_popen() failed!");

	while (!feof(pipe.get()))
	{
		if (_fgetts(buffer.data(), 10240, pipe.get()) != NULL)
			result += buffer.data();
	}

	Wow64Disable(false);

	return result;
}
/*
std::string	run_process(const char* cmd)
{
	void* prevSet = NULL;

	Wow64Disable(true);

	std::array<char, 10240> buffer;
	std::string result;
	std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) throw std::runtime_error("_popen() failed!");
	while (!feof(pipe.get())) {
		if (fgets(buffer.data(), 10240, pipe.get()) != NULL)
			result += buffer.data();
	}

	Wow64Disable(false);

	return result;
}
*/

CString run_process(CString exePath, bool wait_process_exit, bool return_after_first_read)
{
	Wow64Disable(true);

	TCHAR cmd[10240] = { 0, };
	CString result(_T(""));
	STARTUPINFO si{ sizeof(si) };
	PROCESS_INFORMATION pi{};
	bool is_gui_app = is_gui_application(exePath);

	_stprintf(cmd, _T("%s"), exePath);

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	HANDLE hChildStdoutRd;
	HANDLE hChildStdoutWr;

	// Create security attributes to create pipe.
	SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe to get results from child's stdout.
	if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
	{
		Wow64Disable(false);
		return result;
	}

	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.hStdOutput = hChildStdoutWr;
	si.hStdError = hChildStdoutWr;
	si.wShowWindow = (is_gui_app ? SW_SHOWNORMAL : SW_HIDE);

	//dir과 같은 DOS 명령은 외부 실행파일이 아니므로 exePath를 "dir c:\\*.*"과 같이 주면 에러가 발생한다.
	//"cmd.exe /c dir c:\\*.*"와 같이 전달해야 한다.
	//하지만 run_process()를 호출하는 사용자 입장에서는 "dir c:\\*.*"과 같이 호출할 것이므로
	//exePath를 가공해준다.
	if (!is_gui_app)
	{
		//exePath = _T("/c ") + exePath;
	}

	// Start the child process. 
	if (!CreateProcess(NULL,
		(TCHAR*)(const TCHAR*)(exePath),//_T("dir")과 같이 상수값을 직접 주면 실패.	// Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		TRUE,          // Set handle inheritance to FALSE
		CREATE_NEW_CONSOLE,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)

	//if (!CreateProcess(NULL,
	//	(TCHAR*)(const TCHAR*)(exePath),//_T("dir")과 같이 상수값을 직접 주면 실패.	// Command line
	//	NULL,           // Process handle not inheritable
	//	NULL,           // Thread handle not inheritable
	//	TRUE,          // Set handle inheritance to FALSE
	//	CREATE_NEW_CONSOLE,              // No creation flags
	//	NULL,           // Use parent's environment block
	//	NULL,           // Use parent's starting directory 
	//	&si,            // Pointer to STARTUPINFO structure
	//	&pi)           // Pointer to PROCESS_INFORMATION structure
	//	)
	{
		TRACE(_T("error = %s\n"), get_error_str(GetLastError()));
		Wow64Disable(false);
		return result;
	}

	if (wait_process_exit)
	{
		// Wait until child process exits.
		WaitForSingleObject(pi.hProcess, INFINITE);
	}
	else
	{
		DWORD dwWait = ::WaitForInputIdle(pi.hProcess, 1000);// INFINITE);
	}

	if (!CloseHandle(hChildStdoutWr))
	{
		Wow64Disable(false);
		return result;
	}

	//외부 실행파일일 경우는 result string이 없으므로 이 코드를 실행하면 안된다.
	if (!is_gui_app)
	{
		for (;;)
		{
			DWORD dwRead;
			char chBuf[10240] = { 0, };

			bool done = !ReadFile(hChildStdoutRd, chBuf, 10240, &dwRead, NULL) || dwRead == 0;

			if (done)
				break;

			result += chBuf;
			if (return_after_first_read)
				break;
		}
	}

	// Close process and thread handles. 
	CloseHandle(hChildStdoutRd);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	Wow64Disable(false);

	result.Replace(_T("\r\n"), _T("\n"));

	return result;
}

//서비스 관련 명령을 쉽게 처리하기 위해 작성.
//cmd는 다음과 같은 키워드를 사용하며 명령에 따라 리턴값의 의미도 다르므로 주의할 것.
//"query"	: status를 리턴
//"stop"	: 서비스를 중지시키고 최종 status = "SERVICE_STOPPED"를 리턴, 그렇지 않으면 detail 참조.
//			: 서비스가 존재하지 않거나 이미 중지된 경우에도 "SERVICE_STOPPED"를 리턴함.
//"delete"	: 서비스 삭제가 성공하면 0이 아닌 값을 리턴. 실패하면 0을 리턴하므로 이 경우는 detail 참조.
DWORD service_command(CString service_name, CString cmd, CString *detail)
{
	DWORD res = 0;
	SERVICE_STATUS status;
	ZeroMemory(&status, sizeof(status));

	cmd.MakeLower();

	//service control manager를 얻어와서 서비스 상태값을 조회.
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL; 

	if ((hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) == NULL)
	{
		res = GetLastError();
		TRACE(_T("get_error_str = %s\n"), get_error_str(res));
		return res;
	}

	if ((hService = OpenService(hManager, service_name, SC_MANAGER_ALL_ACCESS)) == NULL)
	{
		res = GetLastError();
		switch (res)
		{
			case ERROR_ACCESS_DENIED:
				if (detail)
					*detail = _T("ERROR_ACCESS_DENIED");
				break;
			case ERROR_INVALID_HANDLE:
				if (detail)
					*detail = _T("ERROR_INVALID_HANDLE");
				break;
			case ERROR_INVALID_NAME:
				if (detail)
					*detail = _T("ERROR_INVALID_NAME");
				break;
			case ERROR_SERVICE_DOES_NOT_EXIST:
				if (detail)
					*detail = _T("ERROR_SERVICE_DOES_NOT_EXIST");
				break;
			default :
				*detail = _T("not defined error.");
		}

		TRACE(_T("detail = %s\n"), (detail ? *detail : _T("")));
		CloseServiceHandle(hManager);
		return res;
	}

	//서비스에게 INTERROGATE 제어신호를 보내면 해당 서비스는 자신의 현재 상태를 리턴한다.
	//SCM이 서비스의 상태를 조사하는 것이 아니라 서비스 자신이 스스로의 상태를 보고하는 것이므로
	//QueryServiceStatus 함수를 사용하는 것보다 훨씬 더 정확하다.
	//https://blog.naver.com/wwwkasa/80150694337
	//하지만 MSDN에는 해당 항목에 "SCM이 서비스의 현재 상태를 인식하기 때문에 이 컨트롤은 일반적으로 유용하지 않습니다."라는
	//설명이 있으므로 

	if (QueryServiceStatus(hService, &status))
		//if (ControlService(hService, SERVICE_CONTROL_INTERROGATE, &status))	//error = ERROR_ACCESS_DENIED
	{
		if (cmd == _T("query"))
			res = status.dwCurrentState;
	}
	else
	{
		res = GetLastError();
		TRACE(_T("get_error_str = %s\n"), get_error_str(res));
	}

	//delete할 경우는 반드시 stop후에 delete해야 함.
	if (cmd == _T("stop") || cmd == _T("delete"))
	{
		if (status.dwCurrentState != SERVICE_STOPPED)
		{
			if (ControlService(hService, SERVICE_CONTROL_STOP, &status))
			{
				//SERVICE_CONTROL_STOP 명령을 내린 후 SERVICE_STOP_PENDING (3) 상태가 된 후 SERVICE_STOPPED 상태로 변경되므로
				//
				while (status.dwCurrentState != SERVICE_STOPPED)
					QueryServiceStatus(hService, &status);
				res = SERVICE_STOPPED;
			}
			else
			{
				res = GetLastError();
				TRACE(_T("get_error_str = %s\n"), get_error_str(res));
			}
		}

		if (cmd == _T("delete"))
		{
			res = DeleteService(hService);
			if (res == 0)
				res = GetLastError();
		}
	}

	//5		: ERROR_ACCESS_DENIED
	//6		: ERROR_INVALID_HANDLE
	//87	: ERROR_INVALID_PARAMETER
	//1051	: ERROR_DEPENDENT_SERVICES_RUNNING
	//1052	: ERROR_INVALID_SERVICE_CONTROL
	//1053	: ERROR_SERVICE_REQUEST_TIMEOUT
	//1061	: ERROR_SERVICE_CANNOT_ACCEPT_CTRL
	//1062	: ERROR_SERVICE_NOT_ACTIVE
	//1115	: ERROR_SHUTDOWN_IN_PROGRESS

	CloseServiceHandle(hService);
	CloseServiceHandle(hManager);

	return res;
}

/*
//서비스 상태가 무엇이든 종료, 제거시킨다. sc queryex -> taskkill /pid -> sc delete
//process_name이 주어지면 좀 더 간단히 제거된다.
//정상 제거(또는 서비스가 없을 경우) : true
//제거 실패 : false
bool kill_service(CString service_name, CString process_name)
{
	CString result;

	if (!process_name.IsEmpty())
	{
		result = run_process(_T("taskkill /f /im ") + process_name, true);
		result = run_process(_T("sc delete ") + service_name, true);
		return true;
	}

	result = run_process(_T("sc queryex ") + service_name, true);
	int pid_pos = result.Find(_T("PID"));

	//해당 서비스가 없을 경우
	if (pid_pos <= 0)
		return true;

	//PID 이후의 문자열로 만들고
	result = result.Mid(pid_pos + 3);
	//':' 이후의 문자열로 만들고
	result = result.Mid(result.Find(':') + 1);
	result.Trim();

	result = result.Left(result.Find('\r'));

	int pid = _ttoi(result);
	CString cmd;

	//해당 서비스의 실제 프로세스를 강제 종료시키면 해당 서비스는 "중지됨"으로 변경된다.
	//cmd.Format(_T("taskkill /pid %d /f"), pid);
	//result = run_process(cmd, true).MakeLower();
	//if (result.Find(_T("성공")) < 0 && result.Find(_T("success")) < 0)
	//	return false;

	//위에서 서비스의 실제 프로세스를 강제 종료시켜봤으나 결과는 성공이라고 나오지만 실제로는 종료되지 않거나
	//종료되더라도 다시 살아나는 현상이 발생하여 sc stop으로 변경
	result = run_process(_T("sc stop ") + service_name, true);
	if (result.Find(_T("STOP_PENDING")) < 0)
		return false;


	//중지된 서비스를 삭제한다. 
	result = run_process(_T("sc delete ") + service_name, true);
	if (result.Find(_T("성공")) < 0 && result.Find(_T("success")) < 0)
		return false;

	return true;
}
*/

bool RectInRect(CRect rMain, CRect rSub)
{
	CRect	rUnion;

	rUnion.UnionRect(&rMain, &rSub);

	if (rUnion == rMain)
		return true;

	return false;
}

//r에서 except영역을 제외하고 cr컬러로 채운다.
void fill_except_rect(CDC* pDC, CRect r, CRect except, COLORREF cr)
{
	//채워지는 영역 확인용
	bool confirm = false;

	//left area
	pDC->FillSolidRect(r.left, r.top, except.left - r.left, r.Height(), (confirm ? red : cr));
	//right area
	pDC->FillSolidRect(except.left + except.Width(), r.top, r.right - except.left - except.Width(), r.Height(), (confirm ? green : cr));
	//top area
	pDC->FillSolidRect(except.left, r.top, except.Width(), except.top - r.top, (confirm ? blue : cr));
	//bottom area
	pDC->FillSolidRect(except.left, except.top + except.Height(), except.Width(), r.bottom - except.top - except.Height(), (confirm ? violet : cr));
}

//side 배열의 인덱스는 resize하는 영역 인덱스로서
//DefWindowProc의 두번째 파라미터에 (SC_SIZE + m_nSideIndex)로 쓰이므로 그 차례를 따른다.
//단, top의 경우, 테두리가 없는 dialog인 경우는 상단을 타이틀바 영역처럼 쓰므로
//margin의 크기를 그대로 잡으면 타이틀바가 좁아지는 불편이 있다.
//따라서 top의 경우는 4픽셀로 고정하는 것이 적절하다.
void GetSideRect(CRect src, CRect *side, int margin)
{
	side[corner_left] = CRect(src.left, src.top + margin, src.left + margin, src.bottom - margin);
	side[corner_right] = CRect(src.right - margin, src.top + margin, src.right, src.bottom - margin);
	side[corner_top] = CRect(src.left + margin, src.top, src.right - margin, src.top + 4);
	side[corner_topleft] = CRect(src.left, src.top, src.left + margin, src.top + margin);
	side[corner_topright] = CRect(src.right - margin, src.top, src.right, src.top + margin);
	side[corner_bottom] = CRect(src.left + margin, src.bottom - margin, src.right - margin, src.bottom);
	side[corner_bottomleft] = CRect(src.left, src.bottom - margin, src.left + margin, src.bottom);
	side[corner_bottomright] = CRect(src.right - margin, src.bottom - margin, src.right, src.bottom);
}

//8군데의 코너와 사이드 영역에 해당 점이 있는지를 검사하여 해당 영역의 인덱스를 리턴한다.
//인덱스는 CORNER_INDEX의 차례이며 이는 DefWindowProc에서 사용하는 차례와 동일하다.
int get_corner_index(CRect src, CPoint pt, int margin)
{
	if ((src.IsRectEmpty()) ||
		(margin < 0) ||
		(margin > src.Width() / 2))
		return -1;

	CRect r[9];

	GetSideRect(src, r, margin);

	if (r[corner_topleft].PtInRect(pt))
		return corner_topleft;
	else if (r[corner_topright].PtInRect(pt))
		return corner_topright;
	else if (r[corner_bottomleft].PtInRect(pt))
		return corner_bottomleft;
	else if (r[corner_bottomright].PtInRect(pt))
		return corner_bottomright;
	else if (r[corner_left].PtInRect(pt))
		return corner_left;
	else if (r[corner_top].PtInRect(pt))
		return corner_top;
	else if (r[corner_right].PtInRect(pt))
		return corner_right;
	else if (r[corner_bottom].PtInRect(pt))
		return corner_bottom;
	else if (src.PtInRect(pt))
		return corner_inside;

	return -1;
}

//시작점을 주면 정사각형을 이루는 끝점 좌표를 리턴한다.
void getSquareEndPoint(int sx, int sy, int& ex, int& ey)
{
	int difX = ex - sx;
	int difY = ey - sy;

	if (abs(difX) > abs(difY))
	{
		if (difY > 0)
			ey = sy + abs(difX);
		else
			ey = sy - abs(difX);
					}
	else
	{
		if (difX > 0)
			ex = sx + abs(difY);
		else
			ex = sx - abs(difY);
	}
}

//src 사각형의 크기조정 및 이동을 위한 9개의 사각형 값을 리턴한다. sz는 핸들 크기 한 변의 길이가 아닌 1/2을 의미한다.
void get_resizable_handle(CRect src, CRect handle[], int sz)
{
	//right, bottom을 -1씩 줄여줘야 정확하다.
	src.DeflateRect(0, 0, 1, 1);
	// 
	//순번은 CORNER_INDEX와 일관되게 처리한다.
	handle[corner_inside]		= CRect(src.CenterPoint().x - sz, src.CenterPoint().y - sz, src.CenterPoint().x + sz, src.CenterPoint().y + sz);
	handle[corner_left]			= CRect(src.left - sz, src.CenterPoint().y - sz, src.left + sz, src.CenterPoint().y + sz);
	handle[corner_right]		= CRect(src.right - sz, src.CenterPoint().y - sz, src.right + sz, src.CenterPoint().y + sz);
	handle[corner_top]			= CRect(src.CenterPoint().x - sz, src.top - sz, src.CenterPoint().x + sz, src.top + sz);
	handle[corner_topleft]		= CRect(src.left - sz, src.top - sz, src.left + sz, src.top + sz);
	handle[corner_topright]		= CRect(src.right - sz, src.top - sz, src.right + sz, src.top + sz);
	handle[corner_bottom]		= CRect(src.CenterPoint().x - sz, src.bottom - sz, src.CenterPoint().x + sz, src.bottom + sz);
	handle[corner_bottomleft]	= CRect(src.left - sz, src.bottom - sz, src.left + sz, src.bottom + sz);
	handle[corner_bottomright]	= CRect(src.right - sz, src.bottom - sz, src.right + sz, src.bottom + sz);
}

//src 사각형의 크기조정 및 이동을 위한 9개의 사각형 중 pt가 위치한 사각형의 인덱스를 리턴한다.
//인덱스 정의는 enum CORNER_INDEX 정의를 공통으로 사용한다.
//int	get_handle_index(CRect src, CPoint pt, int sz)
//{
//
//}

BOOL IsWow64()
{
	BOOL bIsWow64 = FALSE;
	typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			// handle error
		}
	}
	return bIsWow64;
}

BOOL Is64BitWindows()
{
#if defined(_WIN64)
	return TRUE;  // 64-bit programs run only on Win64
#elif defined(_WIN32)
	// 32-bit programs run on both 32-bit and 64-bit Windows
	// so must sniff
	BOOL f64 = FALSE;
	return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#else
	return FALSE; // Win64 does not support Win16
#endif
}

BOOL IsXpOr2000()
{
	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof(ovi);
	GetVersionEx(&ovi);

	if (ovi.dwMajorVersion == 5)
	{
		// 0 : 2000, 1 : xp
		if (ovi.dwMinorVersion == 0 || ovi.dwMinorVersion == 1)
		{
			return TRUE;
		}
	}
	return FALSE;
}

int	GetTrimLength(CString str)
{
	str.Trim();

	return str.GetLength();
}

CString	ExtractSubString(CString src, CString sPrev, CString sPost)
{
	int nPrevPos = src.Find(sPrev);
	int nPostPos = src.Find(sPost);

	if ((nPrevPos > sPrev.GetLength() - 1) && (nPostPos > nPrevPos))
		return src.Mid(nPrevPos + sPrev.GetLength(), nPostPos - nPrevPos - sPrev.GetLength());

	return _T("");
}

//-----------------------------------------------------------------
// PID로 프로세스 찾기
//-----------------------------------------------------------------
bool CheckProcessUsingPID(unsigned long pid) // unsigned long = DWORD
{
	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	if (!snapshotHandle)
		return false;

	//-----------------------------------------------------------------
	// 프로세스를 찾는다
	//-----------------------------------------------------------------
	Process32First(snapshotHandle, &processEntry);
	do
	{
		if (processEntry.th32ProcessID == pid)
			return true;
	} while (Process32Next(snapshotHandle, &processEntry));

	return false;
}

//-----------------------------------------------------------------
// 윈도우 타이틀, 클래스 이름으로 프로세스 찾기
//-----------------------------------------------------------------
bool CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName)
{
	if (!className && !titleName)
		return false;

	//-----------------------------------------------------------------
	// 클래스 이름 및 타이틀 이름으로 윈도우를 찾는다
	//-----------------------------------------------------------------
	HWND targetWindow = FindWindow(className ? className : NULL, titleName ? titleName : NULL);
	if (targetWindow)
	{
		//-----------------------------------------------------------------
		// 찾은 윈도우 핸들로 PID, TID를 취득하여 프로세스를 찾는다
		//-----------------------------------------------------------------
		unsigned long pid = 0, tid = GetWindowThreadProcessId(targetWindow, &pid);

		PROCESSENTRY32 processEntry;
		processEntry.dwSize = sizeof(PROCESSENTRY32);

		HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
		if (!snapshotHandle)
			return false;

		Process32First(snapshotHandle, &processEntry);
		do
		{
			if (processEntry.th32ProcessID == pid)
				return true;
		} while (Process32Next(snapshotHandle, &processEntry));

		return false;
	}

	return false;
}

//-----------------------------------------------------------------
// 프로세스 이름으로 프로세스 찾기
//-----------------------------------------------------------------
bool CheckProcessUsingProcessName(LPCTSTR processName) // unsigned long = DWORD
{
	if (!processName)
		return false;

	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	if (!snapshotHandle)
		return false;

	//-----------------------------------------------------------------
	// 프로세스를 찾는다
	//-----------------------------------------------------------------
	Process32First(snapshotHandle, &processEntry);
	do
	{
		TRACE(_T("processEntry.szExeFile = %s\n"), processEntry.szExeFile);
		if (!_tcscmp(processName, processEntry.szExeFile))
			return true;
	} while (Process32Next(snapshotHandle, &processEntry));

	return false;
}


CImage* capture_window(CRect r, CString filename)
{
	HDC hDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	CImage *imgCapture = new ATL::CImage();
	imgCapture->Create(r.Width(), r.Height(), GetDeviceCaps(hDC, BITSPIXEL), 0);

	::BitBlt(imgCapture->GetDC(), 0, 0, r.Width(), r.Height(), hDC, r.left, r.top, SRCCOPY);
	/*
	CTime t = CTime::GetCurrentTime();
	CString tName;
	tName.Format(_T("%04d-%02d-%02d_%02d%02d%02d"),
		t.GetYear(), t.GetMonth(), t.GetDay(),
		t.GetHour(), t.GetMinute(), t.GetSecond());
	*/
	CString ext = get_part(filename, fn_ext).MakeLower();
	GUID format = Gdiplus::ImageFormatJPEG;

	if (ext == _T("bmp"))
		format = Gdiplus::ImageFormatBMP;
	else if (ext == _T("png"))
		format = Gdiplus::ImageFormatPNG;
	else
		//jpeg

	imgCapture->Save(filename, format);
	::DeleteDC(hDC);

	return imgCapture;
}

void draw_bitmap(HDC hdc, int x, int y, HBITMAP hBitmap)
{
	HDC MemDC;
	HBITMAP OldBitMap;
	int bx, by;
	BITMAP bitmap;

	MemDC = CreateCompatibleDC(hdc);
	OldBitMap = (HBITMAP)SelectObject(MemDC, hBitmap);

	GetObject(hBitmap, sizeof(BITMAP), &bitmap);
	bx = bitmap.bmWidth;
	by = bitmap.bmHeight;

	BitBlt(hdc, x, y, bx, by, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitMap);
	DeleteDC(MemDC);
}

// Capture screen and create GDI bitmap
// (full-screen when pRect is NULL)
HBITMAP capture_screen_to_bitmap(LPRECT pRect, UINT id, int dx, int dy, bool show_cursor)
{
	HDC         hScrDC, hMemDC;         // screen DC and memory DC
	HBITMAP     hBitmap, hOldBitmap;    // handles to deice-dependent bitmaps
	int         nX, nY, nX2, nY2;       // coordinates of rectangle to grab
	int         nWidth, nHeight;        // DIB width and height
	int         xScrn, yScrn;           // screen resolution

	// check for an empty rectangle
	//if (IsRectEmpty(pRect)) return NULL;

	// create a DC for the screen and create
	// a memory DC compatible to screen DC
	hScrDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	hMemDC = CreateCompatibleDC(hScrDC);

	// get screen resolution
	xScrn = GetDeviceCaps(hScrDC, HORZRES);
	yScrn = GetDeviceCaps(hScrDC, VERTRES);

	// get points of rectangle to grab
	if (pRect)
	{
		nX = pRect->left;
		nY = pRect->top;
		nX2 = pRect->right;
		nY2 = pRect->bottom;

		//make sure bitmap rectangle is visible
		//scpark 모니터 위치 설정에 따라 음수일 수 있다. 아래 보정은 하지 않아야 한다.
		//if (nX < 0) nX = 0;
		//if (nY < 0) nY = 0;
	}
	else
	{
		nX = 0;
		nY = 0;
		nX2 = xScrn;
		nY2 = yScrn;
	}

	nWidth = nX2 - nX;
	nHeight = nY2 - nY;

#if 0
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = nWidth;
	bmi.bmiHeader.biHeight = -nHeight; // Negative height to make it top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32; // 24-bit bitmap (RGB)
	bmi.bmiHeader.biCompression = BI_RGB;
	// create a bitmap compatible with the screen DC
	void* bits;
	hBitmap = CreateDIBSection(hScrDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
#else

	// create a bitmap compatible with the screen DC
	hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);	//32bit로 만들어진다.
#endif

	// select new bitmap into memory DC
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

	// bitblt screen DC to memory DC
	BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY);

	if (id > 0)
	{
		//HBITMAP hbmp = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(id));
		HBITMAP hbmp = (HBITMAP)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		draw_bitmap(hMemDC, dx, dy, hbmp);
	}

	//캡처화면에 커서를 포함시킬지
	if (show_cursor)
	{
		CURSORINFO cursor = { sizeof(cursor) };
		GetCursorInfo(&cursor);
		ICONINFO info = { sizeof(info) };
		GetIconInfo(cursor.hCursor, &info);
		const int x = cursor.ptScreenPos.x - pRect->left - pRect->left - info.xHotspot;
		const int y = cursor.ptScreenPos.y - pRect->top - pRect->top - info.yHotspot;
		BITMAP bmpCursor = { 0 };
		GetObject(info.hbmColor, sizeof(bmpCursor), &bmpCursor);
		DrawIconEx(hMemDC, x, y, cursor.hCursor, bmpCursor.bmWidth, bmpCursor.bmHeight,
			0, NULL, DI_NORMAL);
	}


	// select old bitmap back into memory DC and get handle to
	// bitmap of the screen
	hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);

	// clean up
	DeleteDC(hScrDC);
	DeleteDC(hMemDC);

	// return handle to the bitmap
	return hBitmap;
}

// Capture window area and create GDI bitmap
HBITMAP CaptureWindowToBitmap(HWND hWnd, LPRECT pRect /*= NULL*/)
{
	HBITMAP hBitmap = NULL;  // handle to device-dependent bitmap
	RECT rectWnd;

	// check for a valid window handle
	if (!hWnd) return NULL;

	// get the window rectangle
	GetWindowRect(hWnd, &rectWnd);

	if (pRect != NULL)
	{
		rectWnd.left	+= pRect->left;
		rectWnd.top		+= pRect->top;
		rectWnd.right	= rectWnd.left + pRect->right - pRect->left;
		rectWnd.bottom	= rectWnd.top + pRect->bottom - pRect->top;
	}


	// get the bitmap of that window by calling
	// CopyScreenToBitmap and passing it the window rect
	hBitmap = capture_screen_to_bitmap(&rectWnd);

	// return handle to the bitmap
	return hBitmap;
}

// Capture window client area and create GDI bitmap
HBITMAP CaptureClientToBitmap(HWND hWnd, LPRECT pRect /*= NULL*/)
{
	HBITMAP hBitmap = NULL;  // handle to device-dependent bitmap
	RECT rectClient;
	POINT pt1, pt2;

	// check for a valid window handle
	if (!hWnd) return NULL;

	// get client dimensions
	GetClientRect(hWnd, &rectClient);

	// convert client coords to screen coords
	pt1.x = rectClient.left;
	pt1.y = rectClient.top;
	pt2.x = rectClient.right;
	pt2.y = rectClient.bottom;
	ClientToScreen(hWnd, &pt1);
	ClientToScreen(hWnd, &pt2);
	rectClient.left = pt1.x;
	rectClient.top = pt1.y;
	rectClient.right = pt2.x;
	rectClient.bottom = pt2.y;

	// get the bitmap of the client area by calling
	// CopyScreenToBitmap and passing it the client rect
	hBitmap = capture_screen_to_bitmap(&rectClient);

	// return handle to the bitmap
	return hBitmap;
}

HBITMAP	PrintWindowToBitmap(HWND hTargetWnd, LPRECT pRect)
{
	HDC hDC = ::GetDC(hTargetWnd);
	//HDC hdcBitmap = ::CreateCompatibleDC(hDC);

	CRect rct, DsRct;
	if (hTargetWnd)
	{
		if (pRect)
		{
			rct = CRect(pRect);
		}
		else
		{
			//::GetWindowRect(hTargetWnd, &rct);
			::GetWindowRect(::GetDesktopWindow(), &DsRct);
		}
	}
	else
	{
		return NULL;
	}

	int nw = rct.Width();
	nw = MAKE_MULTIPLY_U(nw, 4);
	rct.right = rct.left + nw;

	HBITMAP hBitmap = NULL;
	BOOL bSuccess = FALSE;
	HDC hMemDC = ::CreateCompatibleDC(hDC);

#if 0
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = rct.Width();
	bmi.bmiHeader.biHeight = -(rct.Height()); // Negative height to make it top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32; // 24-bit bitmap (RGB)
	bmi.bmiHeader.biCompression = BI_RGB;
	// create a bitmap compatible with the screen DC
	void* bits;
	hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
#else
	// create a bitmap compatible with the screen DC
	hBitmap = ::CreateCompatibleBitmap(hDC, rct.Width(), rct.Height());
#endif


	if (!hBitmap)
	{
		::DeleteDC(hDC);
		::DeleteDC(hMemDC);
		return NULL;
	}

	::SelectObject(hMemDC, hBitmap);

	if (!::PrintWindow(hTargetWnd, hMemDC, 2))
	{
		bSuccess = FALSE;
	}
	else
	{
		//전체화면일때와 창모드일때 불필요한 여백을 잘라내는 코드들이나
		//여백을 잘라내면 해당 크기로 mpeg녹화되지 않는 에러가 발생하여
		//우선 잘라내는 코드는 사용하지 않는다.
		if (true)//(rct.left + 8 == DsRct.left || rct.left + 8 == DsRct.left + DsRct.right) && (rct.top + 8 == DsRct.top || rct.top + 8 == DsRct.top + DsRct.bottom) && (rct.right - 8 == DsRct.right || rct.right - 8 == DsRct.right * 2) && (rct.bottom + 32 == DsRct.bottom || rct.bottom + 32 == DsRct.bottom * 2))
		{
			// 전체화면

			//StretchBlt(hMemDC, -8, -8, rct.right - rct.left + 8, rct.bottom - rct.top + 8, hMemDC, 0, 0, rct.right - rct.left - 8, rct.bottom - rct.top - 8, SRCCOPY);
			StretchBlt(hMemDC, 0, 0, rct.Width(), rct.Height(), hMemDC, 0, 0, rct.Width(), rct.Height(), SRCCOPY);
			// 캡쳐 시 여백을 없애기 위해 8픽셀을 가감
		}
		else
		{
			// 창모드

			StretchBlt(hMemDC, -7, 0, rct.right - rct.left + 7, rct.bottom - rct.top, hMemDC, 0, 0, rct.right - rct.left - 7, rct.bottom - rct.top - 7, SRCCOPY);
			// 캡쳐 시 여백을 없애기 위해 7픽셀을 가감
		}
		bSuccess = TRUE;
	}

	::DeleteDC(hDC);
	::DeleteDC(hMemDC);

	//WriteBMP(hBitmap, hMemDC, _T("d:\\temp\\test_capture.bmp"));
	return hBitmap;
}

#define _M(exp) (([](HRESULT hr) { if (FAILED(hr)) /*_com_raise_error(hr);*/ return hr; })(exp));
void save_bitmap(HBITMAP bitmap, LPCTSTR filename)
{
	PICTDESC pictdesc = {};
	pictdesc.cbSizeofstruct = sizeof(pictdesc);
	pictdesc.picType = PICTYPE_BITMAP;
	pictdesc.bmp.hbitmap = bitmap;

	CComPtr<IPicture> picture;
	_M(OleCreatePictureIndirect(&pictdesc, __uuidof(IPicture), FALSE, (LPVOID*)&picture));

	// Save to a stream

	CComPtr<IStream> stream;
	_M(CreateStreamOnHGlobal(NULL, TRUE, &stream));
	LONG cbSize = 0;
	_M(picture->SaveAsFile(stream, TRUE, &cbSize));

	// Or save to a file

	CComPtr<IPictureDisp> disp;
	_M(picture->QueryInterface(&disp));
	_M(OleSavePictureFile(disp, CComBSTR(filename)));
	return;

	HDC hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
	BITMAP bmp; 
	PBITMAPINFO pbmi; 
	WORD cClrBits; 
	HANDLE hf; // file handle 
	BITMAPFILEHEADER hdr; // bitmap file-header 
	PBITMAPINFOHEADER pbih; // bitmap info-header 
	LPBYTE lpBits; // memory pointer 
	DWORD dwTotal; // total count of bytes 
	DWORD cb; // incremental count of bytes 
	BYTE *hp; // byte pointer 
	DWORD dwTmp; 

	// create the bitmapinfo header information
	int ret = GetObject(bitmap, sizeof(BITMAP), (LPSTR)&bmp);
	if (ret == 0)
	{
		AfxMessageBox(_T("Could not retrieve bitmap info"));
		return;
	}

	// Convert the color format to a count of bits. 
	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
	if (cClrBits == 1) 
		cClrBits = 1; 
	else if (cClrBits <= 4) 
		cClrBits = 4; 
	else if (cClrBits <= 8) 
		cClrBits = 8; 
	else if (cClrBits <= 16) 
		cClrBits = 16; 
	else if (cClrBits <= 24) 
		cClrBits = 24; 
	else cClrBits = 32; 
	// Allocate memory for the BITMAPINFO structure.
	if (cClrBits != 24) 
		pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
			sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (static_cast<unsigned long long>(1) << cClrBits));
	else 
		pbmi = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER)); 

	// Initialize the fields in the BITMAPINFO structure. 

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	pbmi->bmiHeader.biWidth = bmp.bmWidth; 
	pbmi->bmiHeader.biHeight = bmp.bmHeight; 
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
	if (cClrBits < 24) 
		pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

	// If the bitmap is not compressed, set the BI_RGB flag. 
	pbmi->bmiHeader.biCompression = BI_RGB; 

	// Compute the number of bytes in the array of color 
	// indices and store the result in biSizeImage. 
	pbmi->bmiHeader.biSizeImage = (pbmi->bmiHeader.biWidth + 7) /8 * pbmi->bmiHeader.biHeight * cClrBits; 
	// Set biClrImportant to 0, indicating that all of the 
	// device colors are important. 
	pbmi->bmiHeader.biClrImportant = 0; 

	// now open file and save the data
	pbih = (PBITMAPINFOHEADER) pbmi; 
	lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

	if (!lpBits) {
		AfxMessageBox(_T("writeBMP::Could not allocate memory"));
		return;
	}

	// Retrieve the color table (RGBQUAD array) and the bits 
	if (!GetDIBits(hDC, HBITMAP(bitmap), 0, (WORD) pbih->biHeight, lpBits, pbmi, 
		DIB_RGB_COLORS)) {
			AfxMessageBox(_T("writeBMP::GetDIB error"));
			return;
	}

	// Create the .BMP file. 
	hf = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, (DWORD) 0, 
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 
		(HANDLE) NULL); 
	if (hf == INVALID_HANDLE_VALUE){
		AfxMessageBox(_T("Could not create file for writing"));
		return;
	}
	hdr.bfType = 0x4d42; // 0x42 = "B" 0x4d = "M" 
	// Compute the size of the entire file. 
	hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
		pbih->biSize + pbih->biClrUsed 
		* sizeof(RGBQUAD) + pbih->biSizeImage); 
	hdr.bfReserved1 = 0; 
	hdr.bfReserved2 = 0; 

	// Compute the offset to the array of color indices. 
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
		pbih->biSize + pbih->biClrUsed 
		* sizeof (RGBQUAD); 

	// Copy the BITMAPFILEHEADER into the .BMP file. 
	if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
		(LPDWORD) &dwTmp, NULL)) {
			AfxMessageBox(_T("Could not write in to file"));
			return;
	}

	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
	if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
		+ pbih->biClrUsed * sizeof (RGBQUAD), 
		(LPDWORD) &dwTmp, (NULL))){
			AfxMessageBox(_T("Could not write in to file"));
			return;
	}


	// Copy the array of color indices into the .BMP file. 
	dwTotal = cb = pbih->biSizeImage; 
	hp = lpBits; 
	if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL)){
		AfxMessageBox(_T("Could not write in to file"));
		return;
	}

	// Close the .BMP file. 
	if (!CloseHandle(hf)){
		AfxMessageBox(_T("Could not close file"));
		return;
	}

	// Free memory. 
	GlobalFree((HGLOBAL)lpBits);
}

bool GetLockKeyState(uint8_t nLockKey)
{
	BYTE  keyState[256];

	GetKeyboardState((LPBYTE)&keyState);
	return (bool)keyState[nLockKey];
}

void SetLockKeyState(BYTE nLockKey, bool bOn)
{
	BYTE  keyState[256];

	GetKeyboardState((LPBYTE)&keyState);

	if((bOn && !(keyState[nLockKey] & 1)) ||
		(!bOn && (keyState[nLockKey] & 1)))
	{
		// Simulate a key press
		keybd_event(nLockKey,
					0x45,
					KEYEVENTF_EXTENDEDKEY | 0,
					0);

		// Simulate a key release
		keybd_event(nLockKey,
					0x45,
					KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
					0);
	}

/*	//toggle using SendInput
	INPUT input[2];
	::ZeroMemory(input, sizeof(input));        
	input[0].type = input[1].type = INPUT_KEYBOARD;
	input[0].ki.wVk  = input[1].ki.wVk = VK_CAPITAL;        
	input[1].ki.dwFlags = KEYEVENTF_KEYUP;  // THIS IS IMPORTANT
	::SendInput(2, input, sizeof(INPUT));
*/
}

void RGB2HSL(int R, int G, int B, int& h, int& s, int& l)
{
	double r = R/255.0;
	double g = G/255.0;
	double b = B/255.0;
	double v;
	double m;
	double vm;
	double r2, g2, b2;
	double H, S, L;
	H = 0; // default to black
	S = 0;
	L = 0;

	v = std::max(r,g);
	v = std::max(v,b);
	m = std::min(r,g);
	m = std::min(m,b);

	L = (m + v) / 2.0;

	if (L <= 0.0) 
	{
		return;
	}

	vm = v - m;
	S = vm;

	if (S > 0.0) 
		S /= (L <= 0.5) ? (v + m) : (2.0 - v - m) ;
	else 
		return;

	r2 = (v - r) / vm;
	g2 = (v - g) / vm;
	b2 = (v - b) / vm;

	if (r == v) 
		H = (g == m ? 5.0 + b2 : 1.0 - g2);
	else if (g == v) 
		H = (b == m ? 1.0 + r2 : 3.0 - b2);
	else 
		H = (r == m ? 3.0 + g2 : 5.0 - r2);

	H /= 6.0;

	h = H * 255.0;
	s = S * 255.0;
	l = L * 255.0;

/*
	double var_R = (r / 255);                     //RGB from 0 to 255
	double var_G = (g / 255);
	double var_B = (b / 255);
	double H, S, L;

	double var_Min = min(min(var_R, var_G), var_B);    //Min. value of RGB
	double var_Max = max(max(var_R, var_G), var_B);    //Max. value of RGB
	double del_Max = var_Max - var_Min;             //Delta RGB value

	double del_R;
	double del_G;
	double del_B;

	L = (var_Max + var_Min) / 2;

	if (del_Max == 0)                     //This is a gray, no chroma...
	{
		H = 0;                                //HSL results from 0 to 1
		S = 0;
	}
	else                                    //Chromatic data...
	{
		if (L < 0.5)
			S = del_Max / (var_Max + var_Min);
		else
			S = del_Max / (2 - var_Max - var_Min);

		del_R = (((var_Max - var_R) / 6) + (del_Max / 2)) / del_Max;
		del_G = (((var_Max - var_G) / 6) + (del_Max / 2)) / del_Max;
		del_B = (((var_Max - var_B) / 6) + (del_Max / 2)) / del_Max;

		if      (var_R == var_Max) H = del_B - del_G;
		else if (var_G == var_Max) H = (1 / 3) + del_R - del_B;
		else if (var_B == var_Max) H = (2 / 3) + del_G - del_R;

		if (H < 0) H += 1;
		if (H > 1) H -= 1;
	}

	h = H * 255.0;
	s = S * 255.0;
	v = L * 255.0;
*/

/*
	double delta, dh, dr, dg, db, mx, mn, rh, rs, rv;
	dr = (double)r;
	dg = (double)g;
	db = (double)b;
	mx = max(dr, max(dg, db));
	mn = min(dr, min(dg, db));
	delta = mx - mn;
	rv = mx;
	if (mx != 0) rs = (delta / mx) * 255.0;
	else rs = 0;
	if (rs == 0) 
	{
		rh = 0; 
	}
	else 
	{
		if (dr == mx) dh = (dg - db) / delta;
		else if (dg == mx) dh = 2 + (db - dr) / delta;
		else if (db == mx) dh = 4 + (dr - dg) / delta;
		dh *= 42.5;
		if (dh < 0) dh += 255.0;
		rh = dh;
	}
	// 	if (h) *h = (unsigned char)rh;
	// 	if (s) *s = (unsigned char)rs;
	// 	if (v) *v = (unsigned char)rv;
	h = rh;
	s = rs;
	v = rv;
*/
}

double GetProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double default)
{
	CString sDefault;
	CString sValue;

	sDefault.Format(_T("%f"), default);
	sValue = pApp->GetProfileString(lpszSection, lpszEntry, sDefault);

	return _tstof(sValue);
}

bool WriteProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double value)
{
	CString sValue;

	sValue.Format(_T("%f"), value);
	return pApp->WriteProfileString(lpszSection, lpszEntry, sValue);
}

/*
int GetSystemImageListIcon(CString szFile, BOOL bDrive)
{
	SHFILEINFO shFileInfo;

	if (szFile == _T("내 PC"))//GetStringById(NFTD_IDS_COMPUTER))
	{
		LPITEMIDLIST pidl_Computer = NULL;
		SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl_Computer); // 컴퓨터
		SHGetFileInfo((TCHAR*)pidl_Computer, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if (szFile == _T("내 문서"))//GetStringById(NFTD_IDS_DOCUMENT))
	{
		LPITEMIDLIST pidl_Document = NULL;
		SHGetFolderLocation(NULL, CSIDL_MYDOCUMENTS, NULL, 0, &pidl_Document); // 내문서
		SHGetFileInfo((TCHAR*)pidl_Document, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if (szFile == _T("바탕 화면"))//GetStringById(NFTD_IDS_DESKTOP))
	{
		LPITEMIDLIST pidl_Desktop = NULL;
		SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &pidl_Desktop); // 바탕화면
		SHGetFileInfo((TCHAR*)pidl_Desktop, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else
	{
		if (bDrive)
			SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		else
			SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

		//SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		//if (shFileInfo.iIcon < 0)
		//	SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		//else
		//{
		//	if (PathIsDirectory(szFile))
		//		SHGetFileInfo(szFile, FILE_ATTRIBUTE_DIRECTORY, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		//	else
		//		SHGetFileInfo(szFile, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		//}
	}

	return shFileInfo.iIcon;
}
*/

HINSTANCE	g_hInst_msimg32 = NULL;
LPFNDLLFUNC1 g_dllfunc_GradientFill;

void gradient_rect(CDC* pDC, CRect &rect, int preset, bool vertical)
{
	std::deque<COLORREF> dqColor;
	/*
	switch (preset)
	{
		//case 
	}
	*/
}

void gradient_rect(CDC* pDC, CRect &rect, std::deque<Gdiplus::Color> dqColor, bool vertical)
{
	rect.NormalizeRect();

	if (g_hInst_msimg32 == NULL)
	{
		g_hInst_msimg32 = LoadLibrary(_T("msimg32.dll"));

		if (!g_hInst_msimg32)
			return;

		g_dllfunc_GradientFill = ((LPFNDLLFUNC1) GetProcAddress(g_hInst_msimg32, "GradientFill"));
	}

	if (g_dllfunc_GradientFill == NULL)
		return;

	int i;
	double x1 = rect.left;
	double y1 = rect.top;
	double x2, y2;
	int size = dqColor.size();
	double divide;
	TRIVERTEX rcVertex[2];
	//std::deque<COLORREF>::iterator it;

	if (size == 1)
	{
		//it = dqColor.begin();
		dqColor.push_back(::GetSysColor(COLOR_3DFACE));
		size++;
	}

	//width 또는 height를 n등분해서 gradient fill을 한다.
	if (vertical)
	{
		divide = (double)rect.Height() / (double)(size-1);
	}
	else
	{
		divide = (double)rect.Width() / (double)(size-1);
	}

	for (i = 0; i < dqColor.size() - 1; i++)
	{
		rcVertex[0].x		= x1;
		rcVertex[0].y		= y1;
		rcVertex[0].Red		= dqColor[i].GetR() << 8;	// color values from 0x0000 to 0xff00 !!!!
		rcVertex[0].Green	= dqColor[i].GetG() << 8;
		rcVertex[0].Blue	= dqColor[i].GetB() << 8;
		rcVertex[0].Alpha	= 0;//128<<8;

		if (vertical)
		{
			x2 = rect.right;
			y2 = y1 + divide;
		}
		else
		{
			x2 = x1 + divide;
			y2 = rect.bottom;
		}


		rcVertex[1].x		= x2;
		rcVertex[1].y		= y2;
		rcVertex[1].Red		= dqColor[i+1].GetR() << 8;
		rcVertex[1].Green	= dqColor[i+1].GetG() << 8;
		rcVertex[1].Blue	= dqColor[i+1].GetB() << 8;
		rcVertex[1].Alpha	= 0;//255<<8;

		GRADIENT_RECT rect;
		rect.UpperLeft=0;
		rect.LowerRight=1;

		// fill the area 
		g_dllfunc_GradientFill(*pDC, rcVertex, 2, &rect, 1, vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);

		if (vertical)
			y1 = y2;
		else
			x1 = x2;
	}
}

void safe_release_gradient_rect_handle()
{
	if (g_hInst_msimg32)
	{
		FreeLibrary(g_hInst_msimg32);
		g_hInst_msimg32 = NULL;
	}
}

//format
//0 : "1 2 3 4"
//1 : "(1,2)~(4,8)"
//2 : "(1,2)~(4,8) (2x6)"
//3 : "l = 1, t = 2, r = 3, b = 4"
CString get_rect_info_string(CRect r, int nFormat)
{
	CString str;

	if (nFormat == 1)
		str.Format(_T("(%d, %d)~(%d, %d)"), r.left, r.top, r.right, r.bottom);
	else if (nFormat == 2)
		str.Format(_T("(%d, %d)~(%d, %d) (%d x %d)"), r.left, r.top, r.right, r.bottom, r.Width(), r.Height());
	else if (nFormat == 3)
		str.Format(_T("l = %d, t = %d, r = %d, b = %d"), r.left, r.top, r.right, r.bottom);
	else
		str.Format(_T("%d, %d, %d, %d"), r.left, r.top, r.right, r.bottom);

	return str;
}

CString get_rect_info_string(Gdiplus::Rect r, int nFormat)
{
	CString str;
	if (nFormat == 1)
		str.Format(_T("(%d, %d)~(%d, %d)"), r.X, r.Y, r.X + r.Width, r.Y + r.Height);
	else if (nFormat == 2)
		str.Format(_T("(%d, %d)~(%d, %d) (%d x %d)"), r.X, r.Y, r.X + r.Width, r.Y + r.Height, r.Width, r.Height);
	else if (nFormat == 3)
		str.Format(_T("l = %d, t = %d, r = %d, b = %d"), r.X, r.Y, r.X + r.Width, r.Y + r.Height);
	else
		str.Format(_T("%d, %d, %d, %d"), r.X, r.Y, r.X + r.Width, r.Y + r.Height);
	return str;
}

CString get_rect_info_string(Gdiplus::RectF r, int nFormat)
{
	CString str;
	if (nFormat == 1)
		str.Format(_T("(%f, %f)~(%f, %f)"), r.X, r.Y, r.X + r.Width, r.Y + r.Height);
	else if (nFormat == 2)
		str.Format(_T("(%f, %f)~(%f, %f) (%f x %f)"), r.X, r.Y, r.X + r.Width, r.Y + r.Height, r.Width, r.Height);
	else if (nFormat == 3)
		str.Format(_T("l = %f, t = %f, r = %f, b = %f"), r.X, r.Y, r.X + r.Width, r.Y + r.Height);
	else
		str.Format(_T("%f, %f, %f, %f"), r.X, r.Y, r.X + r.Width, r.Y + r.Height);
	return str;
}


void adjust_rect_range(float*l, float*t, float*r, float*b, float minx, float miny, float maxx, float maxy, bool retainSize, bool includeBottomRight)
{
	if (*l < minx)
	{
		if (retainSize)
		{
			*r += (minx - *l);
			if (*r >= maxx)
				*r = maxx - 1;
		}
		*l = minx;
	}

	if (*t < miny)
	{
		if (retainSize)
		{
			*b += (miny - *t);
			if (*b >= maxy)
				*b = maxy - 1;
		}
		*t = miny;
	}

	if (*r >= maxx)
	{
		if (retainSize)
		{
			*l -= (*r - maxx);
			if (*l < minx)
				*l = minx;
		}

		*r = maxx - (includeBottomRight ? 1 : 0);
	}

	if (*b >= maxy)
	{
		if (retainSize)
		{
			*t -= (*b - maxy);
			if (*t < miny)
				*t = miny;
		}

		*b = maxy - (includeBottomRight ? 1 : 0);
	}
}

void adjust_rect_range(CRect& rect, CRect rLimit, bool bRetainSize, bool includeBottomRight)
{
	rect.NormalizeRect();
	rLimit.NormalizeRect();

	if (rect.left < rLimit.left)
	{
		if (bRetainSize)
			rect.MoveToX(rLimit.left);
		else
			rect.left = rLimit.left;
	}
	if (rect.top < rLimit.top)
	{
		if (bRetainSize)
			rect.MoveToY(rLimit.top);
		else
			rect.top = rLimit.top;
	}
	if (rect.right > rLimit.right)
	{
		if (bRetainSize)
			rect.MoveToX(rLimit.right - rect.Width());
		else
			rect.right = rLimit.right;
	}
	if (rect.bottom > rLimit.bottom)
	{
		if (bRetainSize)
			rect.MoveToY(rLimit.bottom - rect.Height());
		else
			rect.bottom = rLimit.bottom;
	}

	if (!includeBottomRight)
	{
		if (rect.right == rLimit.right)
			rect.right--;
		if (rect.bottom == rLimit.bottom)
			rect.bottom--;
	}

	//adjust_rect_range(rect, rLimit.left, rLimit.top, rLimit.right, rLimit.bottom, bRetainSize);
}

void adjust_rect_range(CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize, bool includeBottomRight)
{
	float l = rect.left;
	float t = rect.top;
	float r = rect.right;
	float b = rect.bottom;
	adjust_rect_range(&l, &t, &r, &b, minx, miny, maxx, maxy, bRetainSize, includeBottomRight);
	rect = CRect(l, t, r, b);
}

void adjust_rect_range(Gdiplus::RectF& rect, Gdiplus::RectF rLimit, bool bRetainSize, bool includeBottomRight)
{
	float l = rect.X;
	float t = rect.Y;
	float r = rect.X + rect.Width;
	float b = rect.Y + rect.Height;
	adjust_rect_range(&l, &t, &r, &b, rLimit.X, rLimit.Y, rLimit.X + rLimit.Width, rLimit.Y + rLimit.Height, bRetainSize, includeBottomRight);
	rect.X = l;
	rect.Y = t;
	rect.Width = r - l;
	rect.Height = b - t;
}

//모니터의 한쪽에 붙은 사각형을 새로운 크기로 변경할 경우 붙은 상태를 유지하고 변경할 필요가 있을 경우 사용.
void adjust_with_monitor_attached(CRect rOld, CRect &rNew)
{
	int nw = rNew.Width();
	int nh = rNew.Height();

	if (rOld.left == g_monitors[0].rMonitor.left)
		rNew.MoveToX(g_monitors[0].rMonitor.left);
	if (rOld.top == g_monitors[0].rMonitor.top)
		rNew.MoveToY(g_monitors[0].rMonitor.top);
	if (rOld.right == g_monitors[0].rMonitor.right)
		rNew.MoveToX(g_monitors[0].rMonitor.right - nw);
	if (rOld.bottom == g_monitors[0].rMonitor.bottom)
		rNew.MoveToY(g_monitors[0].rMonitor.bottom - nh);
}

void normalize_rect(Gdiplus::RectF& r)
{
	if (r.X > r.X + r.Width)
	{
		float tmp = r.X;
		r.X = r.X + r.Width;
		r.Width = tmp - r.X;
	}
	if (r.Y > r.Y + r.Height)
	{
		float tmp = r.Y;
		r.Y = r.Y + r.Height;
		r.Height = tmp - r.Y;
	}
}

//rTarget에 접하는 dRatio인 최대 사각형을 구한다.
//attach_left 등의 옵션을 줄 필요가 있다.
CRect get_ratio_rect(CRect rTarget, int w, int h, int attach, bool stretch)
{
	return get_ratio_rect(rTarget, (double)w / (double)h, attach, stretch);
}

//rTarget에 접하는 dRatio인 최대 사각형을 구한다.
CRect get_ratio_rect(CRect rTarget, double dRatio, int attach, bool stretch)
{
	int		w = rTarget.Width();
	int		h = rTarget.Height();
	int		nNewW;
	int		nNewH;
	double	dTargetRatio = (double)rTarget.Width() / (double)rTarget.Height();

	CRect	rResult;

	if (rTarget.IsRectEmpty())
		return CRect();

	bool bResizeWidth;

	if (dRatio > 1.0)
	{
		if (dTargetRatio < dRatio)
			bResizeWidth = false;
		else
			bResizeWidth = true;
	}
	else
	{
		if (dTargetRatio > dRatio)
			bResizeWidth = true;
		else
			bResizeWidth = false;
	}


	if (bResizeWidth)
	{
		rResult.top = rTarget.top;
		rResult.bottom = rTarget.bottom;

		nNewW = (double)(rTarget.Height()) * dRatio;
		if (attach & attach_left)
			rResult.left = rTarget.left;
		else if (attach & attach_right)
			rResult.left = rTarget.right - nNewW;
		else
			rResult.left = rTarget.left + (rTarget.Width() - nNewW) / 2.0;

		rResult.right = rResult.left + nNewW;
	}
	else
	{
		rResult.left	= rTarget.left;
		rResult.right	= rTarget.right;

		nNewH = (double)(rTarget.Width()) / dRatio;

		if (attach & attach_top)
			rResult.top = rTarget.top;
		else if (attach & attach_bottom)
			rResult.top = rTarget.bottom - nNewH;
		else
			rResult.top = rTarget.top + (rTarget.Height() - nNewH) / 2.0;

		rResult.bottom	= rResult.top + nNewH;
	}

	return rResult;
}

//w x h 사각형을 target안에 넣을 때 중앙에 표시되게 하는 사각형 영역을 리턴한다.
//w, h보다 target이 적을때는 target보다 큰 영역이 리턴될 것이다.
CRect get_center_rect(CRect target, int w, int h)
{
	return CRect(target.left + (target.Width() - w) / 2, target.top + (target.Height() - h) / 2, w, h);
}

//주어진 점들을 포함하는 최대 사각형을 구한다.
CRect get_max_rect(CPoint *pt, int nPoints)
{
	std::vector<CPoint> pts;
	for (int i = 0; i < nPoints; i++)
		pts.push_back(pt[i]);
	return get_max_rect(pts);
}

//pt_max
CRect get_max_rect(std::vector<CPoint> pts, int pt_max /*= -1*/)
{
	int max_pt = pts.size();
	CRect	rMax(INT_MAX, INT_MAX, -1, -1);

	if (pt_max > 0)
		max_pt = pt_max;

	for (int i = 0; i < max_pt; i++)
	{
		if (pts[i].x < rMax.left)
			rMax.left = pts[i].x;
		if (pts[i].x > rMax.right)
			rMax.right = pts[i].x;
		if (pts[i].y < rMax.top)
			rMax.top = pts[i].y;
		if (pts[i].y > rMax.bottom)
			rMax.bottom = pts[i].y;
	}

	return rMax;
}


//다각형의 넓이를 구한다. 단, 변이 하나라도 교차되면 성립하지 않는다.
double		GetPolygonAreaSize(CPoint *pt, int nPoints)
{
	double dArea = 0.0;

	for (int i = 0; i < nPoints; i++)
	{
		if (i < nPoints - 1)
			dArea = dArea + (pt[i].x + pt[i+1].x) * (pt[i].y - pt[i+1].y);
		else if (i == nPoints - 1)
			dArea = dArea + (pt[i].x + pt[0].x) * (pt[i].y - pt[0].y);
	}

	dArea /= 2.0;

	return dArea;
}

//return받은 char*는 반드시 사용 후 free()해줘야 함.
TCHAR* replace(TCHAR* src, const TCHAR* olds, const TCHAR* news)
{
	TCHAR* result;
	int i, cnt = 0;
	int newWlen = _tcslen(news);
	int oldWlen = _tcslen(olds);

	// Counting the number of times old word 
	// occur in the string 
	for (i = 0; src[i] != '\0'; i++) {
		if (_tcsstr(&src[i], olds) == &src[i]) {
			cnt++;

			// Jumping to index after the old word. 
			i += oldWlen - 1;
		}
	}

	// Making new string of enough length 
	result = (TCHAR*)malloc(i + cnt * (newWlen - oldWlen) + 1);

	i = 0;
	while (*src)
	{
		// compare the substring with the result 
		if (_tcsstr(src, olds) == src)
		{
			_tcscpy(&result[i], news);
			i += newWlen;
			src += oldWlen;
		}
		else
			result[i++] = *src++;
	}

	result[i] = '\0';
	return result;
	/*
	char* result, * sr;
	size_t i, count = 0;
	size_t oldlen = strlen(olds); if (oldlen < 1) return s;
	size_t newlen = strlen(news);

	if (newlen != oldlen)
	{
		for (i = 0; s[i] != '\0';)
		{
			if (memcmp(&s[i], olds, oldlen) == 0) count++, i += oldlen;
			else i++;
		}
	}
	else
	{
		i = strlen(s);
	}

	result = (char*)malloc(i + 1 + count * (newlen - oldlen));
	if (result == NULL)
		return NULL;

	sr = result;
	while (*s)
	{
		if (memcmp(s, olds, oldlen) == 0)
		{
			memcpy(sr, news, newlen);
			sr += newlen;
			s += oldlen;
		}
		else
		{
			*sr++ = *s++;
		}
	}
	*sr = '\0';

	return result;
	*/
}

//src의 끝에서 length 길이 만큼 잘라낸다.
CString truncate(CString &src, int length)
{
	CString result;

	int src_len = src.GetLength();
	if (length < src_len)
	{
		result = src.Left(src.GetLength() - length);
	}
	
	src = result;
	return result;
}

//src끝의 문자열이 sub와 일치하면 잘라낸다.
CString	truncate(CString &src, CString sub)
{
	CString result;

	int sub_len = sub.GetLength();
	if (src.GetLength() > sub_len)
	{
		result = src.Left(src.GetLength() - sub_len);
	}

	src = result;
	return result;
}

void Trim(char* src)
{
	int		i, idx = 0;
	int		len = strlen(src);
	char*	temp = new char[len + 1];
	
	strcpy(temp, src);
	memset(src, 0, len);

	for (i = 0; i < len; i++)
	{
		if (temp[i] != ' ' && temp[i] != '\t' && temp[i] != '\n' && temp[i] != '\r')
			src[idx++] = temp[i];
	}

	delete [] temp;
}

//공백, '\t', '\r', '\n', '\0' 모두 제거
void trim(std::string& str)
{
	trim_left(str);
	trim_right(str);
}

//공백, '\t', '\r', '\n' 모두 제거
void trim_left(std::string& str)
{
	int none_trim_char_pos = 0;
	std::string trim_char = " \r\n\t";

	for (int i = 0; i < str.length(); i++)
	{
		if (trim_char.find(str[i]) == std::string::npos)
		{
			none_trim_char_pos = i;
			break;
		}
	}

	if (none_trim_char_pos < str.length() - 1)
	{
		str.erase(str.begin(), str.begin() + none_trim_char_pos);
	}
}

void trim_right(std::string& str)
{
	str.erase(str.find_last_not_of(" \r\n\t") + 1);
}


void trim(std::deque<CString>* dq)
{
	for (int i = 0; i < dq->size(); i++)
		dq->at(i).Trim();
}

//src문자열에서 chars를 모두 제거한다.
void remove_chars(CString &src, CString chars)
{
	for (int i = 0; i < chars.GetLength(); i++)
		src.Remove(chars[i]);
}

void make_lower(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void make_upper(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

std::string lower(std::string str)
{
	std::string dst;
	dst.resize(str.size());
	std::transform(str.begin(), str.end(), dst.begin(), ::tolower);
	return dst;
}

std::string upper(std::string str)
{
	std::string dst;
	dst.resize(str.size());
	std::transform(str.begin(), str.end(), dst.begin(), ::toupper);
	return dst;
}

float Similarity(char *str1, char *str2)
{
	size_t	len1=strlen(str1), len2=strlen(str2);
	float	lenLCS;
	unsigned j, k, *previous, *next;

	if (len1 == 0) return (float)len2;
	if (len2 == 0) return (float)len1;

	previous = (unsigned *)calloc(len1 + 1, sizeof(unsigned));
	next = (unsigned *)calloc(len1 + 1, sizeof(unsigned));

	for (j = 0; j < len2; ++j)
	{
		for (k = 1; k <= len1; ++k)
		{
			if (str1[k-1] == str2[j])
				next[k] = previous[k-1] + 1;
			else
				next[k] = previous[k] >= next[k-1] ? previous[k] : next[k-1];
		}

		Swap(previous, next);
	}

	lenLCS = (float)previous[len1];

	free(previous);
	free(next);

	return lenLCS /= len1;
}

int minimum(int a, int b, int c)
{
	return MIN(a, MIN(b, c));
}

int cost;
int LevenshteinDistance(std::string s, int len_s, std::string t, int len_t)
{
	/* base case: empty strings */
	if (len_s == 0) return len_t;
	if (len_t == 0) return len_s;

	/* test if last characters of the strings match */
	if (s[len_s-1] == t[len_t-1])
		cost = 0;
	else
		cost = 1;

	/* return minimum of delete char from s, delete char from t, and delete char from both */
	return minimum(LevenshteinDistance(s, len_s - 1, t, len_t) + 1,
		LevenshteinDistance(s, len_s    , t, len_t - 1) + 1,
		LevenshteinDistance(s, len_s - 1, t, len_t - 1) + cost);
}
/*
int LevenshteinDistance(char* s, char* t)
{
	// degenerate cases
	if (strcmp(s, t) == 0)
		return 0;

	if (strlen(s) == 0) return strlen(t);
	if (strlen(t) == 0) return strlen(s);

	// create two work vectors of integer distances
	int* v0 = new int[strlen(t) + 1];
	int* v1 = new int[strlen(t) + 1];

	// initialize v0 (the previous row of distances)
	// this row is A[0][i]: edit distance for an empty s
	// the distance is just the number of characters to delete from t
	for (int i = 0; i < v0.Length; i++)
		v0[i] = i;

	for (int i = 0; i < s.Length; i++)
	{
		// calculate v1 (current row distances) from the previous row v0

		// first element of v1 is A[i+1][0]
		//   edit distance is delete (i+1) chars from s to match empty t
		v1[0] = i + 1;

		// use formula to fill in the rest of the row
		for (int j = 0; j < t.Length; j++)
		{
			var cost = (s[i] == t[j]) ? 0 : 1;
			v1[j + 1] = Minimum(v1[j] + 1, v0[j + 1] + 1, v0[j] + cost);
		}

		// copy v1 (current row) to v0 (previous row) for next iteration
		for (int j = 0; j < v0.Length; j++)
			v0[j] = v1[j];
	}

	return v1[t.Length];
}
*/

/*
std::set<std::string> get_shingle(const std::string& str, int size)

{

	std::set<std::string> shingle;

	int length = (int)(str.size());

	int max_pos = length - size;

	int i = 0;

	for(i = 0; i<=max_pos; i++){

		shingle.insert(str.substr(i, size));

	}

	return shingle;

}


bool get_resemblance(


	const std::string& str1, 


	const std::string& str2, 


	double *r, double *c1, double *c2)


{

	std::set<std::string> s1 = get_shingle(str1, 2);

	std::set<std::string> s2 = get_shingle(str2, 2);

	if(s1.empty() == true || s2.empty() == true) return false;

	std::set<std::string> com;

	std::set<std::string> all;

	std::set_intersection(s1.begin(), s1.end(), 



		s2.begin(), s2.end(), 


		std::inserter(com, com.begin()));


	std::set_union(s1.begin(), s1.end(), 



		s2.begin(), s2.end(), 


		std::inserter(all, all.begin()));






	double com_size = (double)(com.size());

	double union_size = (double)(all.size());



	*r = com_size/union_size;

	*c1 = com_size/s1.size();

	*c2 = com_size/s2.size();



	return true;

}
*/

/*
int get_ucs4_levenshtein_distance(string str1, string str2, double* ratio)
{
	wchar_t wcs1[1024];
	wchar_t wcs2[1024];
	int ret = mbstowcs(wcs1, str1.c_str(), str1.length());
	wcs1[ret] = 0;
	ret = mbstowcs(wcs2, str2.c_str(), str2.length());
	wcs2[ret] = 0;

	UINT len1 = wcslen(wcs1);
	UINT len2 = wcslen(wcs2);
	int d[1024][1024];

	for (UINT i = 0; i <= len1; ++i) {
		d[i][0] = i;
	}
	for (UINT j = 0; j <= len2; ++j) {
		d[0][j] = j;
	}

	for (UINT i = 0; i < len1; ++i) {
		for (UINT j = 0; j < len2; ++j) {
			int cost;
			if (wcs1[i] == wcs2[j]) {
				cost = 0;
			} else {
				cost = 1;
			}
			d[i + 1][j + 1] =
				min(min(d[i][j + 1] + 1, d[i + 1][j] + 1), d[i][j] + cost);
		}
	}

	if (ratio != NULL)
	{
		*ratio = 1.0 - double(d[len1][len2]) / double(max(len1, len2));
	}

	return d[len1][len2];
}
*/

//차량정보파일(xml) 저장 관련
#if 0
bool SavePlateInfoFile(char* sfile, char* sPlate, RECT* rect /*= NULL*/)
{
	FILE*	fp = fopen(sfile, "wt");

	if (fp == NULL)
		return false;

	CPlateCode plateCode(sPlate);

	fprintf(fp,"<?xml version=\"1.0\" encoding=\"EUC-KR\"?>\n<RexMetaInfo>\n<PlateInfo>\n");

	if (rect != NULL)
		fprintf(fp, "<Rect>%d,%d,%d,%d</Rect>\n", rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);

	fprintf(fp, "<Code>%s</Code>\n<LocalCode value=\"%s\"> </LocalCode>\n<TypeCode value=\"%s\"> </TypeCode>\n<UseCode value=\"%s\"> </UseCode>\n<DigitCode value=\"%s\"> </DigitCode>\n</PlateInfo>\n</RexMetaInfo>\n",
				 (strcmp(sPlate, "") == 0) ? "X" : sPlate,
				 plateCode.sLocalCode, plateCode.sTypeCode, plateCode.sUseCode, plateCode.sDigitCode);

	fclose(fp);

	return true;
}
#endif 

//32비트인 dw에 들어있는 R, G, B를 추출하여 16비트(5+6+5) 컬러로 리턴한다.
WORD RGB24ToRGB565(DWORD dw)
{
	BYTE	bt;
	WORD	wd = 0;

	bt = GetRValue(dw) >> 3;
	wd = bt;
	wd <<= 6;

	bt = GetGValue(dw) >> 2;
	wd |= bt;
	wd <<= 5;

	bt = GetBValue(dw) >> 3;
	wd |= bt;

	return wd;
}

//두 DWORD를 WORD로 변환하여 하나의 DWORD로 변환한다.
DWORD RGB24ToRGB565(DWORD dw1, DWORD dw2)
{
	DWORD dw = RGB24ToRGB565(dw1);
	dw <<= 16;

	dw |= RGB24ToRGB565(dw2);
	return dw;
}

//RGB565로 축약된 WORD를 다시 24비트 RGB(DWORD)로 복원시킨다.
DWORD RGB565ToRGB24(WORD wd)
{
	//565로 만들어진 wd에서 다시 rgb로 복원시켜보자.
	BYTE	r = (wd & 0xf800) >> 11 << 3;
	BYTE	g = (wd & 0x07e0) >> 5 << 2;
	BYTE	b = (wd & 0x001f) << 3;
	
	return RGB(r, g, b);
}

#include <winspool.h>
int	GetPrinterList(CStringArray *arPrinters)
{
	BYTE*	pPrinterInfo = NULL;
	DWORD	dwOptions = PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL;
	DWORD	dwNeeds = 0;
	DWORD dwRet = 0;
	DWORD nIndex = 0;
	bool	fRet = FALSE;

	arPrinters->RemoveAll();

	::EnumPrinters(dwOptions, NULL, 1, NULL, dwNeeds, &dwNeeds, &dwRet);
	if (!dwNeeds)
		return 0;

	pPrinterInfo = (BYTE*)malloc(sizeof(BYTE)* dwNeeds + 1);
	if (!pPrinterInfo)
		return 0;
	memset(pPrinterInfo, 0x00, dwNeeds + 1);

	fRet = ::EnumPrinters(dwOptions, NULL, 1, (BYTE*)pPrinterInfo, dwNeeds, &dwNeeds, &dwRet);
	if (!fRet)
		return 0;

	while(nIndex < dwRet)
	{
		arPrinters->Add(((PRINTER_INFO_1*)pPrinterInfo)[nIndex].pName);
		nIndex++;
	}

	if (pPrinterInfo)
		free(pPrinterInfo);

	return dwRet;
}

CString GetDefaultPrinterName()
{
	TCHAR szPrnName[250];
	DWORD dwSize = 250;

	if (!GetDefaultPrinter(szPrnName, &dwSize))
	{
		int d = GetLastError();
		if (d == ERROR_INSUFFICIENT_BUFFER)
			AfxMessageBox(_T("GetDefaultPrinterName() : ERROR_INSUFFICIENT_BUFFER"));
		else if (d == ERROR_FILE_NOT_FOUND)
			AfxMessageBox(_T("GetDefaultPrinterName() : ERROR_FILE_NOT_FOUND"));
		return _T("");
	}

	return szPrnName;
}

//unit : mm
CSize GetPrinterPaperSize(CString sPrinterName)
{
	CDC   PrinterDC;
	DOCINFO  DocInfo = { sizeof(DOCINFO) };
	CString  str;

	DocInfo.lpszDocName = _T("");

	if (!PrinterDC.CreateDC(NULL, sPrinterName, NULL, NULL))
	{
		str.Format(_T("\"%s\"\nCan't find this printer."), sPrinterName);
		AfxMessageBox(str);
		return CSize(-1,-1);
	}

	int		nPaperWidth = GetDeviceCaps(PrinterDC, HORZSIZE);
	int		nPaperHeight = GetDeviceCaps(PrinterDC, VERTSIZE);

	PrinterDC.DeleteDC();

	return CSize(nPaperWidth, nPaperHeight);
}

//#include <system_error>>
CString	get_error_str(DWORD dwError)
{
	LPTSTR lpBuffer = NULL;
	CString result;

	if (dwError >= 12000 && dwError <= 12174)
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
			//for ERROR_WINHTTP_TIMEOUT(12002), then use winhttp.dll,
			//for ERROR_INTERNET_TIMEOUT(12002), then use wininet.dll.
			GetModuleHandle(_T("wininet.dll")),
			dwError, 0, (LPTSTR)&lpBuffer, 0, NULL);
	}
	else
	{
		std::string msg = std::system_category().message(dwError);
		return CString(msg.c_str());

		//FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		//	NULL,
		//	dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lpBuffer, 0, NULL);
	}

	result.Format(_T("%s"), CString(lpBuffer));
	result.Trim();
	LocalFree(lpBuffer);

	return result;
}

//원점에서 벡터 b가 벡터 a의 반시계 방향이면 양수, 시계방향이면 음수, 평행이면 0을 반환 한다.
double ccw(vector2 a, vector2 b)
{
    return a.cross(b);
}

//점 p를 기준으로 벡터 b가 벡터 a의 반시계 방향이면 양수, 시계방향이면 음수, 평행이면 0을 반환 한다.
double ccw(vector2 p, vector2 a, vector2 b)
{
    return ccw(a-p, b-p);
}

// - 점 a, b를 지나는 직선과 점 c, d를 지나는 직선의 교점을 x에 반환한다.
// - 두 직선이 평행이면(겹치는 경우 포함) 거짓을, 아니면 참을 반환한다.
// 출처 : http://bowbowbow.tistory.com/17
bool GetIntersectionPoint(vector2 a, vector2 b, vector2 c, vector2 d, vector2& x)
{
    double det = (b-a).cross(d-c);
    //두선이 평행인 경우
    if(fabs(det) < DBL_EPSILON) return false;
    x = a+(b-a)*((c-a).cross(d-c)/det);
    return true;
}

bool GetIntersectionPoint(CPoint* pt, CPoint* ispt)
{
	vector2		a(pt[0].x, pt[0].y), b(pt[1].x, pt[1].y), c(pt[2].x, pt[2].y), d(pt[3].x, pt[3].y);
	vector2		ptResult;
	bool		bIntersect = GetIntersectionPoint(a, b, c, d, ptResult);

	if (ispt != NULL)
	{
		ispt->x = (int)ptResult.x;
		ispt->y = (int)ptResult.y;
	}

	return bIntersect;
}

bool GetIntersectionPoint(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int& isx, int& isy)
{
	CPoint pt[4] = { CPoint(x0,y0), CPoint(x1,y1), CPoint(x2,y2), CPoint(x3,y3) };
	CPoint ispt;
	bool bIntersect = GetIntersectionPoint(pt, &ispt);
	isx = ispt.x;
	isy = ispt.y;

	return bIntersect;
}

//4점으로 구성된 두 선분이 서로 교차하는지 여부를 판별한다.
bool IsIntersect2LineSegment(vector2 a, vector2 b, vector2 c, vector2 d)
{
    double ab = ccw(a, b, c)*ccw(a, b, d);
    double cd = ccw(c, d ,a)*ccw(c, d, b);

    //두 선분이 한 직선에 위에 있거나 끝점이 겹치는 경우
    if (ab == 0 && cd == 0)
	{
        if(b < a) std::swap(a, b);
        if(d < c) std::swap(c, d);
        return !(b< c || d <a);
    }

    return ab <=0 && cd <=0;
}

bool IsIntersect2LineSegment(CPoint* pt, bool bCheckCrossOver)
{
	vector2	a(pt[0].x, pt[0].y), b(pt[1].x, pt[1].y), c(pt[2].x, pt[2].y), d(pt[3].x, pt[3].y);

	bool b0123 = IsIntersect2LineSegment(a, b, c, d);
	bool b0312 = IsIntersect2LineSegment(a, d, b, c);

	if (bCheckCrossOver)
		return b0123 || b0312;

	return b0123;
}

//점 a, b와 점 c, d가 평행한 두 선분 일 때 이들이 한 점에서 겹치는지 확인한다.
bool paralleSegments(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p){
    if(b < a) std::swap(a,b);
    if(d < c) std::swap(c,d);

    //한 직선위에 없거나 두 선분이 겹치지 않는 경우를 우선 걸러낸다. 본문의 1번 관계인 경우이다.
    if(ccw(a, b, c) != 0 || b < c || d < a) return false;

    //두 선분이 확실히 겹친다면 교차점 하나를 찾는다.
    if(a<c) p = c;
    else p = a;
    return true;
}


// - p가 두 점 a, b를 감싸면서 각 변이 x, y축에 평행한 최소사각형 내부에 있는지 확인한다.
// a, b, p는 일직선 상에 있다고 가정한다.
bool inBoundingRectangle(vector2 p, vector2 a, vector2 b){
    if(b < a) std::swap(a, b);
    return p == a || p == b || (a <p && p < b);
}


// - 두 점 a, b를 지나는 선분과 두 점 c, b를 지나는 선분을 p에 반환한다.
// - 교짐이 여러개일 경우 아무점이나 반환한다.
bool segmentIntersection(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p){
    //두 직선이 평행인 경우를 우선 예외로 처리한다.
    if(!GetIntersectionPoint(a, b, c, d, p))
        return paralleSegments(a, b, c, d, p);
    //p가 두 선분에 포함되어 있는 경우에만 참을 반환한다.
    return inBoundingRectangle(p, a, b) && inBoundingRectangle(p, c, d);
}


//삼각형 외접원의 중심을 기하학적으로 구한다.(http://kipl.tistory.com/113)
int circumCenter(CPoint A, CPoint B, CPoint C, double *xc, double *yc)
{
	double ax = A.x - C.x ;
	double ay = A.y - C.y ;
	double bx = B.x - C.x ;
	double by = B.y - C.y ;
	double asq = ax * ax + ay * ay;
	double bsq = bx * bx + by * by;
	double ccw = ax * by - ay * bx;

	 // 세 점임 일직선 위에 있지 않는 경우; 이 경우만 외접원이 정의됨;
	if (ccw == 0.)
		return 0;

	*xc = C.x + (by * asq - ay * bsq) / (2. * ccw) ;
	*yc = C.y + (-bx * asq + ax * bsq) / (2. * ccw) ;
	return 1;
}

//삼각형 외접원의 중심을 대수적으로 구한다.
int circumCenter2(CPoint P, CPoint Q, CPoint R, double *xc, double *yc)
{
	double A = Q.x - P.x;
	double B = Q.y - P.y;
	double C = R.x - P.x;
	double D = R.y - P.y;
	double E = A * (P.x + Q.x) + B * (P.y + Q.y);
	double F = C * (P.x + R.x) + D * (P.y + R.y);
	double G = 2. * (A * D - B * C);

	// 세 점이 일직선에 놓이지 않는 경우; 이 경우만 외접원이 정의된다;
	if (G == 0.)
		return 0;

	*xc = (D * E - B * F) / G;
	*yc = (A * F - C * E) / G;
	return 1;
}

//삼각형 외접원의 반지름을 구한다.
double circumRadius(CPoint A, CPoint B, CPoint C)
{
	double ax = C.x - B.x; 
	double ay = C.y - B.y;
	double bx = A.x - C.x; 
	double by = A.y - C.y;
	double crossab = ax * by - ay * bx;

	if (crossab == 0.)
		return -1.0;

	double a = sqrt(SQR(ax) + SQR(ay));
	double b = sqrt(SQR(bx) + SQR(by)); 
	double cx = B.x - A.x; 
	double cy = B.y - A.y;       
	double c = sqrt(SQR(cx) + SQR(cy));

	return (0.5 * a * b * c/fabs(crossab));
}


//임의 점이 폴리곤 내에 존재하는지 판별
//이 함수 아래의 PtInPolygon0 ~ PtInPolygon2까지는 미완인지 뭔지 모르겠으나 올바른 판별을 하지 못한다.
bool PtInPolygon(CPoint *ptPolygons, CPoint pt, int nCorners)
{
	int counter = 0;
	int i;
	double xinters;
	CPoint p1, p2; p1 = ptPolygons[0];
	
	for (i = 1; i <= nCorners; i++)
	{
		p2 = ptPolygons[i % nCorners];
		
		if (pt.y > MIN(p1.y,p2.y))
		{
			if (pt.y <= MAX(p1.y,p2.y))
			{
				if (pt.x <= MAX(p1.x,p2.x))
				{
					if (p1.y != p2.y)
					{
						xinters = (pt.y-p1.y) * (p2.x-p1.x) / (p2.y-p1.y) + p1.x;
						
						if (p1.x == p2.x || pt.x <= xinters)
							counter++;
					}
				}
			}
		}
		
		p1 = p2;
	}
	
	return (counter % 2 != 0);
}

bool PtInPolygon0(CPoint* ptPolygons, CPoint pt, int nCorners)
{
	int		i, j = nCorners - 1;
	bool	bOddNodes = false;

	for (i = 0; i < nCorners; i++)
	{
		if (ptPolygons[i].y < pt.y && ptPolygons[j].y >= pt.y || ptPolygons[j].y < pt.y && ptPolygons[i].y >= pt.y)
		{
			if (ptPolygons[i].x + (pt.y-ptPolygons[i].y) / (ptPolygons[j].y - ptPolygons[i].y) * (ptPolygons[j].x-ptPolygons[i].x) < pt.x)
				bOddNodes = !bOddNodes;
		}

		j = i;
	}

	return bOddNodes;
}  

bool PtInPolygon1(CPoint* ptPolygons, CPoint pt, int nCorners)
{

	int		i, j = nCorners - 1;
	bool	bOddNodes = false;

	for (i = 0; i < nCorners; i++)
	{
		if ((ptPolygons[i].y < pt.y && ptPolygons[j].y >= pt.y	||
			  ptPolygons[j].y < pt.y && ptPolygons[i].y >= pt.y) &&
			 (ptPolygons[i].x <= pt.x || ptPolygons[j].x <= pt.x))
		{
			if (ptPolygons[i].x + (pt.y - ptPolygons[i].y) / (ptPolygons[j].y - ptPolygons[i].y) * (ptPolygons[j].x - ptPolygons[i].x) < pt.x)
			{
				bOddNodes = !bOddNodes;
			}
		}

		j = i;
	}

	return bOddNodes;
}  


bool PtInPolygon2(CPoint* ptPolygons, CPoint pt, int nCorners)
{
	int		i, j = nCorners - 1;
	bool	bOddNodes = false;

	for (i = 0; i < nCorners; i++)
	{
		if ((ptPolygons[i].y < pt.y && ptPolygons[j].y >= pt.y ||
			   ptPolygons[j].y < pt.y && ptPolygons[i].y >= pt.y) &&
			   (ptPolygons[i].x <= pt.x || ptPolygons[j].x <= pt.x))
		{
			bOddNodes ^= (ptPolygons[i].x + (pt.y - ptPolygons[i].y) / (ptPolygons[j].y - ptPolygons[i].y) * (ptPolygons[j].x - ptPolygons[i].x) < pt.x);
		}

		j = i;
	}

	return bOddNodes;
}  

CString bstrToCString(BSTR bstr)
{
	char* pbstr;
	USES_CONVERSION; //convert를 위한 매크로
	pbstr=OLE2A(bstr); //bstr을 char로 convert
	return CString(pbstr);
} 

BSTR CStringTobstr(CString str) 
{
	return str.AllocSysString(); //CString을 bstr로 convert
}

//[bstr-> char*]
char* bstrTocharStr(BSTR bstr)
{
	char *pbstr;
	USES_CONVERSION;
	pbstr = OLE2A(bstr);
	return pbstr;
}
/*
//[char* -> bstr]
BSTR charStrToBSTR(char* pbstr)
{
	TCHAR szTitle[ 512 ];
	_bstr_t bstrText = pbstr;
	*bstrTitle = bstrText.copy();
}
*/
/*
//CString ==> CComVariant로 변환 
CComVariant out(str.AllocSysString()); or CComVariant out(str);


//CComVariant ==> CString
CString szTmp;
szTmp = comvariant.bstrVal;
*/


VARIANT VariantString(BSTR str)
{
	VARIANT var;
	VariantInit(&var);
	V_BSTR(&var) = SysAllocString(str);
	V_VT(&var) = VT_BSTR;
	return var;
}

//256 gray bmi 생성
HBITMAP MakeDIBSection(CDC& dc, int width, int height)
{
	LPVOID pBits;
	HBITMAP hBitmap;

	struct BITMAPINFO256
	{
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColors[256];
	} bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO256));

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biBitCount = 8;
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biClrUsed = 256;
	bmi.bmiHeader.biClrImportant = 0;


	for (int i = 0; i < 256; i++)
	{
		bmi.bmiColors[i].rgbRed = i;
		bmi.bmiColors[i].rgbGreen = i;
		bmi.bmiColors[i].rgbBlue = i;
	}

	hBitmap = ::CreateDIBSection(dc.GetSafeHdc(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);

	return hBitmap;
}

//main에서 EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); 를 실행하고
//이 파일에 전역변수로 선언된 g_dqMonitor를 이용하면 된다.
//단, Win32API인 EnumDisplayMonitors()를 호출할때는 반드시 g_monitors.clear()를 해줘야 하므로
//enum_display_monitors()함수로 대체한다.
void enum_display_monitors()
{
	g_monitors.clear();
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
}

//main에서 enum_display_monitors(); 를 실행하고
//이 파일에 전역변수로 선언된 g_dqMonitor를 이용하면 된다.
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	// 모니터 정보를 가져올 구조체
	MONITORINFOEX mi;
	mi.cbSize = sizeof(MONITORINFOEX);

	// 모니터 핸들을 사용하여 해당 모니터 정보를 가져온다.
	GetMonitorInfo(hMonitor, &mi);

	CString str;

	// 주모니터로 설정된 모니터 정보인지를 체크하고 주모니터는 맨 앞 0번에 추가한다.
	if (mi.dwFlags & MONITORINFOF_PRIMARY) 
	{
		str.Format(_T("hMonitor = %X, name = %s, rcMonitor = %s, rcWork = %s, <Primary-Monitor> %s"), 
					hMonitor, mi.szDevice, get_rect_info_string(mi.rcMonitor), get_rect_info_string(mi.rcWork), mi.szDevice);

		g_monitors.push_front(CSCMonitorInfo(&mi, hMonitor));
	}
	else
	{
		str.Format(_T("hMonitor = %X, name = %s, rcMonitor = %s, rcWork = %s, %s"), 
					hMonitor, mi.szDevice, get_rect_info_string(mi.rcMonitor), get_rect_info_string(mi.rcWork), mi.szDevice);
		g_monitors.push_back(CSCMonitorInfo(&mi, hMonitor));
	}

	//TRACE(_T("%s\n"), str);

	return TRUE;
}

//x, y가 속해있는 모니터 인덱스를 리턴
int	get_monitor_index(int x, int y)
{
	enum_display_monitors();

	for (int i = 0; i < g_monitors.size(); i++)
	{
		if (g_monitors[i].rMonitor.PtInRect(CPoint(x, y)))
			return i;
	}

	return -1;
}

//r이 걸쳐있는 모니터 인덱스를 리턴. 겹쳐지는 영역이 어디에도 없다면 -1을 리턴.
int	get_monitor_index(CRect r, bool entire_included)
{
	enum_display_monitors();

	for (int i = 0; i < g_monitors.size(); i++)
	{
		if (entire_included)
		{
			if (RectInRect(g_monitors[i].rMonitor, r))
				return i;
		}
		else if (r.IntersectRect(r, g_monitors[i].rMonitor))
		{
			return i;
		}
	}

	return -1;
}

//멀티모니터 전체 영역 사각형 리턴
CRect get_entire_monitor_rect()
{
	enum_display_monitors();
	CRect rEntire(0, 0, 0, 0);

	for (int i = 0; i < g_monitors.size(); i++)
	{
		rEntire.left = MIN(rEntire.left, g_monitors[i].rMonitor.left);
		rEntire.top = MIN(rEntire.top, g_monitors[i].rMonitor.top);
		rEntire.right = MAX(rEntire.right, g_monitors[i].rMonitor.right);
		rEntire.bottom = MAX(rEntire.bottom, g_monitors[i].rMonitor.bottom);
	}

	return rEntire;
}

void SetForegroundWindowForce(HWND hWnd, bool makeTopMost)
{
	//최상위로 올리는 코드가 이 함수의 맨 처음 부분에서 실행되지 않으면
	//다른 최상위로 실행된 창의 위로 항상 올라오지 않는 현상이 있다.
	if (makeTopMost)
		::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	if (IsIconic(hWnd))
		::ShowWindow(hWnd, SW_RESTORE);

	HWND hWndForeground = ::GetForegroundWindow();
	if (!hWndForeground || hWndForeground == hWnd)
		return;

	DWORD Strange = ::GetWindowThreadProcessId(hWndForeground, NULL);
	DWORD My = ::GetWindowThreadProcessId(hWnd, NULL);

	if (!::AttachThreadInput(Strange, My, TRUE))
	{
		//ASSERT(0);
		OutputDebugString(_T("AttachThreadInput(Strange, My, TRUE) failed.\n"));
	}

	::SetForegroundWindow(hWnd);
	//::BringWindowToTop(hWnd);
	//::SetFocus(hWnd);

	if (!::AttachThreadInput(Strange, My, FALSE))
	{
		//ASSERT(0);
		OutputDebugString(_T("AttachThreadInput(Strange, My, FALSE) failed.\n"));
	}
}

bool is_top_most(HWND hWnd)
{
	DWORD dwExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
	return ((dwExStyle & WS_EX_TOPMOST) != 0);
}

//2D 단일 영상에서 이미 알려진 설정값을 기준으로 영상내의 한 점과 렌즈와의 거리를 계산(by sucwon)
//단, 차량에 장착된 카메라에서 촬영된 영상이므로 피사체와 렌즈와의 거리가 아닌 차체와의 거리가 더 정확한 값이며
//따라서 렌즈의 위치인 영상 하단 센터가 아닌 영상의 하단과 피사체의 수직 거리가
//오히려 더 정확한 계산이 된다.
double getObjectDistance(int width, int height, int vanishing_y, int x, int y, int cam_height, double *dx, double *dy, int cali_width, int cali_height, double fl_x, double fl_y)
{
		//// Focal Length (4개의 상수값은 협각 기준이며 카메라마다 다른 값을 가짐)
/*
		double fl_x = 2361.130;
		double fl_y = 2357.436;

		//// 주점
		double c_x = 1066.161;
		double c_y = 407.246;
*/
/*
		double tilt = atan2(vanishing_y - c_y, fl_y);				// 1번째 parameter 는 주점, 2번째 parameter는 Camera Focal Length
		double m_x = (x - c_x) / fl_x;								// 정규좌표 (x)
		double m_y = (y - c_y) / fl_y;								// 정규좌표 (y)

		double C1P1 = (double)cam_height * tan((PI/2) + tilt - atan(m_y));	// C`P`
		double CP1 = sqrt(((double)cam_height * (double)cam_height) + (C1P1*C1P1));					// CP`
		double Cp1 = sqrt(1 + (m_y*m_y));
		double PP1 = m_x * CP1 / Cp1;

		double real_distance = sqrt((C1P1*C1P1) + (PP1*PP1)) / 100.0f;

		return real_distance;
*/
	//카메라 캘리브레이션 앱을 통해서 직접 구해야 한다.
	//주점은 보통 입력 영상의 1/2이다.
	double c_x = (double)width / 2.0;	//691.448;
	double c_y = (double)height / 2.0;	//404.974;

	//1920, 1080은 캘리브레이션에 사용한 카메라 영상의 크기이다.
	double tilt = atan2((vanishing_y/(double)(height)*((double)(cali_height))) - c_y, fl_y);
	double csh = (double)cam_height / 10.0;                              // 카메라 설치높이
	double m_x = ((x / (double)(width) * (double)cali_width) - c_x) / fl_x;                        // 정규좌표 (x)
	double m_y = ((y / (double)(height)*(double)(cali_height)) - c_y) / fl_y;                        // 정규좌표 (y)

	double C1P1 = csh * tan((3.141592 / 2.0) + tilt - atan(m_y));   // C`P`
	double CP1 = sqrt((csh*csh) + (C1P1*C1P1));               // CP`
	double Cp1 = sqrt(1 + (m_y*m_y));
	double PP1 = m_x * CP1 / Cp1;
	*dx = PP1 / 100.0;
	*dy = C1P1 / 100.0;
	//TRACE(_T("C1P1 = %f, PP1 = %f\n"), *dx, *dy);

	return (sqrt((C1P1*C1P1) + (PP1*PP1)) / 100.0);
	//if(real_distance > 100) real_distance = 100;
}



//gray 계열인지
bool isGray(COLORREF cr, int tolerance)
{
	BYTE r = GetBValue(cr);
	BYTE g = GetGValue(cr);
	BYTE b = GetRValue(cr);

	if ((r >= MAX(0, g-tolerance)) && (r <= MIN(255, g+tolerance)) &&
		(g >= MAX(0, b-tolerance)) && (g <= MIN(255, b+tolerance)) &&
		(b >= MAX(0, r-tolerance)) && (b <= MIN(255, r+tolerance)))
		return true;

	return false;
}


/*
std::string string_format(const std::string fmt, ...)
{
    int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}
*/
std::string string_format(const std::string fmt_str, ...)
{
	int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
	std::string str;
	std::unique_ptr<char[]> formatted;
	va_list ap;
	while(1) {
		formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
		strcpy(&formatted[0], fmt_str.c_str());
		va_start(ap, fmt_str);
		final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
		va_end(ap);
		if (final_n < 0 || final_n >= n)
			n += abs(final_n - n + 1);
		else
			break;
	}
	return std::string(formatted.get());
}

CString		loadResString(UINT nID)
{
	CString str;
	str.LoadString(nID);
	return str;
}

void make_gaussian_mask(float sigma, int *mask, int *divisor)
{
	int x,y;
	float coef;
	float value[9];

	// get values from 2D -Gaussian function
	coef = 1/(2*PI*sigma*sigma);
	
	for(y=0; y < 3; y++)
	{
		for(x=0; x < 3; x++)
		{
			 value[y*3+x] = (float)(coef * exp(-((x-1)*(x-1)+ (y-1)*(y-1))/(2*sigma*sigma)));
		}
	}

	*divisor = 0;

	// intergerization
	for(y=0; y < 3; y++)
	{
		for(x=0; x < 3; x++)
		{
			 mask[y*3+x] = (int)(value[y*3+x] / value[0] +0.5f);
			 *divisor += mask[y*3+x];
		}
	}

}


void gaussian_blur(uint8_t *image, int width, int height)
{
	int x,y;
	uint8_t *first_line;
	uint8_t  *second_line;
	uint8_t  *third_line;
	uint8_t  *buf, *buf_pos;
//	float sigma[ROI_NUM]={  0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
//												0.5f, 0.5f, 0.5f, 0.5f, 0.5f};

	int  GM[9];
	int  divisor=0;
	

	//make_gaussian_mask(sigma[image_index], GM,&divisor);

	make_gaussian_mask(0.5f, GM,&divisor);

	buf = (uint8_t *)malloc(width*height);

	first_line = image; // a pointer to the first row
	second_line = first_line+ width; // a pointer to the second row
	third_line = second_line + width; // a pointer to the third row

	buf_pos=buf+width+1;

	for(y=0; y<(height-2); y++)
	{
		for(x=0; x<(width-2); x++)
		{
 		     *(buf_pos++) = (			first_line[0]	+ GM[1] * first_line[1]	+			first_line[2] +
			 					GM[3] * second_line[0]	+ GM[4] * second_line[1]+  GM[5] *	second_line[2] +
										third_line[0]	+ GM[7] * third_line[1] +			third_line[2])  / divisor;
			first_line++;
			second_line++;
			third_line++;
		}

		first_line+=2;
		second_line+=2;
		third_line+=2;
		buf_pos+=2;

	}

	for(x=0; x < width ; x++)
	{
		buf[x] = image[x];
		buf[(height-1)*width+x] = image[(height-1)*width+x];
	}

	for(y=0; y < height ; y++)
	{
		buf[y*width] = image[y*width];
		buf[y*width+width-1] = image[y*width+width-1];
	}

	memcpy(image,buf,width*height);
	free(buf);
}

/*
void create_integral_image(uchar*	source_img, 
											   int source_width,
											   int source_height,
											   FeatureInfo*	feature_info)
{
	int x, y;
	int s;
	int *integral_img_upper_line;
	int *integral_img_current_line;
	uchar *source_ptr;
	int *integral_img = feature_info->integral_img;
	int integral_width_step = feature_info->integral_img_step;

	feature_info->integral_img_size.width = source_width+1;
	feature_info->integral_img_size.height = source_height+1;

	memset(integral_img, 0x00, integral_width_step*sizeof(int)); // to initialize the first line.
	integral_img_upper_line = integral_img + 1; // coordinate (1,0)
	integral_img_current_line = integral_img + integral_width_step + 1; //coordinate (1,1)

	source_ptr = source_img;

	for(y = 0; y < source_height; y++)
	{
		integral_img_current_line[-1] = 0;	//	가장 왼쪽 열을 0으로 셋팅

		for(s=0, x = 0; x < source_width; x++)
		{
			s += source_ptr[x]; 
			integral_img_current_line[x] = integral_img_upper_line[x] + s; // just upper line
		}

		source_ptr += source_width;
		integral_img_upper_line += integral_width_step;
		integral_img_current_line += integral_width_step;
	}
}
*/

void resize_image(uint8_t *source_ptr,
					int source_width, 
					int source_height, 
					uint8_t *destination_ptr, 
					int destination_width, 
					int destination_height)
{
	if (source_width == destination_width && source_height == destination_height)
	{
		memcpy(destination_ptr, source_ptr, source_width * source_height);
		return;
	}

	memset(destination_ptr, 0, destination_width * destination_height);

	int src_x;	//	x coordinate of source
	int next_src_x;	//	x+1 coordinate of source
	int float_src_x;		//	modified x coordinate of source which has decimal data using "<<"
	int x,y; 

	int *src_x_buf = (int*) new int[source_width];	//	buffer made up of x coordinates of source
	int *alpha_buf = (int*) new int[source_width];	//	buffer of alpha

	int src_y;			//	y coordinate of source
	int float_src_y;	//	//	modified x coordinate of source which has decimal data using "<<"

	int ratio_height, ratio_width;		//	ratio between roi and destination size using "<<"
	uint8_t *destination_dup_ptr ;		//	duplicated pointer of destination
	uint8_t *source_current_line_ptr, *source_nextline_ptr;	//	duplicated pointers of source
	int r0c0, r0c1, r1c0, r1c1;			//	f(x,y), f(x+1,y), f(x,y+1), f(x+1,y+1)
	int alpha,beta;

	int inverse_alpha,inverse_beta;	//	inverse (alpha or beta) = 2048 - (alpha or beta)

	if(destination_width<=0||destination_height<=0) return ;

	ratio_width  = (source_width <<11) / destination_width;		//ifloatRatioWidth  = nSrcWidth * 2048 / nDstWidth;
	ratio_height = (source_height<< 11) /destination_height;		//ifloatRatioHeight = nSrcHeight * 2048 /nDstHeight;

	for(x=0; x<destination_width-1; x++)
	{
		float_src_x = x * ratio_width;	// x coordinate of source which has decimal data mapped from destination 
		alpha_buf[x] = float_src_x&0x07ff;

		src_x_buf [x]= float_src_x>>11;		 // x coordinate of source mapped from destination 
	}

	destination_dup_ptr = destination_ptr;

	for(y =0; y<destination_height-1; y++)
	{
		float_src_y=  y * ratio_height;
		src_y = float_src_y>>11;		 // y coordinate of source mapped from destination 

		source_current_line_ptr = source_ptr + src_y * source_width;	//	current line of source
		source_nextline_ptr = source_current_line_ptr + source_width;	//	next line of source

		beta=float_src_y&0x07ff;		
		inverse_beta = 2048 - beta;

		for(x=0; x<destination_width-1; x++)  
		{
			src_x = src_x_buf[x];	//	currnet x of source
			next_src_x = src_x+1;	//	next x of source

			r0c0=source_current_line_ptr[src_x];
			r0c1=source_current_line_ptr[next_src_x];
			r1c0=source_nextline_ptr[src_x];
			r1c1=source_nextline_ptr[next_src_x];

			alpha= alpha_buf[x];
			inverse_alpha = 2048-alpha;

			destination_dup_ptr[x] = (uint8_t)((((inverse_beta * ((inverse_alpha*r0c0 + alpha*r0c1)>> 4)) >> 16) + ((beta * ((inverse_alpha*r1c0 + alpha*r1c1) >> 4)) >> 16))>>2);
		}
		destination_dup_ptr+=destination_width;
	}

	delete [] src_x_buf;
	delete [] alpha_buf;
}

//https://stackoverrun.com/ko/q/200236
//아직 테스트 하지 않음. 축소만 가능? 확대, 축소 모두 가능? skip이 있는걸로 봐서 축소만 가능할듯하다.
void scaleImage(uint8_t*src, uint8_t*dest, int srcWidth, int srcHeight, int destWidth, int destHeight)
{
	//these are internal counters
	int srcx = 0;
	int srcy = 0;
	int skipx;
	int skipy;

	skipx = (destWidth>>8)/srcWidth;
	skipy = (destHeight>>8)/srcHeight;

	for(int y=0; y<destHeight; y++)
	{
		//calc from which y coord we need to copy pixel
		uint8_t*src2 = src + ((srcy>>8)*srcWidth*3);

		for(int x=0; x<destWidth; x++)
		{
			//calc from which x coord we need to copy pixel
			uint8_t*src3 = src2 + ((srcx>>8)*3);

			//copy rgb
			*dest++ = *src3++;
			*dest++ = *src3++;
			*dest++ = *src3++;

			//go to next x pixel
			srcx += skipx;
		}

		//go to next y pixel
		srcy += skipy;
	}
}

void resize_ROI(uint8_t *source_ptr,
							int source_width, 
							int source_height, 
							int x_roi, 
							int y_roi, 
							int w_roi, 
							int h_roi, 
							uint8_t *destination_ptr, 
							int destination_width, 
							int destination_height)
{
	int src_x;	//	x coordinate of source
	int next_src_x;	//	x+1 coordinate of source
	int float_src_x;		//	modified x coordinate of source which has decimal data using "<<"
	int x,y; 

	int src_x_buf[640];	//	buffer made up of x coordinates of source
	int alpha_buf[640];	//	buffer of alpha

	int src_y;			//	y coordinate of source
	int float_src_y;	//	//	modified x coordinate of source which has decimal data using "<<"

	int ratio_height, ratio_width;		//	ratio between roi and destination size using "<<"
	uint8_t *destination_dup_ptr ;		//	duplicated pointer of destination
	uint8_t *source_current_line_ptr, *source_nextline_ptr;	//	duplicated pointers of source
	int r0c0, r0c1, r1c0, r1c1;			//	f(x,y), f(x+1,y), f(x,y+1), f(x+1,y+1)
	int alpha,beta;

	int inverse_alpha,inverse_beta;	//	inverse (alpha or beta) = 2048 - (alpha or beta)

	if(destination_width<=0||destination_height<=0||w_roi<=0||h_roi<=0) return ;

	ratio_width  = (w_roi <<11) / destination_width;		//ifloatRatioWidth  = nSrcWidth * 2048 / nDstWidth;
	ratio_height = (h_roi<< 11) /destination_height;		//ifloatRatioHeight = nSrcHeight * 2048 /nDstHeight;

	y_roi<<=11;
	x_roi<<=11;

	for(x=0; x<destination_width-1; x++)
	{
		float_src_x = x * ratio_width + x_roi;	// x coordinate of source which has decimal data mapped from destination 
		alpha_buf[x] = float_src_x&0x07ff;

		src_x_buf [x]= float_src_x>>11;		 // x coordinate of source mapped from destination 
	}

	destination_dup_ptr = destination_ptr;

	for(y =0; y<destination_height-1; y++)
	{
		float_src_y=  y * ratio_height + y_roi;
		src_y = float_src_y>>11;		 // y coordinate of source mapped from destination 

		source_current_line_ptr = source_ptr + src_y * source_width;	//	current line of source
		source_nextline_ptr = source_current_line_ptr + source_width;	//	next line of source

		beta=float_src_y&0x07ff;		
		inverse_beta = 2048 - beta;

		for(x=0; x<destination_width-1; x++)  
		{
			src_x = src_x_buf[x];	//	currnet x of source
			next_src_x = src_x+1;	//	next x of source

			r0c0=source_current_line_ptr[src_x];
			r0c1=source_current_line_ptr[next_src_x];
			r1c0=source_nextline_ptr[src_x];
			r1c1=source_nextline_ptr[next_src_x];

			alpha= alpha_buf[x];
			inverse_alpha = 2048-alpha;

			destination_dup_ptr[x] = (uint8_t)((((inverse_beta * ((inverse_alpha*r0c0 + alpha*r0c1)>> 4)) >> 16) + ((beta * ((inverse_alpha*r1c0 + alpha*r1c1) >> 4)) >> 16))>>2);
		}
		destination_dup_ptr+=destination_width;
	}
}

//gray image를 3채널 또는 4채널 이미지 포맷으로 복사해준다.
void gray2color(uint8_t *gray, int gray_width, int gray_height, uint8_t *dst, int dst_width, int dst_height, int dst_ch, uint8_t alpha)
{
	int x, y, k;
	uint8_t *src = gray;
	bool resized = false;

	//gray와 dst의 크기가 다르면 resize한 후 복사해야 한다.
	if (gray_width != dst_width || gray_height != dst_height)
	{
		resized = true;
		src = (uint8_t*)malloc(dst_width * dst_height);
		resize_image(gray, gray_width, gray_height, src, dst_width, dst_height);
		gray_width = dst_width;
		gray_height = dst_height;
	}


	for (y = 0; y < gray_height; y++)
	{
		for (x = 0; x < gray_width; x++)
		{
			for (k = 0; k < dst_ch; k++)
			{
				if (dst_ch == 4 && k == dst_ch-1)
					*(dst + y * dst_width * dst_ch + x * dst_ch + k) = alpha;
				else
					*(dst + y * dst_width * dst_ch + x * dst_ch + k) = *(src + y * gray_width + x);
			}
		}
	}

	if (resized)
		free(src);
}

//3채널 또는 4채널 이미지를 단일 흑백 이미지로 복사한다.
//dst는 반드시 할당된 메모리주소이어야 한다.
void color2gray(uint8_t *src, int src_width, int src_height, int src_ch, uint8_t *dst, int dst_width, int dst_height)
{
	if (src_ch < 3 || src_ch > 4)
		return;

	int x, y;
	int r, g, b;
	uint8_t *gray = dst;
	bool resized = false;
	
	//src와 dst의 크기가 다르면 gray로 변환한 후 resize해줘야 한다.
	if (src_width != dst_width || src_height != dst_height)
	{
		resized = true;
		gray = (uint8_t*)malloc(src_width * src_height * 3);
		//resize_image1(gray, gray_width, gray_height, src, dst_width, dst_height);
		//gray_width = dst_width;
		//gray_height = dst_height;
	}
	

	for (y = 0; y < src_height; y++)
	{
		for (x = 0; x < src_width; x++)
		{
			b = *(src + y * src_width * src_ch + x * src_ch + 0);
			g = *(src + y * src_width * src_ch + x * src_ch + 1);
			r = *(src + y * src_width * src_ch + x * src_ch + 2);

			if ((b == g) && (g == r))
				*(gray + y * src_width + x) = b;
			else
				*(gray + y * src_width + x) = RGB2Y(r, g, b);
		}
	}

	if (resized)
	{
		resize_image(gray, src_width, src_height, dst, dst_width, dst_height);
		free(gray);
	}
}

double scv_image_mean(uint8_t *src, int width, int height)
{
	int i, j;
	double sum = 0.0;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			sum += *(src + i * width + j); 
		}
	}
	
	return sum / (double)(width*height);
}

void scv_image_threshold(uint8_t *src, int width, int height, int threshold, int minValid, bool invert)
{
	int i, j;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if ((*(src + i * width + j) >= minValid) &&
				 (*(src + i * width + j) >= threshold))
				*(src + i * width + j) = (invert ? 0 : 255);
			else
				*(src + i * width + j) = (invert ? 255 : 0);
		}
	}
}

void scv_absdiff(uint8_t*src1, uint8_t*src2, uint8_t*dst, int w, int h)
{
	int i, j;

	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			dst[i * w + j] = abs(src1[i * w + j] - src2[i * w + j]);
		}
	}
}

bool scv_subImage(uint8_t* src, uint8_t* dst, int source_width, int source_height, int startx, int starty, int cut_width, int cut_height, int ch)
{
   int x, y, c;

   if ((startx < 0) || (startx + cut_width - 1 >= source_width))
	   return false;

   if ((starty < 0) || (starty + cut_height - 1 >= source_height))
	   return false;

   for(y = 0; y < cut_height; y++)
   {
      for(x = 0; x < cut_width; x++)
	  {
         for(c = 0; c < ch; c++)
		 {
            dst[(y * cut_width + x) * ch + c] = src[((starty + y) * source_width + (startx + x)) * ch + c];
		 }
	  }
   }

   return true;
}

int scv_countNonZero(uint8_t*src, int w, int h)
{
	int i, j;
	int count = 0;

	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			if (src[i * w + j])
				count++;
		}
	}

	return count;
}

//현재는 1채널 영상만 지원된다.
//우선 급하게 -90도만 구현한다.
void rotate90(uint8_t *src, int width, int height, uint8_t *dst, int degree)
{
	if (dst == NULL)
		return;

	int x, y;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			*(dst + (width - x - 1) * height + y) = *(src + y * width + x);
		}
	}
}

//opencv의 flip과 같은 동작이지만 이름 충돌을 피하기 위해 mirror라는 이름을 사용함.
//현재는 1채널 영상만 지원된다.
//method : 0(flip vertical), +(flip horizontal), -(both)
void mirror(uint8_t *src, int width, int height, uint8_t *dst, int method)
{
	if (dst == NULL)
		return;

	int x, y;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			if (method == 0)
			{
				*(dst + (height - y - 1) * width + x) = *(src + y * width + x);
			}
			else if (method > 0)
			{
				*(dst + y * width + (width - x - 1)) = *(src + y * width + x);
			}
			else
			{
				*(dst + (height - y - 1) * width + (width - x - 1)) = *(src + y * width + x);
			}
		}
	}
}

double getOverlappedRatio(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
	int x, y, w, h;
	x = y = w = h = 0;

	if (x1 > x2 + w2) return 0.0;
    if (x1 + w1 < x2) return 0.0;
    if (y1 > y2 + h2) return 0.0;
    if (y1 + h1 < y2) return 0.0;

	x = MAX(x1, x2);
    y = MAX(y1, y2);
    w = MIN(x1 + w1, x2 + w2) - x2;
    h = MIN(y1 + h1, y2 + h2) - y2;

	if (w1 == 0 || h1 == 0)
		return 0.0;

	return ((double)(w * h) / (double)(w1 * h1));
}

CRect subtract(CRect r0, CRect r1)
{
	CRect r;
	r.left = r0.left - r1.left;
	r.top = r0.top - r1.top;
	r.right = r0.right - r1.right;
	r.bottom = r0.bottom - r1.bottom;
	return r;
}

//스크린에 표시된 이미지에 그려진 사각형의 실제 이미지상의 사각형 좌표
//sr : 이미지에 그려진 사각형
//displayed : 이미지가 표시되고 있는 사각형 영역
//real : 실제 이미지의 크기
//resized : zoom in/out에 의해 변경된 크기
CRect get_real_from_screen_coord(CRect sr, CRect displayed, CSize real, CSize resized)
{
	CRect r;
	return r;
}

CRect getIntersectionRect(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
	CRect r;

	r.SetRectEmpty();

	if (x1 > x2 + w2) return r;
    if (x1 + w1 < x2) return r;
    if (y1 > y2 + h2) return r;
    if (y1 + h1 < y2) return r;

	r.left = MAX(x1, x2);
    r.top = MAX(y1, y2);
    r.right = MIN(x1 + w1, x2 + w2);
    r.bottom = MIN(y1 + h1, y2 + h2);

	return r;
}

//두 사각형의 겹치는 영역을 리턴한다.
CRect getIntersectionRect(CRect r1, CRect r2)
{
	CRect	r(0, 0, 0, 0);
	
	if (r1.left > r2.left + r2.Width()) return r;
    if (r1.left + r1.Width() < r2.left) return r;
    if (r1.top > r2.top + r2.Height()) return r;
    if (r1.top + r1.Height() < r2.top) return r;

	r.left = MAX(r1.left, r2.left);
    r.top = MAX(r1.top, r2.top);
    r.right = MIN(r1.left + r1.Width(), r2.left + r2.Width());
    r.bottom = MIN(r1.top + r1.Height(), r2.top + r2.Height());

	return r;
}

double tangentfunc(double x, double y)
{
	const double constant = 58.8235294117647f;
	const double lookup_table[59] = 
	{
		0.0, //0

		1.0, //1
		2.0,
		3.0,
		4.0,
		5.0,
		6.0,
		7.0,
		8.0,
		9.0,
		10.0, //10

		11.0, //11
		12.0,
		13.0,
		14.0,
		15.0,
		16.0,
		17.0,
		17.5,
		18.0,
		19.0, 

		20.0, //21
		21.0,
		22.0,
		23.0,
		23.5,
		24.0,
		25.0,
		26.0,
		27.0,
		27.5,

		28.0, //31
		29.0, 
		30.0, 
		30.5, 
		31.0,
		32.0,
		32.5,
		33.0,
		34.0,
		34.5, 

		35.0, //41
		36.0,
		36.5,
		37.0,
		38.0,
		38.5,
		39.0,
		39.5,
		40.0,
		40.5,

		41.0, //51
		42.0,
		42.5,
		43.0,
		43.5,
		44.0,
		44.5,
		45.0
	};

	//↑ 전역 상수

	const double xv = fabs(x);
	const double yv = fabs(y);
	double degree = 0.0;
	unsigned char a=1;
	unsigned char b=0;



	if(xv > yv)
		b++;

	if(x<0.0)
		a++;
	if(y<0.0)
		a+=2;

	switch (a)
	{
	case 1:
		if(b)
			degree =      + lookup_table[(int)((yv/xv) * constant)];
		else
			degree = 90.0   - lookup_table[(int)((xv/yv) * constant)];
		return degree;
	case 2:
		if(b)
			degree = 180.0   - lookup_table[(int)((yv/xv) * constant)];
		else
			degree = 90.0   + lookup_table[(int)((xv/yv) * constant)];
		return degree;
	case 3:
		if(b)
			degree = 270.0   + lookup_table[(int)((yv/xv) * constant)];
		else
			degree = 360.0   - lookup_table[(int)((xv/yv) * constant)];
		return degree;
	case 4:
		if(b)
			degree = 180.0   + lookup_table[(int)((yv/xv) * constant)];
		else
			degree = 270.0   - lookup_table[(int)((xv/yv) * constant)];
		return degree;
	}

	return degree;
}

void drawCircle(CDC* pDC, int xMidPoint,  int yMidPoint,  int radius)
{
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;
	COLORREF cr = RGB(255,255,0);
	
	//Bottom middle
	pDC->SetPixel(CPoint(xMidPoint, yMidPoint + radius), cr);
	//Top Middle
	pDC->SetPixel(CPoint(xMidPoint, yMidPoint - radius), cr);
	//Right Middle
	pDC->SetPixel(CPoint(xMidPoint + radius, yMidPoint), cr);
	//Left Middle
	pDC->SetPixel(CPoint(xMidPoint - radius, yMidPoint), cr);
	
	while(x < y)
	{        
		if(f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		
		x++;
		ddF_x += 2;
		f += ddF_x;
		
		//Lower Right
		pDC->SetPixel(CPoint(xMidPoint + x, yMidPoint + y), cr);
		pDC->SetPixel(CPoint(xMidPoint + y, yMidPoint + x), cr);
		
		//Lower Left
		pDC->SetPixel(CPoint(xMidPoint - x, yMidPoint + y), cr);
		pDC->SetPixel(CPoint(xMidPoint - y, yMidPoint + x), cr);
		//Top Right
		pDC->SetPixel(CPoint(xMidPoint + x, yMidPoint - y), cr);
		pDC->SetPixel(CPoint(xMidPoint + y, yMidPoint - x), cr);
		
		//Top Left
		pDC->SetPixel(CPoint(xMidPoint - x, yMidPoint - y), cr);
		pDC->SetPixel(CPoint(xMidPoint - y, yMidPoint - x), cr);
	}
}

void drawArc(CDC *pDC, double cx, double cy,double r1, double r2, double start, double end, int width, int style, COLORREF cr, int mode)
{
	LOGBRUSH lb;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = cr;

	CPen	Pen(PS_GEOMETRIC | style, width, &lb);
	CPen*	pOldPen = (CPen*)pDC->SelectObject(&Pen);
	int		nOldDrawMode = pDC->SetROP2(mode);

	double degree;
	double deg_to_rads = 0.0174532925;
	double dx, dy;

	if (start > end)
		std::swap(start, end);

	// set up loop to draw arc
	for (int i = start; i < end - 1; i++)
	{
		degree = i * deg_to_rads;
		dx = cx + (r1 * cos(degree));
		dy = cy - (r2 * sin(degree));

		if (i == (int)start)
			pDC->MoveTo(dx, dy);
		else
			pDC->LineTo(dx, dy);
	}

	pDC->SelectObject(pOldPen);
	Pen.DeleteObject();

	pDC->SetROP2(nOldDrawMode);
}

HICON load_icon(HINSTANCE hInstance, UINT nID, int cx, int cy /*= 0*/)
{
	if (cy == 0)
		cy = cx;

	if (hInstance == NULL)
		hInstance = AfxGetInstanceHandle();

	//아래 LoadImage 함수를 통해서 아이콘 파일을 불러온 경우
	//프로그램 종료 시 반드시 DestroyIcon을 해줘야 한다.
	//그래서 DestroyIcon 안해도 되는 LR_SHARED를 사용했다.
	return static_cast<HICON>(::LoadImage(hInstance, MAKEINTRESOURCE(nID), IMAGE_ICON, cx, cy, LR_SHARED));
}

CSize draw_icon(CDC* pDC, HICON hIcon, CRect r)
{
	if (hIcon == NULL)
		return CSize(0, 0);

	//int cxIcon = GetSystemMetrics(SM_CXICON);
	//int cyIcon = GetSystemMetrics(SM_CYICON);

	//SM_CXICON을 이용해서 기본값으로 그리는게 아니라
	//실제 hIcon의 크기를 구해서
	int w;
	int h;

	ICONINFO info;
	ZeroMemory(&info, sizeof(info));

	BITMAP bmp;
	ZeroMemory(&bmp, sizeof(bmp));

	GetIconInfo(hIcon, &info);

	if (info.hbmColor)
	{
		const int nWrittenBytes = GetObject(info.hbmColor, sizeof(bmp), &bmp);
		if (nWrittenBytes > 0)
		{
			w = bmp.bmWidth;
			h = bmp.bmHeight;
			//myinfo.nBitsPerPixel = bmp.bmBitsPixel;
		}
	}
	else if (info.hbmMask)
	{
		// Icon has no color plane, image data stored in mask
		const int nWrittenBytes = GetObject(info.hbmMask, sizeof(bmp), &bmp);
		if (nWrittenBytes > 0)
		{
			w = bmp.bmWidth;
			h = bmp.bmHeight / 2;
			//myinfo.nBitsPerPixel = 1;
		}
	}

	if (info.hbmColor)
		DeleteObject(info.hbmColor);
	if (info.hbmMask)
		DeleteObject(info.hbmMask);

	int x = r.left + (r.Width() - w + 1) / 2;
	int y = r.top + (r.Height() - h + 1) / 2;

	::DrawIconEx(pDC->GetSafeHdc(), x, y, hIcon, w, h, 0, NULL, DI_NORMAL);

	return CSize(w, h);
}

//font size to LOGFONT::lfHeight
LONG get_pixel_size_from_font_size(HWND hWnd, int font_size)
{
	HDC hDC = ::GetDC(hWnd);
	LONG size = -MulDiv(font_size, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	::DeleteDC(hDC);

	return size;
}

//LOGFONT::lfHeight to font size
LONG get_font_size_from_pixel_size(HWND hWnd, int logical_size)
{
	HDC hDC = ::GetDC(hWnd);
	LONG size = -MulDiv(logical_size, 72, GetDeviceCaps(hDC, LOGPIXELSY));
	::DeleteDC(hDC);

	return size;
}

//메모리 일부 복사 함수
//단, src의 데이터가 연속된 메모리상에 저장되어 있어야 하는 제약이 있다.
//src의 일부 블럭을 복사. 테스트 필요함.
bool memcpy_block(uint8_t *src, int srcx, int srcy, int srcw, int srch, uint8_t *dst, int dstx, int dsty, int dstw, int dsth, int w, int h, int ch)
{
	int x, y, c;

	if (srcx + w > srcw)
		return false;
	if (srcy + h > srch)
		return false;

	if (dstx + w > dstw)
		return false;
	if (dsty + h > dsth)
		return false;

	uint8_t *p, *q;

	for (y = 0; y < h; y++)
	{
		p = (uint8_t*)(src + (srcy + y) * srcw * ch + srcx * ch);
		q = (uint8_t*)(dst + (dsty + y) * dstw * ch + dstx * ch);

		for (x = 0; x < w; x++)
		{
			for (c = 0; c < ch; c++)
			{
				*(q + x * ch + c) = *(p + x * ch + c);
			}
		}
	}

	return true;
}

bool memcpy_block(uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, int ch, uint8_t *dst)
{
	for (int y = 0; y < h_roi; y++)
	{
		memcpy((uint8_t*)(dst + y * w_roi * ch), (uint8_t*)(src + (y_roi + y) * src_width * ch + (x_roi * ch)), w_roi * ch);
	}

	return true;
}

//src에서 roi 영역을 잘라서 dst_width * dst_height 크기로 resize 시킨다.
bool resize_roi(uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, uint8_t *dst, int dst_width, int dst_height)
{
	uint8_t *tmp = (uint8_t*)malloc(w_roi * h_roi);

	memcpy_block(src, src_width, src_height, x_roi, y_roi, w_roi, h_roi, 1, tmp);

	if ((dst_width == w_roi) && (dst_height == h_roi))
		memcpy(dst, tmp, w_roi * h_roi);
	else
		resize_image(tmp, w_roi, h_roi, dst, dst_width, dst_height);

	free(tmp);

	return true;
}

//출처: http://boongubbang.tistory.com/255 [Boongubbang]
//unicode를 지원하는 함수라서 테스트 미완!
/*
void NewTypingMessageFromCodePage(TCHAR* Message, UINT CodePage=0)  
{  
    TCHAR Word[2];  
    TCHAR WordCode[64];  
    char MultiByte[64];  
  
    static const BYTE NumCode[10]={0x2D, 0x23, 0x28, 0x22, 0x25, 0x0C, 0x27, 0x24, 0x26, 0x21};  
    int Length = wcslen(Message);  
  
    for(int i=0; i<Length; i++)  
    {  
        Word[0] = Message[i];  
        Word[1] = L'\0';  
        WideCharToMultiByte(CodePage, 0, Word, -1, MultiByte, 64, NULL, NULL);  
        _itow((int)(((~MultiByte[0])^0xff)<<8)+((~MultiByte[1])^0xff), WordCode, 10);  
        keybd_event(VK_MENU, MapVirtualKey(VK_MENU, 0), 0, 0);  
        for(int j=0; j<wcslen(WordCode); j++)  
        {  
            keybd_event(NumCode[(int)WordCode[j]-48], MapVirtualKey(NumCode[(int)WordCode[j]-48], 0), 0, 0);  
            keybd_event(NumCode[(int)WordCode[j]-48], MapVirtualKey(NumCode[(int)WordCode[j]-48], 0), KEYEVENTF_KEYUP, 0);  
        }  
        keybd_event(VK_MENU, MapVirtualKey(VK_MENU, 0), KEYEVENTF_KEYUP, 0);  
    }  
}  
*/

int swapByte1And3(int inValue)
{
    int swap = inValue & 0xFF;
    swap = swap << 16 | (inValue >> 16 & 0xFF);
    return inValue & 0xFF00FF00 | swap;
}

int convertBRGtoRBG(int inColor)
{
	return swapByte1And3(inColor);
}

int convertABRGtoRBGA(int inColor)
{
	int swap = inColor >> 24;
	inColor = convertBRGtoRBG(inColor) << 8;
	return inColor | swap;
}
/*
template<class T> void quicksort(T& v, int end, int start, bool bAscending)
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
*/

//OnInitDialog와 같이 최초 한번 실행되는 함수내에서 호출한다.
//기억된 좌표대로 복원하여 표시한다.
//단, 그 크기가 모니터 밖이면 CenterWindow()를,
//크기가 invalid하다면 원래 크기로 표시한다.
void RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection, bool use_maximize, bool resize_window)
{
	CRect	rc;
	CString sSection = _T("screen");

	//resize window인지 자동 체크하는 코드가 안먹히는듯하다.
	//bool is_resizable_window = ((::GetWindowLongPtr(pWnd->m_hWnd, GWL_STYLE) & WS_THICKFRAME) == WS_THICKFRAME);

	if (sSubSection != "")
		sSection = sSubSection + "\\screen";	//슬래시가 아닌 역슬래시를 써야 한다.

	rc.left		= pApp->GetProfileInt(sSection, _T("left"), 0);
	rc.top		= pApp->GetProfileInt(sSection, _T("top"), 0);
	rc.right	= pApp->GetProfileInt(sSection, _T("right"), 0);
	rc.bottom	= pApp->GetProfileInt(sSection, _T("bottom"), 0);

	if (rc.IsRectNull())
	{
		pWnd->CenterWindow();
		return;
	}

	//사각형 정보가 유효하고 그 크기도 10x10을 넘는다면 그 크기로 복원시키고
	if (rc.IsRectEmpty() == false && rc.Width() > 10 && rc.Height() > 10)
	{
		UINT_PTR flag = SWP_NOZORDER;
		if (!resize_window)
			flag |= SWP_NOSIZE;

		SetWindowPos(pWnd->m_hWnd, NULL, rc.left, rc.top, rc.Width(), rc.Height(), flag);
		//pWnd->MoveWindow(rc);
	}
	else
	{
		pWnd->MoveWindow(0, 0, 800, 600);
		pWnd->CenterWindow();
	}

	if (use_maximize && pApp->GetProfileInt(sSection, _T("maximized"), false))
		pWnd->ShowWindow(SW_SHOWMAXIMIZED);
}

//OnBnClickedCancel()과 같이 프로그램이 종료될 때 호출한다.
//OnWindowPosChanged()에서 매번 호출했었으나 
//OnInitDialog에서 RestoreWindowPosition를 호출할 때
//OnWindowPosChanged가 호출되면서 maximized 정보가 false로 초기화되어 버린다.
//sSubSection이 존재하면 그 이름에 "\\screen"을 붙여서 저장한다.
void SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection)
{
	if (!pWnd || !pWnd->m_hWnd || pWnd->IsWindowVisible() == false || pWnd->IsIconic())
	{
		TRACE(_T("[warning] SaveWindowPosition() just return because pWnd is invalid.\n"));
		return;
	}

	CRect	rc;
	CString sSection = _T("screen");

	if (sSubSection != "")
		sSection = sSubSection + "\\screen";	//슬래시가 아닌 역슬래시를 써야 한다.

	pWnd->GetWindowRect(rc);
	pApp->WriteProfileInt(sSection, _T("maximized"), pWnd->IsZoomed());
	
	if (!(pWnd->IsZoomed()) && !(pWnd->IsIconic()))
	{
		pApp->WriteProfileInt(sSection, _T("left"), rc.left);
		pApp->WriteProfileInt(sSection, _T("top"), rc.top);
		pApp->WriteProfileInt(sSection, _T("right"), rc.right);
		pApp->WriteProfileInt(sSection, _T("bottom"), rc.bottom);
	}
}


double YY[256], BU[256], GV[256], GU[256], RV[256];
unsigned char YUV_B[256][256];
unsigned char YUV_R[256][256];
unsigned char YUV_G[256][256][256];

void init_YUV_lookup_table()
{
	int i, j, k;
	double i_value;

	initialized_YUV_lookup_table = true;

	for(i=255; i>=0; i--)
	{
		YY[i] = (1.164*(i-16.0));
		BU[i] = (2.018*(i-128.0));
		GV[i] = (0.831*(i-128.0));
		GU[i] = (0.391*(i-128.0));
		RV[i] = (1.596*(i-128.0));
	}

	for(i=255; i>=0; i--)
	{
		for(j=255; j>=0; j--)
		{
			i_value = YY[i] + BU[j];

			if (i_value > 255) i_value=255;
			else if (i_value < 0) i_value=0;

			YUV_B[i][j]=(int)i_value;

			i_value = YY[i] + RV[j];

			if (i_value > 255) i_value=255;
			else if (i_value < 0) i_value=0;

			YUV_R[i][j]=(int)i_value;

			for(k=0; k<256; k++)
			{
				i_value = YY[i] - (GU[j] + GV[k]);

				if (i_value > 255) i_value=255;
				else if (i_value < 0) i_value=0;

				YUV_G[i][j][k] =(int)i_value;
			}
		}
	}
}

//init_YUV_lookup_table(); 함수를 반드시 한번 호출한 후 사용할 것.
void yuv420_yv12_to_bgr(unsigned char *src, unsigned char *dst, int w, int h)
{
	if (!initialized_YUV_lookup_table)
		init_YUV_lookup_table();

	int x,y;
	double imgsize = w*h;
	int w3 = w*3;
	double uvsize = imgsize/4.0;

	unsigned char *pY = src;
	unsigned char *pV = src + (int)imgsize;
	unsigned char *pU = src + (int)imgsize + (int)uvsize;

	int y00, y01, y10, y11;
	int u,v;    
	unsigned char *p;

	// 윈도우에서는 영상의 상하가 거꾸로 저장되지 때문에 아래와 같이 코드 작성.
	//mat을 사용하여 converting하는 경우 아래 코드가 뒤집힌다.
	/*
	for(y=0; y<=h-2; y+=2)
	{
		for(x=0; x<=w-2; x+=2)
		{
			p = out + w3*(h-y-1) + x*3;
			u = *pU;
			v = *pV;

			y00 = *pY;
			y01 = *(pY+1);
			y10 = *(pY+w);
			y11 = *(pY+w+1);

			*(p)        = YUV_B[y00][u];
			*(p+1)      = YUV_G[y00][u][v];
			*(p+2)      = YUV_R[y00][v];
			*(p+3)      = YUV_B[y01][u];
			*(p+3+1)    = YUV_G[y01][u][v];
			*(p+3+2)    = YUV_R[y01][v];

			*(p-w3)     = YUV_B[y10][u];
			*(p-w3+1)   = YUV_G[y10][u][v];
			*(p-w3+2)   = YUV_R[y10][v];
			*(p-w3+3)   = YUV_B[y11][u];
			*(p-w3+3+1) = YUV_G[y11][u][v];
			*(p-w3+3+2) = YUV_R[y11][v];

			pU++;
			pV++;

			pY = pY + 2;
		}
		pY = pY + w;
	}
	*/

	// 일반적인 경우 아래의 코드 사용함.
	for(y=0; y<=h-2; y+=2)
	{
		for(x=0; x<=w-2; x+=2)
		{
			p = dst + w3*y + x*3;
			u = *pU;
			v = *pV;

			y00 = *pY;
			y01 = *(pY+1);
			y10 = *(pY+w);
			y11 = *(pY+w+1);

			*(p)        = YUV_R[y00][v];
			*(p+1)      = YUV_G[y00][u][v];
			*(p+2)      = YUV_B[y00][u];
			*(p+3)      = YUV_R[y01][v];
			*(p+3+1)    = YUV_G[y01][u][v];
			*(p+3+2)    = YUV_B[y01][u];

			*(p+w3)     = YUV_R[y10][v];
			*(p+w3+1)   = YUV_G[y10][u][v];
			*(p+w3+2)   = YUV_B[y10][u];
			*(p+w3+3)   = YUV_R[y11][v];
			*(p+w3+3+1) = YUV_G[y11][u][v];
			*(p+w3+3+2) = YUV_B[y11][u];

			pU++;
			pV++;
			pY = pY + 2;
		}

		pY = pY + w;
	}
}
/*
//이 함수는 yv12용이 아닌것같다. 좀 더 확인해봐야 한다.
cv::Mat yuv420_yv12_to_bgr(uchar *pBuffer,long bufferSize, int width,int height)
{
    cv::Mat result(height,width,CV_8UC3);
    uchar y,cb,cr;

    long ySize = width*height;
    long uSize;
    uSize = ySize >> 2;

    assert(bufferSize == ySize + uSize * 2);

    uchar *output = result.data;
    uchar *pY = pBuffer;
    uchar *pU = pY + ySize;
    uchar *pV = pU + uSize;

    uchar r,g,b;

    for (int i = 0; i < uSize; ++i)
    {
        for(int j = 0; j < 4; ++j)
        {
            y = pY[i * 4 + j];
            cb = (uchar)pU[i];
            cr = (uchar)pV[i];

                //ITU-R standard
            b = saturate_cast<uchar>(y + 1.772*(cb-128));
            g = saturate_cast<uchar>(y - 0.344*(cb-128)-0.714*(cr-128));
            r = saturate_cast<uchar>(y + 1.402*(cr-128));

			COLOR_RANGE(b);
			COLOR_RANGE(g);
			COLOR_RANGE(r);

            *output++ = b;
            *output++ = g;
            *output++ = r;
        }
    }
    return result;
}
*/
void yuv420_nv12_to_gray(unsigned char* src, unsigned char* dst, int width, int height)
{  
	int frameSize = width * height;  

	for (int j = 0, yp = 0; j < height; j++)
	{
		int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;  
		for (int i = 0; i < width; i++, yp++)
		{  
			int y = (0xff & ((int) src[yp])) - 16;  
			if (y < 0)  
				y = 0;  
			dst[yp] = y;
		}  
	}  
}  

void yuv420_nv12_to_bgr(unsigned char* src, unsigned char* dst, int width, int height)
{  
	int frameSize = width * height;  

	for (int j = 0, yp = 0; j < height; j++)
	{
		int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;  
		for (int i = 0; i < width; i++, yp++)
		{  
			int y = (0xff & ((int) src[yp])) - 16;  
			if (y < 0)  
				y = 0;  
			if ((i & 1) == 0) {  
				v = (0xff & src[uvp++]) - 128;  
				u = (0xff & src[uvp++]) - 128;  
			}

			int y1192 = 1192 * y;  
			int r = (y1192 + 1634 * v) >> 10;  
			int g = (y1192 - 833 * v - 400 * u) >> 10;
			int b = (y1192 + 2066 * u) >> 10;  

			Clamp(b, 0, 255);
			Clamp(g, 0, 255);
			Clamp(r, 0, 255);

			dst[j*width*3 + i*3 + 0] = b;
			dst[j*width*3 + i*3 + 1] = g;
			dst[j*width*3 + i*3 + 2] = r;
		}  
	}  
}  

void yuv422_uyvy_to_bgr(unsigned char *src, unsigned char *dst, int w, int h)
{
	double y, u, v;
	int r, g, b;
	int index = 0;

	for (int i = 0; i < w * h * 3; i += 6)
	{
		y = src[index+1];
		u = src[index+0];
		v = src[index+2];
        b = (int)(y * 1. + (u-128.0) * 1.772);
        g = (int)(y * 1. + (u-128.0) *-0.34414 + (v-128.0)*-0.71414);
        r = (int)(y * 1. + (v-128.0)*1.402);

		if (i == 710 * 1920 * 3 + 940 * 3)
			printf("\n");

		//if (b < 0) b = 0;
		//else if (b > 255) b = 255;
		//if (g < 0) g = 0;
		//else if (g > 255) g = 255;
		//if (r < 0) r = 0;
		//else if (r > 255) r = 255;
		Clamp(b, 0, 255);
		Clamp(g, 0, 255);
		Clamp(r, 0, 255);

		//if ((y <= 50) && (g >= 240))
		//	printf("");

		dst[i+0] = (uint8_t)b;
		dst[i+1] = (uint8_t)g;
		dst[i+2] = (uint8_t)r;

		y = src[index+3];

        //b = y * 1. + (u-128.0) * 1.772;
        //g = y * 1. + (u-128.0) *-0.34414 + (v-128.0)*-0.71414;
        //r = y * 1. + (v-128.0)*1.402;
		b = (int)(y * 1. + (u-128.0) * 1.772);
		g = (int)(y * 1. + (u-128.0) *-0.34414 + (v-128.0)*-0.71414);
		r = (int)(y * 1. + (v-128.0)*1.402);

		//if (b < 0) b = 0;
		//else if (b > 255) b = 255;
		//if (g < 0) g = 0;
		//else if (g > 255) g = 255;
		//if (r < 0) r = 0;
		//else if (r > 255) r = 255;
		Clamp(b, 0, 255);
		Clamp(g, 0, 255);
		Clamp(r, 0, 255);

		if ((b <= 10) && (g == 255) && (r <= 10))
			printf("");

		dst[i+3] = (uint8_t)b;
		dst[i+4] = (uint8_t)g;
		dst[i+5] = (uint8_t)r;

		index += 4;
	}

/*
	for (int i = 0; i < w * h * 2; i += 4)
	{
		y = src[i+1];
		u = src[i+0];
		v = src[i+2];
        dst[index++] = (byte)(y * 1. + (u-128.0) * 1.772);
        dst[index++] = (byte)(y * 1. + (u-128.0) *-0.34414 + (v-128.0)*-0.71414);
        dst[index++] = (byte)(y * 1. + (v-128.0)*1.402);

		y = src[i+3];

        dst[index++] = (byte)(y * 1. + (u-128.0) * 1.772);
        dst[index++] = (byte)(y * 1. + (u-128.0) *-0.34414 + (v-128.0)*-0.71414);
        dst[index++] = (byte)(y * 1. + (v-128.0)*1.402);
	}
*/
}


bool yuv444_to_yuv420(BYTE* u, BYTE* v, BYTE *U, BYTE *V, int w, int h)
{
	int i, j;
	int value;
	int addr;
	int pos0, pos1, pos2, pos3;

	addr = 0;

	for (i=0; i<h; i+=2) for(j=0; j<w; j+=2)
	{
		pos0 = i*w + j;       pos1 = pos0 + 1;
		pos2 = pos0 + w;      pos3 = pos2 + 1;

		//오른쪽 쉬프트 연산 2번
		value = ((int)u[pos0]+(int)u[pos1]+(int)u[pos2]+(int)u[pos3])>>2;
		Clamp(value, 0, 255); 
		U[addr] = (BYTE)value;

		//즉 나누기 4 : 평균내기
		value = ((int)v[pos0]+(int)v[pos1]+(int)v[pos2]+(int)v[pos3])>>2;
		Clamp(value, 0, 255); 
		V[addr] = (BYTE)value;
		addr++;
	}
	return  true;
}

//테스트 필요함.
void yuv_yuyv_to_bgr(unsigned char* src, unsigned char* dst, int w, int h)
{
	const int K1 = int(1.402f * (1 << 16));
	const int K2 = int(0.714f * (1 << 16));
	const int K3 = int(0.334f * (1 << 16));
	const int K4 = int(1.772f * (1 << 16));
	const int pitch = w * 2; // 2 bytes per one YU-YV pixel

	for (int y=0; y<h; y++)
	{
		const unsigned char* src_data = src + pitch * y;

		for (int x=0; x<w*2; x+=4)	// Y1 U Y2 V
		{
			unsigned char Y1 = src_data[x + 0];
			unsigned char U  = src_data[x + 1];
			unsigned char Y2 = src_data[x + 2];
			unsigned char V  = src_data[x + 3];
			int uf = U - 128;
			int vf = V - 128;

			int R = Y1 + (K1*vf >> 16);
			int G = Y1 - (K2*vf >> 16) - (K3*uf >> 16);
			int B = Y1 + (K4*uf >> 16);

			Clamp(R, 0, 255); 
			Clamp(G, 0, 255); 
			Clamp(B, 0, 255); 

			dst[y*w+3 + x*3 + 0] = (unsigned char)R;
			dst[y*w+3 + x*3 + 1] = (unsigned char)G;
			dst[y*w+3 + x*3 + 2] = (unsigned char)B;

			R = Y2 + (K1*vf >> 16);
			G = Y2 - (K2*vf >> 16) - (K3*uf >> 16);
			B = Y2 + (K4*uf >> 16);

			Clamp(R, 0, 255); 
			Clamp(G, 0, 255);
			Clamp(B, 0, 255); 

			dst[y*w+3 + x*3 + 3] = (unsigned char)R;
			dst[y*w+3 + x*3 + 4] = (unsigned char)G;
			dst[y*w+3 + x*3 + 5] = (unsigned char)B;
		}
	}
}

typedef union {
	uint32_t yuv;
	uint8_t d[4];
} _yuv422pack;

typedef union {
	uint32_t bgra;
	uint8_t d[4];
} _cv8u3;

void bgr_to_yuv422(uint8_t* src, uint8_t* dst, int width, int height, bool yuyv)
{
	int32_t y0, y1, u0, v0;
	int32_t r1, g1, b1;
	int32_t r2, g2, b2;
	int32_t bytes;
	bytes = width * height * 3;

	uint32_t *src32;
	uint32_t *dst32 = (uint32_t *)dst;
	_yuv422pack yuv;
	_cv8u3 bgr0, bgr1;
	for (int32_t cnt = 0; cnt < bytes; cnt += 6)
	{
		src32 = (uint32_t *)(src + cnt + 0);
		bgr0.bgra = *src32;
		src32 = (uint32_t *)(src + cnt + 3);
		bgr1.bgra = *src32;
		b1 = (int32_t)bgr0.d[0];
		g1 = (int32_t)bgr0.d[1];
		r1 = (int32_t)bgr0.d[2];
		b2 = (int32_t)bgr1.d[0];
		g2 = (int32_t)bgr1.d[1];
		r2 = (int32_t)bgr1.d[2];

		y0 = ((66 * r1 + 129 * g1 + 25 * b1 + 128) >> 8) + 16;
		y1 = ((66 * r2 + 129 * g2 + 25 * b2 + 128) >> 8) + 16;

		u0 = ((-38 * r1 -  74 * g1 + 112 * b1 + 128) >> 8) + 128;
		v0 = ((112 * r1 -  94 * g1 -  18 * b1 + 128) >> 8) + 128;

		if (yuyv)
		{
			//y0
			yuv.d[0] = Clamp(y0, 0, 255);
			//y1
			yuv.d[2] = Clamp(y1, 0, 255);
			//u0
			yuv.d[1] = Clamp(u0, 0, 255);
			//v0
			yuv.d[3] = Clamp(v0, 0, 255);
		}
		else
		{
			//y0
			yuv.d[1] = Clamp(y0, 0, 255);
			//y1
			yuv.d[3] = Clamp(y1, 0, 255);
			//u0
			yuv.d[0] = Clamp(u0, 0, 255);
			//v0
			yuv.d[2] = Clamp(v0, 0, 255);
		}

		//store
		*dst32++ = yuv.yuv;
	}
}

//convert bgr or gray to yuv_plain_gray(YYYY....CbCb..CrCr..) Cb = Cr = 128 constant value
//dst must be allocated before calling.
//ex
//dst = new uint8_t[width * height * 3 / 2];
//gray_to_yuv_plain_gray(src, dst, width, height);
//...
//delete [] dst;
void gray_to_yuv_plain_gray(uint8_t *src, uint8_t *dst, int width, int height)
{
	uint32_t size = width * height;
	memcpy(dst, src, size);
	memset(dst + size, 128, size / 4);
	memset(dst + size + size / 4, 128, size / 4);
}

/* with uint8_t : faster but less accurate
void bgr_to_hsv(uint8_t* src, uint8_t* dst, int width, int height)
{
	uint8_t b, g, r, h, s, v;
	uint8_t bgrMin, bgrMax;
	int x, y;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width * 3; x += 3)
		{
			b = src[y * width * 3 + x + 0];
			g = src[y * width * 3 + x + 1];
			r = src[y * width * 3 + x + 2];

			bgrMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
			bgrMax = r > g ? (r > b ? r : b) : (g > b ? g : b);

			v = bgrMax;
			if (v == 0)
			{
				h = 0;
				s = 0;
			}
			else
			{
				s = 255 * long((double)(bgrMax - bgrMin) / (double)v);
				if (s == 0)
				{
					h = 0;
				}
				else
				{
					if (bgrMax == r)
						h = 0 + 43 * (double)(g-b)/(double)(bgrMax - bgrMin);
					else if (bgrMax == g)
						h = 85 + 43 * (double)(b-r)/(double)(bgrMax - bgrMin);
					else
						h = 171 + 43 * (double)(r-g)/(double)(bgrMax - bgrMin);
				}
			}

			dst[y * width * 3 + x + 0] = h;
			dst[y * width * 3 + x + 1] = s;
			dst[y * width * 3 + x + 2] = v;
		}
	}
}
*/
void bgr_to_hsv(uint8_t *src, uint8_t *dst, int w, int h)
{
	double r, g, b;
	double hsv_h, hsv_s, hsv_v;
	int index = 0;
	for (int i = 0; i < w * h * 3; i += 3)
	{
		b = src[index+0];
		g = src[index+1];
		r = src[index+2];

		double maxRGB, minRGB, delta;

		maxRGB = (r > g ? (r > b ? r : b) : (r > b ? g : (g > b ? g : b)));
		minRGB = (r < g ? (r < b ? r : b) : (r < b ? g : (g < b ? g : b)));
		delta = maxRGB - minRGB;

		hsv_v = maxRGB;

		if (hsv_v != 0) hsv_s = 255.0 * delta / hsv_v;
		else hsv_s = 0;

		if (hsv_s != 0) {
			if (maxRGB == r) hsv_h = (g - b) / hsv_s;
			if (maxRGB == g) hsv_h = 2.0 + ((b - r)) / hsv_s;
			if (maxRGB == b) hsv_h = 4.0 + ((r - g)) / hsv_s;
		}
		else {
			hsv_h = -1;
		}

		hsv_h *= 60;

		if (hsv_h < 0) hsv_h += 360;
		if (hsv_h != 0) hsv_h /= 2;

		dst[i+0] = (uint8_t)hsv_h; 
		dst[i+1] = (uint8_t)hsv_s; 
		dst[i+2] = (uint8_t)hsv_v; 

		index += 3;
	}
}

void printMessage(std::string msg, uint8_t bNewLine)
{
#ifdef _WIN32
	MessageBox(NULL, CString(msg.c_str()), _T("Message"), MB_OK);
#else
	printf("%s%s", sMsg, (bNewLine ? "\n" : ""));
#endif
}

void get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, double sx, double sy, double *dx, double *dy)
{
	if (srcWidth <= 0)
		return;

	double dZoom = (double)rDisplayedImageRect.Width() / (double)(srcWidth);

	*dx = sx;
	*dy = sy;

	//화면에 표시된 영상의 l,t값을 빼서 확대 영상내에서의 상대좌표로 변환한 후
	*dx -= rDisplayedImageRect.left;
	*dy -= rDisplayedImageRect.top;

	//확대된 비율로 나누고
	*dx /= dZoom;
	*dy /= dZoom;
}

void get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint *pt_dst)
{
	double dx = (double)pt_src.x;
	double dy = (double)pt_src.y;

	get_real_coord_from_screen_coord(rDisplayedImageRect, srcWidth, dx, dy, &dx, &dy);
	pt_dst->x = dx;
	pt_dst->y = dy;
}

void get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect *r_dst)
{
	double x1 = (double)(r_src.left);
	double y1 = (double)(r_src.top);
	double x2 = (double)(r_src.right);
	double y2 = (double)(r_src.bottom);

	get_real_coord_from_screen_coord(rDisplayedImageRect, srcWidth, x1, y1, &x1, &y1);
	get_real_coord_from_screen_coord(rDisplayedImageRect, srcWidth, x2, y2, &x2, &y2);

	r_dst->left = x1;
	r_dst->top = y1;
	r_dst->right = x2;
	r_dst->bottom = y2;
}

void get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, Gdiplus::RectF r_src, Gdiplus::RectF* r_dst)
{
	double x1 = (double)(r_src.X);
	double y1 = (double)(r_src.Y);
	double x2 = (double)(r_src.GetRight());
	double y2 = (double)(r_src.GetBottom());

	get_real_coord_from_screen_coord(rDisplayedImageRect, srcWidth, x1, y1, &x1, &y1);
	get_real_coord_from_screen_coord(rDisplayedImageRect, srcWidth, x2, y2, &x2, &y2);

	r_dst->X = x1;
	r_dst->Y = y1;
	r_dst->Width = x2 - x1;
	r_dst->Height = y2 - y1;
}

void get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, double sx, double sy, double* dx, double* dy)
{
	if (srcWidth <= 0)
		return;

	double dZoom = (double)rDisplayedImageRect.Width() / (double)(srcWidth);

	*dx = sx;
	*dy = sy;

	//확대된 비율을 곱하고
	*dx *= dZoom;
	*dy *= dZoom;

	//옵셋만큼 상대좌표로 이동시킨다.
	*dx += rDisplayedImageRect.left;
	*dy += rDisplayedImageRect.top;
}

void get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint* pt_dst)
{
	double x = (double)pt_src.x;
	double y = (double)pt_src.y;

	get_screen_coord_from_real_coord(rDisplayedImageRect, srcWidth, x, y, &x, &y);
	pt_dst->x = x;
	pt_dst->y = y;
}

void get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect* r_dst)
{
	double x1 = (double)(r_src.left);
	double y1 = (double)(r_src.top);
	double x2 = (double)(r_src.right);
	double y2 = (double)(r_src.bottom);

	get_screen_coord_from_real_coord(rDisplayedImageRect, srcWidth, x1, y1, &x1, &y1);
	get_screen_coord_from_real_coord(rDisplayedImageRect, srcWidth, x2, y2, &x2, &y2);

	r_dst->left = x1;
	r_dst->top = y1;
	r_dst->right = x2;
	r_dst->bottom = y2;
}

void get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, Gdiplus::RectF r_src, Gdiplus::RectF* r_dst)
{
	double x1 = (double)(r_src.X);
	double y1 = (double)(r_src.Y);
	double x2 = (double)(r_src.GetRight());
	double y2 = (double)(r_src.GetBottom());

	get_screen_coord_from_real_coord(rDisplayedImageRect, srcWidth, x1, y1, &x1, &y1);
	get_screen_coord_from_real_coord(rDisplayedImageRect, srcWidth, x2, y2, &x2, &y2);

	r_dst->X = x1;
	r_dst->Y = y1;
	r_dst->Width = x2 - x1;
	r_dst->Height = y2 - y1;
}

//str의 from 위치 이후에 있는 숫자 영역값을 num에 넣어주고 숫자 시작위치를 return한다.
//리턴되는 숫자의 시작위치는 from부터가 아닌 str 전체 문자열에서의 위치이다.
//숫자 앞에 붙은 '+' or '-'은 무시되므로 만약 부호를 판별해야 한다면
//리턴되는 위치의 앞에 있는 문자를 검사하여 양수인지, 음수인지 판별하면 된다.
int extract_digit_number(char *str, int from, double *num)
{
	int i;
	int first_pos = -1;
	char num_str[256] = {0,};
	int num_len = 0;
	bool punc = false;

	if (from >= strlen(str))
		return -1;

	for (i = from; i < strlen(str); i++)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			num_str[num_len++] = str[i];
			if (first_pos < 0)
				first_pos = i;
		}
		//소수점은 맨 처음오거나 중복되면 스킵된다.
		else if (str[i] == '.')
		{
			if ((num_len > 0) && !punc)
			{
				num_str[num_len++] = str[i];
				punc = true;
			}
			else if (punc)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if (i == strlen(str) || strlen(num_str) == 0)
		return -1;

	num_str[num_len] = '\0';
	*num = atof(num_str);
	return first_pos;
}

//version string valid check
//digits : 자릿수(1.0.0.1일 경우는 자릿수 4)
bool valid_version_string(CString versionStr, int digits)
{
	std::deque<CString> token;
	get_token_string(versionStr, token, '.');
	if (token.size() == digits)
		return true;

	return false;
}

//버전 또는 IP주소등은 그냥 문자열로 비교하면 1.0.9.0이 1.0.10.0보다 더 크다고 나오므로
//.을 없앤 숫자로 비교했으나 이 방법도 오류 발생(1.0.1.13 > 1.0.10.3보다 크다고 판단함)
//결국 각 자릿수끼리 구분하거나 자릿수를 맞춘 후 비교한다.
//리턴값은 strcmp와 동일한 규칙으로 판단한다.(+:str0가 큼, -:str1이 큼, 0:같음)
int	compare_string(CString str0, CString str1, TCHAR separator)
{
	size_t i = 0, j = 0;
	while (i < str0.GetLength() || j < str1.GetLength())
	{
		int acc1 = 0, acc2 = 0;

		while (i < str0.GetLength() && str0[i] != separator) { acc1 = acc1 * 10 + (str0[i] - '0');  i++; }
		while (j < str1.GetLength() && str1[j] != separator) { acc2 = acc2 * 10 + (str1[j] - '0');  j++; }

		if (acc1 < acc2)  return -1;
		if (acc1 > acc2)  return +1;

		++i;
		++j;
	}
	return 0;
	/*
	int i;
	std::deque<CString> token0;
	std::deque<CString> token1;
	std::deque<int> num0;
	std::deque<int> num1;

	get_token_string(str0, token0, separator);
	get_token_string(str1, token1, separator);

	num0.resize(token0.size());
	num1.resize(token1.size());

	for (i = 0; i < token0.size(); i++)
		num0[i] = _ttoi(token0[i]);

	for (i = 0; i < token1.size(); i++)
		num1[i] = _ttoi(token1[i]);

	return std::lexicographical_compare(num0.begin(), num0.end(), num1.begin(), num1.end());
	*/
}

//src를 n번 연결한 문자열 리턴. n개의 공백, 탭이 필요할 경우 사용
CString make_string(CString src, int n)
{
	CString result;

	for (int i = 0; i < n; i++)
		result += src;

	return result;
}

//button의 종류를 리턴한다.
DWORD		getButtonStyle(HWND hWnd)
{
	UINT button_style = BS_PUSHBUTTON;

	DWORD dwStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE); 

	return dwStyle;

	// Check Box 컨트롤인 경우, 3STATE 설정을 확인함. 
	if (((dwStyle & BS_AUTO3STATE) == BS_AUTO3STATE) || 
		((dwStyle & BS_3STATE) == BS_3STATE))
		button_style = BS_CHECKBOX; 

	switch (dwStyle & 0x0F) // 버튼속성. 
	{ 
	case BS_CHECKBOX: 
	case BS_AUTOCHECKBOX: 
		button_style = BS_CHECKBOX;
		break ; 
	case BS_RADIOBUTTON: 
	case BS_AUTORADIOBUTTON: 
		button_style = BS_RADIOBUTTON; 
		break ; 
	} 
	/*
	DWORD dwNewStyle;

	if ((button_style == BS_CHECKBOX) || (button_style == BS_RADIOBUTTON))
	{ 
		// BS_AUTO3STATE | BS_3STATE 상태를 제거한다. 
		DWORD dwRemove = BS_AUTO3STATE | BS_3STATE;        
		// BS_OWNERDRAW 속성을 설정한다. 
		dwNewStyle = (dwStyle & ~dwRemove) | BS_OWNERDRAW; 
	}
	else
	{
		dwNewStyle = dwStyle | BS_OWNERDRAW; 
	}

	::SetWindowLongPtr(hWnd, GWL_STYLE, dwNewStyle); 
	*/
	return button_style;
}

//그룹내의 연속된 radio 버튼들에 대한 일괄 처리용 함수.
//MFC의 기본 CheckRadioButton() 함수같은 경우는 BST_UNCHECKED 기능을 제공하지 않기 때문에
//아래 함수를 새로이 정의해서 사용한다.
//id_offset이 0보다 작으면 first ~ last까지 모두 unchecked.
//또한 특정 radio button을 사용자의 마우스 클릭이나 키보드 등과 같이
//직접 체크하는 경우가 아닌
//프로그램상에서 SetCheck와 같이 자동으로 선택된 상태로 표시하게 되면
//이전 선택되었던 radio가 여전히 focus를 가지고 있게 되고
//이는 다른 창에 focus를 주었다가 현재 dlg가 다시 focus를 가져온 순간
//ON_CONTROL_RANGE함수가 호출되면서 의도치 않은 현상이 발생하게 된다.
//따라서 특정 범위의 radio button 중에 하나를 프로그램상에서 자동으로 선택되게 하려면
//반드시 focus까지 같이 줘야만 이러한 오동작을 피할 수 있다.
void CheckRadioButtons(CWnd *pWnd, int idFirst, int idLast, int id_offset, int nCheck)
{
	int i;

	if (id_offset < 0)
	{
		for (i = idFirst; i <= idLast; i++)
		{
			((CButton*)(pWnd->GetDlgItem(i)))->SetCheck(BST_UNCHECKED);
			pWnd->GetDlgItem(i)->SendMessage(WM_KILLFOCUS, NULL);
		}
	}
	else
	{
		pWnd->CheckRadioButton(idFirst, idLast, idFirst + id_offset);
		pWnd->GetDlgItem(idFirst + id_offset)->SetFocus();
	}
}

//dialog based에서 키입력으로 동작을 정의하는데 CEdit과 같은 입력창에 포커스가 있으면
//PreTranslateMessage에서 방향키나 char키를 처리하기가 곤란하다.
//따라서 현재 포커스를 가진 컨트롤이 CEdit이고 enable이고 readonly가 아닌 경우에는
//PreTranslateMessage에서 입력된 키를 처리하도록 한다.
bool IsEditCtrlAcceptKeyState(CWnd *pWnd)
{
	if (pWnd->IsKindOf(RUNTIME_CLASS(CEdit)) == false || pWnd->IsWindowEnabled() == false)
		return false;

	CEdit *pEdit = (CEdit*)pWnd;
	if (pEdit->GetStyle() & ES_READONLY)
		return false;

	return true;
}


void printf_string(const char* psz, ...)
{
#ifndef _DEBUG
	return;
#endif
	char buffer[4096];
	va_list vaList;
	va_start(vaList, psz);
	_vsnprintf(buffer, 4096, psz, vaList);
	va_end(vaList); 

	printf(buffer);
	TRACE(buffer);
}

void trace_output(bool only_text, TCHAR* func, int line, bool linefeed, LPCTSTR format, ...)
{
	va_list args;
	va_start(args, format);

	if (args == NULL)
		return;

	CString str;
	str.FormatV(format, args);
	va_end(args);

	if (str.IsEmpty())
		return;

	if (!only_text)
		str.Format(_T("%s [%s][%d] %s%c"), get_cur_datetime_str(1, true, _T(" "), true, true, true), func, line, str, (linefeed ? '\n' : '\0'));

	OutputDebugString(str);
}

//not tested
/*
void usleep(int microSec)
{
	__int64 time1 = 0, time2 = 0, freq = 0;

	QueryPerformanceCounter((LARGE_INTEGER *) &time1);
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

	do {
		QueryPerformanceCounter((LARGE_INTEGER *) &time2);
	} while((time2-time1) < microSec);
}
*/

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag = 0;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		tmpres /= 10;  /*convert into microseconds*/
		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}

//"yy-MM-dd" 또는 "yy/MM/dd" 날짜 표시 형식에 맞는지 검사
//31, 30
bool is_valid_date(CString str)
{
	if (get_char_count(str, '/') == 2)
		str.Replace(_T("/"), _T("-"));

	if (get_char_count(str, '-') != 2)
		return false;

	int year = _ttoi(str.Left(2));
	if (year < 0 || year > 2100)
		return false;

	int month = _ttoi(str.Mid(3, 2));
	if (month < 0 || month > 12)
		return false;

	int max_days = GetDaysOfMonth(year, month);
	int day = _ttoi(str.Right(2));
	if (day < 0 || day > max_days)
		return false;

	return true;
}

//"hh:mm:ss" 시간 표시 형식에 맞는지 검사
bool is_valid_time(CString str)
{
	//1:23:45 => 01:23:45
	if ((str.GetLength() == 7) && (get_char_count(str, ':') == 2))
		str = _T("0") + str;

	if (str.GetLength() != 8)
		return false;

	if (get_char_count(str, ':') != 2)
		return false;

	str.Replace(_T(":"), _T(""));
	if (!IsNatural(str))
		return false;

	int n = _ttoi(str.Left(2));
	if (n < 0 || n > 23)
		return false;

	n = _ttoi(str.Mid(2, 2));
	if (n < 0 || n > 59)
		return false;

	n = _ttoi(str.Right(2));
	if (n < 0 || n > 59)
		return false;

	return true;
}

//날짜시각 형식을 yyyy/mm/dd hh:mm:ss 포맷으로 맞춘다.
//230303192736			//최소한의 길이는 12
//23/3/3 9:7:6			//최소한의 길이는 12
//2023/3/3 9:7:6
//=>2023/03/14 19:27:36.123	//최대길이는 23
void normalize_datetime(CString &src)
{
	CString result;

	if (src.GetLength() < 12)
		return;

	std::deque<CString> token;

	if (src[2] == '-')
		src.Replace('-', '/');
	if (src[2] == '/')
		src = _T("20") + src;

	if (src.Find(' ') < 0)
		return;

	CString sub = src.Left(src.Find(' '));
	get_token_string(sub, token, '/');
	if (token.size() != 3)
		return;

	int y = _ttoi(token[0]);
	int M = _ttoi(token[1]);
	int d = _ttoi(token[2]);

	result.Format(_T("%d/%02d/%02d "), y, M, d);


	sub = src.Mid(src.Find(' ') + 1);
	get_token_string(sub, token, ':');
	if (token.size() != 3)
		return;

	int h = _ttoi(token[0]);
	int m = _ttoi(token[1]);
	int s = _ttoi(token[2]);

	result.Format(_T("%s%02d:%02d:%02d"), result, h, m, s);
	src = result;
}

//수행시간을 측정하는데
//고정밀 측정은 QueryPerformanceFrequency와 QueryPerformanceCounter를 이용한다.
//이는 무겁기도 하고 thread safe하지 않으므로 가능하면 사용을 권장하지 않는다.
//clock을 이용한 방식으로도 거의 유사한 소요 시간을 측정할 수 있고
//linux에서도 그대로 사용할 수 있는 장점이 있다.
//GetTickCount()는 편차가 많이 발생하는 단점이 있다.
//unit : ms
//단, 이 값이 long으로 계산되므로 그 리턴값은 음수로 표현될 수 있다.
//하지만 시간 측정하는데는 문제되지 않는다.
long getClock()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	long ret = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	//TRACE("%d %d = %d\n", tv.tv_sec, tv.tv_usec, ret);
	return ret;
}


void resize_bilinear_c1(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h)
{
	const int INTER_RESIZE_COEF_BITS=11;
	const int INTER_RESIZE_COEF_SCALE=1 << INTER_RESIZE_COEF_BITS;
	//     const int ONE=INTER_RESIZE_COEF_SCALE;

	double scale_x = (double)srcw / w;
	double scale_y = (double)srch / h;

	int* buf = new int[w + h + w + h];

	int* xofs = buf;//new int[w];
	int* yofs = buf + w;//new int[h];

	short* ialpha = (short*)(buf + w + h);//new short[w * 2];
	short* ibeta = (short*)(buf + w + h + w);//new short[h * 2];

	float fx;
	float fy;
	int sx;
	int sy;

#define SATURATE_CAST_SHORT(X) (short)::std::min(::std::max((int)(X + (X >= 0.f ? 0.5f : -0.5f)), SHRT_MIN), SHRT_MAX);
//#define SATURATE_CAST_SHORT(X) (short)MIN(MAX((int)(X + (X >= 0.f ? 0.5f : -0.5f)), SHRT_MIN), SHRT_MAX);

	for (int dx = 0; dx < w; dx++)
	{
		fx = (float)((dx + 0.5) * scale_x - 0.5);
		sx = fx;//cvFloor(fx);
		fx -= sx;

		if (sx >= srcw - 1)
		{
			sx = srcw - 2;
			fx = 1.f;
		}

		xofs[dx] = sx;

		float a0 = (1.f - fx) * INTER_RESIZE_COEF_SCALE;
		float a1 =        fx  * INTER_RESIZE_COEF_SCALE;

		ialpha[dx*2    ] = SATURATE_CAST_SHORT(a0);
		ialpha[dx*2 + 1] = SATURATE_CAST_SHORT(a1);
	}

	for (int dy = 0; dy < h; dy++)
	{
		fy = (float)((dy + 0.5) * scale_y - 0.5);
		sy = fy;//cvFloor(fy);
		fy -= sy;

		if (sy >= srch - 1)
		{
			sy = srch - 2;
			fy = 1.f;
		}

		yofs[dy] = sy;

		float b0 = (1.f - fy) * INTER_RESIZE_COEF_SCALE;
		float b1 =        fy  * INTER_RESIZE_COEF_SCALE;

		ibeta[dy*2    ] = SATURATE_CAST_SHORT(b0);
		ibeta[dy*2 + 1] = SATURATE_CAST_SHORT(b1);
	}

#undef SATURATE_CAST_SHORT

	// loop body
	//Mat rowsbuf0((w >> 1) + 1);
	//Mat rowsbuf1((w >> 1) + 1);
	//short* rows0 = (short*)rowsbuf0.data;
	//short* rows1 = (short*)rowsbuf1.data;
	short* rows0 = new short[(w >> 1) + 1];
	short* rows1 = new short[(w >> 1) + 1];
	memset(rows0, 0, sizeof(short) * ((w >> 1) + 1));
	memset(rows1, 0, sizeof(short) * ((w >> 1) + 1));

	int prev_sy1 = -1;

	for (int dy = 0; dy < h; dy++)
	{
		int sy = yofs[dy];

		if (sy == prev_sy1)
		{
			// hresize one row
			short* rows0_old = rows0;
			rows0 = rows1;
			rows1 = rows0_old;
			const unsigned char *S1 = src + srcw * (sy+1);

			const short* ialphap = ialpha;
			short* rows1p = rows1;
			for (int dx = 0; dx < w; dx++)
			{
				int sx = xofs[dx];
				short a0 = ialphap[0];
				short a1 = ialphap[1];

				const unsigned char* S1p = S1 + sx;
				rows1p[dx] = (S1p[0]*a0 + S1p[1]*a1) >> 4;

				ialphap += 2;
			}
		}
		else
		{
			// hresize two rows
			const unsigned char *S0 = src + srcw * (sy);
			const unsigned char *S1 = src + srcw * (sy+1);

			const short* ialphap = ialpha;
			short* rows0p = rows0;
			short* rows1p = rows1;
			for (int dx = 0; dx < w; dx++)
			{
				int sx = xofs[dx];
				short a0 = ialphap[0];
				short a1 = ialphap[1];

				const unsigned char* S0p = S0 + sx;
				const unsigned char* S1p = S1 + sx;
				rows0p[dx] = (S0p[0]*a0 + S0p[1]*a1) >> 4;
				rows1p[dx] = (S1p[0]*a0 + S1p[1]*a1) >> 4;

				ialphap += 2;
			}
		}

		prev_sy1 = sy + 1;

		// vresize
		short b0 = ibeta[0];
		short b1 = ibeta[1];

		short* rows0p = rows0;
		short* rows1p = rows1;
		unsigned char* Dp = dst + w * (dy);

#if __ARM_NEON
		int nn = w >> 3;
#else
		int nn = 0;
#endif
		int remain = w - (nn << 3);

#if __ARM_NEON
#if __aarch64__
		int16x4_t _b0 = vdup_n_s16(b0);
		int16x4_t _b1 = vdup_n_s16(b1);
		int32x4_t _v2 = vdupq_n_s32(2);
		for (; nn>0; nn--)
		{
			int16x4_t _rows0p_sr4 = vld1_s16(rows0p);
			int16x4_t _rows1p_sr4 = vld1_s16(rows1p);
			int16x4_t _rows0p_1_sr4 = vld1_s16(rows0p+4);
			int16x4_t _rows1p_1_sr4 = vld1_s16(rows1p+4);

			int32x4_t _rows0p_sr4_mb0 = vmull_s16(_rows0p_sr4, _b0);
			int32x4_t _rows1p_sr4_mb1 = vmull_s16(_rows1p_sr4, _b1);
			int32x4_t _rows0p_1_sr4_mb0 = vmull_s16(_rows0p_1_sr4, _b0);
			int32x4_t _rows1p_1_sr4_mb1 = vmull_s16(_rows1p_1_sr4, _b1);

			int32x4_t _acc = _v2;
			_acc = vsraq_n_s32(_acc, _rows0p_sr4_mb0, 16);
			_acc = vsraq_n_s32(_acc, _rows1p_sr4_mb1, 16);

			int32x4_t _acc_1 = _v2;
			_acc_1 = vsraq_n_s32(_acc_1, _rows0p_1_sr4_mb0, 16);
			_acc_1 = vsraq_n_s32(_acc_1, _rows1p_1_sr4_mb1, 16);

			int16x4_t _acc16 = vshrn_n_s32(_acc, 2);
			int16x4_t _acc16_1 = vshrn_n_s32(_acc_1, 2);

			uint8x8_t _D = vqmovun_s16(vcombine_s16(_acc16, _acc16_1));

			vst1_u8(Dp, _D);

			Dp += 8;
			rows0p += 8;
			rows1p += 8;
		}
#else
		if (nn > 0)
		{
			asm volatile(
				"vdup.s16   d16, %8         \n"
				"mov        r4, #2          \n"
				"vdup.s16   d17, %9         \n"
				"vdup.s32   q12, r4         \n"
				"pld        [%0, #128]      \n"
				"vld1.s16   {d2-d3}, [%0 :128]!\n"
				"pld        [%1, #128]      \n"
				"vld1.s16   {d6-d7}, [%1 :128]!\n"
				"0:                         \n"
				"vmull.s16  q0, d2, d16     \n"
				"vmull.s16  q1, d3, d16     \n"
				"vorr.s32   q10, q12, q12   \n"
				"vorr.s32   q11, q12, q12   \n"
				"vmull.s16  q2, d6, d17     \n"
				"vmull.s16  q3, d7, d17     \n"
				"vsra.s32   q10, q0, #16    \n"
				"vsra.s32   q11, q1, #16    \n"
				"pld        [%0, #128]      \n"
				"vld1.s32   {d2-d3}, [%0 :128]!\n"
				"vsra.s32   q10, q2, #16    \n"
				"vsra.s32   q11, q3, #16    \n"
				"pld        [%1, #128]      \n"
				"vld1.s32   {d6-d7}, [%1 :128]!\n"
				"vshrn.s32  d20, q10, #2    \n"
				"vshrn.s32  d21, q11, #2    \n"
				"vqmovun.s16 d20, q10        \n"
				"vst1.8     {d20}, [%2]!    \n"
				"subs       %3, #1          \n"
				"bne        0b              \n"
				"sub        %0, #16         \n"
				"sub        %1, #16         \n"
				: "=r"(rows0p), // %0
				"=r"(rows1p), // %1
				"=r"(Dp),     // %2
				"=r"(nn)      // %3
				: "0"(rows0p),
				"1"(rows1p),
				"2"(Dp),
				"3"(nn),
				"r"(b0),      // %8
				"r"(b1)       // %9
				: "cc", "memory", "r4", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12"
				);
		}
#endif // __aarch64__
#endif // __ARM_NEON
		for (; remain; --remain)
		{
			//             D[x] = (rows0[x]*b0 + rows1[x]*b1) >> INTER_RESIZE_COEF_BITS;
			*Dp++ = (unsigned char)(((short)((b0 * (short)(*rows0p++)) >> 16) + (short)((b1 * (short)(*rows1p++)) >> 16) + 2)>>2);
		}

		ibeta += 2;
	}

	delete[] buf;
	delete [] rows0;
	delete [] rows1;
}

void resize_bilinear_image(uint8_t *src, int sw, int sh, uint8_t *dst, int w, int h)
{
	double scale_x = (double)sw / w;
	double scale_y = (double)sh / h;

	int* buf = new int[w + h + w*2 + h*2];

	int* xofs = buf;//new int[w];
	int* yofs = buf + w;//new int[h];

	float* alpha = (float*)(buf + w + h);//new float[w * 2];
	float* beta = (float*)(buf + w + h + w*2);//new float[h * 2];

	float fx;
	float fy;
	int sx;
	int sy;

	for (int dx = 0; dx < w; dx++)
	{
		fx = (float)((dx + 0.5) * scale_x - 0.5);
		sx = fx;//cvFloor(fx);
		fx -= sx;

		if(sx >= sw-1)
		{
			sx = sw - 2;
			fx = 1.f;
		}

		xofs[dx] = sx;

		alpha[dx*2    ] = 1.f - fx;
		alpha[dx*2 + 1] = fx;
	}

	for (int dy = 0; dy < h; dy++)
	{
		fy = (float)((dy + 0.5) * scale_y - 0.5);
		sy = fy;//cvFloor(fy);
		fy -= sy;

		if (sy >= sh - 1)
		{
			sy = sh - 2;
			fy = 1.f;
		}

		yofs[dy] = sy;

		beta[dy*2    ] = 1.f - fy;
		beta[dy*2 + 1] = fy;
	}

	// loop body
	//Mat rowsbuf0(w + 1);
	//Mat rowsbuf1(w + 1);
	//float* rows0 = rowsbuf0;
	//float* rows1 = rowsbuf1;
	float* rows0 = (float*) new float[w + 1];
	float* rows1 = (float*) new float[w + 1];

	int prev_sy1 = -1;

	for (int dy = 0; dy < h; dy++)
	{
		int sy = yofs[dy];

		if (sy == prev_sy1)
		{
			// hresize one row
			float* rows0_old = rows0;
			rows0 = rows1;
			rows1 = rows0_old;
			//const float* S1 = src.row(sy+1);
			const float* S1 = (float*)src + sw * (sy+1);

			const float* alphap = alpha;
			float* rows1p = rows1;
			int dx = 0;
#if __ARM_NEON
			for (; dx+1 < w; dx += 2)
			{
				int sx = xofs[dx];
				int sxn = xofs[dx+1];
				const float* S1p = S1 + sx;
				const float* S1np = S1 + sxn;

				float32x4_t _a = vld1q_f32(alphap);
				float32x2_t _S1 = vld1_f32(S1p);
				float32x2_t _S1n = vld1_f32(S1np);

				float32x4_t _S1S1n = vcombine_f32(_S1, _S1n);
				float32x4_t _ms1 = vmulq_f32(_S1S1n, _a);
				float32x2_t _rows1 = vpadd_f32(vget_low_f32(_ms1), vget_high_f32(_ms1));

				vst1_f32(rows1p + dx, _rows1);

				alphap += 4;
			}
#endif // __ARM_NEON
			for (; dx < w; dx++)
			{
				int sx = xofs[dx];
				const float* S1p = S1 + sx;

				float a0 = alphap[0];
				float a1 = alphap[1];
				rows1p[dx] = S1p[0]*a0 + S1p[1]*a1;

				alphap += 2;
			}
		}
		else
		{
			// hresize two rows
			const float* S0 = (float*)src + sw * (sy);//src.row(sy);
			const float* S1 = (float*)src + sw * (sy+1);//src.row(sy+1);

			const float* alphap = alpha;
			float* rows0p = rows0;
			float* rows1p = rows1;
			int dx = 0;
#if __ARM_NEON
			for (; dx+1 < w; dx += 2)
			{
				int sx = xofs[dx];
				int sxn = xofs[dx+1];
				const float* S0p = S0 + sx;
				const float* S1p = S1 + sx;
				const float* S0np = S0 + sxn;
				const float* S1np = S1 + sxn;

				float32x4_t _a = vld1q_f32(alphap);
				float32x2_t _S0 = vld1_f32(S0p);
				float32x2_t _S1 = vld1_f32(S1p);
				float32x2_t _S0n = vld1_f32(S0np);
				float32x2_t _S1n = vld1_f32(S1np);

				float32x4_t _S0S0n = vcombine_f32(_S0, _S0n);
				float32x4_t _S1S1n = vcombine_f32(_S1, _S1n);
				float32x4_t _ms0 = vmulq_f32(_S0S0n, _a);
				float32x4_t _ms1 = vmulq_f32(_S1S1n, _a);
				float32x2_t _rows0 = vpadd_f32(vget_low_f32(_ms0), vget_high_f32(_ms0));
				float32x2_t _rows1 = vpadd_f32(vget_low_f32(_ms1), vget_high_f32(_ms1));

				vst1_f32(rows0p + dx, _rows0);
				vst1_f32(rows1p + dx, _rows1);

				alphap += 4;
			}
#endif // __ARM_NEON
			for (; dx < w; dx++)
			{
				int sx = xofs[dx];
				const float* S0p = S0 + sx;
				const float* S1p = S1 + sx;

				float a0 = alphap[0];
				float a1 = alphap[1];
				rows0p[dx] = S0p[0]*a0 + S0p[1]*a1;
				rows1p[dx] = S1p[0]*a0 + S1p[1]*a1;

				alphap += 2;
			}
		}

		prev_sy1 = sy + 1;

		// vresize
		float b0 = beta[0];
		float b1 = beta[1];

		float* rows0p = rows0;
		float* rows1p = rows1;
		float* Dp = (float*)dst + w * (dy);//dst.row(dy);

#if __ARM_NEON
		int nn = w >> 3;
#else
		int nn = 0;
#endif
		int remain = w - (nn << 3);

#if __ARM_NEON
		float32x4_t _b0 = vdupq_n_f32(b0);
		float32x4_t _b1 = vdupq_n_f32(b1);
		for (; nn>0; nn--)
		{
			float32x4_t _rows0 = vld1q_f32(rows0p);
			float32x4_t _rows1 = vld1q_f32(rows1p);

			float32x4_t _D = vmulq_f32(_rows0, _b0);
			_D = vmlaq_f32(_D, _rows1, _b1);

			vst1q_f32(Dp, _D);

			float32x4_t _rows0n = vld1q_f32(rows0p+4);
			float32x4_t _rows1n = vld1q_f32(rows1p+4);

			float32x4_t _Dn = vmulq_f32(_rows0n, _b0);
			_Dn = vmlaq_f32(_Dn, _rows1n, _b1);

			vst1q_f32(Dp+4, _Dn);

			Dp += 8;
			rows0p += 8;
			rows1p += 8;
		}
#endif // __ARM_NEON
		for (; remain; --remain)
		{
			//             D[x] = rows0[x]*b0 + rows1[x]*b1;
			*Dp++ = *rows0p++ * b0 + *rows1p++ * b1;
		}

		beta += 2;
	}

	delete[] buf;
	delete[] rows0;
	delete[] rows1;
}

void resize_bilinear_c3(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h)
{
	const int INTER_RESIZE_COEF_BITS=11;
	const int INTER_RESIZE_COEF_SCALE=1 << INTER_RESIZE_COEF_BITS;
	//     const int ONE=INTER_RESIZE_COEF_SCALE;

	double scale_x = (double)srcw / w;
	double scale_y = (double)srch / h;

	int* buf = new int[w + h + w + h];

	int* xofs = buf;//new int[w];
	int* yofs = buf + w;//new int[h];

	short* ialpha = (short*)(buf + w + h);//new short[w * 2];
	short* ibeta = (short*)(buf + w + h + w);//new short[h * 2];

	float fx;
	float fy;
	int sx;
	int sy;

#define SATURATE_CAST_SHORT(X) (short)::std::min(::std::max((int)(X + (X >= 0.f ? 0.5f : -0.5f)), SHRT_MIN), SHRT_MAX);
//#define SATURATE_CAST_SHORT(X) (short)MIN(MAX((int)(X + (X >= 0.f ? 0.5f : -0.5f)), SHRT_MIN), SHRT_MAX);

	for (int dx = 0; dx < w; dx++)
	{
		fx = (float)((dx + 0.5) * scale_x - 0.5);
		sx = fx;//cvFloor(fx);
		fx -= sx;

		if (sx >= srcw - 1)
		{
			sx = srcw - 2;
			fx = 1.f;
		}

		xofs[dx] = sx*3;

		float a0 = (1.f - fx) * INTER_RESIZE_COEF_SCALE;
		float a1 =        fx  * INTER_RESIZE_COEF_SCALE;

		ialpha[dx*2    ] = SATURATE_CAST_SHORT(a0);
		ialpha[dx*2 + 1] = SATURATE_CAST_SHORT(a1);
	}

	for (int dy = 0; dy < h; dy++)
	{
		fy = (float)((dy + 0.5) * scale_y - 0.5);
		sy = fy;//cvFloor(fy);
		fy -= sy;

		if (sy >= srch - 1)
		{
			sy = srch - 2;
			fy = 1.f;
		}

		yofs[dy] = sy*3;

		float b0 = (1.f - fy) * INTER_RESIZE_COEF_SCALE;
		float b1 =        fy  * INTER_RESIZE_COEF_SCALE;

		ibeta[dy*2    ] = SATURATE_CAST_SHORT(b0);
		ibeta[dy*2 + 1] = SATURATE_CAST_SHORT(b1);
	}

#undef SATURATE_CAST_SHORT

	// loop body
	//Mat rowsbuf0((w*3 >> 1) + 3);
	//Mat rowsbuf1((w*3 >> 1) + 3);
	//short* rows0 = (short*)rowsbuf0.data;
	//short* rows1 = (short*)rowsbuf1.data;
	short* rows0 = new short[(w*3 >> 1) + 3];
	short* rows1 = new short[(w*3 >> 1) + 3];

	int prev_sy1 = -1;

	for (int dy = 0; dy < h; dy++)
	{
		int sy = yofs[dy];

		if (sy == prev_sy1)
		{
			// hresize one row
			short* rows0_old = rows0;
			rows0 = rows1;
			rows1 = rows0_old;
			const unsigned char *S1 = src + srcw * (sy+3);

			const short* ialphap = ialpha;
			short* rows1p = rows1;
			for (int dx = 0; dx < w; dx++)
			{
				int sx = xofs[dx];
				short a0 = ialphap[0];
				short a1 = ialphap[1];

				const unsigned char* S1p = S1 + sx;
#if __ARM_NEON
				int16x4_t _a0 = vdup_n_s16(a0);
				int16x4_t _a1 = vdup_n_s16(a1);
				uint8x8_t _S1 = vld1_u8(S1p);
				int16x8_t _S116 = vreinterpretq_s16_u16(vmovl_u8(_S1));
				int16x4_t _S1low = vget_low_s16(_S116);
				int16x4_t _S1high = vext_s16(_S1low, vget_high_s16(_S116), 3);
				int32x4_t _rows1 = vmull_s16(_S1low, _a0);
				_rows1 = vmlal_s16(_rows1, _S1high, _a1);
				int16x4_t _rows1_sr4 = vshrn_n_s32(_rows1, 4);
				vst1_s16(rows1p, _rows1_sr4);
#else
				rows1p[0] = (S1p[0]*a0 + S1p[3]*a1) >> 4;
				rows1p[1] = (S1p[1]*a0 + S1p[4]*a1) >> 4;
				rows1p[2] = (S1p[2]*a0 + S1p[5]*a1) >> 4;
#endif // __ARM_NEON

				ialphap += 2;
				rows1p += 3;
			}
		}
		else
		{
			// hresize two rows
			const unsigned char *S0 = src + srcw * (sy);
			const unsigned char *S1 = src + srcw * (sy+3);

			const short* ialphap = ialpha;
			short* rows0p = rows0;
			short* rows1p = rows1;
			for (int dx = 0; dx < w; dx++)
			{
				int sx = xofs[dx];
				short a0 = ialphap[0];
				short a1 = ialphap[1];

				const unsigned char* S0p = S0 + sx;
				const unsigned char* S1p = S1 + sx;
#if __ARM_NEON
				int16x4_t _a0 = vdup_n_s16(a0);
				int16x4_t _a1 = vdup_n_s16(a1);
				uint8x8_t _S0 = vld1_u8(S0p);
				uint8x8_t _S1 = vld1_u8(S1p);
				int16x8_t _S016 = vreinterpretq_s16_u16(vmovl_u8(_S0));
				int16x8_t _S116 = vreinterpretq_s16_u16(vmovl_u8(_S1));
				int16x4_t _S0low = vget_low_s16(_S016);
				int16x4_t _S1low = vget_low_s16(_S116);
				int16x4_t _S0high = vext_s16(_S0low, vget_high_s16(_S016), 3);
				int16x4_t _S1high = vext_s16(_S1low, vget_high_s16(_S116), 3);
				int32x4_t _rows0 = vmull_s16(_S0low, _a0);
				int32x4_t _rows1 = vmull_s16(_S1low, _a0);
				_rows0 = vmlal_s16(_rows0, _S0high, _a1);
				_rows1 = vmlal_s16(_rows1, _S1high, _a1);
				int16x4_t _rows0_sr4 = vshrn_n_s32(_rows0, 4);
				int16x4_t _rows1_sr4 = vshrn_n_s32(_rows1, 4);
				vst1_s16(rows0p, _rows0_sr4);
				vst1_s16(rows1p, _rows1_sr4);
#else
				rows0p[0] = (S0p[0]*a0 + S0p[3]*a1) >> 4;
				rows0p[1] = (S0p[1]*a0 + S0p[4]*a1) >> 4;
				rows0p[2] = (S0p[2]*a0 + S0p[5]*a1) >> 4;
				rows1p[0] = (S1p[0]*a0 + S1p[3]*a1) >> 4;
				rows1p[1] = (S1p[1]*a0 + S1p[4]*a1) >> 4;
				rows1p[2] = (S1p[2]*a0 + S1p[5]*a1) >> 4;
#endif // __ARM_NEON

				ialphap += 2;
				rows0p += 3;
				rows1p += 3;
			}
		}

		prev_sy1 = sy + 1;

		// vresize
		short b0 = ibeta[0];
		short b1 = ibeta[1];

		short* rows0p = rows0;
		short* rows1p = rows1;
		unsigned char* Dp = dst + w * 3 * (dy);

#if __ARM_NEON
		int nn = (w * 3) >> 3;
#else
		int nn = 0;
#endif
		int remain = (w * 3) - (nn << 3);

#if __ARM_NEON
#if __aarch64__
		int16x4_t _b0 = vdup_n_s16(b0);
		int16x4_t _b1 = vdup_n_s16(b1);
		int32x4_t _v2 = vdupq_n_s32(2);
		for (; nn>0; nn--)
		{
			int16x4_t _rows0p_sr4 = vld1_s16(rows0p);
			int16x4_t _rows1p_sr4 = vld1_s16(rows1p);
			int16x4_t _rows0p_1_sr4 = vld1_s16(rows0p+4);
			int16x4_t _rows1p_1_sr4 = vld1_s16(rows1p+4);

			int32x4_t _rows0p_sr4_mb0 = vmull_s16(_rows0p_sr4, _b0);
			int32x4_t _rows1p_sr4_mb1 = vmull_s16(_rows1p_sr4, _b1);
			int32x4_t _rows0p_1_sr4_mb0 = vmull_s16(_rows0p_1_sr4, _b0);
			int32x4_t _rows1p_1_sr4_mb1 = vmull_s16(_rows1p_1_sr4, _b1);

			int32x4_t _acc = _v2;
			_acc = vsraq_n_s32(_acc, _rows0p_sr4_mb0, 16);
			_acc = vsraq_n_s32(_acc, _rows1p_sr4_mb1, 16);

			int32x4_t _acc_1 = _v2;
			_acc_1 = vsraq_n_s32(_acc_1, _rows0p_1_sr4_mb0, 16);
			_acc_1 = vsraq_n_s32(_acc_1, _rows1p_1_sr4_mb1, 16);

			int16x4_t _acc16 = vshrn_n_s32(_acc, 2);
			int16x4_t _acc16_1 = vshrn_n_s32(_acc_1, 2);

			uint8x8_t _D = vqmovun_s16(vcombine_s16(_acc16, _acc16_1));

			vst1_u8(Dp, _D);

			Dp += 8;
			rows0p += 8;
			rows1p += 8;
		}
#else
		if (nn > 0)
		{
			asm volatile(
				"vdup.s16   d16, %8         \n"
				"mov        r4, #2          \n"
				"vdup.s16   d17, %9         \n"
				"vdup.s32   q12, r4         \n"
				"pld        [%0, #128]      \n"
				"vld1.s16   {d2-d3}, [%0 :128]!\n"
				"pld        [%1, #128]      \n"
				"vld1.s16   {d6-d7}, [%1 :128]!\n"
				"0:                         \n"
				"vmull.s16  q0, d2, d16     \n"
				"vmull.s16  q1, d3, d16     \n"
				"vorr.s32   q10, q12, q12   \n"
				"vorr.s32   q11, q12, q12   \n"
				"vmull.s16  q2, d6, d17     \n"
				"vmull.s16  q3, d7, d17     \n"
				"vsra.s32   q10, q0, #16    \n"
				"vsra.s32   q11, q1, #16    \n"
				"pld        [%0, #128]      \n"
				"vld1.s16   {d2-d3}, [%0 :128]!\n"
				"vsra.s32   q10, q2, #16    \n"
				"vsra.s32   q11, q3, #16    \n"
				"pld        [%1, #128]      \n"
				"vld1.s16   {d6-d7}, [%1 :128]!\n"
				"vshrn.s32  d20, q10, #2    \n"
				"vshrn.s32  d21, q11, #2    \n"
				"vqmovun.s16 d20, q10        \n"
				"vst1.8     {d20}, [%2]!    \n"
				"subs       %3, #1          \n"
				"bne        0b              \n"
				"sub        %0, #16         \n"
				"sub        %1, #16         \n"
				: "=r"(rows0p), // %0
				"=r"(rows1p), // %1
				"=r"(Dp),     // %2
				"=r"(nn)      // %3
				: "0"(rows0p),
				"1"(rows1p),
				"2"(Dp),
				"3"(nn),
				"r"(b0),      // %8
				"r"(b1)       // %9
				: "cc", "memory", "r4", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12"
				);
		}
#endif // __aarch64__
#endif // __ARM_NEON
		for (; remain; --remain)
		{
			//             D[x] = (rows0[x]*b0 + rows1[x]*b1) >> INTER_RESIZE_COEF_BITS;
			*Dp++ = (unsigned char)(((short)((b0 * (short)(*rows0p++)) >> 16) + (short)((b1 * (short)(*rows1p++)) >> 16) + 2)>>2);
		}

		ibeta += 2;
	}

	delete[] buf;
	delete[] rows0;
	delete[] rows1;
}


void resize_bilinear_c4(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h)
{
	const int INTER_RESIZE_COEF_BITS=11;
	const int INTER_RESIZE_COEF_SCALE=1 << INTER_RESIZE_COEF_BITS;
	//     const int ONE=INTER_RESIZE_COEF_SCALE;

	double scale_x = (double)srcw / w;
	double scale_y = (double)srch / h;

	int* buf = new int[w + h + w + h];

	int* xofs = buf;//new int[w];
	int* yofs = buf + w;//new int[h];

	short* ialpha = (short*)(buf + w + h);//new short[w * 2];
	short* ibeta = (short*)(buf + w + h + w);//new short[h * 2];

	float fx;
	float fy;
	int sx;
	int sy;

#define SATURATE_CAST_SHORT(X) (short)::std::min(::std::max((int)(X + (X >= 0.f ? 0.5f : -0.5f)), SHRT_MIN), SHRT_MAX);
//#define SATURATE_CAST_SHORT(X) (short)MIN(MAX((int)(X + (X >= 0.f ? 0.5f : -0.5f)), SHRT_MIN), SHRT_MAX);

	for (int dx = 0; dx < w; dx++)
	{
		fx = (float)((dx + 0.5) * scale_x - 0.5);
		sx = fx;//cvFloor(fx);
		fx -= sx;

		if (sx >= srcw - 1)
		{
			sx = srcw - 2;
			fx = 1.f;
		}

		xofs[dx] = sx*4;

		float a0 = (1.f - fx) * INTER_RESIZE_COEF_SCALE;
		float a1 =        fx  * INTER_RESIZE_COEF_SCALE;

		ialpha[dx*2    ] = SATURATE_CAST_SHORT(a0);
		ialpha[dx*2 + 1] = SATURATE_CAST_SHORT(a1);
	}

	for (int dy = 0; dy < h; dy++)
	{
		fy = (float)((dy + 0.5) * scale_y - 0.5);
		sy = fy;//cvFloor(fy);
		fy -= sy;

		if (sy >= srch - 1)
		{
			sy = srch - 2;
			fy = 1.f;
		}

		yofs[dy] = sy*4;

		float b0 = (1.f - fy) * INTER_RESIZE_COEF_SCALE;
		float b1 =        fy  * INTER_RESIZE_COEF_SCALE;

		ibeta[dy*2    ] = SATURATE_CAST_SHORT(b0);
		ibeta[dy*2 + 1] = SATURATE_CAST_SHORT(b1);
	}

#undef SATURATE_CAST_SHORT

	// loop body
	//Mat rowsbuf0((w*4 >> 1) + 4);
	//Mat rowsbuf1((w*4 >> 1) + 4);
	//short* rows0 = (short*)rowsbuf0.data;
	//short* rows1 = (short*)rowsbuf1.data;
	short* rows0 = new short[(w*4 >> 1) + 4];
	short* rows1 = new short[(w*4 >> 1) + 4];

	int prev_sy1 = -1;

	for (int dy = 0; dy < h; dy++)
	{
		int sy = yofs[dy];

		if (sy == prev_sy1)
		{
			// hresize one row
			short* rows0_old = rows0;
			rows0 = rows1;
			rows1 = rows0_old;
			const unsigned char *S1 = src + srcw * (sy+4);

			const short* ialphap = ialpha;
			short* rows1p = rows1;
			for (int dx = 0; dx < w; dx++)
			{
				int sx = xofs[dx];
				short a0 = ialphap[0];
				short a1 = ialphap[1];

				const unsigned char* S1p = S1 + sx;
#if __ARM_NEON
				int16x4_t _a0 = vdup_n_s16(a0);
				int16x4_t _a1 = vdup_n_s16(a1);
				uint8x8_t _S1 = vld1_u8(S1p);
				int16x8_t _S116 = vreinterpretq_s16_u16(vmovl_u8(_S1));
				int16x4_t _S1low = vget_low_s16(_S116);
				int16x4_t _S1high = vget_high_s16(_S116);
				int32x4_t _rows1 = vmull_s16(_S1low, _a0);
				_rows1 = vmlal_s16(_rows1, _S1high, _a1);
				int16x4_t _rows1_sr4 = vshrn_n_s32(_rows1, 4);
				vst1_s16(rows1p, _rows1_sr4);
#else
				rows1p[0] = (S1p[0]*a0 + S1p[4]*a1) >> 4;
				rows1p[1] = (S1p[1]*a0 + S1p[5]*a1) >> 4;
				rows1p[2] = (S1p[2]*a0 + S1p[6]*a1) >> 4;
				rows1p[3] = (S1p[3]*a0 + S1p[7]*a1) >> 4;
#endif // __ARM_NEON

				ialphap += 2;
				rows1p += 4;
			}
		}
		else
		{
			// hresize two rows
			const unsigned char *S0 = src + srcw * (sy);
			const unsigned char *S1 = src + srcw * (sy+4);

			const short* ialphap = ialpha;
			short* rows0p = rows0;
			short* rows1p = rows1;
			for (int dx = 0; dx < w; dx++)
			{
				int sx = xofs[dx];
				short a0 = ialphap[0];
				short a1 = ialphap[1];

				const unsigned char* S0p = S0 + sx;
				const unsigned char* S1p = S1 + sx;
#if __ARM_NEON
				int16x4_t _a0 = vdup_n_s16(a0);
				int16x4_t _a1 = vdup_n_s16(a1);
				uint8x8_t _S0 = vld1_u8(S0p);
				uint8x8_t _S1 = vld1_u8(S1p);
				int16x8_t _S016 = vreinterpretq_s16_u16(vmovl_u8(_S0));
				int16x8_t _S116 = vreinterpretq_s16_u16(vmovl_u8(_S1));
				int16x4_t _S0low = vget_low_s16(_S016);
				int16x4_t _S1low = vget_low_s16(_S116);
				int16x4_t _S0high = vget_high_s16(_S016);
				int16x4_t _S1high = vget_high_s16(_S116);
				int32x4_t _rows0 = vmull_s16(_S0low, _a0);
				int32x4_t _rows1 = vmull_s16(_S1low, _a0);
				_rows0 = vmlal_s16(_rows0, _S0high, _a1);
				_rows1 = vmlal_s16(_rows1, _S1high, _a1);
				int16x4_t _rows0_sr4 = vshrn_n_s32(_rows0, 4);
				int16x4_t _rows1_sr4 = vshrn_n_s32(_rows1, 4);
				vst1_s16(rows0p, _rows0_sr4);
				vst1_s16(rows1p, _rows1_sr4);
#else
				rows0p[0] = (S0p[0]*a0 + S0p[4]*a1) >> 4;
				rows0p[1] = (S0p[1]*a0 + S0p[5]*a1) >> 4;
				rows0p[2] = (S0p[2]*a0 + S0p[6]*a1) >> 4;
				rows0p[3] = (S0p[3]*a0 + S0p[7]*a1) >> 4;
				rows1p[0] = (S1p[0]*a0 + S1p[4]*a1) >> 4;
				rows1p[1] = (S1p[1]*a0 + S1p[5]*a1) >> 4;
				rows1p[2] = (S1p[2]*a0 + S1p[6]*a1) >> 4;
				rows1p[3] = (S1p[3]*a0 + S1p[7]*a1) >> 4;
#endif // __ARM_NEON

				ialphap += 2;
				rows0p += 4;
				rows1p += 4;
			}
		}

		prev_sy1 = sy + 1;

		// vresize
		short b0 = ibeta[0];
		short b1 = ibeta[1];

		short* rows0p = rows0;
		short* rows1p = rows1;
		unsigned char* Dp = dst + w * 4 * (dy);

#if __ARM_NEON
		int nn = (w * 4) >> 3;
#else
		int nn = 0;
#endif
		int remain = (w * 4) - (nn << 3);

#if __ARM_NEON
#if __aarch64__
		int16x4_t _b0 = vdup_n_s16(b0);
		int16x4_t _b1 = vdup_n_s16(b1);
		int32x4_t _v2 = vdupq_n_s32(2);
		for (; nn>0; nn--)
		{
			int16x4_t _rows0p_sr4 = vld1_s16(rows0p);
			int16x4_t _rows1p_sr4 = vld1_s16(rows1p);
			int16x4_t _rows0p_1_sr4 = vld1_s16(rows0p+4);
			int16x4_t _rows1p_1_sr4 = vld1_s16(rows1p+4);

			int32x4_t _rows0p_sr4_mb0 = vmull_s16(_rows0p_sr4, _b0);
			int32x4_t _rows1p_sr4_mb1 = vmull_s16(_rows1p_sr4, _b1);
			int32x4_t _rows0p_1_sr4_mb0 = vmull_s16(_rows0p_1_sr4, _b0);
			int32x4_t _rows1p_1_sr4_mb1 = vmull_s16(_rows1p_1_sr4, _b1);

			int32x4_t _acc = _v2;
			_acc = vsraq_n_s32(_acc, _rows0p_sr4_mb0, 16);
			_acc = vsraq_n_s32(_acc, _rows1p_sr4_mb1, 16);

			int32x4_t _acc_1 = _v2;
			_acc_1 = vsraq_n_s32(_acc_1, _rows0p_1_sr4_mb0, 16);
			_acc_1 = vsraq_n_s32(_acc_1, _rows1p_1_sr4_mb1, 16);

			int16x4_t _acc16 = vshrn_n_s32(_acc, 2);
			int16x4_t _acc16_1 = vshrn_n_s32(_acc_1, 2);

			uint8x8_t _D = vqmovun_s16(vcombine_s16(_acc16, _acc16_1));

			vst1_u8(Dp, _D);

			Dp += 8;
			rows0p += 8;
			rows1p += 8;
		}
#else
		if (nn > 0)
		{
			asm volatile(
				"vdup.s16   d16, %8         \n"
				"mov        r4, #2          \n"
				"vdup.s16   d17, %9         \n"
				"vdup.s32   q12, r4         \n"
				"pld        [%0, #128]      \n"
				"vld1.s16   {d2-d3}, [%0 :128]!\n"
				"pld        [%1, #128]      \n"
				"vld1.s16   {d6-d7}, [%1 :128]!\n"
				"0:                         \n"
				"vmull.s16  q0, d2, d16     \n"
				"vmull.s16  q1, d3, d16     \n"
				"vorr.s32   q10, q12, q12   \n"
				"vorr.s32   q11, q12, q12   \n"
				"vmull.s16  q2, d6, d17     \n"
				"vmull.s16  q3, d7, d17     \n"
				"vsra.s32   q10, q0, #16    \n"
				"vsra.s32   q11, q1, #16    \n"
				"pld        [%0, #128]      \n"
				"vld1.s32   {d2-d3}, [%0 :128]!\n"
				"vsra.s32   q10, q2, #16    \n"
				"vsra.s32   q11, q3, #16    \n"
				"pld        [%1, #128]      \n"
				"vld1.s32   {d6-d7}, [%1 :128]!\n"
				"vshrn.s32  d20, q10, #2    \n"
				"vshrn.s32  d21, q11, #2    \n"
				"vqmovun.s16 d20, q10        \n"
				"vst1.8     {d20}, [%2]!    \n"
				"subs       %3, #1          \n"
				"bne        0b              \n"
				"sub        %0, #16         \n"
				"sub        %1, #16         \n"
				: "=r"(rows0p), // %0
				"=r"(rows1p), // %1
				"=r"(Dp),     // %2
				"=r"(nn)      // %3
				: "0"(rows0p),
				"1"(rows1p),
				"2"(Dp),
				"3"(nn),
				"r"(b0),      // %8
				"r"(b1)       // %9
				: "cc", "memory", "r4", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12"
				);
		}
#endif // __aarch64__
#endif // __ARM_NEON
		for (; remain; --remain)
		{
			//             D[x] = (rows0[x]*b0 + rows1[x]*b1) >> INTER_RESIZE_COEF_BITS;
			*Dp++ = (unsigned char)(((short)((b0 * (short)(*rows0p++)) >> 16) + (short)((b1 * (short)(*rows1p++)) >> 16) + 2)>>2);
		}

		ibeta += 2;
	}

	delete[] buf;
	delete[] rows0;
	delete[] rows1;
}

/*
IsAvailableMemory : 1. 주어진 메모리 주소의 상태가 물리 주소로 확정되었는지 검사 (실제 사용을 위해 필요)
					2. 주어진 메모리의 보호 속성이 읽기나 쓰기가 가능한지 검사
	Param :
		LPVOID  pMemoryAddr : 검사하고자 하는 메모리의 주소
	Return Value :
		ERROR_SUCCESS : System Error Code, 모든 것이 성공할 경우의 에러코드
		기타 값 : Read/Write 가능한 메모리가 아니면 해당 주소의 Protect Mode 를
					나타내는 0이 아닌 값을 리턴함.
	Reference :
		1. https://docs.microsoft.com/en-us/previous-versions/aa915370(v=msdn.10)/
		2. https://docs.microsoft.com/en-us/windows/desktop/debug/system-error-codes--0-499-/
*/
INT IsAvailableMemory(LPVOID pMemoryAddr)
{
	MEMORY_BASIC_INFORMATION    MemInfo = { 0, };
	SIZE_T  nResult = 0;

	nResult = VirtualQuery(pMemoryAddr, &MemInfo, sizeof(MemInfo));

	if (nResult == 0) // 커널 영역인 경우 VirtualQuery 자체가 Fail함.  
	{
		return -1;
	}
	else if (!(MemInfo.State & MEM_COMMIT))
	{
		return MemInfo.State;
	}
	else if ((MemInfo.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)))
	{
		return  ERROR_SUCCESS; // System Error Code 성공 : Reference 참조
	}
	else
	{
		return  MemInfo.Protect;
	}
}

#if 0
void resize11(int* input, int* output, int sourceWidth, int sourceHeight, int targetWidth, int targetHeight)
{
	// Let's create some lookup tables!
	// you can move them into 2-dimensional arrays to
	// group together values used at the same time to help processor cache
	int *sx = new int[targetWidth ]; // target->source X lookup
	int *sy = new int[targetHeight]; // target->source Y lookup
	int *mx = new int[targetWidth ]; // left pixel's multiplier
	int *my = new int[targetHeight]; // bottom pixel's multiplier

							 // we don't have to calc indexes every time, find out when
	bool *reloadPixels = new bool[targetWidth ];
	bool *shiftPixels = new bool[targetWidth ];
	int  *shiftReloadPixels = new int[targetWidth ]; // can be combined if necessary

	int v; // temporary value
	for (int j = 0; j < targetWidth; j++){
		// (8bit + targetBits + sourceBits) should be < max int
		v = 256 * j * (sourceWidth-1) / (targetWidth-1);

		sx[j] = v / 256;
		mx[j] = v % 256;

		reloadPixels[j] = j ? (sx[j-1] != sx[j] ? 1 : 0)
			: 1; // always load first pixel

				 // if no reload -> then no shift too
		shiftPixels[j]  = j ? (*(sx + (j-1)+1) = sx[j] ? 2 : 0)
			: 0; // nothing to shift at first pixel

		shiftReloadPixels[j] = reloadPixels[j] | shiftPixels[j];
	}

	for (int i = 0; i < targetHeight; i++){
		v = 256 * i * (sourceHeight-1) / (targetHeight-1);
		sy[i] = v / 256;
		my[i] = v % 256;
	}

	int shiftReload;
	int srcIndex;
	int srcRowIndex;
	int offset = 0;
	int lm, rm, tm, bm; // left / right / top / bottom multipliers
	int a, b, c, d;

	for (int i = 0; i < targetHeight; i++){
		srcRowIndex = sy[ i ] * sourceWidth;
		tm = my[i];
		bm = 255 - tm;

		for (int j = 0; j < targetWidth; j++){

			// too much ifs can be too slow, measure.
			// always true for first pixel in a row
			if(shiftReload = shiftReloadPixels[ j ]){
				srcIndex = srcRowIndex + sx[j];
				if(shiftReload & 2){
					a = b;
					c = d;
				}else{
					a = input[ srcIndex                   ];
					c = input[ srcIndex +     sourceWidth ];
				}
				b = input[ srcIndex + 1               ];
				d = input[ srcIndex + 1 + sourceWidth ];
			}

			lm = mx[j];
			rm = 255 - lm;

			// WTF?
			// Input  AA RR GG BB
			// Output RR GG BB AA

			if(j){
				leftOutput = rightOutput ^ 0xFFFFFF00;
			}else{
				leftOutput =
					// blue element
					(((((a&0xFF)*tm
						+ (c&0xFF)*bm)*lm
						) & 0xFF0000) >> 8)

					// green element
					| ((((((a>>8)&0xFF)*tm
						+ ((c>>8)&0xFF)*bm)*lm
						) & 0xFF0000)) // no need to shift

										// red element
					| ((((((a>>16)&0xFF)*tm
						+ ((c>>16)&0xFF)*bm)*lm
						) & 0xFF0000) << 8)
					;
			}

			rightOutput =
				// blue element
				(((((b&0xFF)*tm
					+ (d&0xFF)*bm)*lm
					) & 0xFF0000) >> 8)

				// green element
				| ((((((b>>8)&0xFF)*tm
					+ ((d>>8)&0xFF)*bm)*lm
					) & 0xFF0000)) // no need to shift

									// red element
				| ((((((b>>16)&0xFF)*tm
					+ ((d>>16)&0xFF)*bm)*lm
					) & 0xFF0000) << 8)
				;

			output[offset++] =
				// alpha
				0x000000ff
				| leftOutput
				| rightOutput
				;

		}
	}
}
#endif

/*
//get sorted filelist, not include sub directories.
int getFilelist(char (*sfiles)[NAME_MAX+1], char* folder, char* sfilter, int max, int nOrder)
{
	struct  dirent  **direntp;
	int	 i, n;
	int	 count = 0;
	struct  stat	 statbuf;

	n = scandir(folder, &direntp, 0, alphasort);

	if (n > max)
		n = max;

	if (nOrder == 0)
	{
		for (i = 0; i < n; i++)
		{
			stat(direntp[i]->d_name, &statbuf);

			//check filename length.
			if (strlen(direntp[i]->d_name) > 4)
			{
				char*	ssub = strstr(direntp[i]->d_name, sfilter);
				//printf("ssub = %s\n", ssub);

				if (ssub != NULL && strcmp(ssub, sfilter) == 0)
				{
					//printf("%9d %s\n", statbuf.st_size, direntp[i]->d_name);
					strcpy(sfiles[count++], direntp[i]->d_name);
				}
			}

			free (direntp[i]);
		}
	}
	else
	{
		for (i = n - 1; i >= 0; i--)
		{
			stat(direntp[i]->d_name, &statbuf);

			//check filename length.
			if (strlen(direntp[i]->d_name) > 4)
			{
				char*	ssub = strstr(direntp[i]->d_name, sfilter);
				//printf("ssub = %s\n", ssub);

				if (ssub != NULL && strcmp(ssub, sfilter) == 0)
				{
					//printf("%9d %s\n", statbuf.st_size, direntp[i]->d_name);
					strcpy(sfiles[count++], direntp[i]->d_name);
				}
			}

			free (direntp[i]);
		}
	}

	free (direntp);

	return count;
}
*/
/* Returns a list of files in a directory (except the ones that begin with a dot) */

/**
 * @ingroup HttpFileUpload
 * @brief HTTP POST 메소드로 파일을 업로드한다.
 * @param pszUrl   HTTP URL
 * @param pszFilePath 업로드 파일 full path
 * @returns 성공하면 true 를 리턴하고 그렇지 않으면 false 를 리턴한다.
 * 현재 코드는 chatIdx 필드와 image 필드(파일경로)를 사용하는 코드이므로
 * 필요에 따라 코드를 수정하여 사용하면 된다.
 *	if (HttpUploadFile(_T("http://54.180.43.235:4300/rest/saveImage"), _T("c:\\scpark\\media\\test_image\\waiting_status.png"), 6340))
		AfxMessageBox(_T("Upload success."));
 */
bool HttpUploadFile(CString url, CString filepath, int chatIndex)
{
	bool bRes = false;

	USES_CONVERSION;

	WCHAR* pszUrl;
	WCHAR* pszFilePath;

#ifdef _UNICODE
	pszUrl = T2W(url.GetBuffer());
	pszFilePath = T2W(filepath.GetBuffer());
#else
	pszUrl = T2W(url);
	pszFilePath = T2W(filepath);
#endif

	// pszUrl 에서 host, path 를 가져온다.
	WCHAR* pszHost = NULL;
	DWORD dwFlag = 0;
	int iPort = 80;

	if (!_wcsnicmp(pszUrl, L"http://", 7))
	{
		pszHost = pszUrl + 7;
	}
	else if (!_wcsnicmp(pszUrl, L"https://", 8))
	{
		pszHost = pszUrl + 8;
		dwFlag = INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
		iPort = 4300;
	}
	else
	{
		return false;
	}

	const WCHAR* pszPath = wcsstr(pszHost, L"/");
	if (pszPath == NULL)
	{
		return false;
	}

	std::wstring strHost;
	strHost.append(pszHost, pszPath - pszHost);
	const WCHAR* pszPort = wcsstr(strHost.c_str(), L":");
	if (pszPort)
	{
		iPort = _wtoi(pszPort + 1);
		if (iPort <= 0) return false;
		strHost.erase(pszPort - strHost.c_str());
	}

	// 파일 경로에서 파일 이름을 가져온다.
	int iLen = wcslen(pszFilePath);
	WCHAR* pszFileName = NULL;
	for (int i = iLen - 1; i >= 0; --i)
	{
		if (pszFilePath[i] == L'\\')
		{
			pszFileName = pszFilePath + i + 1;
			break;
		}
	}

	// 파일 크기를 가져온다.
	struct _stat sttStat;
	if (_wstat(pszFilePath, &sttStat) == -1)
	{
		return false;
	}

	FILE* fd;
	_wfopen_s(&fd, pszFilePath, L"rb");
	if (fd == NULL)
	{
		return false;
	}
	else
	{
		char szBuf[8192];
		CInternetSession clsSession;

		// HTTP 연결하고 파일을 전송한다.
		CHttpConnection* pclsHttpConn = clsSession.GetHttpConnection(CString(strHost.c_str()), dwFlag, (INTERNET_PORT)iPort, NULL, NULL);
		if (pclsHttpConn)
		{
			int m_secureFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;

			CHttpFile* pclsHttpFile = pclsHttpConn->OpenRequest(CHttpConnection::HTTP_VERB_POST, CString(pszPath), NULL, 0, NULL, NULL, m_secureFlags);

			if (pclsHttpFile)
			{
				USES_CONVERSION;
				std::string strBoundary = "55CA7C5681214d98912C76366A8042BA";

				// HTTP 요청 header 를 생성한다.
				std::wstring strContentType = L"Content-Type: multipart/form-data; boundary=";
				//strContentType.append(A2T(strBoundary.c_str()));
				strContentType.append(strBoundary.begin(), strBoundary.end());
				strContentType.append(L"\r\n");
				pclsHttpFile->AddRequestHeaders(CString(strContentType.c_str()));// .c_str());

				std::string strBody;
				std::string strChatIndex = std::to_string(chatIndex);

				// body에 chatIdx를 저장한다.
				strBody.append("--");
				strBody.append(strBoundary);
				strBody.append("\r\n");
				strBody.append("Content-Disposition: form-data; name=\"chatIdx\"\r\n\r\n");
				strBody.append(strChatIndex);
				strBody.append("\r\n");

				// body에 파일명을 저장한다.
				strBody.append("--");
				strBody.append(strBoundary);
				strBody.append("\r\n");
				strBody.append("Content-Disposition: form-data; name=\"image\"; filename=\"");
				std::wstring filename(pszFilePath);
				strBody.append(filename.begin(), filename.end());
				strBody.append("\"\r\nContent-Type: application/octet-stream\r\n\r\n");

				while (1)
				{
					iLen = fread(szBuf, 1, sizeof(szBuf), fd);
					if (iLen <= 0) break;
					strBody.append(szBuf, iLen);
				}
				fclose(fd);

				strBody.append("\r\n");
				strBody.append("--");
				strBody.append(strBoundary);
				strBody.append("--\r\n");

				try
				{
					// HTTP 요청 header 를 전송한다.
					pclsHttpFile->SendRequestEx(strBody.length(), HSR_SYNC | HSR_INITIATE);

					// HTTP 요청 body 를 전송한다.
					pclsHttpFile->Write(strBody.c_str(), strBody.length());
					pclsHttpFile->EndRequest(HSR_SYNC);

					// HTTP 응답 body 를 수신한다.
					std::string strResponse;
					DWORD dwCode;
					while (1)
					{
						iLen = pclsHttpFile->Read(szBuf, sizeof(szBuf));
						if (iLen <= 0) break;
						strResponse.append(szBuf, iLen);
					}

					// 응답 코드가 200 OK 인지 확인한다.
					pclsHttpFile->QueryInfoStatusCode(dwCode);
					if (dwCode == HTTP_STATUS_OK)
					{
						bRes = true;
					}
				}
				catch (CInternetException* pclsException)
				{
					// 웹서버 연결에 실패하면 CInternetException 이 발생한다.
				}

				delete pclsHttpFile;
			}
			delete pclsHttpConn;
		}
		fclose(fd);
	}
	return bRes;
}


//서버의 한글명 파일에 대한 처리때문에 request_url()함수 대신 추가하여 테스트 해봤으나
//서버측의 문제인듯하여 우선 이 함수 사용은 보류중...
bool HttpDownloadFile(CString url, CString local_path)
{
	return false;
#if 0
	DWORD dwServiceType = AFX_INET_SERVICE_HTTP;
	CString szServer, szObject, szInfo;

	INTERNET_PORT nPort;
	INTERNET_PROXY_INFO m_proxyinfo;

	CInternetSession m_SessionDownload;
	CHttpConnection* m_pConnection = NULL;
	CHttpFile* m_pHttpFile = NULL;
	CFile FileWrite;

	DWORD d_BytesRead = 0;
	DWORD d_FileSize = 0;

	char szHTTPBuffer[199926];

	ZeroMemory(szHTTPBuffer, sizeof(szHTTPBuffer));

	//start Download Routine
	::AfxParseURL(url.GetBuffer(url.GetLength()), dwServiceType, szServer, szObject, nPort);

	try
	{
		m_pConnection = m_SessionDownload.GetHttpConnection(szServer, INTERNET_FLAG_KEEP_CONNECTION, nPort, NULL, NULL);
		m_pHttpFile = m_pConnection->OpenRequest("GET", szObject, NULL, 0, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION);
	}
	catch (CInternetException* m_pException)
	{
		//exception found
		//lots of clean up code
		return false;
	}

	if (m_pHttpFile)
	{
		if (!FileWrite.Open("c:\\scpark\\모듈.pdf", CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyNone))
		{
			//exception found
			//lots of clean up code
			return false;
		}

		try
		{
			m_pHttpFile->SendRequest(NULL);
		}
		catch (CInternetException* m_pException)
		{
			//exception found
			//lots of clean up code
			return false;
		}

		m_pHttpFile->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, d_FileSize);
		m_pHttpFile->QueryInfo(HTTP_QUERY_MAX, szInfo);

		while (d_BytesRead = m_pHttpFile->Read((void*)szHTTPBuffer, 199926))
		{
			FileWrite.Write((void*)szHTTPBuffer, d_BytesRead);
			memset(szHTTPBuffer, 0, sizeof(szHTTPBuffer));
		}

		szServer.Empty();
		szObject.Empty();

		m_SessionDownload.Close();
		m_pConnection->Close();
		m_pHttpFile->Close();

		//delete m_SessionDownload;
		delete m_pConnection;
		delete m_pHttpFile;

		FileWrite.Close();
	}
	return true;
#endif
#if 0
	bool bRes = false;

	USES_CONVERSION;

	WCHAR* pszUrl;
	WCHAR* pszFilePath;

#ifdef _UNICODE
	pszUrl = T2W(url.GetBuffer());
	pszFilePath = T2W(filepath.GetBuffer());
#else
	pszUrl = T2W(url);
	pszFilePath = T2W(local_path);
#endif

	// pszUrl 에서 host, path 를 가져온다.
	WCHAR* pszHost = NULL;
	DWORD dwFlag = INTERNET_FLAG_KEEP_CONNECTION;
	int iPort = 80;

	if (!_wcsnicmp(pszUrl, L"http://", 7))
	{
		pszHost = pszUrl + 7;
	}
	else if (!_wcsnicmp(pszUrl, L"https://", 8))
	{
		pszHost = pszUrl + 8;
		dwFlag = INTERNET_FLAG_SECURE;
		iPort = 443;
	}
	else
	{
		return false;
	}

	const WCHAR* pszPath = wcsstr(pszHost, L"/");
	if (pszPath == NULL)
	{
		return false;
	}

	std::wstring strHost;
	strHost.append(pszHost, pszPath - pszHost);
	const WCHAR* pszPort = wcsstr(strHost.c_str(), L":");
	if (pszPort)
	{
		iPort = _wtoi(pszPort + 1);
		if (iPort <= 0) return false;
		strHost.erase(pszPort - strHost.c_str());
	}

	// 파일 경로에서 파일 이름을 가져온다.
	int iLen = wcslen(pszFilePath);
	WCHAR* pszFileName = NULL;
	for (int i = iLen - 1; i >= 0; --i)
	{
		if (pszFilePath[i] == L'\\')
		{
			pszFileName = pszFilePath + i + 1;
			break;
		}
	}

	// 파일 크기를 가져온다.
	//struct _stat sttStat;
	//if (_wstat(pszFilePath, &sttStat) == -1)
	//{
	//	return false;
	//}

	//FILE* fd;
	//_wfopen_s(&fd, pszFilePath, L"wb");

	//if (fd == NULL)
	//{
	//	return false;
	//}
	//else
	{
		char szBuf[8192];
		//CString szServer, szObject, szInfo;
		CInternetSession clsSession;
		CFile FileWrite;
		DWORD dBytesRead = 0;
		DWORD dFileSize = 0;
		CString szInfo;
		char szHTTPBuffer[4096];

		//::AfxParseURL(url.GetBuffer(url.GetLength()), AFX_INET_SERVICE_HTTP, 

		// HTTP 연결하고 파일을 전송한다.
		CHttpConnection* pclsHttpConn = clsSession.GetHttpConnection(CString(strHost.c_str()), dwFlag, (INTERNET_PORT)iPort, NULL, NULL);
		if (pclsHttpConn)
		{
			CHttpFile* pclsHttpFile = pclsHttpConn->OpenRequest(CHttpConnection::HTTP_VERB_GET, CString(pszPath));
			if (pclsHttpFile)
			{
				// HTTP 요청 header 를 생성한다.
				std::wstring strContentType = L"Accept: text";
				pclsHttpFile->AddRequestHeaders(CString(strContentType.c_str()));// .c_str());

				if (!FileWrite.Open(local_path, CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyNone))
				{

				}

				pclsHttpFile->SendRequest(NULL);

				pclsHttpFile->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, dFileSize);
				pclsHttpFile->QueryInfo(HTTP_QUERY_MAX, szInfo);

				while (dBytesRead = pclsHttpFile->Read((void*)szHTTPBuffer, 4096))
				{
					FileWrite.Write((void*)szHTTPBuffer, dBytesRead);
					memset(szHTTPBuffer, 0, sizeof(szHTTPBuffer));
					TRACE(_T("%d bytes write.\n"), dBytesRead);
				}

				FileWrite.Close();
				pclsHttpFile->Close();

				delete pclsHttpFile;
			}
			delete pclsHttpConn;
		}
		//fclose(fd);
	}
	return bRes;
#endif
}

//webView2 Runtime
bool is_WebView2Runtime_installed()
{
	//VS에서 이 프로젝트를 실행하여 WebView2를 설치하면 HKEY_CURRENT_USER에 기록이 반영되고
	//urlscheme을 통해서 KoinoAVCSupporter.exe -> WebView2를 설치하면 HKEY_LOCAL_MACHINE에 기록이 남는다.
	//webview2를 uninstall하면(Microsoft Edge WebView2 런타임) 두 곳 모두 제거된다.
	LONG result1 = IsExistRegistryKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"));
	LONG result2 = IsExistRegistryKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"));

	if (result1 != ERROR_SUCCESS && result2 != ERROR_SUCCESS)
		return false;
	return true;
}

bool install_WebView2Runtime(CString runtimeExePath, bool silentInstall)
{
	LONG result1 = -1;// IsExistRegistryKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"));
	LONG result2 = -1;// IsExistRegistryKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"));

	if (!PathFileExists(runtimeExePath))
		return false;

	ShellExecute(NULL, _T("open"), runtimeExePath, (silentInstall ? _T("/silent /install") : NULL), NULL, SW_SHOWNORMAL);

	while (result1 != ERROR_SUCCESS && result2 != ERROR_SUCCESS)
	{
		Wait(1000);
		result1 = IsExistRegistryKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"));
		result2 = IsExistRegistryKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"));
		TRACE(_T("waiting for installation to complete WebView2 Runtime...\n"));
	}
	TRACE(_T("WebView2 Runtime installed successfully.\n"));

	if (silentInstall)
		return true;

	//gLog.write(_T("WebView2 Runtime installed successfully."));
	Wait(1000);

	//HANDLE hProcess = NULL;
	HWND hWnd = NULL;
	int wait_count = 0;
	//설치가 완료되면 설치와 관련된 창은 모두 닫아준다.
	do
	{
		hWnd = get_hwnd_by_exe_file(_T("MicrosoftEdgeWebView2RuntimeInstallerX64.exe"));
		//hProcess = GetProcessHandleFromHwnd(hWnd);
		if (hWnd != INVALID_HANDLE_VALUE)
			TerminateProcess(hWnd, 0);
		else
			break;

		Wait(500);

		wait_count++;

		if (wait_count > 10)
			break;
	} while (hWnd != INVALID_HANDLE_VALUE);
	CloseHandle(hWnd);

	wait_count = 0;
	do
	{
		hWnd = get_hwnd_by_exe_file(_T("MicrosoftEdgeUpdate.exe"));
		if (hWnd != INVALID_HANDLE_VALUE)
			TerminateProcess(hWnd, 0);
		else
			break;
		Wait(500);

		wait_count++;

		if (wait_count > 10)
			break;

	} while (hWnd != INVALID_HANDLE_VALUE);

	CloseHandle(hWnd);

	return true;
}
/*
BOOL PlayResource(LPTSTR lpName)
{
	BOOL bRtn;
	LPTSTR lpRes;
	HRSRC hResInfo;
	HGLOBAL hRes;

	// Find the WAVE resource. 

	hResInfo = FindResource(NULL, lpName, _T("WAVE"));
	if (hResInfo == NULL)
		return FALSE;

	// Load the WAVE resource. 

	hRes = LoadResource(NULL, hResInfo);
	if (hRes == NULL)
		return FALSE;

	// Lock the WAVE resource and play it. 

	lpRes = (LPTSTR)LockResource(hRes);
	if (lpRes != NULL) {
		bRtn = sndPlaySound(lpRes, SND_MEMORY | SND_SYNC |
			SND_NODEFAULT);
		UnlockResource(hRes);
	}
	else
		bRtn = 0;

	// Free the WAVE resource and return success or failure. 

	FreeResource(hRes);
	return bRtn;
}
*/

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	/*
	UINT num, size;
	Gdiplus::GetImageEncodersSize(&num, &size);
	Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
	bool found = false;
	for (UINT ix = 0; !found && ix < num; ++ix) {
		if (0 == _wcsicmp(pImageCodecInfo[ix].MimeType, format) == 0) {
			*pClsid = pImageCodecInfo[ix].Clsid;
			found = true;
		}
	}
	free(pImageCodecInfo);
	return found;
	*/
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

bool save(Gdiplus::Bitmap* bitmap, CString filename)
{
	if (!bitmap)
		return false;

	CLSID				encoderClsid;

	CString ext = filename.Right(3).MakeLower();//GetFileExtension(filename).MakeLower();

	if (ext == _T("jpg") || ext == _T("jpeg"))
		GetEncoderClsid(L"image/jpeg", &encoderClsid);
	else if (ext == _T("png"))
		GetEncoderClsid(L"image/png", &encoderClsid);
	else if (ext == _T("gif"))
		GetEncoderClsid(L"image/gif", &encoderClsid);
	else if (ext == _T("bmp"))
		GetEncoderClsid(L"image/bmp", &encoderClsid);
	else
	{
		AfxMessageBox(_T("처리 코드가 추가되지 않은 포맷. 코드 수정 필요"));
		return false;
	}
	/*
	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;

	// Save the image as a JPEG with quality level 0.
	encoderParameters.Parameter[0].Value = &quality;
	*/
	Gdiplus::Status s;

	s = bitmap->Save(CStringW(filename), &encoderClsid);// , & encoderParameters);

	if (s == Gdiplus::Ok)
		return true;

	return false;
}

//paragraph text 정보를 dc에 출력할 때 출력 크기를 계산하고 각 텍스트가 출력될 위치까지 CSCParagraph 멤버에 저장한다.
CRect calc_text_size(CRect rc, CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, LOGFONT* lf, DWORD align)
{
	if (para.empty())
		return CRect();

	int i, j;
	int sx = 0;
	int sy = 0;				//각 라인의 시작 위치(높이값 누적)
	int total_text_height;
	CRect rect_text;
	CFont font, * pOldFont;
	LOGFONT lf_origin;

	Gdiplus::Graphics g(pDC->m_hDC);

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	Gdiplus::StringFormat sf;

	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);
	//sf.SetFormatFlags(Gdiplus::StringFormatFlagsMeasureTrailingSpaces);

	int max_width = 0;
	int max_width_line = 0;

	memcpy(&lf_origin, lf, sizeof(LOGFONT));

	for (i = 0; i < para.size(); i++)
	{
		CSize sz_text = CSize(0, 0);

		for (j = 0; j < para[i].size(); j++)
		{
			CSize sz;
#if 0
			pOldFont = select_paragraph_font(pDC, para, i, j, lf, &font);

			//GetTextExtent()와 DrawText(DT_CALCRECT)로 구한 크기는 동일하며 italic은 약간 잘림.
			sz = pDC->GetTextExtent(para[i][j].text);
			//GetTextExtentExPoint(dc.m_hDC, m_paragraph[i].text, m_paragraph[i].text.GetLength(), 0, NULL, NULL, &sz);

			//if (para[i][j].italic)
			//{
			//	TEXTMETRIC tm;
			//	GetTextMetrics(pDC->m_hDC, &tm);
			//	if (tm.tmOverhang > 0)
			//		sz.cx += tm.tmOverhang;
			//	else
			//		sz.cx += (pDC->GetTextExtent(_T("M")).cx / 4);
			//}

			para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
#else
			Gdiplus::Font* font = NULL;
			get_paragraph_font(g, para, i, j, &font);
			Gdiplus::RectF boundRect;
			//g.MeasureString(CStringW(m_para[i][j].text), -1, font, Gdiplus::PointF(0, 0), &boundRect);
			g.MeasureString(CStringW(para[i][j].text), -1, font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &boundRect);
			//Gdiplus::Region rgn[] = Gdiplus::Region[1];
			//g.MeasureCharacterRanges(CStringW(para[i][j].text), -1, font, boundRect, &sf, 1, &rgn);
			//boundRect = rgn[0].GetBounds(g);
			sz.cx = boundRect.Width;
			sz.cy = boundRect.Height;
			para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
#endif
			TRACE(_T("[%d][%d] text = %s, sz = %dx%d, r = %s\n"), i, j, para[i][j].text, sz.cx, sz.cy, get_rect_info_string(para[i][j].r));
			sz_text.cx += sz.cx;

			//한 라인에서 가장 cy가 큰 값을 기억시킨다.
			sz_text.cy = std::max(sz_text.cy, sz.cy);
		}

		//각 라인들 중에서 최대 너비를 구한다.
		if (sz_text.cx > max_width)
		{
			max_width = sz_text.cx;
			max_width_line = i;
		}

		//각 라인 시작 위치는 누적된다.
		sy += sz_text.cy;
	}

	total_text_height = sy;

	font.DeleteObject();

	//한 라인내에서 height가 가장 높은 항목으로 통일시키느냐? 아니면 각자의 높이를 그대로 유지하느냐...
	//for (i = 0; i < m_paragraph.size(); i++)
	//{
	//	m_paragraph[i].r.bottom = m_paragraph[i].r.top + m_sz_text.cy;
	//}

	//align 옵션에 따른 보정
	/*
	DWORD dwStyle = GetStyle();
	DWORD dwText = DT_NOCLIP;// | DT_WORDBREAK;

	if (m_dwStyle == 0)
	{
		MAP_STYLE(SS_LEFT, DT_LEFT);
		MAP_STYLE(SS_RIGHT, DT_RIGHT);
		MAP_STYLE(SS_CENTER, DT_CENTER);
		MAP_STYLE(SS_NOPREFIX, DT_NOPREFIX);
		MAP_STYLE(SS_WORDELLIPSIS, DT_WORD_ELLIPSIS);
		MAP_STYLE(SS_ENDELLIPSIS, DT_END_ELLIPSIS);
		MAP_STYLE(SS_PATHELLIPSIS, DT_PATH_ELLIPSIS);
	}
	*/

	CRect margin;

	//align에 따른 보정
	if (align & DT_CENTER)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//아이콘을 포함하여 center에 표시할 지, 텍스트만 center에 표시할 지...
			//if (m_hIcon)
			//	total_width -= (m_sz_icon.cx + 4);

			//cx에서 total_width/2를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.CenterPoint().x - total_width / 2;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else if (align & DT_RIGHT)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//rc.right에서 total_width를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.right - margin.right - total_width;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else //SS_LEFT (default)
	{
		if (false)//m_hIcon)
		{
			for (i = 0; i < para.size(); i++)
			{
				for (j = 0; j < para[i].size(); j++)
				{
					//para[i][j].r.OffsetRect(m_margin.left + m_sz_icon.cx + 4, 0);
				}
			}
		}
	}

	if (align & DT_VCENTER)// SS_CENTERIMAGE)
	{
		//전체 높이에서 전체 텍스트 높이 합계를 뺀 1/2 만큼 shift 시킨다.
		sy = (rc.Height() - total_text_height) / 2;
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, sy);
			}
		}
	}
	else //top align
	{
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, 0);// m_margin.top);
			}
		}
	}


	if (para.size() > 0)
	{
		//m_pt_icon.x = m_para[m_max_width_line][0].r.left - m_sz_icon.cx - 4;
		//아이콘을 top 정렬하느냐, 모든 라인의 vcenter에 정렬하느냐...
		//m_pt_icon.y = m_para[0][0].r.top;

		rect_text.left = para[max_width_line][0].r.left;	//최대 넓이 라인의 0번 아이템의 left
		rect_text.top = para[0][0].r.top;					//최상단 항목의 top
		rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;	//최대 넓이 라인의 마지막 항목의 right
		rect_text.bottom = para[0][0].r.top + total_text_height;	//최상단 항목의 top + 전체 텍스트 높이
	}
	else
	{
		//m_pt_icon.x = sx - m_sz_icon.cx;
		//m_pt_icon.y = sy - m_sz_icon.cy / 2;

		//m_rect_text = make_rect(m_pt_icon.x, m_pt_icon.y, m_sz_icon.cx, m_sz_icon.cy);
	}

	//text 크기에 맞춰 컨트롤의 크기를 조정하는 것은 해당 윈도우에서 처리할 일이다.
	/*
	if (false)//m_auto_ctrl_size)
	{
		if (rect_text.Width() > rc.Width() || rect_text.Height() > rc.Height())
		{
			//MoveWindow(m_rect_text);
			SetWindowPos(NULL, 0, 0, rect_text.Width(), rect_text.Height(), SWP_NOMOVE | SWP_NOZORDER);
		}
	}
	*/

	return rect_text;
}

//현재는 calc_text_size()에서만 사용되는 함수로 주어진 폰트로 설정하고 pOldFont를 리턴한다.
CFont* select_paragraph_font(CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, int line, int index, LOGFONT* lf_origin, CFont* font)
{
	font->DeleteObject();

	LOGFONT lf;
	memcpy(&lf, lf_origin, sizeof(LOGFONT));

	_tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), para[line][index].name);

	lf.lfHeight = get_pixel_size_from_font_size(NULL, para[line][index].size);
	lf.lfWeight = para[line][index].bold ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = para[line][index].italic;
	lf.lfUnderline = para[line][index].underline;
	lf.lfStrikeOut = para[line][index].strike;

	font->CreateFontIndirect(&lf);
	return (CFont*)pDC->SelectObject(font);
}

void get_paragraph_font(Gdiplus::Graphics& g, std::deque<std::deque<CSCParagraph>>& para, int line, int index, Gdiplus::Font** font)
{
	Gdiplus::Unit unit = g.GetPageUnit();
	float fDpiX = g.GetDpiX();
	float fDpiY = g.GetDpiY();

	int logPixelsY = ::GetDeviceCaps(NULL, LOGPIXELSY);
	//Gdiplus::REAL emSize = (Gdiplus::REAL)MulDiv(font_size, 96, logPixelsY);
	float emSize = fDpiY * para[line][index].size / 96.0;

	//if (fontFamily == NULL)
	Gdiplus::FontFamily fontFamily((WCHAR*)(const WCHAR*)CStringW(para[line][index].name));

	//if (!fontFamily->IsAvailable())
	//{
	//	delete fontFamily;
	//	fontFamily = new Gdiplus::FontFamily(CStringW("Arial"));
	//}

	int font_style = 0;
	if (para[line][index].bold)
		font_style |= Gdiplus::FontStyleBold;
	if (para[line][index].italic)
		font_style |= Gdiplus::FontStyleItalic;
	if (para[line][index].underline)
		font_style |= Gdiplus::FontStyleUnderline;
	if (para[line][index].strike)
		font_style |= Gdiplus::FontStyleStrikeout;

	Gdiplus::Font ff(&fontFamily, emSize, font_style);
	*font = ff.Clone();

	//if (fontFamily)
		//delete fontFamily;
}

//#define USING_HDC
void draw_text(Gdiplus::Graphics& g, std::deque<std::deque<CSCParagraph>>& para, LOGFONT* lf)
{
	int i, j;
	CFont font, * pOldFont = NULL;

	Gdiplus::StringFormat sf;

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	Gdiplus::Unit unit = g.GetPageUnit();
	float fDpiX = g.GetDpiX();
	float fDpiY = g.GetDpiY();

	int logPixelsY = ::GetDeviceCaps(NULL, LOGPIXELSY);
	//Gdiplus::REAL emSize = (Gdiplus::REAL)MulDiv(font_size, 96, logPixelsY);

	//g를 이용해서 pDC를 구해서 사용하는 경우는 g.ReleaseHDC(hdc);를 호출하기 전까지는 g의 어떤 함수 사용도 하지 않아야 한다.
	// Make GDI calls, but don't call any methods
	// on g until after the call to ReleaseHDC.
#ifdef USING_HDC
	HDC hdc = g.GetHDC();
	CDC* pDC = CDC::FromHandle(hdc);
	pDC->SetBkMode(TRANSPARENT);
#endif
	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

	//dc.SetBkColor()로 지정된 배경색을 설정하면 편하지만
	//글자 속성에 따라 그 높낮이가 다른 경우도 있다.
	//따라서 recalc_text_size()에서 max height를 모든 paragraph에 적용했으며
	//여기서도 배경색으로 칠한 뒤 텍스트를 표시한다.

	for (i = 0; i < para.size(); i++)
	{
		for (j = 0; j < para[i].size(); j++)
		{
#ifdef USING_HDC
			pOldFont = select_paragraph_font(pDC, para, i, j, lf, &font);

			//text 배경색을 칠하고
			if (para[i][j].cr_back.GetA() != 0)
				pDC->FillSolidRect(para[i][j].r, para[i][j].cr_back.ToCOLORREF());

			//text를 출력한다.
			pDC->SetTextColor(para[i][j].cr_text.ToCOLORREF());
			pDC->DrawText(para[i][j].text, para[i][j].r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
			//draw_text(g, m_para[i][j].r, m_para[i][j].cr_text, m_para[i][j].text, m_para[i][j].size, m_para[i][j].);
			//g.DrawString(CStringW(m_para[i][j].text), m_para[i][j].text.GetLength(), font, Gdiplus::PointF((Gdiplus::REAL)m_para[i][j].r.left, (Gdiplus::REAL)m_para[i][j].r.top), &sf);
			pDC->SelectObject(pOldFont);
#else
			//Gdiplus::Font* font;
			Gdiplus::FontFamily ff((WCHAR*)(const WCHAR*)CStringW(para[i][j].name));

			int font_style = 0;
			if (para[i][j].bold)
				font_style |= Gdiplus::FontStyleBold;

			//get_paragraph_font(g, para, i, j, &font);
			float emSize = fDpiY * para[i][j].size / 96.0;
			Gdiplus::Font font(&ff, emSize, font_style);


			//text 배경색을 칠하고
			draw_rectangle(g, para[i][j].r, Gdiplus::Color::Transparent, para[i][j].cr_back);

			//text를 출력한다.
			Gdiplus::SolidBrush text_brush(para[i][j].cr_text);


			if (false)//para[i][j].thickness > 0.0)
			{
				Gdiplus::GraphicsPath str_path;

				//emSize
				//fontsize=
				//CSCShapeDlg에서는 96주면 넘쳐나서 안그려지고 68을 주면 DrawString()과 유사.
				//MiniClock은 fontsize=14일 때 16.5이하여야 그려진다. 이 변환식은?
				str_path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), &ff,
					font_style, 68, CRect2GpRectF(para[i][j].r), sf.GenericTypographic());

				Gdiplus::Pen   gp(para[i][j].cr_stroke, 5.0f);// thickness);
				//gp.SetLineJoin(Gdiplus::LineJoinMiter);
				Gdiplus::SolidBrush gb(para[i][j].cr_text);

				g.DrawPath(&gp, &str_path);
				g.FillPath(&gb, &str_path);
			}
			else
			{
				g.DrawString(CStringW(para[i][j].text),
					para[i][j].text.GetLength(), &font,
					Gdiplus::PointF((Gdiplus::REAL)para[i][j].r.left, (Gdiplus::REAL)para[i][j].r.top), sf.GenericTypographic(), &text_brush);
			}
#endif

			//if (m_draw_word_hover_rect && CPoint(i, j) == m_pos_word_hover)
			//{
			//	draw_rectangle(g, m_para[i][j].r, Gdiplus::Color::Red, Gdiplus::Color(96, 255, 255, 255));
			//}
		}
	}

	//텍스트 출력 영역 확인용
#ifdef _DEBUG
	//draw_rectangle(g, m_rect_text, Gdiplus::Color::Blue, Gdiplus::Color::Transparent, 1);
#endif
	//TRACE(_T("m_rect_text = %s\n"), get_rect_info_string(m_rect_text));

#ifdef USING_HDC
	font.DeleteObject();
	pDC->SelectObject(pOldFont);

	g.ReleaseHDC(hdc);
#endif
}

void draw_text(CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, LOGFONT* lf)
{
	Gdiplus::Graphics g(pDC->m_hDC);
	draw_text(g, para, lf);
}

CString json_value(CString json, CString key)
{
	bool isFindToken = false;
	CString result;
	CString temp;

	int i;
	int index = json.Find(key);

	if (index == -1)
		return _T("");

	for (i = index; i < json.GetLength(); i++)
	{
		wchar_t ch = json.GetAt(i);

		if (isFindToken)
		{
			temp += ch;
		}

		if (ch == ':')
		{
			isFindToken = TRUE;
		}

		if (ch == ',' || ch == '}')
		{
			break;
		}
	}

	result.Format(_T("%s"), temp);

	result.Replace(_T("]"), _T(""));
	result.Replace(_T("}"), _T(""));
	result.Replace(_T("\""), _T(""));
	result.Replace(_T(","), _T(""));
	result.Trim();

	return result;
}

#if 0
//파일 속성 변경 목적의 코드지만 좀 더 테스트가 필요하고 XP에서는 지원되지 않으므로 우선 주석처리함.
HRESULT PrintProperty(IPropertyStore* pps, REFPROPERTYKEY key, PCWSTR pszCanonicalName)
{
	PROPVARIANT propvarValue = { 0 };
	HRESULT hr = pps->GetValue(key, &propvarValue);
	if (SUCCEEDED(hr))
	{
		PWSTR pszDisplayValue = NULL;
		hr = PSFormatForDisplayAlloc(key, propvarValue, PDFF_DEFAULT, &pszDisplayValue);
		if (SUCCEEDED(hr))
		{
			wprintf(L"%s = %s\n", pszCanonicalName, pszDisplayValue);
			CoTaskMemFree(pszDisplayValue);
		}
		PropVariantClear(&propvarValue);
	}
	return hr;
}


HRESULT GetPropertyStore(CString pszFilename, GETPROPERTYSTOREFLAGS gpsFlags, IPropertyStore** ppps)
{
	TCHAR szExpanded[MAX_PATH];
	HRESULT hr = ExpandEnvironmentStrings(pszFilename, szExpanded, ARRAYSIZE(szExpanded)) ? S_OK : HRESULT_FROM_WIN32(GetLastError());

	USES_CONVERSION;

#ifdef _UNICODE
	hr = SHGetPropertyStoreFromParsingName(pszFilename, NULL, gpsFlags, IID_PPV_ARGS(ppps));
#else
	hr = SHGetPropertyStoreFromParsingName(A2CW(LPCTSTR(pszFilename)), NULL, gpsFlags, IID_PPV_ARGS(ppps));
#endif
	//if (SUCCEEDED(hr))
	{
		//WCHAR szAbsPath[MAX_PATH];
		//hr = _wfullpath(szAbsPath, szExpanded, ARRAYSIZE(szAbsPath)) ? S_OK : E_FAIL;
		//if (SUCCEEDED(hr))
		//{
			//hr = SHGetPropertyStoreFromParsingName(szAbsPath, NULL, gpsFlags, IID_PPV_ARGS(ppps));
		//}
	}
	return hr;
}

HRESULT EnumerateProperties(PCWSTR pszFilename)
{
	IPropertyStore* pps = NULL;

	// Call the helper to get the property store for the initialized item
	// Note that as long as you have the property store, you are keeping the file open
	// So always release it once you are done.

	HRESULT hr = GetPropertyStore(pszFilename, GPS_DEFAULT, &pps);
	if (SUCCEEDED(hr))
	{
		// Retrieve the number of properties stored in the item.
		DWORD cProperties = 0;
		hr = pps->GetCount(&cProperties);
		if (SUCCEEDED(hr))
		{
			for (DWORD i = 0; i < cProperties; i++)
			{
				// Get the property key at a given index.
				PROPERTYKEY key;
				hr = pps->GetAt(i, &key);
				if (SUCCEEDED(hr))
				{
					// Get the canonical name of the property
					PWSTR pszCanonicalName = NULL;
					hr = PSGetNameFromPropertyKey(key, &pszCanonicalName);
					if (SUCCEEDED(hr))
					{
						hr = PrintProperty(pps, key, pszCanonicalName);
						TRACE(_T("%s\n"), pszCanonicalName);
						CoTaskMemFree(pszCanonicalName);
					}
				}
			}
		}
		pps->Release();
	}
	else
	{
		wprintf(L"Error %x: getting the propertystore for the item.\n", hr);
	}
	return hr;
}

HRESULT SetPropertyValue(PCWSTR pszFilename, PCWSTR pszCanonicalName, PCWSTR pszValue)
{
	// Convert the Canonical name of the property to PROPERTYKEY
	PROPERTYKEY key;
	HRESULT hr = PSGetPropertyKeyFromName(pszCanonicalName, &key);
	if (SUCCEEDED(hr))
	{
		IPropertyStore* pps = NULL;

		// Call the helper to get the property store for the
		// initialized item
		hr = GetPropertyStore(pszFilename, GPS_READWRITE, &pps);
		if (SUCCEEDED(hr))
		{
			PROPVARIANT propvarValue = { 0 };
			hr = InitPropVariantFromString(pszValue, &propvarValue);
			if (SUCCEEDED(hr))
			{
				hr = PSCoerceToCanonicalValue(key, &propvarValue);
				if (SUCCEEDED(hr))
				{
					// Set the value to the property store of the item.
					hr = pps->SetValue(key, propvarValue);
					if (SUCCEEDED(hr))
					{
						// Commit does the actual writing back to the file stream.
						hr = pps->Commit();
						if (SUCCEEDED(hr))
						{
							wprintf(L"Property %s value %s written successfully \n", pszCanonicalName, pszValue);
						}
						else
						{
							wprintf(L"Error %x: Commit to the propertystore failed.\n", hr);
						}
					}
					else
					{
						wprintf(L"Error %x: Set value to the propertystore failed.\n", hr);
					}
				}
				PropVariantClear(&propvarValue);
			}
			pps->Release();
		}
		else
		{
			wprintf(L"Error %x: getting the propertystore for the item.\n", hr);
		}
	}
	else
	{
		wprintf(L"Invalid property specified: %s\n", pszCanonicalName);
	}
	return hr;
}

CString	set_file_property(CString sFilePath, CString sProperty, CString value)
{
	//EnumerateProperties(sFilePath);
	USES_CONVERSION;

#ifdef _UNICODE
	SetPropertyValue(sFilePath, sProperty, value);
#else
	SetPropertyValue(A2CW(LPCTSTR(sFilePath)), A2CW(LPCTSTR(sProperty)), A2CW(LPCTSTR(value)));
#endif
	return CString();
}
#endif

#ifndef REMOTESDK
CString base64_encode(CString in)
{
	USES_CONVERSION;

	std::string sstr = CT2CA(in);
	sstr = base64_encode(sstr);
	CString str = CString(sstr.c_str());
	return str;
}

CString base64_decode(CString in)
{
	std::string sstr = CT2CA(in);
	sstr = base64_decode(sstr);
	CString str = CString(sstr.c_str());
	return str;
}

std::string base64_decode(const std::string& in)
{
	static const std::string b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	int val = 0, valb = -8;
	std::string out;
	std::vector<int> T(256, -1);

	for (int i = 0; i < 64; i++)
		T[b[i]] = i;

	for (uint8_t c : in)
	{
		if (T[c] == -1)
			break;

		val = (val << 6) + T[c];
		valb += 6;

		if (valb >= 0)
		{
			out.push_back(char((val >> valb) & 0xFF));
			valb -= 8;
		}
	}
	return out;
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';

	}

	return ret;
}

//https://ikcoo.tistory.com/213
std::string base64_encode(const std::string& in)
{
	std::string out;
	int val = 0, valb = -6;

	for (uint8_t c : in)
	{
		val = (val << 8) + c;
		valb += 8;

		while (valb >= 0)
		{
			out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}

	if (valb > -6)
		out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);

	while (out.size() % 4)
		out.push_back('=');

	return out;
}
#endif

CRequestUrlParams::CRequestUrlParams(CString _full_url, CString _verb, bool _is_https, std::deque<CString>* _headers, CString _body, CString _local_file_path)
{
	full_url = _full_url;
	
	parse_url(full_url, ip, port, sub_url, _is_https);
	is_https = _is_https;

	CRequestUrlParams(ip, port, sub_url, _verb, is_https, _headers, _body, _local_file_path);
}

// recreate the combo box by copying styles etc, and list items
// and applying them to a newly created control
BOOL recreate_combobox(CComboBox* pCombo, LPVOID lpParam/*=0*/)
{
	if (pCombo == NULL)
		return FALSE;
	if (pCombo->GetSafeHwnd() == NULL)
		return FALSE;

	CWnd* pParent = pCombo->GetParent();
	if (pParent == NULL)
		return FALSE;

	// get current attributes
	DWORD dwStyle = pCombo->GetStyle();
	DWORD dwStyleEx = pCombo->GetExStyle();
	CRect rc;
	pCombo->GetDroppedControlRect(&rc);
	pParent->ScreenToClient(&rc);	// map to client co-ords
	UINT nID = pCombo->GetDlgCtrlID();
	CFont* pFont = pCombo->GetFont();
	CWnd* pWndAfter = pCombo->GetNextWindow(GW_HWNDPREV);

	// get the currently selected text (and whether it is a valid list selection)
	CString sCurText;
	int nCurSel = pCombo->GetCurSel();
	BOOL bItemSelValid = nCurSel != -1;
	if (bItemSelValid)
		pCombo->GetLBText(nCurSel, sCurText);
	else
		pCombo->GetWindowText(sCurText);

	// copy the combo box items into a temp combobox (not sorted)
	// along with each item's data
	CComboBox comboNew;
	
	if (!comboNew.CreateEx(dwStyleEx, _T("COMBOBOX"), _T(""), dwStyle, rc, pParent, nID, lpParam))
	//if (!comboNew.Create(dwStyle, rc, pParent, nID))
		return FALSE;

	comboNew.SetFont(pFont);
	int nNumItems = pCombo->GetCount();
	for (int n = 0; n < nNumItems; n++)
	{
		CString sText;
		pCombo->GetLBText(n, sText);
		int nNewIndex = comboNew.AddString(sText);
		comboNew.SetItemData(nNewIndex, pCombo->GetItemData(n));
	}
	// re-set selected text
	if (bItemSelValid)
		comboNew.SetCurSel(comboNew.FindStringExact(-1, sCurText));
	else
		comboNew.SetWindowText(sCurText);

	// destroy the existing window, then attach the new one
	pCombo->DestroyWindow();
	HWND hwnd = comboNew.Detach();
	pCombo->Attach(hwnd);

	// position correctly in z-order
	pCombo->SetWindowPos(pWndAfter == NULL ?
		&CWnd::wndBottom :
		pWndAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	return TRUE;
}

//콘솔 명령인지 윈도우 프로그램인지 구분
bool is_gui_application(CString fullPath)
{
	//fullPath에서 실행파일명 또는 cmd를 추출한다.
	CString cmd = fullPath;

	if (fullPath[0] == '\"')
		cmd = fullPath;
	else if (fullPath.Find(' ') > 0)
		cmd = fullPath.Left(fullPath.Find(' '));

	if (PathFileExists(cmd) == FALSE)
	{
		LPTSTR lpFilePart;
		TCHAR filename[MAX_PATH] = { 0, };
		if (!SearchPath(NULL, cmd, _T(""), MAX_PATH, filename, &lpFilePart))
		{
			TRACE(_T("%s cmd is maybe DOS command.\n"), cmd);
			return false;
		}

		fullPath = filename;
	}

	std::string filePath = CString2string(fullPath);

	std::ifstream file(filePath, std::ios::binary);

	if (!file)
	{
		LPTSTR lpFilePart;
		TCHAR filename[MAX_PATH] = { 0, };
		if (!SearchPath(NULL, fullPath, _T(""), MAX_PATH, filename, &lpFilePart))
		{
			TRACE(_T("Unable to open file: %s\n"), filePath);
			return false;
		}
	}

	// Read the first two bytes to check for "MZ" signature
	uint16_t mzSignature;
	file.read(reinterpret_cast<char*>(&mzSignature), sizeof(mzSignature));
	if (mzSignature != IMAGE_DOS_SIGNATURE)// 0x5A4D) { // "MZ" in little-endian
	{
		return false;
	}

	// Read the offset to the PE header (at 0x3C)
	file.seekg(0x3C, std::ios::beg);
	uint32_t peOffset;
	file.read(reinterpret_cast<char*>(&peOffset), sizeof(peOffset));

	// Move to the PE header and read the signature
	file.seekg(peOffset, std::ios::beg);
	uint32_t peSignature;
	file.read(reinterpret_cast<char*>(&peSignature), sizeof(peSignature));

	//파일정보만 가지고 해당 파일이 console에서 실행하는 명령어인지,
	//GUI를 가지고 실행되는 앱인지를 판별하는게 아직은 명확하지 않다.
	//우선 직접 명시해준다.
	cmd.MakeLower();
	if (cmd == _T("powershell.exe"))
		return false;

	if (peSignature == IMAGE_NT_SIGNATURE)//0x00004550) { // "PE\0\0" in little-endian
	{
		return true;
	}

	return false;
}

typedef struct tagCHARMENUITEM
{
	// Font of text on the menu item.
	HFONT hFont;

	// The length of the string pointed to by szItemText.
	int cchItemText;

	// A pointer to a buffer that specifies the text string. The string does 
	// not need to be null-terminated, because the c parameter specifies the 
	// length of the string.
	wchar_t szItemText[1];

} CHARMENUITEM, * PCHARMENUITEM;

bool	get_menu_item_info(HMENU hMenu, UINT uItem, UINT *uID, CString *caption, BOOL fByPos)
{
	MENUITEMINFO mii;

	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_FTYPE | MIIM_ID; // | MIIM_STATE
	mii.dwTypeData = NULL;
	GetMenuItemInfo(hMenu, uItem, fByPos, &mii);

	PCHARMENUITEM pcmi = NULL;

	pcmi = (PCHARMENUITEM)LocalAlloc(LPTR,
		sizeof(*pcmi) + mii.cch * sizeof(*pcmi->szItemText));

	pcmi->cchItemText = mii.cch;
	mii.dwTypeData = (LPTSTR)(pcmi->szItemText);
	mii.cch++;

	if (!GetMenuItemInfo(hMenu, uItem, TRUE, &mii))
		return false;

	*uID = mii.wID;
	*caption = mii.dwTypeData;

	return true;
}

//taskbar의 크기 및 show/hide 상태를 리턴한다.
bool get_taskbar_size(CSize* sz)
{
	bool is_shown = false;
	HWND hTaskbarWnd = ::FindWindow(_T("Shell_TrayWnd"), NULL);
	HMONITOR hMonitor = MonitorFromWindow(hTaskbarWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info = { sizeof(MONITORINFO) };

	if (GetMonitorInfo(hMonitor, &info))
	{
		RECT rect;
		GetWindowRect(hTaskbarWnd, &rect);
		if ((rect.top >= info.rcMonitor.bottom - 4) ||
			(rect.right <= 2) ||
			(rect.bottom <= 4) ||
			(rect.left >= info.rcMonitor.right - 2))
		{
			is_shown = false;
		}
		else
		{
			is_shown = true;
		}

		if (sz)
		{
			sz->cx = rect.right - rect.left;
			sz->cy = rect.bottom - rect.top;
		}
	}

	return is_shown;
}

bool get_taskbar_state(UINT state, CSize* sz)
{
	APPBARDATA  appBarData;
	UINT        uState;
	HWND        hTaskbarWnd;

	hTaskbarWnd = ::FindWindow(_T("Shell_TrayWnd"), NULL);

	if (IsWindow(hTaskbarWnd))
	{
		ZeroMemory(&appBarData, sizeof(APPBARDATA));

		appBarData.hWnd = hTaskbarWnd;
		appBarData.cbSize = sizeof(APPBARDATA);

		uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &appBarData);

		if (sz != NULL)
		{
			RECT rect;
			GetWindowRect(hTaskbarWnd, &rect);
			sz->cx = rect.right - rect.left;
			sz->cy = rect.bottom - rect.top;
		}

		return (uState == state);

		// 바뀐 속성 적용
		//appBarData.lParam = uState;
		//SHAppBarMessage(ABM_SETSTATE, &appBarData);
	}

	return false;
}

#if 1
bool refresh_tray_icon()
{
	HANDLE m_hProcess;
	LPVOID m_lpData;
	TBBUTTON tb;
	TRAYDATA tray;
	DWORD dwTrayPid;
	int TrayCount;

	// Tray 의 윈도우 핸들 얻기
	HWND m_hTrayWnd = find_tray_toolbar_window();
	if (m_hTrayWnd == NULL)
		return false;

	// Tray 의 개수를 구하고
	TrayCount = (int)::SendMessage(m_hTrayWnd, TB_BUTTONCOUNT, 0, 0);

	// Tray 윈도우 핸들의 PID 를 구한다
	GetWindowThreadProcessId(m_hTrayWnd, &dwTrayPid);

	// 해당 Tray 의 Process 를 열어서 메모리를 할당한다
	m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwTrayPid);
	if (!m_hProcess)
		return false;

	// 해당 프로세스 내에 메모리를 할당
	m_lpData = VirtualAllocEx(m_hProcess, NULL, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);
	if (!m_lpData)
		return false;

	// Tray 만큼 루프
	for (int i = 0; i < TrayCount; i++)
	{
		::SendMessage(m_hTrayWnd, TB_GETBUTTON, i, (LPARAM)m_lpData);
		// TBBUTTON 의 구조체와 TRAYDATA 의 내용을 얻기
		ReadProcessMemory(m_hProcess, m_lpData, (LPVOID)&tb, sizeof(TBBUTTON), NULL);
		ReadProcessMemory(m_hProcess, (LPCVOID)tb.dwData, (LPVOID)&tray, sizeof(tray), NULL);

		// 각각 트레이의 프로세스 번호를 얻어서
		DWORD dwProcessId = 0;
		GetWindowThreadProcessId(tray.hwnd, &dwProcessId);

		// Process 가 없는 경우 TrayIcon 을 삭제한다
		if (dwProcessId == 0)
		{
			NOTIFYICONDATA        icon;
			icon.cbSize = sizeof(NOTIFYICONDATA);
			icon.hIcon = tray.hIcon;
			icon.hWnd = tray.hwnd;
			icon.uCallbackMessage = tray.uCallbackMessage;
			icon.uID = tray.uID;
			Shell_NotifyIcon(NIM_DELETE, &icon);
		}
	}

	// 가상 메모리 해제와 프로세스 핸들 닫기
	VirtualFreeEx(m_hProcess, m_lpData, NULL, MEM_RELEASE);
	CloseHandle(m_hProcess);

	return true;
}

HWND find_tray_toolbar_window()
{
	HWND hWnd_ToolbarWindow32 = NULL;
	HWND hWnd_ShellTrayWnd;

	hWnd_ShellTrayWnd = ::FindWindow(_T("Shell_TrayWnd"), NULL);

	if (hWnd_ShellTrayWnd)
	{
		HWND hWnd_TrayNotifyWnd = ::FindWindowEx(hWnd_ShellTrayWnd, NULL, _T("TrayNotifyWnd"), NULL);

		if (hWnd_TrayNotifyWnd)
		{
			HWND hWnd_SysPager = ::FindWindowEx(hWnd_TrayNotifyWnd, NULL, _T("SysPager"), NULL);// WinXP

			// WinXP 에서는 SysPager 까지 추적            
			if (hWnd_SysPager)
				hWnd_ToolbarWindow32 = ::FindWindowEx(hWnd_SysPager, NULL, _T("ToolbarWindow32"), NULL);

				// Win2000 일 경우에는 SysPager 가 없이 TrayNotifyWnd -> ToolbarWindow32 로 넘어간다
			else
				hWnd_ToolbarWindow32 = ::FindWindowEx(hWnd_TrayNotifyWnd, NULL, _T("ToolbarWindow32"), NULL);
		}
	}

	return hWnd_ToolbarWindow32;
}
#else
void refresh_tray_icon()
{
	//step #1
	HWND hWnd = ::FindWindow(_T("Shell_TrayWnd"), NULL);

	if (hWnd != NULL)
		hWnd = ::FindWindowEx(hWnd, 0, _T("TrayNotifyWnd"), NULL);

	if (hWnd != NULL)
		hWnd = ::FindWindowEx(hWnd, 0, _T("SysPager"), NULL);

	if (hWnd != NULL)
		hWnd = ::FindWindowEx(hWnd, 0, _T("ToolbarWindow32"), NULL);

	process_notification_area(hWnd);


	//step #2
	hWnd = ::FindWindow(_T("NotifyIconOverflowWindow"), NULL);

	if (hWnd != NULL)
		hWnd = ::FindWindowEx(hWnd, 0, _T("ToolbarWindow32"), NULL);

	process_notification_area(hWnd);
}

void process_notification_area(HWND hWnd)
{
	if (!hWnd)
		return;

	RECT rcClient;

	BOOL bRet = ::GetClientRect(hWnd, &rcClient);

	if (!bRet)
		return;

	for (int i = rcClient.bottom - 8; i >= 0; i -= 8)
	{
		for (int j = rcClient.right - 8; j >= 0; j -= 8)
		{
			::PostMessage(hWnd, WM_MOUSEMOVE, NULL, MAKELPARAM(i, j));
			Wait(10);
		}
	}
}
#endif

CString get_asterisk_addr(CString ip)
{
	CString result;
	bool is_num_ip = false;

	if (ip.IsEmpty())
		return result;

	CString temp = ip;
	temp.Replace(_T("."), _T(""));
	is_num_ip = IsNatural(temp);

	int dot1 = ip.Find('.');
	int dot2 = ip.ReverseFind('.');

	result = ip.Left(dot1);

	if (is_num_ip)
	{
		for (int i = 0; i < dot2 - dot1; i++)
			result += '*';
		result += ip.Mid(dot2 + 1);
	}
	else
	{
		for (int i = 0; i < ip.GetLength() - dot1; i++)
			result += '*';
	}

	return result;
}

#if 1
//윈도우 네트워크 정보에 설정된 proxy 정보를 읽어온다. use proxy이면 true 리턴.
void get_proxy_info(bool& proxy_enable, CString& ip, int& port, CString& bypass, CString& PAC_url)
{
	DWORD is_enabled;
	get_registry_int(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), _T("ProxyEnable"), &is_enabled);
	proxy_enable = (is_enabled == 1);

	get_registry_str(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), _T("ProxyServer"), &ip);
	if (ip.GetLength() > 3 && ip.Find(_T(":")) > 0)
	{
		std::deque<CString> token;
		get_token_string(ip, token, _T(":"), false);
		if (token.size() == 2)
		{
			ip = token[0];
			port = _ttoi(token[1]);
		}
	}

	get_registry_str(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), _T("ProxyOverride"), &bypass);
	get_registry_str(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), _T("AutoConfigURL"), &PAC_url);

	/*
	DWORD dwSize = sizeof(INTERNET_PROXY_INFO);
	auto* pInfo = (INTERNET_PROXY_INFO*)GlobalAlloc(GPTR, dwSize);

	if (InternetQueryOption(nullptr, INTERNET_OPTION_PROXY, pInfo, &dwSize))
	{
		switch (pInfo->dwAccessType)
		{
			case INTERNET_OPEN_TYPE_DIRECT:
				TRACE(_T("no proxy-direct connection\n"));
				break;
			case INTERNET_OPEN_TYPE_PROXY:
				TRACE(_T("Proxy server: %s, Bypass list: %s\n"), pInfo->lpszProxy, pInfo->lpszProxyBypass);
				break;
			case INTERNET_OPEN_TYPE_PRECONFIG:
				TRACE(_T("Use user's IE settings\n"));
				break;
		}
	}
	else
	{
		TRACE(_T("InternetQueryOption(INTERNET_OPTION_PROXY) failed: %d\n"), GetLastError());
		return false;
	}
	*/

	/*
	HINTERNET hHttpSession = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;
	BOOL	bProxyEnv = FALSE;

	WINHTTP_AUTOPROXY_OPTIONS  AutoProxyOptions;
	WINHTTP_PROXY_INFO         ProxyInfo;
	DWORD                      cbProxyInfoSize = sizeof(ProxyInfo);

	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ieProxyConfig = { 0 };
	ZeroMemory(&AutoProxyOptions, sizeof(AutoProxyOptions));
	ZeroMemory(&ProxyInfo, sizeof(ProxyInfo));

	char logbuf[512];
	memset(logbuf, 0x00, sizeof(logbuf));

	if (!::WinHttpGetIEProxyConfigForCurrentUser(&ieProxyConfig))
	{
		goto NEXT;
	}

	//if (m_log)
	{
		TRACE(_T("[SOCKET]FindProxyFromWinHttp(ieProxyConfig ==>\r\n\t(AutoDetect : %d)\r\n\t(configUrl : %s)\r\n\t(szProxy : %s)\r\n\t(proxyBypass : %s)\r\n"),
			ieProxyConfig.fAutoDetect, ieProxyConfig.lpszAutoConfigUrl, ieProxyConfig.lpszProxy, ieProxyConfig.lpszProxyBypass);
	}

	//PAC 방식이면
	if (ieProxyConfig.lpszAutoConfigUrl != NULL) //auto config
	{
		//wsprintf(logbuf , "[SUCCESS] Get PAC file (%S) \r\n" , ieProxyConfig.lpszAutoConfigUrl);
		//if (m_log)
		{
			//m_log->koino(_T("[SUCCESS] Get PAC file (%S)") , ieProxyConfig.lpszAutoConfigUrl); 
			TRACE(_T("[SOCKET][SUCCESS] Get PAC file (%s)"), ieProxyConfig.lpszAutoConfigUrl); // 2018.10.11 pac 프록시 잘못 된 부분 수정
		}

		if (strlen((LPSTR)ieProxyConfig.lpszAutoConfigUrl))
		{
			AutoProxyOptions.lpszAutoConfigUrl = ieProxyConfig.lpszAutoConfigUrl;

			hHttpSession = WinHttpOpen(0,
				WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
				WINHTTP_NO_PROXY_NAME,
				WINHTTP_NO_PROXY_BYPASS,
				WINHTTP_FLAG_ASYNC);

			// Exit if WinHttpOpen failed.
			if (!hHttpSession)
			{
				goto NEXT;
			}

			// Use auto-detection because the Proxy 
			// Auto-Config URL is not known.
			AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;

			// Use DHCP and DNS-based auto-detection.
			AutoProxyOptions.dwAutoDetectFlags =
				WINHTTP_AUTO_DETECT_TYPE_DHCP |
				WINHTTP_AUTO_DETECT_TYPE_DNS_A;

			// If obtaining the PAC script requires NTLM/Negotiate
			// authentication, then automatically supply the client
			// domain credentials.
			AutoProxyOptions.fAutoLogonIfChallenged = TRUE;

			AutoProxyOptions.lpszAutoConfigUrl = ieProxyConfig.lpszAutoConfigUrl;

			if (WinHttpGetProxyForUrl(hHttpSession,
				L"http://www.anysupport.net",
				&AutoProxyOptions,
				&ProxyInfo))
			{

				if (ProxyInfo.lpszProxy == NULL)
				{
					goto NEXT;
				}

				if (strlen((LPSTR)ProxyInfo.lpszProxy) == 0)
				{
					goto NEXT;
				}

				//wsprintf(logbuf , "[SUCCESS] Get PAC Proxy Info (%S) \r\n" , ProxyInfo.lpszProxy);

				//if (m_log)
				{
					//m_log->koino(_T("[SUCCESS] Get PAC Proxy Info (%S)") , ProxyInfo.lpszProxy);
					TRACE(_T("[SOCKET][SUCCESS] Get PAC Proxy Info (%s)"), ProxyInfo.lpszProxy); // 2018.10.11 pac 프록시 잘못 된 부분 수정
				}

				CString strProyxIP;
				strProyxIP.Format(_T("%s"), ProxyInfo.lpszProxy);
				strProyxIP.Remove(' ');

				int n = -1;
				n = strProyxIP.Find(_T(";"));

				if (n == -1)
				{
					n = strProyxIP.Find(_T(":"));
					if (n == -1)
					{
						ip = strProyxIP;
						port = 80;

						TRACE(_T("[SOCKET][SUCCESS] Get PAC Proxy Info1 (%s:%d)"), ip, port);

						bProxyEnv = TRUE;
						goto END;
					}
					else
					{
						ip = strProyxIP.Left(n);
						port = _ttoi(strProyxIP.Right(strProyxIP.GetLength() - (n + 1)));

						TRACE(_T("[SOCKET][SUCCESS] Get PAC Proxy Info2 (%s:%d)"), ip, port);
						bProxyEnv = TRUE;
						goto END;
					}

				}
				else
				{
					CString strProxyIP2;
					strProxyIP2 = strProyxIP.Left(n);

					n = strProxyIP2.Find(_T(":"));
					if (n == -1)
					{
						ip = strProxyIP2;
						port = 80;

						TRACE(_T("[SOCKET][SUCCESS] Get PAC Proxy Info3 (%s:%d)"), ip, port);

						bProxyEnv = TRUE;
						goto END;
					}
					else
					{
						ip = strProxyIP2.Left(n);
						port = _ttoi(strProxyIP2.Right(strProxyIP2.GetLength() - (n + 1)));

						TRACE(_T("[SOCKET][SUCCESS] Get PAC Proxy Info4 (%s:%d)"), ip, port);
						bProxyEnv = TRUE;
						goto END;
					}

				}
			}
			else
			{
				goto NEXT;
			}

		}
	}

NEXT:

	if (ieProxyConfig.lpszProxy != NULL) //manual config
	{
		//if(strlen((LPSTR)ieProxyConfig.lpszProxy)) //수동 방식이면
		if (_tcslen((LPWSTR)ieProxyConfig.lpszProxy)) //수동 방식이면 // 2018.10.11 pac 프록시 잘못 된 부분 수정
		{
			//Socket::PrintLog(1 , "[SUCCESS] Get Manual Proxy Info from IE(%S) \r\n" , ieProxyConfig.lpszProxy);
			ip = ieProxyConfig.lpszProxy; // 2018.10.14 fix -  호스트 프로그램에서 프록시 안되는 문제, 로그가 설정안되어도 프록시 IP를 설정해야한다.

			//if (m_log)
			{
				///strProxyIP.setString(ieProxyConfig.lpszProxy); // 2018.10.14 fix -  호스트 프로그램에서 프록시 안되는 문제, 로그가  설정안되어도 프록시 IP를 설정해야한다.
				TRACE(_T("[SOCKET][SUCCESS] Get Manual Proxy Info from IE(%s)"), ip);
			}

			if (ip.Find(_T(":") > 0))
			{
				std::deque<CString> chunks;
				get_token_string(ip, chunks, _T(":"), false);
				ip = chunks[0];
				port = _ttoi(chunks[1]);

				//if (m_log)
					TRACE(_T("[SOCKET][SUCCESS] Get Proxy IP : Port from IE(%s : %d)"), ip, port);
				bProxyEnv = TRUE;
				goto END;
			}
			else
			{
				//if (m_log)
					TRACE(_T("[SOCKET][SUCCESS] Get Proxy IP : Port from IE(%s : %d)"), ip, port);

				bProxyEnv = FALSE;
				goto END;
			}
		}
		else //proxy 환경이 아님..
		{
			bProxyEnv = FALSE;
			goto END;
		}
	}
	else
	{
		bProxyEnv = FALSE;
		// No Proxy Env
	}

END:
	//
	// Clean up the WINHTTP_PROXY_INFO structure.
	//
	if (ieProxyConfig.lpszAutoConfigUrl != NULL)
		GlobalFree(ieProxyConfig.lpszAutoConfigUrl);

	if (ieProxyConfig.lpszProxyBypass != NULL)
		GlobalFree(ieProxyConfig.lpszProxyBypass);

	if (ieProxyConfig.lpszProxy != NULL)
		GlobalFree(ieProxyConfig.lpszProxy);


	if (ProxyInfo.lpszProxy != NULL)
		GlobalFree(ProxyInfo.lpszProxy);

	if (ProxyInfo.lpszProxyBypass != NULL)
		GlobalFree(ProxyInfo.lpszProxyBypass);

	//
	// Close the WinHTTP handles.
	//
	if (hRequest != NULL)
		WinHttpCloseHandle(hRequest);

	if (hConnect != NULL)
		WinHttpCloseHandle(hConnect);

	if (hHttpSession != NULL)
		WinHttpCloseHandle(hHttpSession);

	return bProxyEnv;
	*/
}
#endif

//x86에서만 가능
#ifdef _M_IX86
bool is_VMWare()
{
	bool res = true;

	__try
	{
		__asm
		{
			push   edx
			push   ecx
			push   ebx

			mov    eax, 'VMXh'
			mov    ebx, 0
			mov    ecx, 10 // get VMWare version
			mov    edx, 'VX' // port number

			in     eax, dx // read port
			// on return EAX returns the VERSION

			cmp    ebx, 'VMXh' // compare with target
			setz[res] // set return value

				pop    ebx
					pop    ecx
					pop    edx
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		res = false;
	}

	return res;
}
#else
bool is_VMWare()
{
	int cpuInfo[4] = {};

	//
	// Upon execution, code should check bit 31 of register ECX
	// (the “hypervisor present bit”). If this bit is set, a hypervisor is present.
	// In a non-virtualized environment, the bit will be clear.
	//
	__cpuid(cpuInfo, 1);


	if (!(cpuInfo[2] & (1 << 31)))
		return false;

	//
	// A hypervisor is running on the machine. Query the vendor id.
	//
	const auto queryVendorIdMagic = 0x40000000;
	__cpuid(cpuInfo, queryVendorIdMagic);

	const int vendorIdLength = 13;
	using VendorIdStr = char[vendorIdLength];

	VendorIdStr hyperVendorId = {};

	memcpy(hyperVendorId + 0, &cpuInfo[1], 4);
	memcpy(hyperVendorId + 4, &cpuInfo[2], 4);
	memcpy(hyperVendorId + 8, &cpuInfo[3], 4);
	hyperVendorId[12] = '\0';

	static const VendorIdStr vendors[]{
	"KVMKVMKVM\0\0\0", // KVM 
	"Microsoft Hv",    // Microsoft Hyper-V or Windows Virtual PC */
	"VMwareVMware",    // VMware 
	"XenVMMXenVMM",    // Xen 
	"prl hyperv  ",    // Parallels
	"VBoxVBoxVBox"     // VirtualBox 
	};

	for (const auto& vendor : vendors)
	{
		if (!memcmp(vendor, hyperVendorId, vendorIdLength))
			return true;
	}

	return false;
}
#endif

