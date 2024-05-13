#pragma once

#include <afxcmn.h>
#include "ThemeLib.h"

/*
* window 기본 버튼을 gdi로 직접 그리지 않고 theme을 이용하여 기본 버튼을 그릴 수 있다.
	//사용하고자 하는 클래스에서 아래 변수 및 함수 선언.
 	CTheme			m_theme;
	bool			m_theme_initialized = false;
	void			theme_init();
	void			draw_checkbox(CDC* pDC, CRect r, int check_state);

	//두 함수의 사용법은 이 파일 맨 아래 예제 참조.
*/

/************************************************************
**
**	Simple Theme class for CHoverBitmapButton
**
**	You could use the WTL CTheme class instead.
**
**	by Rail Jon Rogut Feb 2003
**
*************************************************************/

class CTheme
{
	public:
		CTheme();
		~CTheme();

	public:
		void	Init(void);
		void	Init(HWND hWnd);
		void	ReInit(void);
		void	ThemeChanged(HWND hWnd);
		void	OpenTheme(HWND hWnd);

		// Setup defaults for CHoverBitmapButton:

		void	DrawThemeBackground(HDC dc, RECT *pRect, 
											int iPartID = BP_PUSHBUTTON, 
											int iStateID = PBS_HOT, 
											RECT *pClipRect = NULL); 

		void	DrawThemeEdge(HDC dc, RECT *pRect, 
											UINT uEdge = EDGE_BUMP, 
											UINT uFlags = BF_RECT, 
											int iPartID = BP_PUSHBUTTON, 
											int iStateID = PBS_HOT, 
											RECT *pClipRect = NULL);
											
		void	DrawThemeText( HDC hdc,
							   const RECT *pRect,
							   CString strText, 
							   DWORD dwTextFlags,
							   DWORD dwTextFlags2 = 0,
							   int iPartId = BP_PUSHBUTTON,
							   int iStateId = PBS_HOT );


		BOOL	GetAppearance(void);
		BOOL	CheckOSVer();
		BOOL	IsAppThemed();

	public:
		BOOL	m_bXPTheme;
		BOOL	m_bThemeExists;
		BOOL	m_bLibLoaded;
		HMODULE	m_hModThemes;
		HTHEME	m_hTheme;

		HWND	m_hWnd;

		PFNOPENTHEMEDATA					zOpenThemeData;
		PFNDRAWTHEMEBACKGROUND				zDrawThemeBackground;
		PFNCLOSETHEMEDATA					zCloseThemeData;
		PFNDRAWTHEMEEDGE					zDrawThemeEdge;
		PFNDRAWTHEMEICON					zDrawThemeIcon;
		PFNDRAWTHEMETEXT					zDrawThemeText;
		PFNGETTHEMEBACKGROUNDCONTENTRECT	zGetThemeBackgroundContentRect;
		
		typedef BOOL(__stdcall *PFNISAPPTHEMED)();
};

/*

void CSCTreeCtrl::theme_init()
{
	if (!m_theme_initialized)
	{
		m_theme.Init(m_hWnd);
		m_theme_initialized = true;
	}
}

void CSCTreeCtrl::draw_checkbox(CDC* pDC, CRect r, int check_state)
{
	theme_init();

	if (m_theme.GetAppearance())
	{
		int	nState = 0;

		// Translate from the button state to the appropriate draw state flags
		switch (check_state)
		{
		case BST_CHECKED:
			nState = CBS_CHECKEDNORMAL;
			break;

		case BST_UNCHECKED:
			nState = CBS_UNCHECKEDNORMAL;
			break;

		default:
			nState = CBS_MIXEDNORMAL;
			break;
		}

		// Now do the actual drawing...
		m_theme.DrawThemeBackground(pDC->GetSafeHdc(), r, BP_CHECKBOX, nState);
	}
	else // No themes - just draw it conventionally
	{
		UINT nState = 0;

		// Translate from the button state to the appropriate draw state flags
		switch (check_state)
		{
		case BST_CHECKED:
			nState = DFCS_BUTTONCHECK | DFCS_CHECKED;
			break;

		case BST_UNCHECKED:
			nState = DFCS_BUTTONCHECK;
			break;

		default:
			nState = DFCS_BUTTON3STATE;
			break;
		}

		// Now do the actual drawing...
		pDC->DrawFrameControl(r, DFC_BUTTON, nState);
	}
}

*/