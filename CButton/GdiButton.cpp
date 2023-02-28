// GdiButton.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "GdiButton.h"

#include "../Functions.h"
#include "../MemoryDC.h"
#include "../GdiPlusBitmap.h"

#pragma warning(disable: 4305)	//'argument': truncation from 'double' to 'Gdiplus::REAL'

#define	TIMER_BLINK		0

// CGdiButton

IMPLEMENT_DYNAMIC(CGdiButton, CButton)

CGdiButton::CGdiButton()
{
	m_button_style		= BS_PUSHBUTTON;

	m_bAsStatic			= false;

	m_nAnchor			= ANCHOR_NONE;
	m_nAnchorMarginX	= 0;
	m_nAnchorMarginY	= 0;

	m_bPushed			= false;
	m_use_hover			= true;
	m_bHover			= false;
	m_bIsTracking		= false;
	m_down_offset		= CPoint(1, 1);

	m_bHasFocus			= false;
	m_bShowFocusRect	= false;
	m_crFocusRect		= RGB(6, 205, 255);
	m_nFocusRectWidth	= 2;

	m_b3DRect			= true;

	m_bBlink			= FALSE;
	m_bBlinkStatus		= FALSE;
	m_nBlinkTime0		= 400;
	m_nBlinkTime1		= 1200;

	memset(&m_lf, 0, sizeof(LOGFONT));

	EnableToolTips(true);
	EnableTrackingToolTips(true);

	//매트릭스는 CGdiplusBitmap으로 이동할것!
	m_grayMatrix = {
		0.299f, 0.299f, 0.299f, 0.00f, 0.00f,
		0.587f, 0.587f, 0.587f, 0.00f, 0.00f,
		0.114f, 0.114f, 0.114f, 0.00f, 0.00f,
		0.00f, 0.00f, 0.00f, 1.00f, 0.00f,
		0.101f, 0.101f, 0.101f, 0.00f, 1.00f
		//버튼 disable일때 회색조가 좀 어두워서 0.50f로 변경했으나 다른 이미지일 경우는 너무 밝아져서 0.01f로 변경
	};

	m_hoverMatrix = {
		1.15f, 0.00f, 0.00f, 0.00f, 0.00f,		// red scaling factor
		0.00f, 1.15f, 0.00f, 0.00f, 0.00f,		// green scaling factor 
		0.00f, 0.00f, 1.15f, 0.00f, 0.00f,		// blue scaling factor 
		0.00f, 0.00f, 0.00f, 1.00f, 0.00f,		// alpha scaling factor 
		0.00f, 0.00f, 0.00f, 0.00f, 1.00f
	};	// three translations 

	m_downMatrix = {
		0.80f, 0.00f, 0.00f, 0.00f, 0.00f,		// red scaling factor
		0.00f, 0.80f, 0.00f, 0.00f, 0.00f,		// green scaling factor 
		0.00f, 0.00f, 0.80f, 0.00f, 0.00f,		// blue scaling factor 
		0.00f, 0.00f, 0.00f, 1.00f, 0.00f,		// alpha scaling factor 
		0.00f, 0.00f, 0.00f, 0.00f, 1.00f
	};	// three translations 
}

CGdiButton::~CGdiButton()
{
	release_all();
}

void CGdiButton::release_all()
{
	m_back.release();
	m_back_origin.release();

	for (int i = 0; i < m_image.size(); i++)
	{
		delete m_image[i];
		//m_image[i]->normal.release();
		//m_image[i]->over.release();
		//m_image[i]->down.release();
		//m_image[i]->disabled.release();
	}

	m_image.clear();
}

BEGIN_MESSAGE_MAP(CGdiButton, CButton)
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	//ON_NOTIFY_EX(TTN_NEEDTEXT, 0, &CGdiButton::OnToolTipNotify)
END_MESSAGE_MAP()



// CGdiButton 메시지 처리기입니다.

//기존 CButton::SetButtonStyle 함수를 overriding하여 OWNER_DRAW를 추가시켜줘야 한다.
void CGdiButton::SetButtonStyle(UINT nStyle, BOOL bRedraw)
{
	DWORD dwStyle = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);

	CButton::SetButtonStyle(nStyle, bRedraw);

	// BS_OWNERDRAW 속성을 설정한다.
	//주의 : dwStyle은 반드시 CButton::SetButtonStyle() 함수 호출 전에 구한 값을 사용해야 한다.
	::SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle | BS_OWNERDRAW);
	m_button_style = nStyle;
}

bool CGdiButton::add_image(UINT normal, UINT over, UINT down, UINT disabled)
{
	return add_image(_T("PNG"), normal, over, down, disabled);
}

bool CGdiButton::add_image(CString lpType, UINT normal, UINT over, UINT down, UINT disabled)
{
	//m_image라는 list에 push_back하여 유지시키기 위해서는
	//반드시 동적 할당받은 인스턴스를 넣어줘야 한다.
	//그렇지 않으면 어느 시점에서 소멸되어 잘못된 참조가 된다.
	CButtonImage* btn = new CButtonImage();

	//normal은 0이어서는 안된다.
	if (normal == 0)
		return false;

	btn->normal.load(lpType, normal);
	if (btn->normal.empty())
		return false;

	m_width = btn->normal.width;
	m_height = btn->normal.height;

	if (over > 0)
	{
		btn->over.load(lpType, over);
	}
	else
	{
		btn->normal.deep_copy(&btn->over);
		btn->over.set_matrix(&m_hoverMatrix);
	}

	if (down > 0)
	{
		btn->down.load(lpType, down);
	}
	else
	{
		btn->normal.deep_copy(&btn->down);
		btn->down.set_matrix(&m_downMatrix);
	}

	if (disabled > 0)
	{
		btn->disabled.load(lpType, disabled);
	}
	else
	{
		btn->normal.deep_copy(&btn->disabled);
		if (!m_use_normal_image_on_disabled)
			btn->disabled.set_matrix(&m_grayMatrix);
	}


	m_image.push_back(btn);

	fit_to_image(m_fit2image);

	return true;
}

bool CGdiButton::add_image(CGdiplusBitmap img)
{
	//m_image라는 list에 push_back하여 유지시키기 위해서는
	//반드시 동적 할당받은 인스턴스를 넣어줘야 한다.
	//그렇지 않으면 어느 시점에서 소멸되어 잘못된 참조가 된다.
	CButtonImage* btn = new CButtonImage();

	//normal은 0이어서는 안된다.
	if (img.empty())
		return false;

	img.deep_copy(&btn->normal);
	if (btn->normal.empty())
		return false;

	m_width = btn->normal.width;
	m_height = btn->normal.height;

	btn->normal.deep_copy(&btn->over);
	btn->over.set_matrix(&m_hoverMatrix);

	btn->normal.deep_copy(&btn->down);
	btn->down.set_matrix(&m_downMatrix);

	btn->normal.deep_copy(&btn->disabled);
	if (!m_use_normal_image_on_disabled)
		btn->disabled.set_matrix(&m_grayMatrix);

	m_image.push_back(btn);

	fit_to_image(m_fit2image);

	return true;
}

void CGdiButton::use_normal_image_on_disabled(bool use)
{
	m_use_normal_image_on_disabled = use;

	for (int i = 0; i < m_image.size(); i++)
	{
		if (m_image[i]->normal)
		{
			m_image[i]->normal.deep_copy(&m_image[i]->disabled);

			if (!m_use_normal_image_on_disabled)
				m_image[i]->disabled.set_matrix(&m_grayMatrix);
		}
	}
}

void CGdiButton::fit_to_image(bool fit)
{
	if (m_image.size() == 0)
		return;

	m_fit2image = fit;

	if (m_fit2image)
	{
		m_width = m_image[0]->normal.width;
		m_height = m_image[0]->normal.height;
	}
	else
	{
		m_width = m_rOrigin.Width();
		m_height = m_rOrigin.Height();
	}

	resize_control(m_width, m_height);

	UpdateSurface();
}

void CGdiButton::active_index(int index, bool bErase)
{
	if (index < 0 || index >= m_image.size())
		return;

	m_idx = index;
	UpdateSurface();
}

//true이면 1번 이미지를 표시해준다. false일 경우는 0번을 표시하는데 만약 없으면 AddGrayImage로 회색 이미지를 자동 추가한 후 선택해준다.
//기존에는 AddImage에서 checkbox나 radiobutton이면 알아서 grayimage를 추가해줬으나 아래와 같은 문제가 있어서
//자동 추가는 삭제되었다.
//문제1. AddImage단계에서는 추가적인 AddImage가 있을지 없을지 알 수 없으므로 이 단계에서 gray image를 넣는 것은 문제가 된다.
//문제2. GetButtonStyle()로 버튼 타입이 잘 구분되지 않는다. 뭔가 명확한 방법을 찾아야 한다.
//check나 radio의 on, off를 하나의 이미지로만 간단히 사용한다면
//on이미지를 추가한 후 AddGrayImage()를 추가로 호출하여 사용하자.
void CGdiButton::SetCheck(bool bCheck)
{
	m_idx = bCheck;
	UpdateSurface();

	//radio 버튼이 눌려지거나 SetCheck(true)가 호출되면
	//같은 group 내의 다른 버튼들은 unchecked로 만들어 줘야한다.
	//owner draw 속성때문에 WindowProc의 윈도우 기본 메시지를 받아서 처리할 수 없다.
	if ((m_button_style == BS_RADIOBUTTON) && bCheck)
	{
		CWnd *pWndParent = GetParent(); 
		CWnd *pWnd = pWndParent->GetNextDlgGroupItem(this); 
		CWnd* pWndFirstCheck = NULL;

		//만약 버튼이 visible이 false이면 아래 while문은 무한루프를 돈다.
		//이를 방지하기 위해 한번 검사한 윈도우를 기억했다가 그 윈도우를 만나면 종료시킨다.
		while (pWnd && (pWnd != this) && (pWnd != pWndFirstCheck)) 
		{ 
			if (pWnd->GetStyle() & WS_GROUP)
				TRACE(_T("%p : group property\n"), pWnd);
			else
				TRACE(_T("%p : not group property\n"), pWnd);

			if (pWnd->IsKindOf(RUNTIME_CLASS(CGdiButton))) 
			{ 
				if (((CGdiButton*)pWnd)->m_button_style == BS_RADIOBUTTON)
				{
					//((CGdiButton*)pWnd)->SetCheck(BST_UNCHECKED); 
					((CGdiButton*)pWnd)->m_idx = 0;
					((CGdiButton*)pWnd)->UpdateSurface();
				}
			} 

			if (pWndFirstCheck == NULL)
				pWndFirstCheck = pWnd;

			pWnd = pWndParent->GetNextDlgGroupItem(pWnd); 
		}
	}
}
/*
void CGdiButton::SetBackImage(Bitmap* pBack)
{
	if (pBack == NULL)
		return;

	if (m_pBack)
		safe_release(&m_pBack);

	CRect	rc;
	CPoint	pt;

	GetWindowRect(rc);
	pt = rc.TopLeft();
	::ScreenToClient(GetParent()->m_hWnd, &pt);

	Rect cutRect(pt.x, pt.y, rc.Width(), rc.Height());
	m_pBack = pBack->Clone(cutRect, pBack->GetPixelFormat());

	UpdateSurface();
}
*/

void CGdiButton::set_alpha(float alpha)
{
	for (int i = 0; i < m_image.size(); i++)
	{
		m_image[i]->normal.set_alpha(alpha);
		m_image[i]->over.set_alpha(alpha);
		m_image[i]->down.set_alpha(alpha);
		m_image[i]->disabled.set_alpha(alpha);
	}
}

void CGdiButton::add_rgb(int red, int green, int blue, COLORREF crExcept)
{
	for (int i = 0; i < m_image.size(); i++)
	{
		m_image[i]->normal.add_rgb_loop(red, green, blue, GRAY(50));
		//m_image[i]->over.set_alpha(alpha);
		//m_image[i]->down.set_alpha(alpha);
		//m_image[i]->disabled.set_alpha(alpha);
	}
	//Invalidate();
	UpdateSurface();
}

CGdiButton& CGdiButton::text(CString text)
{
	m_text = text;
	SetWindowText(m_text);
	UpdateSurface();
	return *this;
}

CGdiButton& CGdiButton::text_color(COLORREF normal)
{
	return text_color(normal, normal, normal, gray_color(normal));
}

CGdiButton& CGdiButton::text_color(COLORREF normal, COLORREF over, COLORREF down, COLORREF disabled)
{
	m_cr_text.clear();

	m_cr_text.push_back(normal);
	m_cr_text.push_back(over);
	m_cr_text.push_back(down);
	m_cr_text.push_back(disabled);

	UpdateSurface();
	return *this;
}

CGdiButton& CGdiButton::back_color(COLORREF normal)
{
	return back_color(normal, normal, normal, gray_color(normal));
}

CGdiButton& CGdiButton::back_color(COLORREF normal, COLORREF over, COLORREF down, COLORREF disabled)
{
	//배경색을 설정하면 배경 이미지는 해제시킨다.
	m_transparent = false;

	m_back.release();
	m_back_origin.release();

	m_cr_back.clear();
	m_cr_back.push_back(normal);
	m_cr_back.push_back(over);
	m_cr_back.push_back(down);
	m_cr_back.push_back(disabled);

	UpdateSurface();
	return *this;
}

CGdiButton& CGdiButton::set_hover_color_matrix(float fScale)	//1.0f = no effect.
{
	m_hoverMatrix.m[0][0] = fScale;
	m_hoverMatrix.m[1][1] = fScale;
	m_hoverMatrix.m[2][2] = fScale;

	if (m_image.size())
		m_image[0]->over.set_matrix(&m_hoverMatrix);
	return *this;
}

CGdiButton& CGdiButton::set_down_color_matrix(float fScale)	//1.0f = no effect.
{
	m_downMatrix.m[0][0] = fScale;
	m_downMatrix.m[1][1] = fScale;
	m_downMatrix.m[2][2] = fScale;

	if (m_image.size())
		m_image[0]->down.set_matrix(&m_downMatrix);
	return *this;
}

//그림의 크기에 맞게 컨트롤을 resize하고 dx, dy, nAnchor에 따라 move해준다.
void CGdiButton::resize_control(int cx, int cy)
{
	CRect	rc, rParentRect;

	GetParent()->GetClientRect(&rParentRect);

	//parent의 클라이언트 영역 내에서 이 컨트롤의 좌표를 구한다.
	//ScreenToClient(rc);와 ::ScreenToClient(GetParent()->m_hWnd, &pt);의 rc.topleft와 pt는 다른 결과를 보인다.
	GetWindowRect(rc);
	//ScreenToClient(rc);
	CPoint	pt = rc.TopLeft();
	::ScreenToClient(GetParent()->m_hWnd, &pt);
	/*
	if (cx < 0)
		dx = pt.x;

	if (dy < 0)
		dy = pt.y;

	if (m_nAnchor > ANCHOR_NONE)
	{
		if (m_nAnchor & ANCHOR_LEFT)
			dx = m_nAnchorMarginX;

		if (m_nAnchor & ANCHOR_TOP)
			dy = m_nAnchorMarginY;

		if (m_nAnchor & ANCHOR_RIGHT)
			dx = rParentRect.right - m_width - m_nAnchorMarginX;

		if (m_nAnchor & ANCHOR_BOTTOM)
			dy = rParentRect.bottom - m_height - m_nAnchorMarginY;

		if (m_nAnchor & ANCHOR_HCENTER)
			dx = rParentRect.CenterPoint().x - m_width / 2;

		if (m_nAnchor & ANCHOR_VCENTER)
			dy = rParentRect.CenterPoint().y - m_height / 2;
	}
	*/
	//w, h에 +1을 해 준 이유는 버튼의 크기가 fit하면 눌렸을 때 (x+1), (y+1)에 그려지므로
	//오른쪽과 하단의 1픽셀씩 안보이게 된다. 따라서 버튼의 크기는 실제 이미지의 w, h보다 1이 더 커야 한다.
	SetWindowPos(NULL, 0, 0, m_width, m_height, SWP_NOMOVE | SWP_NOZORDER);
}


void CGdiButton::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	// Set control to owner draw
	m_button_style = getButtonStyle(m_hWnd);
	
	CString text;
	GetWindowText(text);
	//TRACE(_T("%s = %d\n"), str, m_button_style);
	
	ModifyStyle(0, BS_OWNERDRAW, SWP_FRAMECHANGED);
	//ModifyStyle(0, WS_TABSTOP, WS_TABSTOP);

	GetWindowRect(m_rOrigin);
	GetParent()->ScreenToClient(m_rOrigin);
	//TRACE(_T("%s : %s\n"), text, GetRectInfoString(m_rOrigin, 0));

	CFont* font = GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf),&m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT),sizeof(m_lf),&m_lf);

	ReconstructFont();

	m_tooltip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE);
	//m_tooltip.SetDelayTime(TTDT_AUTOPOP, -1);
	//m_tooltip.SetDelayTime(TTDT_INITIAL, 0);
	//m_tooltip.SetDelayTime(TTDT_RESHOW, 0);
	//m_tooltip.SetMaxTipWidth(400);
	//m_tooltip.AddTool(this, _T(""));
	//m_tooltip.Activate(TRUE);
	//EnableToolTips(TRUE);
	//EnableTrackingToolTips(TRUE);

	/*
	TOOLINFO ti;
	ti.cbSize = TTTOOLINFOW_V2_SIZE;// sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0;
	ti.hwnd = GetParent()->GetSafeHwnd();
	ti.uId = (UINT)GetSafeHwnd();
	ti.hinst = AfxGetInstanceHandle();
	ti.lpszText = (LPTSTR)_T("skldfjkl");

	SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti);
	SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

	EnableTrackingToolTips(TRUE);
	m_tooltip.Activate(true);
	*/

	CButton::PreSubclassWindow();
}

void CGdiButton::set_tooltip_text(CString text)
{
	m_tooltip_text = text;

	if (!text.IsEmpty())
		m_use_tooltip = true;

	m_tooltip.UpdateTipText(m_tooltip_text, this);
	m_tooltip.AddTool(this, m_tooltip_text);
}


void CGdiButton::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	ASSERT(bCreated);
}


BOOL CGdiButton::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//이 코드를 넣어줘야 disabled에서도 툴팁이 동작하는데
	//이 코드를 컨트롤 클래스에 넣어줘도 소용없다.
	//이 코드는 main에 있어야 한다.
	//disabled가 아닌 경우는 잘 표시된다.
	if (m_use_tooltip && m_tooltip.m_hWnd)
	{
		//m_tooltip.RelayEvent(pMsg);
		
		//msg를 따로 선언해서 사용하지 않고 *pMsg를 그대로 이용하면 이상한 현상이 발생한다.
		MSG msg = *pMsg;
		msg.hwnd = (HWND)m_tooltip.SendMessage(TTM_WINDOWFROMPOINT, 0, (LPARAM) & (msg.pt));

		CPoint pt = msg.pt;

		if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
			::ScreenToClient(msg.hwnd, &pt);

		msg.lParam = MAKELONG(pt.x, pt.y);

		// relay mouse event before deleting old tool 
		m_tooltip.SendMessage(TTM_RELAYEVENT, 0, (LPARAM)&msg);
		
	}

	return CButton::PreTranslateMessage(pMsg);
}

#define MAP_STYLE(src, dest) if(dwStyle & (src)) dwText |= (dest)

void CGdiButton::DrawItem(LPDRAWITEMSTRUCT lpDIS/*lpDrawItemStruct*/)
{
	if (!IsWindowVisible())
		return;

	CDC*		pDC1 = CDC::FromHandle(lpDIS->hDC);
	CRect		rc;
	CRect		rText;
	CPoint		pt(0, 0);
	CGdiplusBitmap*	pImage = NULL;
	CString		text;
	COLORREF	cr_text = ::GetSysColor(COLOR_BTNTEXT);
	COLORREF	cr_back = ::GetSysColor(COLOR_3DFACE);
	DWORD		dwStyle = GetStyle();
	DWORD		dwText = 0;

	GetClientRect(rc);
	GetWindowText(text);

	rText = rc;

	CMemoryDC	dc(pDC1, &rc, false);
	Graphics	g(dc.m_hDC, rc);

	bool is_down = lpDIS->itemState & ODS_SELECTED;
	bool is_disabled = (lpDIS->itemState & ODS_DISABLED);

	//TRACE(_T("is_down = %d\n"), is_down);

	int idx = MIN(m_idx, m_image.size()-1);
	if (idx < 0)
		return;


	//만약 parent에 배경색이나 배경 그림이 있고
	//그려지는 이미지가 배경이 투명한 PNG라면 투명하게 그리기 위해.
	if (m_transparent)
	{
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

	//check or radio인데 m_image.size()가 1뿐이라면
	//checked = normal을, unchecked = disabled를 표시한다.
	bool use_disabled_image = false;
	if (m_button_style != BS_PUSHBUTTON && m_image.size() == 1 && m_idx == 0)
		use_disabled_image = true;

	if (m_cr_text.size() == 4)
	{
		if (is_disabled)
			cr_text = m_cr_text[3];
		else if (is_down)
			cr_text = m_cr_text[2];
		else if (m_use_hover && m_bHover)
			cr_text = m_cr_text[1];
		else
			cr_text = m_cr_text[0];
	}
	//글자색과는 달리 배경색은 세팅되면 그리지만
	//세팅되지 않으면 그리지 않는다.
	if (m_cr_back.size() == 4)
	{
		if (is_disabled)
			cr_back = m_cr_back[3];
		else if (is_down)
			cr_back = m_cr_back[2];
		else if (m_use_hover && m_bHover)
			cr_back = m_cr_back[1];
		else
			cr_back = m_cr_back[0];

		dc.FillSolidRect(rc, cr_back);
	}

	//이미지가 있다면 이미지를 먼저 그려주고
	if (m_image.size() > 0 && m_image[idx]->normal != NULL)
	{
		RectF			grect;
		ImageAttributes ia;

		//원본 화소를 거의 유지하지만 일부 화소는 사라짐. 그래서 더 거친 느낌
		//g.SetInterpolationMode(InterpolationModeNearestNeighbor);

		//부드럽게 resize되지만 약간 뿌옇게 변함
		g.SetInterpolationMode(InterpolationModeHighQualityBilinear);

		//중간 느낌
		//g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

		if (m_bAsStatic)
		{
			pImage = &m_image[idx]->normal;
		}
		else
		{
			if (is_disabled)
			{
				pImage = &m_image[idx]->disabled;
			}
			//다운 이미지. 반드시 hovering보다 먼저 체크되어야 한다.
			else if (is_down)
			{
				pImage = (use_disabled_image ? &m_image[idx]->disabled : &m_image[idx]->down);
				pt = m_down_offset;
			}
			//
			else if (m_use_hover && m_bHover)
			{
				pImage = (use_disabled_image ? &m_image[idx]->disabled : &m_image[idx]->over);
			}
			else
			{
				pImage = (use_disabled_image ? &m_image[idx]->disabled : &m_image[idx]->normal);
			}
		}

		if (pImage == NULL)
			pImage = &m_image[idx]->normal;

		//배경을 그리고
		if (!m_back.empty())
			g.DrawImage(m_back, 0, 0);
		//else
			//dc.FillSolidRect(rc, cr_back);

		if (m_use_hover && m_bHover && m_hover_rect)
		{
			Color color;
			color.SetFromCOLORREF(m_hover_rect_color);
			Pen pen(color, m_hover_rect_thick);
			g.DrawRectangle(&pen, Gdiplus::Rect(0, 0, m_width, m_height));
		}

		g.DrawImage(*pImage, pt.x, pt.y, m_width - pt.x * 2, m_height - pt.y * 2);

		if (m_bShowFocusRect)//&& m_bHasFocus)
		{
			//TRACE(_T("draw focus rect\n"));
			//pDC->DrawFocusRect(rc);
			Color	color;

			color.SetFromCOLORREF(m_crFocusRect);
			Pen	pen(color, m_nFocusRectWidth);
			pen.SetDashStyle(DashStyleDot);
			g.DrawRectangle(&pen, rc.left, rc.top, rc.Width(), rc.Height());
		}
	}
	//설정된 이미지가 없는 경우 버튼의 이미지를 그려주고
	//기본 텍스트도 출력한다.
	else
	{
		int		size = 6;
		CRect	r = rc;

		m_text = text;

		if (m_button_style == BS_CHECKBOX)
		{
			r.left += 3;
			r.right = r.left + size * 2 + 1;
			r.top = r.CenterPoint().y - size;
			r.bottom = r.top + size * 2 + 1;
			DrawRectangle(&dc, r, cr_text, RGB(255, 255, 255));

			Pen pen(Color(255, 32, 32, 32), 1.51);

			if (GetCheck())
			{
				g.DrawLine(&pen, r.left + 1, r.CenterPoint().y - 1, r.left + 4, r.CenterPoint().y + 3);
				g.DrawLine(&pen, r.left + 4, r.CenterPoint().y + 3, r.right - 3, r.top + 3);
			}

			rText = r;

			rText.left = rText.right + 4;
			rText.right = rc.right;
		}
		else if (m_button_style == BS_RADIOBUTTON)
		{
			r.left += 3;
			r.right = r.left + size * 2 + 1;
			r.top = r.CenterPoint().y - size;
			r.bottom = r.top + size * 2 + 1;

			Color color;
			color.SetFromCOLORREF(cr_text);
			Pen pen(color, 0.8);
			SolidBrush br(color);
			g.DrawEllipse(&pen, r.left, r.top, r.Width(), r.Height());

			if (GetCheck())
			{
				g.FillEllipse(&br, r.left + r.Width() / 4, r.top + r.Height() / 4, r.Width() / 1.7, r.Height() / 1.7);
			}

			rText = r;
			rText.left = rText.right + 4;
			rText.right = rc.right;
		}
		else
		{
			if (m_b3DRect)
			{
				dc.Draw3dRect(rc,
					is_down ? GRAY160 : white,
					is_down ? white : GRAY128
					//down_state ? GetSysColor(COLOR_3DSHADOW) : GetSysColor(COLOR_3DLIGHT),
					//down_state ? GetSysColor(COLOR_3DLIGHT) : GetSysColor(COLOR_3DSHADOW)
				);
				if (is_down)
					r.OffsetRect(1, 1);
			}

			rText = r;
		}
	}

	if (m_text.IsEmpty())
		return;

	//뭔가 제대로 검사되지 않는다. 우선 푸시버튼만 대상으로 한다.
#if 1
	if (m_button_style == BS_PUSHBUTTON)
	{
		/*
		if ((dwStyle & BS_RIGHT) == BS_RIGHT)
			dwText |= DT_RIGHT;
		else if ((dwStyle & BS_LEFT) == BS_LEFT)
			dwText |= DT_LEFT;
		else//if (dwStyle & BS_CENTER)
		*/
			dwText |= DT_CENTER;
	}
	else
	{
		dwText |= DT_LEFT;
	}
#else		
	MAP_STYLE(BS_LEFT,	 DT_LEFT);
	MAP_STYLE(BS_RIGHT,	 DT_RIGHT);
	MAP_STYLE(BS_CENTER, DT_CENTER);
#endif
	dwText |= (DT_SINGLELINE | DT_VCENTER);

	g.SetSmoothingMode(SmoothingModeAntiAlias);
	CFont *pOldFont = dc.SelectObject(&m_font);

	dc.SetBkMode(TRANSPARENT);

	dc.SetTextColor(cr_text);

	dc.DrawText(m_text, rText, dwText);

	dc.SelectObject(pOldFont);
}


BOOL CGdiButton::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//if (m_pBack)
		return FALSE;
	//else
		return TRUE;

	return CButton::OnEraseBkgnd(pDC);
}


void CGdiButton::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//SendMessage(TTM_TRACKPOSITION, 0, (LPARAM)MAKELPARAM(point.x, point.y));

	if (!m_bHover)//m_bIsTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE;// | TME_HOVER;
		tme.dwHoverTime = 1;
		m_bHover = true;
		m_bIsTracking = _TrackMouseEvent(&tme);
		RedrawWindow();
	}

	CButton::OnMouseMove(nFlags, point);
}


void CGdiButton::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//if (!m_use_hover)
	//	return;

	m_bHover = true;
	Invalidate();

	CButton::OnMouseHover(nFlags, point);
}


void CGdiButton::OnMouseLeave()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//if (!m_use_hover)// || !m_bHover)
	//	return;

	m_bIsTracking = false;
	m_bHover = false;
	UpdateSurface();

	CButton::OnMouseLeave();
}

void CGdiButton::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == TIMER_BLINK)
	{
		m_bBlinkStatus = !m_bBlinkStatus;
		ShowWindow(m_bBlinkStatus ? SW_SHOW : SW_HIDE);

		KillTimer(TIMER_BLINK);

		if (m_bBlinkStatus)
			SetTimer(TIMER_BLINK, m_nBlinkTime1, NULL);
		else
			SetTimer(TIMER_BLINK, m_nBlinkTime0, NULL);
	}

	CButton::OnTimer(nIDEvent);
}


void CGdiButton::OnSetFocus(CWnd* pOldWnd)
{
	CButton::OnSetFocus(pOldWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//TRACE("set focus\n");
	m_bHasFocus = true;
	UpdateSurface();
}


void CGdiButton::OnKillFocus(CWnd* pNewWnd)
{
	CButton::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//TRACE("kill focus\n");
	m_bHasFocus = false;
	UpdateSurface();
}


BOOL CGdiButton::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	return CButton::OnSetCursor(pWnd, nHitTest, message);
}

void CGdiButton::ReAlign()
{
	int dx, dy;
	CRect	rc, rParentRect;

	GetParent()->GetClientRect(&rParentRect);

	if (m_nAnchor > ANCHOR_NONE)
	{
		if (m_nAnchor & ANCHOR_LEFT)
			dx = m_nAnchorMarginX;

		if (m_nAnchor & ANCHOR_TOP)
			dy = m_nAnchorMarginY;

		if (m_nAnchor & ANCHOR_RIGHT)
			dx = rParentRect.right - m_width - m_nAnchorMarginX;

		if (m_nAnchor & ANCHOR_BOTTOM)
			dy = rParentRect.bottom - m_height - m_nAnchorMarginY;

		if (m_nAnchor & ANCHOR_HCENTER)
			dx = rParentRect.CenterPoint().x - m_width / 2;

		if (m_nAnchor & ANCHOR_VCENTER)
			dy = rParentRect.CenterPoint().y - m_height / 2;
	
		SetWindowPos(NULL, dx, dy, m_width, m_height, SWP_NOZORDER | SWP_NOSIZE);
		//SetBackImage(m_pBackOrigin);
	}
}

//버튼의 위치 변경
void CGdiButton::Offset(int x, int y)
{
	CRect	rc;

	//parent의 클라이언트 영역 내에서 이 컨트롤의 좌표를 구한다.
	GetWindowRect(rc);
	CPoint	pt = rc.TopLeft();
	::ScreenToClient(GetParent()->m_hWnd, &pt);

	pt.x += x;
	pt.y += y;

	if (pt.x < 0)
		pt.x = 0;

	if (pt.y < 0)
		pt.y = 0;

	SetWindowPos(&wndNoTopMost, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);	

	//SetBackImage(m_pBackOrigin);
}

void CGdiButton::Inflate(int cx, int cy)
{
	Inflate(cx, cy, cx, cy);
}

//버튼 크기를 늘리거나 줄인다.
//버튼 이미지는 항상 버튼 영역의 중앙에 그려준다.
void CGdiButton::Inflate(int l, int t, int r, int b)
{
	CRect	rc;

	//parent의 클라이언트 영역 내에서 이 컨트롤의 좌표를 구한다.
	GetWindowRect(rc);
	CPoint	pt = rc.TopLeft();
	::ScreenToClient(GetParent()->m_hWnd, &pt);

	rc.MoveToXY(pt.x, pt.y);
	rc.InflateRect(l, t, r, b);

	//배경 그림이 존재했다면 배경 또한 새로 따와야 한다.
	if (m_back.valid() && m_back_origin.valid())
	{
		m_back_origin.deep_copy(&m_back);
		m_back.sub_image(rc);
	}

	SetWindowPos(&wndNoTopMost, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER);	
	UpdateSurface();
}

bool CGdiButton::GetCheck()
{
	return m_idx;
}

void CGdiButton::UpdateSurface(bool bErase)
{
	if (m_transparent)
	{
		CRect rc;

		GetWindowRect(&rc);
		//RedrawWindow();

		GetParent()->ScreenToClient(&rc);
		GetParent()->InvalidateRect(rc, bErase);
		//GetParent()->UpdateWindow();
	}
	else
	{
		Invalidate();
	}

	return;
}

void CGdiButton::Toggle()
{
	m_idx = !m_idx;
	UpdateSurface();
}

LRESULT CGdiButton::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

// 	CString str;
// 	GetWindowText(str);

	//TRACE("%s\n", str);
	//if (str == "Radio3")

/*
	if (message == WM_GETDLGCODE)
		return (DLGC_BUTTON | DLGC_RADIOBUTTON);

	if (message == BM_SETCHECK)
	{
		if (wParam == BST_CHECKED)
		//TRACE("%d, %d, %d\n", message, wParam, lParam);
		TRACE("checked\n");

	 	else if (message == BST_UNCHECKED)
		{
			TRACE("unchecked\n");
		}
	}
*/


	return CButton::WindowProc(message, wParam, lParam);
}

void CGdiButton::SetBlinkTime(int nTime0 /*= 500*/, int nTime1 /*= 500*/)
{
	KillTimer(TIMER_BLINK);

	m_nBlinkTime0		= nTime0;
	m_nBlinkTime1		= nTime1;

	SetBlink(m_bBlink);
}

void CGdiButton::SetBlink(BOOL bBlink /*= TRUE*/)
{
	m_bBlink = bBlink;
	m_bBlinkStatus = FALSE;

	if (m_bBlink)
	{
		SetTimer(TIMER_BLINK, m_nBlinkTime0, NULL);
	}
	else
	{
		m_bBlink = false;
		KillTimer(TIMER_BLINK);
		ShowWindow(SW_SHOW);
		//UpdateSurface();
	}
}

CGdiButton& CGdiButton::SetFontName(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	ReconstructFont();

	return *this;
}

CGdiButton& CGdiButton::SetFontSize(int nSize)
{
	m_lf.lfHeight = -MulDiv(nSize, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	ReconstructFont();

	return *this;
}

CGdiButton& CGdiButton::SetFontBold(bool bBold)
{
	m_lf.lfWeight = (bBold ? FW_BOLD : FW_NORMAL);
	ReconstructFont();

	return *this;
}

//m_bDown과 같은 플래그를 둬서 다운 상태를 판단하게 하는것은
//불편하거나 위험한 코드가 될 수 있다.
//다운 상태에서 마우스가 나가는 경우 등등 체크할 것이 많을수도 있다.
void CGdiButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	UpdateSurface();
	CButton::OnLButtonDown(nFlags, point);
}


void CGdiButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CRect rc;
	GetClientRect(rc);

	//down되면 SetCapture가 자동으로 들어가서 버튼 밖에서 up되었을때도
	//이 함수가 호출된다.
	//밖에서 up되는 경우는 스킵시킨다.
	if (rc.PtInRect(point))
	{
		if (m_button_style == BS_CHECKBOX)
		{
			Toggle();
		}
		else if (m_button_style == BS_RADIOBUTTON)
		{
			SetCheck((m_idx = 1));
		}
	}

	CButton::OnLButtonUp(nFlags, point);
}

void CGdiButton::use_hover(bool use)
{
	m_use_hover = use;
}

void CGdiButton::set_hover_rect(int thick, COLORREF cr)
{
	m_hover_rect = true;
	m_hover_rect_thick = thick;
	m_hover_rect_color = cr;
	Invalidate();
}

void CGdiButton::set_hover_rect_thick(int thick)
{
	m_hover_rect_thick = thick;
	Invalidate();
}

void CGdiButton::set_hover_rect_color(COLORREF cr)
{
	m_hover_rect_color = cr;
	Invalidate();
}

void CGdiButton::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CButton::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	UpdateSurface();
}


void CGdiButton::OnSize(UINT nType, int cx, int cy)
{
	CButton::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_fit2image)
	{
	}
	else
	{
		m_width = cx;
		m_height = cy;
		resize_control(cx, cy);
	}
}

int	CGdiButton::width()
{
	if (m_image.size() == 0)
	{
		CRect rc;
		GetClientRect(rc);
		m_width = rc.Width();
		m_height = rc.Height();
	}

	return m_width;
}

int	CGdiButton::height()
{
	if (m_image.size() == 0)
	{
		CRect rc;
		GetClientRect(rc);
		m_width = rc.Width();
		m_height = rc.Height();
	}

	return m_height;
}

void CGdiButton::replace_color(int index, int state_index, int x, int y, Gdiplus::Color newColor)
{
	if (m_image.size() == 0)
		return;

	int start_index = index;
	int end_index = start_index + 1;

	Gdiplus::Color oldColor;

	if (index < 0)
	{
		start_index = 0;
		end_index = m_image.size();
	}

	for (int i = start_index; i < end_index; i++)
	{
		if (state_index < 0)
		{
			m_image[i]->normal.m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->normal.replace_color(oldColor, newColor);
			m_image[i]->over.m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->over.replace_color(oldColor, newColor);
			m_image[i]->down.m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->down.replace_color(oldColor, newColor);
		}
		else if (state_index == 0)
		{
			m_image[i]->normal.m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->normal.replace_color(oldColor, newColor);
		}
		else if (state_index == 1)
		{
			m_image[i]->over.m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->over.replace_color(oldColor, newColor);
		}
		else if (state_index == 2)
		{
			m_image[i]->down.m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->down.replace_color(oldColor, newColor);
		}
	}

	UpdateSurface();
}

void CGdiButton::apply_effect_hsl(int hue, int sat, int light)
{
	if (m_image.size() == 0)
		return;

	int start_index = 0;
	int end_index = start_index + 1;
	int state_index = 0;

	Gdiplus::Color oldColor;

	if (true)//index < 0)
	{
		start_index = 0;
		end_index = m_image.size();
	}

	for (int i = start_index; i < end_index; i++)
	{
		if (state_index < 0)
		{
			m_image[i]->normal.apply_effect_hsl(hue, sat, light);
			m_image[i]->over.apply_effect_hsl(hue, sat, light);
			m_image[i]->down.apply_effect_hsl(hue, sat, light);
		}
		else if (state_index == 0)
		{
			m_image[i]->normal.apply_effect_hsl(hue, sat, light);
		}
		else if (state_index == 1)
		{
			m_image[i]->over.apply_effect_hsl(hue, sat, light);
		}
		else if (state_index == 2)
		{
			m_image[i]->down.apply_effect_hsl(hue, sat, light);
		}
	}

	UpdateSurface();
}

BOOL CGdiButton::OnToolTipNotify(UINT /*id*/, NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText = "";
	CString rString = "";
	UINT nID = pNMHDR->idFrom;

	ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

	// if there is a top level routing frame then let it handle the message
	if (GetRoutingFrame() != NULL) return FALSE;


	// to be through we will need to handle UNICODE versions of the message also !!
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		UINT nIdentifier = ::GetDlgCtrlID((HWND)pNMHDR->idFrom);

		pTTT->lpszText = NULL;

		//set tooltips for buttons
		/*
		switch (nIdentifier)
		{
		case IDC_STARTALL: // use the resource ID's of control that need text
			pTTT->lpszText = "Start all messages";
			break;

		case IDC_STOPALL:
			pTTT->lpszText = "Stop all messages";
			break;

		case IDC_STARTSELECTED:
			pTTT->lpszText = "Start selected messages";
			break;

		case IDC_STOPSELECTED:
			pTTT->lpszText = "Stop selected messages";
			break;

		case IDC_EDITITEM:
			pTTT->lpszText = "Edit selected item";
			break;

		case IDC_APPLY_ADD:
			GetDlgItem(IDC_APPLY_ADD)->GetWindowText(rString);
			if (rString == "Add Message") pTTT->lpszText = "Add message to list";
			else if (rString == "Apply Changes") pTTT->lpszText = "Apply changes";
			break;
		}
		*/
		if (pTTT->lpszText != NULL)
			return TRUE; // there is text to display
	}

	//set tooltips for toolbar
	if (nID != 0) // will be zero on a separator
	{
		strTipText.LoadString(nID);

#ifndef _UNICODE
		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			lstrcpyn(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
		}
		else
		{
			_mbstowcsz(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
		}
#else
		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			_wcstombsz(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
		}
		else
		{
			lstrcpyn(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
		}
#endif

		// bring the tooltip window above other popup windows
		::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE |
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER); return TRUE;
	}
	return FALSE;
}
