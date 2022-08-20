// StaticFader.cpp : implementation file
//

#include "stdafx.h"
#include "StaticFader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TIMERINTERVAL 10

/////////////////////////////////////////////////////////////////////////////
// CStaticFader

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Function Header
CStaticFader::CStaticFader()
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
	// Set defaults:

	m_uAlignFlag = DT_CENTER;
	m_bDrawBorder = true;
	m_bDrawShadow=false;
	m_crBackground = RGB(210,210,210);
	m_crSubText = RGB(0,0,60);
	m_crMainText = RGB(0,0,60);

	m_strSubText = m_strMainText = "";

	//m_rMargin = 0;	//이 문장은 절대 멤버변수를 0으로 초기화 하지 않는다.
	m_rMargin.SetRectEmpty();


	m_pSubFont = new CAutoFont("Arial");
	m_pMainFont = new CAutoFont("Arial");
	
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Function Header
CStaticFader::~CStaticFader()
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
	delete m_pSubFont;
	delete m_pMainFont;
}


BEGIN_MESSAGE_MAP(CStaticFader, CStatic)
	//{{AFX_MSG_MAP(CStaticFader)
	ON_WM_TIMER()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStaticFader message handlers

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Function Header
void CStaticFader::OnTimer(UINT nIDEvent) 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
	if ( (GetTickCount()-m_dwStopwatch) > 1000)	KillTimer(500);

	Invalidate(FALSE);
	
	CStatic::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Function Header
void CStaticFader::Display(CString strText, CString strSubText, int nFadePercent, bool bResetColours, CString strSubFont, CString strMainFont)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
	if (!strMainFont.IsEmpty())		m_pMainFont->ExtractFont(strMainFont);
	if (!strSubFont.IsEmpty())		m_pSubFont->ExtractFont(strSubFont);

	if (bResetColours)
	{
		m_crBackground = RGB(210,210,210);
		m_crSubText = RGB(0,0,60);
		m_crMainText = RGB(0,0,60);
	}

	m_strMainText = strText;
	m_strSubText = strSubText;
	m_nFadePercent = nFadePercent;
	m_dwStopwatch = GetTickCount();
	SetTimer(500, TIMERINTERVAL, NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Function Header
void CStaticFader::OnPaint() 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
	CPaintDC dc(this); // device context for painting
	CMemoryDC memDC(&dc, &m_rectClient);
	CMemoryDC* pDC = &memDC;
	GetClientRect(&m_rectClient);

	pDC->SetBkMode(TRANSPARENT);

	if ( m_strSubText.IsEmpty() )
	{
		pDC->FillSolidRect( m_rectClient.left, m_rectClient.top, m_rectClient.right, m_rectClient.bottom, m_crBackground );
		
		if (m_bDrawBorder)	
			pDC->Draw3dRect( m_rectClient.left, m_rectClient.top, m_rectClient.right, m_rectClient.bottom, ::GetSysColor(COLOR_BTNSHADOW), ::GetSysColor(COLOR_BTNHILIGHT) );
	}
	else
	{
		pDC->FillSolidRect( m_rectClient.left, m_rectClient.top, m_rectClient.left+49, m_rectClient.bottom, m_crBackground );

		if (m_bDrawBorder)	
			pDC->Draw3dRect( m_rectClient.left, m_rectClient.top, m_rectClient.left+49, m_rectClient.bottom, ::GetSysColor(COLOR_BTNSHADOW), ::GetSysColor(COLOR_BTNHILIGHT) );

		pDC->FillSolidRect( m_rectClient.left+50, m_rectClient.top, m_rectClient.right-50, m_rectClient.bottom, m_crBackground);
		
		if (m_bDrawBorder)	
			pDC->Draw3dRect( m_rectClient.left+50, m_rectClient.top, m_rectClient.right-50, m_rectClient.bottom, ::GetSysColor(COLOR_BTNSHADOW), ::GetSysColor(COLOR_BTNHILIGHT) );
	}

    DrawText(pDC);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Function Header
void CStaticFader::DrawText(CMemoryDC *pDC)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
	CFont *pOldFont = NULL;
	
	if ( ! m_strSubText.IsEmpty())
	{
	    pOldFont=pDC->SelectObject(m_pSubFont);
		CRect rect = m_rectClient;
		rect.right=50;
		pDC->SetTextColor( GetCurrentColour( m_crSubText ) );
		int x = pDC->DrawText((LPCTSTR)m_strSubText,m_strSubText.GetLength(),&rect, DT_CENTER| DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER );
		pDC->SelectObject(pOldFont);
	}

	if ( ! m_strMainText.IsEmpty())
	{
	    pOldFont=pDC->SelectObject(m_pMainFont);
		CRect rect = m_rectClient;
		rect.right-=1;
		
		if ( ! m_strSubText.IsEmpty())	rect.left+=50;

		// If required draw text with darker colour to appear as a shadow
		rect -= m_rMargin;

		if (m_bDrawShadow)
		{
			CRect shadowrect = rect;
			shadowrect.top+=2;
			shadowrect.left+=2;

			int r = GetRValue(m_crBackground);	
			int g = GetGValue(m_crBackground);	
			int b = GetBValue(m_crBackground);

			if ( (r-=50)<0 ) r = 0;	
			if ( (g-=50)<0 ) g = 0;		
			if ( (b-=50)<0 ) b = 0;
			
			pDC->SetTextColor( GetCurrentColour( RGB(r,g,b) ) );
			pDC->DrawText((LPCTSTR)m_strMainText, m_strMainText.GetLength(),&shadowrect, m_uAlignFlag | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER );
		}

		pDC->SetTextColor( GetCurrentColour( m_crMainText ) );
		pDC->DrawText((LPCTSTR)m_strMainText, m_strMainText.GetLength(),&rect, m_uAlignFlag | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER );
		pDC->SelectObject(pOldFont);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Function Header
COLORREF CStaticFader::GetCurrentColour(COLORREF crText)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
	UINT uCurrentPercent = (UINT) (GetTickCount() - m_dwStopwatch)/10;
	
	if ( uCurrentPercent >= (UINT)m_nFadePercent) return crText;

	short Back_r = GetRValue(m_crBackground);
	short Back_g = GetGValue(m_crBackground);
	short Back_b = GetBValue(m_crBackground);

	short Target_r = GetRValue(crText);
	short Target_g = GetGValue(crText);
	short Target_b = GetBValue(crText);

	short Diff_r = Back_r - Target_r;
	short Diff_g = Back_g - Target_g;
	short Diff_b = Back_b - Target_b;

	short Current_r = Back_r - (short) (((float)Diff_r/(float)m_nFadePercent)*uCurrentPercent);
	short Current_g = Back_g - (short) (((float)Diff_g/(float)m_nFadePercent)*uCurrentPercent);
	short Current_b = Back_b - (short) (((float)Diff_b/(float)m_nFadePercent)*uCurrentPercent);

	return RGB( Current_r, Current_g, Current_b );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Function Header
void CStaticFader::Initialise(COLORREF crBG, COLORREF crMainText, COLORREF crSubText)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
	m_crBackground = crBG;
	m_crMainText = crMainText;
	m_crSubText = crSubText;
}

void CStaticFader::SetFaceName( CString sName )
{
	m_pMainFont->SetFaceName( sName );
}

void CStaticFader::SetFontHeight( int nHeight )
{
	m_pMainFont->SetHeight( nHeight );
}

void CStaticFader::SetFontWidth( int nWidth )
{
	m_pMainFont->SetWidth( nWidth );
}

void CStaticFader::SetFontBold( bool bBold )
{
	m_pMainFont->SetBold( bBold );
}

void CStaticFader::SetMargin( int l, int t, int r, int b )
{
	m_rMargin = CRect( l, t, r, b );
}
