/*|*\
|*|  File:      DimEditCtrl.cpp
|*|  
|*|  By:        James R. Twine, TransactionWorks, Inc.
|*|             Copyright 2000, TransactionWorks, inc.
|*|  Date:      Thursday, September 21, 2000
|*|             
|*|  Notes:     This Is The Implementation Of A "Dim Edit Control".
|*|             It Provides Visual Instructions Within The Edit
|*|             Control Itself.  It Can Be Used To Indicate Special
|*|             Properties Of A Edit Control Used On A Crowded
|*|             Interface
|*|             
|*|             May Be Freely Incorporated Into Projects Of Any Type
|*|             Subject To The Following Conditions:
|*|             
|*|             o This Header Must Remain In This File, And Any
|*|               Files Derived From It
|*|             o Do Not Misrepresent The Origin Of This Code
|*|               (IOW, Do Not Claim You Wrote It)
|*|             
|*|             A "Mention In The Credits", Or Similar Acknowledgement,
|*|             Is *NOT* Required.  It Would Be Nice, Though! :)
\*|*/
//#include "stdafx.h"
#include "DimEditCtrl.h"
#include "../Functions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDimEditCtrl

CDimEditCtrl::CDimEditCtrl() :
	m_bShowDimText( true ),									// Set The Dim Flag
//	m_cRedOS( -0x40 ), 										// Set The Default Dim Offset Colors
//	m_cGreenOS( -0x40 ),									// Set The Default Dim Offset Colors
//	m_cBlueOS( -0x40 ),										// Set The Default Dim Offset Colors
//	m_bUseDimOffset( true ),								// Use The Offset Colors
	m_iDimTextLen( 0 ),										// No Dim Text Set Yet
	m_crDimTextColor( RGB( 0x00, 0x00, 0x00 ) )				// No "Hard" Dim Text Color
{
	m_crText = RGB( 0, 0, 0 );
	m_caDimText[ 0 ] = _T( '\0' );							// Terminate The Buffer
	SetDimOffset( -0x40, -0x40, -0x40 );					// Set The Dim Offset

	m_dwStyle = 0;

	memset(&m_lf, 0, sizeof(LOGFONT));

	return;													// Done!
}


CDimEditCtrl::~CDimEditCtrl()
{
	return;													// Done!
}


BEGIN_MESSAGE_MAP(CDimEditCtrl, CEdit)
	//{{AFX_MSG_MAP(CDimEditCtrl)
	ON_CONTROL_REFLECT_EX(EN_CHANGE, OnChange)
	//ON_CONTROL_REFLECT(EN_SETFOCUS, OnSetfocus)
	ON_CONTROL_REFLECT_EX(EN_SETFOCUS, OnSetfocus)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SETTINGCHANGE()
	//}}AFX_MSG_MAP
//	ON_WM_LBUTTONDOWN()
//	ON_WM_LBUTTONDBLCLK()
	ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, &CDimEditCtrl::OnEnKillfocus)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDimEditCtrl message handlers

void CDimEditCtrl::PreSubclassWindow() 
{
	CEdit::PreSubclassWindow();								// Do Default...

	// Get Defalut Font 
	//자기 자신에게 부여된 폰트가 없다면 null이 리턴된다.
	//dlg의 parent의 font를 얻어와야 한다.
	CFont* font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	m_nDefaultHeight = m_lf.lfHeight;
	reconstruct_font();

	SetShowDimControl( true );								// Default To Show The Dim Control
		
	return;													// Done!
}

void CDimEditCtrl::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, true);

	m_font_size = get_font_size();

	ASSERT(bCreated);
}

int CDimEditCtrl::get_font_size()
{
	m_font_size = -MulDiv(m_lf.lfHeight, 72, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY));
	return m_font_size;
}

//-1 : reduce, +1 : enlarge
void CDimEditCtrl::set_font_size(int font_size)
{
	if (font_size == 0)
		return;

	if (font_size == -1)
	{
		enlarge_font_size(false);
		return;
	}
	else if (font_size == 1)
	{
		enlarge_font_size(true);
		return;
	}

	m_font_size = font_size;
	AfxGetApp()->WriteProfileInt(_T("dim edit"), _T("font size"), m_font_size);
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();
}

void CDimEditCtrl::enlarge_font_size(bool enlarge)
{
	m_font_size = get_font_size();
	enlarge ? m_font_size++ : m_font_size--;

	if (m_font_size < 4)
		m_font_size = 4;
	if (m_font_size > 40)
		m_font_size = 40;

	AfxGetApp()->WriteProfileInt(_T("file list"), _T("font size"), m_font_size);
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();
}

void CDimEditCtrl::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	if (sFontname == _T(""))
		return;
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();
}

void CDimEditCtrl::set_font_bold(bool bBold)
{
	m_lf.lfWeight = (bBold ? FW_BOLD : FW_NORMAL);
	reconstruct_font();
}

void CDimEditCtrl::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
}

void CDimEditCtrl::set_log_font(LOGFONT lf)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	reconstruct_font();
	Invalidate();
}

void CDimEditCtrl::SetDimText( LPCTSTR cpDimText )
{
	if( cpDimText )											// If Dim Text Specified
	{
		_tcsncpy_s(m_caDimText, _countof(m_caDimText), cpDimText, DIM_TEXT_LEN);	// Copy Over The Text
		m_caDimText[ DIM_TEXT_LEN ] = _T( '\0' );			// Enforce Termination (I Am Paranoid, I Know!) 
		m_iDimTextLen = _tcslen( m_caDimText );				// Store Length Of The Dim Text
	}
	else													// If No Dim Text
	{
		m_caDimText[ 0 ] = _T( '\0' );						// Just Terminate The Buffer (No Text)
		m_iDimTextLen = 0;									// No Dim Text
	}

	DrawDimText();										// Draw The Dim Text

	return;													// Done!
}

void	CDimEditCtrl::SetShowDimControl( bool bShow )
{
	m_bShowDimText = bShow;									// Set The Dim Flag
	DrawDimText();										// Draw The Dim Text

	return;													// Done!
}

BOOL	CDimEditCtrl::Create( LPCTSTR lpszClassName, LPCTSTR lpszWindowName, 
				DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, 
				CCreateContext* pContext ) 
{
	BOOL	bCreated = CWnd::Create( lpszClassName, 
					lpszWindowName, dwStyle, rect, 
					pParentWnd, nID, pContext );			// Try To Create Ourselves...

	if( bCreated )											// If We Got Created
		SetShowDimControl( true );							// Show The Dim Control

	return( bCreated );										// Return Creation Status
}


BOOL CDimEditCtrl::OnChange() 
{
	int		iLen = GetWindowTextLength();					// Get Control's Text Length

	if( !iLen )												// If No Text
		SetShowDimControl( true );							// Show The Dim Text
	else													// If Text Now In The Control
		SetShowDimControl( false );							// Disable The Dim Text

	return FALSE;													// Done!
}


BOOL CDimEditCtrl::OnSetfocus() 
{
	Invalidate();

	return FALSE;
}


BOOL CDimEditCtrl::OnEnKillfocus()
{
	DrawDimText();										// Draw The Dim Text
	return FALSE;
}

void CDimEditCtrl::OnPaint() 
{
	Default();												// Do Default Control Drawing
	DrawDimText();										// Draw The Dim Text
}


void	CDimEditCtrl::DrawDimText()
{
	if (GetFocus() == this || !IsWindowEnabled())
		return;

	if( !m_bShowDimText || !m_iDimTextLen )				// If No Dim Text
		return;											// Stop Here

	CClientDC	dc( this );
	CRect		rRect;
	int			iState = dc.SaveDC();					// Save The DC State

	//pDC = &dc;
	
	GetClientRect( &rRect );							// Get Drawing Area
	dc.FillSolidRect( rRect, RGB(255,0,0) );
	//rRect.OffsetRect( 1, 1 );							// Add Sanity Space
	
	dc.SelectObject( (*GetFont()) );					// Use The Control's Current Font
	dc.SetTextColor( m_crDimTextColor );				// Set The Text Color
	//pDC->SetBkColor( GetSysColor( COLOR_WINDOW ) );	// Set The Bk Color
	dc.SetBkMode(TRANSPARENT);

	DWORD dwStyle = GetStyle();
	DWORD dwText = 0;

	if (m_dwStyle == 0)
	{
		MAP_STYLE(ES_LEFT, DT_LEFT);
		MAP_STYLE(ES_RIGHT, DT_RIGHT);
		MAP_STYLE(ES_CENTER, DT_CENTER);
		//MAP_STYLE(ES_CENTERIMAGE, DT_VCENTER | DT_SINGLELINE);
		//MAP_STYLE(ES_NOPREFIX, DT_NOPREFIX);
		//MAP_STYLE(ES_WORDELLIPSIS, DT_WORD_ELLIPSIS);
		//MAP_STYLE(ES_ENDELLIPSIS, DT_END_ELLIPSIS);
		//MAP_STYLE(ES_PATHELLIPSIS, DT_PATH_ELLIPSIS);
	}

	dc.DrawText(m_caDimText, m_iDimTextLen, &rRect, dwText | DT_SINGLELINE | DT_VCENTER);
	dc.RestoreDC( iState );								// Restore The DC State

	return;												// Done!
}


BOOL CDimEditCtrl::OnEraseBkgnd(CDC* pDC) 
{
	BOOL	bStatus = CEdit::OnEraseBkgnd(pDC);

	if (bStatus && m_bShowDimText)					// If All Good, And Showing Any Dim Text
	{
		DrawDimText();										// Draw The Dim Text
	}
	return( bStatus );										// Return Erase Status
}

void	CDimEditCtrl::SetDimOffset( char cRedOS, char cGreenOS, char cBlueOS )
{
	COLORREF	crWindow = GetSysColor( COLOR_WINDOW );
	BYTE		btRedOS = ( GetRValue( crWindow ) + cRedOS );
	BYTE		btGreenOS = ( GetGValue( crWindow ) + cGreenOS );
	BYTE		btBlueOS = ( GetBValue( crWindow ) + cBlueOS );

	m_bUseDimOffset = true;									// Set The Flag
	m_cRedOS = cRedOS;										// Store Red Offset
	m_cGreenOS = cGreenOS;									// Store Green Offset
	m_cBlueOS = cBlueOS;									// Store Blue Offset
	m_crDimTextColor = RGB( (BYTE)btRedOS, (BYTE)btGreenOS, 
			(BYTE)btBlueOS );								// Build The New Dim Color

	return;													// Done!
}


void	CDimEditCtrl::SetDimColor( COLORREF crColor )
{
	m_bUseDimOffset = false;								// Unset The Flag
	m_crDimTextColor = crColor;								// Set The New Dim Color
	m_cRedOS = m_cGreenOS = m_cBlueOS = 0;					// No Offset

	return;													// Done!
}


void CDimEditCtrl::OnSettingChange(UINT uFlags, LPCTSTR lpszSection) 
{
	CEdit::OnSettingChange(uFlags, lpszSection);
	
	if( m_bUseDimOffset )									// If Using An Offset For The Dim Color
	{
		COLORREF	crWindow = GetSysColor( COLOR_WINDOW );

		m_crDimTextColor = RGB( GetRValue( crWindow ) + 
								m_cRedOS, GetGValue( crWindow ) + 
								m_cGreenOS, GetBValue( crWindow ) + 
								m_cBlueOS );								// Rebuild The Dim Color
	}
	return;													// Done!
}

void CDimEditCtrl::SetAutoFontSize( bool bAuto )
{
	CRect	r;
	GetClientRect( r );

	if ( bAuto )
		m_lf.lfHeight = (double)r.Height();// * 0.9;
	else
		m_lf.lfHeight = m_nDefaultHeight;

	reconstruct_font();
}

CDimEditCtrl& CDimEditCtrl::SetFontBold( bool bBold /*= true*/ )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	reconstruct_font();

	return *this;
}
