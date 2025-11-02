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
	//m_cr_back_disabled.SetFromCOLORREF(GetSysColor(COLOR_3DSHADOW));

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
	//ON_WM_NCPAINT()
	ON_CONTROL_REFLECT_EX(EN_SETFOCUS, &CSCEdit::OnEnSetfocus)
	ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, &CSCEdit::OnEnKillfocus)
	ON_CONTROL_REFLECT_EX(EN_UPDATE, &CSCEdit::OnEnUpdate)
	ON_WM_WINDOWPOSCHANGED()
	//}}AFX_MSG_MAP
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_CONTROL_REFLECT_EX(EN_CHANGE, &CSCEdit::OnEnChange)
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

	//border를 자체적으로 그리고 있으므로 resource editor에서 WS_BORDER는 해제시켜야 한다.
	//이 때 WS_BORDER만 제거하면 안되고 WS_EX_CLIENTEDGE까지 함께 제거시켜야 한다.
	if ((GetStyle() & WS_BORDER) ||
		(GetExStyle() & WS_EX_CLIENTEDGE) ||
		(GetExStyle() & WS_EX_STATICEDGE))
	{
		m_draw_border = true;
		ModifyStyle(WS_BORDER, 0);
		ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
		ModifyStyleEx(WS_EX_STATICEDGE, 0);
	}

	//ModifyStyle(0, ES_MULTILINE);

	//style을 변경했음에도 불구하고 스타일이 기대와 다르게 표시된다.
	//뭔가 resize된 CSCEdit은 기대한대로 표시되는데
	//크기가 동일하고 resize되지 않는 CSCEdit은 스타일이 다르게 표시된다.
	//아래 코드를 통해 인위적으로 크기조정 코드를 실행하니 스타일이 원하는대로 표시된다.
	//뭔가 근본적인 해결 방법을 찾는중...
	CRect rc;
	GetWindowRect(rc);
	rc.right++;
	GetParent()->ScreenToClient(rc);
	MoveWindow(rc);
	rc.right--;
	MoveWindow(rc);

	//parent에서 이 함수를 직접 호출하여 세팅하지 않을 경우
	//기본적으로 set_line_align() 함수를 호출해줘야만 m_valign의 기본값이 적용된다.
	//동적 생성하는 CSCEdit의 경우에도 잘 적용되는지 확인이 필요하다.
	//CString text = get_text();
	set_line_align(m_valign);

	//RedrawWindow();
	//UpdateWindow();
}

void CSCEdit::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont( &m_font, true );

	set_line_align(m_valign);

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

//CEdit::SetRect()를 이용해서 상하좌우 여백의 크기를 조정할 수 있는데
//ES_MULTILINE 속성이 있어야만 동작하므로 속성에 반드시 멀티라인 속성을 설정해야 한다.
//ES_MULTILINE 속성은 생성후에는 변경할 수 없으므로 반드시 속성창에서 설정해줘야 한다.
//https://forums.codeguru.com/showthread.php?361420-Want-to-set-quot-ES_MULTILINE-quot-property-of-Edit-object-externally
//생성후에도 SetWindowLong()을 이용하여 변경할 수 있는 속성들
//(ES_LOWERCASE, ES_NUMBER, ES_OEMCONVERT, ES_UPPERCASE, ES_WANTRETURN)
//CDC::DrawText()의 define을 사용한다.(DT_TOP, DT_VCENTER, DT_BOTTOM)
CSCEdit& CSCEdit::set_line_align(DWORD align)
{
	CRect rr, rc;

	GetRect(rr);
	GetClientRect(rc);

	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&m_font);
	CRect rtext;
	CString text = get_text();

	//text가 있어야 DT_CALCRECT에 의해 기본 valign이 계산되므로 없다면 임시 text로 계산해서 SetRect()시켜줘야 한다.
	//그래야 기본 m_valign값에 따라 정렬이 적용된다.
	if (text.IsEmpty())
		text = _T("Test text");

	dc.DrawText(text, rtext, DT_CALCRECT);

	rr = rc;

	m_valign = align;

	if (align & DT_VCENTER)
	{
		rr.top = (rr.Height() - rtext.Height()) / 2 - 1;
		//TRACE(_T("%s vcenter\n"), text);
	}
	else if (align & DT_BOTTOM)
	{
		//TRACE(_T("%s bottom\n"), text);
		//-4를 해주지 않으면 DT_TOP과 같이 표시되고 커서도 보이지 않게 되는데
		//이는 실제 텍스트를 출력할 공간의 크기가 적어서 top에 표시하고 커서도 보이지 않게 되는 현상이므로
		//height를 좀 더 크게 잡아주면 bottom에 맞춰 표시된다.
		rr.top = rr.bottom - rtext.Height() - m_border_width;
	}
	else
	{
		rr.top = m_border_width;
		//TRACE(_T("%s top\n"), text);
	}

	//font에 따라 약간 다르지만 top--해줘야 자연스럽다.
	//rr.top--;
	//rr.top += m_border_width;
	rr.bottom -= m_border_width;

	DWORD margin = GetMargins();
	rr.left += (LOWORD(margin) + m_border_width);
	rr.right -= (HIWORD(margin) + m_border_width);

	//TRACE(_T("ltrl = %d, %d, %d, %d\n"), rr.left, rr.top, rr.right, rr.bottom);
	SetRect(rr);

	//Invalidate();
	//RedrawWindow();
	//UpdateWindow();

	dc.SelectObject(pOldFont);

	return *this;
}

CSCEdit& CSCEdit::set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back)
{
	m_cr_text = cr_text;

	m_cr_back = cr_back; // Passing the value passed by the dialog to the member varaible for Backgound Color
	m_br_back.DeleteObject(); // Deleting any Previous Brush Colors if any existed.
	m_br_back.CreateSolidBrush(cr_back.ToCOLORREF()); // Creating the Brush Color For the Edit Box Background

	RedrawWindow();

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

//read only일 때 배경색을 변경할 수 있다. 파라미터를 주지 않으면 윈도우 기본 readonly 배경색(COLOR_3DFACE)으로 설정된다.
CSCEdit& CSCEdit::set_back_color_readonly(Gdiplus::Color cr_back_readonly)
{
	m_cr_back_readonly = cr_back_readonly; // Passing the value passed by the dialog to the member varaible for ReadOnly Background Color
	RedrawWindow();
	return *this;
}

void CSCEdit::set_use_readonly_color(bool use_default)
{
	m_use_readonly_color = use_default;

	if (use_default)
	{
		//m_cr_back_readonly.SetFromCOLORREF(::GetSysColor(COLOR_3DFACE));
	}
	else
	{
		//m_cr_back_readonly.SetFromCOLORREF(m_cr_back.ToCOLORREF());
	}

	Invalidate();
}

HBRUSH CSCEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
	//if (m_rect_NCtop.IsRectEmpty())
	//{
	//	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
	//}

	HBRUSH hbr = (HBRUSH)m_br_back; // Passing a Handle to the Brush

	CRect rc;
	GetClientRect(rc);

	if (m_transparent)
	{
		m_br_back.DeleteObject();
		m_br_back.CreateStockObject(HOLLOW_BRUSH);
		pDC->SetBkMode(TRANSPARENT);
	}
	else if (GetStyle() & ES_READONLY)
	{
		pDC->SetTextColor(m_cr_text.ToCOLORREF());
		//pDC->SetBkColor(::GetSysColor(COLOR_3DFACE));
		//readonly일 경우에도 배경색을 기본 읽기전용 색으로 바꾸지 않고 지정된 배경색을 유지시켜준다.
		//pDC->SetBkColor(m_cr_back.ToCOLORREF());

		if (IsWindowEnabled())
		{
			pDC->SetBkColor(m_use_readonly_color ? m_cr_back_readonly.ToCOLORREF() : m_cr_back.ToCOLORREF());
			m_br_back.DeleteObject();
			m_br_back.CreateSolidBrush(m_use_readonly_color ? m_cr_back_readonly.ToCOLORREF() : m_cr_back.ToCOLORREF());
			hbr = (HBRUSH)m_br_back;
		}
		else
		{
			pDC->SetBkColor(m_cr_back_disabled.ToCOLORREF());
			m_br_back_disabled.DeleteObject();
			m_br_back_disabled.CreateSolidBrush(m_cr_back_disabled.ToCOLORREF());
			hbr = (HBRUSH)m_br_back_disabled;
		}
	}
	else if (!IsWindowEnabled())// || nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetTextColor(m_cr_text_disabled.ToCOLORREF());
		pDC->SetBkColor(m_cr_back_disabled.ToCOLORREF());
		m_br_back.DeleteObject();
		m_br_back.CreateSolidBrush(m_cr_back.ToCOLORREF());
		m_br_back_disabled.DeleteObject();
		m_br_back_disabled.CreateSolidBrush(m_cr_back_disabled.ToCOLORREF());
		hbr = (HBRUSH)m_br_back_disabled;
	}
	else
	{
		pDC->SetTextColor(m_cr_text.ToCOLORREF());
		pDC->SetBkColor(m_cr_back.ToCOLORREF());
 		m_br_back.DeleteObject();
 		m_br_back.CreateSolidBrush(m_cr_back.ToCOLORREF());
		//Gdiplus::Color cr_back = Gdiplus::Color::Yellow;
		//m_br_back.CreateSolidBrush(m_cr_back.ToCOLORREF());

		hbr = (HBRUSH)m_br_back;
	}

	//ExcludeClipRect()를 해주지 않으면 텍스트 출력 영역에 의해 테두리가 지워진다.
	if (m_draw_border)
	{
		ExcludeClipRect(pDC->m_hDC, rc.left, rc.top, rc.left + m_border_width, rc.bottom);		//left
		ExcludeClipRect(pDC->m_hDC, rc.left, rc.top, rc.right, rc.top + m_border_width);		//top
		ExcludeClipRect(pDC->m_hDC, rc.right - m_border_width, rc.top, rc.right, rc.bottom);	//right
		ExcludeClipRect(pDC->m_hDC, rc.left, rc.bottom - m_border_width, rc.right, rc.bottom);	//bottom
	}
	else
	{
		//ExcludeClipRect(pDC->m_hDC, 0, 0, 0, 0);	//이 코드를 사용하면 border를 그렸다가 해제했을 때 잔상이 남음. SelectClipRgn()로 대체하여 해결됨.
		SelectClipRgn(pDC->m_hDC, NULL);
	}

	if (m_action_button)
	{
		rc.left = rc.right - m_sz_action_button.cx;
		ExcludeClipRect(pDC->m_hDC, rc.left, rc.top, rc.right, rc.bottom);
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

CSCEdit& CSCEdit::set_font_bold(int weight)
{
	m_lf.lfWeight = weight;
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

	//resize되면 vcenter 정렬이 해제되는 현상이 있다. 일단 다시 세팅해준다.
	set_line_align(m_valign);
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
			//TRACE(_T("keydown on CSCEdit. key = %d\n"), (int)pMsg->wParam);
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
	return;
	//Default();

	if (/*m_transparent && */m_draw_border)
	{
		CClientDC dc(this);
		CRect rc;

		GetClientRect(rc);

		CPen pen(PS_SOLID, m_border_width, m_cr_border.ToCOLORREF());
		CPen* pOldPen = (CPen*)(m_draw_border ? dc.SelectObject(&pen) : dc.SelectStockObject(NULL_PEN));
		CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);

		if (m_border_type == border_type_sunken)
		{
			dc.DrawEdge(rc, BDR_SUNKENOUTER, BF_RECT);
		}
		else if (m_border_type == border_type_raised)
		{
			dc.DrawEdge(rc, BDR_RAISEDOUTER, BF_RECT);
		}
		else
		{
			dc.DrawEdge(rc, BDR_OUTER, BF_RECT);
		}
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
	//TRACE(_T("OnEnKillfocus\n"));
	update_ctrl();
	draw_dim_text();
	::SendMessage(GetParent()->m_hWnd, Message_CSCEdit, (WPARAM)this, (LPARAM)WM_KILLFOCUS);
	return FALSE;
}


BOOL CSCEdit::OnEnUpdate()
{
	update_ctrl();

	return FALSE;
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
	//TRACE(_T("OnEnSetfocus\n"));
	update_ctrl();
	return FALSE;
}


#if 1
BOOL CSCEdit::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CRect rc;
	Gdiplus::Color cr_back = m_cr_back;
	Gdiplus::Color cr_border = m_cr_border;

	GetClientRect(rc);
	Gdiplus::Graphics g(pDC->GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	if (!IsWindowEnabled())
	{
		cr_back = m_cr_back_disabled;// gRGB(192, 192, 192);
	}
	else if (GetStyle() & ES_READONLY)
	{
		if (m_use_readonly_color)
			cr_back = m_cr_back_readonly;
		else
			cr_back = m_cr_back;
	}
	else if (m_action_button_down)
	{
		cr_back = m_cr_button_back_down;
	}

	/*
	if (m_action_button)
	{
		rc.left = rc.right - m_sz_action_button.cx;

		CMemoryDC dc(pDC, &rc);
		pDC = &dc;

		if (!IsWindowEnabled())
		{
			pDC->FillSolidRect(rc, RGB(192, 192, 192));
		}
		else if (m_action_button_down)
		{
			pDC->FillSolidRect(rc, m_cr_button_back_down.ToCOLORREF());
		}
		else
		{
			pDC->FillSolidRect(rc, m_cr_button_back.ToCOLORREF());
		}

		Gdiplus::Pen pen(IsWindowEnabled() ? Gdiplus::Color::RoyalBlue : Gdiplus::Color::Gray, 2.0F);

		CPoint cp = rc.CenterPoint();

		//검색일 경우 돋보기 이미지를 그려준다.
		if (m_action_button == action_find)
		{
			if (m_action_button_down)
				cp.Offset(1, 1);

			int size = 12;
			CRect r = make_center_rect(cp.x, cp.y, size, size);
			r.OffsetRect(-2, -2);
			g.DrawEllipse(&pen, CRect2GpRect(r));
			g.DrawLine(&pen, cp.x + 2, cp.y + 2, cp.x + 7, cp.y + 7);
		}
	}
	*/
	//m_draw_border이면 m_cr_border 색상으로 그리지만 false이면 그리지 않는다.
	draw_rect(g, rc, (m_draw_border ? m_cr_border : Gdiplus::Color::Transparent), cr_back, m_border_width);
	//draw_round_rect(&g, CRect2GpRect(rc), (m_draw_border ? m_cr_border : Gdiplus::Color::Transparent), cr_back, rc.Height()/2, m_border_width);

	//pDC->SetBkMode(TRANSPARENT);

	return FALSE;
	return CEdit::OnEraseBkgnd(pDC);
}
#else
BOOL CSCEdit::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CRect rc;
	Gdiplus::Color cr_back = Gdiplus::Color::Red;// m_cr_back;
	Gdiplus::Color cr_border = m_cr_border;

	GetClientRect(rc);
	Gdiplus::Graphics g(pDC->GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	if (m_action_button)
		rc.left = rc.right - m_sz_action_button.cx;

	CMemoryDC dc(pDC, &rc);
	//pDC = &dc;
	/*
	if (!IsWindowEnabled())
	{
		cr_back = gRGB(192, 192, 192);
	}
	else if (GetStyle() & ES_READONLY)
	{
		cr_back = gRGB(255, 192, 192);
	}
	else if (m_action_button_down)
	{
		cr_back = m_cr_button_back_down;
	}
	*/
	if (m_action_button)
	{
		Gdiplus::Pen pen(IsWindowEnabled() ? Gdiplus::Color::RoyalBlue : Gdiplus::Color::Gray, 2.0F);

		CPoint cp = rc.CenterPoint();

		//검색일 경우 돋보기 이미지를 그려준다.
		if (m_action_button == action_find)
		{
			if (m_action_button_down)
				cp.Offset(1, 1);

			int size = 12;
			CRect r = make_center_rect(cp.x, cp.y, size, size);
			r.OffsetRect(-2, -2);
			g.DrawEllipse(&pen, CRect2GpRect(r));
			g.DrawLine(&pen, cp.x + 2, cp.y + 2, cp.x + 7, cp.y + 7);
		}
	}

	//m_draw_border이면 m_cr_border 색상으로 그리지만 false이면 그리지 않는다.
	//또한 IsWindowEnabled(), ES_READONLY에 따라 배경색이 달라진다.
	draw_rect(g, rc, (m_draw_border ? m_cr_border : Gdiplus::Color::Transparent), cr_back, m_border_width);

	return FALSE;
	return CEdit::OnEraseBkgnd(pDC);
}
#endif

void CSCEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_action_button && mouse_in_action_button())
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
	if (m_action_button && mouse_in_action_button())
	{
		m_action_button_down = false;
		Invalidate();
		TRACE(_T("lbutton up\n"));
		return;
	}

	CEdit::OnLButtonUp(nFlags, point);
}

//마우스가 액션버튼내에 있는지 판별
bool CSCEdit::mouse_in_action_button(CPoint pt)
{
	CRect rc;
	GetClientRect(rc);

	if (pt.x == 0 && pt.y == 0)
	{
		GetCursorPos(&pt);
		ScreenToClient(&pt);
	}

	rc.left = rc.right - m_sz_action_button.cx;
	if (rc.PtInRect(pt))
		return true;

	return false;
}

BOOL CSCEdit::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_action_button && mouse_in_action_button())
	{
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
		return true;
	}

	return CEdit::OnSetCursor(pWnd, nHitTest, message);
}


BOOL CSCEdit::OnEnChange()
{
	//TRACE(_T("CSCEdit::OnEnChange()\n"));
	return FALSE;
}

void CSCEdit::draw_dim_text()
{
	CString text;
	GetWindowText(text);

	if (GetFocus() == this || !IsWindowEnabled() || text.GetLength())
		return;

	if (m_dim_text.GetLength() == 0)
		return;

	CClientDC	dc(this);
	CRect		rc;
	int			iState = dc.SaveDC();					// Save The DC State

	//pDC = &dc;
	GetClientRect(&rc);							// Get Drawing Area

	if (m_draw_border)
		rc.DeflateRect(m_border_width, m_border_width);

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

	//dimtext도 m_valign 설정값의 영향을 받는다.
	dc.DrawText(_T(" ") + m_dim_text, -1, &rc, dwText | DT_SINGLELINE | m_valign);
	dc.RestoreDC(iState);								// Restore The DC State
}

CSCEdit& CSCEdit::set_dim_text(CString dim_text, Gdiplus::Color cr)
{
	m_dim_text = dim_text;

	if (cr.GetValue() != Gdiplus::Color::Transparent)
		m_cr_dim_text = cr;

	return *this;
}

//border를 설정할 때 set_draw_border();를 호출하면 모든 설정값은 기본 멤버변수값대로 설정된다.
//즉, border width는 m_border_width 값이 사용된다.
//border를 해제하기 위해 set_draw_border(false);를 호출하면 border를 그리지 않을 뿐 기본 설정값들은 유지된다.
//다시 border를 그리기 위해 set_draw_border();를 호출하면 전에 설정된 세팅값대로 그릴 수 있다.
void CSCEdit::set_draw_border(bool draw, int border_width, Gdiplus::Color cr_border, int border_type)
{
	m_draw_border = draw;

	if (border_width > 0)
		m_border_width = border_width;

	if (border_type > border_type_disregard)
		m_border_type = border_type;

	if (cr_border.GetValue() != Gdiplus::Color::Transparent)
		m_cr_border = cr_border;

	//set_line_align(m_valign);

	Invalidate();
	//RedrawWindow();
	//UpdateWindow();
}