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

!���� : �������ϰ� �Ϸ��� dialog�� child�� ������� �ʴ´�. popup���� �Ǿ��־�� �Ѵ�.
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

	//alpha : 100(transparent), 0(opaque)
	BOOL SetTransparent(HWND hWnd, BYTE alpha_percentage);
	BYTE GetTransparent() { return m_alpha_percentage; }

	static short GetVersionI()		{return 10;}
	static LPCTSTR GetVersionC()	{return (LPCTSTR)_T("1.0");}

private:
	typedef BOOL (WINAPI* lpfnSetLayeredWindowAttributes)(HWND hWnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

	BYTE		m_alpha_percentage;
	HMODULE		m_hDll;
};

#endif
