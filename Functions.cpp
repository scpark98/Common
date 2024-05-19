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

//�Ʒ� �� ������ GetWindowsVersion()�Լ��� ���� ���ԵǾ��µ�
//Functions.h�� include�� ���
//NH�� LiveWebClient���� ����ϴ� opencv�� ���� �浹�� �����.
//�����ϸ� .h�� include�� �ּ�ȭ �� �ʿ䰡 �ִ�.
#include <LM.h>
#pragma comment(lib, "netapi32.lib")


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

std::deque<CRect> g_dqMonitors;

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "version.lib")		//for VerQueryValue
#pragma comment(lib, "comsuppw.lib ")	//for _bstr_t

CString	get_date_string(CTime t, CString sep)
{
	CString str;

	str.Format(_T("%d%s%02d%s%02d"), t.GetYear(), sep, t.GetMonth(), sep, t.GetDay());
	return str;
}

CString get_date_string(COleDateTime t, CString sep)
{
	CString str;

	str.Format(_T("%d%s%02d%s%02d"), t.GetYear(), sep, t.GetMonth(), sep, t.GetDay());
	return str;
}

CString get_date_string(__timeb32 tb, CString sep)
{
	CTime	t(tb.time);
	return get_date_string(t, sep);
}

CString get_date_string(SYSTEMTIME st, CString sep)
{
	CTime	t(st);
	return get_date_string(t, sep);
}

CString get_time_string(CTime t, CString sep, bool h24, bool sec)
{
	return get_time_string(COleDateTime(t.GetTime()), sep, h24, sec);
}

CString get_time_string(COleDateTime t, CString sep, bool h24, bool sec)
{
	CString str;

	int h = t.GetHour();
	int m = t.GetMinute();
	int s = t.GetSecond();
	bool am = (h < 12);

	if (h24)
		str.Format(_T("%02d%s%02d%s%02d"), h, sep, m, sep, s);
	else
		str.Format(_T("%s %2d%s%02d%s%02d"), (am ? _T("����") : _T("����")), (h >= 13 ? h - 12 : h), sep, m, sep, s);

	if (!sec)
		str = str.Left(str.GetLength() - 2 - sep.GetLength());

	return str;
}

CString get_time_string(__timeb32 tb, CString sep, bool h24, bool sec)
{
	CTime	t(tb.time);
	return get_time_string(t, sep, h24, sec);
}

CString	get_time_string(SYSTEMTIME st, CString sep, bool h24, bool sec, bool msec)
{
	CString str;
	CTime t(st);
	str.Format(_T("%s%s%s"), get_time_string(t, sep, h24, sec), (msec ? _T(".") : _T("")), (msec ? i2S(st.wMilliseconds, false, true, 3) : _T("")));
	return str;
}

CString	get_datetime_string(CTime t, int type, bool sep, CString mid, bool h24, bool sec)
{
	return get_datetime_string(COleDateTime(t.GetTime()), type, sep, mid, h24, sec);
}

CString	get_datetime_string(COleDateTime t, int type, bool sep, CString mid, bool h24, bool sec)
{
	CString sDate = (sep ? get_date_string(t) : get_date_string(t, _T("")));
	CString sTime = (sep ? get_time_string(t, _T(":"), h24, sec) : get_time_string(t, _T(""), h24, sec));

	if (type == 0)
		return sDate;
	else if (type == 1)
		return sTime;

	return sDate + mid + sTime;
}

CString	get_datetime_string(__timeb32 tb, int type, bool sep, CString mid, bool h24, bool sec)
{
	CTime	t(tb.time);
	return get_datetime_string(t, type, sep, mid, h24, sec);
}

CString	get_datetime_string(SYSTEMTIME st, int type, bool sep, CString mid, bool h24, bool sec, bool msec)
{
	CString sDate = (sep ? get_date_string(st) : get_date_string(st, _T("")));
	CString sTime = (sep ? get_time_string(st, _T(":"), h24, sec, msec) : get_time_string(st, _T(""), h24, sec, msec));

	if (type == 0)
		return sDate;
	else if (type == 1)
		return sTime;

	return sDate + mid + sTime;
}


CString	get_cur_datetime_string(int type, bool sep, CString mid, bool h24, bool sec, bool msec)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	return get_datetime_string(st, type, sep, mid, h24, sec, msec);
}

#if 0
//type 0(date), 1(time), 2(date+time), ��-��-�� ��:��:�� �������� ���� �ð� ����. mid_char�� ��¥�� �ð� ���� ����
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

//sDate = "20140403"
//sTime = "123456"
//CTime t = 
CTime GetTimeFromTimeString(CString sDate, CString sTime)
{
	sDate.Remove('-');
	sDate.Remove('/');
	sDate.Remove(' ');

	sTime.Remove(':');
	sTime.Remove(' ');

	ASSERT(sDate.GetLength() == 8);
	ASSERT(sTime.GetLength() == 6);

	CTime t(_tstoi((TCHAR*)(LPCTSTR)(sDate.Left(4))), _tstoi((TCHAR*)(LPCTSTR)(sDate.Mid(4, 2))), _tstoi((TCHAR*)(LPCTSTR)(sDate.Right(2))),
			 _tstoi((TCHAR*)(LPCTSTR)(sTime.Left(2))), _tstoi((TCHAR*)(LPCTSTR)(sTime.Mid(2, 2))), _tstoi((TCHAR*)(LPCTSTR)(sTime.Right(2))));

	return t;
}

CTimeSpan GetTimeSpanFromTimeString(CString sTime)
{
	ASSERT(sTime.GetLength() == 6 || sTime.GetLength() == 8);
	sTime.Remove(':');
	return CTimeSpan(0, _ttoi(sTime.Left(2)), _ttoi(sTime.Mid(2,2)), _ttoi(sTime.Right(2)));
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

int	find_string(CString target, CString find_string, bool case_sensitive)
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

//dqSrc�� dqFind�� �ִ��� �˻�. ����� AND �����̹Ƿ� dqFind�� ��� ���Ұ� dqSrc�� ���ԵǾ� �־�� ��.
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
					//���� OR �����̸� ���⼭ true ����
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
					//���� OR �����̸� ���⼭ true ����
					found_count++;

					if (found_index < 0)
						found_index = i;

					if (op == '|')
						break;
				}
			}
		}

		//AND�̸� ã�� ���ڿ��� ��� �����ؾ� �ϰ�
		if (op == '&')
		{
			if (found_count == dqFind.size())
				return found_index;
		}
		//OR�̸� �ϳ��� �����ص� true
		else
		{
			if (found_count > 0)
				return found_index;
		}

		found_index = -1;
	}

	return found_index;
}

//Ŭ������ clipboard
bool copy_to_clipboard(HWND hWnd, CString str)
{
	if (str.IsEmpty())
		return false;

	char	*pString = NULL;
	int len = WideCharToMultiByte(CP_ACP, 0, (CStringW)str, -1, NULL, 0, NULL, NULL);
	pString = (char*)new char[len];
	WideCharToMultiByte(CP_ACP, 0, (CStringW)str, -1, pString, len, NULL, NULL);

	// ������ ���ڿ��� ���̸� ���Ѵ�. ('\0'���� ������ ũ��)
	int string_length = strlen(pString) + 1;

	// Ŭ������� ���ڿ��� �����ϱ� ���Ͽ� �޸𸮸� �Ҵ��Ѵ�. 
	// Ŭ�����忡�� �ڵ��� �ִ� �����̶� HeapAlloc �Լ� ����� �����ϴ�. 
	HANDLE h_data = ::GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, string_length);

	// �Ҵ�� �޸𸮿� ���ڿ��� �����ϱ� ���ؼ� ��� ������ �ּҸ� ��´�. 
	char *p_data = (char *)::GlobalLock(h_data);

	if (NULL != p_data)
	{
		// �Ҵ�� �޸� ������ ������ ���ڿ��� �����Ѵ�. 
		memcpy(p_data, pString, string_length);

		// ���ڿ��� �����ϱ� ���ؼ� Lock �ߴ� �޸𸮸� �����Ѵ�.
		::GlobalUnlock(h_data);

		if (::OpenClipboard(hWnd))
		{
			::EmptyClipboard(); // Ŭ�����带 ����.
			::SetClipboardData(CF_TEXT, h_data);  // Ŭ�����忡 ����� ���� ���ڿ��� �����Ѵ�.
												  // Ŭ������� ���ڿ��� �����Ѵ�.
			::CloseClipboard(); // Ŭ�����带 �ݴ´�.
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

//random19937�� �̿��Ͽ� ���� �÷��� �����Ѵ�.
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
	
	GetModuleFileName(AfxGetInstanceHandle(), sFilePath, MAX_PATH);
	
	CString exe_directory = get_part(sFilePath, fn_folder);
	if (includeSlash && exe_directory.Right(1) != _T("\\"))
		exe_directory += _T("\\");

	return exe_directory;
}

CString get_exe_root_directory()
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

//_tsplitpath("c:\\abc/def\\123.txt", ...)�� �����ϸ�
//"c:"		"\\abc/def\\"		"123"		".txt" �� ���� �и��Ǵµ� ������ ����ϴ� ��밪�� �޶� �����Ѵ�.
//"c:\\"	"c:\\abc/def"		"123"		"txt"	"123.txt"�� ���� �����Ѵ�.
//part : fn_drive(drive), fn_folder(drive+folder), fn_last_folder(folder name), fn_title(filetitle), fn_ext(ext), fn_name(filename)
//���� path�� "d:\\aaa\\b.abc"�̰� b.abc�� ������ �ƴ� ������� �����ȴ�.
//�Ǵ� path�� "d:\\aaa\\ccc"�ε� ������ �ƴ� �����ϼ��� �ִ�.
//�������� ���������� �����ؼ� ó���ϴ� �ڵ�� �ʼ���.(���� �����ϴ� ��쿡�� �˻簡 �����ϴ�)
//��, path�� "�����ҹ���(\\192.168.1.103) (Y:)"�� ���� ��Ʈ��ũ ��θ� ������ ����̺� ������ ����
//�и��ؼ��� �ȵǹǷ� �׳� �����ؾ� �Ѵ�.
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

	//Ȯ���ڸ� ������ ���ϸ�
	parts[fn_name] = parts[fn_title] + parts[fn_ext];

	//�������� ���� '/' �Ǵ� '\\'��� ����.
	if (parts[fn_folder].Right(1) == '/' || parts[fn_folder].Right(1) == '\\')
		parts[fn_folder].Truncate(parts[fn_folder].GetLength() - 1);

	//�������� ����̺� ��α��� ��� ����. "\\abc/def\\" => "c:\\abc/def\\"
	parts[fn_folder] = parts[fn_drive] + parts[fn_folder];

	//����̺� ��Ʈ��� ���� '\'�� �ٿ���� �Ѵ�. "C:" => "C:\\"
	if (parts[fn_folder].GetLength() == 2 && parts[fn_folder].Right(1) == _T(":"))
		parts[fn_folder] += _T("\\");

	//Ȯ���ڴ� .�� ���ܽ�Ų��. parts[3]�� ""�̾ .Mid(1)�� ������ �߻������� �ʴ´�.
	parts[fn_ext] = parts[fn_ext].Mid(1);

	//path�� b.abc�� ���� ���ϸ�ó�� �Ǿ� ������ ������ �ƴ� folder�� ����� ó��(���� ������ ��쿡�� ����)
	if (PathFileExists(path))
	{
		if (PathIsDirectory(path))
		{
			//�ش� ��ΰ� ���丮�̰� part == fn_name�̶�� �Ѿ�Դٸ�
			//�̴� �����δ� fn_last_folder�� ��û�ϴ� ������ ���� �Ѵ�.
			//�Ǵ� ȣ���ϴ� ������ �̸� �Ǻ��Ͽ� ��û�� ���� �ִ�.
			//�����̸� fn_name�� ��û�ϰ� �����̸� fn_last_folder�� ��û�ؾ� �Ѵ�.
			part = fn_last_folder;
			parts[fn_last_folder] = parts[fn_name];
			parts[fn_folder] = parts[fn_folder] + _T("\\") + parts[fn_name];
			parts[fn_title] = parts[fn_ext] = parts[fn_name] = _T("");
		}
	}
	else
	{
		//"���� ȭ��"�� ���� ���� ����� ���� fn_title�� fn_name�� �� ������ ä�����Ƿ�
		//�����̸����� �� ���� �־� ����Ѵ�.
		if (parts[fn_drive].IsEmpty() && parts[fn_folder].IsEmpty() && !parts[fn_name].IsEmpty())
			parts[fn_folder] = parts[fn_last_folder] = parts[fn_name];
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

	//sFullPath�� ������� �׳� �� ���� �����Ѵ�.
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
	// Ȯ����� �ִ� ���� ���� ��츦 �����Ѵ�.
	if (filename.Find('.') > 0)
		return filename.Left(filename.ReverseFind('.'));
	else
		return filename;
}

CString	GetFileExtension(CString sFullPath, bool dot)
{
	CString sString = GetFileNameFromFullPath(sFullPath);

	// Ȯ����� �ִ� ���� ���� ��츦 �����Ѵ�.
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

//���ϸ��� Ȯ���ڸ� newExt�� �����Ѵ�.
//applyRealFile�� true�̸� ���� ���ϸ� �����Ų��.
bool change_extension(CString& filepath, CString newExt, bool applyRealFile)
{
	CString sOldExt = get_part(filepath, fn_ext);
	CString sNewFullPath = filepath.Left(filepath.GetLength() - sOldExt.GetLength()) + newExt;

	bool changeSuccess = false;

	if (applyRealFile)
	{
		//���� ���� Ȯ���ڸ� ������������ filepath�� ��������ش�.
		changeSuccess = MoveFile(filepath, sNewFullPath);
		if (changeSuccess)
			filepath = sNewFullPath;
		return changeSuccess;
	}

	filepath = sNewFullPath;
	return true;
}

//��λ� "\\.." �Ǵ� "/.."�� ��������� ���� ��η� �籸�����ش�.
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

//������ �ִ� ���ϵ� �� filetitle�̰� extension�� �ش��ϴ� ���ϸ��� �����Ѵ�.
std::deque<CString>	get_filelist_from_filetitle(CString folder, CString filetitle, CString extension)
{
	return FindFilesWithExtensions(folder, filetitle, extension);
}

std::deque<CString> get_filelist_from_filetitle(CString filename, CString extension)
{
	return get_filelist_from_filetitle(get_part(filename, fn_folder), get_part(filename, fn_title), extension);
}

uint64_t get_file_size(CString sfile)
{
	CFileStatus		status;
	
	//��ũ ����̺��� ���
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

uint64_t get_folder_size(CString path)
{
	WIN32_FIND_DATA data;
	uint64_t size = 0;
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
//unit_string	: ������ ǥ���� �� (default = true)
CString		get_file_size_string(CString sfile, int unit, int floats, bool unit_string)
{
	return get_size_string(get_file_size(sfile), unit, floats, unit_string);
}

//unit			: 0:bytes, 1:KB, 2:MB, 3:GB ~
//auto�� ���� 1000���� ���������� ������ �Ҽ����� 2�ڸ����� ǥ���Ѵ�.(ex 7.28TB 32.1TB 123.5TB)
//floats		: �Ҽ����� �� �ڸ����� ǥ������
//unit_string	: ������ ǥ���� ��
//comma			: ���� �κп� �ڸ��� �޸��� ǥ���� ��
CString		get_size_string(int64_t size, int unit, int floats, bool unit_string, bool comma)
{
	double dsize = (double)size;
	CString size_str;
	CString unit_str[9] = { _T("Bytes"), _T("KB"), _T("MB"), _T("GB"), _T("TB"), _T("PB"), _T("EB"), _T("ZB"), _T("YB") };

	if (unit >= 0)
	{
		for (int i = 0; i < unit; i++)
			dsize /= 1024.0;
	}
	else
	{
		unit = 0;
		while (dsize >= 1000.0)
		{
			dsize /= 1024.0;
			unit++;
		}

		if (dsize < 10)
			floats = 2;
		else
			floats = 1;
	}

	//dsize = 0.01234;
	//floats = 2;
	double multiply = pow(10.0, floats);
	dsize = ceil(dsize * multiply) / multiply;// ROUND(dsize, floats);
	//0.045KB�� �� floats�� 0�̸� 0KB���� ���ϵ� ���̴�.
	//�� ����� 1KB�� �����ϴ� ���� �´�.
	//���� ������ ũ�Ⱑ 0byte�϶��� 0�� ��������.
	//0.045KB�� �� floats�� 1�̸� ���� 0.0KB�� ���ϵ� ���̴�.
	//�� ����� 0.1KB�� ���ϵǴ°� �´�.
	//floats�� 1�̸� 0.1�̰� �� 0.1���� dsize�� �۴ٸ� �׳� 0.1��� ������.
	double dmin = 1.0 / pow(10.0, floats);
	if (dsize > 0.0 && dsize < dmin)
		dsize = dmin;

	//dsize = ROUND(dsize, floats + 1);
	size_str = d2S(dsize, comma, floats);
	if (unit_string)
		size_str += unit_str[unit];

	return size_str;
}


//src�� �Ľ��ؼ� Ư�� ���� �̻��� ���ڿ���� ������.
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

//src�� pos��ó���� �������� ã�� �� ��ġ�� �����Ѵ�.(���������� ������ ������ �̿�)
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
	const wchar_t start_ch = L'��';
	const wchar_t end_ch = L'�R';

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

//check_prefix�� true�̸� http, https���� üũ�Ѵ�. ���� ������� �ִµ��Ͽ� �켱 ������.(https://mathiasbynens.be/demo/url-regex)
bool is_valid_url(CString url, bool check_prefix)
{
	std::string pattern;
	
	if (check_prefix)
		pattern = "https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{2,256}\.[a-z]{2,4}\b([-a-zA-Z0-9@:%_\+.~#?&//=]*)";
	else //Ȯ�� �ʿ�!!
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

int	get_char_count(CString sStr, TCHAR ch)
{
	int	nCount = 0;
	
	for (int i = 0; i < sStr.GetLength(); i++)
		if (sStr.GetAt(i) == ch)
			nCount++;
		
		return nCount;
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
			// DoubleByteCharacterSet�̸�...
			if (IsDBCSLeadByte((BYTE)(str.GetAt(nCenter))))
				return nCenter-1;
			else
				return nCenter;
		}
	}
}

//�־��� array���� ���ϴ� �׸��� ã�´�.
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

CString	GetTimeStringFromSeconds(double dSec, bool bHasHour /*= true*/, bool bMilliSec /*=false*/)
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

//���� double dSec�� ����Ͽ� ���� �Լ��� ��������� �Ҽ��� ǥ�� ����� ������ �����ϹǷ�
//1.0�̾�� �� ���� 0.999999999998 �� ���� ǥ���Ǿ� ���� �Լ��� ����� �� ���� ��찡 �ִ�.
//�׷��� �ƿ� millisecond���� int�� �����Ͽ� ó���� �Ʒ� �Լ��� ����Ѵ�.
CString		GetTimeStringFromMilliSeconds(int ms, bool bHasHour, bool bHasMilliSec)
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
//srt�ڸ��� ���� .��� ,�� ����ϹǷ� �� �� ������ش�.
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
// �⺻������ 16���� ��Ʈ���� �Ѿ�´ٰ� ����...
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
		0,						// time-out period (<- ���⸦ 20 �̶�� ���� 20�� �� �� �������Ѵ�.)
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

		//�Ҽ����� �ִ� ���
		int iDotPos = strData.Find(_T(".")); 
		if (0 <= iDotPos)
		{
			if (0<= strData.Find(_T("."), iDotPos + 1))    // �ι�° "."
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
					//bool bDotNothing = (iDotPos<0);//.�� ������ Error
					//if(bDotNothing)
					//{
					// return FALSE;
					//}
					bool bDotPosLater = (i<iDotPos);//.�� E���� �ڿ� ������ Error
					if(bDotPosLater)
					{
						return FALSE;
					}
					iCountE++;
					if(1<iCountE){//E�� �ΰ� ������ Error
						return FALSE;
					}
					bNextSign=TRUE;
					continue;
				}

				//E�ڿ��� +�� -�� �־���ϰ� �ڿ��� ���ڰ� �־�� ��
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
				bool bSignError = bSign && (0!=i); //ó���� �ƴϸ� Error
				if(bSignError)
				{
					return FALSE;
				}
			}
			else //������ ���
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

//�־��� ���ڿ��� ���ĺ��� ���ڷθ� ������ ���ڿ����� �˻��Ѵ�.
//excepts���� ���Եǵ� �Ǵ� ���ڿ����� ����ִµ� �̵��� ;���� ���еǾ� �ְ�
//�˻��ϱ� ���� �̸� ������ �� �˻��Ѵ�.
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

//ASCII �ڵ��� #33(0x21)(' ') ~ #126(0x7E)('~') ��������(���� �� �ִ� ���ڿ�����)
bool is_readable_char(CString src)
{
	for (int i = 0; i < src.GetLength(); i++)
	{
		if (src[i] < 33 || src[i] > '~')
			return false;
	}

	return true;
}

//'��'~'�R'������ �ѱ۷θ� ������ ���ڿ����� �˻��Ѵ�.
bool is_hangul(CString str)
{
	const wchar_t start_ch = L'��';
	const wchar_t end_ch = L'�R';

	for (int i = 0; i < str.GetLength(); i++)
	{
		if (str[i] < start_ch || str[i] > end_ch)
			return false;
	}

	return true;
}

static const char* const initial_array[] = {
	"��", "��", "��", "��", "��", "��", "��", "��", "��", "��",
	"��", "��", "��", "��", "��", "��", "��", "��", "��"
};
static const char* const medial_array[] = {
	"��", "��", "��", "��", "��", "��", "��", "��", "��", "��",
	"��", "��", "��", "��", "��", "��", "��", "��", "��", "��",
	"��"
};
static const char* const final_array[] = {
	  "", "��", "��", "��", "��", "��", "��", "��", "��", "��",
	"��", "��", "��", "��", "��", "��", "��", "��", "��", "��",
	"��", "��", "��", "��", "��", "��", "��", "��"
};

//���ڿ��� �������� ���� ���������� �Ǻ�(Ư�� �ѱ� ���ڵ� ���� �Ǻ�)
//"�ѱ�'??�통??�연��ݨ트?�재??"
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

//�ϼ��� �ѱ��� �� ���ڸ� �ʼ�, �߼�, �������� �и��Ѵ�.
bool get_consonant(CString src, wchar_t* cho, wchar_t* jung, wchar_t* jong)
{
	const wchar_t *tsrc = CString2WCHAR(src);
	//wchar_t *asdf = L"��";
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

//start���� �����ؼ� ó�� ������ ���� ������ �����ؼ� ���ڷ� �����Ѵ�.
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

//���ڿ��� ���Ե� ���ڹ��ڸ� ���ڷ� �����Ͽ� ���Ѵ�.
//"a5"�� "a12345"���� �۴�.
//��, "a5"�� "a-22"�� ���� "-"��ȣ�� ��ȣ�ε�, Ư�����ڷε� �������� �����Ƿ� "a5"�� �۴ٰ� ó���ȴ�.
bool is_greater_with_numeric(CString str0, CString str1)
{
	//StrCmpLogicalW�����δ� Ž����� ���� ���ʷ� ���ĵ��� �ʾƼ�
	//�Ʒ� �ڵ带 �־����� ���� �ٽ� ��������
	//StrCmpLogicalW�����ε� Ž����� ���� ���ʷ� ���ĵȴ�.
	//�и��� StrCmpLogicalW�����δ� ���� �ʾҴµ�...
	/*
	int i = 0;

	while (true)
	{
		//���� �ٸ� ���ڸ� ���������� ��ŵ�Ѵ�.
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

	//���� ���ϸ� ó�� ~ i�ٷ� ������ ���ڿ��� i���� ���ڰ� �ƴϸ�
	//���ϸ� ���ۺ��� ���ڷ� �� ���ϸ��� ����.
	
	if ((i > 0) && (str0[i] < '0' || str0[i] > '9' || str1[i] < '0' || str1[i] > '9') &&
		IsNumeric(str0.Left(i)))
	{
		//�Ʒ� if���� �׻� true�� �� �ۿ� ���� ���̴�.
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

	return (StrCmpLogicalW(s0, s1) == 1);
}


//n�� 26���� ���� �÷� �ε����� ��ȯ�� ���ڿ��� ����
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
		return FALSE;  // �̸����� ������ false

	int iAtCount = 0;   //@ ��ġ
	int iDotCount = 0;  // . ��ġ
	int i;

	for(i = 0; i < sEMail.GetLength(); i++)
	{
		if (i > 0 && sEMail.GetAt(i) == '@')
			iAtCount = i+1;    // ��

		if(iAtCount > 0 && i > iAtCount && sEMail.GetAt(i) == '.')
			iDotCount = i+1;   // ��
	}

	if (i > iDotCount && iAtCount > 0 && iDotCount > 0)
		return TRUE;     // ��
	
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

	//AfxParseURL()�� ����ϱ� ���ؼ��� url�� �ݵ�� http:// �Ǵ� https:// ��� ���� ���� ������ ǥ�õǾ�� �Ѵ�.
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
//	params->status = request_url(params->result, params->ip, params->port, params->sub_url, params->method, &params->headers, params->body, params->local_file_path);
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
//url�� ȣ���Ͽ� ������� �����ϰų� ������ ���� ���Ϸ� �ٿ�ε� �Ѵ�.
//local_file_path�� ""�̸� ������� ���ڿ��� ���Ϲ޴´�.
//local_file_path�� �����Ǿ� ������ ���Ϸ� �ٿ�޴´�. (�̶� result_str�� "")
//���ϰ��� 200�� �ƴ� ���� ���ϵ� �����ڵ�� result_str�� ����� ���� �޽����� �����Ͽ� ���� ó���Ѵ�.
//DWORD request_url(CString &result_str, CString ip, int port, CString sub_url, CString verb, std::vector<CString> *headers, CString jsonBody, CString local_file_path)
void request_url(CRequestUrlParams* params)
{
	long t0 = clock();

	//ip�� http://���� https://������ ��õǾ� �ִٸ� �̴� ��Ȯ�ϹǷ�
	//�̸� �Ǵ��Ͽ� params->is_https���� �缳���Ѵ�.
	//��Ʈ��ȣ�� https�� �Ǻ��ϴ� ���� �Ѱ谡 �����Ƿ� ip�� ����ϵ�, params->is_https�� ��Ȯ�� ����Ͽ� ����Ѵ�.
	//���� ip�� http:// �Ǵ� https:// �� �پ� ������ InternetConnect()�� �����Ѵ�. �����ϰ� ȣ������� �Ѵ�.
	if (params->ip.Left(7) == _T("http://"))
	{
		params->is_https = false;
		params->ip.Replace(_T("http://"), _T(""));
	}
	else if (params->ip.Left(8) == _T("https://"))
	{
		params->is_https = true;
		params->ip.Replace(_T("https://"), _T(""));
	}


	if (params->full_url.IsEmpty() == false)
	{
		parse_url(params->full_url, params->ip, params->port, params->sub_url, params->is_https);
	}


	//sub_url�� �� �տ��� �ݵ�� '/'�� �پ��־�� �Ѵ�.
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


	//��Ʈ�� 0���� ������ �⺻ ��Ʈ�� ����Ѵ�.
	if (params->port <= 0)
	{
		if (params->is_https)
			params->port = 443;
		else
			params->port = 80;
	}

	if (params->method.IsEmpty())
		params->method = _T("GET");

	params->method.MakeUpper();
	if (!is_one_of(params->method, _T("GET"), _T("PUT"), _T("POST"), _T("DELETE")))
	{
		params->status = HTTP_STATUS_BAD_METHOD;
		params->result = _T("Unknown HTTP Request method(\"") + params->method + _T("\")");
		TRACE(_T("result = %s\n"), params->result);
		return;
	}

	HINTERNET hInternetRoot = InternetOpen(_T("request_url"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternetRoot == NULL)
	{
		params->status = -1;
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
		params->status = -1;
		TRACE(_T("result = %s\n"), params->result);
		return;
	}


	//HINTERNET hURL = InternetOpenUrl(hInternetRoot, remoteURL, szHead, -1L, secureFlags, 0);
	//if (hURL == NULL) {
	//	InternetCloseHandle(hInternetRoot);
	//	return _T("error=InternetOpenUrl() failed.");
	//}

	int secureFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;// | INTERNET_FLAG_TRANSFER_BINARY; // http
	if (params->is_https)
	{
		secureFlags |= INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID; // https
	}

	HINTERNET hOpenRequest = HttpOpenRequest(hInternetConnect,
		params->method,
		params->sub_url,
		HTTP_VERSION,
		_T(""),
		NULL,
		secureFlags,
		0);

	DWORD dwTimeout = 10000;
	InternetSetOption(hOpenRequest, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(DWORD));

	if (params->is_https)
	{
		DWORD dwFlags = 0;
		DWORD dwBuffLen = sizeof(dwFlags);

		dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
			SECURITY_FLAG_IGNORE_REVOCATION |
			SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP |
			SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS |
			SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
			SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
		InternetSetOption(hOpenRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
	}

	//����Ʈ ��� ����
	params->headers.push_front(_T("Content-Type: application/json; charset=utf-8\r\n"));

	if (params->headers.size() >= 1)
	{
		//����ڰ� ����� ����� �߰��Ѵ�.
		for (int i = 0; i < params->headers.size(); i++)
		{
			if (params->headers[i].Right(2) != _T("\r\n"))
				params->headers[i] += _T("\r\n");
			HttpAddRequestHeaders(hOpenRequest, params->headers[i], -1, HTTP_ADDREQ_FLAG_ADD);
		}
	}


#ifdef _UNICODE
	int char_str_len = WideCharToMultiByte(CP_UTF8, 0, params->body, -1, NULL, NULL, NULL, NULL);
	char jsonData[1024] = { 0, };
	ZeroMemory(jsonData, char_str_len);
	WideCharToMultiByte(CP_UTF8, 0, params->body, -1, jsonData, char_str_len, 0, 0);

	BOOL res = HttpSendRequest(hOpenRequest, NULL, 0, jsonData, strlen(jsonData));
#else
	char jsonData[1024] = { 0, };
	char* charMsg = params->body.GetBuffer(params->body.GetLength());
	//int char_str_len = WideCharToMultiByte(CP_UTF8, 0, CT2CA(jsonBody), -1, NULL, 0, NULL, NULL);
	//sprintf(jsonData, "%s", jsonBody);
	int char_str_len = WideCharToMultiByte(CP_UTF8, 0, (CStringW)(params->body), -1, NULL, 0, NULL, NULL);

	WideCharToMultiByte(CP_UTF8, 0, (CStringW)params->body, -1, jsonData, char_str_len, 0, 0);
	//CString2char()
	BOOL res = HttpSendRequest(hOpenRequest, NULL, 0, jsonData, strlen(jsonData));
#endif

	if (!res)
	{
		DWORD dwError = GetLastError();
		params->status = -1;
		params->result.Format(_T("HttpSendRequest failed. error code = %d(%s)"), dwError, get_last_error_string(dwError, false));
		TRACE(_T("result = %s\n"), params->result);

		InternetCloseHandle(hOpenRequest);
		InternetCloseHandle(hInternetConnect);
		InternetCloseHandle(hInternetRoot);

		return;
	}

	DWORD buffer_size = 1024 * 1024;
	DWORD dwRead, dwWritten, dwTotalSize = 0;
	char* buffer = new char[buffer_size];
	char* total_buffer = NULL;
	TCHAR query_buffer[32] = { 0, };
	DWORD query_buffer_size = sizeof(query_buffer);

	memset(buffer, 0, buffer_size);

	if (params->local_file_path.IsEmpty())
	{
		total_buffer = new char[buffer_size * 10];
		memset(total_buffer, 0, buffer_size * 10);
	}

	//size_buffer�� char�� ����ũ�⸦ �����´�.
	//������ �������� �ʾƵ� ���� ������ ���Ե� html�� �Ѿ���Ƿ� �׻� �� ���� 0���� ũ��.
	// �������� Ȯ��

	ret = HttpQueryInfo(hOpenRequest, HTTP_QUERY_STATUS_CODE, (LPVOID)&query_buffer, &query_buffer_size, NULL);
	params->status = _ttol(query_buffer);

	if (!ret)
	{
		SAFE_DELETE_ARRAY(buffer);
		SAFE_DELETE_ARRAY(total_buffer);
		InternetCloseHandle(hOpenRequest);
		InternetCloseHandle(hInternetConnect);
		InternetCloseHandle(hInternetRoot);

		params->result = _T("HttpQueryInfo(HTTP_QUERY_STATUS_CODE) failed.");
		params->status = -1;
		TRACE(_T("result = %s\n"), params->result);
		return;
	}

	if (params->status != HTTP_STATUS_OK)
	{
		SAFE_DELETE_ARRAY(buffer);
		SAFE_DELETE_ARRAY(total_buffer);
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

	//0����Ʈ�� ������ �ٿ���� �ʾƵ� �� �� ������
	//������ ���ϰ� �ٿ���� ������ ������ ���� ������ ���� ���� ���� �����Ƿ� ��������.
	//HTTP_QUERY_FLAG_NUMBER�� ���� ������ HttpQueryInfo()���� ������ �߻��Ѵ�.
	DWORD dwBufLen = sizeof(dwTotalSize);
	uint64_t total_read = 0;
	ret = HttpQueryInfo(hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwTotalSize, &dwBufLen, NULL);

	do
	{
		//InternetQueryDataAvailable(hOpenRequest, &dwSize, 0, 0); //�� �Լ��� ���������� ũ�⸦ �����ϴ� ���ϴ�.
		long t2 = clock();
		//1.48GB, buffer_size�� ���� �ð� ��. 1K:133s, 4K:65s, 1M:63s, 4M:64s
		//���� ũ�Ⱑ 1K�� �ʹ� ����� read�� �߻��Ͽ� �������� 4K�̻��̸� ū ���̴� �߻����� �ʴ´�.
		InternetReadFile(hOpenRequest, buffer, buffer_size, &dwRead);

		total_read += dwRead;

		if (dwRead == 0)
			break;

		if (params->local_file_path.IsEmpty())
		{
			strncat(total_buffer, buffer, dwRead);
		}
		else
		{
			//remote file�� �������� ���� ��� ���ÿ� ������ ������ �ʱ� ���� ���⼭ üũ.
			if (hFile == NULL)
			{
				if (PathFileExists(params->local_file_path) && !DeleteFile(params->local_file_path))
				{
					SAFE_DELETE_ARRAY(buffer);
					SAFE_DELETE_ARRAY(total_buffer);
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
					SAFE_DELETE_ARRAY(total_buffer);
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

	if (params->local_file_path.IsEmpty())
	{
		params->result = UTF8toCString(total_buffer);
	}
	else
	{
		CloseHandle(hFile);
		params->result = _T("");
	}

	SAFE_DELETE_ARRAY(buffer);
	SAFE_DELETE_ARRAY(total_buffer);
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
	// ���ͳ� ������ �ȵ� ��� ���� ������ �õ��ϸ� ������ �߻��ϹǷ� �켱 üũ�ؾ� �Ѵ�.
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
	
	// ������ �����ϸ�...
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
		return _T("��Ʈ��ũ�� ������ �� �����ϴ�.");

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

CString GetDefaultBrowserPath()
{
	TCHAR szPath[_MAX_PATH];

	HFILE h = _lcreat("dummy.htm", 0) ;
	_lclose(h);

	FindExecutable(_T("dummy.htm"), NULL, szPath);
	DeleteFile(_T("dummy.htm"));

	return szPath;
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
	LPVOID lpMsgBuf;

	if (nError == ERROR_SUCCESS)
	{
		if (hkey)
		{
			//WinXP SP3���� RegGetValue()�� �������� �ʾ� RegQueryValueEx()�� ������.
			//nError = RegGetValue(hKeyRoot, sSubKey, sEntry, RRF_RT_DWORD, &dwType, value, &cbData);

			nError = RegQueryValueEx(hkey, sEntry, NULL, NULL, reinterpret_cast<LPBYTE>(value), &buf_size);

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
				AfxMessageBox((LPCTSTR)lpMsgBuf, MB_ICONERROR);
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

LONG get_registry_string(HKEY hKeyRoot, CString sSubKey, CString entry, CString *str)
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

LONG set_registry_string(HKEY hKeyRoot, CString sSubKey, CString entry, CString str)
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

bool	LoadBitmapFromFile(CBitmap &bmp, CString strFile)
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
	// BITMAPFILEHEADER�� bfSize�� ����Ѵ�.
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



//���� ������ �ε��� ����
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

CString		GetMostRecentDateFile(CString sFolder, CString sWildCard /*= "*.*"*/)	//���� �ֱ� ��¥ ���ϸ� ����
{
	CFileFind	FileFind;
	bool		bWorking;
	CString		sMostRecentDateFile = _T("");
	CString		sfile;
	CTime		tFile;
	
	//���� �ð� �ִ�� 1970�� 1�� 1�� 0�ð� �ƴ� 9�ô�.
	//�ֳĸ� �츮���� GMP+9�̹Ƿ�. 1970-1-1 8:59:59 �̰͵� assert fail ����.
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
			_tcscpy(pInfo->sDescription, A2T(pAdapter->Description));

			if (pAdapter->Type == MIB_IF_TYPE_ETHERNET && CString(pInfo->sDescription).Find(sTargetDeviceDescription) >= 0)
			{
				_tcscpy(pInfo->sIPAddress, A2T(pAdapter->IpAddressList.IpAddress.String));
				_tcscpy(pInfo->sGateway, A2T(pAdapter->GatewayList.IpAddress.String));
				_tcscpy(pInfo->sSubnetMask, A2T(pAdapter->IpAddressList.IpMask.String));

				//TCHAR sMacAddress[16] �̹Ƿ� �����ڸ� �־��ָ� �ȵȴ�.
				//main���� Functions.h�� get_mac_address_format()�Լ��� �����ڸ� �־ ǥ���� ��.
				_stprintf(pInfo->sMacAddress, _T("%02X%02X%02X%02X%02X%02X"),
					pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2], pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
				result = true;
				break;
			}

			pAdapter = pAdapter->Next;
		}
	}

	if (pAdapterInfo)
		free(pAdapterInfo);

	return result;
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
			sMsg.Format(_T("���� ���� ���� : \n\n��ȣȭ�� ���� %s ������ �� �� �����ϴ�."), sSource);
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
uint64_t get_disk_free_size(CString sDrive)
{
	TCHAR Drive[10];
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

	return (uint64_t)(m_lTotalNumberOfFreeBytes.QuadPart);
}

uint64_t get_disk_total_size(CString sDrive)
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

	return (uint64_t)(m_lTotalNumberOfBytes.QuadPart);
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
	
	if (nDotPos > 0)	// �Ǽ��� ���
	{
		sFinal		= sString.Mid(nDotPos);
		sIntPart	= sString.Left(nDotPos);
	}
	else
	{
		sIntPart	= sString;
	}
	
	//�� �� ���ں��� �ϳ��� �� ���ڿ��� �߰���Ų��.
	//3�ڸ� �� ������ �޸��� �־��ش�.
	for (int i = 0; i < sIntPart.GetLength(); i++)
	{
		sFinal.Insert(0, sIntPart.GetAt(sIntPart.GetLength() - 1 - i));
		
		if (((i + 1) % 3 == 0) &&				//3° �ڸ�����
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

//value�� 3�ڸ��̰� total_digits�� 7�̸� 4���� 0�� �տ� �ٿ��� �����Ѵ�.
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

// ����
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


//�Ҽ��� �ڸ��� ����.
//0.123456789 �� �Ѱ��൵ d���� �״�� �Ѱܹ�����
//sprintf���� 0.123457�� ����ȴ�.(�Ҽ��� 6�ڸ��θ� ó���ȴ�.)
//�켱 ������ �����ϰ� ���� �ڸ����� ���� ��Ȯ�� �����ؾ� �Ѵٸ�
//�ڵ带 �� �� ��������.
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
			//0�� �ƴ� �ڸ����� �ѹ� ������ �� �ķδ� ��ȿ 0�̹Ƿ� ī��Ʈ�ؾ� �Ѵ�.
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

	_tprintf(_T("reverse origin  = %s\n"), getBinaryString(reverse, true));
	reverse = reverse >> startbit;
	_tprintf(_T("reverse shifted = %s\n"), getBinaryString(reverse, true));

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

//��� : unsigned ���� signed ������ �����Ѵ�.
//ex1. 0 ~ 99 ������ ���� unsigned ������ Ÿ���� signed�� ������ ���
//signed�� ������ -50 ~ +49�� ���̴�.
//53�̶�� ���� 53 - 100 = -47�� �ȴ�.
//signed�� max�� 49���� �۰ų� ���� ���� �׳� ����
//ex2. 0 ~ 3 ������ ���� 2bit ������ Ÿ���� signed�� ������ ���
//signed�� ������ -2 ~ +1�� ���̴�.
//3�̶�� ���� 3 - 4 = -1�� �ȴ�.
//signed�� max�� 1���� �۰ų� ���� ���� �׳� ����
int getSignedFromUnsigned(unsigned value, int bit_length)
{
	int range_max = pow(2.0, bit_length);
	int signed_max = range_max / 2 - 1;

	if (value <= signed_max)
		return value;

	return (value - range_max);
}

//dlg�� OnInitDialog()���� �� �Լ��� ȣ���� ���
//�������� �� ����Ǿ����� ��10�� IME ����� ����Ǿ�����
//OnInitDialog()�� ������ ������� �ʴ´�.
//SetTimer�� UI ��ư�� �߰��Ͽ� �����ϸ� �� ������.
//��, OnInitDialog()���� Ÿ�̸Ӹ� 100ms�� ���� �ʹ� ª�� �ָ� ���� �ȵ�. �˳��� 500ms �̻� �� ��.
void IME_Convert_To_NativeCode(HWND hWnd, bool bNative)
{
	//�Ʒ� �ּ�ó���� ���� ���� ����̰�
	//�� �Ʒ� 3���� ������ ������ ���. �� �� ���� ������.
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
	// �����̵Ƿ���
	// 1. 4�� ������ �������� ����� �ϰ�
	// 2. 100���� ����������� �ʰų�,
	// 400���ε� ����������� ��
	
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
				  bool bRecursive, CString sExceptStr, bool auto_sort)
{
	int				i;
	CTime			tTime;
	CString			sfile;
	CString			sFilename;
	CFileFind		finder;

	if (sNameFilter == "")
		sNameFilter = _T("*");

	//���� ����
	std::deque<CString>	dqExtFilter;

	//���� �׸��� ���� ������
	std::deque<CString>	dqExcepts;
	CString			sToken;
	int				curPos;

	if (sExtFilter == "")
		sExtFilter = _T("*");

	//���Ϳ� ���� ���ڿ��� ��ҹ��ڸ� �������� �ʴ´�.
	curPos = 0;
	sToken = sExtFilter;
	sExtFilter.MakeLower();
	dqExtFilter.clear();

	//Ȯ���ڴ� "jpg"�� ���� dot�� ���� ���Ѵ�.
	//Ȯ���ڿ� * �Ǵ� dot�� �ִٸ� �������ش�.
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

	//recursive���� ����Ͽ� �켱�� ��� ������ ������� �ؾ� �Ѵ�.
	bool bWorking = finder.FindFile(sFolder + sNameFilter);

	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		sfile = finder.GetFilePath();
		sFilename = get_part(sfile, fn_name).MakeLower();
		
		if (finder.IsDots())
			continue;
		else if (finder.IsDirectory())
		{
			if (bRecursive)
				FindAllFiles(sfile, dqFiles, sNameFilter, sExtFilter, bRecursive, sExceptStr, auto_sort);
		}
		else
		{
			//sFilter�� �ش��ϴ� ���ϸ��� �˻� ������� �Ѵ�.
			bool bFound = false;

			if (dqExtFilter.size() == 0)
			{
				bFound = true;
			}
			else
			{
				//������� Ȯ���� ��Ͽ� jpg�� �ִµ� ���ϸ� �߰����� jpg�� �߰ߵǸ� ������ �ȴ�.
				if (find_index(dqExtFilter, get_part(sFilename, fn_ext).MakeLower()) >= 0)
					bFound = true;
				/*
				for (i = 0; i < dqExtFilter.size(); i++)
				{
					//if (PathMatchSpec(sFilename, arExtFilter.GetAt(i)))
					CString sExt = GetFileExtensionFromFilename(sFilename);
					if (sExt == dqExtFilter[i])
					{
						bFound = true;
						break;
					}
				}*/
			}

			//if (arFilter.GetCount() == 1 && (arFilter.GetAt(0) == "*" || arFilter.GetAt(0) == "*.*"))

			if (bFound)
			{
				//������ ���ڿ��� ���ԵǾ� ���� ���� ���ϵ鸸 ����Ʈ�� �߰��Ѵ�.
				bool bExcept = false;

				for (i = 0; i < dqExcepts.size(); i++)
				{
					if (sFilename.Find(dqExcepts[i]) >= 0)
					{
						bExcept = true;
						break;
					}
				}

				if (!bExcept)
				{
					dqFiles->push_back(sfile);
				}
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

//list�� NULL�� ȣ���ϸ� ���� sub folder�� ������ ������ �����̴�.
//recursive�� �������� �ʴ´�.
//root�� "�� PC"�� ��� special_folders�� true�̸� �ٿ�ε�, �� ����, ���� ȭ�� �׸���� �߰��Ѵ�.
int get_sub_folders(CString root, std::deque<CString>* list, bool special_folders, bool include_files)
{
	if (list)
		list->clear();

	std::deque<CString> folders;

	CString file;
	CFileFind finder;

	//"���� ��ũ (C:)"
	root = convert_special_folder_to_real_path(root);

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
		folders.push_front(get_system_label(CSIDL_PERSONAL));
		//folders.push_front(get_system_label(CSIDL_DOWNLOAD)_T("�ٿ�ε�"));
	}
	else if (root == get_system_label(CSIDL_DRIVES))
	{
		std::map<TCHAR, CString> drive_map;
		get_drive_map(&drive_map);
		for (std::map<TCHAR, CString>::iterator it = drive_map.begin(); it != drive_map.end(); it++)
			folders.push_back(it->second);

		if (special_folders)
		{
			folders.push_front(get_system_label(CSIDL_DESKTOP));
			folders.push_front(get_system_label(CSIDL_PERSONAL));
			//folders.push_front(_T("�ٿ�ε�"));
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
	if (path.Right(1) != _T("\\"))
		path += _T("\\");

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
	//���Ϳ� ���� ���ڿ��� ��ҹ��ڸ� �������� �ʴ´�.
	int curPos = 0;
	CString sToken = ext_filters;
	CString file;
	CString filetitle;
	CString name_field = name_filter.MakeLower();

	//name_filter���� wildcard�� ������ �κ�
	name_field.Replace(_T("*"), _T(""));
	name_field.Replace(_T("?"), _T(""));

	//name_filter�� �ϴ� *�� ����Ѵ�.
	//abc* : 
	//*abc
	//*abc*
	if (name_filter == "*")
		name_filter = _T("");

	if (ext_filters == "")
		ext_filters = _T("*");

	ext_filters.MakeLower();

	//Ȯ���ڴ� "jpg"�� ���� dot�� ���� ���Ѵ�.
	//Ȯ���ڿ� * �Ǵ� dot�� �ִٸ� �������ش�.
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
			//������ ���ڿ��� ���ԵǾ� ���� ���� ���ϵ鸸 ����Ʈ�� �߰��Ѵ�.
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
			//������ ���ڿ��� ���ԵǾ� ���� ���� ���ϵ鸸 ����Ʈ�� �߰��Ѵ�.
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

	//  UniCode BOM ���
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

	//Ȯ���ڴ� "jpg"�� ���� dot�� ���� ���Ѵ�.
	//Ȯ���ڿ� * �Ǵ� dot�� �ִٸ� �������ش�.
	extensions.Replace(_T("*"), _T(""));
	extensions.Replace(_T("."), _T(""));

	get_token_string(extensions, dqToken, ';');

	//�������� ���ϵ��� ã�Ƽ� ���ϴ°� �ƴ϶�
	//���ǿ� �´� ������ �����ϴ����� �˻��Ѵ�.
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
	FileOp.wFunc = FO_DELETE; // ���� �Ӽ� ����
	FileOp.pFrom = NULL;
	FileOp.pTo = NULL;
	//Ȯ�θ޽����� �ȶߵ��� ����
	FileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI;
	FileOp.fAnyOperationsAborted = false;
	FileOp.hNameMappings = NULL;
	FileOp.lpszProgressTitle = NULL;
	FileOp.pFrom = szTemp;

	int res = SHFileOperation(&FileOp); // ���� �۾�
	if (res == 0)
		return true;

	return false;
}

int	get_text_encoding(CString sfile)
{
	int text_encoding = text_encoding_ansi;

	unsigned char buf[16];

	FILE* fp = _tfopen(sfile, _T("rb"));
	if (fp == NULL)
		return text_encoding_unknown;

	fread(buf, 1, 16, fp);
	if (buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
		text_encoding = text_encoding_utf8bom;
	else if (buf[0] == 0xff && buf[1] == 0xfe)
		text_encoding = text_encoding_unicode;

	fclose(fp);

	return text_encoding;
}

//20231206
//unicode �϶��� code_page�� ���� ansi or utf8�� ����ǳ�
//multibyte �϶��� utf8�� �����ص� ansi�� �����.
bool save(CString filepath, CString text, int code_page)
{
	if (code_page != CP_ACP && code_page != CP_UTF8)
		code_page = CP_UTF8;

	//std::locale::global(std::locale(".UTF-8"));
	//setlocale(LC_ALL, ".utf8");

	//unicode ȯ�濡���� �Ʒ��� ���� �����ϳ� multibyte������ ansi�θ� �����.
	//CT2A(text)			: ANSI�� ����
	//CT2CA(text, CP_UTF8)	: UTF8�� �����
	std::ofstream of;
	
	of.open(filepath, std::ofstream::out);
	
	if (!of.is_open())
		return false;

	//USES_CONVERSION;
	//text = W2A_CP(text, code_page);
	//of << text;
	// 
	of << CT2CA(text, code_page);
	//of << CA2CT(text, code_page);
	of.close();

	/*
	FILE* fp = NULL;

	_tfopen_s(&fp, filepath, _T("wt")CHARSET);

	if (fp == NULL)
	{
		//AfxMessageBox(filepath + _T("\n�� ������ �� �� �����ϴ�."), MB_ICONEXCLAMATION);
		return false;
	}

	_ftprintf(fp, _T("%s\n"), text);

	fclose(fp);
	*/

	return true;
}

bool file_open(FILE** fp, CString mode, CString file)
{
	//encording ����� �о�´�.
	int	text_encoding = get_text_encoding(file);

	if (text_encoding <= text_encoding_ansi)
		_tfopen_s(fp, file, mode);
	else
		_tfopen_s(fp, file, mode + CHARSET);

	return (fp == NULL ? false : true);
}

//�������� �� ���� �������� ������ �����ִ�.
//���� ��°�� �� ������� �ڵ��� ������ �ʿ��ϴ�.
int	delete_all_files(CString folder, CString name_filter, CString ext_filter, bool recursive /*= true*/, bool trash_can /*= false*/)
{
	std::deque<CString> files;
	FindAllFiles(folder, &files, name_filter, ext_filter, recursive);

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

//���ڷ� �Ѿ�� ������ �����Ͽ� ��� ��������, ������ �����.
bool SHDeleteFolder(CString sFolder)
{
    SHFILEOPSTRUCT FileOp = {0};
    TCHAR szTemp[MAX_PATH];
 
	_stprintf(szTemp, _T("%s"), sFolder);
    szTemp[_tcslen(szTemp) + 1] = NULL;
 
    FileOp.hwnd = NULL;
    FileOp.wFunc = FO_DELETE;       // ���� �Ӽ� ����
    FileOp.pTo = NULL;
    FileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SIMPLEPROGRESS; // Ȯ�θ޽����� �ȶߵ��� ����
    FileOp.fAnyOperationsAborted = false;
    FileOp.hNameMappings = NULL;
    FileOp.lpszProgressTitle = sFolder;
    FileOp.pFrom = szTemp;
 
    if (SHFileOperation(&FileOp) == 0)       // ���� �۾�
        return TRUE;

	return FALSE;
}
 
//------------------------------------------------------------
//�������� ��� ����
//���� ���丮 ��� ����
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

//�������� Ư�� ���ڿ��� �� ���ϵ��� �����. �������� ��������.
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

//�� �Լ��� ����Ϸ��� �ݵ�� SetBkMode(TRANSPARENT);�� �����ؾ� ȿ���� ��Ÿ��.
void TextOutShadow(CDC* pDC, int x, int y, CString sText, COLORREF crText, COLORREF crShadow, UINT nFlag)
{
	int oldTextColor = pDC->GetTextColor();
	int oldBkMode = pDC->SetBkMode(TRANSPARENT);
	UINT nOldFlag = pDC->SetTextAlign(nFlag);

	pDC->SetTextColor(crShadow);
	pDC->TextOut(x + 1, y + 1, sText);
	pDC->SetTextColor(crText);
	pDC->TextOut(x, y, sText);

	pDC->SetTextAlign(nOldFlag);
	pDC->SetTextColor(oldTextColor);
	pDC->SetBkMode(oldBkMode);
}

//�� �Լ��� ����Ϸ��� �ݵ�� SetBkMode(TRANSPARENT);�� �����ؾ� ȿ���� ��Ÿ��.
void DrawTextShadow(CDC* pDC, CString sText, CRect r, UINT format,
					COLORREF crText, COLORREF crShadow, int offsetX, int offsetY)
{
	int oldTextColor = pDC->GetTextColor();
	int oldBkMode = pDC->SetBkMode(TRANSPARENT);

	r.OffsetRect(offsetX, offsetY);
	pDC->SetTextColor(crShadow);
	pDC->DrawText(sText, r, format);
	r.OffsetRect(-offsetX, -offsetY);
	pDC->SetTextColor(crText);
	pDC->DrawText(sText, r, format);

	pDC->SetTextColor(oldTextColor);
	pDC->SetBkMode(oldBkMode);
}

//�� �Լ��� ����Ϸ��� �ݵ�� SetBkMode(TRANSPARENT);�� �����ؾ� ȿ���� ��Ÿ��.
void TextOutOutline(CDC* pDC, int x, int y, CString sText, COLORREF crText, COLORREF crBorder, UINT nFlag)
{
	int oldTextColor = pDC->GetTextColor();
	int oldBkMode = pDC->SetBkMode(TRANSPARENT);
	UINT nOldFlag = pDC->SetTextAlign(nFlag);

	pDC->SetTextColor(crBorder);
	pDC->TextOut(x - 1, y + 0, sText);
	pDC->TextOut(x + 0, y - 1, sText);
	pDC->TextOut(x + 1, y + 0, sText);
	pDC->TextOut(x + 0, y + 1, sText);

	pDC->SetTextColor(crText);
	pDC->TextOut(x, y, sText);

	pDC->SetTextAlign(nOldFlag);
	pDC->SetTextColor(oldTextColor);
	pDC->SetBkMode(oldBkMode);
}

//�� �Լ��� ����Ϸ��� �ݵ�� SetBkMode(TRANSPARENT);�� �����ؾ� ȿ���� ��Ÿ��.
void DrawTextOutline(CDC* pDC, CString sText, CRect r, UINT format, COLORREF crText, COLORREF crShadow)
{
	int oldTextColor = pDC->GetTextColor();
	int oldBkMode = pDC->SetBkMode(TRANSPARENT);

	pDC->SetTextColor(crShadow);

	r.OffsetRect(-1, 0);
	pDC->DrawText(sText, r, format);
	r.OffsetRect(+1, -1);
	pDC->DrawText(sText, r, format);
	r.OffsetRect(+1, +1);
	pDC->DrawText(sText, r, format);
	r.OffsetRect(-1, +1);
	pDC->DrawText(sText, r, format);

	pDC->SetTextColor(crText);
	r.OffsetRect(0, -1);
	pDC->DrawText(sText, r, format);

	pDC->SetTextColor(oldTextColor);
	pDC->SetBkMode(oldBkMode);
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

//����� ���ڸ� �۰� ����� �� �̸� �ٽ� ���� ũ��� �÷�
//blur�� ����� �ϰ� �̸� shadow�� ����ϴ� ����ε� ���� ����ϴ�.
//ApplyEffect�� blur�� �����ؼ� �����ϴ� ���� ���� �� �ϴ�.
CRect draw_text(Gdiplus::Graphics* g,
				int x, int y, int w, int h,
				CString text,
				float font_size,
				bool font_bold,
				int shadow_depth,
				float thickness,
				CString font_name,
				Gdiplus::Color cr_text,
				Gdiplus::Color cr_stroke,
				Gdiplus::Color cr_shadow,
				UINT align)
{
	return draw_text(g,
					CRect(x, y, x + w, y + h),
					text,
					font_size,
					font_bold,
					shadow_depth,
					thickness,
					font_name,
					cr_text,
					cr_stroke,
					cr_shadow,
					align);
}

CRect draw_text(Gdiplus::Graphics* g,
				CRect rTarget,
				CString text,
				float font_size,
				bool font_bold,
				int shadow_depth,
				float thickness,
				CString font_name,
				Gdiplus::Color cr_text,
				Gdiplus::Color cr_stroke,
				Gdiplus::Color cr_shadow,
				UINT align)
{
	bool calcRect = false;
	HDC hDC = ::GetDC(AfxGetMainWnd()->m_hWnd);
	//HDC hdcMemory = NULL;

	if (g == NULL)
	{
		calcRect = true;

		//hdcMemory = ::CreateCompatibleDC(hDC);
		g = new Gdiplus::Graphics(hDC);
	}

	//ū �۾��� AntiAlias�� ���ִ°� ������ ���� �۾��� ������ �������Ƿ� ���ϴ°� ����.
	//�Ķ���ͷ� ó���ؾ� �Ѵ�.
	g->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	Gdiplus::Unit unit = g->GetPageUnit();
	float fDpiX = g->GetDpiX();
	float fDpiY = g->GetDpiY();

	int logPixelsY = ::GetDeviceCaps(hDC, LOGPIXELSY);
	//Gdiplus::REAL emSize = (Gdiplus::REAL)MulDiv(font_size, 96, logPixelsY);
	float emSize = fDpiY * font_size / 96.0;

	Gdiplus::FontFamily fontFamily((WCHAR*)(const WCHAR*)CStringW(font_name));
	Gdiplus::Font font(&fontFamily, emSize, font_bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular);

	Gdiplus::SolidBrush shadow_brush(cr_shadow);
	Gdiplus::SolidBrush brush2(cr_text);

	Gdiplus::StringFormat sf;
	sf.SetAlignment(Gdiplus::StringAlignmentCenter);
	sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);

	int x = rTarget.left;
	int y = rTarget.top;

	Gdiplus::RectF boundRect;
	g->MeasureString(CStringW(text), -1, &font, Gdiplus::PointF(x, y), &boundRect);
	//boundRect = measure_string(g, font, text);

	boundRect.Width += shadow_depth;
	boundRect.Height += shadow_depth;

	if (rTarget.Width() == 0)
		rTarget.right = rTarget.left + boundRect.Width;
	if (rTarget.Height() == 0)
		rTarget.bottom = rTarget.top + boundRect.Height;

	if (align & DT_CENTER)
		x = rTarget.CenterPoint().x - boundRect.Width / 2;
	else if (align & DT_RIGHT)
		x = rTarget.right - boundRect.Width;

	if (align & DT_VCENTER)
		y = rTarget.CenterPoint().y - boundRect.Height / 2;
	else if (align & DT_BOTTOM)
		y = rTarget.bottom - boundRect.Height;

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

		delete g;
		::DeleteDC(hDC);
		TRACE(_T("%f, %f, %f x %f\n"), boundRect.X, boundRect.Y, boundRect.Width, boundRect.Height);
		return CRect(x, y, x + boundRect.Width, y + boundRect.Height);
	}

	//g_shadow.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	/*
	Gdiplus::Blur blur;
	BlurParams blurParam;

	blur.SetParameters(&blurParam);

	//Gdiplus::Bitmap::ApplyEffect(&m_pBitmap, 1, &hsl, NULL, NULL, &m_pBitmap);
	bm.ApplyEffect(&hsl, NULL);
	*/

	if (shadow_depth > 0)
	{
		Gdiplus::Bitmap shadow_bitmap(boundRect.Width, boundRect.Height);
		Gdiplus::Graphics g_shadow(&shadow_bitmap);
		g_shadow.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		g_shadow.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		g_shadow.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

		//�׸����� �帴�� ����. 0.0f(�׸��� ����), 0.1f(���� �帲), 0.4f(����), 1.0f�� �������� ������.
		//����� ���ڸ� ����ũ��� �÷��� �帴�� �̹����� ����.
		float ratio = 0.3f;
		Gdiplus::Matrix mx(ratio, 0, 0, ratio, 0.0f, 0.0f);
		g_shadow.SetTransform(&mx);
		g_shadow.DrawString(CStringW(text), -1, &font, boundRect, &sf, &shadow_brush);

		//boundRect.X = x;
		//boundRect.Y = y;
		//boundRect.Width = shadow_bitmap.GetWidth();
		//boundRect.Height = shadow_bitmap.GetHeight();
		g->DrawImage(&shadow_bitmap, (Gdiplus::REAL)shadow_depth, (Gdiplus::REAL)shadow_depth,
			(Gdiplus::REAL)(shadow_bitmap.GetWidth())/ratio, (Gdiplus::REAL)(shadow_bitmap.GetHeight())/ratio);
	}

	if (thickness == 0.0)
	{
		g->DrawString(CStringW(text), -1, &font, boundRect, &sf, &brush2);
	}
	else
	{
		float emSize = fDpiY * font_size / 72.0;
		//float emSize = fDpiY * font.GetSize() / 72.0;
		Gdiplus::GraphicsPath str_path;

		str_path.AddString(CStringW(text), -1, &fontFamily,
			font_bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular, emSize, boundRect, &sf);
		//str_path.AddString(CStringW(text), -1, &fontFamily,
		//	font_bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular, emSize, Gdiplus::Point(x, y), &sf);

		Gdiplus::Pen   gp(cr_stroke, thickness);
		//gp.SetLineJoin(Gdiplus::LineJoinMiter);
		Gdiplus::SolidBrush gb(cr_text);

		g->DrawPath(&gp, &str_path);
		g->FillPath(&gb, &str_path);
	}

	::DeleteDC(hDC);

	return CRect(x, y, x + boundRect.Width, y + boundRect.Height);
}

//text�� ����ȼ� �ʺ� max_width�� ���� ��� ...�� �Բ� ǥ�õ� ������ġ�� ����.
//�� �Լ��� DrawText�ÿ� DT_END_ELLIPSIS�� �༭ ����ϹǷ� �켱 ��� ����!
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

void DrawLinePt(CDC* pDC, CPoint pt1, CPoint pt2, COLORREF crColor /*= 0*/, int nWidth /*= 1*/, int nPenStyle /*= PS_SOLID*/, int nDrawMode /*= R2_COPYPEN*/)
{
	DrawLine(pDC, pt1.x, pt1.y, pt2.x, pt2.y, crColor, nWidth, nPenStyle, nDrawMode);
}

void DrawLine(CDC* pDC, int x1, int y1, int x2, int y2, COLORREF crColor /*= 0*/, int nWidth /*= 1*/, int nPenStyle /*= PS_SOLID*/, int nDrawMode /*= R2_COPYPEN*/)
{
	LOGBRUSH lb;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = crColor;

	CPen	Pen(PS_GEOMETRIC | PS_ENDCAP_ROUND | PS_JOIN_MITER | nPenStyle, nWidth, &lb);
	CPen*	pOldPen = (CPen*)pDC->SelectObject(&Pen);
	int		nOldDrawMode = pDC->SetROP2(nDrawMode);

	pDC->MoveTo(x1, y1);
	pDC->LineTo(x2, y2);

	pDC->SelectObject(pOldPen);
	Pen.DeleteObject();

	pDC->SetROP2(nOldDrawMode);
}

void DrawRectangle(CDC* pDC, int x1, int y1, int x2, int y2, COLORREF crColor/* = RGB(0,0,0)*/, COLORREF crFill /*= NULL_BRUSH*/, int nWidth/* = 1*/, int nPenStyle/* = PS_SOLID*/, int nDrawMode /* = R2_COPYPEN*/)
{
	DrawRectangle(pDC, CRect(x1, y1, x2, y2), crColor, crFill, nWidth, nPenStyle, nDrawMode);
}


void DrawRectangle(CDC* pDC, CRect Rect, COLORREF crColor/* = RGB(0,0,0)*/, COLORREF crFill /*= NULL_BRUSH*/, int nWidth/* = 1*/, int nPenStyle/* = PS_SOLID*/, int nDrawMode /* = R2_COPYPEN*/)
{
	LOGBRUSH lb;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = crColor;

	CPen	Pen(PS_GEOMETRIC | nPenStyle, nWidth, &lb);
	CPen*	pOldPen = (CPen*)pDC->SelectObject(&Pen);
	int		nOldDrawMode = pDC->SetROP2(nDrawMode);
	CBrush	brBrush(crFill);
	CBrush*	pOldBrush;
	
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

void DrawSunkenRect(CDC* pDC, CRect Rect, bool bSunken, COLORREF cr1, COLORREF cr2, int nWidth)
{
	CPen	Pen1(PS_SOLID, nWidth, cr1);
	CPen	Pen2(PS_SOLID, nWidth, cr2);
	CPen*	pOldPen;

	pOldPen = (CPen*)pDC->SelectObject(bSunken ? &Pen1 : &Pen2);

	pDC->MoveTo(Rect.left, Rect.bottom - 1);
	pDC->LineTo(Rect.left, Rect.top);
	pDC->LineTo(Rect.right - 1, Rect.top);

	pDC->SelectObject(pOldPen);

	pOldPen = (CPen*)pDC->SelectObject(bSunken ? &Pen2 : &Pen1);

	pDC->MoveTo(Rect.right - 1, Rect.top);
	pDC->LineTo(Rect.right - 1, Rect.bottom - 1);
	pDC->LineTo(Rect.left, Rect.bottom - 1);

	pDC->SelectObject(pOldPen);

	Pen1.DeleteObject();
	Pen2.DeleteObject();
}

void DrawEllipse(CDC* pDC, int cx, int cy, int rx, int ry, COLORREF crLine, COLORREF crFill, int nPenStyle, int nWidth, int nDrawMode)
{
	LOGBRUSH lb;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = crLine;

	CPen	Pen(PS_GEOMETRIC | nPenStyle, nWidth, &lb);
	CPen*	pOldPen = (crLine == NULL_PEN ? (CPen*)pDC->SelectStockObject(NULL_PEN) : (CPen*)pDC->SelectObject(&Pen));
	int		nOldDrawMode = pDC->SetROP2(nDrawMode);
	CBrush	brBrush(crFill);
	CBrush*	pOldBrush;

	if (crFill == NULL_BRUSH)
		pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
	else
		pOldBrush = (CBrush*)pDC->SelectObject(&brBrush);

	pDC->Ellipse(cx - rx, cy - ry, cx + rx, cy + ry);

	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
	Pen.DeleteObject();
	brBrush.DeleteObject();

	pDC->SetROP2(nOldDrawMode);
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

//������ �ü������ Ư�� ����(�ٿ�ε�, �� ���� ��)�� ���� ��θ� �����Ѵ�.
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
	TCHAR buf[MAX_PATH];
	SHGetSpecialFolderPath(NULL, buf, csidl, FALSE);
	return buf;
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

//Ÿ�̸ӿ� ���� ȣ��� �Լ������� ����� ��쿡�� ����� �������� ����.
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

//�ݺ����� ���� process�� ��������� ���� �ʵ��� �ݺ����ȿ��� ȣ���Ͽ� �޽���ť�� ������ �ٷ� ó����Ų��.
void ProcessWindowMessage()
{
   MSG msg;

   while(::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
   {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
   }
}

// 16��Ʈ ������ 2���� ���ڿ��� ��ȯ �Լ�
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

//� ���� x �ε��� ��ġ�� ��Ʈ���� ����.
int get_bit(int number, int x)
{
	int bit = (number >> x) & 1;
	return bit;
}

//x�� 1�̸� n��° ���� 1�� ����, x�� 0�̸� n��° ���� 0���� ����
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

CRect makeCenterRect(int cx, int cy, int w, int h)
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

CRect GpRect2CRect(Gdiplus::Rect r)
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
	//Arc�� �׸��� ���ʴ� tl-bl-br-tr�� �ƴϸ� round rect�� �׷����� �ʴ´�.
	//top-left round corder. 180�� ��ġ���� �ð�������� 90�� ��ŭ�� ȣ�� �׸���.
	if (true)
	{
		path->AddArc(arc, 180.0, 90.0);
	}
	else
	{
		//path->AddLine(r.X + );
	}

	//bottom-left round corder. 270�� ��ġ���� �ð�������� 90�� ��ŭ�� ȣ�� �׸���.
	arc.X = r.GetRight() - diameter - 1;
	path->AddArc(arc, 270.0, 90.0);

	//bottom-right round corder. 0�� ��ġ���� �ð�������� 90�� ��ŭ�� ȣ�� �׸���.
	arc.Y = r.GetBottom() - diameter - 1;
	path->AddArc(arc, 0.0, 90.0);

	//top-right round corder. 90�� ��ġ���� �ð�������� 90�� ��ŭ�� ȣ�� �׸���.
	arc.X = r.GetLeft();
	path->AddArc(arc, 90.0, 90.0);

	//�׷��� �ױ������� ȣ�� �����Ų��.
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

//0:lt, 1:rt, 2:rb, 3:lb, rb_cut�� true�̸� ����-1�� ���� �����ϰ� false�̸� ���� ��ǥ�� �����Ѵ�.
//���� �簢���� rb_cut�� false������
//���� �Ǵ� �ȼ��� ���ٸ� true�� �ؾ� �´�.
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


// ���Ͽ� ���� ��Ʈ���� �����.
// �Ʒ��� �Լ��� ���� �����ϴ�.
// pszSrcFile:���� ��� ������ Ǯ �н�
// pszLnkFile:���� ������ Ǯ �н�
// pszArgument:��Ʈ�ƿ� ���޵� �μ� ���ڿ�
// pszDesc:��Ʈ�� ���� ���ڿ�
HRESULT	MyCreateShortCut(LPCTSTR pszSrcFile, LPCOLESTR pszLnkFile,
						  LPTSTR pszWorkingDir/* = NULL*/, LPTSTR pszArgument/* = NULL*/, LPTSTR pszDesc/* = NULL*/)
{
	HRESULT			hr;
	IShellLink		*pSl;
	IPersistFile	*pPf;
	TCHAR wszLnkFile[MAX_PATH]={0,};
	
	// IShellLink ��ü�� �����ϰ� �����͸� ���Ѵ�.
	CoInitialize(NULL);

	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		 IID_IShellLink, (void **)&pSl);
	
	if (FAILED(hr))
		return E_FAIL;
	
	// ��Ʈ���� ���ü�� ������ �����Ѵ�.
	pSl->SetPath(pszSrcFile);
	pSl->SetArguments(pszArgument);
	pSl->SetDescription(pszDesc);
	pSl->SetWorkingDirectory(pszWorkingDir);
	
	// �����ϱ� ���� IPersistFile ��ü�� �����Ѵ�.
	hr = pSl->QueryInterface(IID_IPersistFile, (void **)&pPf);

	if (FAILED(hr))
	{
		pSl->Release();
		return E_FAIL;
	}
	
	// �����ڵ�� ���� �н��� ������ �� �����Ѵ�.
	//MultiByteToWideChar(CP_ACP, 0, pszLnkFile, -1, wszLnkFile, MAX_PATH);
	hr=pPf->Save(pszLnkFile, TRUE);
	
	// ��ü ����
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

//���� ���������� �������� ���� ���ڿ��� ���´�.
//rc->Version->VS_VERSION_INFO �׸�
//ex. m_sVersion = GetFileVersionInformation(sExeFile, "FileVersion");
//�Ʒ��� GetFileProperty �Լ��� ������ ����� �����ϴ� �ڵ��̳� �� �׸񺰷� Ȯ�� �ʿ�!
CString get_file_property(CString fullpath, CString strFlag)
{
	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	if (fullpath.IsEmpty())
		fullpath = get_exe_filename(true);

	DWORD dwSize = GetFileVersionInfoSize(fullpath, 0);
	TCHAR * buffer = new TCHAR[dwSize];
	memset(buffer, 0, dwSize);

	GetFileVersionInfo(fullpath, 0, dwSize, buffer);

	UINT cbTranslate;
	UINT dwBytes;

	LPBYTE lpBuffer = NULL;

	VerQueryValue(buffer, _T("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate);

	if(cbTranslate != 0) // ���� ������ ���� ��� "
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

//��õ� FileVersion �Ǵ� ProductVersion�� ���´�.
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

//�ش� ������ ���� ���ϸ� ��ü �Ǵ� �Ϻκ��� ���ο� �̸����� �����Ѵ�.
//��, Ȯ���ڴ� ���Ե��� �ʴ´�.
//���� ȣ���� �� �ƿ� oldName�� newName���� Ȯ���ڴ� �־�� �ȵȴ�.
//����!
//bWholename�� false��� �ص� "*%s*"�� ���� �տ��� *�� ���̸�
//��ġ �ʴ� ����� �߻��� �� ������ �ݵ�� ��������� Ȯ���� �� ����� �ʿ䰡 �ִ�.
//rename�� ������ ������ ������ �����Ѵ�.
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

	FindAllFiles(folder, &files, names, _T("*"), bRecursive, _T(""), false);

	for (int i = 0; i < files.size(); i++)
	{
		//�˻��� ���ϸ��� oldName ������ .�� _Snapshot �� �ΰ��� ������ ���� ���ϱ����� ó���Ѵ�.
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
				get_last_error_string(GetLastError(), true);
		}
	}

	return success;
}

//mp4 ������ Ư�� �±� ������ �� ���ϴ� ��ġ�� �����͸� �����Ѵ�.
//MOBIS ������Ʈ ���� MP4�� mdat �ʵ��� 0x40�������� n bytes��
//�������� ������ ����ð��� ����Ǿ� �ִ�.
//n�� 0x3C���� 4����Ʈ�� �� ũ���̴�.
char* GetDataFromMP4File(char* sfile, char* sTag, uint8_t tagStart, int tagLength)
{
	FILE	*fp = fopen(sfile, "rb");
	if (fp == NULL)
		return NULL;

	int		size;
	char	type[6][4];
	char	*pData = NULL;

	//4~7���� 4����Ʈ�� "ftyp"�̾�� mp4 �����̴�.
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

			//�������� ũ��� 28����Ʈ�̾�� �Ѵ�.
			//�ٸ� ���̶�� timeStamp�� �ƴϰų� ���� �������̴�.
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
	// Change from unicode to mult byte
	WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
	SysFreeString(bstrWide);

	CString strResult = (CString)pszAnsi;
	delete[] pszAnsi;

	return strResult;
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
	// str�� Unicode���� Ansi ���� ���� �ʿ���� T2CW�� ��ȯ    
	const WCHAR* wStr = T2CW(szText);    
	
	// ���̴� -1�� �־� ��NULL ���ڵ� ��ȯ�ǵ���
	//WCHAR -> UTF-8
	
	int nUTF8codeSize = WideCharToMultiByte(CP_UTF8, 0, wStr, -1, NULL, 0, NULL, NULL); //wStr�� ũ�⸦ ����
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
void SetWallPaper(CString sfile)
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

//�� ���� ������ ���Ѵ�.
//���ϴ� ������� 0~360���ε�
//atan�� ��ȣ�� ���� ������ ������ �� ����
//atan2�� -PI ~ +PI�� �����ϹǷ� 0~360 ������ ���� �����Ƿ� �Ʒ��� ���� ���ؾ� �Ѵ�.
//ScreenCoord�϶��� Cartesian coordinate(������ǥ��)�϶��� y�� �ݴ��ӿ� ����.
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

//3���� �̷�� ����
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

//�� ���� ������ �������� x3�� ���Ѵ�.
double getLinePointX(double x1, double y1, double x2, double y2, double y3)
{
	if (x1 == x2)
		return x1;

	double a = (double)(y2 - y1) / (double)(x2 - x1);
	double b = y2 - a * x2;
	return ((y3 - b) / a);
}

//�� ���� ������ �������� y3�� ���Ѵ�.
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

//ptCenter�� �������� dAngle ��ŭ ȸ���� dDist�Ÿ��� ���� ��ǥ�� ���Ѵ�.
CPoint GetRotatedPoint(CPoint ptCenter, double dAngle, double dDist)
{
	CPoint	pt;

	pt.x = cos(RADIAN(dAngle)) * dDist;
	pt.y = -sin(RADIAN(dAngle)) * dDist;
	pt.Offset(ptCenter);

	return pt;
}

//cx, cy�� �߽����� tx, ty���� degree�� ȸ���� ��� tx, ty���� ���� ��ǥ
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

//���ڿ����� n��° ��ū�� �����Ѵ�.
CString GetToken(CString src, CString separator, int index)
{
	int i = 0;
	CStringArray saItems;

	for (CString sItem = src.Tokenize(separator,i); i >= 0; sItem = src.Tokenize(separator,i))
	{
		saItems.Add(sItem);
	}

	if (index < 0 || index >= saItems.GetSize())
		return _T("");

	return saItems.GetAt(index);
}

//Tokenize�� �̿��ϸ� ������ ��ū�� ó������ �ʰ� ���õǹǷ� ���ϴ� ������ŭ ������� �ʴ´�.
//���� �Ʒ� �Լ��� ������� �ʴ´�.
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
int get_token_string(CString src, std::deque<CString> &dqToken, CString separator, bool allowEmpty, int nMaxToken /*= -1*/)
{
	int i, j;
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
			//�� �̻� separator�� ���ٸ� �� ������ token�̴�.
			token = src;
		}

		if (!token.IsEmpty() || allowEmpty)
			dqToken.push_back(token);

		if (nMaxToken > 0 && dqToken.size() == nMaxToken)
			return dqToken.size();

		if (pos < 0)
			break;
	}

	return dqToken.size();
}


//nMaxToken ���� ��ŭ ��ū�� �и��ؼ� sToken�� �ְ�
//���� ó���� ��ū ������ �����Ѵ�.
//src�� ��ū�� �и��� ������ �پ���.
//�̴� CString::Tokenize() �Ǵ� AfxExtractSubString()���� �ٸ���.
//(seps = " ,\t\n" �� ���� �и���ȣ��� �̷���� ��Ʈ�� ������)
//strtok_s �Լ��� ���� �����̳� ���� ��ǥ �� �ߺ��� �и��ڴ� ��� �ϳ��� �����ڷ� ��޵ȴ�.
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

//dq�׸��� �ϳ��� ���ڿ��� �����ش�.
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
		//���� ��ȣ�� �ִµ� �� ��ȣ�� ���ٸ� ������ �ƴϴ�.
		//������ �����͸� �������, ������� ������ �������� ����ϴ� ������ �Ǵ��Ѵ�.
		if (end < 0)
			return false;

		sub = src.Mid(start + 1, end - start - 1);
		token->push_back(sub);
		start = src.Find(bracket, end + 1);

		//�� �̻� ���� ��ȣ�� ���ٸ� ����.
		if (start < 0)
			break;
	}

	return true;
}

//update livesupport_report set endtime = '2023-01-01 22:01:29', env_type = '5', viewer_type = '0' where accesscode = '31108355' and sptnum = '50400'
//���� ���� ���Ŀ��� �ʵ��� ���� �����Ѵ�.
//fields���� ������ �ʵ���� �Ѱ��ش�. "endtime", "env_type"...
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
//���� ���� sql���� field�� value�� �����Ѵ�.
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

	//src�� sql_cmd�� ���۵Ǵ� �����̸� true
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
		//table_name �������� ���� �Ǵ� '('�� ���Ƿ� �� ��ġ�� ã�´�.
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


//src ���ڿ��� set_of_keyword�� ������ �ܾ �ִ��� �˻�.
//set_of_keyword�� �����ݷ����� �����ؼ� ���� ���� �Ǵ� ���ڿ��� ���� �� �ִ�.
//ex. src = "abcd1234"�� �� set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
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

//��ο� '\\' �Ǵ� '/'�� ȥ��Ǿ� ���Ǵ� ��찡 �����Ƿ� �̸� �����ؾ� �Ѵ�.
//c:\\folder1\\folder2\\	=> c:\\folder1
//c:\\folder1\\folder2		=> c:\\folder1
//c:\\folder1\\				=> c:\\
//c:\\folder1				=> c:\\

CString	GetParentDirectory(CString sFolder)
{
	//"/"�� "\\"�� ȥ��� ��� ���Ͻ�Ų��.
	sFolder.Replace(_T("/"), _T("\\"));

	//sFolder ���� "\\"�� �پ��ٸ� �������ش�.
	if (sFolder.Right(1) == _T("\\"))
		sFolder = sFolder.Left(sFolder.GetLength() - 1);

	//���� ������ ������ "c:"�� ���� ����̺� ��Ʈ��� ���� "\\"�� �ٽ� �ٿ�����Ѵ�.
	if (sFolder.GetLength() == 2 && sFolder.GetAt(1) == ':')
	{
		sFolder += "\\";
		return sFolder;
	}

	//�ش� ������ parent �������� ���Ѵ�.
	sFolder = sFolder.Left(sFolder.ReverseFind('\\'));

	//�ٽ� ������ ������ "c:"�� ���� ����̺� ��Ʈ��� ���� "\\"�� �ٽ� �ٿ�����Ѵ�.
	if (sFolder.GetLength() == 2 && sFolder.GetAt(1) == ':')
	{
		sFolder += "\\";
		return sFolder;
	}

	return sFolder;
}

//MAX_COMPUTERNAME_LENGTH(15) ���̱����� ���ϵʿ� ����.
//GetComputerName API �Լ��� �׻� �빮�ڷ� �����Ѵ�.
//�׳� Ȯ���ϰ� GetComputerNameString().MakeLower() ��� ����
//��ҹ��ڸ� ��Ȯ�� �Ͽ� ���ϴ� ���� ����.
//=>255���ڱ��� ���ϵǴ� GetComputerNameEx()�� ������.
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

DWORD get_windows_major_version()
{
	OSVERSIONINFOEX osInfo;
	NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEX);

	*(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

	if (NULL != RtlGetVersion)
	{
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);
		RtlGetVersion(&osInfo);
	}

	return osInfo.dwMajorVersion;

}

CString	get_windows_version_string(OSVERSIONINFOEX* posInfo)
{
	OSVERSIONINFOEX osInfo;
	if (posInfo == NULL)
		osInfo = get_windows_version();
	else
		osInfo = *posInfo;

	return get_windows_version_string(osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber);
}

//ex. version = "10.0.12345"
CString	get_windows_version_string(CString version)
{
	std::deque<CString> dq;
	get_token_string(version, dq, '.');

	DWORD dwMajor = 0;
	DWORD dwMinor = 0;
	DWORD dwBuild = 0;
	
	if (dq.size() >= 2)
	{
		dwMajor = _ttoi(dq[0]);
		dwMinor = _ttoi(dq[1]);
	}

	if (dq.size() == 3)
	{
		dwBuild = _ttoi(dq[2]);
	}

	return get_windows_version_string(dwMajor, dwMinor, dwBuild);
}

CString	get_windows_version_string(DWORD dwMajor, DWORD dwMinor, DWORD dwBuild)
{
	if (dwMajor == 5 && dwMinor == 1)
		return _T("Windows XP");
	else if (dwMajor == 6 && dwMinor == 0)
		return _T("Windows Vista");
	else if (dwMajor == 6 && dwMinor == 1)
		return _T("Windows 7");
	else if (dwMajor == 6 && dwMinor == 2)
		return _T("Windows 8");
	else if (dwMajor == 6 && dwMinor == 3)
		return _T("Windows 8.1");
	else if (dwMajor == 10 && dwMinor == 0)
	{
		if (dwBuild >= 22000)
			return _T("Windows 11");
		else
			return _T("Windows 10");
	}

	return _T("Unknown OS version");
}

CString	get_system_label(int csidl, int* sysIconIndex)
{
	LPMALLOC pMalloc;
	SHGetMalloc(&pMalloc);
	SHFILEINFO sfi = { 0 };
	// 'My Computer' or '�� ��ǻ��' text
	LPITEMIDLIST pidl;
	SHGetSpecialFolderLocation(NULL, csidl, &pidl);
	SHGetFileInfo((TCHAR*)pidl, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	pMalloc->Free(pidl);
	pMalloc->Release();

	if (sysIconIndex != NULL)
	{
		*sysIconIndex = sfi.iIcon;
	}

	return sfi.szDisplayName;
}

//�� ���� �����찡 ��ġ�� �� �����ǰ� �缳ġ���� ������ �����ȴ�.
//��, HDD �����ÿ��� �����ǹǷ� �ӽſ� ���� unique�ϴٰ� �� �� ����.
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

	// CoInitializeEx() �Լ��� ȣ���Ͽ� COM ���̺귯���� �ʱ�ȭ�մϴ�.
	if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		IAutomaticUpdates* pAutomaticUpdates = NULL;
		// Automatic Updates ��ü ����
		HRESULT hr = CoCreateInstance(CLSID_AutomaticUpdates, NULL, CLSCTX_INPROC_SERVER, IID_IAutomaticUpdates, (LPVOID*)&pAutomaticUpdates);

		if (SUCCEEDED(hr))
		{
			VARIANT_BOOL isAutoUpdateEnabled = VARIANT_FALSE;
			// �ڵ� ������Ʈ Ȱ��ȭ ���� Ȯ��
			hr = pAutomaticUpdates->get_ServiceEnabled(&isAutoUpdateEnabled);
			if (SUCCEEDED(hr))
			{
				if (isAutoUpdateEnabled == VARIANT_TRUE)
				{
					//AfxMessageBox(_T("�ڵ� ������Ʈ�� Ȱ��ȭ�Ǿ����ϴ�."));
					auto_update = true;
				}
				else
				{
					//AfxMessageBox(_T("�ڵ� ������Ʈ�� ��Ȱ��ȭ�Ǿ����ϴ�."));
					auto_update = true;
				}
			}
			else
			{
				//AfxMessageBox(_T("�ڵ� ������Ʈ ���¸� �������� �� �����߽��ϴ�."));
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

	// COM ���̺귯�� ����
	CoUninitialize();

	return true;
}

//SystemParametersInfo(SPI_GETSCREENSAVEACTIVE...)���δ� ����� �������� ������ ���Ѵ�.
//SCRNSAVE.EXE��� �׸��� �����ϸ� ������ ���̰� ������ ������ ���̴�.
//�����ð��� ���ȭ���� ǥ�������� ���� �������� ���� �� �ִ�.
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

//HDD serial�� HardwareInfo.exe�� �˷��ִ´�� S4EVNM0T230338R 15�ڸ� ����,
//�Ǵ� 0025_3852_2190_FE03 ���� ���·� ���ϵȴ�.
//� PC�� ���� �˷��ִ� ���� �� ���α׷����� ���� ���� ������(mwj, sdh)
//� PC�� ���� �˷��ִ� ���� �ٸ� �������� �����Ѵ�.(scpark)
//�� �̷� ���̰� �߻��ϴ����� ���� �� �� ������
//unique�� Ű���� ����ϰ��� ���̹Ƿ� �켱 16�ڸ��� ���缭 ����Ѵ�.
//unify16 = true�̸� 16�ڸ��� ���Ͻ��� �����Ѵ�.

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

//���ڷ� ������ ���ڿ��� �Է¹޾� ������ ������ �� �� �ڼ��� ���ڹ��ڿ��� �����Ѵ�.
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

	if (sLabel.IsEmpty())
	{
		sLabel.Format(_T("���� ��ũ (%c:)"), drive_letter);
	}
	else
	{
		sLabel.Format(_T("%s (%c:)"), sLabel, drive_letter);
	}

	if (GetDriveType(drive_root.Left(2)) == DRIVE_REMOTE)
	{
		unsigned long bufferLength = MAX_PATH;
		WNetGetConnection(drive_root.Left(2), Label, &bufferLength);
		//��Ʈ��ũ ����̺��� Label�� "\\\\192.168.1.103\\�����ҹ���" �� ���� ��������.
		//���� Ž���⿡���� "�����ҹ���(\\192.168.1.103) (Y:)" �� ���� ǥ���ϰ� �ִ�. �������ش�.
		sLabel = Label;
		sLabel.Format(_T("%s(%s) (%c:)"),
			sLabel.Mid(sLabel.ReverseFind('\\') + 1),
			sLabel.Left(sLabel.ReverseFind('\\')),
			drive_letter);
	}

	return sLabel;
}

void get_drive_map(std::map<TCHAR, CString> *drive_map, bool include_legacy)
{
	DWORD dwError = 0;
	TCHAR tzDriveString[MAX_PATH] = { 0, };
	CString strDrive;

	drive_map->clear();

	DWORD logicalDrives = GetLogicalDrives();
	unsigned int i = 64;

	do
	{
		i++;
		if ((logicalDrives & 1) != 0)
		{
			strDrive.Format(_T("%c:\\"), i);
			UINT driveType = GetDriveType(strDrive);

			// CD-ROM�̳� floppy disk�� ���� ������ ǥ������ �ʴ´�.
			if (!include_legacy && (driveType == DRIVE_REMOVABLE || driveType == DRIVE_CDROM))
				continue;

			drive_map->insert(std::pair<TCHAR, CString>(strDrive[0], get_drive_volume(strDrive[0])));
		}
	} while ((logicalDrives >>= 1) != 0);
}

//"���� ��ũ (C:)" <-> "C:\\" //���� ���� ���� ������ ������� ��ȯ
CString	convert_special_folder_to_real_path(CString special_folder, std::map<int, CString>* csidl_map)
{
	TCHAR buf[MAX_PATH];
	int csidl = -1;
	std::map<int, CString> csidl_map_temp;

	//"���� ȭ��\\"�� ���� �Ѿ���� ��� �߶󳽴�.
	if (special_folder.GetLength() > 2 && special_folder[1] != ':' && special_folder.Right(1) == _T("\\"))
		special_folder = special_folder.Left(special_folder.GetLength() - 1);

	CString real_path = special_folder;

	if (!csidl_map)
	{
		csidl_map = &csidl_map_temp;
		SHGetSpecialFolderPath(NULL, buf, CSIDL_DESKTOP, FALSE);
		csidl_map->insert(std::pair<int, CString>(CSIDL_DESKTOP, buf));
		SHGetSpecialFolderPath(NULL, buf, CSIDL_PERSONAL, FALSE);
		csidl_map->insert(std::pair<int, CString>(CSIDL_PERSONAL, buf));
		SHGetSpecialFolderPath(NULL, buf, CSIDL_DRIVES, FALSE);
		csidl_map->insert(std::pair<int, CString>(CSIDL_DRIVES, buf));
	}

	for (auto it = csidl_map->begin(); it != csidl_map->end(); ++it)
	{
		if (it->second == special_folder)
		{
			csidl = it->first;
			break;
		}
	}

	if (csidl == CSIDL_DESKTOP)
	{
#ifndef _USING_V110_SDK71_
		real_path = get_known_folder(FOLDERID_Desktop);
#else
		SHGetSpecialFolderPath(NULL, buf, csidl, FALSE);
		real_path = buf;
#endif
	}
	else if (csidl == CSIDL_PERSONAL)
	{
#ifndef _USING_V110_SDK71_
		real_path = get_known_folder(FOLDERID_Documents);
#else
		SHGetSpecialFolderPath(NULL, buf, csidl, FALSE);
		real_path = buf;
#endif
	}
	//�� PC�� ���� real path�� �������� �ʴ´�.
	else if (csidl == CSIDL_DRIVES)
	{
		;
	}
	/*
	else if (csidl == _T("�ٿ�ε�"))
	{
#ifndef _USING_V110_SDK71_
		real_path = get_known_folder(FOLDERID_Downloads);
#else
		SHGetSpecialFolderPath(NULL, buf, CSIDL_DOWNLOADS, FALSE);
		real_path = buf;
#endif
	}
	*/

	if (real_path == get_system_label(CSIDL_DRIVES))
		return real_path;

	real_path.Replace(get_system_label(CSIDL_DRIVES), _T(""));

	int pos = real_path.Find(_T(":)"));
	if (pos < 0)
		return real_path;

	CString rest = real_path.Mid(pos + 2);
	CString drive_letter = real_path.Mid(pos - 1, 1);

	if (rest.Left(1) == _T("\\"))
		rest = rest.Mid(1);

	real_path.Format(_T("%s:\\%s"), drive_letter, rest);
	return real_path;
}

//"c:\\abc\\def" => "���� ��ũ (C:)\\abc\\def"
CString	convert_real_path_to_special_folder(CString real_path, std::map<int, CString>* csidl_map)
{
	CString volume_path = real_path;

	if (real_path.Mid(1, 2) != _T(":\\"))
		return real_path;

	std::map<TCHAR, CString> drive_map;
	get_drive_map(&drive_map);

	CString volume = drive_map[toupper(real_path[0])];

	volume_path.Replace(CString(real_path[0]) + _T(":"), volume);

	return volume_path;
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

//char chStr[100] = { 0, };�� ���� pointer ������ �ƴ� �迭�� ����� �����
//chStr = CString2char(str); ������ ������ �߻��ϹǷ� �Ʒ��� ���� ����� ��.
//sprintf(chStr, "%s", (LPSTR)(LPCTSTR)str);	//MBCS : ok, UNICODE : fail
//sprintf(chStr, "%s", CStringA(str));		//both : ok
//���Ϲ޾� ����� char* �������� ��� �� �ݵ�� delete [] ���ٰ�
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
// CString �� TCHAR
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
	//unicode ȯ�濡�� Casting�� ����������
	//���� LPCSTR ���ڷ� �Ѱܼ� ��� �ϴ� ���� �߻�.
	CStringA strA(str);
	LPCSTR lpcstr = strA;
	return lpcstr;
#else
	//Multibyte ȯ�濡���� ĳ�������� ���� ���� ��� ����
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
// CString �� std::string
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

//cstr�� ��ȿ�� ���̸� �̹� �˰� �ִٸ� length�� ��������� ��Ȯ�ϴ�.
//�׷��� ���� ��� cstr�� ���� '\0'�� ���� ��� ������ ���ڵ���� ���Ե� �� �ִ�.
//cstr�� '\0'�� �����ٸ� �����ڵ�, ��Ƽ����Ʈ ȯ�濡�� CString str = cstr;�� ���� ó���ȴ�.
CString char2CString(char* cstr, int length)
{
	CString str;

#if defined(UNICODE) || defined(_UNICODE)
	//length�� -1�� ���, �� ���̸� �𸣰ų� Ư���� �������� �ʰ�
	//cstr�� '\0'���ڷ� ������ ������ ���̶��
	//unicode���� str = CString(cstr); �ε� ���� �����Ѵ�. ���ܰ� �ִ��� Ȯ�� �ʿ�!
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
// Char �� TCHAR
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
// TCHAR �� CString
//
CString TCHAR2CString(TCHAR* str)
{
	CString cStr;
	cStr.Format(_T("%s"), str);
	return cStr;
}

//
// TCHAR �� Char
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

int	GetNumberFromFile(TCHAR* sfile)		//������ �о �Ѱ��� ���ڰ��� �����Ѵ�. (������ ���ų� ������ ������ -99999 �� �����Ѵ�.)
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

bool WriteNumberToFile(TCHAR* sfile, int n)	//���ڰ��� ���Ͽ� ������ش�.
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

double GetCpuUsage(const char* process)
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
	// Process ��ü(CPU_INDEX)���� �ش� process �̸��� ���� ���μ����� CPU ���� ��´�.
	if(GetCounterValue(CPU_INDEX, CPU_COUNTER, process, &cpuData, newVal) < 0)
		return -1;
	// ȹ���� ���� ������ �̿��ؼ� CPU ������� ���
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

//���� ���� �޸𸮸� �����Ѵ�. (total_memory : ��ü �޸� �뷮)
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

double GetElapsedTime(__timeb32 pOldTime)	//pOldTime�� ���� �ð��� ���� ���
{
	__timeb32	tCur;

	_ftime32(&tCur);

	time_t t0 = pOldTime.time * 1000 + pOldTime.millitm;
	time_t t1 = tCur.time * 1000 + tCur.millitm;

	return (double)(t1 - t0) / 1000.0 ;
}

CString	GetDayTimeCountString(int format, CTimeSpan ts, bool bShowZero, bool bIncludeSec)	//ts���� �Ѱ� �޾� "a�� b�ð� c�� d��" ���·� ǥ��
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
		str.Format(_T("%d��"), ts.GetDays());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetHours() || bShowZero)
	{
		str.Format(_T("%d�ð�"), ts.GetHours());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetMinutes() || bShowZero)
	{
		str.Format(_T("%d��"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	if (bIncludeSec && (sResult != "" || ts.GetMinutes() || bShowZero))
	{
		str.Format(_T("%d��"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	sResult.TrimLeft();

	if (sResult == "")
	{
		if (bIncludeSec)
			sResult = "0��";
		else
			sResult = "0��";
	}

	return sResult;
}

//ts���� �Ѱ� �޾� "a�� b�ð� c�� d��" ���·� ǥ��
CString	GetDayTimeCountString(COleDateTimeSpan ts, bool bShowZero, bool bIncludeSec)
{
	CString str;
	CString sResult = _T("");

	if (ts.GetDays() || bShowZero)
	{
		str.Format(_T("%d��"), ts.GetDays());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetHours() || bShowZero)
	{
		str.Format(_T("%d�ð�"), ts.GetHours());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetMinutes() || bShowZero)
	{
		str.Format(_T("%d��"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	if (bIncludeSec && (sResult != "" || ts.GetMinutes() || bShowZero))
	{
		str.Format(_T("%d��"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	sResult.TrimLeft();

	if (sResult == "")
	{
		if (bIncludeSec)
			sResult = "0��";
		else
			sResult = "0��";
	}

	return sResult;
}

bool IsAM(CTime t /*= 0*/)
{
	//t���� 0�� ���� ���� �ð��� ���ͼ� ����, ���ĸ� �Ǵ����ش�.
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
			case 1	:	sWeek = "�Ͽ���";
						break;
			case 2	:	sWeek = "������";
						break;
			case 3	:	sWeek = "ȭ����";
						break;
			case 4	:	sWeek = "������";
						break;
			case 5	:	sWeek = "�����";
						break;
			case 6	:	sWeek = "�ݿ���";
						break;
			case 7	:	sWeek = "�����";
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

//�ش� ���� ��¥�� ����
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
/*
bool GetMemory
#ifdef _WIN64
MEMORYSTATUSEX memStatus;
memStatus.dwLength = sizeof(MEMORYSTATUSEX);

if(!GlobalMemoryStatusEx(&memStatus))
{
	m_static_availableMemory.SetWindowText("N/A");
	m_static_totalMemory.SetWindowText("N/A");
	m_static_memoryLoad.SetWindowText("N/A");
}
else
{
	counterString.Format("%5.2f MB", (float)(memStatus.ullAvailPhys / (float)ONE_MEG));
	m_static_availableMemory.SetWindowText(counterString);

	counterString.Format("%5.2f MB", (float)(memStatus.ullTotalPhys / (float)ONE_MEG));
	m_static_totalMemory.SetWindowText(counterString);

	counterString.Format("%d%%", memStatus.dwMemoryLoad);
	m_static_memoryLoad.SetWindowText(counterString);
}
#else

MEMORYSTATUSEX memStatus;
memStatus.dwLength = sizeof(MEMORYSTATUSEX);

HANDLE procHandle = GetCurrentProcess();
PROCESS_MEMORY_COUNTERS pmc;

if(!GlobalMemoryStatusEx(&memStatus) || !GetProcessMemoryInfo(procHandle, &pmc, sizeof(pmc)))
{
	m_static_availableMemory.SetWindowText("N/A");
	m_static_totalMemory.SetWindowText("N/A");
	m_static_memoryLoad.SetWindowText("N/A");
}
else
{
	DWORDLONG availMemory_32bit = 0;
	DWORDLONG totalMemory_32bit = 0;

	if (memStatus.ullAvailPhys > (TWO_GIG - pmc.WorkingSetSize))
	{
		totalMemory_32bit = TWO_GIG;
		availMemory_32bit = (TWO_GIG - pmc.WorkingSetSize);
	}
	else
	{
		totalMemory_32bit = (pmc.WorkingSetSize + memStatus.ullAvailPhys);
		availMemory_32bit = memStatus.ullAvailPhys;
	}

	counterString.Format("%5.2f MB", (float)(availMemory_32bit)/(float)ONE_MEG);
	m_static_availableMemory.SetWindowText(counterString);

	counterString.Format("%5.2f MB", (float)(totalMemory_32bit) / (float)ONE_MEG);
	m_static_totalMemory.SetWindowText(counterString);

	counterString.Format("%3.1f%%",  ((float)(totalMemory_32bit - availMemory_32bit)/(float)totalMemory_32bit) * 100.0);
	m_static_memoryLoad.SetWindowText(counterString);
}
#endif
}
*/



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
	//���⿡�� srand�� ȣ���ϸ� ���ϴ´�� �������� �ʴ´�. ���ο��� ���۽� �ѹ��� ȣ������.
	//srand(time(NULL));
	if (minimum > maximum)
		Swap(minimum, maximum);
	return minimum + rand() % (maximum - minimum + 1);
}

double random(double minimum, double maximum)
{
	//���⿡�� srand�� ȣ���ϸ� ���ϴ´�� �������� �ʴ´�. ���ο��� ���۽� �ѹ��� ȣ������.
	//srand(time(NULL));
	if (minimum > maximum)
		Swap(minimum, maximum);
	return minimum + (maximum - minimum) * rand() / (RAND_MAX + 1.0);
}
*/
//���� ��ǥ <-> ������ ��ȯ
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

//src���� ��� ���ڿ� ���� digits�ڸ����� ���� ����
void combination(std::vector<TCHAR> src, CString temp, std::vector<CString> &result, int depth)
{
	if (depth == 0)//result.size())  // depth == n // ��� �Ȼ̴ٰ� r ���� ä���� ���� ���� �� ���� �ɷ��� �Ѵ�.
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

//�밢 ������ ����, ���� ȭ���� ���������� ����Ѵ�.
//���� ���� ����� �ݵ�� ��ġ���� �ʴ´�.
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

//<���μ��� �̸����� ���μ��� ID ���>
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

	do {
		if (StrCmp(pe32.szExeFile, processname) == 0)

			return pe32.th32ProcessID;



	} while (Process32Next(hProcessSnap, &pe32));


	CloseHandle(hProcessSnap);

	return false;
}

// ������ �ڵ�� ���μ��� ���̵� ���   
ULONG ProcIDFromWnd(HWND hwnd)
{   
	ULONG idProc;   
	::GetWindowThreadProcessId(hwnd, &idProc);   
	return idProc;   
}

// ���μ��� ���̵�� ������ �ڵ� ���   
HWND GetHWNDbyPID(ULONG pid)
{   
	HWND tempHwnd = ::FindWindow(NULL,NULL); // �ֻ��� ������ �ڵ� ã��   

	while(tempHwnd != NULL)   
	{   
		//TRACE("tempHwnd = %p\n", tempHwnd);
		if(::GetParent(tempHwnd) == NULL) // �ֻ��� �ڵ����� üũ, ��ư � �ڵ��� ���� �� �����Ƿ� �����ϱ� ����   
			if(pid == ProcIDFromWnd(tempHwnd))   
				return tempHwnd;   
		tempHwnd = ::GetWindow(tempHwnd, GW_HWNDNEXT); // ���� ������ �ڵ� ã��   
	}   
	return NULL;
}

//��ó: https://smok95.tistory.com/300?category=28201 [Only YOUng:Ƽ���丮]
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
	DWORD sessionIDTemp;

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
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
		if (hProcess != NULL)
		{
			if (_tcscmp(pe32.szExeFile, szFilename) == 0)
			{
				if (ProcessIdToSessionId(pe32.th32ProcessID, &sessionIDTemp) && sessionIDTemp == sessionID)
				{
					CloseHandle(hProcessSnapshot);
					return hProcess;
				}
			}

			CloseHandle(hProcess);
		}
	} while (Process32Next(hProcessSnapshot, &pe32));

	CloseHandle(hProcessSnapshot);
	return INVALID_HANDLE_VALUE;
}

//default : bCaseSensitive = false, bExceptThis = true
//���� ������ �ִ�. �ٽ� �׽�Ʈ�ؾ� ��.
HWND GetHWndByExeFilename(CString sExeFile, bool bWholeWordsOnly, bool bCaseSensitive, bool bExceptThis)
{
	bool			bFlag = false;
	HANDLE			hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL); 
	PROCESSENTRY32	pe;
	HWND			hWnd = NULL;

	if (!hSnapShot)
	{
		::MessageBoxA(NULL, "CreateToolhelp32Snapshot() : Fail!", NULL,NULL);
		return NULL;
	}

	hWndToFind = NULL;

	pe.dwSize = sizeof(PROCESSENTRY32);
	CString szExeFile;
	bool found = false;

	if (Process32First(hSnapShot, &pe)) 
	{
		do 
		{
			found = false;
			szExeFile = pe.szExeFile;

			//TRACE(_T("pe.szExeFile = %s\n"), pe.szExeFile);
			if (!bCaseSensitive)
			{
				szExeFile.MakeLower();
				sExeFile.MakeLower();
			}

			if (bWholeWordsOnly)
			{
				if (szExeFile == sExeFile)
					found = true;
			}
			else
			{
				if (szExeFile.Find(sExeFile) >= 0)
					found = true;
			}

			if (found)
			{
				//�ڱ� �ڽ��� �����ϰ�...
				if (pe.th32ProcessID != GetCurrentProcessId())
				{
					HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
					if (hProcess != NULL)
					{
						hWnd = GetHWNDbyPID(pe.th32ProcessID);
						CloseHandle(hProcess);
						//hWnd = GetWindowHandleFromProcessID(pe.th32ProcessID);
						break;
					}
					
					//if (::EnumWindows(&EnumWindowsProc, (LPARAM)pe.th32ProcessID) == FALSE)
					//{
					//	hWnd = hWndToFind;
					//	break;
					//}
				}
				//SetPriorityClass(hProcess, THREAD_PRIORITY_HIGHEST);
			}
		}
		while (Process32Next(hSnapShot, &pe)); 
	}

	CloseHandle(hSnapShot);

	return hWnd;
}

//�ش� ������ �������� ī��Ʈ�� �����ϴ� �Լ��̸�
//fullpath�� �ָ� ��α��� �����ؾ� ī��Ʈ�ǵ��� ����� ����������
//���ѹ������� ���� PC������ �� ��������
//�ٸ� PC������ �ƿ� ������������ �ʴ´�.
//�켱 �����Ѵ�.
#if 0
int get_process_running_count(CString processname)
{
	if (processname.IsEmpty())
		return 0;

	HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pe32 = { 0, };
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnapshot, &pe32))
	{
		OutputDebugString(_T("Error checking process"));
		CloseHandle(hProcessSnapshot);
		return 0;
	}

	int running_count = 0;

	do
	{
		//TRACE(_T("process = %s\n"), pe32.szExeFile);
		//if (_tcsicmp(pe32.szExeFile, _T("natsvc.exe")) == 0)
		//{
		//	TRACE(_T("process = %s\n"), pe32.szExeFile);;
		//}

		if (pe32.th32ProcessID != 0)
		{
			TCHAR sFilePath[1024] = { 0, };
			DWORD bufLen = 1024;

			_tcscpy(sFilePath, pe32.szExeFile);

			//processname�� �������ϸ� �ִٸ� exe ���ϸ� ���ϰ�
			//��ü ��ζ�� fullpath�� ���ؼ� ���Ѵ�.
			//�� hProcess�� NULL�̶� ��ü��θ� ������ ���ϴ� ���μ����� �ִ�.
			if (PathFileExists(processname))
			{
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
				//HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

				if (hProcess)
				{
					QueryFullProcessImageName(hProcess, NULL, sFilePath, &bufLen);
					//::GetModuleFileNameEx(hProcess, NULL, sFilePath, MAX_PATH);
					CloseHandle(hProcess);
				}
				else
				{
					TRACE(_T("fail to get OpenProcess(). %s\n"), get_last_error_message(false));
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
	// �������� ���μ������� ù���� ������ �����´�.
	Process32First(hSnapShot, &pEntry);

	int procCount = 0;
	// Process �� ���������� Ȯ��
	while (1)
	{
		// ������ ���μ����� ������ �����´�.
		BOOL hRes = Process32Next(hSnapShot, &pEntry);

		if (hRes == FALSE)
			break;

		//������ ���� ��� Ư�� ���μ������� OpenProcess()�� NULL�� �����Ѵ�.
		//������ �ش� ���μ��� ī��Ʈ�� ���´ٸ� OpenProcess()�� ȣ������ ����.
		//������ �ְ� ���;� �Ѵٸ� AdjustTokenPrivileges ���� �̿��ؾ� �Ѵ�.
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

//���μ��� ���� ����.
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

CString run_process(CString exePath, bool wait_process_exit, bool return_after_first_read)
{
	CString result(_T(""));
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

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
		return result;
	}

	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.hStdOutput = hChildStdoutWr;
	si.hStdError = hChildStdoutWr;
	si.wShowWindow = SW_HIDE;


	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
		(TCHAR*)(const TCHAR*)(exePath),        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		TRUE,          // Set handle inheritance to FALSE
		CREATE_NEW_CONSOLE,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
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
		return result;

	for (;;)
	{
		DWORD dwRead;
		char chBuf[4096] = { 0, };

		bool done = !ReadFile(hChildStdoutRd, chBuf, 4096, &dwRead, NULL) || dwRead == 0;

		if (done)
			break;

		result += chBuf;
		if (return_after_first_read)
			break;
	}

	// Close process and thread handles. 
	CloseHandle(hChildStdoutRd);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return result;
}

#include <array>
std::string	run_process(const char* cmd)
{
	std::array<char, 128> buffer;
	std::string result;
	std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) throw std::runtime_error("_popen() failed!");
	while (!feof(pipe.get())) {
		if (fgets(buffer.data(), 128, pipe.get()) != NULL)
			result += buffer.data();
	}
	return result;
}

//���� ���°� �����̵� ����, ���Ž�Ų��. sc queryex -> taskkill /pid -> sc delete
//process_name�� �־����� �� �� ������ ���ŵȴ�.
//���� ����(�Ǵ� ���񽺰� ���� ���) : true
//���� ���� : false
bool kill_service(CString service_name, CString process_name /*= _T("")*/)
{
	CString result;

	if (!process_name.IsEmpty())
	{
		result = run_process(_T("taskkill /f /im ") + process_name, true);
		result = run_process(_T("sc delete ") + service_name, true);
		return true;
	}

	//CString result = run_process(_T("sc queryex ") + service_name, true);
	int pid_pos = result.Find(_T("PID"));

	//�ش� ���񽺰� ���� ���
	if (pid_pos <= 0)
		return true;

	//PID ������ ���ڿ��� �����
	result = result.Mid(pid_pos + 3);
	//':' ������ ���ڿ��� �����
	result = result.Mid(result.Find(':') + 1);
	result.Trim();

	result = result.Left(result.Find('\r'));

	int pid = _ttoi(result);
	CString cmd;

	//�ش� ������ ���� ���μ����� ���� �����Ű�� �ش� ���񽺴� "������"���� ����ȴ�.
	cmd.Format(_T("taskkill /pid %d /f"), pid);
	result = run_process(cmd, true).MakeLower();
	if (result.Find(_T("����")) < 0 && result.Find(_T("success")) < 0)
		return false;

	//������ ���񽺸� �����Ѵ�. 
	result = run_process(_T("sc delete ") + service_name, true);
	if (result.Find(_T("����")) < 0 && result.Find(_T("success")) < 0)
		return false;

	return true;
}

bool RectInRect(CRect rMain, CRect rSub)
{
	CRect	rUnion;

	rUnion.UnionRect(&rMain, &rSub);

	if (rUnion == rMain)
		return true;

	return false;
}

//r���� except������ �����ϰ� cr�÷��� ä���.
void fill_except_rect(CDC* pDC, CRect r, CRect except, COLORREF cr)
{
	//ä������ ���� Ȯ�ο�
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

//side �迭�� �ε����� resize�ϴ� ���� �ε����μ�
//DefWindowProc�� �ι�° �Ķ���Ϳ� (SC_SIZE + m_nSideIndex)�� ���̹Ƿ� �� ���ʸ� ������.
//��, top�� ���, �׵θ��� ���� dialog�� ���� ����� Ÿ��Ʋ�� ����ó�� ���Ƿ�
//margin�� ũ�⸦ �״�� ������ Ÿ��Ʋ�ٰ� �������� ������ �ִ�.
//���� top�� ���� 4�ȼ��� �����ϴ� ���� �����ϴ�.
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

//8������ �ڳʿ� ���̵� ������ �ش� ���� �ִ����� �˻��Ͽ� �ش� ������ �ε����� �����Ѵ�.
//�ε����� CORNER_INDEX�� �����̸� �̴� DefWindowProc���� ����ϴ� ���ʿ� �����ϴ�.
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

//�������� �ָ� ���簢���� �̷�� ���� ��ǥ�� �����Ѵ�.
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
// PID�� ���μ��� ã��
//-----------------------------------------------------------------
bool CheckProcessUsingPID(unsigned long pid) // unsigned long = DWORD
{
	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	if (!snapshotHandle)
		return false;

	//-----------------------------------------------------------------
	// ���μ����� ã�´�
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
// ������ Ÿ��Ʋ, Ŭ���� �̸����� ���μ��� ã��
//-----------------------------------------------------------------
bool CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName)
{
	if (!className && !titleName)
		return false;

	//-----------------------------------------------------------------
	// Ŭ���� �̸� �� Ÿ��Ʋ �̸����� �����츦 ã�´�
	//-----------------------------------------------------------------
	HWND targetWindow = FindWindow(className ? className : NULL, titleName ? titleName : NULL);
	if (targetWindow)
	{
		//-----------------------------------------------------------------
		// ã�� ������ �ڵ�� PID, TID�� ����Ͽ� ���μ����� ã�´�
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
// ���μ��� �̸����� ���μ��� ã��
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
	// ���μ����� ã�´�
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
HBITMAP capture_screen_to_bitmap(LPRECT pRect, UINT id, int dx, int dy)
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
		//scpark ����� ��ġ ������ ���� ������ �� �ִ�. �Ʒ� ������ ���� �ʾƾ� �Ѵ�.
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
	hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);	//32bit�� ���������.
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
		//��üȭ���϶��� â����϶� ���ʿ��� ������ �߶󳻴� �ڵ���̳�
		//������ �߶󳻸� �ش� ũ��� mpeg��ȭ���� �ʴ� ������ �߻��Ͽ�
		//�켱 �߶󳻴� �ڵ�� ������� �ʴ´�.
		if (true)//(rct.left + 8 == DsRct.left || rct.left + 8 == DsRct.left + DsRct.right) && (rct.top + 8 == DsRct.top || rct.top + 8 == DsRct.top + DsRct.bottom) && (rct.right - 8 == DsRct.right || rct.right - 8 == DsRct.right * 2) && (rct.bottom + 32 == DsRct.bottom || rct.bottom + 32 == DsRct.bottom * 2))
		{
			// ��üȭ��

			//StretchBlt(hMemDC, -8, -8, rct.right - rct.left + 8, rct.bottom - rct.top + 8, hMemDC, 0, 0, rct.right - rct.left - 8, rct.bottom - rct.top - 8, SRCCOPY);
			StretchBlt(hMemDC, 0, 0, rct.Width(), rct.Height(), hMemDC, 0, 0, rct.Width(), rct.Height(), SRCCOPY);
			// ĸ�� �� ������ ���ֱ� ���� 8�ȼ��� ����
		}
		else
		{
			// â���

			StretchBlt(hMemDC, -7, 0, rct.right - rct.left + 7, rct.bottom - rct.top, hMemDC, 0, 0, rct.right - rct.left - 7, rct.bottom - rct.top - 7, SRCCOPY);
			// ĸ�� �� ������ ���ֱ� ���� 7�ȼ��� ����
		}
		bSuccess = TRUE;
	}

	::DeleteDC(hDC);
	::DeleteDC(hMemDC);

	//WriteBMP(hBitmap, hMemDC, _T("d:\\temp\\test_capture.bmp"));
	return hBitmap;
}

#define _S(exp) (([](HRESULT hr) { if (FAILED(hr)) /*_com_raise_error(hr);*/ return hr; })(exp));
void save_bitmap(HBITMAP bitmap, LPCTSTR filename)
{
	PICTDESC pictdesc = {};
	pictdesc.cbSizeofstruct = sizeof(pictdesc);
	pictdesc.picType = PICTYPE_BITMAP;
	pictdesc.bmp.hbitmap = bitmap;

	CComPtr<IPicture> picture;
	_S(OleCreatePictureIndirect(&pictdesc, __uuidof(IPicture), FALSE, (LPVOID*)&picture));

	// Save to a stream

	CComPtr<IStream> stream;
	_S(CreateStreamOnHGlobal(NULL, TRUE, &stream));
	LONG cbSize = 0;
	_S(picture->SaveAsFile(stream, TRUE, &cbSize));

	// Or save to a file

	CComPtr<IPictureDisp> disp;
	_S(picture->QueryInterface(&disp));
	_S(OleSavePictureFile(disp, CComBSTR(filename)));
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

	if (szFile == _T("�� PC"))//GetStringById(NFTD_IDS_COMPUTER))
	{
		LPITEMIDLIST pidl_Computer = NULL;
		SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl_Computer); // ��ǻ��
		SHGetFileInfo((TCHAR*)pidl_Computer, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if (szFile == _T("�� ����"))//GetStringById(NFTD_IDS_DOCUMENT))
	{
		LPITEMIDLIST pidl_Document = NULL;
		SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl_Document); // ������
		SHGetFileInfo((TCHAR*)pidl_Document, 0, &shFileInfo, sizeof(shFileInfo), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | SHGFI_PIDL);
	}
	else if (szFile == _T("���� ȭ��"))//GetStringById(NFTD_IDS_DESKTOP))
	{
		LPITEMIDLIST pidl_Desktop = NULL;
		SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &pidl_Desktop); // ����ȭ��
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

void gradient_rect(CDC* pDC, CRect &rect, std::deque<COLORREF> dqColor, bool vertical)
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

	//width �Ǵ� height�� n����ؼ� gradient fill�� �Ѵ�.
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
		rcVertex[0].Red		= GetRValue(dqColor[i])<<8;	// color values from 0x0000 to 0xff00 !!!!
		rcVertex[0].Green	= GetGValue(dqColor[i])<<8;
		rcVertex[0].Blue	= GetBValue(dqColor[i])<<8;
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
		rcVertex[1].Red		= GetRValue(dqColor[i+1])<<8;
		rcVertex[1].Green	= GetGValue(dqColor[i+1])<<8;
		rcVertex[1].Blue	= GetBValue(dqColor[i+1])<<8;
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
		str.Format(_T("(%d,%d)~(%d,%d)"), r.left, r.top, r.right, r.bottom);
	else if (nFormat == 2)
		str.Format(_T("(%d,%d)~(%d,%d) (%dx%d)"), r.left, r.top, r.right, r.bottom, r.Width(), r.Height());
	else if (nFormat == 3)
		str.Format(_T("l = %d, t = %d, r = %d, b = %d"), r.left, r.top, r.right, r.bottom);
	else
		str.Format(_T("%d, %d, %d, %d"), r.left, r.top, r.right, r.bottom);

	return str;
}


void adjust_rect_range(int32_t *l, int32_t *t, int32_t *r, int32_t *b, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool retainSize)
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

		*r = maxx - 1;
	}

	if (*b >= maxy)
	{
		if (retainSize)
		{
			*t -= (*b - maxy);
			if (*t < miny)
				*t = miny;
		}

		*b = maxy - 1;
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

void adjust_rect_range(CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize)
{
	int32_t l = rect.left;
	int32_t t = rect.top;
	int32_t r = rect.right;
	int32_t b = rect.bottom;
	adjust_rect_range(&l, &t, &r, &b, minx, miny, maxx, maxy, bRetainSize);
	rect = CRect(l, t, r, b);
}

//������� ���ʿ� ���� �簢���� ���ο� ũ��� ������ ��� ���� ���¸� �����ϰ� ������ �ʿ䰡 ���� ��� ���.
void adjust_with_monitor_attached(CRect rOld, CRect &rNew)
{
	int nw = rNew.Width();
	int nh = rNew.Height();

	if (rOld.left == g_dqMonitors[0].left)
		rNew.MoveToX(g_dqMonitors[0].left);
	if (rOld.top == g_dqMonitors[0].top)
		rNew.MoveToY(g_dqMonitors[0].top);
	if (rOld.right == g_dqMonitors[0].right)
		rNew.MoveToX(g_dqMonitors[0].right - nw);
	if (rOld.bottom == g_dqMonitors[0].bottom)
		rNew.MoveToY(g_dqMonitors[0].bottom - nh);
}


//rTarget�� ���ϴ� dRatio�� �ִ� �簢���� ���Ѵ�.
//attach_left ���� �ɼ��� �� �ʿ䰡 �ִ�.
CRect get_ratio_max_rect(CRect rTarget, int w, int h, int attach)
{
	return get_ratio_max_rect(rTarget, (double)w / (double)h, attach);
}

//rTarget�� ���ϴ� dRatio�� �ִ� �簢���� ���Ѵ�.
CRect get_ratio_max_rect(CRect rTarget, double dRatio, int attach)
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

//w x h �簢���� target�ȿ� ���� �� �߾ӿ� ǥ�õǰ� �ϴ� �簢�� ������ �����Ѵ�.
//w, h���� target�� �������� target���� ū ������ ���ϵ� ���̴�.
CRect get_center_rect(CRect target, int w, int h)
{
	return CRect(target.left + (target.Width() - w) / 2, target.top + (target.Height() - h) / 2, w, h);
}

//�־��� ������ �����ϴ� �ִ� �簢���� ���Ѵ�.
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


//�ٰ����� ���̸� ���Ѵ�. ��, ���� �ϳ��� �����Ǹ� �������� �ʴ´�.
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

//return���� char*�� �ݵ�� ��� �� free()����� ��.
char* replace(char* s, const char* olds, const char* news)
{
	char* result;
	int i, cnt = 0;
	int newWlen = strlen(news);
	int oldWlen = strlen(olds);

	// Counting the number of times old word 
	// occur in the string 
	for (i = 0; s[i] != '\0'; i++) {
		if (strstr(&s[i], olds) == &s[i]) {
			cnt++;

			// Jumping to index after the old word. 
			i += oldWlen - 1;
		}
	}

	// Making new string of enough length 
	result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1);

	i = 0;
	while (*s) {
		// compare the substring with the result 
		if (strstr(s, olds) == s) {
			strcpy(&result[i], news);
			i += newWlen;
			s += oldWlen;
		}
		else
			result[i++] = *s++;
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

//����, '\t', '\r', '\n', '\0' ��� ����
void trim(std::string& str)
{
	trim_left(str);
	trim_right(str);
}

//����, '\t', '\r', '\n' ��� ����
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

//src���ڿ����� chars�� ��� �����Ѵ�.
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

//������������(xml) ���� ����
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

//32��Ʈ�� dw�� ����ִ� R, G, B�� �����Ͽ� 16��Ʈ(5+6+5) �÷��� �����Ѵ�.
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

//�� DWORD�� WORD�� ��ȯ�Ͽ� �ϳ��� DWORD�� ��ȯ�Ѵ�.
DWORD RGB24ToRGB565(DWORD dw1, DWORD dw2)
{
	DWORD dw = RGB24ToRGB565(dw1);
	dw <<= 16;

	dw |= RGB24ToRGB565(dw2);
	return dw;
}

//RGB565�� ���� WORD�� �ٽ� 24��Ʈ RGB(DWORD)�� ������Ų��.
DWORD RGB565ToRGB24(WORD wd)
{
	//565�� ������� wd���� �ٽ� rgb�� �������Ѻ���.
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

CString	get_last_error_string(bool show_msgBox)
{
	return get_last_error_string(GetLastError(), show_msgBox);
}

CString	get_last_error_string(DWORD errorId, bool show_msgBox)
{
	TCHAR* message = nullptr;
	CString result;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		nullptr,
		errorId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (TCHAR*)&message, 0, nullptr);

	result.Format(_T("error code = %d (%s)"), errorId, CString(message).Trim());
	LocalFree(message);

	if (show_msgBox)
		AfxMessageBox(result);

	return result;
}

//�������� ���� b�� ���� a�� �ݽð� �����̸� ���, �ð�����̸� ����, �����̸� 0�� ��ȯ �Ѵ�.
double ccw(vector2 a, vector2 b)
{
    return a.cross(b);
}

//�� p�� �������� ���� b�� ���� a�� �ݽð� �����̸� ���, �ð�����̸� ����, �����̸� 0�� ��ȯ �Ѵ�.
double ccw(vector2 p, vector2 a, vector2 b)
{
    return ccw(a-p, b-p);
}

// - �� a, b�� ������ ������ �� c, d�� ������ ������ ������ x�� ��ȯ�Ѵ�.
// - �� ������ �����̸�(��ġ�� ��� ����) ������, �ƴϸ� ���� ��ȯ�Ѵ�.
// ��ó : http://bowbowbow.tistory.com/17
bool GetIntersectionPoint(vector2 a, vector2 b, vector2 c, vector2 d, vector2& x)
{
    double det = (b-a).cross(d-c);
    //�μ��� ������ ���
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

//4������ ������ �� ������ ���� �����ϴ��� ���θ� �Ǻ��Ѵ�.
bool IsIntersect2LineSegment(vector2 a, vector2 b, vector2 c, vector2 d)
{
    double ab = ccw(a, b, c)*ccw(a, b, d);
    double cd = ccw(c, d ,a)*ccw(c, d, b);

    //�� ������ �� ������ ���� �ְų� ������ ��ġ�� ���
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

//�� a, b�� �� c, d�� ������ �� ���� �� �� �̵��� �� ������ ��ġ���� Ȯ���Ѵ�.
bool paralleSegments(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p){
    if(b < a) std::swap(a,b);
    if(d < c) std::swap(c,d);

    //�� �������� ���ų� �� ������ ��ġ�� �ʴ� ��츦 �켱 �ɷ�����. ������ 1�� ������ ����̴�.
    if(ccw(a, b, c) != 0 || b < c || d < a) return false;

    //�� ������ Ȯ���� ��ģ�ٸ� ������ �ϳ��� ã�´�.
    if(a<c) p = c;
    else p = a;
    return true;
}


// - p�� �� �� a, b�� ���θ鼭 �� ���� x, y�࿡ ������ �ּһ簢�� ���ο� �ִ��� Ȯ���Ѵ�.
// a, b, p�� ������ �� �ִٰ� �����Ѵ�.
bool inBoundingRectangle(vector2 p, vector2 a, vector2 b){
    if(b < a) std::swap(a, b);
    return p == a || p == b || (a <p && p < b);
}


// - �� �� a, b�� ������ ���а� �� �� c, b�� ������ ������ p�� ��ȯ�Ѵ�.
// - ������ �������� ��� �ƹ����̳� ��ȯ�Ѵ�.
bool segmentIntersection(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p){
    //�� ������ ������ ��츦 �켱 ���ܷ� ó���Ѵ�.
    if(!GetIntersectionPoint(a, b, c, d, p))
        return paralleSegments(a, b, c, d, p);
    //p�� �� ���п� ���ԵǾ� �ִ� ��쿡�� ���� ��ȯ�Ѵ�.
    return inBoundingRectangle(p, a, b) && inBoundingRectangle(p, c, d);
}


//�ﰢ�� �������� �߽��� ������������ ���Ѵ�.(http://kipl.tistory.com/113)
int circumCenter(CPoint A, CPoint B, CPoint C, double *xc, double *yc)
{
	double ax = A.x - C.x ;
	double ay = A.y - C.y ;
	double bx = B.x - C.x ;
	double by = B.y - C.y ;
	double asq = ax * ax + ay * ay;
	double bsq = bx * bx + by * by;
	double ccw = ax * by - ay * bx;

	 // �� ���� ������ ���� ���� �ʴ� ���; �� ��츸 �������� ���ǵ�;
	if (ccw == 0.)
		return 0;

	*xc = C.x + (by * asq - ay * bsq) / (2. * ccw) ;
	*yc = C.y + (-bx * asq + ax * bsq) / (2. * ccw) ;
	return 1;
}

//�ﰢ�� �������� �߽��� ��������� ���Ѵ�.
int circumCenter2(CPoint P, CPoint Q, CPoint R, double *xc, double *yc)
{
	double A = Q.x - P.x;
	double B = Q.y - P.y;
	double C = R.x - P.x;
	double D = R.y - P.y;
	double E = A * (P.x + Q.x) + B * (P.y + Q.y);
	double F = C * (P.x + R.x) + D * (P.y + R.y);
	double G = 2. * (A * D - B * C);

	// �� ���� �������� ������ �ʴ� ���; �� ��츸 �������� ���ǵȴ�;
	if (G == 0.)
		return 0;

	*xc = (D * E - B * F) / G;
	*yc = (A * F - C * E) / G;
	return 1;
}

//�ﰢ�� �������� �������� ���Ѵ�.
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


//���� ���� ������ ���� �����ϴ��� �Ǻ�
//�� �Լ� �Ʒ��� PtInPolygon0 ~ PtInPolygon2������ �̿����� ���� �𸣰����� �ùٸ� �Ǻ��� ���� ���Ѵ�.
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
	USES_CONVERSION; //convert�� ���� ��ũ��
	pbstr=OLE2A(bstr); //bstr�� char�� convert
	return CString(pbstr);
} 

BSTR CStringTobstr(CString str) 
{
	return str.AllocSysString(); //CString�� bstr�� convert
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
//CString ==> CComVariant�� ��ȯ 
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

//256 gray bmi ����
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

//main���� EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); �� �����ϰ�
//�� ���Ͽ� ���������� ����� g_dqMonitor�� �̿��ϸ� �ȴ�.
//��, Win32API�� EnumDisplayMonitors()�� ȣ���Ҷ��� �ݵ�� g_dqMonitors.clear()�� ����� �ϹǷ�
//enum_display_monitors()�Լ��� ��ü�Ѵ�.
void enum_display_monitors()
{
	g_dqMonitors.clear();
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
}

//main���� enum_display_monitors(); �� �����ϰ�
//�� ���Ͽ� ���������� ����� g_dqMonitor�� �̿��ϸ� �ȴ�.
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	// ����� ������ ������ ����ü
	MONITORINFOEX mi;
	mi.cbSize = sizeof(MONITORINFOEX);

	// ����� �ڵ��� ����Ͽ� �ش� ����� ������ �����´�.
	GetMonitorInfo(hMonitor, &mi);

	CString str;

	// �ָ���ͷ� ������ ����� ���������� üũ�Ѵ�.
	if (mi.dwFlags & MONITORINFOF_PRIMARY) 
	{
		str.Format(_T("hMonitor = %X,  ��ǥ : [ (%04d, %04d) - (%04d, %04d) ], < Primary-Monitor > %s"), 
					hMonitor, lprcMonitor->left,lprcMonitor->top,lprcMonitor->right,lprcMonitor->bottom, mi.szDevice);
	}
	else
	{
		str.Format(_T("hMonitor = %X,  ��ǥ : [ (%04d, %04d) - (%04d, %04d) ], %s"), 
					hMonitor, lprcMonitor->left,lprcMonitor->top,lprcMonitor->right,lprcMonitor->bottom, mi.szDevice);
	}

	g_dqMonitors.push_back(lprcMonitor);

	TRACE(_T("%s\n"), str);

	return TRUE;
}

//x, y�� �����ִ� ����� �ε����� ����
int	get_monitor_index(int x, int y)
{
	enum_display_monitors();

	for (int i = 0; i < g_dqMonitors.size(); i++)
	{
		if (g_dqMonitors[i].PtInRect(CPoint(x, y)))
			return i;
	}

	return -1;
}

//r�� �����ִ� ����� �ε����� ����. �������� ������ ��𿡵� ���ٸ� -1�� ����.
int	get_monitor_index(CRect r, bool entire_included)
{
	enum_display_monitors();

	for (int i = 0; i < g_dqMonitors.size(); i++)
	{
		if (entire_included)
		{
			if (RectInRect(g_dqMonitors[i], r))
				return i;
		}
		else if (r.IntersectRect(r, g_dqMonitors[i]))
		{
			return i;
		}
	}

	return -1;
}

//��Ƽ����� ��ü ���� �簢�� ����
CRect get_entire_monitor_rect()
{
	enum_display_monitors();
	CRect rEntire(0, 0, 0, 0);

	for (int i = 0; i < g_dqMonitors.size(); i++)
	{
		rEntire.left = MIN(rEntire.left, g_dqMonitors[i].left);
		rEntire.top = MIN(rEntire.top, g_dqMonitors[i].top);
		rEntire.right = MAX(rEntire.right, g_dqMonitors[i].right);
		rEntire.bottom = MAX(rEntire.bottom, g_dqMonitors[i].bottom);
	}

	return rEntire;
}

void SetForegroundWindowForce(HWND hWnd, bool makeTopMost)
{
	//�ֻ����� �ø��� �ڵ尡 �� �Լ��� �� ó�� �κп��� ������� ������
	//�ٸ� �ֻ����� ����� â�� ���� �׻� �ö���� �ʴ� ������ �ִ�.
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

//2D ���� ���󿡼� �̹� �˷��� �������� �������� ������ �� ���� ������� �Ÿ��� ���(by sucwon)
//��, ������ ������ ī�޶󿡼� �Կ��� �����̹Ƿ� �ǻ�ü�� ������� �Ÿ��� �ƴ� ��ü���� �Ÿ��� �� ��Ȯ�� ���̸�
//���� ������ ��ġ�� ���� �ϴ� ���Ͱ� �ƴ� ������ �ϴܰ� �ǻ�ü�� ���� �Ÿ���
//������ �� ��Ȯ�� ����� �ȴ�.
double getObjectDistance(int width, int height, int vanishing_y, int x, int y, int cam_height, double *dx, double *dy, int cali_width, int cali_height, double fl_x, double fl_y)
{
		//// Focal Length (4���� ������� ���� �����̸� ī�޶󸶴� �ٸ� ���� ����)
/*
		double fl_x = 2361.130;
		double fl_y = 2357.436;

		//// ����
		double c_x = 1066.161;
		double c_y = 407.246;
*/
/*
		double tilt = atan2(vanishing_y - c_y, fl_y);				// 1��° parameter �� ����, 2��° parameter�� Camera Focal Length
		double m_x = (x - c_x) / fl_x;								// ������ǥ (x)
		double m_y = (y - c_y) / fl_y;								// ������ǥ (y)

		double C1P1 = (double)cam_height * tan((PI/2) + tilt - atan(m_y));	// C`P`
		double CP1 = sqrt(((double)cam_height * (double)cam_height) + (C1P1*C1P1));					// CP`
		double Cp1 = sqrt(1 + (m_y*m_y));
		double PP1 = m_x * CP1 / Cp1;

		double real_distance = sqrt((C1P1*C1P1) + (PP1*PP1)) / 100.0f;

		return real_distance;
*/
	//ī�޶� Ķ���극�̼� ���� ���ؼ� ���� ���ؾ� �Ѵ�.
	//������ ���� �Է� ������ 1/2�̴�.
	double c_x = (double)width / 2.0;	//691.448;
	double c_y = (double)height / 2.0;	//404.974;

	//1920, 1080�� Ķ���극�̼ǿ� ����� ī�޶� ������ ũ���̴�.
	double tilt = atan2((vanishing_y/(double)(height)*((double)(cali_height))) - c_y, fl_y);
	double csh = (double)cam_height / 10.0;                              // ī�޶� ��ġ����
	double m_x = ((x / (double)(width) * (double)cali_width) - c_x) / fl_x;                        // ������ǥ (x)
	double m_y = ((y / (double)(height)*(double)(cali_height)) - c_y) / fl_y;                        // ������ǥ (y)

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



//gray �迭����
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
		integral_img_current_line[-1] = 0;	//	���� ���� ���� 0���� ����

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
//���� �׽�Ʈ ���� ����. ��Ҹ� ����? Ȯ��, ��� ��� ����? skip�� �ִ°ɷ� ���� ��Ҹ� �����ҵ��ϴ�.
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

//gray image�� 3ä�� �Ǵ� 4ä�� �̹��� �������� �������ش�.
void gray2color(uint8_t *gray, int gray_width, int gray_height, uint8_t *dst, int dst_width, int dst_height, int dst_ch, uint8_t alpha)
{
	int x, y, k;
	uint8_t *src = gray;
	bool resized = false;

	//gray�� dst�� ũ�Ⱑ �ٸ��� resize�� �� �����ؾ� �Ѵ�.
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

//3ä�� �Ǵ� 4ä�� �̹����� ���� ��� �̹����� �����Ѵ�.
//dst�� �ݵ�� �Ҵ�� �޸��ּ��̾�� �Ѵ�.
void color2gray(uint8_t *src, int src_width, int src_height, int src_ch, uint8_t *dst, int dst_width, int dst_height)
{
	if (src_ch < 3 || src_ch > 4)
		return;

	int x, y;
	int r, g, b;
	uint8_t *gray = dst;
	bool resized = false;
	
	//src�� dst�� ũ�Ⱑ �ٸ��� gray�� ��ȯ�� �� resize����� �Ѵ�.
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

//����� 1ä�� ���� �����ȴ�.
//�켱 ���ϰ� -90���� �����Ѵ�.
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

//opencv�� flip�� ���� ���������� �̸� �浹�� ���ϱ� ���� mirror��� �̸��� �����.
//����� 1ä�� ���� �����ȴ�.
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

//��ũ���� ǥ�õ� �̹����� �׷��� �簢���� ���� �̹������� �簢�� ��ǥ
//sr : �̹����� �׷��� �簢��
//displayed : �̹����� ǥ�õǰ� �ִ� �簢�� ����
//real : ���� �̹����� ũ��
//resized : zoom in/out�� ���� ����� ũ��
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

//�� �簢���� ��ġ�� ������ �����Ѵ�.
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

	//�� ���� ���

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

	//�Ʒ� LoadImage �Լ��� ���ؼ� ������ ������ �ҷ��� ���
	//���α׷� ���� �� �ݵ�� DestroyIcon�� ����� �Ѵ�.
	//�׷��� DestroyIcon ���ص� �Ǵ� LR_SHARED�� ����ߴ�.
	return static_cast<HICON>(::LoadImage(hInstance, MAKEINTRESOURCE(nID), IMAGE_ICON, cx, cy, LR_SHARED));
}

CSize draw_icon(CDC* pDC, HICON hIcon, CRect r)
{
	//int cxIcon = GetSystemMetrics(SM_CXICON);
	//int cyIcon = GetSystemMetrics(SM_CYICON);

	//SM_CXICON�� �̿��ؼ� �⺻������ �׸��°� �ƴ϶�
	//���� hIcon�� ũ�⸦ ���ؼ�
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

	int x = (r.Width() - w + 1) / 2;
	int y = (r.Height() - h + 1) / 2;

	::DrawIconEx(pDC->GetSafeHdc(), x, y, hIcon, w, h, 0, NULL, DI_NORMAL);

	return CSize(w, h);
}

//font size to LOGFONT::lfHeight
LONG get_logical_size_from_font_size(HWND hWnd, int font_size)
{
	HDC hDC = ::GetDC(hWnd);
	LONG size = -MulDiv(font_size, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	::DeleteDC(hDC);

	return size;
}

//LOGFONT::lfHeight to font size
LONG get_font_size_from_logical_size(HWND hWnd, int logical_size)
{
	HDC hDC = ::GetDC(hWnd);
	LONG size = -MulDiv(logical_size, 72, GetDeviceCaps(hDC, LOGPIXELSY));
	::DeleteDC(hDC);

	return size;
}

//�޸� �Ϻ� ���� �Լ�
//��, src�� �����Ͱ� ���ӵ� �޸𸮻� ����Ǿ� �־�� �ϴ� ������ �ִ�.
//src�� �Ϻ� ���� ����. �׽�Ʈ �ʿ���.
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

//src���� roi ������ �߶� dst_width * dst_height ũ��� resize ��Ų��.
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

//��ó: http://boongubbang.tistory.com/255 [Boongubbang]
//unicode�� �����ϴ� �Լ��� �׽�Ʈ �̿�!
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

				//�Լ� ������ quicksort(T *v, ...)�� ���� �����ؼ� ����ϸ� �Ʒ� ���忡�� ����� ������ �߻��Ѵ�.
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

//OnInitDialog�� ���� ���� �ѹ� ����Ǵ� �Լ������� ȣ���Ѵ�.
//���� ��ǥ��� �����Ͽ� ǥ���Ѵ�.
//��, �� ũ�Ⱑ ����� ���̸� CenterWindow()��,
//ũ�Ⱑ invalid�ϴٸ� ���� ũ��� ǥ���Ѵ�.
void RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection, bool use_maximize, bool resize_window)
{
	CRect	rc;
	CString sSection = _T("screen");

	//resize window���� �ڵ� üũ�ϴ� �ڵ尡 �ȸ����µ��ϴ�.
	//bool is_resizable_window = ((::GetWindowLongPtr(pWnd->m_hWnd, GWL_STYLE) & WS_THICKFRAME) == WS_THICKFRAME);

	if (sSubSection != "")
		sSection = sSubSection + "\\screen";	//�����ð� �ƴ� �������ø� ��� �Ѵ�.

	rc.left		= pApp->GetProfileInt(sSection, _T("left"), 0);
	rc.top		= pApp->GetProfileInt(sSection, _T("top"), 0);
	rc.right	= pApp->GetProfileInt(sSection, _T("right"), 0);
	rc.bottom	= pApp->GetProfileInt(sSection, _T("bottom"), 0);

	if (rc.IsRectNull())
	{
		pWnd->CenterWindow();
		return;
	}

	//�簢�� ������ ��ȿ�ϰ� �� ũ�⵵ 10x10�� �Ѵ´ٸ� �� ũ��� ������Ű��
	if (rc.IsRectEmpty() == false && rc.Width() > 10 && rc.Height() > 10)
	{
		UINT_PTR flag = SWP_NOZORDER;
		if (!resize_window)
			flag |= SWP_NOSIZE;

		SetWindowPos(pWnd->m_hWnd, NULL, rc.left, rc.top, rc.Width(), rc.Height(), 0);
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

//OnBnClickedCancel()�� ���� ���α׷��� ����� �� ȣ���Ѵ�.
//OnWindowPosChanged()���� �Ź� ȣ���߾����� 
//OnInitDialog���� RestoreWindowPosition�� ȣ���� ��
//OnWindowPosChanged�� ȣ��Ǹ鼭 maximized ������ false�� �ʱ�ȭ�Ǿ� ������.
//sSubSection�� �����ϸ� �� �̸��� "\\screen"�� �ٿ��� �����Ѵ�.
void SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection)
{
	if (pWnd->IsWindowVisible() == false ||	pWnd->IsIconic())
		return;

	CRect	rc;
	CString sSection = _T("screen");

	if (sSubSection != "")
		sSection = sSubSection + "\\screen";	//�����ð� �ƴ� �������ø� ��� �Ѵ�.

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

//init_YUV_lookup_table(); �Լ��� �ݵ�� �ѹ� ȣ���� �� ����� ��.
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

	// �����쿡���� ������ ���ϰ� �Ųٷ� ������� ������ �Ʒ��� ���� �ڵ� �ۼ�.
	//mat�� ����Ͽ� converting�ϴ� ��� �Ʒ� �ڵ尡 ��������.
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

	// �Ϲ����� ��� �Ʒ��� �ڵ� �����.
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
//�� �Լ��� yv12���� �ƴѰͰ���. �� �� Ȯ���غ��� �Ѵ�.
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

		//������ ����Ʈ ���� 2��
		value = ((int)u[pos0]+(int)u[pos1]+(int)u[pos2]+(int)u[pos3])>>2;
		Clamp(value, 0, 255); 
		U[addr] = (BYTE)value;

		//�� ������ 4 : ��ճ���
		value = ((int)v[pos0]+(int)v[pos1]+(int)v[pos2]+(int)v[pos3])>>2;
		Clamp(value, 0, 255); 
		V[addr] = (BYTE)value;
		addr++;
	}
	return  true;
}

//�׽�Ʈ �ʿ���.
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

	//ȭ�鿡 ǥ�õ� ������ l,t���� ���� Ȯ�� ���󳻿����� �����ǥ�� ��ȯ�� ��
	*dx -= rDisplayedImageRect.left;
	*dy -= rDisplayedImageRect.top;

	//Ȯ��� ������ ������
	*dx /= dZoom;
	*dy /= dZoom;
}

void get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint *pt_dst)
{
	double dx = (double)pt_src.x;
	double dy = (double)pt_src.y;

	get_real_coord_from_screen_coord(rDisplayedImageRect, srcWidth, dx, dy, &dx, &dy);
	pt_dst->x = (long)dx;
	pt_dst->y = (long)dy;
}

void get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect *r_dst)
{
	double x1 = (double)(r_src.left);
	double y1 = (double)(r_src.top);
	double x2 = (double)(r_src.right);
	double y2 = (double)(r_src.bottom);

	get_real_coord_from_screen_coord(rDisplayedImageRect, srcWidth, x1, y1, &x1, &y1);
	get_real_coord_from_screen_coord(rDisplayedImageRect, srcWidth, x2, y2, &x2, &y2);

	r_dst->left = (long)(x1);
	r_dst->top = (long)(y1);
	r_dst->right = (long)(x2);
	r_dst->bottom = (long)(y2);
}

void get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, double sx, double sy, double* dx, double* dy)
{
	if (srcWidth <= 0)
		return;

	double dZoom = (double)rDisplayedImageRect.Width() / (double)(srcWidth);

	*dx = sx;
	*dy = sy;

	//Ȯ��� ������ ���ϰ�
	*dx *= dZoom;
	*dy *= dZoom;

	//�ɼ¸�ŭ �����ǥ�� �̵���Ų��.
	*dx += rDisplayedImageRect.left;
	*dy += rDisplayedImageRect.top;
}

void get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint* pt_dst)
{
	double x = (double)pt_src.x;
	double y = (double)pt_src.y;

	get_screen_coord_from_real_coord(rDisplayedImageRect, srcWidth, x, y, &x, &y);
	pt_dst->x = (long)x;
	pt_dst->y = (long)y;
}

void get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect* r_dst)
{
	double x1 = (double)(r_src.left);
	double y1 = (double)(r_src.top);
	double x2 = (double)(r_src.right);
	double y2 = (double)(r_src.bottom);

	get_screen_coord_from_real_coord(rDisplayedImageRect, srcWidth, x1, y1, &x1, &y1);
	get_screen_coord_from_real_coord(rDisplayedImageRect, srcWidth, x2, y2, &x2, &y2);

	r_dst->left = (long)(x1);
	r_dst->top = (long)(y1);
	r_dst->right = (long)(x2);
	r_dst->bottom = (long)(y2);
}

//str�� from ��ġ ���Ŀ� �ִ� ���� �������� num�� �־��ְ� ���� ������ġ�� return�Ѵ�.
//���ϵǴ� ������ ������ġ�� from���Ͱ� �ƴ� str ��ü ���ڿ������� ��ġ�̴�.
//���� �տ� ���� '+' or '-'�� ���õǹǷ� ���� ��ȣ�� �Ǻ��ؾ� �Ѵٸ�
//���ϵǴ� ��ġ�� �տ� �ִ� ���ڸ� �˻��Ͽ� �������, �������� �Ǻ��ϸ� �ȴ�.
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
		//�Ҽ����� �� ó�����ų� �ߺ��Ǹ� ��ŵ�ȴ�.
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
//digits : �ڸ���(1.0.0.1�� ���� �ڸ��� 4)
bool valid_version_string(CString versionStr, int digits)
{
	std::deque<CString> token;
	get_token_string(versionStr, token, '.');
	if (token.size() == digits)
		return true;

	return false;
}

//���� �Ǵ� IP�ּҵ��� �׳� ���ڿ��� ���ϸ� 1.0.9.0�� 1.0.10.0���� �� ũ�ٰ� �����Ƿ�
//.�� ���� ���ڷ� �������� �� ����� ���� �߻�(1.0.1.13 > 1.0.10.3���� ũ�ٰ� �Ǵ���)
//�ᱹ �� �ڸ������� �����ϰų� �ڸ����� ���� �� ���Ѵ�.
//���ϰ��� strcmp�� ������ ��Ģ���� �Ǵ��Ѵ�.(+:str0�� ŭ, -:str1�� ŭ, 0:����)
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

//src�� n�� ������ ���ڿ� ����. n���� ����, ���� �ʿ��� ��� ���
CString make_string(CString src, int n)
{
	CString result;

	for (int i = 0; i < n; i++)
		result += src;

	return result;
}

//button�� ������ �����Ѵ�.
DWORD		getButtonStyle(HWND hWnd)
{
	UINT button_style = BS_PUSHBUTTON;

	DWORD dwStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE); 

	return dwStyle;

	// Check Box ��Ʈ���� ���, 3STATE ������ Ȯ����. 
	if (((dwStyle & BS_AUTO3STATE) == BS_AUTO3STATE) || 
		((dwStyle & BS_3STATE) == BS_3STATE))
		button_style = BS_CHECKBOX; 

	switch (dwStyle & 0x0F) // ��ư�Ӽ�. 
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
		// BS_AUTO3STATE | BS_3STATE ���¸� �����Ѵ�. 
		DWORD dwRemove = BS_AUTO3STATE | BS_3STATE;        
		// BS_OWNERDRAW �Ӽ��� �����Ѵ�. 
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

//�׷쳻�� ���ӵ� radio ��ư�鿡 ���� �ϰ� ó���� �Լ�.
//MFC�� �⺻ CheckRadioButton() �Լ����� ���� BST_UNCHECKED ����� �������� �ʱ� ������
//�Ʒ� �Լ��� ������ �����ؼ� ����Ѵ�.
//id_offset�� 0���� ������ first ~ last���� ��� unchecked.
//���� Ư�� radio button�� ������� ���콺 Ŭ���̳� Ű���� ��� ����
//���� üũ�ϴ� ��찡 �ƴ�
//���α׷��󿡼� SetCheck�� ���� �ڵ����� ���õ� ���·� ǥ���ϰ� �Ǹ�
//���� ���õǾ��� radio�� ������ focus�� ������ �ְ� �ǰ�
//�̴� �ٸ� â�� focus�� �־��ٰ� ���� dlg�� �ٽ� focus�� ������ ����
//ON_CONTROL_RANGE�Լ��� ȣ��Ǹ鼭 �ǵ�ġ ���� ������ �߻��ϰ� �ȴ�.
//���� Ư�� ������ radio button �߿� �ϳ��� ���α׷��󿡼� �ڵ����� ���õǰ� �Ϸ���
//�ݵ�� focus���� ���� ��߸� �̷��� �������� ���� �� �ִ�.
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

//dialog based���� Ű�Է����� ������ �����ϴµ� CEdit�� ���� �Է�â�� ��Ŀ���� ������
//PreTranslateMessage���� ����Ű�� charŰ�� ó���ϱⰡ ����ϴ�.
//���� ���� ��Ŀ���� ���� ��Ʈ���� CEdit�̰� enable�̰� readonly�� �ƴ� ��쿡��
//PreTranslateMessage���� �Էµ� Ű�� ó���ϵ��� �Ѵ�.
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
		str.Format(_T("%s [%s][%d] %s%c"), get_cur_datetime_string(1, true, _T(" "), true, true, true), func, line, str, (linefeed ? '\n' : '\0'));

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

//"yy-MM-dd" �Ǵ� "yy/MM/dd" ��¥ ǥ�� ���Ŀ� �´��� �˻�
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

//"hh:mm:ss" �ð� ǥ�� ���Ŀ� �´��� �˻�
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

//��¥�ð� ������ yyyy/mm/dd hh:mm:ss �������� �����.
//230303192736			//�ּ����� ���̴� 12
//23/3/3 9:7:6			//�ּ����� ���̴� 12
//2023/3/3 9:7:6
//=>2023/03/14 19:27:36.123	//�ִ���̴� 23
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

//����ð��� �����ϴµ�
//������ ������ QueryPerformanceFrequency�� QueryPerformanceCounter�� �̿��Ѵ�.
//�̴� ���̱⵵ �ϰ� thread safe���� �����Ƿ� �����ϸ� ����� �������� �ʴ´�.
//clock�� �̿��� ������ε� ���� ������ �ҿ� �ð��� ������ �� �ְ�
//linux������ �״�� ����� �� �ִ� ������ �ִ�.
//GetTickCount()�� ������ ���� �߻��ϴ� ������ �ִ�.
//unit : ms
//��, �� ���� long���� ���ǹǷ� �� ���ϰ��� ������ ǥ���� �� �ִ�.
//������ �ð� �����ϴµ��� �������� �ʴ´�.
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
IsAvailableMemory : 1. �־��� �޸� �ּ��� ���°� ���� �ּҷ� Ȯ���Ǿ����� �˻� (���� ����� ���� �ʿ�)
					2. �־��� �޸��� ��ȣ �Ӽ��� �б⳪ ���Ⱑ �������� �˻�
	Param :
		LPVOID  pMemoryAddr : �˻��ϰ��� �ϴ� �޸��� �ּ�
	Return Value :
		ERROR_SUCCESS : System Error Code, ��� ���� ������ ����� �����ڵ�
		��Ÿ �� : Read/Write ������ �޸𸮰� �ƴϸ� �ش� �ּ��� Protect Mode ��
					��Ÿ���� 0�� �ƴ� ���� ������.
	Reference :
		1. https://docs.microsoft.com/en-us/previous-versions/aa915370(v=msdn.10)/
		2. https://docs.microsoft.com/en-us/windows/desktop/debug/system-error-codes--0-499-/
*/
INT IsAvailableMemory(LPVOID pMemoryAddr)
{
	MEMORY_BASIC_INFORMATION    MemInfo = { 0, };
	SIZE_T  nResult = 0;

	nResult = VirtualQuery(pMemoryAddr, &MemInfo, sizeof(MemInfo));

	if (nResult == 0) // Ŀ�� ������ ��� VirtualQuery ��ü�� Fail��.  
	{
		return -1;
	}
	else if (!(MemInfo.State & MEM_COMMIT))
	{
		return MemInfo.State;
	}
	else if ((MemInfo.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)))
	{
		return  ERROR_SUCCESS; // System Error Code ���� : Reference ����
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
 * @brief HTTP POST �޼ҵ�� ������ ���ε��Ѵ�.
 * @param pszUrl   HTTP URL
 * @param pszFilePath ���ε� ���� full path
 * @returns �����ϸ� true �� �����ϰ� �׷��� ������ false �� �����Ѵ�.
 * ���� �ڵ�� chatIdx �ʵ�� image �ʵ�(���ϰ��)�� ����ϴ� �ڵ��̹Ƿ�
 * �ʿ信 ���� �ڵ带 �����Ͽ� ����ϸ� �ȴ�.
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

	// pszUrl ���� host, path �� �����´�.
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

	// ���� ��ο��� ���� �̸��� �����´�.
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

	// ���� ũ�⸦ �����´�.
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

		// HTTP �����ϰ� ������ �����Ѵ�.
		CHttpConnection* pclsHttpConn = clsSession.GetHttpConnection(CString(strHost.c_str()), dwFlag, (INTERNET_PORT)iPort, NULL, NULL);
		if (pclsHttpConn)
		{
			int m_secureFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;

			CHttpFile* pclsHttpFile = pclsHttpConn->OpenRequest(CHttpConnection::HTTP_VERB_POST, CString(pszPath), NULL, 0, NULL, NULL, m_secureFlags);

			if (pclsHttpFile)
			{
				USES_CONVERSION;
				std::string strBoundary = "55CA7C5681214d98912C76366A8042BA";

				// HTTP ��û header �� �����Ѵ�.
				std::wstring strContentType = L"Content-Type: multipart/form-data; boundary=";
				//strContentType.append(A2T(strBoundary.c_str()));
				strContentType.append(strBoundary.begin(), strBoundary.end());
				strContentType.append(L"\r\n");
				pclsHttpFile->AddRequestHeaders(CString(strContentType.c_str()));// .c_str());

				std::string strBody;
				std::string strChatIndex = std::to_string(chatIndex);

				// body�� chatIdx�� �����Ѵ�.
				strBody.append("--");
				strBody.append(strBoundary);
				strBody.append("\r\n");
				strBody.append("Content-Disposition: form-data; name=\"chatIdx\"\r\n\r\n");
				strBody.append(strChatIndex);
				strBody.append("\r\n");

				// body�� ���ϸ��� �����Ѵ�.
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
					// HTTP ��û header �� �����Ѵ�.
					pclsHttpFile->SendRequestEx(strBody.length(), HSR_SYNC | HSR_INITIATE);

					// HTTP ��û body �� �����Ѵ�.
					pclsHttpFile->Write(strBody.c_str(), strBody.length());
					pclsHttpFile->EndRequest(HSR_SYNC);

					// HTTP ���� body �� �����Ѵ�.
					std::string strResponse;
					DWORD dwCode;
					while (1)
					{
						iLen = pclsHttpFile->Read(szBuf, sizeof(szBuf));
						if (iLen <= 0) break;
						strResponse.append(szBuf, iLen);
					}

					// ���� �ڵ尡 200 OK ���� Ȯ���Ѵ�.
					pclsHttpFile->QueryInfoStatusCode(dwCode);
					if (dwCode == HTTP_STATUS_OK)
					{
						bRes = true;
					}
				}
				catch (CInternetException* pclsException)
				{
					// ������ ���ῡ �����ϸ� CInternetException �� �߻��Ѵ�.
				}

				delete pclsHttpFile;
			}
			delete pclsHttpConn;
		}
		fclose(fd);
	}
	return bRes;
}


//������ �ѱ۸� ���Ͽ� ���� ó�������� request_url()�Լ� ��� �߰��Ͽ� �׽�Ʈ �غ�����
//�������� �����ε��Ͽ� �켱 �� �Լ� ����� ������...
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
		if (!FileWrite.Open("c:\\scpark\\���.pdf", CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyNone))
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

	// pszUrl ���� host, path �� �����´�.
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

	// ���� ��ο��� ���� �̸��� �����´�.
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

	// ���� ũ�⸦ �����´�.
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

		// HTTP �����ϰ� ������ �����Ѵ�.
		CHttpConnection* pclsHttpConn = clsSession.GetHttpConnection(CString(strHost.c_str()), dwFlag, (INTERNET_PORT)iPort, NULL, NULL);
		if (pclsHttpConn)
		{
			CHttpFile* pclsHttpFile = pclsHttpConn->OpenRequest(CHttpConnection::HTTP_VERB_GET, CString(pszPath));
			if (pclsHttpFile)
			{
				// HTTP ��û header �� �����Ѵ�.
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
	//VS���� �� ������Ʈ�� �����Ͽ� WebView2�� ��ġ�ϸ� HKEY_CURRENT_USER�� ����� �ݿ��ǰ�
	//urlscheme�� ���ؼ� KoinoAVCSupporter.exe -> WebView2�� ��ġ�ϸ� HKEY_LOCAL_MACHINE�� ����� ���´�.
	//webview2�� uninstall�ϸ�(Microsoft Edge WebView2 ��Ÿ��) �� �� ��� ���ŵȴ�.
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

	//logWrite(_T("WebView2 Runtime installed successfully."));
	Wait(1000);

	//HANDLE hProcess = NULL;
	HWND hWnd = NULL;
	int wait_count = 0;
	//��ġ�� �Ϸ�Ǹ� ��ġ�� ���õ� â�� ��� �ݾ��ش�.
	do
	{
		hWnd = GetHWndByExeFilename(_T("MicrosoftEdgeWebView2RuntimeInstallerX64.exe"));
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
		hWnd = GetHWndByExeFilename(_T("MicrosoftEdgeUpdate.exe"));
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
		AfxMessageBox(_T("ó�� �ڵ尡 �߰����� ���� ����. �ڵ� ���� �ʿ�"));
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


CRequestUrlParams::CRequestUrlParams(CString _full_url, CString _method, bool _is_https, std::deque<CString>* _headers, CString _body, CString _local_file_path)
{
	full_url = _full_url;
	
	parse_url(full_url, ip, port, sub_url, _is_https);
	is_https = _is_https;

	CRequestUrlParams(ip, port, sub_url, _method, is_https, _headers, _body, _local_file_path);
}
