// WndShadow.h : header file
//
// Version 0.3
//
// Copyright (c) 2006-2007 Mingliang Zhu, All Rights Reserved.
//
// mailto:perry@live.com
//
//
// This source file may be redistributed unmodified by any means PROVIDING 
// it is NOT sold for profit without the authors expressed written 
// consent, and providing that this notice and the author's name and all 
// copyright notices remain intact. This software is by no means to be 
// included as part of any third party components library, or as part any
// development solution that offers MFC extensions that are sold for profit. 
// 
// If the source code is used in any commercial applications then a statement 
// along the lines of:
// "Portions Copyright (c) 2006-2007 Mingliang Zhu"
// must be included in the "Startup Banner", "About Box" or "Printed
// Documentation". This software is provided "as is" without express or
// implied warranty. Use it at your own risk! The author accepts no 
// liability for any damage/loss of business that this product may cause.
//
/////////////////////////////////////////////////////////////////////////////
//****************************************************************************

//****************************************************************************
// Update history--
//
// Version 0.3, 2007-06-14
//    -The shadow is made Windows Vista Aero awareness.
//    -Fixed a bug that causes the shadow to appear abnormally on Windows Vista.
//    -Fixed a bug that causes the shadow to appear abnormally if parent window
//     is initially minimized or maximized
//
// Version 0.2, 2006-11-23
//    -Fix a critical issue that may make the shadow fail to work under certain
//     conditions, e.g., on Win2000, on WinXP or Win2003 without the visual
//     theme enabled, or when the frame window does not have a caption bar.
//
// Version 0.1, 2006-11-10
//    -First release
//****************************************************************************

#pragma once

#pragma warning(push)
#pragma warning(disable:4786)
#include "map"
#pragma warning(pop) 

#include <afxwin.h>

class CWndShadow
{
public:
	CWndShadow(void);
public:
	virtual ~CWndShadow(void);

	HWND m_hWnd = NULL;

protected:

	// Instance handle, used to register window class and create window 
	static HINSTANCE s_hInstance;

#pragma warning(push)
#pragma warning(disable:4786)
	// Parent HWND and CWndShadow object pares, in order to find CWndShadow in ParentProc()
	static std::map<HWND, CWndShadow *> s_Shadowmap;
#pragma warning(pop) 

	// Layered window APIs
	typedef BOOL (WINAPI *pfnUpdateLayeredWindow)(HWND hWnd, HDC hdcDst, POINT *pptDst,
		SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey,
		BLENDFUNCTION *pblend, DWORD dwFlags);
	static pfnUpdateLayeredWindow s_UpdateLayeredWindow;

	// Vista compatibility APIs
	static bool s_bVista;	// Whether running on Win Vista
	typedef HRESULT (WINAPI *pfnDwmIsCompositionEnabled)(BOOL *pfEnabled);
	static pfnDwmIsCompositionEnabled s_DwmIsCompositionEnabled;

	LONG m_OriParentProc;	// Original WndProc of parent window

	enum ShadowStatus
	{
		SS_ENABLED = 1,	// Shadow is enabled, if not, the following one is always false
		SS_VISABLE = 1 << 1,	// Shadow window is visible
		SS_PARENTVISIBLE = 1<< 2,	// Parent window is visible, if not, the above one is always false
		SS_DISABLEDBYAERO = 1 << 3	// Shadow is enabled, but do not show because areo is enabled
	};
	BYTE m_Status;

	unsigned char m_nDarkness;	// Darkness, transparency of blurred area
	unsigned char m_nSharpness;	// Sharpness, width of blurred border of shadow window
	signed char m_nSize;	// Shadow window size, relative to parent window size

	// The X and Y offsets of shadow window,
	// relative to the parent window, at center of both windows (not top-left corner), signed
	signed char m_nxOffset;
	signed char m_nyOffset;

	// Restore last parent window size, used to determine the update strategy when parent window is resized
	LPARAM m_WndSize;

	// Set this to true if the shadow should not be update until next WM_PAINT is received
	bool m_bUpdate;

	COLORREF m_Color;	// Color of shadow

public:
	static bool Initialize(HINSTANCE hInstance);
	void Create(HWND hParentWnd);
	void DestroyWindow(); // modified by James - duy.trinh@fix8.com

	bool SetSize(int NewSize = 0);
	bool SetSharpness(unsigned int NewSharpness = 5);
	bool SetDarkness(unsigned int NewDarkness = 200);
	bool SetPosition(int NewXOffset = 5, int NewYOffset = 5);
	bool SetColor(COLORREF NewColor = 0);

protected:
	//static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Redraw, resize and move the shadow
	// called when window resized or shadow properties changed, but not only moved without resizing
	void Update(HWND hParent);

	// Fill in the shadow window alpha blend bitmap with shadow image pixels
	void MakeShadow(UINT32 *pShadBits, HWND hParent, RECT *rcParent);

	// Helper to calculate the alpha-premultiled value for a pixel
	inline DWORD PreMultiply(COLORREF cl, unsigned char nAlpha)
	{
		// It's strange that the byte order of RGB in 32b BMP is reverse to in COLORREF
		return (GetRValue(cl) * (DWORD)nAlpha / 255) << 16 |
			(GetGValue(cl) * (DWORD)nAlpha / 255) << 8 |
			(GetBValue(cl) * (DWORD)nAlpha / 255);
	}

	// Show or hide the shadow, depending on the enabled status stored in m_Status
	void Show(HWND hParent); 
	void MoveCenter(HWND hWnd);
};
