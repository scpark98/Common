#include <afxinet.h>	// for Internet
#include <string>
#include <filesystem>
#include <imm.h>
#include <comutil.h>	//for _bstr_t
#include <TlHelp32.h>	//for CreateToolhelp32Snapshot
#include "Psapi.h"		//for GetCurrentMemUsage()

#include "Functions.h"

#include <limits>
//#include "SystemInfo.h"
#pragma comment(lib, "Psapi.lib")

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

CString	GetNextIndexFile(CString sCurrentFile, bool bNext /* = TRUE */)
{
	if (sCurrentFile.GetLength() == 0)
		return _T("");
	
	// Ex #1 :	Filename-cd1.avi
	//			Filename-cd2.avi
	// Ex #2 :	Filename-cd1-postfix.avi
	//			Filename-cd2-postfix.avi
	// Ex #3 :	다모 01 0728.avi
	//			다모 02 0728.avi
	
	// 같은 폴더안에 존재하는 같은 확장자의 파일들을 나열하여 현재 항목의 다음 항목이
	// 다음 인덱스일 가능성이 매우 높다.
	// 단, 다음 인덱스의 파일이 존재한다는 가정하에서 찾는 함수이다.
	// 다음 인덱스의 파일이 존재하지 않으면 ""을 리턴한다.
	CString		sNextFile = _T("");
	CString		extension = GetFileExtension(sCurrentFile);
	
	int			i;
	int			j;
	int			nTotal = 0;
	CString		filename;
	CString		sFiles[1000];
	CString		sTemp;
	CString		sCurrentPath = sCurrentFile.Left(sCurrentFile.ReverseFind('\\'));
	CFileFind	FileFind;
	bool		bWorking;
	
	if (sCurrentPath.Right(1) != "\\")
		sCurrentPath += "\\";
	
	bWorking = FileFind.FindFile(sCurrentPath + _T("*.") + extension);
	
	while (bWorking)
	{
		bWorking = FileFind.FindNextFile();
		
		if (!FileFind.IsDots() && !FileFind.IsDirectory())
		{
			filename = FileFind.GetFilePath();
			if (GetFileExtension(filename) == extension)
				sFiles[nTotal++] = filename;
		}
	}
	
	if (nTotal < 2)
		return _T("");
	
	for (i = 0; i < nTotal - 1; i++)
		for (j = 0; j < nTotal - 1 - i; j++)
		{
			if (sFiles[j] > sFiles[j+1])
			{
				sTemp		= sFiles[j];
				sFiles[j]	= sFiles[j+1];
				sFiles[j+1]	= sTemp;
			}
		}
		
		if (bNext)
		{
			for (i = 0; i < nTotal; i++)
			{
				if ((i < nTotal - 1) &&									// 마지막 파일이 아니고
					(sFiles[i] == sCurrentFile) &&							// 현재파일과 이름이 같고
					(sFiles[i].GetLength() == sFiles[i+1].GetLength()))	// 현재파일과 길이가 같다면
				{
					int nDiffer = 0;
					CString s1 = sFiles[i];
					CString s2 = sFiles[i+1];
					s1.MakeLower();
					s2.MakeLower();
					
					for (int j = 0; j < s1.GetLength(); j++)
					{
						if (s1.GetAt(j) != s2.GetAt(j))
							nDiffer++;
					}
					
					if (nDiffer < 5)
						return sFiles[i+1];
				}
			}
		}
		else
		{
			for (i = 0; i < nTotal; i++)
			{
				if ((i > 0) &&											// 처음 파일이 아니고
					(sFiles[i] == sCurrentFile) &&						// 현재파일과 타이틀이 같고
					(sFiles[i].GetLength() == sFiles[i-1].GetLength()))	// 현재파일과 길이가 같다면
				{
					int nDiffer = 0;
					CString s1 = sFiles[i];
					CString s2 = sFiles[i-1];
					s1.MakeLower();
					s2.MakeLower();
					
					for (int j = 0; j < s1.GetLength(); j++)
					{
						if (s1.GetAt(j) != s2.GetAt(j))
							nDiffer++;
					}
					
					if (nDiffer < 5)
						return sFiles[i-1];
				}
			}
		}
		
		return _T("");
}

CString GetDateStringFromTime(CTime t, CString sMark /*= _T("-")*/)
{
	CString str;

	str.Format(_T("%d%s%02d%s%02d"), t.GetYear(), sMark, t.GetMonth(), sMark, t.GetDay());
	return str;
}

CString GetDateStringFromTime(COleDateTime t, CString sMark /*= _T("-")*/)
{
	CString str;

	str.Format(_T("%d%s%02d%s%02d"), t.GetYear(), sMark, t.GetMonth(), sMark, t.GetDay());
	return str;
}

CString GetDateStringFromTime(__timeb32 t, CString sMark /*= _T("-")*/)
{
	CTime	ct(t.time);
	return GetDateStringFromTime(ct, sMark);
}

CString GetTimeStringFromTime(CTime t, CString sMark /*=":"*/)
{
	CString str;
	
	str.Format(_T("%02d%s%02d%s%02d"), t.GetHour(), sMark, t.GetMinute(), sMark, t.GetSecond());
	return str;
}

CString GetTimeStringFromTime(COleDateTime t, CString sMark /*= _T("-")*/)
{
	CString str;

	str.Format(_T("%02d%s%02d%s%02d"), t.GetHour(), sMark, t.GetMinute(), sMark, t.GetSecond());
	return str;
}

CString GetTimeStringFromTime(__timeb32 t, CString sMark /*=":"*/)
{
	CTime	ct(t.time);
	return GetTimeStringFromTime(ct, sMark);
}

//type 0(date), 1(time), 2(date+time), 년-월-일 시:분:초 형식으로 현재 시간 리턴. mid_char는 날짜와 시간 사이 문자
CString GetCurrentDateTimeString(int nType, bool bSeparator /*= true*/, TCHAR mid_char /*= ' '*/)
{
	CString str = _T("");
	CTime	t = CTime::GetCurrentTime();
	CString sDate = (bSeparator ? GetDateStringFromTime(t) : GetDateStringFromTime(t, _T("")));
	CString sTime = (bSeparator ? GetTimeStringFromTime(t) : GetTimeStringFromTime(t, _T("")));

	if (nType == 0)
		return sDate;
	else if (nType == 1)
		return sTime;
	
	return sDate + mid_char + sTime;
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

CString GetTimeString(COleDateTime t, bool bSeparator /*= true*/)
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
CString	GetTimeString(__timeb32 t, bool bSeparator /*= true*/, bool bUnderline /*= false*/, bool bHasMilliSec /*= true*/)
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
CString	GetCurrentTimeString(bool bSeparator, bool bUnderline /*= false*/, bool bHasMilliSec /*= true*/)
{
	__timeb32	tCur;

	_ftime32(&tCur);
	return GetTimeString(tCur, bSeparator, bUnderline, bHasMilliSec);
}

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

CString GetDateTimeStringFromTime(CTime t, bool bSeparator /*= true*/)
{
	CString str;

	str.Format(_T("%s %s"), GetDateStringFromTime(t), GetTimeStringFromTime(t));

	if (!bSeparator)
	{
		str.Remove('-');
		str.Remove('/');
		str.Remove(':');
	}

	return str;
}

CString GetDateTimeStringFromTime(COleDateTime t, bool bSeparator /*= true*/)
{
	CString str;

	str.Format(_T("%s %s"), GetDateStringFromTime(t), GetTimeStringFromTime(t));

	if (!bSeparator)
	{
		str.Remove('-');
		str.Remove('/');
		str.Remove(':');
	}

	return str;
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
		dqFind = GetTokenString(strFind, _T("&"));
	}
	else if (strFind.Find(_T("|")) >= 0)
	{
		op = '|';
		dqFind = GetTokenString(strFind, _T("|"));
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
		case 1 : return RGB( 91, 155, 213);
		case 2 : return RGB(165, 255, 165);
		case 3 : return RGB(255, 192,   0);
		case 4 : return RGB( 68, 114, 196);
		case 5 : return RGB(112, 173,  71);
		case 6 : return RGB( 37,  94, 255);
		case 7 : return RGB(158,  72,  14);
		case 8 : return RGB( 99,  99, 199);
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

CString	GetExeDirectory(bool includeSlash)
{
	TCHAR	sFilePath[1024];
	
	GetModuleFileName(AfxGetInstanceHandle(), sFilePath, MAX_PATH);
	
	return GetFolderNameFromFullPath(sFilePath, includeSlash);
}

CString GetExeRootDirectory()
{
	CString sExePath = GetExeDirectory();
	return sExePath.Left(sExePath.ReverseFind('\\'));
}

CString		GetExeFilename(bool bFullPath /*= FALSE*/)
{
	TCHAR	sFilePath[1024];
	CString sExeFile;
	
	GetModuleFileName(AfxGetInstanceHandle(), sFilePath, MAX_PATH);
	
	if (bFullPath)
		sExeFile = sFilePath;
	else
		sExeFile = GetFileNameFromFullPath(sFilePath);
	
	return sExeFile;
}

CString		GetExeFileTitle()
{
	return GetFileTitle(GetExeFilename(false));
}

CString		GetCurrentDirectory()
{
	TCHAR	sFilePath[1024];
	CString sExeFolder;
	
	GetCurrentDirectory(MAX_PATH, sFilePath);
	
	return sFilePath;
}

CString		GetFileNameFromFullPath(CString fullpath)
{
	fullpath.Replace(_T("/"), _T("\\"));

	if (fullpath.Find(_T("\\")) > 0)
		return fullpath.Right(fullpath.GetLength() - fullpath.ReverseFind('\\') - 1);
	
	return fullpath;
}

CString		GetFolderNameFromFullPath(CString sFullPath, bool includeSlash)
{
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

CString		GetFileExtension(CString sFullPath, bool dot)
{
	// mms://211.106.66.140/H_1TV
	// 위와 같은 웹주소의 경우에는 ""을 리턴한다.
	// http://babobus.com/Html/content/0828_1.asf
	CString sString;
	
	if (CheckFileIsURL(sFullPath))
	{
		sString = GetFileNameFromFullPath(sFullPath);
		// 확장명이 있는 경우와 없는 경우를 구분한다.
		if (sString.Find('.') > 0)
		{
			if (dot)
				sString = sString.Right(sString.GetLength() - sString.ReverseFind('.'));
			else
				sString = sString.Right(sString.GetLength() - sString.ReverseFind('.') - 1);
			return sString;
		}
		else
			return _T("");
	}
	else
	{
		if (dot && sString.Find('.') > 0)
			sString = sFullPath.Right(sFullPath.GetLength() - sFullPath.ReverseFind('.'));
		else
			sString = sFullPath.Right(sFullPath.GetLength() - sFullPath.ReverseFind('.') - 1);
	}
	
	return sString;
}

int	GetFileTypeFromFilename(CString filename)
{
	return GetFileTypeFromExtension(GetFileExtension(filename));
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
bool ChangeExtension(CString& filepath, CString newExt, bool applyRealFile)
{
	CString sOldExt = GetFileExtension(filepath);
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
//c:/abc/def/../../test.txt => c:/abc/def/../../test.txt
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

//폴더에 있는 파일들 중 filetitle이고 extension에 해당하는 파일명을 리턴한다.
std::deque<CString>	get_filename_from_filetitle(CString folder, CString filetitle, CString extension)
{
	return FindFilesWithExtensions(folder, filetitle, extension);
}

std::deque<CString> get_filename_from_filetitle(CString filename, CString extension)
{
	return get_filename_from_filetitle(GetFolderNameFromFullPath(filename), GetFileTitle(filename), extension);
}

uint64_t	GetFileSize(CString sfile)
{
	CFileStatus		status;
	
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
//unit_string	: 단위를 표시할 지 (default = true)
CString		GetFileSizeString(CString sfile, int unit_limit, int floats, bool unit_string)
{
	return GetUnitSizeString(GetFileSize(sfile), unit_limit, floats, unit_string);
}

//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
//floats		: 소수점을 몇 자리까지 표시할지 (default = 0)
//unit_string	: 단위를 표시할 지 (default = true)
//comma			: 정수 부분에 자리수 콤마를 표시할 지 (default = false)
CString		GetUnitSizeString(int64_t size, int unit, int floats, bool unit_string, bool comma)
{
	double dsize = (double)size;
	CString size_str;
	CString unit_str;

	if (/*(dsize > 1024.0) && */(unit > 0))
	{
		dsize /= 1024.0;
		unit_str = _T("KB");
	}

	if (/*(dsize > 1024.0) && */(unit > 1))
	{
		dsize /= 1024.0;
		unit_str = _T("MB");
	}

	if (/*(dsize > 1024.0) && */(unit > 2))
	{
		dsize /= 1024.0;
		unit_str = _T("GB");
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
		size_str += unit_str;

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

bool		CheckFileIsURL(CString sURL)
{
	if (sURL.Find(_T("://")) > 0)
		return TRUE;
	else
		return FALSE;
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
		
		CString filename = GetFileNameFromFullPath(sURL);
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

//원래 double dSec를 사용하여 위의 함수를 사용했으나 소수점 표시 방식의 오류가 존재하므로
//1.0이어야 할 값이 0.999999999998 과 같이 표현되어 위의 함수를 사용할 수 없을 경우가 있다.
//그래서 아예 millisecond값도 int로 간주하여 처리한 아래 함수를 사용한다.
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
	GetTokenString(timeString, dqToken, ':');
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
	else if (sColor ==	"aqua")				return RGB(  0, 255, 255);
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
		GetTokenString(excepts, dqExcepts, ';');
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

//한글로만 구성된 문자열인지
bool IsHangul(CString str)
{
	int len = str.GetLength();
	const wchar_t start_ch = L'가';
	const wchar_t end_ch = L'힣';

	for (int i = 0; i < str.GetLength(); i++)
	{
		if (str[i] < start_ch || str[i] > end_ch)
			return false;
	}

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

	return (StrCmpLogicalW(s0, s1) == 1);
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

//http://localhost:4300/test_doc_favorite/test.avi
CString	get_uri(CString full_remote_url, CString local_path)
{
	int colon = full_remote_url.Find(_T(":"));
	bool is_https = true;

	//http 또는 https에 따라 port 추출을 위한 콜론 위치 추출
	if (full_remote_url.Left(7) == _T("http://"))
	{
		colon = full_remote_url.Find(_T(":"), 8);
		is_https = false;
	}
	else if (full_remote_url.Left(8) == _T("https://"))
	{
		colon = full_remote_url.Find(_T(":"), 9);
	}
	else
	{
		return _T("[error] invalid url.");
	}

	CString ip;
	CString port;
	CString remote_path;
	int slash;
	
	if (colon > 0)
	{
		ip = full_remote_url.Left(colon);
		slash = full_remote_url.Find(_T("/"), colon);
		port = full_remote_url.Mid(colon + 1, slash - colon - 1);
		remote_path = full_remote_url.Mid(slash);
	}
	else
	{
		slash = full_remote_url.Find(_T("//"));
		if (slash > 0)
		{
			slash = full_remote_url.Find(_T("/"), slash);
			ip = full_remote_url.Left(slash);
			remote_path = full_remote_url.Mid(slash);

			if (is_https)
				port = _T("443");
			else
				port = _T("80");
		}
	}

	return get_uri(ip, _ttoi(port), remote_path, local_path);
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

CString	get_uri(CString ip, int port, CString remote_path, CString local_path)
{
	CString result = _T("ok");
	CString logStr;

	try
	{
		HINTERNET hInternet = InternetOpen(_T("get_uri"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (hInternet == NULL)
		{
			return _T("[error] InternetOpen Fail");
		}

		BOOL isHTTPS = FALSE;
		CString strServerIP = ip;

		if (ip.Find(_T("https://")) >= 0)
		{
			isHTTPS = TRUE;
			strServerIP.Replace(_T("https://"), _T(""));
		}
		else if (ip.Find(_T("http://")) >= 0)
		{
			isHTTPS = FALSE;
			strServerIP.Replace(_T("http://"), _T(""));
		}

		HINTERNET hConnect = InternetConnect(hInternet, strServerIP, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		if (hConnect == NULL)
		{
			::InternetCloseHandle(hInternet);
			return _T("[error] InternetConnect Fail");
		}

		int secureFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE; // http
		if (isHTTPS)
		{
			secureFlags = INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID; // https
		}

		/*
		USES_CONVERSION;
		
		wchar_t strUnicode[256] = { 0, };
		char    *strMultibyte = CString2char(remote_path);
		//strcpy_s(strMultibyte, remote_path.GetLength(), remote_path.GetBuffer());
		int nLen = MultiByteToWideChar(CP_ACP, 0, strMultibyte, strlen(strMultibyte), NULL, NULL);
		MultiByteToWideChar(CP_ACP, 0, strMultibyte, strlen(strMultibyte), strUnicode, nLen);

		char strUtf8[256] = { 0, };
		nLen = WideCharToMultiByte(CP_UTF8, 0, strUnicode, lstrlenW(strUnicode), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, strUnicode, lstrlenW(strUnicode), strUtf8, nLen, NULL, NULL);
		
		HINTERNET hRequest = HttpOpenRequest(hConnect, _T("GET"), unicodeToUtf8(strUnicode).c_str(), NULL, NULL, NULL, secureFlags, 0);
		//HINTERNET hRequest = HttpOpenRequest(hConnect, _T("GET"), strUtf8, NULL, NULL, NULL, secureFlags, 0);
		//HINTERNET hRequest = HttpOpenRequest(hConnect, _T("GET"), CT2A(remote_path, CP_UTF8), NULL, NULL, NULL, secureFlags, 0);
		*/
		HINTERNET hRequest = HttpOpenRequest(hConnect, _T("GET"), remote_path, NULL, NULL, NULL, secureFlags, 0);

		//delete[] strMultibyte;

		if (hRequest == NULL)
		{
			::InternetCloseHandle(hConnect);
			hConnect = NULL;

			::InternetCloseHandle(hInternet);
			hInternet = NULL;

			return _T("[error] HttpOpenRequest Fail");
		}
		
		//HttpAddRequestHeaders(hRequest, _T("Content-Type: application/json; charset=utf-8"), -1, HTTP_ADDREQ_FLAG_ADD);
		HttpAddRequestHeaders(hRequest, _T("Content-Type: application/json"), -1, HTTP_ADDREQ_FLAG_ADD);

		if (isHTTPS)
		{
			DWORD dwFlags;
			DWORD dwBuffLen = sizeof(dwFlags);
			InternetQueryOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwBuffLen);
			dwFlags |= SECURITY_FLAG_SECURE;
			dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
			dwFlags |= SECURITY_FLAG_IGNORE_REVOCATION;
			dwFlags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
			dwFlags |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
			InternetSetOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
		}

		BOOL res = HttpSendRequest(hRequest, NULL, 0, NULL, 0);
		if (!res)
		{
			InternetCloseHandle(hRequest);
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hInternet);
			return _T("[error] HttpSendRequest Fail");
		}

		HANDLE hFile = NULL;

		//로컬 파일이 저장될 폴더가 존재하지 않으면 생성해준다.
		if (!local_path.IsEmpty())
		{
			CString folder = GetFolderNameFromFullPath(local_path);
			make_full_directory(folder);

			hFile = CreateFile(local_path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				InternetCloseHandle(hRequest);
				InternetCloseHandle(hConnect);
				InternetCloseHandle(hInternet);

				return local_path + _T("\n\nCan't create file.");
			}
		}

		int buf_size = 8192;
		char *chBufAll = new char[buf_size * buf_size];
		char *chBuf = new char[buf_size];
		DWORD dwSize = 0;
		DWORD dwBytesWrite = 0;

		memset(chBuf, 0, buf_size);
		memset(chBufAll, 0, buf_size);

		int cnt = 0;
		while (InternetReadFile(hRequest, chBuf, buf_size, &dwSize))
		{
			if (dwSize == 0)
				break;

			if (local_path.IsEmpty())
			{
				TRACE(_T("%s"), chBuf);

				for (int i = 0; i < dwSize; i++)
				{
					chBufAll[cnt] = chBuf[i];
					chBuf[i] = 0;
					cnt++;
				}
			}
			else
			{
				if (!WriteFile(hFile, chBuf, dwSize, &dwBytesWrite, NULL))
				{
					CloseHandle(hFile);

					InternetCloseHandle(hRequest);
					InternetCloseHandle(hConnect);
					InternetCloseHandle(hInternet);

					return local_path + _T("\n\nCan't write file.");
				}
				TRACE(_T("%d bytes written.\n"), dwBytesWrite);
			}
		}

		if (local_path.IsEmpty())
		{
			result = UTF8toCString(chBufAll);
		}
		else
		{
			CloseHandle(hFile);
			result = _T("ok");
		}

		delete[] chBuf;
		delete[] chBufAll;

		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);

		return result;
	}
	catch (...)
	{
		return _T("");
	}
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

LONG GetRegistryValue(HKEY hKeyRoot, CString sSubKey, CString sEntry, int *value)
{
	HKEY	hkey = NULL;
	DWORD	dwType;
	DWORD	cbData;
	//LPVOID	lpMsgBuf;
	
	LONG nError = RegOpenKeyEx(hKeyRoot, sSubKey, 0, KEY_ALL_ACCESS, &hkey);

	if (nError == ERROR_SUCCESS)
	{
		if (hkey)
		{
			nError = RegGetValue(hKeyRoot, sSubKey, sEntry, RRF_RT_DWORD, &dwType, value, &cbData);
			
			if (nError != ERROR_SUCCESS)
			{
				/*
				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								nError,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL);
				//AfxMessageBox((LPCTSTR)lpMsgBuf, MB_ICONERROR);
				LocalFree(lpMsgBuf);
				*/
			}
		}
	}
	else
	{
		/*
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						nError,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL);
		//AfxMessageBox((LPCTSTR)lpMsgBuf, MB_ICONERROR);
		LocalFree(lpMsgBuf);
		*/
	}
	
	RegCloseKey(hkey);
	
	return nError;
}

LONG GetRegistryString(HKEY hKeyRoot, CString sSubKey, CString entry, CString *str)
{
	HKEY	hkey = NULL;
	DWORD	dwType;
	DWORD	dwBytes=1000;
	BYTE	buffer[1000];
	LPVOID	lpMsgBuf;
	
	LONG nError = RegOpenKeyEx(hKeyRoot, sSubKey, 0, KEY_ALL_ACCESS, &hkey);

	if (nError == ERROR_SUCCESS)
	{
		if (hkey)
		{
			nError = RegQueryValueEx(hkey, entry, NULL, &dwType, buffer, &dwBytes);
			
			if (nError != ERROR_SUCCESS)
			{
				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
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
		}
	}
	else
	{
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
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
	
	*str = CString(buffer);
	return nError;
}

LONG SetRegistryValue(HKEY hKeyRoot, CString sSubKey, CString entry, int value)
{
	HKEY	hkey;
	DWORD	dwDesc;
	DWORD	lResult = 0;

	RegOpenKeyEx(hKeyRoot, sSubKey, 0, KEY_WRITE, &hkey);

	lResult = RegCreateKeyEx(hKeyRoot, sSubKey, 0,
		NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwDesc);

	if (lResult == ERROR_SUCCESS)
		lResult = RegSetValueEx(hkey, entry, NULL, REG_DWORD, (LPBYTE)&value, sizeof(int));

	RegCloseKey(hkey);

	return lResult;
}

LONG SetRegistryString(HKEY hKeyRoot, CString sSubKey, CString entry, CString str)
{
	HKEY	hkey;
	LONG	lResult;
	DWORD	dwDesc;
	TCHAR	buffer[1000];
	DWORD	dwReturn;
	
	_tprintf(buffer, "%s\0", str);
	
	RegOpenKeyEx(hKeyRoot, sSubKey, 0, KEY_WRITE, &hkey);
	
	lResult = RegCreateKeyEx(hKeyRoot, sSubKey, 0,
		buffer, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwDesc);
	
	if (lResult == ERROR_SUCCESS)
		dwReturn = RegSetValueEx(hkey, entry, NULL, REG_SZ, (BYTE*)buffer, _tcslen(buffer) + 1);
	
	RegCloseKey(hkey);

	if (dwReturn != ERROR_SUCCESS)
		return false;

	return true;
}


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
	bmpInfo.bmiHeader.biWidth			= MAKE4WIDTH_U(w);
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
	CString fileTitle = GetFileTitle(sCurrentFile);
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
		return GetFileNameFromFullPath(sMostRecentFile);
	else if (nReturnType == 2)
		return GetFileTitle(sMostRecentFile);
	
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

void GetNetworkInformation(TCHAR* sFindDescription, NETWORK_INFO* pInfo)
{
	bool				bResult = FALSE;
	IP_ADAPTER_INFO*	p1 = NULL;
	IP_ADAPTER_INFO*	p = NULL;
	ULONG				len = 0;

	USES_CONVERSION;

	if (GetAdaptersInfo(NULL, &len) == ERROR_BUFFER_OVERFLOW)
	{
		p1 = new IP_ADAPTER_INFO[ len ];
		GetAdaptersInfo(p1, &len);

		//p를 사용하지 않고 직접 p1을 이용하여 아래 strcpy를 수행하면 debug heap에러가 발생한다.
		p = p1;

 		while (p)
		{
			_tcscpy(pInfo->sDescription, A2T(p->Description));

			if (_tcsstr(pInfo->sDescription, sFindDescription) != NULL)
			{
				_tcscpy(pInfo->sIPAddress, A2T(p->IpAddressList.IpAddress.String));
				_tcscpy(pInfo->sGateway, A2T(p->GatewayList.IpAddress.String));
				_tcscpy(pInfo->sSubnetMask, A2T(p->IpAddressList.IpMask.String));
				_tprintf(pInfo->sMacAddress, "%02X-%02X-%02X-%02X-%02X-%02X",
						p->Address[0], p->Address[1], p->Address[2], p->Address[3], p->Address[4], p->Address[5]);
				break;
			}

			p = p->Next;
		}
	}

	delete[] p1;
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
		
		if (bMatchWholeWord)
		{
			if (sCaptionString == sCaption)
				return pWnd;
		}
		else
		{
			TRACE(_T("%s\n"), sCaptionString);
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
LONGLONG GetDiskFreeSize(CString sDrive)
{
	TCHAR Drive[10];
	ULARGE_INTEGER	m_lFreeBytesAvailableToCaller;    
	ULARGE_INTEGER	m_lTotalNumberOfBytes;
	ULARGE_INTEGER	m_lTotalNumberOfFreeBytes;
	
	if (sDrive.GetLength() == 1)
		_stprintf(Drive, _T("%s:\\"), sDrive);
	else
		_stprintf(Drive, _T("%s"), sDrive);
	
	int res =  GetDiskFreeSpaceEx(Drive, 
		&m_lFreeBytesAvailableToCaller, 
		&m_lTotalNumberOfBytes, 
		&m_lTotalNumberOfFreeBytes);

	return (LONGLONG)(m_lTotalNumberOfFreeBytes.QuadPart);
}

LONGLONG GetDiskTotalSize(CString sDrive)
{
	TCHAR			Drive[10];
	ULARGE_INTEGER	m_lFreeBytesAvailableToCaller;    
	ULARGE_INTEGER	m_lTotalNumberOfBytes;
	ULARGE_INTEGER	m_lTotalNumberOfFreeBytes;
	
	if (sDrive.GetLength() == 1)
		_stprintf(Drive, _T("%s:\\"), sDrive);
	else
		_stprintf(Drive, _T("%s"), sDrive);
	
	int res =  GetDiskFreeSpaceEx(Drive, 
		&m_lFreeBytesAvailableToCaller, 
		&m_lTotalNumberOfBytes, 
		&m_lTotalNumberOfFreeBytes);

	return (LONGLONG)(m_lTotalNumberOfBytes.QuadPart);
}

CString	GetDiskSizeString(CString sDrive)
{
	CString str;

	str.Format(_T("%.1f GB / %.1f GB"),
				GetDiskFreeSize(sDrive) / 1024.0 / 1024.0 / 1024.0,
				GetDiskTotalSize(sDrive) / 1024.0 / 1024.0 / 1024.0);
	return str;
}

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
    ImmSetConversionStatus(himc, (bNative ? IME_CMODE_NATIVE : IME_CMODE_ALPHANUMERIC),IME_SMODE_CONVERSATION);
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

void sort_like_explorer(std::deque<CString> *dq, bool compare_only_filename)
{
	bool only_filename = compare_only_filename;
	std::sort(dq->begin(), dq->end(),
		[only_filename](CString a, CString b)
	{
		if (only_filename)
		{
			a = GetFileNameFromFullPath(a);
			b = GetFileNameFromFullPath(b);
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
		sFilename = GetFileNameFromFullPath(sfile).MakeLower();
		
		if (finder.IsDots())
			continue;
		else if (finder.IsDirectory())
		{
			if (bRecursive)
				FindAllFiles(sfile, dqFiles, sNameFilter, sExtFilter, bRecursive, sExceptStr, auto_sort);
		}
		else
		{
			//sFilter에 해당하는 파일만을 검색 대상으로 한다.
			bool bFound = false;

			if (dqExtFilter.size() == 0)
			{
				bFound = true;
			}
			else
			{
				//예를들어 확장자 목록에 jpg가 있는데 파일명 중간에서 jpg가 발견되면 문제가 된다.
				if (find_index(&dqExtFilter, GetFileExtension(sFilename).MakeLower()) >= 0)
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
				//제외할 문자열이 포함되어 있지 않은 파일들만 리스트에 추가한다.
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
			filetitle = GetFileTitle(file).MakeLower();

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
					if (GetFileExtension(file, true).MakeLower() == dq_ext[i])
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
			filetitle = GetFileTitle(file).MakeLower();

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
					if (GetFileExtension(file, true).MakeLower() == dq_ext[i])
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

void save_dqlist(std::deque<CString>* dqlist, CString path)
{
	int i;
	FILE* fp;

	if ((fp = _tfopen(path, _T("wt")CHARSET)) == NULL)
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

	GetTokenString(extensions, dqToken, ';');

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


//시작폴더 및 하위 폴더들은 여전히 남아있다.
//폴더 통째로 다 지우려면 코드의 수정이 필요하다.
int DeleteAllFiles(CString sFolder, CString sFilter, bool bRecursive/*=TRUE*/, bool bResetCount /*= TRUE*/)
{
	static int	nDeleteCount = 0;
	CString		sfile, str1, str2;
	CFileFind	finder;

	if (bResetCount)
		nDeleteCount = 0;

	bool bWorking = finder.FindFile(sFolder + "\\*");

	sFilter.MakeLower();

	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		sfile = finder.GetFilePath();
		sfile.MakeLower();
		
		if (finder.IsDots())
			continue;
		else if (finder.IsDirectory())
		{
			if (bRecursive)
				DeleteAllFiles(sfile, sFilter, bRecursive, FALSE);
		}
		else
		{
			if (sFilter == "*" || sFilter == "*.*")
			{
				if (delete_file(sfile, true))
				{
					TRACE("delete ok : %s\n", sfile);
					nDeleteCount++;
				}
				else
				{
					TRACE("delete fail : %s\n", sfile);
				}
			}
			else
			{
				//특정 확장자 파일들을 지우는 경우
				if (sFilter.Left(2) == "*.")
				{
					str1 = GetFileExtension(sfile);
					str2 = sFilter.Right(sFilter.GetLength() - sFilter.ReverseFind('.') - 1);
				}
				//특정 파일명 파일들을 지우는 경우
				else if (sFilter.Right(2) == ".*")
				{
					str1 = GetFileTitle(sfile);
					str2 = sFilter.Left(sFilter.ReverseFind('.'));
				}
				else
				{
					str1 = GetFileNameFromFullPath(sfile);
					str2 = sFilter;
				}

				str1.MakeLower();
				str2.MakeLower();

				if (str1 == str2)
				{
					if (delete_file(sfile, true))
					{
						TRACE("delete ok : %s\n", sfile);
						nDeleteCount++;
					}
					else
					{
						TRACE("delete fail : %s\n", sfile);
					}
				}
			}
		}
	}

	//RemoveDirectory(sFolder);
	//TRACE("%s\n", sFolder);
	return nDeleteCount;
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
		filename = GetFileNameFromFullPath(sfile);

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
	CString folder(lpPathName);
	return (ERROR_SUCCESS == SHCreateDirectoryEx(NULL, (LPCTSTR)folder, lpsa));
}

//이 함수를 사용하려면 반드시 SetBkMode(TRANSPARENT);로 설정해야 효과가 나타남.
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

//이 함수를 사용하려면 반드시 SetBkMode(TRANSPARENT);로 설정해야 효과가 나타남.
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

//이 함수를 사용하려면 반드시 SetBkMode(TRANSPARENT);로 설정해야 효과가 나타남.
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

//이 함수를 사용하려면 반드시 SetBkMode(TRANSPARENT);로 설정해야 효과가 나타남.
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

      pdc->DrawText(strText, &rcSavedRect,  DT_CALCRECT | DT_WORDBREAK | DT_CENTER | DT_VCENTER | DT_NOCLIP);

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


void DrawLinePt(CDC* pDC, CPoint pt1, CPoint pt2, COLORREF crColor /*= 0*/, int nWidth /*= 1*/, int nPenStyle /*= PS_SOLID*/, int nDrawMode /*= R2_COPYPEN*/)
{
	DrawLine(pDC, pt1.x, pt1.y, pt2.x, pt2.y, crColor, nWidth, nPenStyle, nDrawMode);
}

void DrawLine(CDC* pDC, int x1, int y1, int x2, int y2, COLORREF crColor /*= 0*/, int nWidth /*= 1*/, int nPenStyle /*= PS_SOLID*/, int nDrawMode /*= R2_COPYPEN*/)
{
	LOGBRUSH lb;

	lb.lbStyle = BS_SOLID;
	lb.lbColor = crColor;

	CPen	Pen(PS_GEOMETRIC | nPenStyle, nWidth, &lb);
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

//윈도우 운영체제에서 특정 폴더의 실제 경로를 리턴한다.
//nFolder = https://docs.microsoft.com/ko-kr/windows/win32/shell/csidl
//http://blog.naver.com/PostView.nhn?blogId=sobakmt&logNo=60058711792&widgetTypeCall=true
CString get_special_folder(int nFolder)
{
	LPITEMIDLIST itemList = nullptr;
	wchar_t szPath[MAX_PATH] = {0, };

	SHGetSpecialFolderLocation(nullptr, nFolder, &itemList);
	SHGetPathFromIDListW(itemList, szPath);

	CoTaskMemFree(itemList);

	return szPath;
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
CString GetFileVersionInformation(CString strFileName, CString strFlag)
{
	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	DWORD dwSize = GetFileVersionInfoSize(strFileName , 0);
	TCHAR * buffer = new TCHAR[dwSize];
	memset(buffer, 0, dwSize);

	GetFileVersionInfo(strFileName, 0, dwSize, buffer); 

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

	FindAllFiles(folder, &files, names, _T("*"), bRecursive, _T(""), false);

	for (int i = 0; i < files.size(); i++)
	{
		//검색된 파일명에서 oldName 다음에 . _Snapshot 이 두개의 패턴은 같은 파일군으로 처리한다.
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
				get_error_message(GetLastError(), true);
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
CString GetToken(CString src, CString separator, int n)
{
	int i = 0;
	CStringArray saItems;

	for (CString sItem = src.Tokenize(separator,i); i >= 0; sItem = src.Tokenize(separator,i))
	{
		saItems.Add(sItem);
	}

	if (n < 0 || n >= saItems.GetSize())
		return _T("");

	return saItems.GetAt(n);
}

std::deque<CString>	GetTokenString(CString src, CString separator)
{
	int i = 0;
	std::deque<CString> dq;
	for (CString sItem = src.Tokenize(separator,i); i >= 0; sItem = src.Tokenize(separator,i))
		dq.push_back(sItem);

	return dq;
}

int GetTokenString(CString src, std::deque<CString> &dqToken, TCHAR separator, bool allowEmpty, int nMaxToken /*= -1*/)
{
	std::deque<TCHAR> dqSeparator;
	
	dqSeparator.push_back(separator);
	return GetTokenString(src, dqToken, dqSeparator, allowEmpty, nMaxToken);
	/*
	dqToken.clear();

	int i = 0;
	for (CString token = src.Tokenize(separator,i); i >= 0; token = src.Tokenize(separator,i))
	{
		if (!token.IsEmpty() || allowEmpty)
			dqToken.push_back(token);
		if ((nMaxToken > 0) && (dqToken.size() == nMaxToken))
			dqToken.size();
	}

	return dqToken.size();
	*/
}

int GetTokenString(CString src, std::deque<CString>& dqToken, std::deque<TCHAR> separator, bool allowEmpty, int nMaxToken)
{
	int i, j;
	CString token;

	dqToken.clear();

	for (i = 0; i < src.GetLength(); i++)
	{
		bool found = false;
		for (j = 0; j < separator.size(); j++)
		{
			if (src[i] == separator[j])
			{
				found = true;
				if (!token.IsEmpty() || allowEmpty)
					dqToken.push_back(token);
				if (nMaxToken > 0 && dqToken.size() == nMaxToken)
					return dqToken.size();
				token.Empty();
				break;
			}
		}

		if (!found)
		{
			token += src[i];
		}
	}

	if (!token.IsEmpty())
		dqToken.push_back(token);

	return dqToken.size();
}


//nMaxToken 숫자 만큼 토큰을 분리해서 sToken에 넣고
//실제 처리된 토큰 개수를 리턴한다.
//src는 토큰이 분리될 때마다 줄어든다.
//이는 CString::Tokenize() 또는 AfxExtractSubString()과는 다르다.
//(seps = " ,\t\n" 과 같이 분리기호들로 이루어진 스트링 데이터)
//strtok_s 함수는 연속 공백이나 연속 쉼표 등 중복된 분리자는 모두 하나의 구분자로 취급된다.
//ex. "1,, ,,2" = "1,2"
int getTokenString(TCHAR *src, TCHAR *seps, CString *sToken, int nMaxToken)
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

int getTokenString(char *src, char *seps, char **sToken, int nMaxToken)
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

CString	get_tokenized(std::deque<CString> dq, TCHAR separator)
{
	CString res = _T("");

	for (int i = 0; i < dq.size(); i++)
		res = res + dq[i] + separator;

	return res;
}

CString get_str(CString& buff, CString sep)
{
	buff.TrimLeft();

	int pos = buff.Find(sep);
	if(pos < 0)
	{
		pos = buff.GetLength();
		if(pos < 1) throw 1;
	}

	CString ret = buff.Left(pos);
	if (pos < buff.GetLength())
		buff = buff.Mid(pos + 1);

	return(ret);
}

int get_int(CString& buff, CString sep)
{
	CString str;

	str = get_str(buff, sep);
	str.MakeLower();

	CString fmtstr = str.GetLength() > 2 && (str.Left(2) == _T("&h") || str.Left(2) == _T("0x"))
		? str = str.Mid(2), _T("%x")
		: _T("%d");

	int ret;
	if(_stscanf(str, fmtstr, &ret) != 1) throw 1;

	return(ret);
}

double get_double(CString& buff, CString sep)
{
	CString str;

	str = get_str(buff, sep);
	str.MakeLower();

	float ret;
	if(_stscanf(str, _T("%f"), &ret) != 1) throw 1;

	return((double)ret);
}

//맨 마지막 인자는 반드시 NULL을 넣어줘야 끝을 알 수 있다.
bool isOneOf(LPCTSTR src, ...)
{
	va_list vl;
	LPCTSTR arg;

	if (src == _T(""))
		return false;

	va_start(vl, src);
	while ((arg = va_arg(vl, LPCTSTR)))
	{
		if (arg == NULL)
			break;

		if (_tcscmp(arg, src) == 0)
		{
			va_end(vl);
			return true;
		}
	}

	va_end(vl);

	return false;
}

//src 문자열에 set_of_keyword에 나열된 단어가 있는지 검사.
//set_of_keyword는 세미콜론으로 구분해서 여러 문자 또는 문자열을 넣을 수 있다.
//ex. src = "abcd1234"일 때 set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
bool is_exist_keyword(CString src, CString set_of_keyword, bool case_sensitive, bool whole_word)
{
	std::deque<CString> dqKeyword;

	GetTokenString(set_of_keyword, dqKeyword, ';');

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


SIZE_T GetCurrentMemUsage()
{
	DWORD dwpid = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |	PROCESS_VM_READ, FALSE, dwpid);
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

//경로에 '\\' 또는 '/'가 혼용되어 사용되는 경우가 있으므로 이를 감안해야 한다.
//c:\\folder1\\folder2\\	=> c:\\folder1
//c:\\folder1\\folder2		=> c:\\folder1
//c:\\folder1\\				=> c:\\
//c:\\folder1				=> c:\\

CString	GetParentDirectory(CString sFolder)
{
	//"/"와 "\\"가 혼용된 경우 통일시킨다.
	sFolder.Replace(_T("/"), _T("\\"));

	//sFolder 끝에 "\\"가 붙었다면 제거해준다.
	if (sFolder.Right(1) == _T("\\"))
		sFolder = sFolder.Left(sFolder.GetLength() - 1);

	//만약 구해진 폴더가 "c:"와 같이 드라이브 루트라면 끝에 "\\"를 다시 붙여줘야한다.
	if (sFolder.GetLength() == 2 && sFolder.GetAt(1) == ':')
	{
		sFolder += "\\";
		return sFolder;
	}

	//해당 폴더의 parent 폴더명을 구한다.
	sFolder = sFolder.Left(sFolder.ReverseFind('\\'));

	//다시 구해진 폴더가 "c:"와 같이 드라이브 루트라면 끝에 "\\"를 다시 붙여줘야한다.
	if (sFolder.GetLength() == 2 && sFolder.GetAt(1) == ':')
	{
		sFolder += "\\";
		return sFolder;
	}

	return sFolder;
}

//MAX_COMPUTERNAME_LENGTH(15) 길이까지만 리턴됨에 주의.
//GetComputerName API 함수는 항상 대문자로 리턴한다.
//그냥 확실하게 GetComputerNameString().MakeLower() 등과 같이
//대소문자를 명확히 하여 비교하는 것이 좋다.
CString GetComputerNameString()
{
// 	char	sComputerName[MAX_COMPUTERNAME_LENGTH + 1];
// 	DWORD	nSize = MAX_COMPUTERNAME_LENGTH + 1;
// 	GetComputerName(sComputerName, &nSize);
// 
// 	return sComputerName;
	TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(computerName) / sizeof(computerName[0]);
	GetComputerName(computerName, &size);
	return computerName;
}
/*
//여러곳에 자주 사용하는 Functions.h에 아래 함수때문에 매번 WQL.h를 포함시키기가 그래서
//이 기능은 거의 사용하지 않으므로 주석처리하고 필요한 경우에만 사용하자.
#include "WQL.h"

CString GetHDDSerialNumber(int nPhysicalDrive)
{
	CWQL	myWQL;
	CString sHDDSerialNumber = "";

	if (!myWQL.connect(WMI_LOCALHOST, WMI_DEFAULTNAME, WMI_DEFAULTPW))
	{
		AfxMessageBox("WQL connection failed!");
		return "";
	}

	CWQL::RowSet rs;

	if (!myWQL.getClassProperties( L"Win32_DiskDrive", rs))
	{
		AfxMessageBox("WQL Execute failed! ");
		return "";
	}

	TCHAR szBiosSN[32 + 1];
	ZeroMemory(szBiosSN, sizeof(szBiosSN));

	sHDDSerialNumber = (rs[nPhysicalDrive][L"SerialNumber"]).c_str();
	sHDDSerialNumber.Trim();

	TCHAR	cTemp;

	//시리얼넘버가 제품에 부착된(ADATA SSD) 시리얼넘버와는 달리 2자리씩 swap되어 있다.
 	for (int i = 0; i <= sHDDSerialNumber.GetLength() - 2; i += 2)
 	{
		cTemp = sHDDSerialNumber.GetAt(i);
		sHDDSerialNumber.SetAt(i, sHDDSerialNumber.GetAt(i + 1));
		sHDDSerialNumber.SetAt(i + 1, cTemp);
	}

	return sHDDSerialNumber;
}
*/

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

//
// CString → std::string
//
std::string CString2string(CString str)
{
	std::string stdStr;
	char* szStr = CString2char(str);
	if (szStr)
	{
		stdStr = szStr;
		delete[] szStr;
	}

	return stdStr;
}

//
// Char → CString
//
CString char2CString(char* chStr, int length)
{
	CString str;

#if defined(UNICODE) || defined(_UNICODE)
	int len;
	BSTR buf;

	if (length < 0)
		len = MultiByteToWideChar(CP_ACP, 0, chStr, strlen(chStr), NULL, NULL);
	else
		len = length;

	buf = SysAllocStringLen(NULL, len);
	MultiByteToWideChar(CP_ACP, 0, chStr, len, buf, len);

	str.Format(_T("%s"), buf);
#else
	str.Format("%s", chStr);
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
	// Process 객체(CPU_INDEX)에서 해당 process 이름을 갖는 프로세스의 CPU 값을 얻는다.
	if(GetCounterValue(CPU_INDEX, CPU_COUNTER, process, &cpuData, newVal) < 0)
		return -1;
	// 획득한 성능 정보를 이용해서 CPU 사용율을 계산
	cpuTime_100n = cpuData->PerfTime100nSec;

	if(isCpuFirst){
		isCpuFirst = false;
		oldCpuVal = newVal;
		oldCpuTime_100n = cpuTime_100n;
	} else {

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

CString	GetDayTimeCountString(CTimeSpan ts, bool bShowZero, bool bIncludeSec)	//ts값을 넘겨 받아 "a일 b시간 c분 d초" 형태로 표시
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

	do {
		if (StrCmp(pe32.szExeFile, processname) == 0)

			return pe32.th32ProcessID;



	} while (Process32Next(hProcessSnap, &pe32));


	CloseHandle(hProcessSnap);

	return false;
}

// 윈도우 핸들로 프로세스 아이디 얻기   
ULONG ProcIDFromWnd(HWND hwnd)
{   
	ULONG idProc;   
	::GetWindowThreadProcessId(hwnd, &idProc);   
	return idProc;   
}

// 프로세스 아이디로 윈도우 핸들 얻기   
HWND GetHWNDbyPID(ULONG pid)
{   
	HWND tempHwnd = ::FindWindow(NULL,NULL); // 최상위 윈도우 핸들 찾기   

	while(tempHwnd != NULL)   
	{   
		//TRACE("tempHwnd = %p\n", tempHwnd);
		if(::GetParent(tempHwnd) == NULL) // 최상위 핸들인지 체크, 버튼 등도 핸들을 가질 수 있으므로 무시하기 위해   
			if(pid == ProcIDFromWnd(tempHwnd))   
				return tempHwnd;   
		tempHwnd = ::GetWindow(tempHwnd, GW_HWNDNEXT); // 다음 윈도우 핸들 찾기   
	}   
	return NULL;
}

//출처: https://smok95.tistory.com/300?category=28201 [Only YOUng:티스토리]
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

//default : bCaseSensitive = false, bExceptThis = true
HWND GetHWndByExeFilename(CString sExeFile, bool bCaseSensitive, bool bExceptThis)
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

	if (Process32First(hSnapShot, &pe)) 
	{
		do 
		{
			//대소문자 구분하지 않고 실행프로그램 이름이 같은지 확인..
			if ((bCaseSensitive && _tcsicmp(pe.szExeFile, sExeFile) == 0) ||
				(!bCaseSensitive && _tcscmp(pe.szExeFile, sExeFile) == 0))
			{
				//자기 자신은 제외하고...
				if (pe.th32ProcessID != GetCurrentProcessId())
				{
					if (OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID) != NULL)
					{
						hWnd = GetHWNDbyPID(pe.th32ProcessID);
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

//<응용 - 프로세스가 실행 중인지를 체크>
bool IsRunning(CString processname)
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

			return true; // 실행중이면 True를 반환

	} while (Process32Next(hProcessSnap, &pe32));


	CloseHandle(hProcessSnap);

	return false; //실행중이 아니면 False를 반환
} 

#if 0
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
#endif

static HWND shWndCurWnd;

BOOL CALLBACK cbfEnumWindowsProc( HWND hWnd, LPARAM lParam)
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
	CString ext = GetFileExtension(filename).MakeLower();
	GUID format = Gdiplus::ImageFormatJPEG;

	if (ext == _T("bmp"))
		format = Gdiplus::ImageFormatBMP;
	else if (ext == _T("png"))
		format = Gdiplus::ImageFormatPNG;
	else
		//jpeg

	imgCapture->Save(filename, format);
	return imgCapture;
}

// Capture screen and create GDI bitmap
// (full-screen when pRect is NULL)
HBITMAP CaptureScreenToBitmap(LPRECT pRect)
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
		if (nX < 0) nX = 0;
		if (nY < 0) nY = 0;
		//if (nX2 > xScrn) nX2 = xScrn;
		//if (nY2 > yScrn) nY2 = yScrn;
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

	// create a bitmap compatible with the screen DC
	hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);

	// select new bitmap into memory DC
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

	// bitblt screen DC to memory DC
	BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY);

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
	hBitmap = CaptureScreenToBitmap(&rectWnd);

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
	hBitmap = CaptureScreenToBitmap(&rectClient);

	// return handle to the bitmap
	return hBitmap;
}

void WriteBMP(HBITMAP bitmap, HDC hDC, LPTSTR filename)
{
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
	if (!GetObject(bitmap, sizeof(BITMAP), (LPSTR)&bmp))
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


HICON LoadIconEx(HINSTANCE hInstance, UINT nID, int cx, int cy /*= 0*/)
{
	if (cy == 0)
		cy = cx;

	//아래 LoadImage 함수를 통해서 아이콘 파일을 불러온 경우
	//프로그램 종료 시 반드시 DestroyIcon을 해줘야 한다.
	//그래서 DestroyIcon 안해도 되는 LR_SHARED를 사용했다.
	return static_cast<HICON>(::LoadImage(hInstance,
		MAKEINTRESOURCE(nID), IMAGE_ICON, cx, cy, LR_SHARED));
}

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
//0 : "1, 2, 3, 4"
//1 : "(1,2) x (3,4)"
//2 : "l = 1, t = 2, r = 3, b = 4"
CString GetRectInfoString(CRect r, int nFormat)
{
	CString str;

	if (nFormat == 2)
		str.Format(_T("l = %d, t = %d, r = %d, b = %d"), r.left, r.top, r.right, r.bottom);
	else if (nFormat == 1)
		str.Format(_T("(%d,%d) x (%d,%d)"), r.left, r.top, r.right, r.bottom);
	else if (nFormat == 3)
		str.Format(_T("(%d,%d) (%d x %d)"), r.left, r.top, r.Width(), r.Height());
	else
		str.Format(_T("%d, %d, %d, %d"), r.left, r.top, r.right, r.bottom);

	return str;
}


void adjustRectRange(int32_t *l, int32_t *t, int32_t *r, int32_t *b, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool retainSize)
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

void AdjustRectRange(CRect& rect, CRect rLimit, bool bRetainSize, bool includeBR)
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

	if (!includeBR)
	{
		if (rect.right == rLimit.right)
			rect.right--;
		if (rect.bottom == rLimit.bottom)
			rect.bottom--;
	}

	//AdjustRectRange(rect, rLimit.left, rLimit.top, rLimit.right, rLimit.bottom, bRetainSize);
}

void AdjustRectRange(CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize)
{
	int32_t l = rect.left;
	int32_t t = rect.top;
	int32_t r = rect.right;
	int32_t b = rect.bottom;
	adjustRectRange(&l, &t, &r, &b, minx, miny, maxx, maxy, bRetainSize);
	rect = CRect(l, t, r, b);
}

//모니터의 한쪽에 붙은 사각형을 새로운 크기로 변경할 경우 붙은 상태를 유지하고 변경할 필요가 있을 경우 사용.
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


//rTarget에 접하는 dRatio인 최대 사각형을 구한다.
//attach_left 등의 옵션을 줄 필요가 있다.
CRect GetRatioRect(CRect rTarget, int w, int h, int attach)
{
	return GetRatioRect(rTarget, (double)w / (double)h, attach);
}

//rTarget에 접하는 dRatio인 최대 사각형을 구한다.
CRect GetRatioRect(CRect rTarget, double dRatio, int attach)
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

//주어진 점들을 포함하는 최대 사각형을 구한다.
CRect get_max_rect(CPoint	*pt, int nPoints)
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
	return minimum(LevenshteinDistance(s, len_s - 1, t, len_t   ) + 1,
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

CString	get_error_message(DWORD errorId, bool show_msgBox)
{
	TCHAR* message = nullptr;
	CString result;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		nullptr,
		errorId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (TCHAR*)&message, 0, nullptr);

	result = message;
	LocalFree(message);

	if (show_msgBox)
		AfxMessageBox(result);

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

void Trace(char* szFormat, ...)
{
    char szTempBuf[2048] ;
    va_list vlMarker ;

    va_start(vlMarker,szFormat) ;
    vsprintf(szTempBuf,szFormat,vlMarker) ;
    va_end(vlMarker) ;

#ifdef _DEBUG
	TRACE(szTempBuf);
#else
    //OutputDebugString(szTempBuf) ;
	printf(szTempBuf);
#endif
}

//main에서 EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); 를 실행하고
//이 파일에 전역변수로 선언된 g_dqMonitor를 이용하면 된다.
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	// 모니터 정보를 가져올 구조체
	MONITORINFOEX mi;
	mi.cbSize = sizeof(MONITORINFOEX);

	// 모니터 핸들을 사용하여 해당 모니터 정보를 가져온다.
	GetMonitorInfo(hMonitor, &mi);

	CString str;

	// 주모니터로 설정된 모니터 정보인지를 체크한다.
	if (mi.dwFlags & MONITORINFOF_PRIMARY) 
	{
		str.Format(_T("hMonitor = %X,  좌표 : [ (%04d, %04d) - (%04d, %04d) ], < Primary-Monitor > %s"), 
					hMonitor, lprcMonitor->left,lprcMonitor->top,lprcMonitor->right,lprcMonitor->bottom, mi.szDevice);
	}
	else
	{
		str.Format(_T("hMonitor = %X,  좌표 : [ (%04d, %04d) - (%04d, %04d) ], %s"), 
					hMonitor, lprcMonitor->left,lprcMonitor->top,lprcMonitor->right,lprcMonitor->bottom, mi.szDevice);
	}

	g_dqMonitors.push_back(lprcMonitor);

	TRACE(_T("%s\n"), str);

	return TRUE;
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

//font size to LOGFONT::lfHeight
LONG get_logical_size_from_font_size(HDC hDC, int font_size)
{
	return -MulDiv(font_size, GetDeviceCaps(hDC, LOGPIXELSY), 72);
}

//LOGFONT::lfHeight to font size
LONG get_font_size_from_logical_size(HDC hDC, int logical_size)
{
	return -MulDiv(logical_size, 72, GetDeviceCaps(hDC, LOGPIXELSY));
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

//OnBnClickedCancel()과 같이 프로그램이 종료될 때 호출한다.
//OnWindowPosChanged()에서 매번 호출했었으나 
//OnInitDialog에서 RestoreWindowPosition를 호출할 때
//OnWindowPosChanged가 호출되면서 maximized 정보가 false로 초기화되어 버린다.
//sSubSection이 존재하면 그 이름에 "\\screen"을 붙여서 저장한다.
void SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection)
{
	if (pWnd->IsWindowVisible() == false ||	pWnd->IsIconic())
		return;

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

void GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, double *x, double *y)
{
	if (srcWidth <= 0)
		return;

	double dZoom = (double)rDisplayedImageRect.Width() / (double)(srcWidth);

	//화면에 표시된 영상의 l,t값을 빼서 확대 영상내에서의 상대좌표로 변환한 후
	*x -= rDisplayedImageRect.left;
	*y -= rDisplayedImageRect.top;

	//확대된 비율로 나누고
	*x /= dZoom;
	*y /= dZoom;
}

void GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, CPoint *pt)
{
	double x = (double)pt->x;
	double y = (double)pt->y;

	GetRealPosFromScreenPos(rDisplayedImageRect, srcWidth, &x, &y);
	pt->x = (long)x;
	pt->y = (long)y;
}

void GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, CRect *r)
{
	double x1 = (double)(r->left);
	double y1 = (double)(r->top);
	double x2 = (double)(r->right);
	double y2 = (double)(r->bottom);

	GetRealPosFromScreenPos(rDisplayedImageRect, srcWidth, &x1, &y1);
	GetRealPosFromScreenPos(rDisplayedImageRect, srcWidth, &x2, &y2);

	r->left = (long)(x1);
	r->top = (long)(y1);
	r->right = (long)(x2);
	r->bottom = (long)(y2);
}

void GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, double *x, double *y)
{
	if (srcWidth <= 0)
		return;

	double dZoom = (double)rDisplayedImageRect.Width() / (double)(srcWidth);

	//확대된 비율을 곱하고
	*x *= dZoom;
	*y *= dZoom;

	//옵셋만큼 상대좌표로 이동시킨다.
	*x += rDisplayedImageRect.left;
	*y += rDisplayedImageRect.top;
}

void GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, CPoint *pt)
{
	double x = (double)pt->x;
	double y = (double)pt->y;

	GetScreenPosFromRealPos(rDisplayedImageRect, srcWidth, &x, &y);
	pt->x = (long)x;
	pt->y = (long)y;
}

void GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, CRect *r)
{
	double x1 = (double)(r->left);
	double y1 = (double)(r->top);
	double x2 = (double)(r->right);
	double y2 = (double)(r->bottom);

	GetScreenPosFromRealPos(rDisplayedImageRect, srcWidth, &x1, &y1);
	GetScreenPosFromRealPos(rDisplayedImageRect, srcWidth, &x2, &y2);

	r->left = (long)(x1);
	r->top = (long)(y1);
	r->right = (long)(x2);
	r->bottom = (long)(y2);
}

int	find_index_from_deque(std::deque <CString> dq, CString str)
{
	for (int i = 0; i < dq.size(); i++)
	{
		if (dq[i] == str)
			return i;
	}

	return -1;
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
	std::deque<CString> token = GetTokenString(versionStr, _T("."));
	if (token.size() == digits)
		return true;

	return false;
}

//그냥 문자열로 비교하면 1.0.9.0이 1.0.10.0보다 더 크다고 나오므로 .을 없앤 숫자로 비교한다.
//리턴값은 strcmp와 동일한 규칙으로 판단한다.(+:ver0가 큼, -:ver1이 큼, 0:같음)
int	compare_version_string(CString ver0, CString ver1, TCHAR separator)
{
	ver0.Remove(separator);
	ver1.Remove(separator);

	int v0 = _ttol(ver0);
	int v1 = _ttol(ver1);

	if (v0 > v1)
		return 1;
	else if (v0 < v1)
		return -1;

	return 0;
}


//button의 종류를 리턴한다.
UINT		getButtonStyle(HWND hWnd)
{
	UINT button_style = BS_PUSHBUTTON;

	DWORD dwStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE); 

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

void trace(LPCTSTR format, ...)
{
#ifndef _DEBUG
	return;
#endif
	va_list args;
	va_start(args, format);

	CString str;
	str.FormatV(format, args);
	va_end(args);

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
					((( ((a&0xFF)*tm
						+ (c&0xFF)*bm)*lm
						) & 0xFF0000) >> 8)

					// green element
					| ((( (((a>>8)&0xFF)*tm
						+ ((c>>8)&0xFF)*bm)*lm
						) & 0xFF0000)) // no need to shift

										// red element
					| ((( (((a>>16)&0xFF)*tm
						+ ((c>>16)&0xFF)*bm)*lm
						) & 0xFF0000) << 8)
					;
			}

			rightOutput =
				// blue element
				((( ((b&0xFF)*tm
					+ (d&0xFF)*bm)*lm
					) & 0xFF0000) >> 8)

				// green element
				| ((( (((b>>8)&0xFF)*tm
					+ ((d>>8)&0xFF)*bm)*lm
					) & 0xFF0000)) // no need to shift

									// red element
				| ((( (((b>>16)&0xFF)*tm
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


//서버의 한글명 파일에 대한 처리때문에 get_uri()함수 대신 추가하여 테스트 해봤으나
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

	//logWrite(LOG_LEVEL_RELEASE, _T("WebView2 Runtime installed successfully."));
	Wait(1000);

	//HANDLE hProcess = NULL;
	HWND hWnd = NULL;
	int wait_count = 0;
	//설치가 완료되면 설치와 관련된 창은 모두 닫아준다.
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
