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
// SCEdit.cpp, SCEdit.h and Color.h
// Color.h has (#define)'s for different colors (add any color you desire).
//
// Add #include "SCEdit.h" to your Dialogs Header file.
// Declare an instance of CSCEdit for each edit box being modified.
// Ex. CSCEdit m_ebName;
//
// In your OnInitDialog() add a SubclassDlgItem for each CSCEdit member variable.
// Ex. m_ebName.SubclassDlgItem(IDC_EB_NAME, this);
// In this same function initialize your color for each box unless you want the default.


// SCEdit.cpp : implementation file
//

//#include "stdafx.h"
#include "SCEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSCEdit

CSCEdit::CSCEdit()
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

CSCEdit::~CSCEdit()
{
}


BEGIN_MESSAGE_MAP(CSCEdit, CEdit)
	//{{AFX_MSG_MAP(CSCEdit)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SIZE()
	//ON_WM_PAINT()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_CONTROL_REFLECT(EN_SETFOCUS, &CSCEdit::OnEnSetfocus)
	ON_CONTROL_REFLECT(EN_KILLFOCUS, &CSCEdit::OnEnKillfocus)
	ON_CONTROL_REFLECT(EN_UPDATE, &CSCEdit::OnEnUpdate)
	ON_WM_WINDOWPOSCHANGED()
//}}AFX_MSG_MAP
ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSCEdit message handlers

void CSCEdit::PreSubclassWindow()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	CEdit::PreSubclassWindow();

	//�ڱ� �ڽſ��� �ο��� ��Ʈ�� ���ٸ� null�� ���ϵȴ�.
	//dlg�� parent�� font�� ���;� �Ѵ�.
	CFont* font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	m_default_height = m_lf.lfHeight;
	reconstruct_font();
}

void CSCEdit::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont( &m_font, true );

	ASSERT(bCreated);
}

int CSCEdit::get_font_size(bool pixel_size)
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

//CEdit::SetRect()�� �̿��ؼ� �����¿� ũ�⸦ ������ �� �ִµ�
//ES_MULTILINE �Ӽ��� �־�߸� �����ϹǷ� �Ӽ��� �ݵ�� ��Ƽ���� �Ӽ��� �����ؾ� �Ѵ�.
//ES_MULTILINE �Ӽ��� �����Ŀ��� ������ �� ���� �Ӽ��̴�.
//https://forums.codeguru.com/showthread.php?361420-Want-to-set-quot-ES_MULTILINE-quot-property-of-Edit-object-externally
//�����Ŀ��� SetWindowLong()�� �̿��Ͽ� ������ �� �ִ� �Ӽ���
//(ES_LOWERCASE, ES_NUMBER, ES_OEMCONVERT, ES_UPPERCASE, ES_WANTRETURN)
//CDC::DrawText()�� define�� ����Ѵ�.(DT_TOP, DT_VCENTER, DT_BOTTOM)
CSCEdit& CSCEdit::set_line_align(DWORD align)
{
	CRect rc;
	GetClientRect(rc);

	rc.DeflateRect(2, 2);

	if (align == DT_VCENTER)
		rc.top = rc.top + (rc.Height() - get_font_size(true)) / 2 - 1;
	else if (align == DT_BOTTOM)
		rc.top = rc.bottom - get_font_size(true);

	SetRect(&rc);

	return *this;
}

CSCEdit& CSCEdit::set_text_color(COLORREF crColor)
{
	m_cr_text = crColor; // Passing the value passed by the dialog to the member varaible for Text Color
	RedrawWindow();

	return *this;
}

CSCEdit& CSCEdit::set_back_color(COLORREF crColor)
{
	m_cr_back = crColor; // Passing the value passed by the dialog to the member varaible for Backgound Color
	m_br_back.DeleteObject(); // Deleting any Previous Brush Colors if any existed.
	m_br_back.CreateSolidBrush(crColor); // Creating the Brush Color For the Edit Box Background
	//m_br_back.CreateSolidBrush(HOLLOW_BRUSH);
	RedrawWindow();

	return *this;
}

CSCEdit& CSCEdit::set_text_color_disabled(COLORREF cr_text_disabled)
{
	m_cr_text_disabled = cr_text_disabled;
	RedrawWindow();

	return *this;
}

CSCEdit& CSCEdit::set_back_color_disabled(COLORREF cr_back_disabled)
{
	m_cr_back_disabled = cr_back_disabled;
	m_br_back_disabled.DeleteObject();
	m_br_back_disabled.CreateSolidBrush(m_cr_back_disabled);
	RedrawWindow();

	return *this;
}

HBRUSH CSCEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
	//if (m_rect_NCtop.IsRectEmpty())
	//{
	//	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
	//}

	HBRUSH hbr = (HBRUSH)m_br_back; // Passing a Handle to the Brush

	if (m_transparent)
	{
		m_br_back.DeleteObject();
		m_br_back.CreateStockObject(HOLLOW_BRUSH);
		pDC->SetBkMode(TRANSPARENT);
	}
	else if (GetStyle() & ES_READONLY)
	{
		pDC->SetTextColor(m_cr_text);
		pDC->SetBkColor(::GetSysColor(COLOR_3DFACE));
	}
	//else if (!IsWindowEnabled() || nCtlColor == CTLCOLOR_STATIC)
	//{
	//	pDC->SetTextColor(m_cr_text_disabled);
	//	pDC->SetBkColor(m_cr_back_disabled);
	//	m_br_back_disabled.DeleteObject();
	//	m_br_back_disabled.CreateSolidBrush(m_cr_back_disabled);
	//	hbr = (HBRUSH)m_br_back_disabled;
	//}
	else
	{
		pDC->SetTextColor(m_cr_text);
		pDC->SetBkColor(m_cr_back);
 		m_br_back.DeleteObject();
 		m_br_back.CreateSolidBrush(m_cr_back);
		hbr = (HBRUSH)m_br_back;
	}

	return hbr;
}


CSCEdit& CSCEdit::set_transparent(bool transparent)
{
	m_transparent = transparent;

	m_br_back.DeleteObject();

	if (m_transparent)
	{
		m_br_back.CreateStockObject(HOLLOW_BRUSH);
	}
	else
	{
		m_br_back.CreateSolidBrush(m_cr_back);
	}

	update_ctrl();

	return *this;
}

bool CSCEdit::set_read_only(bool bReadOnly)
{
	//readonly�� �� set_back_color�� ȣ���ϸ� ���� m_cr_backColor�� ��������Ƿ�
	//�� �������� �������� ���� ���� ������ �������ش�.
	if (bReadOnly)
	{
		m_br_back.DeleteObject();
		m_br_back.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));
	}
	else
	{
		set_back_color(m_cr_back);
	}

	return CEdit::SetReadOnly(bReadOnly);
}

CSCEdit& CSCEdit::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();

	return *this;
}

CSCEdit& CSCEdit::set_font_size( int nSize )
{
	m_font_size = nSize;
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();

	return *this;
}

CSCEdit& CSCEdit::set_font_bold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	reconstruct_font();

	return *this;
}

CSCEdit& CSCEdit::set_auto_font_size(bool bAuto, double ratio)
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

void CSCEdit::recalc_font_size()
{
	CRect	r;
	GetClientRect( r );

	if (m_auto_resize_font)
		m_lf.lfHeight = -((double)r.Height() * m_auto_resize_ratio);
	else
		m_lf.lfHeight = m_default_height;

	reconstruct_font();
}

void CSCEdit::OnSize(UINT nType, int cx, int cy)
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
	if (m_auto_resize_font)
		recalc_font_size();
}

/*
void CSCEdit::OnWindowPosChanged(WINDOWPOS* lpwndpos)
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
		recalc_font_size();
}
*/

BOOL CSCEdit::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	//hscroll�� �� ����� ���ŵ��� �ʴ� �������� �켱 �ڵ� �߰�.
	switch (pMsg->message)
	{
		case WM_KEYDOWN :
		case WM_LBUTTONDOWN:
			//������ ��쿡�� ĳ�� �̵��̳� �ܾ� ��� ���� �� ���콺 Ŭ���ÿ��� ȭ�鰻���� �ʿ��ϴ�.
			update_ctrl();
			break;
	}

	return CEdit::PreTranslateMessage(pMsg);
}


void CSCEdit::OnPaint()
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


void CSCEdit::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
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
	m_rect_NCtop = rectWnd;

	m_rect_NCtop.DeflateRect(uiCX, uiCY, uiCX, uiCenterOffset + uiVClientHeight + uiCY);

	m_rect_NCbottom = rectWnd;

	m_rect_NCbottom.DeflateRect(uiCX, uiCenterOffset + uiVClientHeight + uiCY, uiCX, uiCY);

	//lpncsp->rgrc[0].top += uiCenterOffset;
	//lpncsp->rgrc[0].bottom -= uiCenterOffset;

	//lpncsp->rgrc[0].left += uiCX;
	//lpncsp->rgrc[0].right -= uiCY;
	CEdit::OnNcCalcSize(bCalcValidRects, lpncsp);
}


void CSCEdit::OnNcPaint()
{
	Default();

	bool draw_border = true;
	if (m_transparent && draw_border)
	{
		CClientDC dc(this);
		CRect rc;

		GetClientRect(rc);
		CPen pen(PS_SOLID, 1, RGB(128, 128, 128));
		CPen* pOldPen = (CPen*)(draw_border ? dc.SelectObject(&pen) : dc.SelectStockObject(NULL_PEN));
		CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);

		dc.Rectangle(rc);
		//dc.Draw3dRect(rc, RGB(128, 128, 128), RGB(128, 128, 128));

		dc.SelectObject(pOldPen);
		dc.SelectObject(pOldBrush);
	}
	return;

	if (!IsWindowEnabled())
		return;


	//CWindowDC dc(this);
	//CBrush Brush(m_cr_back);//GetSysColor(COLOR_WINDOW));

	//dc.FillRect(m_rect_NCbottom, &m_br_back);
	//dc.FillRect(m_rect_NCtop, &m_br_back);

	//Brush.DeleteObject();
}

void CSCEdit::update_ctrl()
{
	if (!m_transparent)
		return;
	
	CWnd* pParent = GetParent();
	CRect rect;

	GetWindowRect(rect);
	pParent->ScreenToClient(rect);
	//rect.top -= 5;
	//rect.InflateRect( 5, 5 );		//�̰� �����ָ� ��ġ�̵��� �ܻ����

	pParent->InvalidateRect(rect, FALSE);
	

	//���� parent�� �����̳� ��� �׸��� �ְ�
//�׷����� �̹����� ����� ������ PNG��� �����ϰ� �׸��� ����.
	if (m_transparent)
	{
		CClientDC dc(this);
		CRect Rect;
		GetWindowRect(&Rect);
		CWnd* pParent = GetParent();
		ASSERT(pParent);
		pParent->ScreenToClient(&Rect);  //convert our corrdinates to our parents
		//copy what's on the parents at this point
		CDC* pDC = pParent->GetDC();
		CDC MemDC;
		CBitmap bmp;
		MemDC.CreateCompatibleDC(pDC);
		bmp.CreateCompatibleBitmap(pDC, Rect.Width(), Rect.Height());
		CBitmap* pOldBmp = MemDC.SelectObject(&bmp);
		MemDC.BitBlt(0, 0, Rect.Width(), Rect.Height(), pDC, Rect.left, Rect.top, SRCCOPY);
		dc.BitBlt(0, 0, Rect.Width(), Rect.Height(), &MemDC, 0, 0, SRCCOPY);
		MemDC.SelectObject(pOldBmp);
		pParent->ReleaseDC(pDC);
		MemDC.DeleteDC();
	}
}

//OnEnKillfocus()���� ���� ȣ��ȴ�.
void CSCEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
}

void CSCEdit::OnEnKillfocus()
{
	update_ctrl();
	::SendMessage(GetParent()->m_hWnd, Message_CSCEditMessage, (WPARAM)this, (LPARAM)WM_KILLFOCUS);
}


void CSCEdit::OnEnUpdate()
{
	update_ctrl();
}


void CSCEdit::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CEdit::OnWindowPosChanged(lpwndpos);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	update_ctrl();
}
/*
//ON_WM_CTLCOLOR_REFLECT() �������� OnCtlColor()�� ȣ����� �ʴ´�.
HBRUSH CSCEdit::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	return CEdit::OnCtlColor(pDC, pWnd, nCtlColor);
}
*/

void CSCEdit::OnEnSetfocus()
{
	update_ctrl();
}

