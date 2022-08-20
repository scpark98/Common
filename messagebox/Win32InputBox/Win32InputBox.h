#ifndef __03022006__WIN32INPUTBOX__
#define __03022006__WIN32INPUTBOX__

/*

This library is (c) Elias Bachaalany aka lallous <lallousx86@yahoo.com>
You may use this library under the following license agreement:

The zlib/libpng License.
---------------------------
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, 
and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; 
you must not claim that you wrote the original software. 
If you use this software in a product, an acknowledgment in the product 
documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, 
and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution. 

*/

/* 
- scpark
- ��ó : http://www.codeproject.com/Articles/13330/Using-Dialog-Templates-to-create-an-InputBox-in-C
- ���� : InputBox Ŭ������ ������ �����ϳ� UI�� ������ ���� ���۾����� ��� ��ġ�� ������� �Ѵ�.
�ݸ� �� Ŭ������ DlgResToDlgTemplate tool�� �̿��Ͽ� resource editor���� ������� ������
hexa data�� ��ȯ�Ͽ� Win32InputBox.cpp�� definputbox_dlg �迭�� �־��ָ� ������ ���� �� �ִ�.

TCHAR buf[128] = { 0 };
int r = CWin32InputBox::InputBox(_T("Title"), _T("prompt"), buf, 128, CWin32InputBox::NORMAL);

if ( r == IDOK ) //or IDCANCEL
::MessageBox( NULL, buf, "hello", MB_OK );

*/


#include <windows.h>
#include <tchar.h>

class CWin32InputBox;

// Structure used to orient the inputbox behavior
struct WIN32INPUTBOX_PARAM
{
	friend class CWin32InputBox;

	//
	IN OPTIONAL bool bMultiline;

	// Pass this as none zero so to use this memory dlg template
	IN OPTIONAL LPVOID DlgTemplateData;

	// Pass this as none ZERO so to load DLGTEMPLATE from resources
	IN OPTIONAL LPCTSTR DlgTemplateName;

	// passing both "DlgTemplateName" and "DlgTemplateData" ZERO will cause
	// the dialog to use his default embedded resource

	// Center on monitor or owner window?
	IN OPTIONAL bool bCenter;

	// Want to add more styles to the dialog?
	IN OPTIONAL DWORD dwStylesPlus, dwStylesMinus;
	IN OPTIONAL DWORD dwExStylesPlus, dwExStylesMinus;

	IN LPCTSTR szTitle, szPrompt;

	// Return buffer
	OUT LPTSTR szResult;
	IN DWORD nResultSize;

	// Owner window
	HWND hwndOwner;
	HINSTANCE hInstance;

	short xPos, yPos;
	int m_nTimeout;
	int m_nStyle; 

	WIN32INPUTBOX_PARAM();
private:
	HWND hDlg;
};

class CWin32InputBox
{
private:
	WIN32INPUTBOX_PARAM *_param;
	static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
	HWND _hwndEditCtrl;

	void InitDialog();
	void SetParam(WIN32INPUTBOX_PARAM *);
	WIN32INPUTBOX_PARAM * GetParam();


public:

	CWin32InputBox(WIN32INPUTBOX_PARAM *);
	~CWin32InputBox();

	enum BOX_STYLE { NORMAL = 0, MULTILINE, NUMERIC, PASSWORD, MESSAGE, };

	static void	show_template_dlg();

	static INT_PTR InputBoxEx(WIN32INPUTBOX_PARAM *);
	static INT_PTR InputBox(
		LPCTSTR szTitle, 
		LPCTSTR szPrompt, 
		LPTSTR szResult, 
		DWORD nResultSize,
		int nStyle = NORMAL,
		int nTimeout = 0,
		HWND hwndParent = 0);
};

#endif