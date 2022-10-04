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
	// Ex #3 :	´Ù¸ð 01 0728.avi
	//			´Ù¸ð 02 0728.avi
	
	// °°Àº Æú´õ¾È¿¡ Á¸ÀçÇÏ´Â °°Àº È®ÀåÀÚÀÇ ÆÄÀÏµéÀ» ³ª¿­ÇÏ¿© ÇöÀç Ç×¸ñÀÇ ´ÙÀ½ Ç×¸ñÀÌ
	// ´ÙÀ½ ÀÎµ¦½ºÀÏ °¡´É¼ºÀÌ ¸Å¿ì ³ô´Ù.
	// ´Ü, ´ÙÀ½ ÀÎµ¦½ºÀÇ ÆÄÀÏÀÌ Á¸ÀçÇÑ´Ù´Â °¡Á¤ÇÏ¿¡¼­ Ã£´Â ÇÔ¼öÀÌ´Ù.
	// ´ÙÀ½ ÀÎµ¦½ºÀÇ ÆÄÀÏÀÌ Á¸ÀçÇÏÁö ¾ÊÀ¸¸é ""À» ¸®ÅÏÇÑ´Ù.
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
				if ((i < nTotal - 1) &&									// ¸¶Áö¸· ÆÄÀÏÀÌ ¾Æ´Ï°í
					(sFiles[i] == sCurrentFile) &&							// ÇöÀçÆÄÀÏ°ú ÀÌ¸§ÀÌ °°°í
					(sFiles[i].GetLength() == sFiles[i+1].GetLength()))	// ÇöÀçÆÄÀÏ°ú ±æÀÌ°¡ °°´Ù¸é
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
				if ((i > 0) &&											// Ã³À½ ÆÄÀÏÀÌ ¾Æ´Ï°í
					(sFiles[i] == sCurrentFile) &&						// ÇöÀçÆÄÀÏ°ú Å¸ÀÌÆ²ÀÌ °°°í
					(sFiles[i].GetLength() == sFiles[i-1].GetLength()))	// ÇöÀçÆÄÀÏ°ú ±æÀÌ°¡ °°´Ù¸é
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

//type 0(date), 1(time), 2(date+time), ³â-¿ù-ÀÏ ½Ã:ºÐ:ÃÊ Çü½ÄÀ¸·Î ÇöÀç ½Ã°£ ¸®ÅÏ. mid_char´Â ³¯Â¥¿Í ½Ã°£ »çÀÌ ¹®ÀÚ
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

//dqSrc¿¡ dqFind°¡ ÀÖ´ÂÁö °Ë»ç. ÇöÀç´Â AND ¿¬»êÀÌ¹Ç·Î dqFindÀÇ ¸ðµç ¿ø¼Ò°¡ dqSrc¿¡ Æ÷ÇÔµÇ¾î ÀÖ¾î¾ß ÇÔ.
bool find_dqstring(std::deque<CString> dqSrc, std::deque<CString> dqFind, TCHAR op, bool bWholeWord, bool bCaseSensitive)
{
	int i, j;
	bool found = false;

	if (!bCaseSensitive)
	{
		for (i = 0; i < dqSrc.size(); i++)
			dqSrc[i].MakeLower();
		for (i = 0; i < dqFind.size(); i++)
			dqFind[i].MakeLower();
	}

	int found_count = 0;

	for (i = 0; i < dqFind.size(); i++)
	{
		for (j = 0; j < dqSrc.size(); j++)
		{
			if (bWholeWord)
			{
				if (dqFind[i] == dqSrc[j])
				{
					//¸¸¾à OR ¿¬»êÀÌ¸é ¿©±â¼­ true ¸®ÅÏ
					found_count++;
					if (op == '|')
						break;
				}
			}
			else
			{
				if (dqSrc[j].Find(dqFind[i]) >= 0)
				{
					//¸¸¾à OR ¿¬»êÀÌ¸é ¿©±â¼­ true ¸®ÅÏ
					found_count++;
					if (op == '|')
						break;
				}
			}
		}

		if (op == '|' && found_count > 0)
			break;
	}

	//ANDÀÌ¸é Ã£´Â ¹®ÀÚ¿­ÀÌ ¸ðµÎ Á¸ÀçÇØ¾ß ÇÏ°í
	if (op == '&')
	{
		if (found_count == dqFind.size())
			return true;
	}
	//ORÀÌ¸é ÇÏ³ª¸¸ Á¸ÀçÇØµµ true
	else
	{
		if (found_count > 0)
			return true;
	}

	return false;
}

//Å¬¸³º¸µå clipboard
bool copy_to_clipboard(HWND hWnd, CString str)
{
	if (str.IsEmpty())
		return false;

	char	*pString = NULL;
	int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	pString = (char*)new char[len];
	WideCharToMultiByte(CP_ACP, 0, str, -1, pString, len, NULL, NULL);

	// ÀúÀåÇÒ ¹®ÀÚ¿­ÀÇ ±æÀÌ¸¦ ±¸ÇÑ´Ù. ('\0'±îÁö Æ÷ÇÔÇÑ Å©±â)
	int string_length = strlen(pString) + 1;

	// Å¬¸³º¸µå·Î ¹®ÀÚ¿­À» º¹»çÇÏ±â À§ÇÏ¿© ¸Þ¸ð¸®¸¦ ÇÒ´çÇÑ´Ù. 
	// Å¬¸³º¸µå¿¡´Â ÇÚµéÀ» ³Ö´Â Çü½ÄÀÌ¶ó¼­ HeapAlloc ÇÔ¼ö »ç¿ëÀÌ ºí°¡´ÉÇÏ´Ù. 
	HANDLE h_data = ::GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, string_length);

	// ÇÒ´çµÈ ¸Þ¸ð¸®¿¡ ¹®ÀÚ¿­À» º¹»çÇÏ±â À§ÇØ¼­ »ç¿ë °¡´ÉÇÑ ÁÖ¼Ò¸¦ ¾ò´Â´Ù. 
	char *p_data = (char *)::GlobalLock(h_data);

	if (NULL != p_data)
	{
		// ÇÒ´çµÈ ¸Þ¸ð¸® ¿µ¿ª¿¡ »ðÀÔÇÒ ¹®ÀÚ¿­À» º¹»çÇÑ´Ù. 
		memcpy(p_data, pString, string_length);

		// ¹®ÀÚ¿­À» º¹»çÇÏ±â À§ÇØ¼­ Lock Çß´ø ¸Þ¸ð¸®¸¦ ÇØÁ¦ÇÑ´Ù.
		::GlobalUnlock(h_data);

		if (::OpenClipboard(hWnd))
		{
			::EmptyClipboard(); // Å¬¸³º¸µå¸¦ ¿¬´Ù.
			::SetClipboardData(CF_TEXT, h_data);  // Å¬¸³º¸µå¿¡ ÀúÀåµÈ ±âÁ¸ ¹®ÀÚ¿­À» »èÁ¦ÇÑ´Ù.
												  // Å¬¸³º¸µå·Î ¹®ÀÚ¿­À» º¹»çÇÑ´Ù.
			::CloseClipboard(); // Å¬¸³º¸µå¸¦ ´Ý´Â´Ù.
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

//random19937À» ÀÌ¿ëÇÏ¿© ·£´ý ÄÃ·¯¸¦ ¸®ÅÏÇÑ´Ù.
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

CString	GetExeDirectory()
{
	TCHAR	sFilePath[1024];
	CString sExeFolder;
	
	GetModuleFileName(AfxGetInstanceHandle(), sFilePath, MAX_PATH);
	
	sExeFolder = sFilePath;
	
	return sExeFolder.Left(sExeFolder.ReverseFind('\\'));
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

CString		GetFileNameFromFullPath(CString sFullPath)
{
	if (sFullPath.Find(_T("\\")) > 0)
		return sFullPath.Right(sFullPath.GetLength() - sFullPath.ReverseFind('\\') - 1);
	
	return sFullPath;
}

CString		GetFolderNameFromFullPath(CString sFullPath)
{
	//sFullPath°¡ Æú´õ¶ó¸é ±×³É ±× °ªÀ» ¸®ÅÏÇÑ´Ù.
	if (PathIsDirectory(sFullPath))
		return sFullPath;

	if (sFullPath.Find(_T("\\")) > 0)
	{
		sFullPath = sFullPath.Left(sFullPath.ReverseFind('\\'));
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
	// È®Àå¸íÀÌ ÀÖ´Â °æ¿ì¿Í ¾ø´Â °æ¿ì¸¦ ±¸ºÐÇÑ´Ù.
	if (filename.Find('.') > 0)
		return filename.Left(filename.ReverseFind('.'));
	else
		return filename;
}

CString		GetFileExtension(CString sFullPath, bool dot)
{
	// mms://211.106.66.140/H_1TV
	// À§¿Í °°Àº À¥ÁÖ¼ÒÀÇ °æ¿ì¿¡´Â ""À» ¸®ÅÏÇÑ´Ù.
	// http://babobus.com/Html/content/0828_1.asf
	CString sString;
	
	if (CheckFileIsURL(sFullPath))
	{
		sString = GetFileNameFromFullPath(sFullPath);
		// È®Àå¸íÀÌ ÀÖ´Â °æ¿ì¿Í ¾ø´Â °æ¿ì¸¦ ±¸ºÐÇÑ´Ù.
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

//ÆÄÀÏ¸í¿¡¼­ È®ÀåÀÚ¸¦ newExt·Î º¯°æÇÑ´Ù.
//applyRealFileÀÌ trueÀÌ¸é ½ÇÁ¦ ÆÄÀÏ¸íµµ º¯°æ½ÃÅ²´Ù.
bool ChangeExtension(CString& filepath, CString newExt, bool applyRealFile)
{
	CString sOldExt = GetFileExtension(filepath);
	CString sNewFullPath = filepath.Left(filepath.GetLength() - sOldExt.GetLength()) + newExt;

	bool changeSuccess = false;
	if (applyRealFile)
	{
		//½ÇÁ¦ ÆÄÀÏ È®ÀåÀÚ¸¦ º¯°æÇßÀ»¶§¸¸ filepathµµ º¯°æ½ÃÄÑÁØ´Ù.
		changeSuccess = MoveFile(filepath, sNewFullPath);
		if (changeSuccess)
			filepath = sNewFullPath;
		return changeSuccess;
	}

	filepath = sNewFullPath;
	return true;
}

//Æú´õ¿¡ ÀÖ´Â ÆÄÀÏµé Áß filetitleÀÌ°í extension¿¡ ÇØ´çÇÏ´Â ÆÄÀÏ¸íÀ» ¸®ÅÏÇÑ´Ù.
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
//unit_string	: ´ÜÀ§¸¦ Ç¥½ÃÇÒ Áö (default = true)
CString		GetFileSizeString(CString sfile, int unit_limit, int floats, bool unit_string)
{
	return GetUnitSizeString(GetFileSize(sfile), unit_limit, floats, unit_string);
}

//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
//floats		: ¼Ò¼öÁ¡À» ¸î ÀÚ¸®±îÁö Ç¥½ÃÇÒÁö (default = 0)
//unit_string	: ´ÜÀ§¸¦ Ç¥½ÃÇÒ Áö (default = true)
//comma			: Á¤¼ö ºÎºÐ¿¡ ÀÚ¸®¼ö ÄÞ¸¶¸¦ Ç¥½ÃÇÒ Áö (default = false)
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
	//0.045KBÀÏ ¶§ floats°¡ 0ÀÌ¸é 0KBÀ¸·Î ¸®ÅÏµÉ °ÍÀÌ´Ù.
	//ÀÌ °æ¿ì¶ó¸é 1KB·Î ¸®ÅÏÇÏ´Â °ÍÀÌ ¸Â´Ù.
	//½ÇÁ¦ ÆÄÀÏÀÇ Å©±â°¡ 0byteÀÏ¶§¸¸ 0À» ¸®ÅÏÇÏÀÚ.
	//0.045KBÀÏ ¶§ floats°¡ 1ÀÌ¸é ¿ª½Ã 0.0KB·Î ¸®ÅÏµÉ °ÍÀÌ´Ù.
	//ÀÌ °æ¿ì¶ó¸é 0.1KB·Î ¸®ÅÏµÇ´Â°Ô ¸Â´Ù.
	//floats°¡ 1ÀÌ¸é 0.1ÀÌ°í ÀÌ 0.1º¸´Ù dsize°¡ ÀÛ´Ù¸é ±×³É 0.1¶ó°í ÇØÁÖÀÚ.
	double dmin = 1.0 / pow(10.0, floats);
	if (dsize > 0.0 && dsize < dmin)
		dsize = dmin;

	//dsize = ROUND(dsize, floats + 1);
	size_str = d2S(dsize, comma, floats);
	if (unit_string)
		size_str += unit_str;

	return size_str;
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
			// DoubleByteCharacterSetÀÌ¸é...
			if (IsDBCSLeadByte((BYTE)(str.GetAt(nCenter))))
				return nCenter-1;
			else
				return nCenter;
		}
	}
}

//ÁÖ¾îÁø array¿¡¼­ ¿øÇÏ´Â Ç×¸ñÀ» Ã£´Â´Ù.
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

//¿ø·¡ double dSec¸¦ »ç¿ëÇÏ¿© À§ÀÇ ÇÔ¼ö¸¦ »ç¿ëÇßÀ¸³ª ¼Ò¼öÁ¡ Ç¥½Ã ¹æ½ÄÀÇ ¿À·ù°¡ Á¸ÀçÇÏ¹Ç·Î
//1.0ÀÌ¾î¾ß ÇÒ °ªÀÌ 0.999999999998 °ú °°ÀÌ Ç¥ÇöµÇ¾î À§ÀÇ ÇÔ¼ö¸¦ »ç¿ëÇÒ ¼ö ¾øÀ» °æ¿ì°¡ ÀÖ´Ù.
//±×·¡¼­ ¾Æ¿¹ millisecond°ªµµ int·Î °£ÁÖÇÏ¿© Ã³¸®ÇÑ ¾Æ·¡ ÇÔ¼ö¸¦ »ç¿ëÇÑ´Ù.
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
//srtÀÚ¸·ÀÇ °æ¿ì´Â .´ë½Å ,¸¦ »ç¿ëÇÏ¹Ç·Î µÑ ´Ù °í·ÁÇØÁØ´Ù.
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
// ±âº»ÀûÀ¸·Î 16Áø¼ö ½ºÆ®¸µÀÌ ³Ñ¾î¿Â´Ù°í °¡Á¤...
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
		0,						// time-out period (<- ¿©±â¸¦ 20 ÀÌ¶ó°í ¾²¸é 20ÃÊ ÈÄ ¿¡ ¸®ºÎÆÃÇÑ´Ù.)
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

		//¼Ò¼ýÁ¡ÀÌ ÀÖ´Â °æ¿ì
		int iDotPos = strData.Find(_T(".")); 
		if (0 <= iDotPos)
		{
			if (0<= strData.Find(_T("."), iDotPos + 1))    // µÎ¹øÂ° "."
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
					//bool bDotNothing = (iDotPos<0);//.ÀÌ ¾øÀ¸¸é Error
					//if(bDotNothing)
					//{
					// return FALSE;
					//}
					bool bDotPosLater = (i<iDotPos);//.ÀÌ Eº¸´Ù µÚ¿¡ ÀÖÀ¸¸é Error
					if(bDotPosLater)
					{
						return FALSE;
					}
					iCountE++;
					if(1<iCountE){//E°¡ µÎ°³ ÀÖÀ¸¸é Error
						return FALSE;
					}
					bNextSign=TRUE;
					continue;
				}

				//EµÚ¿¡´Â +³ª -°¡ ÀÖ¾î¾ßÇÏ°í µÚ¿¡´Â ¼ýÀÚ°¡ ÀÖ¾î¾ß ÇÔ
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
				bool bSignError = bSign && (0!=i); //Ã³À½ÀÌ ¾Æ´Ï¸é Error
				if(bSignError)
				{
					return FALSE;
				}
			}
			else //¼ýÀÚÀÎ °æ¿ì
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

//ÁÖ¾îÁø ¹®ÀÚ¿­ÀÌ ¾ËÆÄºª°ú ¼ýÀÚ·Î¸¸ ±¸¼ºµÈ ¹®ÀÚ¿­ÀÎÁö °Ë»çÇÑ´Ù.
//excepts¿¡´Â Æ÷ÇÔµÇµµ µÇ´Â ¹®ÀÚ¿­µéÀÌ µé¾îÀÖ´Âµ¥ ÀÌµéÀº ;À¸·Î ±¸ºÐµÇ¾î ÀÖ°í
//°Ë»çÇÏ±â Àü¿¡ ¹Ì¸® »èÁ¦ÇÑ ÈÄ °Ë»çÇÑ´Ù.
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

//ÇÑ±Û·Î¸¸ ±¸¼ºµÈ ¹®ÀÚ¿­ÀÎÁö
bool IsHangul(CString str)
{
	int len = str.GetLength();
	const wchar_t start_ch = L'°¡';
	const wchar_t end_ch = L'ÆR';

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

//startºÎÅÍ ½ÃÀÛÇØ¼­ Ã³À½ ¸¸³ª´Â ¼ýÀÚ ¿µ¿ªÀ» ÃßÃâÇØ¼­ ¼ýÀÚ·Î ¸®ÅÏÇÑ´Ù.
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

//¹®ÀÚ¿­¿¡ Æ÷ÇÔµÈ ¼ýÀÚ¹®ÀÚ¸¦ ¼ýÀÚ·Î °£ÁÖÇÏ¿© ºñ±³ÇÑ´Ù.
//"a5"´Â "a12345"º¸´Ù ÀÛ´Ù.
//´Ü, "a5"´Â "a-22"¿Í °°ÀÌ "-"±âÈ£´Â ºÎÈ£·Îµµ, Æ¯¼ö¹®ÀÚ·Îµµ °£ÁÖÇÏÁö ¾ÊÀ¸¹Ç·Î "a5"°¡ ÀÛ´Ù°í Ã³¸®µÈ´Ù.
bool is_greater_with_numeric(CString str0, CString str1)
{
	//StrCmpLogicalW¸¸À¸·Î´Â Å½»ö±â¿Í °°Àº Â÷·Ê·Î Á¤·ÄµÇÁö ¾Ê¾Æ¼­
	//¾Æ·¡ ÄÚµå¸¦ ³Ö¾úÀ¸³ª Áö±Ý ´Ù½Ã µ¹·Áº¸¸é
	//StrCmpLogicalW¸¸À¸·Îµµ Å½»ö±â¿Í °°Àº Â÷·Ê·Î Á¤·ÄµÈ´Ù.
	//ºÐ¸íÈ÷ StrCmpLogicalW¸¸À¸·Î´Â °°Áö ¾Ê¾Ò´Âµ¥...
	/*
	int i = 0;

	while (true)
	{
		//¼­·Î ´Ù¸¥ ¹®ÀÚ¸¦ ¸¸³¯¶§±îÁö ½ºÅµÇÑ´Ù.
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

	//¸¸¾à ÆÄÀÏ¸í Ã³À½ ~ i¹Ù·Î Àü±îÁö ¼ýÀÚ¿´°í iºÎÅÍ ¼ýÀÚ°¡ ¾Æ´Ï¸é
	//ÆÄÀÏ¸í ½ÃÀÛºÎÅÍ ¼ýÀÚ·Î µÈ ÆÄÀÏ¸íÀÎ °æ¿ì´Ù.
	
	if ((i > 0) && (str0[i] < '0' || str0[i] > '9' || str1[i] < '0' || str1[i] > '9') &&
		IsNumeric(str0.Left(i)))
	{
		//¾Æ·¡ if¹®Àº Ç×»ó trueÀÏ ¼ö ¹Û¿¡ ¾øÀ» °ÍÀÌ´Ù.
		if (get_number_from_string(str0, num0, 0) &&
			get_number_from_string(str1, num1, 0))
			return (num0 > num1);
	}
	*/
	return (StrCmpLogicalW(CString2PCWSTR(str0), CString2PCWSTR(str1)) == 1);
}


//nÀ» 26Áø¼ö ¿¢¼¿ ÄÃ·³ ÀÎµ¦½º·Î º¯È¯ÇÑ ¹®ÀÚ¿­À» ¸®ÅÏ
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
		return FALSE;  // ÀÌ¸ÞÀÏÀÌ ¾øÀ¸¸é false

	int iAtCount = 0;   //@ À§Ä¡
	int iDotCount = 0;  // . À§Ä¡
	int i;

	for(i = 0; i < sEMail.GetLength(); i++)
	{
		if (i > 0 && sEMail.GetAt(i) == '@')
			iAtCount = i+1;    // ¨ç

		if(iAtCount > 0 && i > iAtCount && sEMail.GetAt(i) == '.')
			iDotCount = i+1;   // ¨è
	}

	if (i > iDotCount && iAtCount > 0 && iDotCount > 0)
		return TRUE;     // ¨é
	
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
	// ÀÎÅÍ³Ý ¿¬°áÀÌ ¾ÈµÈ °æ¿ì Á÷Á¢ Á¢¼ÓÀ» ½ÃµµÇÏ¸é ¿À·ù°¡ ¹ß»ýÇÏ¹Ç·Î ¿ì¼± Ã¼Å©ÇØ¾ß ÇÑ´Ù.
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
	
	// ÆÄÀÏÀÌ Á¸ÀçÇÏ¸é...
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
		return _T("³×Æ®¿öÅ©¿¡ Á¢¼ÓÇÒ ¼ö ¾ø½À´Ï´Ù.");

#define HTTPBUFLEN    1024 * 1024 // Size of HTTP Buffer...
	TCHAR		*httpbuff;
	TCHAR		szCause[255];
	CString		Cause;

	Cause.Format(_T("TRUE"));

	TRY
	{
		CInternetSession mysession;
	CHttpFile *remotefile = (CHttpFile*)mysession.OpenURL(sUrl,1,INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD);

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
	bmpInfo.bmiHeader.biWidth			= MAKE4WIDTH(w);
	bmpInfo.bmiHeader.biHeight			= -h;
	bmpInfo.bmiHeader.biBitCount		= 8 * ch;
	bmpInfo.bmiHeader.biSizeImage		= sizeof(BYTE) * w * h * ch;

	file.Write(&bmpInfo, sizeof(BITMAPINFOHEADER));


	DWORD	width4;

	width4 = WIDTHSTEP4(w * bmpInfo.bmiHeader.biBitCount);
	file.Write(pData, sizeof(BYTE) * width4 * h);

	/////////////////////////////////////////////////////////////////////
	// BITMAPFILEHEADER¿¡ bfSize¸¦ ±â·ÏÇÑ´Ù.
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



//´ÙÀ½ ÆÄÀÏÀÇ ÀÎµ¦½º ¸®ÅÏ
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

CString		GetMostRecentDateFile(CString sFolder, CString sWildCard /*= "*.*"*/)	//°¡Àå ÃÖ±Ù ³¯Â¥ ÆÄÀÏ¸í ¸®ÅÏ
{
	CFileFind	FileFind;
	bool		bWorking;
	CString		sMostRecentDateFile = _T("");
	CString		sfile;
	CTime		tFile;
	
	//°ú°Å ½Ã°£ ÃÖ´ë´Â 1970³â 1¿ù 1ÀÏ 0½Ã°¡ ¾Æ´Ñ 9½Ã´Ù.
	//¿Ö³Ä¸é ¿ì¸®³ª¶ó°¡ GMP+9ÀÌ¹Ç·Î. 1970-1-1 8:59:59 ÀÌ°Íµµ assert fail ³­´Ù.
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

		//p¸¦ »ç¿ëÇÏÁö ¾Ê°í Á÷Á¢ p1À» ÀÌ¿ëÇÏ¿© ¾Æ·¡ strcpy¸¦ ¼öÇàÇÏ¸é debug heap¿¡·¯°¡ ¹ß»ýÇÑ´Ù.
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
			sMsg.Format(_T("ÆÄÀÏ ¿­±â ½ÇÆÐ : \n\n¾ÏÈ£È­¸¦ À§ÇØ %s ÆÄÀÏÀ» ¿­ ¼ö ¾ø½À´Ï´Ù."), sSource);
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
	
	if (nDotPos > 0)	// ½Ç¼öÀÎ °æ¿ì
	{
		sFinal		= sString.Mid(nDotPos);
		sIntPart	= sString.Left(nDotPos);
	}
	else
	{
		sIntPart	= sString;
	}
	
	//¸Ç ³¡ ¼ýÀÚºÎÅÍ ÇÏ³ª¾¿ »õ ¹®ÀÚ¿­¿¡ Ãß°¡½ÃÅ²´Ù.
	//3ÀÚ¸® µÉ ¶§¸¶´Ù ÄÞ¸¶¸¦ ³Ö¾îÁØ´Ù.
	for (int i = 0; i < sIntPart.GetLength(); i++)
	{
		sFinal.Insert(0, sIntPart.GetAt(sIntPart.GetLength() - 1 - i));
		
		if (((i + 1) % 3 == 0) &&				//3Â° ÀÚ¸®¸¶´Ù
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

//value°¡ 3ÀÚ¸®ÀÌ°í total_digits°¡ 7ÀÌ¸é 4°³ÀÇ 0À» ¾Õ¿¡ ºÙ¿©¼­ ¸®ÅÏÇÑ´Ù.
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

// »ç¿ë¹ý
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


//¼Ò¼öÁ¡ ÀÚ¸´¼ö ¸®ÅÏ.
//0.123456789 À» ³Ñ°ÜÁàµµ d°ªÀº ±×´ë·Î ³Ñ°Ü¹ÞÀ¸³ª
//sprintf¿¡¼­ 0.123457·Î º¯°æµÈ´Ù.(¼Ò¼öÁ¡ 6ÀÚ¸®·Î¸¸ Ã³¸®µÈ´Ù.)
//¿ì¼± ÀÌÁ¡À» ÁÖÀÇÇÏ°í ¸¹Àº ÀÚ¸®¼ö¸¦ ±»ÀÌ Á¤È®È÷ ¸®ÅÏÇØ¾ß ÇÑ´Ù¸é
//ÄÚµå¸¦ Á» ´õ º¸¿ÏÇÏÀÚ.
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
			//0ÀÌ ¾Æ´Ñ ÀÚ¸´¼ö¸¦ ÇÑ¹ø ¸¸³ª¸é ±× ÈÄ·Î´Â À¯È¿ 0ÀÌ¹Ç·Î Ä«¿îÆ®ÇØ¾ß ÇÑ´Ù.
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

//±â´É : unsigned °ªÀ» signed ¹üÀ§·Î º¯°æÇÑ´Ù.
//ex1. 0 ~ 99 ¹üÀ§¸¦ °¡Áø unsigned µ¥ÀÌÅÍ Å¸ÀÔÀ» signed·Î º¯°æÇÒ °æ¿ì
//signedÀÇ ¹üÀ§´Â -50 ~ +49ÀÏ °ÍÀÌ´Ù.
//53ÀÌ¶ó´Â °ªÀº 53 - 100 = -47ÀÌ µÈ´Ù.
//signedÀÇ maxÀÎ 49º¸´Ù ÀÛ°Å³ª °°Àº °ªÀº ±×³É ¸®ÅÏ
//ex2. 0 ~ 3 ¹üÀ§¸¦ °¡Áø 2bit µ¥ÀÌÅÍ Å¸ÀÔÀ» signed·Î º¯°æÇÒ °æ¿ì
//signedÀÇ ¹üÀ§´Â -2 ~ +1ÀÏ °ÍÀÌ´Ù.
//3ÀÌ¶ó´Â °ªÀº 3 - 4 = -1ÀÌ µÈ´Ù.
//signedÀÇ maxÀÎ 1º¸´Ù ÀÛ°Å³ª °°Àº °ªÀº ±×³É ¸®ÅÏ
int getSignedFromUnsigned(unsigned value, int bit_length)
{
	int range_max = pow(2.0, bit_length);
	int signed_max = range_max / 2 - 1;

	if (value <= signed_max)
		return value;

	return (value - range_max);
}

//dlgÀÇ OnInitDialog()¿¡¼­ ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÒ °æ¿ì
//¿¹Àü¿¡´Â Àß Àû¿ëµÇ¾úÀ¸³ª À©10ÀÇ IME ¹æ½ÄÀÌ º¯°æµÇ¾ú´ÂÁö
//OnInitDialog()¿¡ ³ÖÀ¸¸é Àû¿ëµÇÁö ¾Ê´Â´Ù.
//SetTimer³ª UI ¹öÆ°À» Ãß°¡ÇÏ¿© º¯°æÇÏ¸é Àß µ¿ÀÛÇÔ.
//´Ü, OnInitDialog()¿¡¼­ Å¸ÀÌ¸Ó¸¦ 100ms¿Í °°ÀÌ ³Ê¹« Âª°Ô ÁÖ¸é º¯°æ ¾ÈµÊ. ³Ë³ËÈ÷ 500ms ÀÌ»ó ÁÙ °Í.
void IME_Convert_To_NativeCode(HWND hWnd, bool bNative)
{
	//¾Æ·¡ ÁÖ¼®Ã³¸®µÈ ºí·°Àº ÀÌÀü ¹æ½ÄÀÌ°í
	//¸Ç ¾Æ·¡ 3ÁÙÀº °£·«È÷ ¼öÁ¤µÈ ¹æ½Ä. µÑ ´Ù Á¤»ó µ¿ÀÛÇÔ.
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
	// À±³âÀÌµÇ·Á¸é
	// 1. 4·Î ³ª´©¾î ³ª¸ÓÁö°¡ ¾ø¾î¾ß ÇÏ°í
	// 2. 100À¸·Î ³ª´©¾î¶³¾îÁöÁö ¾Ê°Å³ª,
	// 400À¸·Îµµ ³ª´©¾î¶³¾îÁ®¾ß ÇÔ
	
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

	//ÇÊÅÍ °ü·Ã
	std::deque<CString>	dqExtFilter;

	//Á¦¿Ü Ç×¸ñÀ» À§ÇÑ º¯¼öµé
	std::deque<CString>	dqExcepts;
	CString			sToken;
	int				curPos;

	if (sExtFilter == "")
		sExtFilter = _T("*");

	//ÇÊÅÍ¿Í Á¦¿Ü ¹®ÀÚ¿­Àº ´ë¼Ò¹®ÀÚ¸¦ ±¸ºÐÇÏÁö ¾Ê´Â´Ù.
	curPos = 0;
	sToken = sExtFilter;
	sExtFilter.MakeLower();
	dqExtFilter.clear();

	//È®ÀåÀÚ´Â "jpg"¿Í °°ÀÌ dot°¡ ¾øÀÌ ºñ±³ÇÑ´Ù.
	//È®ÀåÀÚ¿¡ * ¶Ç´Â dot°¡ ÀÖ´Ù¸é Á¦°ÅÇØÁØ´Ù.
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

	//recursive±îÁö °í·ÁÇÏ¿© ¿ì¼±Àº ¸ðµç ÆÄÀÏÀ» ´ë»óÀ¸·Î ÇØ¾ß ÇÑ´Ù.
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
			//sFilter¿¡ ÇØ´çÇÏ´Â ÆÄÀÏ¸¸À» °Ë»ö ´ë»óÀ¸·Î ÇÑ´Ù.
			bool bFound = false;

			if (dqExtFilter.size() == 0)
			{
				bFound = true;
			}
			else
			{
				//¿¹¸¦µé¾î È®ÀåÀÚ ¸ñ·Ï¿¡ jpg°¡ ÀÖ´Âµ¥ ÆÄÀÏ¸í Áß°£¿¡¼­ jpg°¡ ¹ß°ßµÇ¸é ¹®Á¦°¡ µÈ´Ù.
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
				//Á¦¿ÜÇÒ ¹®ÀÚ¿­ÀÌ Æ÷ÇÔµÇ¾î ÀÖÁö ¾ÊÀº ÆÄÀÏµé¸¸ ¸®½ºÆ®¿¡ Ãß°¡ÇÑ´Ù.
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
		sort_like_explorer(dqFiles);
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
	//ÇÊÅÍ¿Í Á¦¿Ü ¹®ÀÚ¿­Àº ´ë¼Ò¹®ÀÚ¸¦ ±¸ºÐÇÏÁö ¾Ê´Â´Ù.
	int curPos = 0;
	CString sToken = ext_filters;
	CString file;
	CString filetitle;
	CString name_field = name_filter;

	//name_filter¿¡¼­ wildcard¸¦ Á¦°ÅÇÑ ºÎºÐ
	name_field.Replace(_T("*"), _T(""));
	name_field.Replace(_T("?"), _T(""));

	//name_filter´Â ÀÏ´Ü *¸¸ Çã¿ëÇÑ´Ù.
	//abc* : 
	//*abc
	//*abc*
	if (name_filter == "*")
		name_filter = _T("");

	if (ext_filters == "")
		ext_filters = _T("*");

	ext_filters.MakeLower();

	//È®ÀåÀÚ´Â "jpg"¿Í °°ÀÌ dot°¡ ¾øÀÌ ºñ±³ÇÑ´Ù.
	//È®ÀåÀÚ¿¡ * ¶Ç´Â dot°¡ ÀÖ´Ù¸é Á¦°ÅÇØÁØ´Ù.
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
			//Á¦¿ÜÇÒ ¹®ÀÚ¿­ÀÌ Æ÷ÇÔµÇ¾î ÀÖÁö ¾ÊÀº ÆÄÀÏµé¸¸ ¸®½ºÆ®¿¡ Ãß°¡ÇÑ´Ù.
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
			//Á¦¿ÜÇÒ ¹®ÀÚ¿­ÀÌ Æ÷ÇÔµÇ¾î ÀÖÁö ¾ÊÀº ÆÄÀÏµé¸¸ ¸®½ºÆ®¿¡ Ãß°¡ÇÑ´Ù.
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
	/*
	std::deque<CString> list1;
	list1.assign(list.begin(), list.end());

	//sort looks like windows10 explorer
	if (list.size() && auto_sort)
	{
		sort_like_explorer(&list);
	}

	if (list == list1)
		;
	if (std::equal(list.begin(), list.end(), list1.begin(), list1.end()))
		;
	*/
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

	//  UniCode BOM ±â·Ï
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

	//È®ÀåÀÚ´Â "jpg"¿Í °°ÀÌ dot°¡ ¾øÀÌ ºñ±³ÇÑ´Ù.
	//È®ÀåÀÚ¿¡ * ¶Ç´Â dot°¡ ÀÖ´Ù¸é Á¦°ÅÇØÁØ´Ù.
	extensions.Replace(_T("*"), _T(""));
	extensions.Replace(_T("."), _T(""));

	GetTokenString(extensions, dqToken, ';');

	//Æú´õ³»ÀÇ ÆÄÀÏµéÀ» Ã£¾Æ¼­ ºñ±³ÇÏ´Â°Ô ¾Æ´Ï¶ó
	//Á¶°Ç¿¡ ¸Â´Â ÆÄÀÏÀÌ Á¸ÀçÇÏ´ÂÁö·Î °Ë»çÇÑ´Ù.
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
	FileOp.wFunc = FO_DELETE; // »èÁ¦ ¼Ó¼º ¼³Á¤
	FileOp.pFrom = NULL;
	FileOp.pTo = NULL;
	//È®ÀÎ¸Þ½ÃÁö°¡ ¾È¶ßµµ·Ï ¼³Á¤
	FileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI;
	FileOp.fAnyOperationsAborted = false;
	FileOp.hNameMappings = NULL;
	FileOp.lpszProgressTitle = NULL;
	FileOp.pFrom = szTemp;

	int res = SHFileOperation(&FileOp); // »èÁ¦ ÀÛ¾÷
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


//½ÃÀÛÆú´õ ¹× ÇÏÀ§ Æú´õµéÀº ¿©ÀüÈ÷ ³²¾ÆÀÖ´Ù.
//Æú´õ ÅëÂ°·Î ´Ù Áö¿ì·Á¸é ÄÚµåÀÇ ¼öÁ¤ÀÌ ÇÊ¿äÇÏ´Ù.
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
				//Æ¯Á¤ È®ÀåÀÚ ÆÄÀÏµéÀ» Áö¿ì´Â °æ¿ì
				if (sFilter.Left(2) == "*.")
				{
					str1 = GetFileExtension(sfile);
					str2 = sFilter.Right(sFilter.GetLength() - sFilter.ReverseFind('.') - 1);
				}
				//Æ¯Á¤ ÆÄÀÏ¸í ÆÄÀÏµéÀ» Áö¿ì´Â °æ¿ì
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

//ÀÎÀÚ·Î ³Ñ¾î¿Â Æú´õ¸¦ Æ÷ÇÔÇÏ¿© ¸ðµç ÇÏÀ§Æú´õ, ÆÄÀÏÀ» Áö¿î´Ù.
bool SHDeleteFolder(CString sFolder)
{
    SHFILEOPSTRUCT FileOp = {0};
    TCHAR szTemp[MAX_PATH];
 
	_stprintf(szTemp, _T("%s"), sFolder);
    szTemp[_tcslen(szTemp) + 1] = NULL;
 
    FileOp.hwnd = NULL;
    FileOp.wFunc = FO_DELETE;       // »èÁ¦ ¼Ó¼º ¼³Á¤
    FileOp.pTo = NULL;
    FileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SIMPLEPROGRESS; // È®ÀÎ¸Þ½ÃÁö°¡ ¾È¶ßµµ·Ï ¼³Á¤
    FileOp.fAnyOperationsAborted = false;
    FileOp.hNameMappings = NULL;
    FileOp.lpszProgressTitle = sFolder;
    FileOp.pFrom = szTemp;
 
    if (SHFileOperation(&FileOp) == 0)       // »èÁ¦ ÀÛ¾÷
        return TRUE;

	return FALSE;
}
 
//------------------------------------------------------------
//ÇÏÀ§Æú´õ ¸ðµÎ »èÁ¦
//ÇÏÀ§ µð·ºÅä¸® ¸ðµÎ »èÁ¦
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

//Æú´õ³»ÀÇ Æ¯Á¤ ¹®ÀÚ¿­ÀÌ µé¾î°£ ÆÄÀÏµéÀ» Áö¿î´Ù. ÇÏÀ§Æú´õ Áö¿ø¾ÈÇÔ.
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
bool MakeFullDirectory(LPCTSTR sFolder)
{
	if (PathFileExists(sFolder) && ::PathIsDirectory(sFolder))
		return true;
	
	TCHAR parent[MAX_PATH] = _T("");

	_tcscpy(parent, sFolder);
	::PathRemoveFileSpec(parent);
	
	if (MakeFullDirectory(parent))
		return (::CreateDirectory(sFolder, NULL) != false);

	return false;
} 

//ÀÌ ÇÔ¼ö¸¦ »ç¿ëÇÏ·Á¸é ¹Ýµå½Ã SetBkMode(TRANSPARENT);·Î ¼³Á¤ÇØ¾ß È¿°ú°¡ ³ªÅ¸³².
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

//ÀÌ ÇÔ¼ö¸¦ »ç¿ëÇÏ·Á¸é ¹Ýµå½Ã SetBkMode(TRANSPARENT);·Î ¼³Á¤ÇØ¾ß È¿°ú°¡ ³ªÅ¸³².
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

//ÀÌ ÇÔ¼ö¸¦ »ç¿ëÇÏ·Á¸é ¹Ýµå½Ã SetBkMode(TRANSPARENT);·Î ¼³Á¤ÇØ¾ß È¿°ú°¡ ³ªÅ¸³².
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

//ÀÌ ÇÔ¼ö¸¦ »ç¿ëÇÏ·Á¸é ¹Ýµå½Ã SetBkMode(TRANSPARENT);·Î ¼³Á¤ÇØ¾ß È¿°ú°¡ ³ªÅ¸³².
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

//À©µµ¿ì ¿î¿µÃ¼Á¦¿¡¼­ Æ¯Á¤ Æú´õÀÇ ½ÇÁ¦ °æ·Î¸¦ ¸®ÅÏÇÑ´Ù.
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

//Å¸ÀÌ¸Ó¿¡ ÀÇÇØ È£ÃâµÈ ÇÔ¼ö³»¿¡¼­ »ç¿ëÇÒ °æ¿ì¿¡´Â Á¦´ë·Î µ¿ÀÛÇÏÁö ¾ÊÀ½.
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

//¹Ýº¹¹®¿¡ ÀÇÇØ process°¡ ÀÀ´ä¾øÀ½ÀÌ µÇÁö ¾Êµµ·Ï ¹Ýº¹¹®¾È¿¡¼­ È£ÃâÇÏ¿© ¸Þ½ÃÁöÅ¥ÀÇ ³»¿ëÀ» ¹Ù·Î Ã³¸®½ÃÅ²´Ù.
void ProcessWindowMessage()
{
   MSG msg;

   while(::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
   {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
   }
}

// 16ºñÆ® Á¤¼ö¸¦ 2Áø¼ö ¹®ÀÚ¿­·Î º¯È¯ ÇÔ¼ö
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

//¾î¶² ¼öÀÇ x ÀÎµ¦½º À§Ä¡ÀÇ ºñÆ®°ªÀ» ¸®ÅÏ.
int get_bit(int number, int x)
{
	int bit = (number >> x) & 1;
	return bit;
}

//x°¡ 1ÀÌ¸é n¹øÂ° °ªÀ» 1·Î º¯°æ, x°¡ 0ÀÌ¸é n¹øÂ° °ªÀ» 0À¸·Î º¯°æ
void set_bit(int& number, int n, int x)
{
	number ^= (-x ^ number) & (1 << n);
}

void makeRect(CRect &Rect, int x, int y, int w, int h)
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

CRect get_zoom_rect(CRect rect, double zoom)
{
	double l, t, r, b;
	l = (double)rect.left * zoom;
	t = (double)rect.top * zoom;
	r = (double)rect.right * zoom;
	b = (double)rect.bottom * zoom;
	return CRect(l, t, r, b);
}

//0:lt, 1:rt, 2:rb, 3:lb, rb_cutÀÌ trueÀÌ¸é ³¡Á¡-1ÀÎ °ªÀ» ¸®ÅÏÇÏ°í falseÀÌ¸é ³¡Á¡ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
//½ÇÁ¦ »ç°¢ÇüÀº rb_cutÀÌ falseÀÌÁö¸¸
//¿µ¿ª ¶Ç´Â ÇÈ¼¿À» º»´Ù¸é true·Î ÇØ¾ß ¸Â´Ù.
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


// ÆÄÀÏ¿¡ ´ëÇÑ ¼îÆ®ÄÆÀ» ¸¸µç´Ù.
// ¾Æ·¡ÀÇ ÇÔ¼ö¿Í °ÅÀÇ µ¿ÀÏÇÏ´Ù.
// pszSrcFile:¼ôÄÆ ´ë»ó ÆÄÀÏÀÇ Ç® ÆÐ½º
// pszLnkFile:¼ôÄÆ ÆÄÀÏÀÇ Ç® ÆÐ½º
// pszArgument:¼îÆ®ÄÆ¿¡ Àü´ÞµÉ ÀÎ¼ö ¹®ÀÚ¿­
// pszDesc:¼îÆ®ÄÆ ¼³¸í ¹®ÀÚ¿­
HRESULT	MyCreateShortCut(LPCTSTR pszSrcFile, LPCOLESTR pszLnkFile,
						  LPTSTR pszWorkingDir/* = NULL*/, LPTSTR pszArgument/* = NULL*/, LPTSTR pszDesc/* = NULL*/)
{
	HRESULT			hr;
	IShellLink		*pSl;
	IPersistFile	*pPf;
	TCHAR wszLnkFile[MAX_PATH]={0,};
	
	// IShellLink °´Ã¼¸¦ »ý¼ºÇÏ°í Æ÷ÀÎÅÍ¸¦ ±¸ÇÑ´Ù.
	CoInitialize(NULL);

	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		 IID_IShellLink, (void **)&pSl);
	
	if (FAILED(hr))
		return E_FAIL;
	
	// ¼îÆ®ÄÆÀÇ ´ë»óÃ¼¿Í ¼³¸íÀ» ¼³Á¤ÇÑ´Ù.
	pSl->SetPath(pszSrcFile);
	pSl->SetArguments(pszArgument);
	pSl->SetDescription(pszDesc);
	pSl->SetWorkingDirectory(pszWorkingDir);
	
	// ÀúÀåÇÏ±â À§ÇØ IPersistFile °´Ã¼¸¦ »ý¼ºÇÑ´Ù.
	hr = pSl->QueryInterface(IID_IPersistFile, (void **)&pPf);

	if (FAILED(hr))
	{
		pSl->Release();
		return E_FAIL;
	}
	
	// À¯´ÏÄÚµå·Î ÆÄÀÏ ÆÐ½º¸¦ º¯°æÇÑ ÈÄ ÀúÀåÇÑ´Ù.
	//MultiByteToWideChar(CP_ACP, 0, pszLnkFile, -1, wszLnkFile, MAX_PATH);
	hr=pPf->Save(pszLnkFile, TRUE);
	
	// °´Ã¼ ÇØÁ¦
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

//ÇöÀç ½ÇÇàÆÄÀÏÀÇ ¹öÀü°ü·Ã Á¤º¸ ¹®ÀÚ¿­À» ¾ò¾î¿Â´Ù.
//rc->Version->VS_VERSION_INFO Ç×¸ñ
//ex. m_sVersion = GetFileVersionInformation(sExeFile, "FileVersion");
//¾Æ·¡ÀÇ GetFileProperty ÇÔ¼öµµ µ¿ÀÏÇÑ ±â´ÉÀ» ¼öÇàÇÏ´Â ÄÚµåÀÌ³ª °¢ Ç×¸ñº°·Î È®ÀÎ ÇÊ¿ä!
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

	if(cbTranslate != 0) // ¹öÀü Á¤º¸°¡ ¾øÀ» °æ¿ì "
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

//¸í½ÃµÈ FileVersion ¶Ç´Â ProductVersionÀ» ¾ò¾î¿Â´Ù.
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

//ÇØ´ç Æú´õÀÇ ÀÌÀü ÆÄÀÏ¸í ÀüÃ¼ ¶Ç´Â ÀÏºÎºÐÀ» »õ·Î¿î ÀÌ¸§À¸·Î º¯°æÇÑ´Ù.
//´Ü, È®ÀåÀÚ´Â Æ÷ÇÔµÇÁö ¾Ê´Â´Ù.
//µû¶ó¼­ È£ÃâÇÒ ¶§ ¾Æ¿¹ oldName°ú newName¿¡´Â È®ÀåÀÚ´Â ÀÖ¾î¼­´Â ¾ÈµÈ´Ù.
//ÁÖÀÇ!
//bWholenameÀÌ false¶ó°í ÇØµµ "*%s*"¿Í °°ÀÌ ¾Õ¿¡µµ *¸¦ ºÙÀÌ¸é
//¿øÄ¡ ¾Ê´Â °á°ú°¡ ¹ß»ýÇÒ ¼ö ÀÖÀ¸´Ï ¹Ýµå½Ã µð¹ö±ëÀ¸·Î È®ÀÎÇÑ ÈÄ »ç¿ëÇÒ ÇÊ¿ä°¡ ÀÖ´Ù.
//renameÀÌ ¼º°øÇÑ ÆÄÀÏÀÇ °³¼ö¸¦ ¸®ÅÏÇÑ´Ù.
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
		//°Ë»öµÈ ÆÄÀÏ¸í¿¡¼­ oldName ´ÙÀ½¿¡ . _Snapshot ÀÌ µÎ°³ÀÇ ÆÐÅÏÀº °°Àº ÆÄÀÏ±ºÀ¸·Î Ã³¸®ÇÑ´Ù.
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

//mp4 ÆÄÀÏÀÇ Æ¯Á¤ ÅÂ±× µ¥ÀÌÅÍ Áß ¿øÇÏ´Â À§Ä¡ÀÇ µ¥ÀÌÅÍ¸¦ ÃßÃâÇÑ´Ù.
//MOBIS ÇÁ·ÎÁ§Æ® ÀúÀå MP4´Â mdat ÇÊµåÀÇ 0x40¹øÁöºÎÅÍ n bytes°¡
//µ¿¿µ»óÀÌ »ý¼ºµÈ Àý´ë½Ã°£ÀÌ ÀúÀåµÇ¾î ÀÖ´Ù.
//nÀº 0x3CºÎÅÍ 4¹ÙÀÌÆ®°¡ ±× Å©±âÀÌ´Ù.
char* GetDataFromMP4File(char* sfile, char* sTag, uint8_t tagStart, int tagLength)
{
	FILE	*fp = fopen(sfile, "rb");
	if (fp == NULL)
		return NULL;

	int		size;
	char	type[6][4];
	char	*pData = NULL;

	//4~7±îÁö 4¹ÙÀÌÆ®°¡ "ftyp"ÀÌ¾î¾ß mp4 ÆÄÀÏÀÌ´Ù.
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

			//µ¥ÀÌÅÍÀÇ Å©±â´Â 28¹ÙÀÌÆ®ÀÌ¾î¾ß ÇÑ´Ù.
			//´Ù¸¥ °ªÀÌ¶ó¸é timeStamp°¡ ¾Æ´Ï°Å³ª ¾ø´Â µ¿¿µ»óÀÌ´Ù.
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

/*
char* Utf8Encode(IN LPCTSTR szText)
{
	USES_CONVERSION;
	// strÀÌ UnicodeÀÎÁö Ansi ÀÎÁö µûÁú ÇÊ¿ä¾ø°Ô T2CW·Î º¯È¯    
	const WCHAR* wStr = T2CW(szText);    
	
	// ±æÀÌ´Â -1·Î ÁÖ¾î ³ÎNULL ¹®ÀÚµµ º¯È¯µÇµµ·Ï
	//WCHAR -> UTF-8
	
	int nUTF8codeSize = WideCharToMultiByte(CP_UTF8, 0, wStr, -1, NULL, 0, NULL, NULL); //wStrÀÇ Å©±â¸¦ ±¸ÇÔ
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

//µÎ Á¡ÀÇ °¢µµ¸¦ ±¸ÇÑ´Ù.
//¿øÇÏ´Â °á°ú°ªÀº 0~360µµÀÎµ¥
//atanÀº ºÎÈ£¿¡ µû¸¥ °¢µµ¸¦ ±¸ºÐÇÒ ¼ö ¾ø°í
//atan2´Â -PI ~ +PI·Î ¸®ÅÏÇÏ¹Ç·Î 0~360 ¹üÀ§¿Í ¸ÂÁö ¾ÊÀ¸¹Ç·Î ¾Æ·¡¿Í °°ÀÌ ±¸ÇØ¾ß ÇÑ´Ù.
//ScreenCoordÀÏ¶§¿Í Cartesian coordinate(Á÷±³ÁÂÇ¥°è)ÀÏ¶§´Â y°¡ ¹Ý´ëÀÓ¿¡ ÁÖÀÇ.
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

//3Á¡ÀÌ ÀÌ·ç´Â °¢µµ
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

//µÎ Á¡À» Áö³ª´Â Á÷¼±»óÀÇ x3¸¦ ±¸ÇÑ´Ù.
double getLinePointX(double x1, double y1, double x2, double y2, double y3)
{
	if (x1 == x2)
		return x1;

	double a = (double)(y2 - y1) / (double)(x2 - x1);
	double b = y2 - a * x2;
	return ((y3 - b) / a);
}

//µÎ Á¡À» Áö³ª´Â Á÷¼±»óÀÇ y3¸¦ ±¸ÇÑ´Ù.
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

//ptCenter¸¦ ±âÁØÀ¸·Î dAngle ¸¸Å­ È¸ÀüµÈ dDist°Å¸®ÀÇ Á¡ÀÇ ÁÂÇ¥¸¦ ±¸ÇÑ´Ù.
CPoint GetRotatedPoint(CPoint ptCenter, double dAngle, double dDist)
{
	CPoint	pt;

	pt.x = cos(RADIAN(dAngle)) * dDist;
	pt.y = -sin(RADIAN(dAngle)) * dDist;
	pt.Offset(ptCenter);

	return pt;
}

//cx, cy¸¦ Áß½ÉÀ¸·Î tx, tyÁ¡ÀÌ degree¸¦ È¸ÀüÇÒ °æ¿ì tx, tyÁ¡ÀÇ º¯°æ ÁÂÇ¥
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

//¹®ÀÚ¿­¿¡¼­ n¹øÂ° ÅäÅ«À» ¸®ÅÏÇÑ´Ù.
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


//nMaxToken ¼ýÀÚ ¸¸Å­ ÅäÅ«À» ºÐ¸®ÇØ¼­ sToken¿¡ ³Ö°í
//½ÇÁ¦ Ã³¸®µÈ ÅäÅ« °³¼ö¸¦ ¸®ÅÏÇÑ´Ù.
//src´Â ÅäÅ«ÀÌ ºÐ¸®µÉ ¶§¸¶´Ù ÁÙ¾îµç´Ù.
//ÀÌ´Â CString::Tokenize() ¶Ç´Â AfxExtractSubString()°ú´Â ´Ù¸£´Ù.
//(seps = " ,\t\n" °ú °°ÀÌ ºÐ¸®±âÈ£µé·Î ÀÌ·ç¾îÁø ½ºÆ®¸µ µ¥ÀÌÅÍ)
//strtok_s ÇÔ¼ö´Â ¿¬¼Ó °ø¹éÀÌ³ª ¿¬¼Ó ½°Ç¥ µî Áßº¹µÈ ºÐ¸®ÀÚ´Â ¸ðµÎ ÇÏ³ªÀÇ ±¸ºÐÀÚ·Î Ãë±ÞµÈ´Ù.
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
	if(swscanf(str, fmtstr, &ret) != 1) throw 1;

	return(ret);
}

double get_double(CString& buff, CString sep)
{
	CString str;

	str = get_str(buff, sep);
	str.MakeLower();

	float ret;
	if(swscanf(str, _T("%f"), &ret) != 1) throw 1;

	return((double)ret);
}

//¸Ç ¸¶Áö¸· ÀÎÀÚ´Â ¹Ýµå½Ã NULLÀ» ³Ö¾îÁà¾ß ³¡À» ¾Ë ¼ö ÀÖ´Ù.
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

//src ¹®ÀÚ¿­¿¡ set_of_keyword¿¡ ³ª¿­µÈ ´Ü¾î°¡ ÀÖ´ÂÁö °Ë»ç.
//set_of_keyword´Â ¼¼¹ÌÄÝ·ÐÀ¸·Î ±¸ºÐÇØ¼­ ¿©·¯ ¹®ÀÚ ¶Ç´Â ¹®ÀÚ¿­À» ³ÖÀ» ¼ö ÀÖ´Ù.
//ex. src = "abcd1234"ÀÏ ¶§ set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
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

//°æ·Î¿¡ '\\' ¶Ç´Â '/'°¡ È¥¿ëµÇ¾î »ç¿ëµÇ´Â °æ¿ì°¡ ÀÖÀ¸¹Ç·Î ÀÌ¸¦ °¨¾ÈÇØ¾ß ÇÑ´Ù.
//c:\\folder1\\folder2\\	=> c:\\folder1
//c:\\folder1\\folder2		=> c:\\folder1
//c:\\folder1\\				=> c:\\
//c:\\folder1				=> c:\\

CString	GetParentDirectory(CString sFolder)
{
	//"/"¿Í "\\"°¡ È¥¿ëµÈ °æ¿ì ÅëÀÏ½ÃÅ²´Ù.
	sFolder.Replace(_T("/"), _T("\\"));

	//sFolder ³¡¿¡ "\\"°¡ ºÙ¾ú´Ù¸é Á¦°ÅÇØÁØ´Ù.
	if (sFolder.Right(1) == _T("\\"))
		sFolder = sFolder.Left(sFolder.GetLength() - 1);

	//¸¸¾à ±¸ÇØÁø Æú´õ°¡ "c:"¿Í °°ÀÌ µå¶óÀÌºê ·çÆ®¶ó¸é ³¡¿¡ "\\"¸¦ ´Ù½Ã ºÙ¿©Áà¾ßÇÑ´Ù.
	if (sFolder.GetLength() == 2 && sFolder.GetAt(1) == ':')
	{
		sFolder += "\\";
		return sFolder;
	}

	//ÇØ´ç Æú´õÀÇ parent Æú´õ¸íÀ» ±¸ÇÑ´Ù.
	sFolder = sFolder.Left(sFolder.ReverseFind('\\'));

	//´Ù½Ã ±¸ÇØÁø Æú´õ°¡ "c:"¿Í °°ÀÌ µå¶óÀÌºê ·çÆ®¶ó¸é ³¡¿¡ "\\"¸¦ ´Ù½Ã ºÙ¿©Áà¾ßÇÑ´Ù.
	if (sFolder.GetLength() == 2 && sFolder.GetAt(1) == ':')
	{
		sFolder += "\\";
		return sFolder;
	}

	return sFolder;
}

//MAX_COMPUTERNAME_LENGTH(15) ±æÀÌ±îÁö¸¸ ¸®ÅÏµÊ¿¡ ÁÖÀÇ.
//GetComputerName API ÇÔ¼ö´Â Ç×»ó ´ë¹®ÀÚ·Î ¸®ÅÏÇÑ´Ù.
//±×³É È®½ÇÇÏ°Ô GetComputerNameString().MakeLower() µî°ú °°ÀÌ
//´ë¼Ò¹®ÀÚ¸¦ ¸íÈ®È÷ ÇÏ¿© ºñ±³ÇÏ´Â °ÍÀÌ ÁÁ´Ù.
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
//¿©·¯°÷¿¡ ÀÚÁÖ »ç¿ëÇÏ´Â Functions.h¿¡ ¾Æ·¡ ÇÔ¼ö¶§¹®¿¡ ¸Å¹ø WQL.h¸¦ Æ÷ÇÔ½ÃÅ°±â°¡ ±×·¡¼­
//ÀÌ ±â´ÉÀº °ÅÀÇ »ç¿ëÇÏÁö ¾ÊÀ¸¹Ç·Î ÁÖ¼®Ã³¸®ÇÏ°í ÇÊ¿äÇÑ °æ¿ì¿¡¸¸ »ç¿ëÇÏÀÚ.
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

	//½Ã¸®¾ó³Ñ¹ö°¡ Á¦Ç°¿¡ ºÎÂøµÈ(ADATA SSD) ½Ã¸®¾ó³Ñ¹ö¿Í´Â ´Þ¸® 2ÀÚ¸®¾¿ swapµÇ¾î ÀÖ´Ù.
 	for (int i = 0; i <= sHDDSerialNumber.GetLength() - 2; i += 2)
 	{
		cTemp = sHDDSerialNumber.GetAt(i);
		sHDDSerialNumber.SetAt(i, sHDDSerialNumber.GetAt(i + 1));
		sHDDSerialNumber.SetAt(i + 1, cTemp);
	}

	return sHDDSerialNumber;
}
*/

//¼ýÀÚ·Î ±¸¼ºµÈ ¹®ÀÚ¿­À» ÀÔ·Â¹Þ¾Æ Á¤ÇØÁø ¿¬»êÀ» ÇÑ ÈÄ µÚ¼¯ÀÎ ¼ýÀÚ¹®ÀÚ¿­À» ¸®ÅÏÇÑ´Ù.
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
// CString ¡æ TCHAR
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
	//unicode È¯°æ¿¡¼­ CastingÀº °¡´ÉÇÏÁö¸¸
	//½ÇÁ¦ LPCSTR ÀÎÀÚ·Î ³Ñ°Ü¼­ »ç¿ë ÇÏ´Ï ¹®Á¦ ¹ß»ý.
	CStringA strA(str);
	LPCSTR lpcstr = strA;
	return lpcstr;
#else
	//Multibyte È¯°æ¿¡¼­´Â Ä³½ºÆÃÀ¸·Î ¹®Á¦ ¾øÀÌ »ç¿ë °¡´É
	LPCSTR lpcstr = (LPSTR)(LPCTSTR)str;
	return lpcstr;
#endif
}

//
// CString ¡æ std::string
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
// Char ¡æ CString
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
// Char ¡æ TCHAR
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
// TCHAR ¡æ CString
//
CString TCHAR2CString(TCHAR* str)
{
	CString cStr;
	cStr.Format(_T("%s"), str);
	return cStr;
}

//
// TCHAR ¡æ Char
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

CString	string2CString(std::string s)
{
	CString cs(s.c_str());
	return cs;
}


PCWSTR CString2PCWSTR(CString str)
{
#ifdef _UNICODE
	PCWSTR p = LPCTSTR(str);
#else
	USES_CONVERSION;
	PCWSTR p = A2CW(LPCTSTR(str));
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

int	GetNumberFromFile(TCHAR* sfile)		//ÆÄÀÏÀ» ÀÐ¾î¼­ ÇÑ°³ÀÇ ¼ýÀÚ°ªÀ» ¸®ÅÏÇÑ´Ù. (ÆÄÀÏÀÌ ¾ø°Å³ª ³»¿ëÀÌ ¾øÀ¸¸é -99999 ¸¦ ¸®ÅÏÇÑ´Ù.)
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

bool WriteNumberToFile(TCHAR* sfile, int n)	//¼ýÀÚ°ªÀ» ÆÄÀÏ¿¡ ±â·ÏÇØÁØ´Ù.
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
	// Process °´Ã¼(CPU_INDEX)¿¡¼­ ÇØ´ç process ÀÌ¸§À» °®´Â ÇÁ·Î¼¼½ºÀÇ CPU °ªÀ» ¾ò´Â´Ù.
	if(GetCounterValue(CPU_INDEX, CPU_COUNTER, process, &cpuData, newVal) < 0)
		return -1;
	// È¹µæÇÑ ¼º´É Á¤º¸¸¦ ÀÌ¿ëÇØ¼­ CPU »ç¿ëÀ²À» °è»ê
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

double GetElapsedTime(__timeb32 pOldTime)	//pOldTime°ú ÇöÀç ½Ã°£ÀÇ Â÷ÀÌ °è»ê
{
	__timeb32	tCur;

	_ftime32(&tCur);

	time_t t0 = pOldTime.time * 1000 + pOldTime.millitm;
	time_t t1 = tCur.time * 1000 + tCur.millitm;

	return (double)(t1 - t0) / 1000.0 ;
}

CString	GetDayTimeCountString(CTimeSpan ts, bool bShowZero, bool bIncludeSec)	//ts°ªÀ» ³Ñ°Ü ¹Þ¾Æ "aÀÏ b½Ã°£ cºÐ dÃÊ" ÇüÅÂ·Î Ç¥½Ã
{
	CString str;
	CString sResult = _T("");

	if (ts.GetDays() || bShowZero)
	{
		str.Format(_T("%dÀÏ"), ts.GetDays());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetHours() || bShowZero)
	{
		str.Format(_T("%d½Ã°£"), ts.GetHours());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetMinutes() || bShowZero)
	{
		str.Format(_T("%dºÐ"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	if (bIncludeSec && (sResult != "" || ts.GetMinutes() || bShowZero))
	{
		str.Format(_T("%dÃÊ"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	sResult.TrimLeft();

	if (sResult == "")
	{
		if (bIncludeSec)
			sResult = "0ÃÊ";
		else
			sResult = "0ºÐ";
	}

	return sResult;
}

//ts°ªÀ» ³Ñ°Ü ¹Þ¾Æ "aÀÏ b½Ã°£ cºÐ dÃÊ" ÇüÅÂ·Î Ç¥½Ã
CString	GetDayTimeCountString(COleDateTimeSpan ts, bool bShowZero, bool bIncludeSec)
{
	CString str;
	CString sResult = _T("");

	if (ts.GetDays() || bShowZero)
	{
		str.Format(_T("%dÀÏ"), ts.GetDays());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetHours() || bShowZero)
	{
		str.Format(_T("%d½Ã°£"), ts.GetHours());
		sResult = sResult + " " + str;
	}

	if (sResult != "" || ts.GetMinutes() || bShowZero)
	{
		str.Format(_T("%dºÐ"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	if (bIncludeSec && (sResult != "" || ts.GetMinutes() || bShowZero))
	{
		str.Format(_T("%dÃÊ"), ts.GetMinutes());
		sResult = sResult + " " + str;
	}

	sResult.TrimLeft();

	if (sResult == "")
	{
		if (bIncludeSec)
			sResult = "0ÃÊ";
		else
			sResult = "0ºÐ";
	}

	return sResult;
}

bool IsAM(CTime t /*= 0*/)
{
	//t°ªÀÌ 0ÀÎ °æ¿ì´Â ÇöÀç ½Ã°¢À» ¾ò¾î¿Í¼­ ¿ÀÀü, ¿ÀÈÄ¸¦ ÆÇ´ÜÇØÁØ´Ù.
	if (t == 0)
		t = CTime::GetCurrentTime();
	
	if (t.GetHour() >= 0 && t.GetHour() < 12)
		return true;

	return false;
}

CString	GetDayOfWeekString(CTime t /*= NULL*/)
{
	if (t == NULL)
		t = CTime::GetCurrentTime();

	CString sWeek;

	switch (t.GetDayOfWeek())
	{
			case 1	:	sWeek = "ÀÏ¿äÀÏ";
						break;
			case 2	:	sWeek = "¿ù¿äÀÏ";
						break;
			case 3	:	sWeek = "È­¿äÀÏ";
						break;
			case 4	:	sWeek = "¼ö¿äÀÏ";
						break;
			case 5	:	sWeek = "¸ñ¿äÀÏ";
						break;
			case 6	:	sWeek = "±Ý¿äÀÏ";
						break;
			case 7	:	sWeek = "Åä¿äÀÏ";
						break;
	}

	return sWeek;
}

//ÇØ´ç ´ÞÀÇ ³¯Â¥¼ö ¸®ÅÏ
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
	//¿©±â¿¡¼­ srand¸¦ È£ÃâÇÏ¸é ¿øÇÏ´Â´ë·Î µ¿ÀÛÇÏÁö ¾Ê´Â´Ù. ¸ÞÀÎ¿¡¼­ ½ÃÀÛ½Ã ÇÑ¹ø¸¸ È£ÃâÇÏÀÚ.
	//srand(time(NULL));
	if (minimum > maximum)
		Swap(minimum, maximum);
	return minimum + rand() % (maximum - minimum + 1);
}

double random(double minimum, double maximum)
{
	//¿©±â¿¡¼­ srand¸¦ È£ÃâÇÏ¸é ¿øÇÏ´Â´ë·Î µ¿ÀÛÇÏÁö ¾Ê´Â´Ù. ¸ÞÀÎ¿¡¼­ ½ÃÀÛ½Ã ÇÑ¹ø¸¸ È£ÃâÇÏÀÚ.
	//srand(time(NULL));
	if (minimum > maximum)
		Swap(minimum, maximum);
	return minimum + (maximum - minimum) * rand() / (RAND_MAX + 1.0);
}
*/
//Áöµµ ÁÂÇ¥ <-> µµºÐÃÊ º¯È¯
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

//´ë°¢ °¢µµ·Î ¼öÆò, ¼öÁ÷ È­°¢À» ¼öÇÐÀûÀ¸·Î °è»êÇÑ´Ù.
//½ÇÁ¦ ·»Áî ½ºÆå°ú ¹Ýµå½Ã ÀÏÄ¡ÇÏÁø ¾Ê´Â´Ù.
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

//<ÇÁ·Î¼¼½º ÀÌ¸§À¸·Î ÇÁ·Î¼¼½º ID ¾ò±â>
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

// À©µµ¿ì ÇÚµé·Î ÇÁ·Î¼¼½º ¾ÆÀÌµð ¾ò±â   
ULONG ProcIDFromWnd(HWND hwnd)
{   
	ULONG idProc;   
	::GetWindowThreadProcessId(hwnd, &idProc);   
	return idProc;   
}

// ÇÁ·Î¼¼½º ¾ÆÀÌµð·Î À©µµ¿ì ÇÚµé ¾ò±â   
HWND GetHWNDbyPID(ULONG pid)
{   
	HWND tempHwnd = ::FindWindow(NULL,NULL); // ÃÖ»óÀ§ À©µµ¿ì ÇÚµé Ã£±â   

	while(tempHwnd != NULL)   
	{   
		//TRACE("tempHwnd = %p\n", tempHwnd);
		if(::GetParent(tempHwnd) == NULL) // ÃÖ»óÀ§ ÇÚµéÀÎÁö Ã¼Å©, ¹öÆ° µîµµ ÇÚµéÀ» °¡Áú ¼ö ÀÖÀ¸¹Ç·Î ¹«½ÃÇÏ±â À§ÇØ   
			if(pid == ProcIDFromWnd(tempHwnd))   
				return tempHwnd;   
		tempHwnd = ::GetWindow(tempHwnd, GW_HWNDNEXT); // ´ÙÀ½ À©µµ¿ì ÇÚµé Ã£±â   
	}   
	return NULL;
}

//ÃâÃ³: https://smok95.tistory.com/300?category=28201 [Only YOUng:Æ¼½ºÅä¸®]
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
			//´ë¼Ò¹®ÀÚ ±¸ºÐÇÏÁö ¾Ê°í ½ÇÇàÇÁ·Î±×·¥ ÀÌ¸§ÀÌ °°ÀºÁö È®ÀÎ..
			if ((bCaseSensitive && _tcsicmp(pe.szExeFile, sExeFile) == 0) ||
				(!bCaseSensitive && _tcscmp(pe.szExeFile, sExeFile) == 0))
			{
				//ÀÚ±â ÀÚ½ÅÀº Á¦¿ÜÇÏ°í...
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

//<ÀÀ¿ë - ÇÁ·Î¼¼½º°¡ ½ÇÇà ÁßÀÎÁö¸¦ Ã¼Å©>
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

			return true; // ½ÇÇàÁßÀÌ¸é True¸¦ ¹ÝÈ¯

	} while (Process32Next(hProcessSnap, &pe32));


	CloseHandle(hProcessSnap);

	return false; //½ÇÇàÁßÀÌ ¾Æ´Ï¸é False¸¦ ¹ÝÈ¯
} 

//ÇÁ·Î¼¼½º °­Á¦ Á¾·á.
//return value : 1 : killed, 0 : fail to kill, -1 : not found
int ProcessKill(CString szProcessName)
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	HANDLE hProcess = 0; 
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof (pEntry);
	BOOL bRes = Process32First(hSnapShot, &pEntry);
	int result;

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

	result = -1;
end:
	CloseHandle(hProcess);
	CloseHandle(hSnapShot);

	return result;
	/*
	HANDLE hndl = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	DWORD dwsma = GetLastError();
	HANDLE hHandle; 

	DWORD dwExitCode = 0;

	PROCESSENTRY32  procEntry={0};
	procEntry.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hndl,&procEntry);

	while(1)
	{
		if(!strcmp(procEntry.szExeFile,szProcessName))
		{
			hHandle = ::OpenProcess(PROCESS_ALL_ACCESS,0,procEntry.th32ProcessID);

			if(::GetExitCodeProcess(hHandle,&dwExitCode))
			{
				if(!::TerminateProcess(hHandle,dwExitCode))
				{
					return 0;
				}
				else
					return 1;
			}   
		}
		if(!Process32Next(hndl,&procEntry))
		{
			return -1;
		}
	}

	return 1;
	*/
}

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

//r¿¡¼­ except¿µ¿ªÀ» Á¦¿ÜÇÏ°í crÄÃ·¯·Î Ã¤¿î´Ù.
void fill_except_rect(CDC* pDC, CRect r, CRect except, COLORREF cr)
{
	//Ã¤¿öÁö´Â ¿µ¿ª È®ÀÎ¿ë
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

//side ¹è¿­ÀÇ ÀÎµ¦½º´Â resizeÇÏ´Â ¿µ¿ª ÀÎµ¦½º·Î¼­
//DefWindowProcÀÇ µÎ¹øÂ° ÆÄ¶ó¹ÌÅÍ¿¡ (SC_SIZE + m_nSideIndex)·Î ¾²ÀÌ¹Ç·Î ±× Â÷·Ê¸¦ µû¸¥´Ù.
void GetSideRect(CRect src, CRect *side, int margin)
{
	side[corner_left] = CRect(src.left, src.top + margin, src.left + margin, src.bottom - margin);
	side[corner_right] = CRect(src.right - margin, src.top + margin, src.right, src.bottom - margin);
	side[corner_top] = CRect(src.left + margin, src.top, src.right - margin, src.top + margin);
	side[corner_topleft] = CRect(src.left, src.top, src.left + margin, src.top + margin);
	side[corner_topright] = CRect(src.right - margin, src.top, src.right, src.top + margin);
	side[corner_bottom] = CRect(src.left + margin, src.bottom - margin, src.right - margin, src.bottom);
	side[corner_bottomleft] = CRect(src.left, src.bottom - margin, src.left + margin, src.bottom);
	side[corner_bottomright] = CRect(src.right - margin, src.bottom - margin, src.right, src.bottom);
}

//8±ºµ¥ÀÇ ÄÚ³Ê¿Í »çÀÌµå ¿µ¿ª¿¡ ÇØ´ç Á¡ÀÌ ÀÖ´ÂÁö¸¦ °Ë»çÇÏ¿© ÇØ´ç ¿µ¿ªÀÇ ÀÎµ¦½º¸¦ ¸®ÅÏÇÑ´Ù.
//ÀÎµ¦½º´Â CORNER_INDEXÀÇ Â÷·ÊÀÌ¸ç ÀÌ´Â DefWindowProc¿¡¼­ »ç¿ëÇÏ´Â Â÷·Ê¿Í µ¿ÀÏÇÏ´Ù.
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

//½ÃÀÛÁ¡À» ÁÖ¸é Á¤»ç°¢ÇüÀ» ÀÌ·ç´Â ³¡Á¡ ÁÂÇ¥¸¦ ¸®ÅÏÇÑ´Ù.
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
// PID·Î ÇÁ·Î¼¼½º Ã£±â
//-----------------------------------------------------------------
bool CheckProcessUsingPID(unsigned long pid) // unsigned long = DWORD
{
	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	if (!snapshotHandle)
		return false;

	//-----------------------------------------------------------------
	// ÇÁ·Î¼¼½º¸¦ Ã£´Â´Ù
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
// À©µµ¿ì Å¸ÀÌÆ², Å¬·¡½º ÀÌ¸§À¸·Î ÇÁ·Î¼¼½º Ã£±â
//-----------------------------------------------------------------
bool CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName)
{
	if (!className && !titleName)
		return false;

	//-----------------------------------------------------------------
	// Å¬·¡½º ÀÌ¸§ ¹× Å¸ÀÌÆ² ÀÌ¸§À¸·Î À©µµ¿ì¸¦ Ã£´Â´Ù
	//-----------------------------------------------------------------
	HWND targetWindow = FindWindow(className ? className : NULL, titleName ? titleName : NULL);
	if (targetWindow)
	{
		//-----------------------------------------------------------------
		// Ã£Àº À©µµ¿ì ÇÚµé·Î PID, TID¸¦ ÃëµæÇÏ¿© ÇÁ·Î¼¼½º¸¦ Ã£´Â´Ù
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
// ÇÁ·Î¼¼½º ÀÌ¸§À¸·Î ÇÁ·Î¼¼½º Ã£±â
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
	// ÇÁ·Î¼¼½º¸¦ Ã£´Â´Ù
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

	//¾Æ·¡ LoadImage ÇÔ¼ö¸¦ ÅëÇØ¼­ ¾ÆÀÌÄÜ ÆÄÀÏÀ» ºÒ·¯¿Â °æ¿ì
	//ÇÁ·Î±×·¥ Á¾·á ½Ã ¹Ýµå½Ã DestroyIconÀ» ÇØÁà¾ß ÇÑ´Ù.
	//±×·¡¼­ DestroyIcon ¾ÈÇØµµ µÇ´Â LR_SHARED¸¦ »ç¿ëÇß´Ù.
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

	//width ¶Ç´Â height¸¦ nµîºÐÇØ¼­ gradient fillÀ» ÇÑ´Ù.
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

//¸ð´ÏÅÍÀÇ ÇÑÂÊ¿¡ ºÙÀº »ç°¢ÇüÀ» »õ·Î¿î Å©±â·Î º¯°æÇÒ °æ¿ì ºÙÀº »óÅÂ¸¦ À¯ÁöÇÏ°í º¯°æÇÒ ÇÊ¿ä°¡ ÀÖÀ» °æ¿ì »ç¿ë.
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


//rTarget¿¡ Á¢ÇÏ´Â dRatioÀÎ ÃÖ´ë »ç°¢ÇüÀ» ±¸ÇÑ´Ù.
//attach_left µîÀÇ ¿É¼ÇÀ» ÁÙ ÇÊ¿ä°¡ ÀÖ´Ù.
CRect GetRatioRect(CRect rTarget, int w, int h, int attach)
{
	return GetRatioRect(rTarget, (double)w / (double)h, attach);
}

//rTarget¿¡ Á¢ÇÏ´Â dRatioÀÎ ÃÖ´ë »ç°¢ÇüÀ» ±¸ÇÑ´Ù.
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

//ÁÖ¾îÁø Á¡µéÀ» Æ÷ÇÔÇÏ´Â ÃÖ´ë »ç°¢ÇüÀ» ±¸ÇÑ´Ù.
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


//´Ù°¢ÇüÀÇ ³ÐÀÌ¸¦ ±¸ÇÑ´Ù. ´Ü, º¯ÀÌ ÇÏ³ª¶óµµ ±³Â÷µÇ¸é ¼º¸³ÇÏÁö ¾Ê´Â´Ù.
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

void Trim(std::deque<CString>* dq)
{
	for (int i = 0; i < dq->size(); i++)
		dq->at(i).Trim();
}

//src¹®ÀÚ¿­¿¡¼­ chars¸¦ ¸ðµÎ Á¦°ÅÇÑ´Ù.
void remove_chars(CString &src, CString chars)
{
	for (int i = 0; i < chars.GetLength(); i++)
		src.Remove(chars[i]);
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

//Â÷·®Á¤º¸ÆÄÀÏ(xml) ÀúÀå °ü·Ã
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

//32ºñÆ®ÀÎ dw¿¡ µé¾îÀÖ´Â R, G, B¸¦ ÃßÃâÇÏ¿© 16ºñÆ®(5+6+5) ÄÃ·¯·Î ¸®ÅÏÇÑ´Ù.
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

//µÎ DWORD¸¦ WORD·Î º¯È¯ÇÏ¿© ÇÏ³ªÀÇ DWORD·Î º¯È¯ÇÑ´Ù.
DWORD RGB24ToRGB565(DWORD dw1, DWORD dw2)
{
	DWORD dw = RGB24ToRGB565(dw1);
	dw <<= 16;

	dw |= RGB24ToRGB565(dw2);
	return dw;
}

//RGB565·Î Ãà¾àµÈ WORD¸¦ ´Ù½Ã 24ºñÆ® RGB(DWORD)·Î º¹¿ø½ÃÅ²´Ù.
DWORD RGB565ToRGB24(WORD wd)
{
	//565·Î ¸¸µé¾îÁø wd¿¡¼­ ´Ù½Ã rgb·Î º¹¿ø½ÃÄÑº¸ÀÚ.
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

//¿øÁ¡¿¡¼­ º¤ÅÍ b°¡ º¤ÅÍ aÀÇ ¹Ý½Ã°è ¹æÇâÀÌ¸é ¾ç¼ö, ½Ã°è¹æÇâÀÌ¸é À½¼ö, ÆòÇàÀÌ¸é 0À» ¹ÝÈ¯ ÇÑ´Ù.
double ccw(vector2 a, vector2 b)
{
    return a.cross(b);
}

//Á¡ p¸¦ ±âÁØÀ¸·Î º¤ÅÍ b°¡ º¤ÅÍ aÀÇ ¹Ý½Ã°è ¹æÇâÀÌ¸é ¾ç¼ö, ½Ã°è¹æÇâÀÌ¸é À½¼ö, ÆòÇàÀÌ¸é 0À» ¹ÝÈ¯ ÇÑ´Ù.
double ccw(vector2 p, vector2 a, vector2 b)
{
    return ccw(a-p, b-p);
}

// - Á¡ a, b¸¦ Áö³ª´Â Á÷¼±°ú Á¡ c, d¸¦ Áö³ª´Â Á÷¼±ÀÇ ±³Á¡À» x¿¡ ¹ÝÈ¯ÇÑ´Ù.
// - µÎ Á÷¼±ÀÌ ÆòÇàÀÌ¸é(°ãÄ¡´Â °æ¿ì Æ÷ÇÔ) °ÅÁþÀ», ¾Æ´Ï¸é ÂüÀ» ¹ÝÈ¯ÇÑ´Ù.
// ÃâÃ³ : http://bowbowbow.tistory.com/17
bool GetIntersectionPoint(vector2 a, vector2 b, vector2 c, vector2 d, vector2& x)
{
    double det = (b-a).cross(d-c);
    //µÎ¼±ÀÌ ÆòÇàÀÎ °æ¿ì
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

//4Á¡À¸·Î ±¸¼ºµÈ µÎ ¼±ºÐÀÌ ¼­·Î ±³Â÷ÇÏ´ÂÁö ¿©ºÎ¸¦ ÆÇº°ÇÑ´Ù.
bool IsIntersect2LineSegment(vector2 a, vector2 b, vector2 c, vector2 d)
{
    double ab = ccw(a, b, c)*ccw(a, b, d);
    double cd = ccw(c, d ,a)*ccw(c, d, b);

    //µÎ ¼±ºÐÀÌ ÇÑ Á÷¼±¿¡ À§¿¡ ÀÖ°Å³ª ³¡Á¡ÀÌ °ãÄ¡´Â °æ¿ì
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

//Á¡ a, b¿Í Á¡ c, d°¡ ÆòÇàÇÑ µÎ ¼±ºÐ ÀÏ ¶§ ÀÌµéÀÌ ÇÑ Á¡¿¡¼­ °ãÄ¡´ÂÁö È®ÀÎÇÑ´Ù.
bool paralleSegments(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p){
    if(b < a) std::swap(a,b);
    if(d < c) std::swap(c,d);

    //ÇÑ Á÷¼±À§¿¡ ¾ø°Å³ª µÎ ¼±ºÐÀÌ °ãÄ¡Áö ¾Ê´Â °æ¿ì¸¦ ¿ì¼± °É·¯³½´Ù. º»¹®ÀÇ 1¹ø °ü°èÀÎ °æ¿ìÀÌ´Ù.
    if(ccw(a, b, c) != 0 || b < c || d < a) return false;

    //µÎ ¼±ºÐÀÌ È®½ÇÈ÷ °ãÄ£´Ù¸é ±³Â÷Á¡ ÇÏ³ª¸¦ Ã£´Â´Ù.
    if(a<c) p = c;
    else p = a;
    return true;
}


// - p°¡ µÎ Á¡ a, b¸¦ °¨½Î¸é¼­ °¢ º¯ÀÌ x, yÃà¿¡ ÆòÇàÇÑ ÃÖ¼Ò»ç°¢Çü ³»ºÎ¿¡ ÀÖ´ÂÁö È®ÀÎÇÑ´Ù.
// a, b, p´Â ÀÏÁ÷¼± »ó¿¡ ÀÖ´Ù°í °¡Á¤ÇÑ´Ù.
bool inBoundingRectangle(vector2 p, vector2 a, vector2 b){
    if(b < a) std::swap(a, b);
    return p == a || p == b || (a <p && p < b);
}


// - µÎ Á¡ a, b¸¦ Áö³ª´Â ¼±ºÐ°ú µÎ Á¡ c, b¸¦ Áö³ª´Â ¼±ºÐÀ» p¿¡ ¹ÝÈ¯ÇÑ´Ù.
// - ±³ÁüÀÌ ¿©·¯°³ÀÏ °æ¿ì ¾Æ¹«Á¡ÀÌ³ª ¹ÝÈ¯ÇÑ´Ù.
bool segmentIntersection(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p){
    //µÎ Á÷¼±ÀÌ ÆòÇàÀÎ °æ¿ì¸¦ ¿ì¼± ¿¹¿Ü·Î Ã³¸®ÇÑ´Ù.
    if(!GetIntersectionPoint(a, b, c, d, p))
        return paralleSegments(a, b, c, d, p);
    //p°¡ µÎ ¼±ºÐ¿¡ Æ÷ÇÔµÇ¾î ÀÖ´Â °æ¿ì¿¡¸¸ ÂüÀ» ¹ÝÈ¯ÇÑ´Ù.
    return inBoundingRectangle(p, a, b) && inBoundingRectangle(p, c, d);
}


//»ï°¢Çü ¿ÜÁ¢¿øÀÇ Áß½ÉÀ» ±âÇÏÇÐÀûÀ¸·Î ±¸ÇÑ´Ù.(http://kipl.tistory.com/113)
int circumCenter(CPoint A, CPoint B, CPoint C, double *xc, double *yc)
{
	double ax = A.x - C.x ;
	double ay = A.y - C.y ;
	double bx = B.x - C.x ;
	double by = B.y - C.y ;
	double asq = ax * ax + ay * ay;
	double bsq = bx * bx + by * by;
	double ccw = ax * by - ay * bx;

	 // ¼¼ Á¡ÀÓ ÀÏÁ÷¼± À§¿¡ ÀÖÁö ¾Ê´Â °æ¿ì; ÀÌ °æ¿ì¸¸ ¿ÜÁ¢¿øÀÌ Á¤ÀÇµÊ;
	if (ccw == 0.)
		return 0;

	*xc = C.x + (by * asq - ay * bsq) / (2. * ccw) ;
	*yc = C.y + (-bx * asq + ax * bsq) / (2. * ccw) ;
	return 1;
}

//»ï°¢Çü ¿ÜÁ¢¿øÀÇ Áß½ÉÀ» ´ë¼öÀûÀ¸·Î ±¸ÇÑ´Ù.
int circumCenter2(CPoint P, CPoint Q, CPoint R, double *xc, double *yc)
{
	double A = Q.x - P.x;
	double B = Q.y - P.y;
	double C = R.x - P.x;
	double D = R.y - P.y;
	double E = A * (P.x + Q.x) + B * (P.y + Q.y);
	double F = C * (P.x + R.x) + D * (P.y + R.y);
	double G = 2. * (A * D - B * C);

	// ¼¼ Á¡ÀÌ ÀÏÁ÷¼±¿¡ ³õÀÌÁö ¾Ê´Â °æ¿ì; ÀÌ °æ¿ì¸¸ ¿ÜÁ¢¿øÀÌ Á¤ÀÇµÈ´Ù;
	if (G == 0.)
		return 0;

	*xc = (D * E - B * F) / G;
	*yc = (A * F - C * E) / G;
	return 1;
}

//»ï°¢Çü ¿ÜÁ¢¿øÀÇ ¹ÝÁö¸§À» ±¸ÇÑ´Ù.
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


//ÀÓÀÇ Á¡ÀÌ Æú¸®°ï ³»¿¡ Á¸ÀçÇÏ´ÂÁö ÆÇº°
//ÀÌ ÇÔ¼ö ¾Æ·¡ÀÇ PtInPolygon0 ~ PtInPolygon2±îÁö´Â ¹Ì¿ÏÀÎÁö ¹ºÁö ¸ð¸£°ÚÀ¸³ª ¿Ã¹Ù¸¥ ÆÇº°À» ÇÏÁö ¸øÇÑ´Ù.
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
	USES_CONVERSION; //convert¸¦ À§ÇÑ ¸ÅÅ©·Î
	pbstr=OLE2A(bstr); //bstrÀ» char·Î convert
	return CString(pbstr);
} 

BSTR CStringTobstr(CString str) 
{
	return str.AllocSysString(); //CStringÀ» bstr·Î convert
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
//CString ==> CComVariant·Î º¯È¯ 
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

//256 gray bmi »ý¼º
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

//main¿¡¼­ EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); ¸¦ ½ÇÇàÇÏ°í
//ÀÌ ÆÄÀÏ¿¡ Àü¿ªº¯¼ö·Î ¼±¾ðµÈ g_dqMonitor¸¦ ÀÌ¿ëÇÏ¸é µÈ´Ù.
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	// ¸ð´ÏÅÍ Á¤º¸¸¦ °¡Á®¿Ã ±¸Á¶Ã¼
	MONITORINFOEX mi;
	mi.cbSize = sizeof(MONITORINFOEX);

	// ¸ð´ÏÅÍ ÇÚµéÀ» »ç¿ëÇÏ¿© ÇØ´ç ¸ð´ÏÅÍ Á¤º¸¸¦ °¡Á®¿Â´Ù.
	GetMonitorInfo(hMonitor, &mi);

	CString str;

	// ÁÖ¸ð´ÏÅÍ·Î ¼³Á¤µÈ ¸ð´ÏÅÍ Á¤º¸ÀÎÁö¸¦ Ã¼Å©ÇÑ´Ù.
	if (mi.dwFlags & MONITORINFOF_PRIMARY) 
	{
		str.Format(_T("hMonitor = %X,  ÁÂÇ¥ : [ (%04d, %04d) - (%04d, %04d) ], < Primary-Monitor > %s"), 
					hMonitor, lprcMonitor->left,lprcMonitor->top,lprcMonitor->right,lprcMonitor->bottom, mi.szDevice);
	}
	else
	{
		str.Format(_T("hMonitor = %X,  ÁÂÇ¥ : [ (%04d, %04d) - (%04d, %04d) ], %s"), 
					hMonitor, lprcMonitor->left,lprcMonitor->top,lprcMonitor->right,lprcMonitor->bottom, mi.szDevice);
	}

	g_dqMonitors.push_back(lprcMonitor);

	TRACE(_T("%s\n"), str);

	return TRUE;
}

//2D ´ÜÀÏ ¿µ»ó¿¡¼­ ÀÌ¹Ì ¾Ë·ÁÁø ¼³Á¤°ªÀ» ±âÁØÀ¸·Î ¿µ»ó³»ÀÇ ÇÑ Á¡°ú ·»Áî¿ÍÀÇ °Å¸®¸¦ °è»ê(by sucwon)
//´Ü, Â÷·®¿¡ ÀåÂøµÈ Ä«¸Þ¶ó¿¡¼­ ÃÔ¿µµÈ ¿µ»óÀÌ¹Ç·Î ÇÇ»çÃ¼¿Í ·»Áî¿ÍÀÇ °Å¸®°¡ ¾Æ´Ñ Â÷Ã¼¿ÍÀÇ °Å¸®°¡ ´õ Á¤È®ÇÑ °ªÀÌ¸ç
//µû¶ó¼­ ·»ÁîÀÇ À§Ä¡ÀÎ ¿µ»ó ÇÏ´Ü ¼¾ÅÍ°¡ ¾Æ´Ñ ¿µ»óÀÇ ÇÏ´Ü°ú ÇÇ»çÃ¼ÀÇ ¼öÁ÷ °Å¸®°¡
//¿ÀÈ÷·Á ´õ Á¤È®ÇÑ °è»êÀÌ µÈ´Ù.
double getObjectDistance(int width, int height, int vanishing_y, int x, int y, int cam_height, double *dx, double *dy, int cali_width, int cali_height, double fl_x, double fl_y)
{
		//// Focal Length (4°³ÀÇ »ó¼ö°ªÀº Çù°¢ ±âÁØÀÌ¸ç Ä«¸Þ¶ó¸¶´Ù ´Ù¸¥ °ªÀ» °¡Áü)
/*
		double fl_x = 2361.130;
		double fl_y = 2357.436;

		//// ÁÖÁ¡
		double c_x = 1066.161;
		double c_y = 407.246;
*/
/*
		double tilt = atan2(vanishing_y - c_y, fl_y);				// 1¹øÂ° parameter ´Â ÁÖÁ¡, 2¹øÂ° parameter´Â Camera Focal Length
		double m_x = (x - c_x) / fl_x;								// Á¤±ÔÁÂÇ¥ (x)
		double m_y = (y - c_y) / fl_y;								// Á¤±ÔÁÂÇ¥ (y)

		double C1P1 = (double)cam_height * tan((PI/2) + tilt - atan(m_y));	// C`P`
		double CP1 = sqrt(((double)cam_height * (double)cam_height) + (C1P1*C1P1));					// CP`
		double Cp1 = sqrt(1 + (m_y*m_y));
		double PP1 = m_x * CP1 / Cp1;

		double real_distance = sqrt((C1P1*C1P1) + (PP1*PP1)) / 100.0f;

		return real_distance;
*/
	//Ä«¸Þ¶ó Ä¶¸®ºê·¹ÀÌ¼Ç ¾ÛÀ» ÅëÇØ¼­ Á÷Á¢ ±¸ÇØ¾ß ÇÑ´Ù.
	//ÁÖÁ¡Àº º¸Åë ÀÔ·Â ¿µ»óÀÇ 1/2ÀÌ´Ù.
	double c_x = (double)width / 2.0;	//691.448;
	double c_y = (double)height / 2.0;	//404.974;

	//1920, 1080Àº Ä¶¸®ºê·¹ÀÌ¼Ç¿¡ »ç¿ëÇÑ Ä«¸Þ¶ó ¿µ»óÀÇ Å©±âÀÌ´Ù.
	double tilt = atan2((vanishing_y/(double)(height)*((double)(cali_height))) - c_y, fl_y);
	double csh = (double)cam_height / 10.0;                              // Ä«¸Þ¶ó ¼³Ä¡³ôÀÌ
	double m_x = ((x / (double)(width) * (double)cali_width) - c_x) / fl_x;                        // Á¤±ÔÁÂÇ¥ (x)
	double m_y = ((y / (double)(height)*(double)(cali_height)) - c_y) / fl_y;                        // Á¤±ÔÁÂÇ¥ (y)

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



//gray °è¿­ÀÎÁö
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
		integral_img_current_line[-1] = 0;	//	°¡Àå ¿ÞÂÊ ¿­À» 0À¸·Î ¼ÂÆÃ

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
//¾ÆÁ÷ Å×½ºÆ® ÇÏÁö ¾ÊÀ½. Ãà¼Ò¸¸ °¡´É? È®´ë, Ãà¼Ò ¸ðµÎ °¡´É? skipÀÌ ÀÖ´Â°É·Î ºÁ¼­ Ãà¼Ò¸¸ °¡´ÉÇÒµíÇÏ´Ù.
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

//gray image¸¦ 3Ã¤³Î ¶Ç´Â 4Ã¤³Î ÀÌ¹ÌÁö Æ÷¸ËÀ¸·Î º¹»çÇØÁØ´Ù.
void gray2color(uint8_t *gray, int gray_width, int gray_height, uint8_t *dst, int dst_width, int dst_height, int dst_ch, uint8_t alpha)
{
	int x, y, k;
	uint8_t *src = gray;
	bool resized = false;

	//gray¿Í dstÀÇ Å©±â°¡ ´Ù¸£¸é resizeÇÑ ÈÄ º¹»çÇØ¾ß ÇÑ´Ù.
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

//3Ã¤³Î ¶Ç´Â 4Ã¤³Î ÀÌ¹ÌÁö¸¦ ´ÜÀÏ Èæ¹é ÀÌ¹ÌÁö·Î º¹»çÇÑ´Ù.
//dst´Â ¹Ýµå½Ã ÇÒ´çµÈ ¸Þ¸ð¸®ÁÖ¼ÒÀÌ¾î¾ß ÇÑ´Ù.
void color2gray(uint8_t *src, int src_width, int src_height, int src_ch, uint8_t *dst, int dst_width, int dst_height)
{
	if (src_ch < 3 || src_ch > 4)
		return;

	int x, y;
	int r, g, b;
	uint8_t *gray = dst;
	bool resized = false;
	
	//src¿Í dstÀÇ Å©±â°¡ ´Ù¸£¸é gray·Î º¯È¯ÇÑ ÈÄ resizeÇØÁà¾ß ÇÑ´Ù.
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

//ÇöÀç´Â 1Ã¤³Î ¿µ»ó¸¸ Áö¿øµÈ´Ù.
//¿ì¼± ±ÞÇÏ°Ô -90µµ¸¸ ±¸ÇöÇÑ´Ù.
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

//opencvÀÇ flip°ú °°Àº µ¿ÀÛÀÌÁö¸¸ ÀÌ¸§ Ãæµ¹À» ÇÇÇÏ±â À§ÇØ mirror¶ó´Â ÀÌ¸§À» »ç¿ëÇÔ.
//ÇöÀç´Â 1Ã¤³Î ¿µ»ó¸¸ Áö¿øµÈ´Ù.
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

//µÎ »ç°¢ÇüÀÇ °ãÄ¡´Â ¿µ¿ªÀ» ¸®ÅÏÇÑ´Ù.
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

	//¡è Àü¿ª »ó¼ö

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

//¸Þ¸ð¸® ÀÏºÎ º¹»ç ÇÔ¼ö
//´Ü, srcÀÇ µ¥ÀÌÅÍ°¡ ¿¬¼ÓµÈ ¸Þ¸ð¸®»ó¿¡ ÀúÀåµÇ¾î ÀÖ¾î¾ß ÇÏ´Â Á¦¾àÀÌ ÀÖ´Ù.
//srcÀÇ ÀÏºÎ ºí·°À» º¹»ç. Å×½ºÆ® ÇÊ¿äÇÔ.
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

//src¿¡¼­ roi ¿µ¿ªÀ» Àß¶ó¼­ dst_width * dst_height Å©±â·Î resize ½ÃÅ²´Ù.
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

//ÃâÃ³: http://boongubbang.tistory.com/255 [Boongubbang]
//unicode¸¦ Áö¿øÇÏ´Â ÇÔ¼ö¶ó¼­ Å×½ºÆ® ¹Ì¿Ï!
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

				//ÇÔ¼ö ¿øÇüÀ» quicksort(T *v, ...)¿Í °°ÀÌ Á¤ÀÇÇØ¼­ »ç¿ëÇÏ¸é ¾Æ·¡ ¹®Àå¿¡¼­ µð¹ö±ë ¿¡·¯°¡ ¹ß»ýÇÑ´Ù.
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

//OnInitDialog¿Í °°ÀÌ ÃÖÃÊ ÇÑ¹ø ½ÇÇàµÇ´Â ÇÔ¼ö³»¿¡¼­ È£ÃâÇÑ´Ù.
//±â¾ïµÈ ÁÂÇ¥´ë·Î º¹¿øÇÏ¿© Ç¥½ÃÇÑ´Ù.
//´Ü, ±× Å©±â°¡ ¸ð´ÏÅÍ ¹ÛÀÌ¸é CenterWindow()¸¦,
//Å©±â°¡ invalidÇÏ´Ù¸é ¿ø·¡ Å©±â·Î Ç¥½ÃÇÑ´Ù.
void RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection, bool use_maximize, bool resize_window)
{
	CRect	rc;
	CString sSection = _T("screen");

	//resize windowÀÎÁö ÀÚµ¿ Ã¼Å©ÇÏ´Â ÄÚµå°¡ ¾È¸ÔÈ÷´ÂµíÇÏ´Ù.
	//bool is_resizable_window = ((::GetWindowLongPtr(pWnd->m_hWnd, GWL_STYLE) & WS_THICKFRAME) == WS_THICKFRAME);

	if (sSubSection != "")
		sSection = sSubSection + "\\screen";	//½½·¡½Ã°¡ ¾Æ´Ñ ¿ª½½·¡½Ã¸¦ ½á¾ß ÇÑ´Ù.

	rc.left		= pApp->GetProfileInt(sSection, _T("left"), 0);
	rc.top		= pApp->GetProfileInt(sSection, _T("top"), 0);
	rc.right	= pApp->GetProfileInt(sSection, _T("right"), 0);
	rc.bottom	= pApp->GetProfileInt(sSection, _T("bottom"), 0);

	if (rc.IsRectNull())
	{
		pWnd->CenterWindow();
		return;
	}

	//»ç°¢Çü Á¤º¸°¡ À¯È¿ÇÏ°í ±× Å©±âµµ 10x10À» ³Ñ´Â´Ù¸é ±× Å©±â·Î º¹¿ø½ÃÅ°°í
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

//OnBnClickedCancel()°ú °°ÀÌ ÇÁ·Î±×·¥ÀÌ Á¾·áµÉ ¶§ È£ÃâÇÑ´Ù.
//OnWindowPosChanged()¿¡¼­ ¸Å¹ø È£ÃâÇß¾úÀ¸³ª 
//OnInitDialog¿¡¼­ RestoreWindowPosition¸¦ È£ÃâÇÒ ¶§
//OnWindowPosChanged°¡ È£ÃâµÇ¸é¼­ maximized Á¤º¸°¡ false·Î ÃÊ±âÈ­µÇ¾î ¹ö¸°´Ù.
//sSubSectionÀÌ Á¸ÀçÇÏ¸é ±× ÀÌ¸§¿¡ "\\screen"À» ºÙ¿©¼­ ÀúÀåÇÑ´Ù.
void SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection)
{
	if (pWnd->IsWindowVisible() == false ||	pWnd->IsIconic())
		return;

	CRect	rc;
	CString sSection = _T("screen");

	if (sSubSection != "")
		sSection = sSubSection + "\\screen";	//½½·¡½Ã°¡ ¾Æ´Ñ ¿ª½½·¡½Ã¸¦ ½á¾ß ÇÑ´Ù.

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

//init_YUV_lookup_table(); ÇÔ¼ö¸¦ ¹Ýµå½Ã ÇÑ¹ø È£ÃâÇÑ ÈÄ »ç¿ëÇÒ °Í.
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

	// À©µµ¿ì¿¡¼­´Â ¿µ»óÀÇ »óÇÏ°¡ °Å²Ù·Î ÀúÀåµÇÁö ¶§¹®¿¡ ¾Æ·¡¿Í °°ÀÌ ÄÚµå ÀÛ¼º.
	//matÀ» »ç¿ëÇÏ¿© convertingÇÏ´Â °æ¿ì ¾Æ·¡ ÄÚµå°¡ µÚÁýÈù´Ù.
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

	// ÀÏ¹ÝÀûÀÎ °æ¿ì ¾Æ·¡ÀÇ ÄÚµå »ç¿ëÇÔ.
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
//ÀÌ ÇÔ¼ö´Â yv12¿ëÀÌ ¾Æ´Ñ°Í°°´Ù. Á» ´õ È®ÀÎÇØºÁ¾ß ÇÑ´Ù.
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

		//¿À¸¥ÂÊ ½¬ÇÁÆ® ¿¬»ê 2¹ø
		value = ((int)u[pos0]+(int)u[pos1]+(int)u[pos2]+(int)u[pos3])>>2;
		Clamp(value, 0, 255); 
		U[addr] = (BYTE)value;

		//Áï ³ª´©±â 4 : Æò±Õ³»±â
		value = ((int)v[pos0]+(int)v[pos1]+(int)v[pos2]+(int)v[pos3])>>2;
		Clamp(value, 0, 255); 
		V[addr] = (BYTE)value;
		addr++;
	}
	return  true;
}

//Å×½ºÆ® ÇÊ¿äÇÔ.
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

	//È­¸é¿¡ Ç¥½ÃµÈ ¿µ»óÀÇ l,t°ªÀ» »©¼­ È®´ë ¿µ»ó³»¿¡¼­ÀÇ »ó´ëÁÂÇ¥·Î º¯È¯ÇÑ ÈÄ
	*x -= rDisplayedImageRect.left;
	*y -= rDisplayedImageRect.top;

	//È®´ëµÈ ºñÀ²·Î ³ª´©°í
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

	//È®´ëµÈ ºñÀ²À» °öÇÏ°í
	*x *= dZoom;
	*y *= dZoom;

	//¿É¼Â¸¸Å­ »ó´ëÁÂÇ¥·Î ÀÌµ¿½ÃÅ²´Ù.
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

//strÀÇ from À§Ä¡ ÀÌÈÄ¿¡ ÀÖ´Â ¼ýÀÚ ¿µ¿ª°ªÀ» num¿¡ ³Ö¾îÁÖ°í ¼ýÀÚ ½ÃÀÛÀ§Ä¡¸¦ returnÇÑ´Ù.
//¸®ÅÏµÇ´Â ¼ýÀÚÀÇ ½ÃÀÛÀ§Ä¡´Â fromºÎÅÍ°¡ ¾Æ´Ñ str ÀüÃ¼ ¹®ÀÚ¿­¿¡¼­ÀÇ À§Ä¡ÀÌ´Ù.
//¼ýÀÚ ¾Õ¿¡ ºÙÀº '+' or '-'Àº ¹«½ÃµÇ¹Ç·Î ¸¸¾à ºÎÈ£¸¦ ÆÇº°ÇØ¾ß ÇÑ´Ù¸é
//¸®ÅÏµÇ´Â À§Ä¡ÀÇ ¾Õ¿¡ ÀÖ´Â ¹®ÀÚ¸¦ °Ë»çÇÏ¿© ¾ç¼öÀÎÁö, À½¼öÀÎÁö ÆÇº°ÇÏ¸é µÈ´Ù.
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
		//¼Ò¼öÁ¡Àº ¸Ç Ã³À½¿À°Å³ª Áßº¹µÇ¸é ½ºÅµµÈ´Ù.
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

//buttonÀÇ Á¾·ù¸¦ ¸®ÅÏÇÑ´Ù.
UINT		getButtonStyle(HWND hWnd)
{
	UINT button_style = BS_PUSHBUTTON;

	DWORD dwStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE); 

	// Check Box ÄÁÆ®·ÑÀÎ °æ¿ì, 3STATE ¼³Á¤À» È®ÀÎÇÔ. 
	if (((dwStyle & BS_AUTO3STATE) == BS_AUTO3STATE) || 
		((dwStyle & BS_3STATE) == BS_3STATE))
		button_style = BS_CHECKBOX; 

	switch (dwStyle & 0x0F) // ¹öÆ°¼Ó¼º. 
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
		// BS_AUTO3STATE | BS_3STATE »óÅÂ¸¦ Á¦°ÅÇÑ´Ù. 
		DWORD dwRemove = BS_AUTO3STATE | BS_3STATE;        
		// BS_OWNERDRAW ¼Ó¼ºÀ» ¼³Á¤ÇÑ´Ù. 
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

//±×·ì³»ÀÇ ¿¬¼ÓµÈ radio ¹öÆ°µé¿¡ ´ëÇÑ ÀÏ°ý Ã³¸®¿ë ÇÔ¼ö.
//MFCÀÇ ±âº» CheckRadioButton() ÇÔ¼ö°°Àº °æ¿ì´Â BST_UNCHECKED ±â´ÉÀ» Á¦°øÇÏÁö ¾Ê±â ¶§¹®¿¡
//¾Æ·¡ ÇÔ¼ö¸¦ »õ·ÎÀÌ Á¤ÀÇÇØ¼­ »ç¿ëÇÑ´Ù.
//id_offsetÀÌ 0º¸´Ù ÀÛÀ¸¸é first ~ last±îÁö ¸ðµÎ unchecked.
//¶ÇÇÑ Æ¯Á¤ radio buttonÀ» »ç¿ëÀÚÀÇ ¸¶¿ì½º Å¬¸¯ÀÌ³ª Å°º¸µå µî°ú °°ÀÌ
//Á÷Á¢ Ã¼Å©ÇÏ´Â °æ¿ì°¡ ¾Æ´Ñ
//ÇÁ·Î±×·¥»ó¿¡¼­ SetCheck¿Í °°ÀÌ ÀÚµ¿À¸·Î ¼±ÅÃµÈ »óÅÂ·Î Ç¥½ÃÇÏ°Ô µÇ¸é
//ÀÌÀü ¼±ÅÃµÇ¾ú´ø radio°¡ ¿©ÀüÈ÷ focus¸¦ °¡Áö°í ÀÖ°Ô µÇ°í
//ÀÌ´Â ´Ù¸¥ Ã¢¿¡ focus¸¦ ÁÖ¾ú´Ù°¡ ÇöÀç dlg°¡ ´Ù½Ã focus¸¦ °¡Á®¿Â ¼ø°£
//ON_CONTROL_RANGEÇÔ¼ö°¡ È£ÃâµÇ¸é¼­ ÀÇµµÄ¡ ¾ÊÀº Çö»óÀÌ ¹ß»ýÇÏ°Ô µÈ´Ù.
//µû¶ó¼­ Æ¯Á¤ ¹üÀ§ÀÇ radio button Áß¿¡ ÇÏ³ª¸¦ ÇÁ·Î±×·¥»ó¿¡¼­ ÀÚµ¿À¸·Î ¼±ÅÃµÇ°Ô ÇÏ·Á¸é
//¹Ýµå½Ã focus±îÁö °°ÀÌ Áà¾ß¸¸ ÀÌ·¯ÇÑ ¿Àµ¿ÀÛÀ» ÇÇÇÒ ¼ö ÀÖ´Ù.
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

//dialog based¿¡¼­ Å°ÀÔ·ÂÀ¸·Î µ¿ÀÛÀ» Á¤ÀÇÇÏ´Âµ¥ CEdit°ú °°Àº ÀÔ·ÂÃ¢¿¡ Æ÷Ä¿½º°¡ ÀÖÀ¸¸é
//PreTranslateMessage¿¡¼­ ¹æÇâÅ°³ª charÅ°¸¦ Ã³¸®ÇÏ±â°¡ °ï¶õÇÏ´Ù.
//µû¶ó¼­ ÇöÀç Æ÷Ä¿½º¸¦ °¡Áø ÄÁÆ®·ÑÀÌ CEditÀÌ°í enableÀÌ°í readonly°¡ ¾Æ´Ñ °æ¿ì¿¡´Â
//PreTranslateMessage¿¡¼­ ÀÔ·ÂµÈ Å°¸¦ Ã³¸®ÇÏµµ·Ï ÇÑ´Ù.
bool IsEditCtrlAcceptKeyState(CWnd *pWnd)
{
	if (pWnd->IsKindOf(RUNTIME_CLASS(CEdit)) == false || pWnd->IsWindowEnabled() == false)
		return false;

	CEdit *pEdit = (CEdit*)pWnd;
	if (pEdit->GetStyle() & ES_READONLY)
		return false;

	return true;
}


void debug_string(const char* psz, ...)
{
	char buffer[4096];
	va_list vaList;
	va_start(vaList, psz);
	_vsnprintf(buffer, 4096, psz, vaList);
	va_end(vaList); 

	printf(buffer);
	TRACE(buffer);
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

//"yy-MM-dd" ¶Ç´Â "yy/MM/dd" ³¯Â¥ Ç¥½Ã Çü½Ä¿¡ ¸Â´ÂÁö °Ë»ç
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

//"hh:mm:ss" ½Ã°£ Ç¥½Ã Çü½Ä¿¡ ¸Â´ÂÁö °Ë»ç
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


//¼öÇà½Ã°£À» ÃøÁ¤ÇÏ´Âµ¥
//°íÁ¤¹Ð ÃøÁ¤Àº QueryPerformanceFrequency¿Í QueryPerformanceCounter¸¦ ÀÌ¿ëÇÑ´Ù.
//ÀÌ´Â ¹«°Ì±âµµ ÇÏ°í thread safeÇÏÁö ¾ÊÀ¸¹Ç·Î °¡´ÉÇÏ¸é »ç¿ëÀ» ±ÇÀåÇÏÁö ¾Ê´Â´Ù.
//clockÀ» ÀÌ¿ëÇÑ ¹æ½ÄÀ¸·Îµµ °ÅÀÇ À¯»çÇÑ ¼Ò¿ä ½Ã°£À» ÃøÁ¤ÇÒ ¼ö ÀÖ°í
//linux¿¡¼­µµ ±×´ë·Î »ç¿ëÇÒ ¼ö ÀÖ´Â ÀåÁ¡ÀÌ ÀÖ´Ù.
//GetTickCount()´Â ÆíÂ÷°¡ ¸¹ÀÌ ¹ß»ýÇÏ´Â ´ÜÁ¡ÀÌ ÀÖ´Ù.
//unit : ms
//´Ü, ÀÌ °ªÀÌ longÀ¸·Î °è»êµÇ¹Ç·Î ±× ¸®ÅÏ°ªÀº À½¼ö·Î Ç¥ÇöµÉ ¼ö ÀÖ´Ù.
//ÇÏÁö¸¸ ½Ã°£ ÃøÁ¤ÇÏ´Âµ¥´Â ¹®Á¦µÇÁö ¾Ê´Â´Ù.
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
IsAvailableMemory : 1. ÁÖ¾îÁø ¸Þ¸ð¸® ÁÖ¼ÒÀÇ »óÅÂ°¡ ¹°¸® ÁÖ¼Ò·Î È®Á¤µÇ¾ú´ÂÁö °Ë»ç (½ÇÁ¦ »ç¿ëÀ» À§ÇØ ÇÊ¿ä)
					2. ÁÖ¾îÁø ¸Þ¸ð¸®ÀÇ º¸È£ ¼Ó¼ºÀÌ ÀÐ±â³ª ¾²±â°¡ °¡´ÉÇÑÁö °Ë»ç
	Param :
		LPVOID  pMemoryAddr : °Ë»çÇÏ°íÀÚ ÇÏ´Â ¸Þ¸ð¸®ÀÇ ÁÖ¼Ò
	Return Value :
		ERROR_SUCCESS : System Error Code, ¸ðµç °ÍÀÌ ¼º°øÇÒ °æ¿ìÀÇ ¿¡·¯ÄÚµå
		±âÅ¸ °ª : Read/Write °¡´ÉÇÑ ¸Þ¸ð¸®°¡ ¾Æ´Ï¸é ÇØ´ç ÁÖ¼ÒÀÇ Protect Mode ¸¦
					³ªÅ¸³»´Â 0ÀÌ ¾Æ´Ñ °ªÀ» ¸®ÅÏÇÔ.
	Reference :
		1. https://docs.microsoft.com/en-us/previous-versions/aa915370(v=msdn.10)/
		2. https://docs.microsoft.com/en-us/windows/desktop/debug/system-error-codes--0-499-/
*/
INT IsAvailableMemory(LPVOID pMemoryAddr)
{
	MEMORY_BASIC_INFORMATION    MemInfo = { 0, };
	SIZE_T  nResult = 0;

	nResult = VirtualQuery(pMemoryAddr, &MemInfo, sizeof(MemInfo));

	if (nResult == 0) // Ä¿³Î ¿µ¿ªÀÎ °æ¿ì VirtualQuery ÀÚÃ¼°¡ FailÇÔ.  
	{
		return -1;
	}
	else if (!(MemInfo.State & MEM_COMMIT))
	{
		return MemInfo.State;
	}
	else if ((MemInfo.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)))
	{
		return  ERROR_SUCCESS; // System Error Code ¼º°ø : Reference ÂüÁ¶
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
