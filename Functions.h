#pragma once

//#ifndef _SCPARK_FUNCTIONS_H
//#define _SCPARK_FUNCTIONS_H

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

#include <WinInet.h>

#define _std_cpp11 201103L
#define _std_cpp14 201402L
#define _std_cpp17 201703L
#define _std_cpp20 202002L

//������Ʈ�� �����ڵ� ���� ������ ����� �� fopen()�� UTF-8�� �����ϰ�
//��Ƽ����Ʈ ���� �����̸� ANSI�� ����ǵ��� �Ѵ�.
//��, UTF-8�� ����� BOM ���ڰ� ����� ��ϵǹǷ� ������ ��������� ������ ��
//fseek�� �̿��ؼ� ����� �����ؾ߸� �ش� ������ UTF-8 without BOM���� �����ȴ�.
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

//20231101 opencv�� trace�� �̹� ���ǵǾ� �־ trace�� Trace�� ������.
//��ũ�η� ���ǵǾ� �׷��� ��Ȥ ������������ ��µǴ� ������ �ִ�.
//�ϴ�, thread ���ο��� ����ϸ� ������ �߻��ϹǷ� ���� thread ���ο����� ������� ����.
//20240429 thread ���ΰ� �ƴ� �������� assertion failed�ǹǷ� �ϴ� ��� ����!
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

#define		GDIPVER 0x0110

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

//num���� ū n�� ����� ������ش�.
#define		MAKE_MULTIPLY_U(num, n)		(((num) + ((n)-1)) & ~((n)-1))
//num���� ���� n�� ����� ������ش�.
#define		MAKE_MULTIPLY_D(num, n)		(((num) - ((n)-1)) & ~((n)-1) | (n))

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
* 	���������� full_url�̳� ip, port�� �������־ �׿� �´� �������Լ��� ȣ���Ͽ� ä��.
	CRequestUrlParams params(m_server_ip, m_server_port, _T(""), _T("GET"));

	//���� sub_url�� �ʿ��� ��� �Ʒ��� ���� ä���ְ�
	params.sub_url.Format(_T("/lmm/api/v1.0/temp_envcheck/config-value-return?input_type=flag_windows_auto_update&mgrid=%s"), m_login_id);

	//���� request�� ȣ���Ѵ�. thread����� �ƴϸ� �ٷ� ����� params�� ä������ ���ϵȴ�.
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

	//thread�� ���� ��������(Ư�� ���� �ٿ�ε� request), request ����� �ٷ� �޾Ƽ� ó������(�ܼ� request)
	bool		use_thread = false;

	//m_request_id�� �ش� �۾��� �������� �����Ѵ�.
	int			request_id = -1;

	//200, 404...�� ���� HTTP_STATUS�� ������ invalid address ��� ���� �����ڵ嵵 ��� ���� int�� ����Ѵ�. 0���� ���� ���� result ���ڿ��� ���� ������ ����ִ�.
	int			status = -1;

	//��Ʈ�� http�� https�� �����ϴ� ���� �����ϴ�. m_isHttps=true �Ǵ� ip�� "https://"�� ���ԵǾ� ������ m_isHttps�� �ڵ� true�� �����ȴ�.
	CString		ip = _T("");

	int			port = 0;
	CString		sub_url;				//domain�� ������ ������ �ּ�
	CString		verb = _T("GET");
	//url�� ������ http���� https����, port�� 80���� 443�������� ������ �Ǵ��� �� ������ ����� ��õ��� �ʰų� ���� ��Ʈ��ȣ�� ����ϴ� ��쵵 ����.
	bool		is_https = true;
	CString		body;					//post data(json format)

	//token_header.Format(_T("token: %s"), ServiceSetting::strManagerToken);
	//�� �׸��� ������ �ݵ�� "\r\n"�� �ٿ�����ϴµ� �̴� requestAPI()���� �˾Ƽ� ó����.
	std::deque<CString> headers;

	//�ѹ� ȣ���ؼ� ������ �� port�� �ּ� �� url���� ������ �����Ͽ� �ٽ� request_url()�� ȣ���� ��
	//full_url�� ""�� ��������� ������ �� ���� �ٷ� ����ؼ� �ٽ� request�ϹǷ� ���� �����ϰ� �ȴ�.
	//�ݵ�� url ���ð��� �����Ͽ� �ٽ� request�� ��쿡�� �ݵ�� full_url = _T("")�� ������ְ� ȣ���ؾ� �Ѵ�.
	CString		full_url;				//[in][out] full_url�� �ְ� ȣ���ϸ� �̸� ip, port, sub_url�� ������ ó���Ѵ�. ""�� ȣ���ϸ� 
	CString		result;
	long		elapsed = 0;			//�ҿ�ð�. ms����.

	//���� �ٿ�ε� ����
	CString		local_file_path;		//url�� ������ �ٿ���� ��� ���� ���� full path ����.
	uint64_t	file_size = 0;			//url ���� ũ��
	uint64_t	downloaded_size = 0;	//������� ���� ũ��
	int			download_index = -1;	//n���� ���� �ٿ�ε�� ���� ������ �ε���. request_id�ʹ� �ٸ�.
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

//////////////////////////////////////////////////////////////////////////
//���μ��� ����
	//fullpath�� ""�̸� ���� �������Ϸ�, strFlag�� �⺻ ���Ϲ����� ���´�.
	CString		get_file_property(CString fullpath = _T(""), CString strFlag = _T("FileVersion"));
	//����, ������ �Ӽ�â�� ǥ���Ѵ�.
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
	//�ش� ���μ��� ������ �������� �ν��Ͻ� ī��Ʈ�� ����.
	//���� ���ϸ� �ָ� ���ϸ� �������� ��ü ��θ� �ָ� ��α��� �¾ƾ� ī��Ʈ ��.
	//ex. Ǯ�н��� c:\test.exe�� �ָ� d:\test.exe�� �������̶� ī��Ʈ���� �ʴ´�.
	int			get_process_running_count(CString processname);
	
	bool		KillProcess(CString processname);
	//���μ��� ���� ����.
	//return value : 1 : killed, 0 : fail to kill, -1 : not found
	bool		ProcessKill(CString szProcessName);

	HWND		GetWindowHandleFromProcessID(DWORD dwProcId);
	bool		IsDuplicatedRun();

	//cmd ��� ���� �� ����� ���ڿ��� ����.
	//wait_until_process_exit : ���� ���μ����� ���� ����ɶ����� ��ٸ���.
	//return_after_first_read : wait_until_process_exit�� false�� �ص� ��ð� ������ �ʴ� ��찡 �־�(ex. telnet)
	//�켱 �� ���� true�̸� �� ó�� read�Ŀ� �ٷ� �����Ų��.
	//����! osk.exe��� ����Ű���� ���α׷��� system�������� �ְ� SysWow64 �������� ���µ�
	//32bit ���α׷����� ShellExecute() �Ǵ� CreateProcess()�� �����ϸ� SysWow64�������� �ش� ������ ã���Ƿ� �����Ѵ�.
	//c:\\windows\\system32\\osk.exe�� �����ص� SysWOW64 ������ redirect�ǹǷ� ���� ������� �ʴ´�.
	//Wow64DisableWow64FsRedirection()�� �̿��ؼ� redirection�� disable�����ְ� ���� �� ����������� �Ѵ�.
	//"dir C:\\*.*"�� �����ϳ� "dir \"C:\\Program Files\\*.*\"" ����� �����Ѵ�.
	CString		run_process(CString cmd, bool wait_until_process_exit, bool return_after_first_read = false);

	//"dir \"C:\\Program Files\\*.*\"" ����� �� �����ϳ� ping -t�� ���� ������ �ʴ� ������ɾ notepad.exe�� ������ ���
	//����â�� ��� �����ִ�.
	CString		run_process(CString cmd);
	extern		void* g_wow64_preset;
	void		Wow64Disable(bool disable = true);

	//���� ���°� �����̵� ����, ���Ž�Ų��. sc queryex -> taskkill /pid -> sc delete
	//process_name�� �־����� �� �� ������ ���ŵȴ�.
	//���� ����(�Ǵ� ���񽺰� ���� ���) : true
	//���� ���� : false
	bool		kill_service(CString service_name, CString process_name = _T(""));

	//Console ������� GUI ������ ���ø����̼����� ����
	bool		is_gui_application(CString fullPath);

	//PID, ���μ��� �̸�, ������ Ÿ��Ʋ �̸�, ������ Ŭ���� �̸����� Ŭ������ ���� ���¸� ���Ҽ� �ֽ��ϴ�. from Devpia
	bool		CheckProcessUsingPID(unsigned long pid);
	bool		CheckProcessUsingWindowName(LPCTSTR className, LPCTSTR titleName);
	bool		CheckProcessUsingProcessName(LPCTSTR processName);

	//����� ����
	//main���� EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0); �� �����ϰ�
	//�� ���Ͽ� ���������� ����� g_dqMonitor�� �̿��ϸ� �ȴ�.
	//��, Win32API�� EnumDisplayMonitors()�� ȣ���Ҷ��� �ݵ�� g_dqMonitors.clear()�� ����� �ϹǷ�
	//enum_display_monitors()�Լ��� ��ü�Ѵ�.
	extern std::deque<CRect> g_dqMonitors;
	void		enum_display_monitors();
	BOOL		CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
	//r�� �����ִ� ����� �ε����� ����. �������� ������ ��𿡵� ���ٸ� -1�� ����.
	//entire_included�� true�̸� � ����Ϳ� ������ �����ִ� ��쿡�� �ش� �ε����� ����.
	int			get_monitor_index(CRect r, bool entire_included = false);
	//x, y�� �����ִ� ����� �ε����� ����
	int			get_monitor_index(int x, int y);
	//��Ƽ����� ��ü ���� �簢�� ����
	CRect		get_entire_monitor_rect();

	//::SetForegroundWindow()�� Win98���ĺ��ʹ� �������� �ʾ� ������ �ڵ�.
	void		SetForegroundWindowForce(HWND hWnd, bool makeTopMost = false);
	bool		is_top_most(HWND hWnd);

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
	CString		GetToken(CString src, CString separator, int index);

	//separator�� ���� CString���� TCHAR�� �����Ѵ�.
	//2char�̻��� ���ڿ��� �ϳ��� separator�� ���� ���� ���� ������
	//1�� �Ǵ� �� �̻��� ���� �ٸ� ���ڸ� separator��� ����� ���� ���� ���̴�.
	//20240426 separator�� �������� ���� Ư�� separator�� ���� ����� token�� �� �ٸ� separator�� ������ �� �����Ƿ�
	//�� ó���� �ſ� ����������. �׳� CString separator�� ó���Ѵ�.
	//include_rest�� nMaxToken�� ��õ� ��� ������ ����� �������� �ϴ��ĸ� ���ϴ� �ɼ��̴�.
	//���� ��� "LastUpdated : 1601-01-01 9:00:00"�� ���� ���ڿ��� ':'�� �Ľ��� ���
	//nMaxToken�� 2�� �� ��� dqToken[0] = "LastUpdated "�� �ǰ�
	//�ι�° ��ū�� " 1601-01-01 9"�� ������(���� ��밪�� " 1601-01-01 9:00:00"�� ���̴�)
	//include_rest�� true�� �ָ� dqToken[1] = " 1601-01-01 9:00:00"�� �ȴ�.
	//��, �ִ� ��ū ������ ������ ���� �� ������ ��ū�� �������� ó���� ���ΰ��� ���� �ɼ��̴�.
	int			get_token_string(CString src, std::deque<CString>& dqToken, CString separator = _T("|"), bool allowEmpty = true, int nMaxToken = -1, bool include_rest = false);
	int			get_token_string(TCHAR *src, TCHAR *separator, CString *sToken, int nMaxToken);
	int			get_token_string(char *src, char *separator, char **sToken, int nMaxToken);

	//��Ȥ \r, \n, \t, \\���� ���ڸ� �״�� Ȯ���� �ʿ䰡 �ִ�.
	CString		get_unescape_string(CString src);

	// a_value : 1.1.24050
	// b_value : Normal
	// c_value : True
	// ���� ���� �Ӽ��̸� �� ������ ���εǴ� ���ڿ��� �Ľ��Ͽ� std::map�� �־��ش�.
	// lfrf�� ���κи����ڿ��̰� ���� "\n"�̰ų� "\r\n" ���� �ְ�
	// separator�� ':' �̸��� ���� �����ϴ� �������̴�.
	// return value : �׸��� ����
	int			get_map_string(CString src, std::map<CString, CString>& map, CString lfrf = _T("\n"), CString separator = _T(":"));

	//dq�׸��� �ϳ��� ���ڿ��� �����ش�.
	CString		get_concat_string(std::deque<CString> dq, CString separator = _T("|"));

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

	//src���� sep�� �����ڷ� �ϳ��� �̾Ƴ��� �� �������� �ٽ� src�� ġȯ�Ѵ�.
	//src�� ��� ����ʿ� ����.
	CString		get_str(CString& src, CString sep = _T("|"));
	int			get_int(CString& src, CString sep = _T("|"));
	double		get_double(CString& src, CString sep = _T("|"));

	//unit			: -1:auto, 0:bytes, 1:KB, 2:MB, 3:GB ~
	//auto�� ���� 1000���� ���������� ������ �Ҽ����� 2�ڸ����� ǥ���Ѵ�.(ex 7.28TB)
	//floats		: �Ҽ����� �� �ڸ����� ǥ������
	//unit_string	: ������ ǥ���� ��
	//comma			: ���� �κп� �ڸ��� �޸��� ǥ���� ��
	CString		get_size_string(int64_t size, int unit = 1, int floats = 0, bool unit_string = true, bool comma = true);

	//src�� �Ľ��ؼ� Ư�� ���� �̻��� ���ڿ���� ������.
	std::deque<CString> parse_divide(CString src, int len);
	//src�� pos��ó���� �������� ã�� �� ��ġ�� �����Ѵ�.(���������� ������ ������ �̿�)
	int			find_punctuation(CString src, int pos);
	bool		is_punctuation(TCHAR ch);

	//isOneOf()�� is_one_of()�� ��ü��.
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


	//src ���ڿ��� set_of_keyword�� ������ �ܾ �ִ��� �˻�.
	//set_of_keyword�� �����ݷ����� �����ؼ� ���� ���� �Ǵ� ���ڿ��� ���� �� �ִ�.
	//ex. src = "abcd1234"�� �� set_of_keyword = "bc;21" => true, set_of_keyword = "212" => false
	bool		is_exist_keyword(CString src, CString set_of_keyword, bool case_sensitive = false, bool whole_word = false);

	//dqList���� element���� ��ġ�ϴ� �׸��� index�� �����Ѵ�. ������ -1�� ����.
	template <typename T> int find_index(std::deque <T> &dqList, T element)
	{
		std::deque<T>::iterator it = std::find(dqList.begin(), dqList.end(), element);
		if (it != dqList.end())
		{
			return distance(dqList.begin(), it);
		}

		return -1;
	}

	//str�� from ��ġ ���Ŀ� �ִ� ���� �������� num�� �־��ְ� ���� ������ġ�� return�Ѵ�.
	int	extract_digit_number(char *str, int from, double *num);

	//version string valid check
	//digits : �ڸ���(1.0.0.1�� ���� �ڸ��� 4)
	bool valid_version_string(CString versionStr, int digits);

	//���� �Ǵ� IP�ּҵ��� �׳� ���ڿ��� ���ϸ� 1.0.9.0�� 1.0.10.0���� �� ũ�ٰ� �����Ƿ�
	//.�� ���� ���ڷ� �������� �� ����� ���� �߻�(1.0.1.13 > 1.0.10.3���� ũ�ٰ� �Ǵ���)
	//�ᱹ �� �ڸ������� �����ؾ� �Ѵ�.
	//���ϰ��� strcmp�� ������ ��Ģ���� �Ǵ��Ѵ�.(+:str0�� ŭ, -:str1�� ŭ, 0:����)
	int	compare_string(CString str0, CString str1, TCHAR separator = '.');

	//src�� n�� ������ ���ڿ� ����. n���� ����, ���� �ʿ��� ��� ���
	CString make_string(CString src, int n);

	//http://yeobi27.tistory.com/280
	//A2W, A2T �� �� �ݴ� ��ũ�ε��� ������ ����ϹǷ� ���� ������ �ְ� ũ�� ���ѵ� �����Ƿ�
	//������ CA2W, CA2T���� ����Ѵ�. �� �� ��ũ�ε��� encoding�� ������ �� ����.
	std::wstring CString2wstring(const char* str);
	//std::string ss = CT2CA(CString(_T("test")); �� ���� CT2CA�� ����ϸ� ������.
	std::string CString2string(CString cs);
	//CString str(sstr.c_str());
	CString		string2CString(std::string sstr);
	//��Ƽ����Ʈ ȯ�濡�� �� �Լ��� ȣ���ؼ� ����ϸ� ��Ȥ ������������ �����Ѵ�.
	//�Ƹ��� �Լ������� �޸𸮰� �Ҵ�� �� ȣ���� ������ ����Ϸ��� ������ �� �� �ִ�.
	//�� �Լ��� �ٵ� �״�� ���� ������ �����Ƿ� �ϴ� �ٵ� �ڵ带 �״�� �����ؼ� ����Ѵ�.
	LPCWSTR		CString2LPCWSTR(CString str);
	//char chStr[100] = { 0, };�� ���� pointer ������ �ƴ� �迭�� ����� �����
	//chStr = CString2char(str); ������ ������ �߻��ϹǷ� �Ʒ��� ���� ����� ��.
	//sprintf(chStr, "%s", (LPSTR)(LPCTSTR)str);	//MBCS : ok, UNICODE : fail
	//sprintf(chStr, "%s", CStringA(str));		//both : ok
	//���Ϲ޾� ����� char* �������� ��� �� �ݵ�� delete [] ���ٰ�
	char*		CString2char(CString str);
	TCHAR*		CString2TCHAR(CString str);
	LPCSTR		CString2LPCSTR(CString str);
	LPCWSTR		LPCTSTR2LPCWSTR(LPCTSTR str, UINT codePage = CP_UTF8);
	WCHAR*		CString2WCHAR(CString str); //{ return (WCHAR*)(const WCHAR*)CStringW(str); }

	//cstr�� ��ȿ�� ���̸� �̹� �˰� �ִٸ� length�� ��������� ��Ȯ�ϴ�.
	//�׷��� ���� ��� cstr�� ���� '\0'�� ���� ��� ������ ���ڵ���� ���Ե� �� �ִ�.
	//cstr�� '\0'�� �����ٸ� �����ڵ�, ��Ƽ����Ʈ ȯ�濡�� CString str = cstr;�� ���� ó���ȴ�.
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

	//return���� char*�� �ݵ�� ��� �� free()����� ��.
	char*		replace(char* s, const char* olds, const char* news);

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
	//A safer and more efficient(http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf)
	std::string string_format(const std::string fmt_str, ...);
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

	//simple json parser. Common/json/rapid_json ��õ.
	CString		json_value(CString json, CString key);

	int			get_char_count(CString sStr, TCHAR ch, bool stop_at_first_mismatch = false, bool forward = true);
	CString		get_mac_address_format(CString src, TCHAR separator = ':');

//������ ��ȯ
	CString		i2S(int64_t nValue, bool bComma = false, bool fill_zero = false, int digits = 0);
	CString		i2HS(int64_t nValue, bool bCapital = true);
	CString		d2S(double dValue, bool bComma = false, int nfDigit = -1);	//nfDigit : �Ҽ��� �ڸ���. -1�̸� �״�� ���.

	//IPv4 ���ڿ��� ���ڷ� �ٲٴ� ���� �ڵ��̹Ƿ� ���� ���ڿ� �� �Ϲ� ���ڿ� �񱳿� ������� ����.
	//���� ���ڿ� ���� �� �ڸ��� ������ �ٸ� �� �����Ƿ� ����� �� ����.
	//���ڿ� �񱳰� �ʿ��ϴٸ� compare_string()�� ����� ��.
	uint32_t	IP2int(CString IP);

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
	template<class T> CString get_binary_string(T n, bool separator = true)
	{
		CString res;

		while (n != 0)
		{
			res = (n % 2 == 0 ? '0' : '1') + res;
			n /= 2;
		}

		//4�ڸ����� ����
		int len = res.GetLength();
		if (len % 4 != 0)
		{
			int count = 4 - len % 4;
			while (count-- > 0)
			{
				res = '0' + res;
			}
		}

		//4�ڸ��� ���� �߰�
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
//���� ����
	//_tsplitpath("c:\\abc/def\\123.txt", ...)�� �����ϸ�
	//"c:", "\\abc/def\\", "123", ".txt" �� ���� �и��Ǵµ� ������ ����ϴ� ��밪�� �޶� �����Ѵ�.
	//"c:\\", "c:\\abc/def", "123", "txt", "123.txt�� ���� �����Ѵ�.
	//part : fn_drive(drive), fn_folder(drive+folder), fn_last_folder(folder name), fn_title(filetitle), fn_ext(ext), fn_name(filename)
	//���� path�� "d:\\aaa\\b.abc"�̰� b.abc�� ������ �ƴ� ������� �����ȴ�.
	//�������� ���������� �����ؼ� ó���ϴ� �ڵ�� �ʼ���.(���� �����ϴ� ��쿡�� �˻簡 �����ϴ�)
	//��, path�� "�����ҹ���(\\192.168.1.103) (Y:)"�� ���� ��Ʈ��ũ ��θ� ������ ����̺� ������ ����
	//�и��ؼ��� �ȵǹǷ� �׳� �����ؾ� �Ѵ�.
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

	//Ȯ���� ���� ���ڿ��� ���Ͽ��� ��ȭ������ filter string�� �����Ѵ�.
	//simple : "bmp;jpg;jpeg;png;webp;gif;yuv;raw => "JPG files|*.jpg|bmp|*.bmp|
	//extension_group = FILE_EXTENSION_VIDEO or FILE_EXTENSION_SOUND or FILE_EXTENSION_IMAGE or FILE_EXTENSION_MEDIA...
	//���� �̿ϼ�!
	//CString		get_filter_string(CString extension_group, bool simple = true);

	//������ �ִ� ���ϵ� �� filetitle�̰� extension�� �ش��ϴ� ���ϸ��� �����Ѵ�.
	std::deque<CString>		get_filelist_from_filetitle(CString folder, CString filetitle, CString extension);
	std::deque<CString>		get_filelist_from_filetitle(CString filename, CString extension);

	uint64_t	get_file_size(CString sfile);
	uint64_t	get_folder_size(CString path);
	//unit_limit	: 0:bytes, 1:KB, 2:MB, 3:GB (default = 3)
	//unit_string	: ������ ǥ���� �� (default = true)
	//������ ���� ""�� ������.
	CString		get_file_size_string(CString sfile, int unit = 1, int floats = 0, bool unit_string = true);
	CTime		GetFileCreationTime(CString sfile);
	//Ž���⿡�� �����ϸ� last modified time�� ���������� web���� �ٿ���� ������ ���� �ٿ�޾� ������ �ð����� �����.
	CTime		GetFileLastModifiedTime(CString sfile);
	CTime		GetFileLastAccessTime(CString sfile);
	CString		GetMostRecentFile(CString sFolder, CString sWildCard = _T("*.*"), int nReturnType = 1);
	CString		GetMostRecentDateFile(CString sFolder, CString sWildCard = _T("*.*"));	//���� �ֱ� ��¥ ���ϸ� ����
	CString		GetFileProperty(CString sFilePath, CString sProperty);

	//Ȯ�� �ʿ�
	//CString		set_file_property(CString sFilePath, CString sProperty, CString value);

	size_t		read_raw(CString sfile, uint8_t *dst, size_t size);
	bool		save2raw(CString sfile, uint8_t *data, size_t size);
	int			RenameFiles(CString folder, CString oldName, CString newName, bool overwrite = false, bool bWholename = true, bool bRecursive = false);
	bool		delete_file(CString fullpath, bool bTrashCan = false);
	int			get_text_encoding(CString sfile);
	bool		save(CString filepath, CString text, int code_page = CP_UTF8);
	bool		file_open(FILE** fp, CString mode, CString file);

	//text ������ ��� dqList�� �־��ش�.
	bool		read_file(CString filepath, std::deque<CString> *dqList, bool using_utf8);

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
	//check_prefix�� true�̸� http, https���� üũ�Ѵ�. ���� ������� �ִµ��Ͽ� �켱 ������.(https://mathiasbynens.be/demo/url-regex)
	bool		is_valid_url(CString url, bool check_prefix);
	void		GetURLFileInfo(CString sURL, bool &bInURL, bool &bFileType);
	bool		ReadURLFile(LPCTSTR pUrl, CString &strBuffer);
	void		ReadURLFileString(CString sURL, CString &sString);

	bool		parse_url(CString full_url, CString &ip, int &port, CString &sub_url, bool &is_https);

	//url�� ȣ���Ͽ� ������� �����ϰų� ������ ���� ���Ϸ� �ٿ�ε� �Ѵ�.
	//local_file_path�� ""�̸� ������� ���ڿ��� ���Ϲ޴´�.
	//local_file_path�� �����Ǿ� ������ ���Ϸ� �ٿ�޴´�.
	//(�̶� ���ϰ��� "")
	//���ϰ��� 200�� �ƴ� ���� ���ϵ� �����ڵ�� result_str�� ����� ���� �޽����� �����Ͽ� ���� ó���Ѵ�.
	//port�� ������ http�� https�� �����ϴ� ���� �����ϹǷ� ��Ȯ�� �����ڷ� �����ؾ� �Ѵ�.
	//(�ݵ�� https�� �����ؾ� �ϴ� ���, port�� �⺻���� 443�� �ƴ϶�� �ּҸ� https://~�� ����Ͽ� ȣ���ؾ� �Ѵ�)
	//DWORD		request_url(CString &result_str, CString ip, int port, CString sub_url, CString verb = _T("GET"), std::vector<CString> *headers = NULL, CString jsonBody = _T(""), CString local_file_path = _T(""));
	//DWORD		request_url(CString& result_str, CString full_url, CString verb = _T("GET"), std::vector<CString>* headers = NULL, CString jsonBody = _T(""), CString local_file_path = _T(""));
	void		request_url(CRequestUrlParams* params);


	//�⺻ �������� ������ ������ �̸��� �����ϰ� �ΰ������� ���, ������ ���� �� �ִ�.
	CString		get_default_browser_info(CString* pPath = NULL, CString* pVersion = NULL);

	//Content-Type: multipart/form-data ������ �̿��� ���������� ���� ���� �Լ�
	bool		HttpUploadFile(CString url, CString filepath, int chatIndex);

	//������ �ѱ۸� ���Ͽ� ���� ó�������� request_url()�Լ� ��� �߰��Ͽ� �׽�Ʈ �غ�����
	//�������� �����ε��Ͽ� �켱 �� �Լ� ����� ������...
	bool		HttpDownloadFile(CString url, CString local_path = _T(""));


//webView2 Runtime
	bool		is_WebView2Runtime_installed();
	bool		install_WebView2Runtime(CString runtimeExePath, bool silentInstall);

//////////////////////////////////////////////////////////////////////////
//���� ����
	//�����ϸ� PathIsDirectory() ����� ��
	bool		IsFolder(CString sfile);				//�������� ��������
	bool		isFolder(char *sfile);
	//���ϸ��̳� ������ '\\', '/' ȥ���� ��찡 �����Ƿ� CString�� '==' �����ڷ� ���ؼ� �ȵȴ�. 
	bool		IsFileFolderPathIsEqual(CString file0, CString file1, bool bCaseSensitive = false);
	CString		GetParentDirectory(CString sFolder);	//���� ������ ���� �������� �����Ѵ�.

	//compare_only_filename : fullpath�� ��������, ���ϸ� �����ؼ� ��������. default = false;
	void		sort_like_explorer(std::deque<CString> *dq, bool compare_only_filename = false);
	void		sort_like_explorer(std::deque<CString>::iterator _first, std::deque<CString>::iterator _last, bool compare_only_filename = false);

	//������ �������� ���� ����� ���´�.
	//sNameFilter�� ���ϵ�ī��� ���� �༭ �˻��ؾ� �Ѵ�.
	//������Ʈ ��ɰ� �����ϰ� ����ǥ�� ��ǥ�� ���� ���ϵ�ī�带 �̿��� �� �ִ�.
	//sNameFilter = "test*", sExtFilter = "jpg;bmp;" �� ���� �Է��ϸ�
	//test�� �����ϰ� Ȯ���ڰ� jpg, bmp�� ���� ����� ���´�.
	//sExceptStr = "test;temp;error" �� ���� �����ݷ����� �����Ͽ� �˻� ������ ���ϸ� ���� ����.
	//����! dqFiles�� �� �Լ��� ���� �ʱ�ȭ���� �����Ƿ� �ʿ��� ��� �ʱ�ȭ�Ͽ� ȣ���� ��!
	//bRecursive�̸� ���� ������ ���� ��� ���ϵ鵵 �˻��Ѵ�. ���� ��ü�� ����Ʈ�� ���Ե��� �ʴ´�.
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
	//include_files�� true�̸� ���ϵ� ���Եȴ�.
	int	get_sub_folders(CString root, std::deque<CString>* list = NULL, bool special_folders = false, bool include_files = false);
	//�� �Լ��� ��ü ���� ������ ����̳� �������� ��� ���ϱ� ������ Ư�� ������ ���� �ӵ��� �ſ� ������
	//������ ���� ���� ������ üũ�ϴ� �Լ��� �߰��Ѵ�.
	bool has_sub_folders(CString root);

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
	CString get_known_folder(int csidl);

//////////////////////////////////////////////////////////////////////////
//��Ʈ��ũ, ���ͳ�
	bool		GetNICAdapterList(IP_ADAPTER_INFO* pAdapters, int& nTotal, int nMax = 10);
	bool		GetNetworkInformation(CString sTargetDeviceDescription, NETWORK_INFO* pInfo);
	bool		CheckInternetIsOnline();
	bool		IsAvailableEMail(CString sEMail);
	CString		get_mac_addres(bool include_colon = true);
	CString		get_ip_error_string(DWORD error_code);
	bool		port_is_open(const std::string& address, int port);

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
//��(shell), ������(window), ������Ʈ��(registry), �ý���(system)
	//MAX_COMPUTERNAME_LENGTH(15) ���̱����� ���ϵʿ� ����.
	//GetComputerName API �Լ��� �׻� �빮�ڷ� �����Ѵ�.
	//�׳� Ȯ���ϰ� GetComputerNameString().MakeLower() ��� ����
	//��ҹ��ڸ� ��Ȯ�� �Ͽ� ���ϴ� ���� ����.
	//=>255���ڱ��� ���ϵǴ� GetComputerNameEx()�� ������.
	CString		get_computer_name_string();
	//���������� ����ü�� ����
	OSVERSIONINFOEX	get_windows_version();
	//���������� ���ڿ� '.'�� ����. ex. "10.0.12345"
	CString		get_windows_version_number();
	DWORD		get_windows_major_version();
	//detail=true�̸� edition �������� ����
	CString		get_windows_version_string(bool detail = true);
	//
	//CString		get_windows_version_string(CString version);

	CString		get_system_label(int csidl, int *sysIconIndex = NULL);

	//�� ���� �����찡 ��ġ�� �� �����ǰ� �缳ġ���� ������ �����ȴ�.
	//��, HDD �����ÿ��� �״�� ����ǹǷ� �ӽſ� ���� unique�ϴٰ� �� �� ����.
	CString		read_windows_GUID();
	CString		create_GUID();

	//������10�̻��� auto_update�� �׻� true.
	//(registry���� Ư������ �߰��Ͽ� ������ ���� false�ε� ���ϵ�)
	//WinXP���� �ڵ� ������Ʈ�� ������� �������� �ص� true�̸� �Ʒ� level=1�� ���ϵ�.
	//level�� AutomaticUpdatesNotificationLevel ����.
	//(0:aunlNotConfigured, 1:aunlDisabled, 2:aunlNotifyBeforeDownload, 3:aunlNotifyBeforeInstallation, 4:aunlScheduledInstallation)
	bool		get_windows_update_setting(bool& auto_update, int& level);

	//SystemParametersInfo(SPI_GETSCREENSAVEACTIVE...)���δ� ����� �������� ������ ���Ѵ�.
	bool		get_screensaver_setting(int *timeout = NULL, int* use_secure = NULL);

	//�� �� �׽�Ʈ �ʿ�!
	HWND		GetHWndByExeFilename(CString sExeFile, bool bWholeWordsOnly = false, bool bCaseSensitive = false, bool bExceptThis = true);
	HANDLE		GetProcessHandleByName(LPCTSTR szFilename);

	CWnd*		FindWindowByCaption(CString sCaption, bool bMatchWholeWord = FALSE);
	HINSTANCE	FindExecutableEx(LPCTSTR lpFile, LPCTSTR lpDir, LPTSTR lpResult);

	LONG		IsExistRegistryKey(HKEY hKeyRoot, CString sSubKey);
//#ifndef _USING_V110_SDK71_
	//HKEY_LOCAL_MACHINE\\SOFTWARE\\MyCompany ���� �о�� ��� x64�̸� ���� �� ��ο��� �о������
	//32bit�̸� HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\MyCompany ���� �о�´�.
	//!!�ݵ�� Linker->Manifest File���� Admin���� ������ ��!!
	LONG		get_registry_int(HKEY hKeyRoot, CString sSubKey, CString sEntry, DWORD *value);
	//!!�ݵ�� Linker->Manifest File���� Admin���� ������ ��!!
	LONG		get_registry_string(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString *str);
	//!!�ݵ�� Linker->Manifest File���� Admin���� ������ ��!!
	LONG		set_registry_int(HKEY hKeyRoot, CString sSubKey, CString sEntry, DWORD value);
	//!!�ݵ�� Linker->Manifest File���� Admin���� ������ ��!!
	LONG		set_registry_string(HKEY hKeyRoot, CString sSubKey, CString sEntry, CString str);
//#endif

	//Windows visual effect registry
	bool		set_windows_visual_effects();


	double		GetProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double default);
	bool		WriteProfileDouble(CWinApp* pApp, LPCTSTR lpszSection, LPCTSTR lpszEntry, double value);

	//int			GetSystemImageListIcon(CString szFile, BOOL bDrive);

	//CTreeCtrl, CListCtrl��� ���õ� �׸� ��ü�� �̹����� ����(drag�ÿ� ���)
	void		get_GdiplusBitmap_from_selected_item(CWnd * pWnd);


	void		SystemShutdown(int nMode);		// 0:logoff  1:reboot  2:shutdown
	void		SystemShutdownNT(int nMode = 2);	// 1:reboot  2:shutdown

	void		HideTaskBar(bool bHide = TRUE);	// FALSE => Show Taskbar

	//taskbar�� ũ�� �� ���� show/hide ���¸� �����Ѵ�. (�ɼ� ���� ���´� get_taskbar_state()���� �Ǻ��ؾ� ��)
	bool		get_taskbar_size(CSize *sz = NULL);
	//�������� auto hide������ get_taskbar_state(ABS_AUTOHIDE)�� ȣ���Ͽ� �Ǻ�.
	//sz�� �ָ� ũ�������� �� �� �ִ�.
	//��7���ʹ� ABS_ALWAYSONTOP�� �׻� true�̹Ƿ� ABS_ALWAYSONTOP�� �Ǻ��ϴ� ���� �ǹ̾���.
	bool		get_taskbar_state(UINT state, CSize *sz = NULL);


	BOOL		IsWow64();
	BOOL		IsXpOr2000();
	BOOL		Is64BitWindows();
	void		ClickMouse(int x, int y);

	//app�� ��ġ�� ũ�⸦ ������Ʈ���� �����ϰ� ���� �� �ٽ� ������Ų��.
	//resize�� �ȵǴ� ���̾�α��� ���� resize��Ű�� �ʰ� ���� ũ��� ǥ���ؾ� �Ѵ�.
	void		RestoreWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""), bool use_maximize = true, bool resize_window = true);
	void		SaveWindowPosition(CWinApp* pApp, CWnd* pWnd, CString sSubSection = _T(""));


	//CPU Usage ����
	LPBYTE		GetPerformanceData(LPTSTR src);
	int			GetCounterValue(const int& objId, const int& counterId, const char* instanceName, PERF_DATA_BLOCK **dataBlock, LONGLONG &value);
	double		get_cpu_usage(const char* process);
	HRESULT		get_cpu_temperature(LPLONG pTemperature);

	//�޸�, memory
	//���� ���� �޸𸮸� �����Ѵ�. (total_memory : ��ü �޸� �뷮)
	uint64_t	get_available_memory(uint64_t *total_memory = NULL);

#ifndef _USING_V110_SDK71_
	//���� ���μ����� �޸� ��뷮�� ���Ѵ�.
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
	uint64_t	get_disk_free_size(CString sDrive);
	uint64_t	get_disk_total_size(CString sDrive);
	//���� �Լ��� ũ�⸦ ���� �� get_size_string()�� �̿��� ��
	//CString		GetDiskSizeString(CString sDrive, int unit = 3, int nfDigit = 0);	// "1.25G / 380.00G", nfDigit�� �Ҽ��� �ڸ���
	CString		GetHDDSerialNumber(int nPhysicalDrive);
	CString		GetHDDVolumeNumber(CString sDrive);

	//HDD serial�� HardwareInfo.exe�� �˷��ִ´�� S4EVNM0T230338R 15�ڸ� ����,
	//�Ǵ� 0025_3852_2190_FE03 ���� ���·� ���ϵȴ�.
	//� PC�� ���� �˷��ִ� ���� �� ���α׷����� ���� ���� ������(mwj, sdh)
	//� PC�� ���� �˷��ִ� ���� �ٸ� �������� �����Ѵ�.(scpark)
	//�� �̷� ���̰� �߻��ϴ����� ���� �� �� ������
	//unique�� Ű���� ����ϰ��� ���̹Ƿ� �켱 16�ڸ��� ���缭 ����Ѵ�.
	//unify16 = true�̸� 16�ڸ��� ���Ͻ��� �����Ѵ�.
	CString		get_HDD_serial_number(int index, bool unify16 = true);

	//��ũ ����̺� ����� ���´�. include_legacy = true�̸� floppy, cdrom���� �ִ´�.
	void		get_drive_map(std::map<TCHAR, CString> *drive_map, bool include_legacy = false);
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

	CString		get_last_error_string(bool show_msgBox = false);
	CString		get_last_error_string(DWORD dwError, bool show_msgBox = false);

//////////////////////////////////////////////////////////////////////////
//date, time ��¥/�ð�
	CString		get_date_string(CTime t, CString sep = _T("-"));
	CString		get_date_string(COleDateTime t, CString sep = _T("-"));
	CString		get_date_string(__timeb32 tb, CString sep = _T("-"));
	CString		get_date_string(SYSTEMTIME st, CString sep = _T("-"));

	CString		get_time_string(CTime t, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_string(COleDateTime t, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_string(__timeb32 tb, CString sep = _T(":"), bool h24 = true, bool sec = true);
	CString		get_time_string(SYSTEMTIME st, CString sep = _T(":"), bool h24 = true, bool sec = true, bool msec = true);

	//type 0(date), 1(time:24h), 2(date+time) ��-��-�� ��:��:�� �������� ���� �ð� ����. mid�� ��¥�� �ð� ���� ���ڿ�
	CString		get_datetime_string(CTime t, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_string(COleDateTime t, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_string(__timeb32 tb, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true);
	CString		get_datetime_string(SYSTEMTIME st, int type = 2, bool sep = true, CString mid = _T(" "), bool h24 = true, bool sec = true, bool msec = true);

	//type 0(date), 1(time:24h), 2(date+time) ��-��-�� ��:��:�� �������� ���� �ð� ����. mid�� ��¥�� �ð� ���� ���ڿ�
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
	double		GetElapsedTime(__timeb32 pOldTime);	//pOldTime�� ���� �ð��� ���� ���
	//ts���� �Ѱ� �޾� "a�� b�ð� c�� d��" ���·� ǥ��(format 0 = "?�� ?�ð� ?�� ?��", 1 = "00:00:00") 
	CString		GetDayTimeCountString(int format, CTimeSpan ts, bool bShowZero = true, bool bIncludeSec = true);
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
	//Gdiplus::MeasureString()�� ������ ��ü.(http://www.soen.kr/lecture/library/gdiplus/1-7.htm)
	Gdiplus::RectF measure_string(Gdiplus::Graphics* g, Gdiplus::Font& font, LPCTSTR String, int length = -1);

	//Gdiplus�� �̿��� �ؽ�Ʈ ���
	CRect		draw_text(Gdiplus::Graphics *g,
							int x, int y, int w, int h,
							CString text,
							float font_size,
							int font_style,
							int shadow_depth,
							float thickness,
							CString font_name = _T("���� ���"),
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
							CString font_name = _T("���� ���"),
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							UINT align = DT_CENTER | DT_VCENTER);

	//text�� ����ȼ� �ʺ� max_width�� ���� ��� ...�� �Բ� ǥ�õ� ������ġ�� ����.
	//�� �Լ��� DrawText�ÿ� DT_END_ELLIPSIS�� �༭ ����ϹǷ� �켱 ��� ����!
	int			get_ellipsis_pos(CDC* pDC, CString text, int max_width);

	//20220914 DrawLine�� DrawLinePt�� ���� �̸����� �ϴ� ��ȣ�ϴٴ� ������ �߻��Ͽ� DrawLinePt�� ����.
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
	
	//.ico ������ ������ ũ�⸦ �����ؼ� �ε��� �����ϴ�. LoadIcon()���δ� �ȵǸ� PNG�� ���� �̹����� �Ұ��ϴ�.
	HICON		load_icon(HINSTANCE hInstance, UINT nID, int cx, int cy = 0);
	//�ش� DC�� �׸��� �������� ���� ũ�⸦ �����Ѵ�.
	CSize		draw_icon(CDC* pDC, HICON hIcon, CRect r);

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
	void gradient_rect(CDC* pDC, CRect &rect, std::deque<Gdiplus::Color> dqColor, bool vertical = false);
	enum GRADIENT_RECT_PRESET
	{
		gradient_rect_white_black_white = 0,
		gradient_rect_white_gray128_white,
		gradient_rect_black_white_black,
		gradient_rect_black_gray128_black,
	};
	//���� �̱���
	void		gradient_rect(CDC* pDC, CRect &rect, int preset, bool vertical);
	void		safe_release_gradient_rect_handle();

//�̹����� ǥ�õǰ� �ִ� ���� ������ ȭ����� ��ǥ�� �ָ� �̹������� ���� ��ǥ�� �����Ѵ�.
//��, ���� �̹������� ���� ��ǥ�� �̹��� ũ�⸦ ����� ��� �������� -1���� ä���� �����Ѵ�.
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, double sx, double sy, double *dx, double *dy);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint *pt_dst);
	void		get_real_coord_from_screen_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect *r_dst);

	//�̹����� ǥ�õǰ� �ִ� ���� ������ �̹��� ���� ��ǥ�� �ָ� ȭ����� ��ǥ�� �����Ѵ�.
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, double sx, double sy, double *dx, double *dy);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CPoint pt_src, CPoint *pt_dst);
	void		get_screen_coord_from_real_coord(CRect rDisplayedImageRect, int srcWidth, CRect r_src, CRect *r_dst);


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


//�簢�� Rectangle
	//�簢�� ������ ���ڿ��� �����Ѵ�. (default : 2)
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

	//0:lt, 1:rt, 2:rb, 3:lb, rb_cut�� true�̸� ����-1�� ���� �����ϰ� false�̸� ���� ��ǥ�� �����Ѵ�.
	CPoint		vertex(CRect r, int index, bool rb_cut = false);	

	//�־��� �簢�� ������ ����� �ʵ��� �������ش�.
	void		adjust_rect_range(int32_t *l, int32_t *t, int32_t *r, int32_t *b, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool retainSize);
	//�̹����� ��� includeBottomRight�� false�� �ؾ� ���� ��ǥ�� ��ȿ�ϴ�.
	void		adjust_rect_range(CRect& rect, CRect rLimit, bool bRetainSize = true, bool includeBottomRight = false);
	void		adjust_rect_range(CRect& rect, int32_t minx, int32_t miny, int32_t maxx, int32_t maxy, bool bRetainSize = true);

	//������� ���ʿ� ���� �簢���� ���ο� ũ��� ������ ��� ���� ���¸� �����ϰ� ������ �ʿ䰡 ���� ��� ���.
	void		adjust_with_monitor_attached(CRect rOld, CRect &rNew);

	//rTarget�� ���ϴ� dRatio�� �����ϴ� �ִ� �簢���� ���Ѵ�.
	CRect		get_ratio_max_rect(CRect rTarget, double dRatio, int attach = attach_hcenter | attach_vcenter);
	CRect		get_ratio_max_rect(CRect rTarget, int w, int h, int attach = attach_hcenter | attach_vcenter);
	//w x h �簢���� target�ȿ� ���� �� �߾ӿ� ǥ�õǰ� �ϴ� �簢�� ������ �����Ѵ�.
	//w, h���� target�� �������� target���� ū ������ ���ϵ� ���̴�.
	CRect		get_center_rect(CRect target, int w, int h);

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

	//��ũ���� ǥ�õ� �̹����� �׷��� �簢���� ���� �̹������� �簢�� ��ǥ
	//sr : �̹����� �׷��� �簢��
	//displayed : �̹����� ǥ�õǰ� �ִ� �簢�� ����
	//real : ���� �̹����� ũ��
	//resized : zoom in/out�� ���� ����� ũ��
	CRect		get_real_from_screen_coord(CRect sr, CRect displayed, CSize real, CSize resized);

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
	//Ư�� ������ ĸó�Ͽ� HBITMAP���� �����Ѵ�.
	//resourceID�� �ָ� �ش� �̹����� overlay�Ͽ� �����Ѵ�.(watermark�� ���� �뵵�� ����)
	HBITMAP		capture_screen_to_bitmap(LPRECT pRect, UINT id = 0, int dx = 0, int dy = 0, bool show_cursor = false);
	HBITMAP		CaptureWindowToBitmap(HWND hWnd, LPRECT pRect = NULL);
	HBITMAP		CaptureClientToBitmap(HWND hWnd, LPRECT pRect = NULL);
	//hwnd�� �ָ� �ش� ������ ������ ĸó������ �����쿡���� ���ʿ��� ������� �������� ó���ϹǷ�
	//pRect�� �༭ ������ ������ ĸó��Ų��.
	HBITMAP		PrintWindowToBitmap(HWND hTargetWnd, LPRECT pRect = NULL);

//HBITMAP
	void		draw_bitmap(HDC hdc, int x, int y, HBITMAP hBitmap);
	void		save_bitmap(HBITMAP bitmap, LPCTSTR filename);

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
DWORD		getButtonStyle(HWND hWnd);	//button�� ������ �����Ѵ�.

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
#ifndef SWAP
template<class T> void SWAP(T& x, T& y)
{
	T temp	= x;
	x		= y;
	y		= temp;
}
#endif

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


//MFC Common Controls
BOOL		recreate_combobox(CComboBox* pCombo, LPVOID lpParam = NULL);



//Ư�� ������ �Լ���
//������������(xml) ���� ����
bool		SavePlateInfoFile(char* sfile, char* sPlate, RECT* rect = NULL);

void		printMessage(std::string msg, uint8_t bNewLine = true);

int readFilenames(std::vector<std::string> &filenames, const std::string &directory);

//CMenu
//HMENU���� �޴�ID�� ĸ���� ���´�.
bool	get_menu_item_info(HMENU hMenu, UINT uItem, UINT *uID, CString *caption, BOOL fByPos = FALSE);
//#endif
