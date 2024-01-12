// This file was created on March 21st 2001 by Robert Brault.
// I created this Class to be able change the Color of your Edit Box
// as well as your Edit Box Text. This is Derived from CEdit so you
// do not have all the overhead of a CRichEditCtrl.
//
// There are three functions available Currently:
// SetBkColor(COLORREF crColor)
// set_text_color(COLORREF crColor)
// set_read_only(BOOL flag = TRUE)
//
// How To Use:
// Add three files to your project
// ColorEdit.cpp, ColorEdit.h and Color.h
// Color.h has (#define)'s for different colors (add any color you desire).
//
// Add #include "ColorEdit.h" to your Dialogs Header file.
// Declare an instance of CColorEdit for each edit box being modified.
// Ex. CColorEdit m_ebName;
//
// In your OnInitDialog() add a SubclassDlgItem for each CColorEdit member variable.
// Ex. m_ebName.SubclassDlgItem(IDC_EB_NAME, this);
// In this same function initialize your color for each box unless you want the default.


// ColorEdit.cpp : implementation file
//

//#include "stdafx.h"
#include "ColorEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorEdit

CColorEdit::CColorEdit()
	: m_rect_NCbottom(0, 0, 0, 0)
	, m_rect_NCtop(0, 0, 0, 0)
{
	m_cr_text = ::GetSysColor(COLOR_WINDOWTEXT);
	m_cr_back = ::GetSysColor(COLOR_WINDOW);
	m_cr_text_disabled = ::GetSysColor(COLOR_GRAYTEXT);
	m_cr_back_disabled = ::GetSysColor(COLOR_3DSHADOW);

	m_br_back.CreateSolidBrush(m_cr_back);
	//m_br_back.CreateSolidBrush(HOLLOW_BRUSH);
	m_br_back_disabled.CreateSolidBrush(m_cr_back_disabled);

	m_auto_resize_font = false;
	m_auto_resize_ratio = 0.5;

	memset(&m_lf, 0, sizeof(LOGFONT));
}

CColorEdit::~CColorEdit()
{
}


BEGIN_MESSAGE_MAP(CColorEdit, CEdit)
	//{{AFX_MSG_MAP(CColorEdit)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_CONTROL_REFLECT(EN_KILLFOCUS, &CColorEdit::OnEnKillfocus)
	ON_CONTROL_REFLECT(EN_UPDATE, &CColorEdit::OnEnUpdate)
	ON_WM_WINDOWPOSCHANGED()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorEdit message handlers

void CColorEdit::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	CEdit::PreSubclassWindow();

	//자기 자신에게 부여된 폰트가 없다면 null이 리턴된다.
	//dlg의 parent의 font를 얻어와야 한다.
	CFont* font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	m_default_height = m_lf.lfHeight;
	reconstruct_font();
}

void CColorEdit::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont( &m_font, true );

	ASSERT(bCreated);
}

int CColorEdit::get_font_size(bool pixel_size)
{
	if (pixel_size)
	{
		if (m_lf.lfHeight < 0)
			return -m_lf.lfHeight;
		else
			return m_lf.lfHeight;
	}

	m_font_size = -MulDiv(m_lf.lfHeight, 72, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY));
	return m_font_size;
}

CColorEdit& CColorEdit::set_text_color(COLORREF crColor)
{
	m_cr_text = crColor; // Passing the value passed by the dialog to the member varaible for Text Color
	RedrawWindow();

	return *this;
}

CColorEdit& CColorEdit::set_back_color(COLORREF crColor)
{
	m_cr_back = crColor; // Passing the value passed by the dialog to the member varaible for Backgound Color
	m_br_back.DeleteObject(); // Deleting any Previous Brush Colors if any existed.
	m_br_back.CreateSolidBrush(crColor); // Creating the Brush Color For the Edit Box Background
	//m_br_back.CreateSolidBrush(HOLLOW_BRUSH);
	RedrawWindow();

	return *this;
}

CColorEdit& CColorEdit::set_text_color_disabled(COLORREF cr_text_disabled)
{
	m_cr_text_disabled = cr_text_disabled;
	RedrawWindow();

	return *this;
}

CColorEdit& CColorEdit::set_back_color_disabled(COLORREF cr_back_disabled)
{
	m_cr_back_disabled = cr_back_disabled;
	m_br_back_disabled.DeleteObject();
	m_br_back_disabled.CreateSolidBrush(m_cr_back_disabled);
	RedrawWindow();

	return *this;
}

HBRUSH CColorEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
	if(m_rect_NCtop.IsRectEmpty())
	{
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
	}

	HBRUSH hbr = (HBRUSH)m_br_back; // Passing a Handle to the Brush

	//pDC->SetBkMode(TRANSPARENT);

	if (GetStyle() & ES_READONLY)
	{
		pDC->SetTextColor(m_cr_text);
		pDC->SetBkColor( ::GetSysColor( COLOR_3DFACE ) );
	}
	else if (!IsWindowEnabled() || nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetTextColor(m_cr_text_disabled);
		pDC->SetBkColor(m_cr_back_disabled);
		m_br_back_disabled.DeleteObject();
		m_br_back_disabled.CreateSolidBrush(m_cr_back_disabled);
		hbr = (HBRUSH)m_br_back_disabled;
	}
	else
	{
		pDC->SetTextColor(m_cr_text);
		pDC->SetBkColor(m_cr_back);
 		m_br_back.DeleteObject();
 		m_br_back.CreateSolidBrush(m_cr_back);
		//m_br_back.CreateSolidBrush(HOLLOW_BRUSH);
		hbr = (HBRUSH)m_br_back;
	}

	return hbr;
}

BOOL CColorEdit::set_read_only( BOOL bReadOnly )
{
	//readonly일 때 set_back_color를 호출하면 원래 m_cr_backColor이 덮어써지므로
	//그 변수값은 변경하지 말고 직접 배경색을 변경해준다.
	if (bReadOnly)
	{
		m_br_back.DeleteObject();
		m_br_back.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));
		//m_br_back.CreateSolidBrush(HOLLOW_BRUSH);
	}
	else
	{
		set_back_color(m_cr_back);
	}

	return CEdit::SetReadOnly(bReadOnly);
}

CColorEdit& CColorEdit::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();

	return *this;
}

CColorEdit& CColorEdit::set_font_size( int nSize )
{
	m_font_size = nSize;
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();

	return *this;
}

CColorEdit& CColorEdit::set_font_bold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	reconstruct_font();

	return *this;
}

CColorEdit& CColorEdit::set_auto_font_size(bool bAuto, double ratio)
{
	CRect	r;
	GetClientRect( r );

	m_auto_resize_font = bAuto;

	
	if (bAuto)
	{
		m_auto_resize_ratio = ratio;
		m_lf.lfHeight = -((double)r.Height() * m_auto_resize_ratio);
	}
	else
	{
		m_lf.lfHeight = m_default_height;
	}

	//m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);

	reconstruct_font();

	return *this;
}

void CColorEdit::recalc_font_size()
{
	CRect	r;
	GetClientRect( r );

	if (m_auto_resize_font)
		m_lf.lfHeight = -((double)r.Height() * m_auto_resize_ratio);
	else
		m_lf.lfHeight = m_default_height;

	reconstruct_font();
}

void CColorEdit::OnSize(UINT nType, int cx, int cy)
{
	CEdit::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.

	//MainDlg에서는 WM_SIZE가 최대화 되었을때는 호출되지 않는다.
	//그래서 OnWindowPosChanged에서 아래 코드를 수행한다.
	//하지만 MainDlg에서 창의 크기가 변하거나 최대화될 때
	//MainDlg의 크기 변화에 따라 차일드 컨트롤의 크기도 변경되는 구조라면(Resize Dialog)
	//차일드 컨트롤에서도 WM_SIZE가 발생한다.
	//따라서 차일드 컨트롤에서는 굳이 OnWindowPosChanged를 사용하지 않고
	//WM_SIZE를 사용해도 된다.
	if (m_auto_resize_font)
		recalc_font_size();
}

/*
void CColorEdit::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CEdit::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if ( m_hWnd == NULL )
		return;

	if ( !m_bAutoResizeFont )
		return;

	TRACE( "%d\n", lpwndpos->flags );

	//창의 이동이 아닌 창의 크기가 변했을 때만 수행하자.
	if ( lpwndpos->flags != 16779797 )
		recalc_font_size();
}
*/

BOOL CColorEdit::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CEdit::PreTranslateMessage(pMsg);
}


void CColorEdit::OnPaint()
{
	//for border???
#if 0
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CEdit::OnPaint() for painting messages
	CRect	rc;

	GetClientRect( rc );

	CPen	Pen( PS_SOLID, 1, RGB(128, 128, 128) );
	CPen*	pOldPen = (CPen*)dc.SelectObject( &Pen );
	//int		nOldDrawMode = dc.SetROP2( nDrawMode );
	CBrush*	pOldBrush = (CBrush*)dc.SelectStockObject( NULL_BRUSH );

	rc.bottom -= 3;
	dc.Rectangle( rc );
#endif
	CEdit::OnPaint();
}


void CColorEdit::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// TODO: Add your message handler code here and/or call default
	CRect rectWnd, rectClient;

	//calculate client area height needed for a font
	CFont *pFont = GetFont();
	CRect rectText;
	rectText.SetRectEmpty();

	CDC *pDC = GetDC();

	CFont *pOld = pDC->SelectObject(pFont);
	pDC->DrawText(_T("gf한글日本語"), rectText, DT_CALCRECT | DT_LEFT);
	UINT uiVClientHeight = rectText.Height();

	pDC->SelectObject(pOld);
	ReleaseDC(pDC);

	//calculate NC area to center text.

	GetClientRect(rectClient);
	GetWindowRect(rectWnd);

	ClientToScreen(rectClient);

	UINT uiCenterOffset = (rectClient.Height() - uiVClientHeight) / 2;
	UINT uiCY = (double)(rectWnd.Height() - rectClient.Height()) / 2;
	UINT uiCX = (rectWnd.Width() - rectClient.Width()) / 2;

	rectWnd.OffsetRect(-rectWnd.left, -rectWnd.top);
	m_rect_NCtop = rectWnd;

	m_rect_NCtop.DeflateRect(uiCX, uiCY, uiCX, uiCenterOffset + uiVClientHeight + uiCY);

	m_rect_NCbottom = rectWnd;

	m_rect_NCbottom.DeflateRect(uiCX, uiCenterOffset + uiVClientHeight + uiCY, uiCX, uiCY);

	lpncsp->rgrc[0].top += uiCenterOffset;
	lpncsp->rgrc[0].bottom -= uiCenterOffset;

	lpncsp->rgrc[0].left += uiCX;
	lpncsp->rgrc[0].right -= uiCY;
	//CEdit::OnNcCalcSize(bCalcValidRects, lpncsp);
}


void CColorEdit::OnNcPaint()
{
	if (!IsWindowEnabled())
		return;

	Default();

	CWindowDC dc(this);
	CBrush Brush(m_cr_back);//GetSysColor(COLOR_WINDOW));

	dc.FillRect(m_rect_NCbottom, &Brush);
	dc.FillRect(m_rect_NCtop, &Brush);

	Brush.DeleteObject();
}

void CColorEdit::update_ctrl()
{
	CWnd* pParent = GetParent();
	CRect rect;

	GetWindowRect(rect);
	pParent->ScreenToClient(rect);
	rect.top -= 5;
	//rect.InflateRect( 5, 5 );		//이거 안해주면 위치이동시 잔상생김

	pParent->InvalidateRect(rect, FALSE);
}


void CColorEdit::OnEnKillfocus()
{
	update_ctrl();
}


void CColorEdit::OnEnUpdate()
{
	update_ctrl();
}


void CColorEdit::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CEdit::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	update_ctrl();
}

//ON_WM_CTLCOLOR_REFLECT() 때문인지 OnCtlColor()는 호출되지 않는다.
HBRUSH CColorEdit::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	return CEdit::OnCtlColor(pDC, pWnd, nCtlColor);
}
