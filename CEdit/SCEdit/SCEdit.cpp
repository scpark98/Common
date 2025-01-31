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

#include "SCEdit.h"
#include "../../Functions.h"
#include "../../MemoryDC.h"

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
	m_cr_text.SetFromCOLORREF(::GetSysColor(COLOR_WINDOWTEXT));
	m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_WINDOW));
	m_cr_text_disabled.SetFromCOLORREF(::GetSysColor(COLOR_GRAYTEXT));
	m_cr_back_disabled.SetFromCOLORREF(GetSysColor(COLOR_3DSHADOW));

	m_cr_button_back = Gdiplus::Color(0, 255, 0);
	m_cr_button_back_hover = Gdiplus::Color(64, 255, 64);
	m_cr_button_back_down = Gdiplus::Color(0, 192, 0);

	m_br_back.CreateSolidBrush(m_cr_back.ToCOLORREF());
	m_br_back_disabled.CreateSolidBrush(m_cr_back_disabled.ToCOLORREF());

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
	ON_WM_PAINT()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_CONTROL_REFLECT_EX(EN_SETFOCUS, &CSCEdit::OnEnSetfocus)
	ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, &CSCEdit::OnEnKillfocus)
	ON_CONTROL_REFLECT(EN_UPDATE, &CSCEdit::OnEnUpdate)
	ON_WM_WINDOWPOSCHANGED()
	//}}AFX_MSG_MAP
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_CONTROL_REFLECT(EN_CHANGE, &CSCEdit::OnEnChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSCEdit message handlers

void CSCEdit::PreSubclassWindow()
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

//CEdit::SetRect()를 이용해서 상하좌우 크기를 조정할 수 있는데
//ES_MULTILINE 속성이 있어야만 동작하므로 속성에 반드시 멀티라인 속성을 설정해야 한다.
//ES_MULTILINE 속성은 생성후에는 변경할 수 없는 속성이다.
//https://forums.codeguru.com/showthread.php?361420-Want-to-set-quot-ES_MULTILINE-quot-property-of-Edit-object-externally
//생성후에도 SetWindowLong()을 이용하여 변경할 수 있는 속성들
//(ES_LOWERCASE, ES_NUMBER, ES_OEMCONVERT, ES_UPPERCASE, ES_WANTRETURN)
//CDC::DrawText()의 define을 사용한다.(DT_TOP, DT_VCENTER, DT_BOTTOM)
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

CSCEdit& CSCEdit::set_text_color(Gdiplus::Color crColor)
{
	m_cr_text = crColor; // Passing the value passed by the dialog to the member varaible for Text Color
	RedrawWindow();

	return *this;
}

CSCEdit& CSCEdit::set_back_color(Gdiplus::Color crColor)
{
	m_cr_back = crColor; // Passing the value passed by the dialog to the member varaible for Backgound Color
	m_br_back.DeleteObject(); // Deleting any Previous Brush Colors if any existed.
	m_br_back.CreateSolidBrush(crColor.ToCOLORREF()); // Creating the Brush Color For the Edit Box Background
	//m_br_back.CreateSolidBrush(HOLLOW_BRUSH);
	RedrawWindow();

	return *this;
}

CSCEdit& CSCEdit::set_text_color_disabled(Gdiplus::Color cr_text_disabled)
{
	m_cr_text_disabled = cr_text_disabled;
	RedrawWindow();

	return *this;
}

CSCEdit& CSCEdit::set_back_color_disabled(Gdiplus::Color cr_back_disabled)
{
	m_cr_back_disabled = cr_back_disabled;
	m_br_back_disabled.DeleteObject();
	m_br_back_disabled.CreateSolidBrush(m_cr_back_disabled.ToCOLORREF());
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
		pDC->SetTextColor(m_cr_text.ToCOLORREF());
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
		pDC->SetTextColor(m_cr_text.ToCOLORREF());
		pDC->SetBkColor(m_cr_back.ToCOLORREF());
 		m_br_back.DeleteObject();
 		m_br_back.CreateSolidBrush(m_cr_back.ToCOLORREF());
		hbr = (HBRUSH)m_br_back;
	}

	if (m_action_button)
	{
		CRect r;
		GetClientRect(r);
		r.left = r.right - m_sz_action_button.cx;
		ExcludeClipRect(pDC->m_hDC, r.left, r.top, r.right, r.bottom);
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
		m_br_back.CreateSolidBrush(m_cr_back.ToCOLORREF());
	}

	update_ctrl();

	return *this;
}

bool CSCEdit::set_read_only(bool bReadOnly)
{
	//readonly일 때 set_back_color를 호출하면 원래 m_cr_backColor이 덮어써지므로
	//그 변수값은 변경하지 말고 직접 배경색을 변경해준다.
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

void CSCEdit::set_action_button(int action)
{
	m_action_button = action;

	CRect r;
	GetRect(r);

	CRect rc;
	GetClientRect(rc);

	m_sz_action_button = CSize(rc.Height(), rc.Height());
	r.right -= (m_action_button > 0 ? m_sz_action_button.cx : -m_sz_action_button.cx);
	SetRect(&r);
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
void CSCEdit::OnWindowPosChanged(WINDOWPOS* lpwndpos)
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

BOOL CSCEdit::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	//hscroll될 때 배경이 갱신되지 않는 현상으로 우선 코드 추가.
	switch (pMsg->message)
	{
		case WM_KEYDOWN :
			TRACE(_T("keydown on CSCEdit. key = %d\n"), (int)pMsg->wParam);
			break;
		case WM_LBUTTONDOWN:
			//투명일 경우에는 캐럿 이동이나 단어 블록 선택 후 마우스 클릭시에도 화면갱신이 필요하다.
			update_ctrl();
			break;
	}

	return CEdit::PreTranslateMessage(pMsg);
}


void CSCEdit::OnPaint()
{
	Default();
	draw_dim_text();

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
	//CEdit::OnPaint();
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

	//lpncsp->rgrc[0].top += uiCenterOffset;
	//lpncsp->rgrc[0].bottom -= uiCenterOffset;

	//lpncsp->rgrc[0].left += uiCX;
	//lpncsp->rgrc[0].right -= uiCY;
	CEdit::OnNcCalcSize(bCalcValidRects, lpncsp);
}


void CSCEdit::OnNcPaint()
{
	//return;
	Default();

	if (m_transparent && m_draw_border)
	{
		CClientDC dc(this);
		CRect rc;

		GetClientRect(rc);
		CPen pen(PS_SOLID, 1, RGB(128, 128, 128));
		CPen* pOldPen = (CPen*)(m_draw_border ? dc.SelectObject(&pen) : dc.SelectStockObject(NULL_PEN));
		CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);

		//dc.Rectangle(rc);
		dc.Draw3dRect(rc, RGB(128, 128, 128), RGB(128, 128, 128));

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
	//rect.InflateRect( 5, 5 );		//이거 안해주면 위치이동시 잔상생김

	pParent->InvalidateRect(rect, FALSE);
	

	//만약 parent에 배경색이나 배경 그림이 있고
//그려지는 이미지가 배경이 투명한 PNG라면 투명하게 그리기 위해.
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

BOOL CSCEdit::OnEnKillfocus()
{
	TRACE(_T("OnEnKillfocus\n"));
	update_ctrl();
	draw_dim_text();
	::SendMessage(GetParent()->m_hWnd, Message_CSCEditMessage, (WPARAM)this, (LPARAM)WM_KILLFOCUS);
	return FALSE;
}


void CSCEdit::OnEnUpdate()
{
	update_ctrl();
}


void CSCEdit::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CEdit::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	update_ctrl();
}
/*
//ON_WM_CTLCOLOR_REFLECT() 때문인지 OnCtlColor()는 호출되지 않는다.
HBRUSH CSCEdit::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	return CEdit::OnCtlColor(pDC, pWnd, nCtlColor);
}
*/

BOOL CSCEdit::OnEnSetfocus()
{
	TRACE(_T("OnEnSetfocus\n"));
	update_ctrl();
	return FALSE;
}



BOOL CSCEdit::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CRect r;

	GetClientRect(r);
	Gdiplus::Graphics g(pDC->GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	if (m_action_button)
	{
		r.left = r.right - m_sz_action_button.cx;

		//CMemoryDC dc(pDC, &r);

		if (!IsWindowEnabled())
		{
			pDC->FillSolidRect(r, RGB(192, 192, 192));
		}
		else if (m_action_button_down)
		{
			pDC->FillSolidRect(r, m_cr_button_back_down.ToCOLORREF());
		}
		else
		{
			pDC->FillSolidRect(r, m_cr_button_back.ToCOLORREF());
		}

		Gdiplus::Pen pen(IsWindowEnabled() ? Gdiplus::Color::RoyalBlue : Gdiplus::Color::Gray, 2.0F);

		CPoint cp = r.CenterPoint();

		//검색일 경우 돋보기 이미지를 그려준다.
		if (m_action_button == action_find)
		{
			if (m_action_button_down)
				cp.Offset(1, 1);

			int size = 12;
			r = make_center_rect(cp.x, cp.y, size, size);
			r.OffsetRect(-2, -2);
			g.DrawEllipse(&pen, CRectTogpRect(r));
			g.DrawLine(&pen, cp.x + 2, cp.y + 2, cp.x + 7, cp.y + 7);
		}
	}

	if (m_draw_border)
	{
		GetClientRect(r);
		draw_rectangle(g, r, IsWindowEnabled() ? Gdiplus::Color::Blue : Gdiplus::Color::Gray);
	}

	return FALSE;
	//return CEdit::OnEraseBkgnd(pDC);
}


void CSCEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_action_button && in_action_button())
	{
		m_action_button_down = true;
		Invalidate();
		TRACE(_T("lbutton down\n"));
		return;
	}

	CEdit::OnLButtonDown(nFlags, point);
}


void CSCEdit::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_action_button && in_action_button())
	{
		m_action_button_down = false;
		Invalidate();
		TRACE(_T("lbutton up\n"));
		return;
	}

	CEdit::OnLButtonUp(nFlags, point);
}

//마우스가 액션버튼내에 있는지 판별
bool CSCEdit::in_action_button()
{
	CRect rc;
	GetClientRect(rc);

	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	rc.left = rc.right - m_sz_action_button.cx;
	if (rc.PtInRect(pt))
		return true;

	return false;
}

BOOL CSCEdit::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_action_button && in_action_button())
	{
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
		return true;
	}

	return CEdit::OnSetCursor(pWnd, nHitTest, message);
}


void CSCEdit::OnEnChange()
{

}

void CSCEdit::draw_dim_text()
{
	CString text;
	GetWindowText(text);

	if (GetFocus() == this || !IsWindowEnabled() || text.GetLength())
		return;

	if (!m_show_dim_text || m_dim_text.GetLength() == 0)
		return;

	CClientDC	dc(this);
	CRect		rc;
	int			iState = dc.SaveDC();					// Save The DC State

	//pDC = &dc;
	GetClientRect(&rc);							// Get Drawing Area

	if (m_action_button)
		rc.right -= m_sz_action_button.cx;

	dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());
	//rRect.OffsetRect( 1, 1 );							// Add Sanity Space

	dc.SelectObject((*GetFont()));					// Use The Control's Current Font
	dc.SetTextColor(m_cr_dim_text.ToCOLORREF());				// Set The Text Color
	//pDC->SetBkColor(GetSysColor(COLOR_WINDOW));	// Set The Bk Color
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

	dc.DrawText(_T(" ") + m_dim_text, -1, &rc, dwText | DT_SINGLELINE | DT_VCENTER);
	dc.RestoreDC(iState);								// Restore The DC State

	return;												// Done!
}

CSCEdit& CSCEdit::set_dim_text(bool show, CString dim_text, Gdiplus::Color cr)
{
	m_show_dim_text = show;
	m_dim_text = dim_text;
	m_cr_dim_text = cr;
	return *this;
}
