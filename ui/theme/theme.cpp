#include "theme.h"

/************************************************************
**
**	Simple Theme class for CHoverBitmapButton
**
**	You could use the WTL CTheme class instead.
**
**	by Rail Jon Rogut Feb 2003
**
*************************************************************/

CTheme::CTheme()
{
	m_bXPTheme = FALSE;
	m_bLibLoaded = FALSE;

	m_bThemeExists = FALSE;

	m_hModThemes = NULL;
	m_hTheme = NULL;

	m_hWnd = NULL;

	zOpenThemeData = NULL;
	zDrawThemeBackground = NULL;
	zCloseThemeData = NULL;
	zDrawThemeText = NULL;
	zGetThemeBackgroundContentRect = NULL;
	zDrawThemeEdge = NULL;
	zDrawThemeIcon = NULL;
}

CTheme::~CTheme()
{
	ReInit();
}

void CTheme::Init(HWND hWnd)
{
	Init();
	OpenTheme(hWnd);
}

void CTheme::Init()
{
	if (!this->CheckOSVer())
	{
		return;
	}

	m_hModThemes = LoadLibrary(_T("UXTHEME.DLL"));
	m_bXPTheme = this->GetAppearance();

	if(m_hModThemes)
	{
		zOpenThemeData = (PFNOPENTHEMEDATA)GetProcAddress(m_hModThemes, "OpenThemeData");
		zDrawThemeBackground = (PFNDRAWTHEMEBACKGROUND)GetProcAddress(m_hModThemes, "DrawThemeBackground");
		zCloseThemeData = (PFNCLOSETHEMEDATA)GetProcAddress(m_hModThemes, "CloseThemeData");
		zDrawThemeText = (PFNDRAWTHEMETEXT)GetProcAddress(m_hModThemes, "DrawThemeText");
		zGetThemeBackgroundContentRect = (PFNGETTHEMEBACKGROUNDCONTENTRECT)GetProcAddress(m_hModThemes, "GetThemeBackgroundContentRect");
		zDrawThemeEdge = (PFNDRAWTHEMEEDGE)GetProcAddress(m_hModThemes, "DrawThemeEdge");
		zDrawThemeIcon = (PFNDRAWTHEMEICON)GetProcAddress(m_hModThemes, "DrawThemeIcon");

		if(zOpenThemeData && zDrawThemeBackground && zCloseThemeData 
									&& zDrawThemeText && zGetThemeBackgroundContentRect 
									&& zDrawThemeEdge && zDrawThemeIcon)
			{
			m_bLibLoaded = TRUE;			
			}
		else
			{
			::FreeLibrary(m_hModThemes);
			m_hModThemes = NULL;
			}
	}
}

void CTheme::ReInit(void)
{
	// PAR 27/06/04 - Clean up the theme handle first
	if (zCloseThemeData && m_hTheme)
		zCloseThemeData(m_hTheme);
		
	// Now free the library...
	if (m_hModThemes)
		::FreeLibrary(m_hModThemes);

	m_hModThemes = NULL;
	m_hTheme = NULL;
	m_bLibLoaded = FALSE;
	m_bXPTheme = FALSE;
	m_bThemeExists = FALSE;
}

void CTheme::ThemeChanged(HWND hWnd)
{
	if (m_bLibLoaded)
	{
		if (m_hTheme)
			ReInit();
	}

	Init();

	OpenTheme(hWnd);
}

void	CTheme::OpenTheme(HWND hWnd)
{
	if (m_bLibLoaded)
		{
		ASSERT(hWnd);

		m_hWnd = hWnd;

		m_hTheme = zOpenThemeData(m_hWnd, L"Button");

		if (m_hTheme)
			m_bThemeExists = TRUE;
		}
}

void	CTheme::DrawThemeBackground(HDC dc, RECT *pRect, int iPartID, int iStateID, RECT *pClipRect)
{
	CRect r = *pRect;

	r.InflateRect(1, 1);

	if (m_hTheme)
		{
		zDrawThemeBackground(m_hTheme, dc, iPartID, iStateID, &r, pClipRect);
		}
}

void	CTheme::DrawThemeEdge(HDC dc, RECT *pRect, UINT uEdge, UINT uFlags, int iPartID, int iStateID, RECT *pClipRect)
{
	if (m_hTheme)
		{
		zDrawThemeEdge(m_hTheme, dc, iPartID, iStateID, pRect, uEdge, uFlags, pClipRect);
		}
}

void	CTheme::DrawThemeText( HDC hdc, const RECT *pRect, CString strText, DWORD dwTextFlags, 
							   DWORD dwTextFlags2 /*= 0*/, int iPartId /*= BP_PUSHBUTTON*/, int iStateId /*= PBS_HOT*/ )
{
	if (m_hTheme)
	{
		USES_CONVERSION;
		zDrawThemeText(m_hTheme, hdc, iPartId, iStateId, T2COLE((LPCTSTR)strText), -1, dwTextFlags, dwTextFlags2, pRect );
	}
}

BOOL CTheme::CheckOSVer()
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if ( (osvi.dwMajorVersion < 5) || ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion < 1)) )
		return FALSE;
		
	return TRUE;
}


BOOL	CTheme::GetAppearance(void)
// Simplified, PAR 27/06/04
{
//return FALSE;
	// For XP - Detect if the Window Style is Classic or XP

	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);

	if ( (osvi.dwMajorVersion < 5) || ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion < 1)) )
		return FALSE;
		
	// OS is OK, so leave it all up to IsAppThemed()
	return this->IsAppThemed();
}

BOOL CTheme::IsAppThemed()
// Calls IsAppThemed on the loaded UXTHEME.DLL - the only reliable way to check
// is we should be drawing things in a theme way or not.
// Simplified, PAR 27/06/04
{
	if ( m_hModThemes )
	{
		PFNISAPPTHEMED pfnIsAppThemed = (PFNISAPPTHEMED) ::GetProcAddress(m_hModThemes, "IsAppThemed");
		if ( pfnIsAppThemed )
			return (*pfnIsAppThemed)();
	}
	
	return FALSE;
}
