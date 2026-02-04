//
//	Class:		CLayeredWindowHelperST
//
//	Compiler:	Visual C++
//	Tested on:	Visual C++ 6.0
//
//	Version:	See GetVersionC() or GetVersionI()
//
//	Created:	17/January/2002
//	Updated:	17/January/2002
//
//	Author:		Davide Calabro'		davide_calabro@yahoo.com
//									http://www.softechsoftware.it
//
//	Disclaimer
//	----------
//	THIS SOFTWARE AND THE ACCOMPANYING FILES ARE DISTRIBUTED "AS IS" AND WITHOUT
//	ANY WARRANTIES WHETHER EXPRESSED OR IMPLIED. NO REPONSIBILITIES FOR POSSIBLE
//	DAMAGES OR EVEN FUNCTIONALITY CAN BE TAKEN. THE USER MUST ASSUME THE ENTIRE
//	RISK OF USING THIS SOFTWARE.
//
//	Terms of use
//	------------
//	THIS SOFTWARE IS FREE FOR PERSONAL USE OR FREEWARE APPLICATIONS.
//	IF YOU USE THIS SOFTWARE IN COMMERCIAL OR SHAREWARE APPLICATIONS YOU
//	ARE GENTLY ASKED TO DONATE 1$ (ONE U.S. DOLLAR) TO THE AUTHOR:
//
//		Davide Calabro'
//		P.O. Box 65
//		21019 Somma Lombardo (VA)
//		Italy
//

/* Usage : in OnInitDialog or Other functions, add below codes.

!주의 : 반투명하게 하려는 dialog가 child면 적용되지 않는다. popup으로 되어있어야 한다.
	(in header...)
	CLayeredWindowHelperST m_layered;

	(in cpp...)
	m_layered.AddLayeredStyle(m_hWnd);
	m_layered.SetTransparent(m_hWnd, 50);
*/

#ifndef _LAYEREDWINDOWHELPERST_H_
#define _LAYEREDWINDOWHELPERST_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxwin.h>

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002
#endif

class CLayeredWindowHelperST  
{
public:
	CLayeredWindowHelperST();
	virtual ~CLayeredWindowHelperST();

	LONG AddLayeredStyle(HWND hWnd);
	BOOL SetLayeredWindowAttributes(HWND hWnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

	//alpha : 0(transparent), 255(opaque)
	//언뜻보면 alpha = 255일때가 완전 투명처럼 생각되지만 SetLayeredWindowAttributes() API의 bAlpha 정의와 일관되게 처리하기 위함.
	BOOL SetTransparent(HWND hWnd, BYTE alpha);
	BYTE GetTransparent() { return m_alpha; }

	static short GetVersionI()		{return 10;}
	static LPCTSTR GetVersionC()	{return (LPCTSTR)_T("1.0");}

private:
	typedef BOOL (WINAPI* lpfnSetLayeredWindowAttributes)(HWND hWnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

	//alpha : 0(transparent), 255(opaque)
	BYTE		m_alpha;
	HMODULE		m_hDll;
};

#endif
