#pragma once

#ifndef _SCPARK_FUNCTIONS_H
#define _SCPARK_FUNCTIONS_H

/*
MFC�� � Ŭ������ ��ӹ޾� ���� �ٸ� cpp, h���ϰ��� �޸�
Common ������ �����ϴ� �Ϲ� cpp, h������ ������ٸ�
MFC�� Ŭ���� ���� ����ϱ� ���ؼ��� Afxwin.h �� �� ���� h���ϵ��� include ������� �Ѵ�.
�������� �׳� cpp ���Ͽ� #include "stdafx.h"��� �ص� h, cpp���Ͽ���
MFC Ŭ�������� ����� �� �־����� Common �������� ������Ʈ ������ �ִ� �� ������ �ν��� �� ����.
������Ʈ ���ÿ��� "Precompiled Header"�� "Use"�� �Ǿ� �־ �����ߴٰ� ����������
IntelliSense�� ���� #include "stdafx.h" ������ ������ ���� ǥ�ð� �߰�
�ٸ� �ڵ�鿡�� ������ ������ ǥ�õǸ鼭 ���� ������ ǥ�õ��� �ʴ� �������� �����.
�̸� ���������� �ذ��ϴ� ����� MFC Ŭ���� �� Ȯ�� Ŭ�������� ����ϱ� ���ؼ���
h���Ͽ� �Ʒ��� ���� �ʿ��� h���ϵ��� include�ϰ�
cpp������ ������Ʈ ���ÿ��� "Precompiled Header"�� ���������� �����ϴ� ���̶�� �����ȴ�.
[MFC ����� ���̺귯�� ���� ����]
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

//���� ������ Ÿ���� #define uchar unsigned char �� ���� �����ϸ�
//�ٸ� define��� �浹�Ѵ�. ���� �Ʒ��� ���� �����ζ�
//Ÿ���� ���Ǵ� #define ��� typedef�� �̿�����.
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

//�迭�� ���� ����
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
#define		DOWNLOAD_SET_TOTAL_SIZE	1	//��ü �ٿ���� ���� ũ��
#define		DOWNLOAD_SET_FILE_SIZE	2	//���� �ٿ���� ���� ũ��
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

//�Ҽ��� n+1 �ڸ����� �ݿø��Ͽ� �Ҽ��� n�ڸ��� ǥ���Ѵ�.
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

//width���� ū 4�� ����� ������ش�.
#define		MAKE4WIDTH_U(width)		(((width) + 3) & ~3)
//width���� ���� 4�� ����� ������ش�.
#define		MAKE4WIDTH_D(width)		(((width) - 3) & ~3 | 4)

extern		int			g_nDaysOfMonth[12];

//serial port ����
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

int array[2][3];�� ������� �ϸ�..
MakeArray2(int,array,2,3);

int array[2][3][4];��
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

   //������
	vector2()
	{
		x = 0.0, y = 0.0;
	}

    vector2(double _x, double _y)
	{
        x = _x, y = _y;
    }
    //����
    double cross(const vector2& other) const
	{
        return x*other.y-y*other.x;
    }

    /* ������ �����ε��� ���� ���� ������ ������ �����մϴ�. */

    //������ �Ǽ���
    vector2 operator * (double r) const
	{
        return vector2(x*r, y*r);
    }
    //������ ����
    vector2 operator + (vector2 other) const
	{
        return vector2(x + other.x, y + other.y);
    }
    //������ ����
    vector2 operator - (vector2 other) const
	{
        return vector2(x - other.x, y - other.y);
    }
    //�� ������ ��
    bool operator == (vector2 other) const
	{
        return x == other.x && y == other.y;
    }
    bool operator < (vector2 other) const
	{
        return x < other.x && y < other.y;
    }
};


//timer ���ú���
extern		double		g_dTime0;
extern		double		g_dTime1;
extern		CString		g_sForTimer;

//�Լ� ���� �ð��� ���. a�� ���ڿ�, x�� ���� �Լ�
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
//���μ��� ����
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
	//���μ��� ���� ����.
	//return value : 1 : killed, 0 : fail to kill, -1 : not found
	bool		ProcessKill(CString szProcessName);

	HWND		GetWindowHandleFromProcessID(DWORD dwProcId);
	bool		IsDuplicatedRun();
	CString		run_process(CString exePath, bool wait_process_exit);

	//PID, ���μ��� �̸�, ������ Ÿ��Ʋ �̸�, ������ Ŭ���� �̸����� Ŭ������ ���� ���¸� ���Ҽ� �ֽ��ϴ�. from Devpia
	bool		CheckProcessUsingPID(unsigned long pid);
	bool		CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName);
	bool		CheckProcessUsingProcessName(LPCTSTR processName);

	//����� ����
	//main���� EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); �� �����ϰ�
	//�� ���Ͽ� ���������� ����� g_dqMonitor�� �̿��ϸ� �ȴ�.
	extern std::deque<CRect> g_dqMonitors;
	BOOL		CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
	//r�� �����ִ� ����� �ε����� ����. �������� ������ ��𿡵� ���ٸ� -1�� ����.
	//entire_included�� true�̸� � ����Ϳ� ������ �����ִ� ��쿡�� �ش� �ε����� ����.
	int			get_monitor_index(CRect r, bool entire_included = false);

	//::SetForegroundWindow()�� Win98���ĺ��ʹ� �������� �ʾ� ������ �ڵ�.
	void		SetForegroundWindowForce(HWND hWnd, bool makeTopMost = false);

//Ŭ������ clipboard
	bool		copy_to_clipboard(HWND hWnd, CString str);

//////////////////////////////////////////////////////////////////////////
//���ڿ�
	bool		Compare_By_Case_Sensitive(CString str1, CString str2, bool bCase);
	int			find_string(CString target, CString find_string, bool case_sensitive = false);
	//dqSrc�� dqFind�� �ִ��� �˻��Ͽ� �ε����� ����. ����� AND �����̹Ƿ� dqFind�� ��� ���Ұ� dqSrc�� ���ԵǾ� �־�� ��.
	int			find_dqstring(std::deque<CString> dqSrc, CString strFind, bool bWholeWord = false, bool bCaseSensitive = false);
	int			find_dqstring(std::deque<CString> dqSrc, std::deque<CString> dqFind, TCHAR op = '&', bool bWholeWord = false, bool bCaseSensitive = false);
	int			Find_Divide_Position_By_Punctuation(CString str);
	int			FindStringFromArray(CStringArray& ar, CString sTarget, bool bCaseSensitive = false, bool bWholeWord = false);
	//int			FindStringFromDeque(std::deque<CString> dq, CString sTarget, bool bCaseSensitive = false, bool bWholeWord = false);
	CString		GetCommaString(CString sString, CString sComma = _T(","));
	bool		IsNumericString(const CString& strSource);
	//����� ������ ���ڿ�����
	bool		IsNatural(LPCTSTR lpszValue);
	//�Ҽ������� ����Ͽ� ���ڰ�����
	bool		IsNumeric(LPCTSTR lpszValue);
	//������ ����ϴ� ������ ���ڿ�����
	bool		IsInteger(LPCTSTR lpszValue);
	//start���� �����ؼ� ó�� ������ ���� ������ �����ؼ� num�� �Ѱ��ش�.
	//���� ������ ������ false�� �����Ѵ�.
	bool		get_number_from_string(CString str, int &num, int start = 0);
	//���ڿ��� ���Ե� ���ڹ��ڸ� ���ڷ� �����Ͽ� ���Ѵ�. "a5"�� "a12"���� �۴�.
	bool		is_greater_with_numeric(CString str0, CString str1);
	//�־��� ���ڿ��� ���ĺ��� ���ڷθ� ������ ���ڿ����� �˻��Ѵ�.
	//excepts���� ���Եǵ� �Ǵ� ���ڿ����� ����ִµ� �̵��� ;���� ���еǾ� �ְ�
	//�˻��ϱ� ���� �̸� ������ �� �˻��Ѵ�.
	bool		IsAlphaNumeric(CString str, CString excepts = _T(""));

	//ASCII �ڵ��� #33(0x21)(' ') ~ #126(0x7E)('~') ��������(���� �� �ִ� ���ڿ�����)
	bool		is_readable_char(CString src);

	//'��'~'?'������ �ѱ۷θ� ������ ���ڿ����� �˻��Ѵ�.
	bool		is_hangul(CString str);

	//���ڿ��� �������� ���� ���������� �Ǻ�(Ư�� �ѱ� ���ڵ� ���� �Ǻ�)
	bool		is_valid_string(CString src, bool include_hangul);

	//�ϼ��� �ѱ��� �� ���ڸ� �ʼ�, �߼�, �������� �и��Ѵ�.
	bool		get_consonant(CString src, wchar_t* cho = 0, wchar_t* jung = 0, wchar_t* jong = 0);

	CString		ConvertInt2AZ(int n);	//n�� 26���� ���� �÷� �ε����� ��ȯ�� ���ڿ��� ����
	CString		GetToken(CString& str, LPCTSTR c);
	CString		GetToken(CString src, CString separator, int n);
	std::deque<CString>	GetTokenString(CString src, CString separator);
	int			GetTokenString(CString src, std::deque<CString>& dqToken, TCHAR separator = ' ', bool allowEmpty = true, int nMaxToken = -1);
	int			GetTokenString(CString src, std::deque<CString>& dqToken, std::deque<TCHAR> separator, bool allowEmpty = true, int nMaxToken = -1);
	int			getTokenString(TCHAR *src, TCHAR *separator, CString *sToken, int nMaxToken);
	int			getTokenString(char *src, char *separator, char **sToken, int nMaxToken);

	//[2023/1/1 22:1:29] [DBMS][NMS_LS_TERMINATE_SESSION_DATA][ID : tmax25][update livesupport_report set endtime = '2023-01-01 22:01:29', env_type = '5', viewer_type = '0' where accesscode = '31108355' and sptnum = '50400']
	//[]�� ������ ��ū�� �и��Ѵ�.
	//��ȣ�� ���� �ȸ����� false�� �����Ѵ�.
	bool		get_bracket_token(CString src, std::deque<CString>* token, TCHAR sep);

	//update livesupport_report set endtime = '2023-01-01 22:01:29', env_type = '5', viewer_type = '0' where accesscode = '31108355' and sptnum = '50400'
	//���� ���� sql���� field�� value�� �����Ѵ�.
	void		get_sql_token_from_assign_form(CString src, std::map<CString, CString> *map);
	//insert into neturo_server_info(userid, com_name, s_pub_ip, s_pri_ip) values('14533821', 'DESKTOP-0CN9VAK', '220.85.215.243', 'publicIP')
	//���� ���� sql���� field�� value�� �����Ѵ�.
	void		get_sql_token_from_bracket_form(CString src, std::map<CString, CString>* map);
	CString		get_sql_cmd(CString src, CString* sql_cmd = NULL, CString *table_name = NULL);

	//deque�� �ִ� ���ҵ��� �����ڷ� �ϴ� �ϳ��� ���ڿ��� ����
	CString		get_tokenized(std::deque<CString> dq, TCHAR separator = ';');
	CString		get_str(CString& buff, CString sep = _T("|"));
	int			get_int(CString& buff, CString sep = _T("|"));
	double		get_double(CString& buff, CString sep = _T("|"));
	//unit			: 0:bytes, 1:KB, 2:MB, 3:GB
	//floats		: �Ҽ����� �� �ڸ����� ǥ������
	//unit_string	: ������ ǥ���� ��
	//comma			: ���� �κп� �ڸ��� �޸��� ǥ���� ��
	CString		get_size_string(int64_t size, int unit = 1, int floats = 0, bool unit_string = true, bool comma = true);

	//src�� �Ľ��ؼ� Ư�� ���� �̻��� ���ڿ���� ������.
	std::deque<CString> parse_divide(CString src, int len);
	//src�� pos��ó���� �������� ã�� �� ��ġ�� �����Ѵ�.(���������� ������ ������ �̿�)
	int			find_punctuation(CString src, int pos);
	bool		is_punctuation(TCHAR ch);

	//�� ������ ���ڴ� �ݵ�� NULL�� �־���� ���� �� �� �ִ�.
	bool		isOneOf(LPCTSTR src, ...);
	//src ���ڿ��� set_of_keyword�� ������ �ܾ �ִ��� �˻�.
	//set_of_keyword�� �����ݷ����� �����ؼ� ���� ���� �Ǵ� ���ڿ��� ���� �� �ִ�.
	//ex. src = "abcd1234"�� �� set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
	bool		is_exist_keyword(CString src, CString set_of_keyword, bool case_sensitive = false, bool whole_word = false);

	//dqList���� element���� ��ġ�ϴ� �׸��� index�� �����Ѵ�. ������ -1�� ����.
	template <typename T> int find_index(std::deque <T> *dqList, T element)
	{
		std::deque<T>::iterator it = std::find(dqList->begin(), dqList->end(), element);
		if (it != dqList->end())
		{
			return distance(dqList->begin(), it);
		}

		return -1;
	}

	//str�� from ��ġ ���Ŀ� �ִ� ���� �������� num�� �־��ְ� ���� ������ġ�� return�Ѵ�.
	int	extract_digit_number(char *str, int from, double *num);

	//version string valid check
	//digits : �ڸ���(1.0.0.1�� ���� �ڸ��� 4)
	bool valid_version_string(CString versionStr, int digits);
	//�׳� ���ڿ��� ���ϸ� 1.0.9.0�� 1.0.10.0���� �� ũ�ٰ� �����Ƿ� .�� ���� ���ڷ� ���Ѵ�.
	//���ϰ��� strcmp�� ������ ��Ģ���� �Ǵ��Ѵ�.(+:ver0�� ŭ, -:ver1�� ŭ, 0:����)
	int	compare_version_string(CString ver0, CString ver1, TCHAR separator = '.');

	//http://yeobi27.tistory.com/280
	//A2W, A2T �� �� �ݴ� ��ũ�ε��� ������ ����ϹǷ� ���� ������ �ְ� ũ�� ���ѵ� �����Ƿ�
	//������ CA2W, CA2T���� ����Ѵ�. �� �� ��ũ�ε��� encoding�� ������ �� ����.
	std::wstring CString2wstring(const char* str);
	std::string CString2string(CString cs);
	CString		string2CString(std::string s);
	//��Ƽ����Ʈ ȯ�濡�� �� �Լ��� ȣ���ؼ� ����ϸ� ��Ȥ ������������ �����Ѵ�.
	//�Ƹ��� �Լ������� �޸𸮰� �Ҵ�� �� ȣ���� ������ ����Ϸ��� ������ �� �� �ִ�.
	//�� �Լ��� �ٵ� �״�� ���� ������ �����Ƿ� �ϴ� �ٵ� �ڵ带 �״�� �����ؼ� ����Ѵ�.
	LPCWSTR		CString2LPCWSTR(CString str);
	char*		CString2char(CString str);
	TCHAR*		CString2TCHAR(CString str);
	LPCSTR		CString2LPCSTR(CString str);
	LPCWSTR		LPCTSTR2LPCWSTR(LPCTSTR str, UINT codePage = CP_UTF8);
	WCHAR*		CString2WCHAR(CString str); //{ return (WCHAR*)(const WCHAR*)CStringW(str); }

	//chStr�� ��ȿ�� ���̸� �̹� �˰� �ִٸ� length�� ��������� ��Ȯ�ϴ�.
	//�׷��� ���� ��� chStr�� ���� '\0'�� ���� ��� ������ ���ڵ���� ���Ե� �� �ִ�.
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



	//����, '\t', '\r', '\n', '\0' ��� ����
	void		trim(char* src);
	void		trim(std::string &str);
	void		trim_left(std::string& str);
	void		trim_right(std::string& str);
	void		trim(std::deque<CString>* dq);
	//src���ڿ����� chars�� ��� �����Ѵ�.
	void		remove_chars(CString &src, CString chars);
	//src���ڿ����� ������ ������ ���ڿ��� �����Ѵ�.

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
	//str���� �յ� ���ڿ� ���̿� �ִ� ���� ���ڿ��� �����Ѵ�. ������ "" ����.
	CString		ExtractSubString(CString src, CString sPrev, CString sPost);	//sPrev(�ʵ� �տ� ���� ���ڿ�), sPost(�ʵ� �ڿ� ���� ���ڿ�)
	//std::string string_format(const std::string fmt, ...);
	std::string string_format(const std::string fmt_str, ...);	//A safer and more efficient(http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf)
	/*
	//format�� �ٶ��� %s�� �ƴ� %S??
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



//������ ��ȯ
	CString		i2S(int64_t nValue, bool bComma = false, bool fill_zero = false, int digits = 0);
	CString		i2HS(int64_t nValue, bool bCapital = true);
	CString		d2S(double dValue, bool bComma = false, int nfDigit = -1);	//nfDigit : �Ҽ��� �ڸ���. -1�̸� �״�� ���.
	char*		ushortToBinary(unsigned short i);
	char*		intToBinary(int i);
	char*		uintToBinary(unsigned int i);
	int			binaryToInt(char *s);
	unsigned int binaryToUint(char *s);
	//� ���� x��° ��Ʈ�� ����.
	int			get_bit(int number, int x);
	//x�� 1�̸� n��° ���� 1�� ����, x�� 0�̸� n��° ���� 0���� ����
	void		set_bit(int& number, int n, int x);
	int			HexaStringToInt(CString str);
	CString		GetByteString(uint8_t* bt, int n, bool upper = true, bool prefix = true);
	CString		BinaryToHexString(BYTE* pData, int length, TCHAR separator = _T(' '));
	int			getPrecision(double d, bool bExceptZero = true);	//�Ҽ��� �ڸ��� ����
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
//���� ����
	CString		GetFileNameFromFullPath(CString fullpath);
	CString		GetFolderNameFromFullPath(CString fullpath, bool includeSlash = false);	//= PathRemoveFileSpec
	CString		GetFileTitle(CString fullpath);
	CString		GetFileExtension(CString filename, bool dot = false);
	int			GetFileTypeFromFilename(CString filename);
	int			GetFileTypeFromExtension(CString sExt);
	bool		ChangeExtension(CString& filepath, CString newExt, bool applyRealFile);
	CString		normalize_path(CString& filepath);

	//������ �ִ� ���ϵ� �� filetitle�̰� extension�� �ش��ϴ� ���ϸ��� �����Ѵ�.
	std::deque<CString>		get_filename_from_filetitle(CString folder, CString filetitle, CString extension);
	std::deque<CString>		get_filename_from_filetitle(CString filename, CString extension);

	uint64_t	get_file_size(CString sfile);
	uint64_t	get_folder_size(CString path);
	//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
	//unit_string	: ������ ǥ���� �� (default = true)
	//������ ���� ""�� ������.
	CString		get_file_size_string(CString sfile, int unit = 1, int floats = 0, bool unit_string = true);
	CTime		GetFileCreationTime(CString sfile);
	CTime		GetFileLastModifiedTime(CString sfile);
	CTime		GetFileLastAccessTime(CString sfile);
	CString		GetMostRecentFile(CString sFolder, CString sWildCard = _T("*.*"), int nReturnType = 1);
	CString		GetMostRecentDateFile(CString sFolder, CString sWildCard = _T("*.*"));	//���� �ֱ� ��¥ ���ϸ� ����
	CString		GetNextIndexFile(CString sCurrentFile, bool bNext = TRUE);	//���� �ε����� ���ϸ� ����
	int			GetNextFileIndex(CString sCurrentFile);						//���� ������ �ε��� ����
	int			GetNumberFromFile(CString sfile);			//������ �о �Ѱ��� ���ڰ��� �����Ѵ�. (������ ������ -99999 �� �����Ѵ�.)
	bool		WriteNumberToFile(CString sfile, int n);	//���ڰ��� ���Ͽ� ������ش�.
	CString		GetFileProperty(CString sFilePath, CString sProperty);
	size_t		read_raw(CString sfile, uint8_t *dst, size_t size);
	bool		save2raw(CString sfile, uint8_t *data, size_t size);
	int			RenameFiles(CString folder, CString oldName, CString newName, bool overwrite = false, bool bWholename = true, bool bRecursive = false);
	bool		delete_file(CString fullpath, bool bTrashCan = false);
	int			get_text_encoding(CString sfile);
	bool		save(CString filepath, CString text);
	bool		file_open(FILE** fp, CString mode, CString file);

	//mp4 ������ Ư�� �±� ������ �� ���ϴ� ��ġ�� �����͸� �����Ѵ�.
	//MOBIS ������Ʈ ���� MP4�� mdat �ʵ��� 0x40�������� 28 bytes��
	//�������� ������ ����ð��� ����Ǿ� �ִ�.
	//n�� 0x3C���� 4����Ʈ�� �� ũ���̴�.(28 bytes)
	char*		GetDataFromMP4File(char* sfile, char* sTag, uint8_t tagStart, int tagLength);

//////////////////////////////////////////////////////////////////////////
//���ͳ� ����
	DWORD		GetURLFileSize(LPCTSTR pUrl);
	bool		DownloadFile(LPCTSTR pUrl, CString strFileName, bool bOverwrite = TRUE, HWND hWnd = NULL);
	CString		DownloadURLFile(CString sUrl, CString sLocalFileName, HWND hWnd = NULL);
	bool		CheckFileIsURL(CString sURL);
	bool		is_valid_url(CString url);
	void		GetURLFileInfo(CString sURL, bool &bInURL, bool &bFileType);
	bool		ReadURLFile(LPCTSTR pUrl, CString &strBuffer);
	void		ReadURLFileString(CString sURL, CString &sString);

	//url���� ������ ������ �аų� ���� ���Ϸ� �ٿ�ε� �Ѵ�.
	//local_path�� �����Ǿ� ������ ���Ϸ� �ٿ�ް�(�̶� ���ϰ��� "ok", ������ ���� ���� ���� �޽���)
	//������ ���ڿ��� ���Ϲ޴´�.
	CString		get_uri(CString full_remote_url, CString local_path = _T(""));
	CString		get_uri(CString ip, int port, CString remote_path, CString local_path = _T(""));

	CString		GetDefaultBrowserPath();	//[��ó] [VC++] Windows �⺻ �� ������ ���� ��� ������|�ۼ��� ����ӽ�
	//Content-Type: multipart/form-data ������ �̿��� ���������� ���� ���� �Լ�
	bool		HttpUploadFile(CString url, CString filepath, int chatIndex);

	//������ �ѱ۸� ���Ͽ� ���� ó�������� get_uri()�Լ� ��� �߰��Ͽ� �׽�Ʈ �غ�����
	//�������� �����ε��Ͽ� �켱 �� �Լ� ����� ������...
	bool		HttpDownloadFile(CString url, CString local_path = _T(""));


//webView2 Runtime
	bool		is_WebView2Runtime_installed();
	bool		install_WebView2Runtime(CString runtimeExePath, bool silentInstall);

//////////////////////////////////////////////////////////////////////////
//���� ����
	bool		IsFolder(CString sfile);				//�������� ��������
	bool		isFolder(char *sfile);
	//���ϸ��̳� ������ '\\', '/' ȥ���� ��찡 �����Ƿ� CString�� '==' �����ڷ� ���ؼ� �ȵȴ�. 
	bool IsFileFolderPathIsEqual(CString file0, CString file1, bool bCaseSensitive = false);
	CString		GetParentDirectory(CString sFolder);	//���� ������ ���� �������� �����Ѵ�.

	//compare_only_filename : fullpath�� ��������, ���ϸ� �����ؼ� ��������. default = false;
	void		sort_like_explorer(std::deque<CString> *dq, bool compare_only_filename = false);
	//������ ������ ���� ����� ���´�.
	//sNameFilter�� ���ϵ�ī��� ���� �༭ �˻��ؾ� �Ѵ�.
	//������Ʈ ��ɰ� �����ϰ� ����ǥ�� ��ǥ�� ���� ���ϵ�ī�带 �̿��� �� �ִ�.
	//sNameFilter = "test*", sExtFilter = "jpg;bmp;" �� ���� �Է��ϸ�
	//test�� �����ϰ� Ȯ���ڰ� jpg, bmp�� ���� ����� ���´�.
	//sExceptStr = "test;temp;error" �� ���� �����ݷ����� �����Ͽ� �˻� ������ ���ϸ� ���� ����.
	//����! dqFiles�� �� �Լ��� ���� �ʱ�ȭ���� �����Ƿ� �ʿ��� ��� �ʱ�ȭ�Ͽ� ȣ���� ��!
	void		FindAllFiles(	CString sFolder, std::deque<CString> *dqFiles,
								CString sNameFilter = _T("*"), CString sExtFilter = _T("*"),
								bool bRecursive = false, CString sExceptStr = _T(""),
								bool auto_sort = true);
	//stdc++17�� std::filesystem�� �̿��� �Լ��μ�
	//FindAllFiles�� recursive function���ٰ� �ɼ��� ���ٰ�� �ص�
	//debug mode���� 8700���� ������ ã�µ�(D:\1.project\0.backup)
	//25,047ms VS 76ms�� �ӵ����̰� ����.(+auto_sort=268ms)
	//32,105���� ���� : 1,773,564ms VS 3,337ms
	//release mode������
	//9,715ms VS 21ms.
#if (_MSVC_LANG >= _std_cpp17)	//__cplusplus ��ũ�θ� ����Ϸ��� C/C++�� ���â���� /Zc:__cplusplus�� �߰����Ѿ� �Ѵ�.
	std::deque<CString>	find_all_files(CString path, CString name_filter = _T(""), CString ext_filters = _T(""), CString except_str = _T(""), bool recursive = true, bool auto_sort = true);
#endif
	//list�� NULL�� ȣ���ϸ� ���� sub folder�� ������ ������ �����̴�.
	//root�� "�� PC"�� ��� special_folders�� true�̸� �ٿ�ε�, �� ����, ���� ȭ�� �׸���� �߰��Ѵ�.
	int	get_sub_folders(CString root, std::deque<CString>* list = NULL, bool special_folders = false);

	void save_dqlist(std::deque<CString>* dqlist, CString output_text_file_path);

	//���� FindAllFiles������ "���ϸ�*"�� ���� ã�� �Ǵµ� �̷� ��� �ø��� �̸��� �ٸ� ���ϵ鵵 ��� ã������.
	//���� ���ϸ��� Ȯ���ǰ� Ȯ���ڸ� ���������� ���� �Ʒ� �Լ��� �̿��ؾ� �Ѵ�.
	//(a.jpg, a.png���� ã�� ������ a1.jpg�� �־ true�� �Ǳ� ������)
	//(a 00.jpg ~ a 99.jpg = a ??.jpg�� ã�� �� �ִ�. ��, "Ÿ��Ʋ ??"�̰� ?�� ���ڰ� ���� �� ã������ҵ��ѵ� �켱 �н�)
	std::deque<CString>		FindFilesWithExtensions(CString folder, CString fileTitle, CString extensions);

	// ������ ��� ������ �����.
	int			delete_all_files(CString folder, CString name_filter, CString ext_filter, bool recursive = true, bool trash_can = false);
	bool		DeleteFolder(LPCTSTR lpFolder);
	bool		SHDeleteFolder(CString sFolder);


	//Ǯ�н��� �ָ� ������ �ڵ����� ������ش�.
	//������ �̹� �����ص� true�� ������.
	//lpPathName�� ���� �� ������ ���ϸ��̶�
	//������ �� �̸��� ������ ������ �����ϰ� �ǹǷ�
	//�ݵ�� �����ϰ��� �ϴ� �������� ����Ұ�.
	bool		make_full_directory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpsa = NULL);
	bool		recursive_make_full_directory(LPCTSTR sFolder);

	CString		normalized_path(CString& path);

	//�������� Ư�� ���ڿ��� �� ���ϵ��� �����. �������� ��������.
	void		DeleteFilesBySubString(CString sFolder, CString filenameSubStr, bool bMatchWholeWordOnly = FALSE, bool bMatchCase = FALSE);

	//void		DeleteAllFiles(CString sFolder);

	//���� ���� ���� �Լ���
	//bool		EnableDebugPrivilege(void);
	//bool		CloseRemoteFileHandles(LPCTSTR);
	//DWORD		CloseRemoteHandle(LPCTSTR lpProcessName, DWORD processID, HANDLE handle);
	//bool		DeleteTheFile(LPCTSTR lpFileName);

	//�������� ��ȭ���ڸ� ����.
	//strSelectedFolder : ������ ����
	//strStartFolder : �⺻ ���õ� ����. ���� ����.
	bool BrowseForFolder(HWND hwndOwner, TCHAR* lpszTitle, CString& strSelectedFolder, const TCHAR* strStartFolder = NULL, bool bNewFolderButton = true);
	static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM lpData);

	//������ �ü������ Ư�� ����(�ٿ�ε�, �� ���� ��)�� ���� ��θ� �����Ѵ�.
	//FOLDERID_Downloads, FOLDERID_Documents, ...
	CString get_known_folder(KNOWNFOLDERID folderID);

//////////////////////////////////////////////////////////////////////////
//��Ʈ��ũ, ���ͳ�
	bool		GetNICAdapterList(IP_ADAPTER_INFO* pAdapters, int& nTotal, int nMax = 10);
	void		GetNetworkInformation(TCHAR* sFindDescription, NETWORK_INFO* pInfo);
	bool		CheckInternetIsOnline();
	int			get_char_count(CString sStr, TCHAR ch);
	bool		IsAvailableEMail(CString sEMail);


//////////////////////////////////////////////////////////////////////////
//��ȣȭ
	//���ڿ��� ��ȣȭ�Ѵ�.
	void		EncryptString(CString& ToCode, TCHAR* key);
	//��ȣȭ�� ���ڿ��� ��ȣȭ�Ѵ�.
	void		DecryptString(CString& ToCode, TCHAR* key);
	//���ڷ� ������ ���ڿ��� �Է¹޾� ������ ������ �� �� �ڼ��� ���ڹ��ڿ��� �����Ѵ�.
	CString		ShuffleNumericString(CString sSrc, bool bSameLength = true);

	//������ ��ȣȭ�Ѵ�.
	bool		FileEncryption(CString sfile, bool bShowErrorMessage = TRUE);


//////////////////////////////////////////////////////////////////////////
//��, ������, ������Ʈ��, �ý���
	CString		GetComputerNameString();
	bool		GetWindowsVersion(OSVERSIONINFO& osversioninfo);
	DWORD		GetWindowsVersion();
	bool		GetWindowsVersion(DWORD& dwMajor, DWORD& dwMinor);
	//bool		GetWindowsVersion(DWORD& dwMajor, DWORD& dwMinor, DWORD& dwServicePack);
	bool		GetWindowsVersion(DWORD& dwMajor, DWORD& dwMinor, DWORD& dwPlatform);

	//�� �� �׽�Ʈ �ʿ�!
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

	//app�� ��ġ�� ũ�⸦ ������Ʈ���� �����ϰ� ���� �� �ٽ� ������Ų��.
	//resize�� �ȵǴ� ���̾�α��� ���� resize��Ű�� �ʰ� ���� ũ��� ǥ���ؾ� �Ѵ�.
	void		RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""), bool use_maximize = true, bool resize_window = true);
	void		SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""));


	//CPU Usage ����
	LPBYTE		GetPerformanceData(LPTSTR src);
	int			GetCounterValue(const int& objId, const int& counterId, const char* instanceName, PERF_DATA_BLOCK **dataBlock, LONGLONG &value);
	double		GetCpuUsage(const char* process);

	//�޸�, memory
#ifndef _USING_V110_SDK71_
	SIZE_T		GetCurrentMemUsage();
#endif
	INT			IsAvailableMemory(LPVOID pMemoryAddr);


/*
src		: �ҽ� ������
srcx	: �ҽ� ���� x ��ǥ
srcy	: �ҽ� ���� y ��ǥ
srcw	: �ҽ� ��ü width(pixel)
srch	: �ҽ� ��ü height(pixel)
dst		: Ÿ�� ������
dstx	: Ÿ�� x ��ǥ
dsty	: Ÿ�� y ��ǥ
dstw	: Ÿ�� ��ü width(pixel)
dsth	: Ÿ�� ��ü height(pixel)
w		: ������ width ũ��(pixel)
h		: ������ height ũ��(pixel)
*/
bool		memcpy_block(uint8_t *src, int srcx, int srcy, int srcw, int srch, uint8_t *dst, int dstx, int dsty, int dstw, int dsth, int w, int h, int ch);
bool		memcpy_block(uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, int ch, uint8_t *dst);

//src���� roi ������ �߶� dst_width * dst_height ũ��� resize ��Ų��.
bool		resize_roi(uint8_t *src, int src_width, int src_height, int x_roi, int y_roi, int w_roi, int h_roi, uint8_t *dst, int dst_width, int dst_height);

//HDD
uint64_t	GetDiskFreeSize(CString sDrive);
uint64_t	GetDiskTotalSize(CString sDrive);
CString		GetDiskSizeString(CString sDrive);	// "1.25G / 380.00G"
//CString		GetHDDSerialNumber(int nPhysicalDrive);
CString		GetHDDVolumeNumber(CString sDrive);
void		get_drive_map(std::map<TCHAR, CString> *drive_map);
CString		get_drive_volume(TCHAR drive_letter);
//"���� ��ũ (C:)" <-> "C:\\" //���� ���� ���� ������ ������� ��ȯ
//���� -> "C:\\Documents", �� �� �Ϲ� ������ �״�� ����.
CString		convert_special_folder_to_real_path(CString special_folder, std::map<int, CString>* csidl_map = NULL);
CString		convert_real_path_to_special_folder(CString real_path, std::map<int, CString>*csidl_map = NULL);


//�Ķ���ͷ� ���� ���ӵ� ���ϸ���� �и��Ѵ�. �������ϸ��� ���ܵ�.(ex. command line or shell command)
void		ParseCommandString(CString sParam, CStringArray& ar);

//������ �̹����� ����ȭ�鿡 ǥ���Ѵ�.
#ifndef _USING_V110_SDK71_
void		SetWallPaper(CString sfile);
#endif

//����������� ������ش�.
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
//�ð�
	CString		GetDateStringFromTime(CTime t, CString sMark = _T("-"));
	CString		GetDateStringFromTime(COleDateTime t, CString sMark = _T("-"));
	CString		GetDateStringFromTime(__timeb32 t, CString sMark = _T("-"));
	CString		GetTimeStringFromTime(CTime t, CString sMark = _T(":"), bool h24 = true, bool include_seconds = true);
	CString		GetTimeStringFromTime(COleDateTime t, CString sMark = _T(":"));
	CString		GetTimeStringFromTime(__timeb32 t, CString sMark = _T(":"));
	//type 0(date), 1(time:24h), 2(date+time) ��-��-�� ��:��:�� �������� ���� �ð� ����. mid�� ��¥�� �ð� ���� ���ڿ�
	CString		GetCurrentDateTimeString(int nType = 2, bool bSeparator = true, CString mid = _T(" "), bool h24 = true);
	//type 0(date), 1(time:24h), 2(date+time) ��-��-�� ��:��:�� �������� ���� �ð� ����. mid�� ��¥�� �ð� ���� ���ڿ�
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
	double		GetElapsedTime(__timeb32 pOldTime);	//pOldTime�� ���� �ð��� ���� ���
	//ts���� �Ѱ� �޾� "a�� b�ð� c�� d��" ���·� ǥ��
	CString		GetDayTimeCountString(CTimeSpan ts, bool bShowZero, bool bIncludeSec);
	//ts���� �Ѱ� �޾� "a�� b�ð� c�� d��" ���·� ǥ��
	CString		GetDayTimeCountString(COleDateTimeSpan ts, bool bShowZero, bool bIncludeSec);
	time_t		_mkgmtime(const struct tm *tm) ;
	time_t		_mkgmtime(const struct tm* tm);
	bool		IsAM(CTime t = 0);	//t=0�̸� ����ð�����, 0���� ũ�� �� �ð��� ����
	CString		GetDayOfWeekString(CTime t = NULL, bool short_str = false);
	int			GetDaysOfMonth(int nYear, int nMonth);	//�ش� ���� ��¥�� ����
	//��¥ ����
	int			GetSeasonIndex();	//��=0, ����=1...
	int			gettimeofday(struct timeval* tv, struct timezone* tz);

	//��¥ ǥ�� ���Ŀ� �´��� �˻�
	bool		is_valid_date(CString str);
	//�ð� ǥ�� ���Ŀ� �´��� �˻�
	bool		is_valid_time(CString str);

	//��¥�ð� ������ yyyy/mm/dd hh:mm:ss �������� �����.
	void		normalize_datetime(CString & src);

//Ÿ�̸� ����
	void		Wait(DWORD dwMillisecond);		//�������� OnTimer() �������� ���۵��� �ʾҾ��µ� ����� �����ϴ�.
	//void		usleep(int microSec);
	void		ProcessWindowMessage();			//�ݺ����� ���� process�� ��������� ���� �ʵ��� �ݺ����ȿ��� ȣ���Ͽ� �޽���ť�� ������ �ٷ� ó����Ų��.
	int			ptimer_start(int instance);		//reset high resolution time lcounter 
	double		ptimer_get_time(int instance);	//get time(double precision) elaspsed since timer reset in ms
	//unit : ms
	long		getClock();


/*
//////////////////////////////////////////////////////////////////////////
//����
	COLORREF	get_color(CString sColor);
	COLORREF	get_color(COLORREF crOrigin, int nOffset);
	//0~9���� �̸� ���س��� �⺻ ������ �����Ѵ�.
	COLORREF	GetDefaultColor(int idx);
	//random19937�� �̿��Ͽ� ���� �÷��� �����Ѵ�.
	COLORREF	get_random_color();
	bool		IsHexaColorString(CString str);
	COLORREF	get_color_from_hexa_string(CString str);
	COLORREF	GetComplementaryColor(COLORREF crColor, COLORREF crBack = RGB(255,255,255));
	void		RGB2HSL(int r, int g, int b, int& h, int& s, int& l);

	//32��Ʈ�� dw�� ����ִ� R, G, B�� �����Ͽ� 16��Ʈ(5+6+5) �÷��� �����Ѵ�.
	WORD		RGB24ToRGB565(DWORD dw);
	//�� DWORD�� WORD�� ��ȯ�Ͽ� �ϳ��� DWORD�� ��ȯ�Ѵ�.
	DWORD		RGB24ToRGB565(DWORD rgb1, DWORD rgb2);
	//565�� ��ȯ�� WORD�� �ٽ� 24��Ʈ RGB�� ����ϱ� ���� DWORD�� ������Ų��.
	DWORD		RGB565ToRGB24(WORD wd);

	//gray �迭����
	bool		isGray(COLORREF cr, int tolerance = 0);
	//gray image�� 3ä�� �Ǵ� 4ä�� �̹��� �������� �������ش�.
	//dst�� �ݵ�� �Ҵ�� �޸��ּ��̾�� �Ѵ�.
	void gray2color(uint8_t *gray, int gray_width, int gray_height, uint8_t *dst, int dst_width, int dst_height, int dst_ch, uint8_t alpha);

	//3ä�� �Ǵ� 4ä�� �̹����� ���� ��� �̹����� �����Ѵ�.
	//dst�� �ݵ�� �Ҵ�� �޸��ּ��̾�� �Ѵ�.
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
								CString font_name = _T("���� ���"),
								Gdiplus::Color crOutline = Gdiplus::Color::White,
								Gdiplus::Color crFill = Gdiplus::Color::Black,
								UINT align = DT_LEFT | DT_TOP);
	void		draw_shadow_text(CDC* pDC, int x, int y, CString text, int font_size, int depth,
								CString font_name = _T("���� ���"),
								Gdiplus::Color crShadow = Gdiplus::Color::Red);

	//text�� ����ȼ� �ʺ� max_width�� ���� ��� ...�� �Բ� ǥ�õ� ������ġ�� ����.
	//�� �Լ��� DrawText�ÿ� DT_END_ELLIPSIS�� �༭ ����ϹǷ� �켱 ��� ����!
	int			get_ellipsis_pos(CDC* pDC, CString text, int max_width);

	//20220914 DrawLine�� DrawLinePt�� ���� �̸����� �ϴ� ��ȣ�ϴٴ� ������ �߻��Ͽ� DrawLinePt�� ����.
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


//gradient_fill�� ���ؼ� ����� �� �ڵ��� ����ϴ� ���α׷��̶��
//����� �� ���������ִ� �Լ��� �ݵ�� ȣ������� �Ѵ�.
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

//�̹����� ǥ�õǰ� �ִ� ���� ������ ȭ����� ��ǥ�� �ָ� �̹������� ���� ��ǥ�� �����Ѵ�.
//��, ���� �̹������� ���� ��ǥ�� �̹��� ũ�⸦ ����� ��� �������� -1���� ä���� �����Ѵ�.
	void		GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, double *x, double *y);
	void		GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, CPoint *pt);
	void		GetRealPosFromScreenPos(CRect rDisplayedImageRect, int srcWidth, CRect *r);

	//�̹����� ǥ�õǰ� �ִ� ���� ������ �̹��� ���� ��ǥ�� �ָ� ȭ����� ��ǥ�� �����Ѵ�.
	void		GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, double *x, double *y);
	void		GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, CPoint *pt);
	void		GetScreenPosFromRealPos(CRect rDisplayedImageRect, int srcWidth, CRect *r);


//����, Line ���� �Լ�

	//�������� ���� b�� ���� a�� �ݽð� �����̸� ���, �ð�����̸� ����, �����̸� 0�� ��ȯ �Ѵ�.
	double ccw(vector2 a, vector2 b);

	//�� p�� �������� ���� b�� ���� a�� �ݽð� �����̸� ���, �ð�����̸� ����, �����̸� 0�� ��ȯ �Ѵ�.
	double ccw(vector2 p, vector2 a, vector2 b);

	// - �� a, b�� ������ ������ �� c, d�� ������ ������ ������ x�� ��ȯ�Ѵ�.
	// - �� ������ �����̸�(��ġ�� ��� ����) ������, �ƴϸ� ���� ��ȯ�Ѵ�.
	bool		GetIntersectionPoint(vector2 a, vector2 b, vector2 c, vector2 d, vector2& x);
	bool		GetIntersectionPoint(CPoint* pt, CPoint* ispt);
	bool		GetIntersectionPoint(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int& isx, int& isy);

	//4������ ������ �� ������ ���� �����ϴ��� ���θ� �Ǻ��Ѵ�.
	//bCheckCrossOver = false�̸� (0,1)�� (2,3)������ �������θ�, true�̸� (0,3)�� (1,2)�� �������α��� �Ǻ��Ѵ�.
	bool		IsIntersect2LineSegment(vector2 a, vector2 b, vector2 c, vector2 d);
	bool		IsIntersect2LineSegment(CPoint* pt, bool bCheckCrossOver);

	//�� a, b�� �� c, d�� ������ �� ���� �� �� �̵��� �� ������ ��ġ���� Ȯ���Ѵ�.
	bool		paralleSegments(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p);

	// - p�� �� �� a, b�� ���θ鼭 �� ���� x, y�࿡ ������ �ּһ簢�� ���ο� �ִ��� Ȯ���Ѵ�.
	// a, b, p�� ������ �� �ִٰ� �����Ѵ�.
	bool		inBoundingRectangle(vector2 p, vector2 a, vector2 b);

	// - �� �� a, b�� ������ ���а� �� �� c, b�� ������ ������ p�� ��ȯ�Ѵ�.
	// - ������ �������� ��� �ƹ����̳� ��ȯ�Ѵ�.
	bool		segmentIntersection(vector2 a, vector2 b, vector2 c, vector2 d, vector2& p);



	//����, �ﰢ��
	//�ﰢ�� �������� �߽��� ������������ ���Ѵ�.(http://kipl.tistory.com/113)
	int circumCenter(CPoint A, CPoint B, CPoint C, double *xc, double *yc);
	//�ﰢ�� �������� �߽��� ��������� ���Ѵ�.
	int circumCenter2(CPoint P, CPoint Q, CPoint R, double *xc, double *yc);
	//�ﰢ�� �������� �������� ���Ѵ�.
	double circumRadius(CPoint A, CPoint B, CPoint C);


//�簢��
	//�簢�� ������ ���ڿ��� �����Ѵ�.
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
	//0:lt, 1:rt, 2:rb, 3:lb, rb_cut�� true�̸� ����-1�� ���� �����ϰ� false�̸� ���� ��ǥ�� �����Ѵ�.
	CPoint		vertex(CRect r, int index, bool rb_cut = false);	

	//�־��� �簢�� ������ ����� �ʵ��� �������ش�.
	void		adjustRectRange(int32_t *l, int32_t *t, int32_t *r, int32_t *b, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool retainSize);
	//�̹����� ��� includeBR�� false�� �ؾ� ���� ��ǥ�� ��ȿ�ϴ�.
	void		AdjustRectRange(CRect& rect, CRect rLimit, bool bRetainSize = true, bool includeBR = false);
	void		AdjustRectRange(CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize = true);

	//������� ���ʿ� ���� �簢���� ���ο� ũ��� ������ ��� ���� ���¸� �����ϰ� ������ �ʿ䰡 ���� ��� ���.
	void		adjust_with_monitor_attached(CRect rOld, CRect &rNew);

	//rTarget�� ���ϴ� dRatio�� �����ϴ� �ִ� �簢���� ���Ѵ�.
	CRect		GetRatioRect(CRect rTarget, double dRatio, int attach = attach_hcenter | attach_vcenter);
	CRect		GetRatioRect(CRect rTarget, int w, int h, int attach = attach_hcenter | attach_vcenter);
	//rSub�� rMain�� ������ ���������� true�� �����Ѵ�.
	bool		RectInRect(CRect rMain, CRect rSub);
	//r���� except������ �����ϰ� cr�÷��� ä���.
	void		fill_except_rect(CDC* pDC, CRect r, CRect except, COLORREF cr);
	//�� �簢���� ��ġ�� ������ �����Ѵ�.
	CRect		getIntersectionRect(CRect r1, CRect r2);
	CRect		getIntersectionRect(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

	//�� �簢���� ��ġ�� ������ r1�� �������� ����ؼ� �����Ѵ�.
	double		getOverlappedRatio(CRect r1, CRect r2);
	double		getOverlappedRatio(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

	CRect		subtract(CRect r0, CRect r1);

//side �迭�� �ε����� resize�ϴ� ���� �ε����μ�
//DefWindowProc�� �ι�° �Ķ���Ϳ� (SC_SIZE + m_nSideIndex)�� ���̹Ƿ� �� ���ʸ� ������.
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
	//src�簢���� marginũ���� �׵θ� ������ pt���� �����ϴ� ������ �ε����� �����Ѵ�.
	//�ε����� CORNER_INDEX�� �����̸� �̴� DefWindowProc���� ����ϴ� ���ʿ� �����ϴ�.
	int			get_corner_index(CRect src, CPoint pt, int margin);

	//�������� �ָ� ���簢���� �̷�� ���� ��ǥ�� �����Ѵ�.
	void		getSquareEndPoint(int sx, int sy, int& ex, int& ey);

//�ٰ��� polygon ����
	//���� ���� �ٰ��� ���� �����ϴ��� �Ǻ�.
	bool PtInPolygon(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon0(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon1(CPoint* ptPolygons, CPoint pt, int nCorners);
	bool PtInPolygon2(CPoint *ptPolygons, CPoint pt, int nCorners);

	//�ٰ����� ���̸� ���Ѵ�. ��, ���� �ϳ��� �����Ǹ� �������� �ʴ´�.
	double		GetPolygonAreaSize(CPoint *pt, int nPoints);
	//�־��� �ٰ��� ������ �����ϴ� �ִ� �簢���� ���Ѵ�.
	CRect		get_max_rect(CPoint	*pt, int nPoints);
	CRect		get_max_rect(std::vector<CPoint> pt, int pt_max = -1);


//region ����
	HRGN		BitmapToRegion (HBITMAP hBmp, COLORREF cTransparentColor/* = 0*/, COLORREF cTolerance/* = 0x101010*/);
	HRGN		BitmapRegion(HBITMAP hBitmap, COLORREF cTransparentColor, bool bIsTransparent);
	HRGN		CreateRgnFromBitmap(HBITMAP hBmp, COLORREF color);

//ĸ�� ���
	//r�� ������ ��ǥ��.
	CImage*		capture_window(CRect r, CString filename);
	HBITMAP		CaptureScreenToBitmap(LPRECT pRect);
	HBITMAP		CaptureWindowToBitmap(HWND hWnd, LPRECT pRect = NULL);
	HBITMAP		CaptureClientToBitmap(HWND hWnd, LPRECT pRect = NULL);
	void		WriteBMP(HBITMAP bitmap, HDC hDC, LPTSTR filename);



//Ű���� �� �� ���� �⺻���� �����Ѵ�.
void		IME_Convert_To_NativeCode(HWND hWnd, bool bNative);


//�����Է�â�� �����.
void		HideIMM(HWND hwnd);


bool		IsLeapYear(int nYear);	//�������� �Ǵ�

//CRichEditCtrlEx�� ����Ͽ� AppendToLog�Լ��� �̿��ϴ� ���� �α׸� UI�� ǥ���ϱⰡ ������
//CRichEditCtrlEx�� �̿����� ���ϴ� ��Ȳ�� ������ ���� �ڵ���� ���̿��Ϸ��� �ڵ� ������ �ʿ��ϴ�.
//���� �Ʒ� �Լ��� �̿��Ѵ�.
//void		AppendToLog(CWnd* pWnd,)



//UI control ����
UINT		getButtonStyle(HWND hWnd);	//button�� ������ �����Ѵ�.

//���ӵ� ��ư�鿡 ���� �ϰ� ó���� �Լ�
//�⺻ CheckRadioButton���� ���� unselect ����� �������� �ʱ� ������
//�Ʒ� �Լ��� ������ �����ؼ� ����Ѵ�.
//id_offset�� 0���� ������ first ~ last���� ��� ����.
void		CheckRadioButtons(CWnd *pWnd, int idFirst, int idLast, int id_offset, int nCheck = BST_CHECKED);

//dialog based���� Ű�Է����� ������ �����ϴµ� CEdit�� ���� �Է�â�� ��Ŀ���� ������
//PreTranslateMessage���� ����Ű�� charŰ�� ó���ϱⰡ ����ϴ�.
//���� ���� ��Ŀ���� ���� ��Ʈ���� CEdit�̰� enable�̰� readonly�� �ƴ� ��쿡��
//PreTranslateMessage���� �Էµ� Ű�� ó���ϵ��� �Ѵ�.
bool		IsEditCtrlAcceptKeyState(CWnd *pWnd);

//start	: ���� �ε���.
//end	: ������ ���ϴ� n��° �׸�
//ex. quicksort(data, 9, 2);�� ȣ���ϸ�
//2�� �ε������� 9��° �׸��� data[2] ~ data[8]������ �����Ͱ� ���ĵȴ�.
//���� n���� �����͸� �����Ѵٸ� quicksort(data, n); �̶� ȣ���ϸ� �ȴ�.
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

/* template�Լ��� Ư¡
- ������ Ÿ�Կ� �����ϰ� �Լ� �ϳ��� Ŀ���� ������
  ������ Ÿ�Ը��� �Լ� �ٵ� obj�ڵ�� �����Ǵ� ������ �ִٰ� �˰��ִ�.
  ���� vs�� debug��忡���� �ӵ� ���ϰ� �߻��Ѵ�.
  release��忡���� �ӵ� ���ϰ� �߻����� �ʴ´�.
*/

//ġȯ �Լ�
template<class T> void SWAP(T& x, T& y)
{
	T temp	= x;
	x		= y;
	y		= temp;
}

//Ŭ���� �Լ�. Ŭ������ �Ͼ�� true�� �����Ѵ�.
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

//���� ��ȯ �Լ�
template<class T> void Cycle(T& n, T min, T max)
{
	if (n < min) n = max;
	else if (n > max) n = min;
}

//������ ����� default������ ����
template<class T> void Validate(T& n, T min, T max, T default)
{
	if (n < min || n > max)
		n = default;
}

//ġȯ �Լ�
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
//����
double		tangentfunc(double x, double y);
//�� ���� ������ ���Ѵ�. screencoord�϶��� Cartesian coordinate(������ǥ��)�϶��� y�� �ݴ��ӿ� ����.
double		GetAngle(double vx, double vy, bool bScreenCoord = true);
double		GetAngle(double x1, double y1, double x2,  double y2, bool bScreenCoord = true);
double		GetAngle(CPoint pt0, CPoint pt1, bool bScreenCoord = true);
double		GetAngle(CPoint a, CPoint b, CPoint c);	//3���� �̷�� ����
CPoint		GetCenterPoint(CPoint pt0, CPoint pt1);
double		GetDistance(CPoint pt0, CPoint pt1);

//�� ���� ������ �������� x3 �Ǵ� y3�� ���Ѵ�.
double		getLinePointX(double x1, double y1, double x2, double y2, double y3);
double		getLinePointY(double x1, double y1, double x2, double y2, double x3);
double		GetManhattanDistance(double x1, double y1, double x2, double y2);
//ptCenter�� �������� dAngle ��ŭ ȸ���� dDist�Ÿ��� ���� ��ǥ�� ���Ѵ�.
CPoint		GetRotatedPoint(CPoint ptCenter, double dAngle, double dDist);
//cx, cy�� �߽����� tx, ty���� degree�� ȸ���� ��� tx, ty���� ���� ��ǥ
void		get_rotated(int cx, int cy, int* tx, int* ty, double degree);
std::vector<CPoint>	get_rotated(int cx, int cy, CRect* r, double degree);

//���� ��ǥ <-> ������ ��ȯ
double		convert_gps_coord(int d, int m, double s);
void		convert_gps_coord(double gps, int &d, int &m, double &s);

//src���� ��� ���ڿ� ���� digits�ڸ����� ���� ����
void		combination(std::vector<TCHAR> src, CString temp, std::vector<CString>& result, int depth);

//http://www.gamedevforever.com/114 (2012�� ����Ʈ)
//���� rand()�� ���� ������ ���� �ӵ��� �����ٰ� �Ұ��Ǿ�����
//���� �����غ��� ������ �����ϰ� �ӵ��� rand()�� 2�� �� ������.
//release mode, 1000000���� SetPixel, GetTickCount()�� �ð�üũ.
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
	std::mt19937 engine((unsigned int)time(NULL));       // MT19937 ���� ����
	std::uniform_int<> distribution(min, max);                     // ���� ����
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
//�밢���� ���̷� ����, ���� ũ�⸦ ���Ѵ�.
void		get_HV_angle_from_diagonal(double diagonal, double *h, double *v, double width, double height);
void		get_HV_angle_from_diagonal(int diagonal, int *h, int *v, int width, int height);

//numlock, capslock, scrolllock
bool		GetLockKeyState(BYTE nLockKey);
void		SetLockKeyState(BYTE nLockKey, bool bOn);

HBITMAP		MakeDIBSection(CDC& dc, int width, int height);

//2D ���� ���󿡼� �̹� �˷��� �������� �������� ������ �� ���� ������� �Ÿ��� ���(by sucwon)
//cam_height	: ī�޶� ��ġ ����. ���� cm
//fl_x, fl_y	: focal length
//c_x, c_y		: ����
double		getObjectDistance(	int width, int height, int vanishing_y, int x, int y, int cam_height, double *dx, double *dy,
								int cali_width = 1920, int cali_height = 1080,
								double fl_x = 2361.130, double fl_y = 2357.436);

//�������� ũ�� resize�� �Ұ���.
void		resize_image(uint8_t *source_ptr,
						int source_width, 
						int source_height, 
						uint8_t *destination_ptr, 
						int destination_width, 
						int destination_height);


//resize_bilinear�� ncnn���� ���������� ���� �޸� ������ �߻��Ѵ�.
//�ϴ� ������� �ʴ´�.
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

//����� 1ä�� ���� �����ȴ�.
//�켱 ���ϰ� -90���� �����Ѵ�.
//dst�� �ݵ�� �޸𸮰� �Ҵ�Ǿ� �־�� �Ѵ�.
void		rotate90(uint8_t *src, int width, int height, uint8_t *dst, int degree);

//opencv�� flip�� ���� ���������� �̸� �浹�� ���ϱ� ���� mirror��� �̸��� �����.
//����� 1ä�� ���� �����ȴ�.
//method : 0(flip vertical), +(flip horizontal), -(both)
//dst�� �ݵ�� �޸𸮰� �Ҵ�Ǿ� �־�� �Ѵ�.
void		mirror(uint8_t *src, int width, int height, uint8_t *dst, int method);




//Ư�� ������ �Լ���
//������������(xml) ���� ����
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

		//plate = ���86��9993
		//local	= ���
		//type	= 86
		//use	= ��
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