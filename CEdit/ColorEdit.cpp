// This file was created on March 21st 2001 by Robert Brault.
// I created this Class to be able change the Color of your Edit Box
// as well as your Edit Box Text. This is Derived from CEdit so you
// do not have all the overhead of a CRichEditCtrl.
//
// There are three functions available Currently:
// SetBkColor(COLORREF crColor)
// SetTextColor(COLORREF crColor)
// SetReadOnly(BOOL flag = TRUE)
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
	: m_rectNCBottom(0, 0, 0, 0)
	, m_rectNCTop(0, 0, 0, 0)
{
	m_crBack	= RGB( 255, 255, 255 );
	m_crText	= ::GetSysColor(COLOR_WINDOWTEXT);
	m_brBkgnd.CreateSolidBrush(m_crBack);

	m_bAutoResizeFont = false;
	m_auto_resize_ratio = 0.5;

	memset(&m_lf, 0, sizeof(LOGFONT));
}

CColorEdit::~CColorEdit()
{
}


BEGIN_MESSAGE_MAP(CColorEdit, CEdit)
	//{{AFX_MSG_MAP(CColorEdit)
	//ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SIZE()
	//ON_WM_WINDOWPOSCHANGED()
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
	//ON_WM_NCCALCSIZE()
	//ON_WM_NCPAINT()
	//ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorEdit message handlers

void CColorEdit::PreSubclassWindow()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	CEdit::PreSubclassWindow();

	//�ڱ� �ڽſ��� �ο��� ��Ʈ�� ���ٸ� null�� ���ϵȴ�.
	//dlg�� parent�� font�� ���;� �Ѵ�.
	CFont* font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf),&m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT),sizeof(m_lf),&m_lf);

	m_nDefaultHeight = m_lf.lfHeight;
	ReconstructFont();
}

void CColorEdit::ReconstructFont()
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

CColorEdit& CColorEdit::SetTextColor(COLORREF crColor)
{
	m_crText = crColor; // Passing the value passed by the dialog to the member varaible for Text Color
	RedrawWindow();

	return *this;
}

CColorEdit& CColorEdit::SetBackColor(COLORREF crColor)
{
	m_crBack = crColor; // Passing the value passed by the dialog to the member varaible for Backgound Color
	m_brBkgnd.DeleteObject(); // Deleting any Previous Brush Colors if any existed.
	m_brBkgnd.CreateSolidBrush(crColor); // Creating the Brush Color For the Edit Box Background
	RedrawWindow();

	return *this;
}

CColorEdit& CColorEdit::SetBackColor_Disabled(COLORREF crBackDisabled)
{
	m_crBackDisabled = crBackDisabled;
	RedrawWindow();

	return *this;
}

HBRUSH CColorEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
	if(m_rectNCTop.IsRectEmpty())
	{
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
	}

	HBRUSH hbr;
	hbr = (HBRUSH)m_brBkgnd; // Passing a Handle to the Brush

	pDC->SetBkMode(TRANSPARENT);

	if (!IsWindowEnabled())
	{
		pDC->SetBkColor(m_crBackDisabled);// ::GetSysColor(COLOR_3DFACE));
		m_brBkgnd.DeleteObject();
		m_brBkgnd.CreateSolidBrush(m_crBackDisabled);// ::GetSysColor(COLOR_3DFACE) );
		hbr = (HBRUSH)m_brBkgnd;
	}
	else if (GetStyle() & ES_READONLY)
	{
		pDC->SetBkColor(m_crBack);// ::GetSysColor(COLOR_3DFACE));
	}
	else
	{
		pDC->SetBkColor(m_crBack);
 		m_brBkgnd.DeleteObject();
 		m_brBkgnd.CreateSolidBrush(m_crBack);
		hbr = (HBRUSH)m_brBkgnd;
	}

	pDC->SetTextColor(m_crText); // Setting the Text Color to the one Passed by the Dialog


	return hbr;
}

BOOL CColorEdit::SetReadOnly( BOOL bReadOnly )
{
	//readonly�� �� SetBackColor�� ȣ���ϸ� ���� m_crBackColor�� ��������Ƿ�
	//�� �������� �������� ���� ���� ������ �������ش�.
	if ( bReadOnly )
	{
		m_brBkgnd.DeleteObject();
		m_brBkgnd.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));
	}
	else
	{
		SetBackColor(m_crBack);
	}

	return CEdit::SetReadOnly(bReadOnly);
}

CColorEdit& CColorEdit::SetFontName(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	ReconstructFont();

	return *this;
}

CColorEdit& CColorEdit::SetFontSize( int nSize )
{
	m_font_size = nSize;
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	ReconstructFont();

	return *this;
}

CColorEdit& CColorEdit::SetFontBold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	ReconstructFont();

	return *this;
}

CColorEdit& CColorEdit::SetAutoFontSize(bool bAuto, double ratio)
{
	CRect	r;
	GetClientRect( r );

	m_bAutoResizeFont = bAuto;

	
	if (bAuto)
	{
		m_auto_resize_ratio = ratio;
		m_lf.lfHeight = -((double)r.Height() * m_auto_resize_ratio);
	}
	else
	{
		m_lf.lfHeight = m_nDefaultHeight;
	}

	//m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);

	ReconstructFont();

	return *this;
}

void CColorEdit::RecalcFontSize()
{
	CRect	r;
	GetClientRect( r );

	if ( m_bAutoResizeFont )
		m_lf.lfHeight = -((double)r.Height() * m_auto_resize_ratio);
	else
		m_lf.lfHeight = m_nDefaultHeight;

	ReconstructFont();
}

void CColorEdit::OnSize(UINT nType, int cx, int cy)
{
	CEdit::OnSize(nType, cx, cy);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.

	//MainDlg������ WM_SIZE�� �ִ�ȭ �Ǿ������� ȣ����� �ʴ´�.
	//�׷��� OnWindowPosChanged���� �Ʒ� �ڵ带 �����Ѵ�.
	//������ MainDlg���� â�� ũ�Ⱑ ���ϰų� �ִ�ȭ�� ��
	//MainDlg�� ũ�� ��ȭ�� ���� ���ϵ� ��Ʈ���� ũ�⵵ ����Ǵ� �������(Resize Dialog)
	//���ϵ� ��Ʈ�ѿ����� WM_SIZE�� �߻��Ѵ�.
	//���� ���ϵ� ��Ʈ�ѿ����� ���� OnWindowPosChanged�� ������� �ʰ�
	//WM_SIZE�� ����ص� �ȴ�.
	if ( m_bAutoResizeFont )
		RecalcFontSize();
}

/*
void CColorEdit::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CEdit::OnWindowPosChanged(lpwndpos);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	if ( m_hWnd == NULL )
		return;

	if ( !m_bAutoResizeFont )
		return;

	TRACE( "%d\n", lpwndpos->flags );

	//â�� �̵��� �ƴ� â�� ũ�Ⱑ ������ ���� ��������.
	if ( lpwndpos->flags != 16779797 )
		RecalcFontSize();
}
*/

BOOL CColorEdit::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	return CEdit::PreTranslateMessage(pMsg);
}


void CColorEdit::OnPaint()
{
	//for border???
#if 1
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CEdit::OnPaint() for painting messages
	CRect	rc;

	GetClientRect( rc );

	//CPen	Pen( PS_SOLID, 1, RGB(128, 128, 128) );
	//CPen*	pOldPen = (CPen*)dc.SelectObject( &Pen );
	////int		nOldDrawMode = dc.SetROP2( nDrawMode );
	//CBrush*	pOldBrush = (CBrush*)dc.SelectStockObject( NULL_BRUSH );

	//rc.bottom -= 3;
	//dc.Rectangle(rc);

	dc.FillSolidRect(rc, IsWindowEnabled() ? m_crBack : m_crBackDisabled);

#endif
	//CEdit::OnPaint();
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
	pDC->DrawText(_T("gf�ѱ�������"), rectText, DT_CALCRECT | DT_LEFT);
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
	m_rectNCTop = rectWnd;

	m_rectNCTop.DeflateRect(uiCX, uiCY, uiCX, uiCenterOffset + uiVClientHeight + uiCY);

	m_rectNCBottom = rectWnd;

	m_rectNCBottom.DeflateRect(uiCX, uiCenterOffset + uiVClientHeight + uiCY, uiCX, uiCY);

	lpncsp->rgrc[0].top += uiCenterOffset;
	lpncsp->rgrc[0].bottom -= uiCenterOffset;

	lpncsp->rgrc[0].left += uiCX;
	lpncsp->rgrc[0].right -= uiCY;



	//CEdit::OnNcCalcSize(bCalcValidRects, lpncsp);
}


void CColorEdit::OnNcPaint()
{
	Default();

	CWindowDC dc(this);
	CBrush Brush(m_crBack);//GetSysColor(COLOR_WINDOW));

	dc.FillRect(m_rectNCBottom, &Brush);
	dc.FillRect(m_rectNCTop, &Brush);
}


HBRUSH CColorEdit::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CEdit::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  Change any attributes of the DC here

	// TODO:  Return a different brush if the default is not desired
	return hbr;
}
