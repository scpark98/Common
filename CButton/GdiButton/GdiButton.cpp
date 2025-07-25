﻿// GdiButton.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "GdiButton.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"
#include "../../GdiPlusBitmap.h"

#pragma warning(disable: 4305)	//'argument': truncation from 'double' to 'Gdiplus::REAL'

// CGdiButton

IMPLEMENT_DYNAMIC(CGdiButton, CButton)

CGdiButton::CGdiButton()
{
	RegisterWindowClass();

	//default text and back color
	//이 기본 색상값은 이미지를 설정하지 않은 경우의 기본값이다.
	//만약 이미지를 설정한다면 clear()시켜야 한다.
	m_cr_text.push_back(RGB2gpColor(::GetSysColor(COLOR_BTNTEXT)));
	m_cr_text.push_back(RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT)));
	m_cr_text.push_back(RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT)));
	//m_cr_text.push_back(RGB2gpColor(::GetSysColor(COLOR_GRAYTEXT)));
	m_cr_text.push_back(Gdiplus::Color(192, 192, 192));

	m_cr_back.push_back(RGB2gpColor(::GetSysColor(COLOR_BTNFACE)));
	m_cr_back.push_back(get_color(m_cr_back[0], 16));
	m_cr_back.push_back(get_color(m_cr_back[0], -16));
	m_cr_back.push_back(RGB2gpColor(::GetSysColor(COLOR_BTNFACE)));

	memset(&m_lf, 0, sizeof(LOGFONT));

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

//동적 생성시에만 사용.
BOOL CGdiButton::create(CString caption, DWORD dwStyle, CRect r, CWnd* parent, UINT button_id)
{
	BOOL res = Create(caption, dwStyle, r, parent, button_id);
	return res;
}

BOOL CGdiButton::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, _T("CGdiButton"), &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = _T("CGdiButton");

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

void CGdiButton::release_all()
{
	m_back_img.release();
	m_back_img_origin.release();

	for (size_t i = 0; i < m_image.size(); i++)
	{
		delete m_image[i];
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
	ON_WM_NCPAINT()
END_MESSAGE_MAP()



// CGdiButton 메시지 처리기입니다.

//기존 CButton::SetButtonStyle 함수를 overriding하여 OWNER_DRAW를 추가시켜줘야 한다.
void CGdiButton::SetButtonStyle(UINT nStyle, BOOL bRedraw)
{
	/*
	DWORD dwStyle = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
	DWORD dwType = CButton::GetButtonStyle();

	CButton::SetButtonStyle(dwType | nStyle, bRedraw);

	// BS_OWNERDRAW 속성을 설정한다.
	//주의 : dwStyle은 반드시 CButton::SetButtonStyle() 함수 호출 전에 구한 값을 사용해야 한다.
	::SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle | BS_OWNERDRAW);
	*/
	ModifyStyle(0, nStyle);
	m_button_type = nStyle;
}

//기본 이미지를 설정할 때 resize한 후 설정
bool CGdiButton::add_image_resize(UINT normal, float ratio)
{
	if (ratio == 1.0f)
		return add_image(normal);

	CGdiButtonImage* btn = new CGdiButtonImage();

	//normal은 0이어서는 안된다.
	if (normal == 0)
		return false;

	btn->img[0].load(_T("PNG"), normal);
	if (btn->img[0].is_empty())
		return false;

	btn->img[0].resize(ratio, ratio);

	btn->img[0].deep_copy(&btn->img[1]);
	btn->img[1].set_matrix(&m_hoverMatrix);

	btn->img[0].deep_copy(&btn->img[2]);
	btn->img[2].set_matrix(&m_downMatrix);

	btn->img[0].deep_copy(&btn->img[3]);
	if (!m_use_normal_image_on_disabled)
		btn->img[3].set_matrix(&m_grayMatrix);

	m_image.push_back(btn);

	m_fit2image = false;

	//이미지를 설정하면 m_cr_back은 clear()시키고 transparent는 true로 세팅되어야 한다.
	//만약 배경색 지정이 필요하다면 add_image()후에 set_back_color()로 세팅한다.
	m_cr_back.clear();
	m_transparent = true;

	return true;
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
	CGdiButtonImage* btn = new CGdiButtonImage();

	//normal은 0이어서는 안된다.
	if (normal == 0)
		return false;

	btn->img[0].load(lpType, normal);
	if (btn->img[0].is_empty())
		return false;

	m_sz_img_origin = CSize(btn->img[0].width, btn->img[0].height);

	if (over > 0)
	{
		btn->img[1].load(lpType, over);
	}
	else
	{
		btn->img[0].deep_copy(&btn->img[1]);
		btn->img[1].set_matrix(&m_hoverMatrix);
	}

	if (down > 0)
	{
		btn->img[2].load(lpType, down);
	}
	else
	{
		btn->img[0].deep_copy(&btn->img[2]);
		btn->img[2].set_matrix(&m_downMatrix);
	}

	if (disabled > 0)
	{
		btn->img[3].load(lpType, disabled);
	}
	else
	{
		btn->img[0].deep_copy(&btn->img[3]);
		if (!m_use_normal_image_on_disabled)
			btn->img[3].set_matrix(&m_grayMatrix);
	}


	m_image.push_back(btn);

	//이미지를 설정하면 m_cr_back은 clear()시키고 transparent는 true로 세팅되어야 한다.
	//만약 배경색 지정이 필요하다면 add_image()후에 set_back_color()로 세팅한다.
	m_cr_back.clear();
	m_transparent = true;

	return true;
}

bool CGdiButton::add_image(CGdiplusBitmap *img)
{
	//m_image라는 list에 push_back하여 유지시키기 위해서는
	//반드시 동적 할당받은 인스턴스를 넣어줘야 한다.
	//그렇지 않으면 어느 시점에서 소멸되어 잘못된 참조가 된다.
	CGdiButtonImage* btn = new CGdiButtonImage();

	//normal은 0이어서는 안된다.
	if (img->is_empty())
		return false;

	img->deep_copy(&btn->img[0]);
	if (btn->img[0].is_empty())
		return false;

	//m_width = btn->img[0].width;
	//m_height = btn->img[0].height;

	btn->img[0].deep_copy(&btn->img[1]);
	btn->img[1].set_matrix(&m_hoverMatrix);

	btn->img[0].deep_copy(&btn->img[2]);
	btn->img[2].set_matrix(&m_downMatrix);

	btn->img[0].deep_copy(&btn->img[3]);
	if (!m_use_normal_image_on_disabled)
		btn->img[3].set_matrix(&m_grayMatrix);

	m_image.push_back(btn);

	//fit_to_image(m_fit2image);

	//이미지를 설정하면 m_cr_back은 clear()시키고 transparent는 true로 세팅되어야 한다.
	//만약 배경색 지정이 필요하다면 add_image()후에 set_back_color()로 세팅한다.
	m_cr_back.clear();
	m_transparent = true;

	return true;
}

void CGdiButton::use_normal_image_on_disabled(bool use)
{
	m_use_normal_image_on_disabled = use;

	for (size_t i = 0; i < m_image.size(); i++)
	{
		if (m_image[i]->img[0])
		{
			//m_image[i]->img[0].save(_T("d:\\normal_before.png"));
			m_image[i]->img[0].deep_copy(&m_image[i]->img[3]);
			//m_image[i]->img[0].save(_T("d:\\normal_after.png"));
			//m_image[i]->img[3].save(_T("d:\\disabled.png"));

			if (!m_use_normal_image_on_disabled)
				m_image[i]->img[3].set_matrix(&m_grayMatrix);
		}
	}
}

CSize CGdiButton::get_img_size(int index)
{
	if (index < 0 || index >= m_image.size())
		return CSize();

	return m_image[index]->img[0].size();
}

void CGdiButton::fit_to_image(bool fit)
{
	if (m_image.size() == 0)
		return;

	m_fit2image = fit;

	if (m_fit2image)
	{
		SetWindowPos(NULL, 0, 0, m_sz_img_origin.cx, m_sz_img_origin.cy, SWP_NOMOVE | SWP_NOZORDER);
	}
	else
	{
		for (auto img : m_image)
		{
			for (int i = 0; i < 4; i++)
				img->img[i].resize(m_rOrigin.Width(), m_rOrigin.Height());
		}
	}

//	resize_control(rc.Width(), rc.Height());

	redraw_window();
}

void CGdiButton::active_index(int index, bool bErase)
{
	if (index < 0 || index >= m_image.size())
		return;

	m_idx = index;
	redraw_window();
}

//true이면 1번 이미지를 표시해준다. false일 경우는 0번을 표시하는데 만약 없으면 AddGrayImage로 회색 이미지를 자동 추가한 후 선택해준다.
//기존에는 AddImage에서 checkbox나 radiobutton이면 알아서 grayimage를 추가해줬으나 아래와 같은 문제가 있어서
//자동 추가는 삭제되었다.
//문제1. AddImage단계에서는 추가적인 AddImage가 있을지 없을지 알 수 없으므로 이 단계에서 gray image를 넣는 것은 문제가 된다.
//문제2. GetButtonStyle()로 버튼 타입이 잘 구분되지 않는다. 뭔가 명확한 방법을 찾아야 한다.
//check나 radio의 on, off를 하나의 이미지로만 간단히 사용한다면
//on이미지를 추가한 후 AddGrayImage()를 추가로 호출하여 사용하자.
//일반적인 2state 버튼은 BST_UNCHECKED(0), BST_CHECKED(1)을 가지지만 3state에서는 BST_INDETERMINATE(2)를 가질 수 있다.
void CGdiButton::SetCheck(int check_state)
{
	m_idx = check_state;
	redraw_window();

	//radio 버튼이 눌려지거나 SetCheck(true)가 호출되면
	//같은 group 내의 다른 버튼들은 unchecked로 만들어 줘야한다.
	//owner draw 속성때문에 WindowProc의 윈도우 기본 메시지를 받아서 처리할 수 없다.
	if (is_button_style(BS_RADIOBUTTON, BS_AUTORADIOBUTTON) && check_state)
	{
		CWnd *pWndParent = GetParent(); 
		CWnd *pWnd = pWndParent->GetNextDlgGroupItem(this); 
		CWnd* pWndFirstCheck = NULL;

		//만약 버튼이 visible이 false이면 아래 while문은 무한루프를 돈다.
		//이를 방지하기 위해 한번 검사한 윈도우를 기억했다가 그 윈도우를 만나면 종료시킨다.
		while (pWnd && (pWnd != this) && (pWnd != pWndFirstCheck)) 
		{ 
			if (pWnd->GetStyle() & WS_GROUP)
			{
				//TRACE(_T("%p : group property\n"), pWnd);
			}
			else
			{
				//TRACE(_T("%p : no group property\n"), pWnd);
			}

			if (pWnd->IsKindOf(RUNTIME_CLASS(CGdiButton))) 
			{ 
				if (((CGdiButton*)pWnd)->is_button_style(BS_RADIOBUTTON, BS_AUTORADIOBUTTON))
				{
					//((CGdiButton*)pWnd)->SetCheck(BST_UNCHECKED); 
					((CGdiButton*)pWnd)->m_idx = 0;
					((CGdiButton*)pWnd)->redraw_window();
				}
			} 

			if (pWndFirstCheck == NULL)
				pWndFirstCheck = pWnd;

			pWnd = pWndParent->GetNextDlgGroupItem(pWnd); 
		}
	}
}
/*
void CGdiButton::set_back_imageBitmap* pBack)
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

	update_surface();
}
*/

//0(transparent) ~ 255(opaque)
void CGdiButton::set_alpha(int alpha)
{
	for (size_t i = 0; i < m_image.size(); i++)
	{
		m_image[i]->img[0].set_alpha(alpha);
		m_image[i]->img[1].set_alpha(alpha);
		m_image[i]->img[2].set_alpha(alpha);
		m_image[i]->img[3].set_alpha(alpha);
	}
}
/*
void CGdiButton::add_rgb(int red, int green, int blue, Gdiplus::Color crExcept)
{
	for (size_t i = 0; i < m_image.size(); i++)
	{
		m_image[i]->img[0].add_rgb_loop(red, green, blue, GRAY(50));
		//m_image[i]->over.set_alpha(alpha);
		//m_image[i]->down.set_alpha(alpha);
		//m_image[i]->disabled.set_alpha(alpha);
	}
	//Invalidate();
	redraw_window();
}
*/
void CGdiButton::set_transparent(bool trans, Gdiplus::Color cr_parent_back)
{
	m_transparent = trans;

	if (cr_parent_back.GetValue() != Gdiplus::Color::Transparent)
		m_cr_parent_back = cr_parent_back;

	//radiobutton 또는 checkbox가 m_img도 없을 경우는 배경이 투명해야 하므로 그때만 m_cr_back을 clear()한다.
	if (m_image.size() == 0 && (is_button_style(BS_CHECKBOX, BS_AUTOCHECKBOX) || (is_button_style(BS_RADIOBUTTON, BS_AUTORADIOBUTTON))))
		m_cr_back.clear();

	redraw_window();
}

void CGdiButton::set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back)
{
	set_text_color(cr_text);// , cr_text, normal, gray_color(normal));
	set_back_color(cr_back);//
}

void CGdiButton::set_text_color(Gdiplus::Color normal)
{
	set_text_color(normal, normal, normal, gray_color(normal));
}

void CGdiButton::set_text_color(Gdiplus::Color normal, Gdiplus::Color over, Gdiplus::Color down, Gdiplus::Color disabled)
{
	m_cr_text.clear();

	m_cr_text.push_back(normal);
	m_cr_text.push_back(over);
	m_cr_text.push_back(down);
	m_cr_text.push_back(disabled);

	redraw_window();
}

void CGdiButton::set_back_color(Gdiplus::Color normal, bool auto_set_color)
{
	//normal 색상에 따라 16이라는 offset이 크거나 작게 느껴진다.
	if (auto_set_color)
		set_back_color(normal, get_color(normal, 16), get_color(normal, -16), gray_color(normal));
	else
		set_back_color(normal, normal, normal, gray_color(normal));
}

void CGdiButton::set_back_color(Gdiplus::Color normal, Gdiplus::Color over, Gdiplus::Color down, Gdiplus::Color disabled)
{
	//배경색을 설정하면 배경 이미지는 해제시킨다.
	//단, round가 들어간다면 투명처리해야 한다.
	if (m_round <= 0)
		m_transparent = false;

	m_back_img.release();
	m_back_img_origin.release();

	m_cr_back.clear();
	m_cr_back.push_back(normal);
	m_cr_back.push_back(over);
	m_cr_back.push_back(down);
	m_cr_back.push_back(disabled);

	redraw_window();
}

void CGdiButton::set_hover_back_color(Gdiplus::Color hover_back)
{
	if (m_cr_back.size() < 2)
		return;

	m_cr_back[1] = hover_back;
}

void CGdiButton::set_hover_color_matrix(float fScale)	//1.0f = no effect.
{
	m_hoverMatrix.m[0][0] = fScale;
	m_hoverMatrix.m[1][1] = fScale;
	m_hoverMatrix.m[2][2] = fScale;

	if (m_image.size())
		m_image[0]->img[1].set_matrix(&m_hoverMatrix);
}

void CGdiButton::set_down_color_matrix(float fScale)	//1.0f = no effect.
{
	m_downMatrix.m[0][0] = fScale;
	m_downMatrix.m[1][1] = fScale;
	m_downMatrix.m[2][2] = fScale;

	if (m_image.size())
		m_image[0]->img[2].set_matrix(&m_downMatrix);
}

//이미지 및 버튼의 크기를 조정한다. image_only = true이면 이미지의 크기만 조정할 뿐 버튼의 크기는 조정하지 않는다.
void CGdiButton::resize(bool image_only, int cx, int cy)
{
	for (int i = 0; i < m_image.size(); i++)
	{
		m_image[i]->img[0].resize(cx, cy);
		m_image[i]->img[1].resize(cx, cy);
		m_image[i]->img[2].resize(cx, cy);
		m_image[i]->img[3].resize(cx, cy);
	}

	if (!image_only)
		SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);
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
	SetWindowPos(NULL, 0, 0, rc.Width(), rc.Height(), SWP_NOMOVE | SWP_NOZORDER);
}

//이미지를 사용하지 않고 직접 그려주는 버튼의 경우 width를 정확히 구해야하는 경우가 있다.
CRect CGdiButton::calc_rect()
{
	CRect rc;
	CClientDC dc(this);
	CString text;

	GetClientRect(rc);
	GetWindowText(text);

	CFont* pOldFont = (CFont*)dc.SelectObject(&m_font);
	CSize sz = dc.GetTextExtent(text);
	dc.SelectObject(pOldFont);
	//dc.DrawText(text, &rText, DT_LEFT | DT_CALCRECT);

	int total_width = 2;	//left margin

	if (is_button_style(BS_CHECKBOX, BS_AUTOCHECKBOX) ||
		is_button_style(BS_RADIOBUTTON, BS_AUTORADIOBUTTON))
	{
		if (is_button_style(BS_PUSHLIKE))
			return CRect(2, 0, 2 + sz.cx + 2, sz.cy);

		total_width += (6 * 2 + 1);		//check or radio image
		total_width += 4;				//the gap of check or radio image and text
		total_width += sz.cx;			//text extend
		total_width += 2;				//right margin
	}

	return CRect(2, 0, 2 + total_width + 2, sz.cy);
}


void CGdiButton::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	// Set control to owner draw
	m_button_style = GetButtonStyle();
	
	m_button_type = m_button_style & BS_TYPEMASK;
	
	//버튼 타입을 같은 종끼리 일단 그룹화한다.
	if ((m_button_type & BS_AUTORADIOBUTTON) == BS_AUTORADIOBUTTON)
	{
		m_button_type = BS_RADIOBUTTON;
	}
	else if ((m_button_type & BS_AUTOCHECKBOX) == BS_AUTOCHECKBOX ||
			 (m_button_type & BS_3STATE) == BS_3STATE ||
			 (m_button_type & BS_AUTO3STATE) == BS_AUTO3STATE)
	{
		m_button_type = BS_CHECKBOX;
	}

	if ((m_button_style & BS_PUSHLIKE) == BS_PUSHLIKE)
	{
		TRACE(_T("BS_PUSHLIKE\n"));
	}
	if ((m_button_style & BS_MULTILINE) == BS_MULTILINE)
	{
		TRACE(_T("BS_MULTILINE\n"));
	}


	if ((m_button_type & BS_CHECKBOX) == BS_CHECKBOX)
	{
		//TRACE(_T("BS_CHECKBOX\n"));
	}
	if ((m_button_type & BS_AUTO3STATE) == BS_AUTO3STATE)
	{
		//TRACE(_T("BS_PUSHLIKE\n"));
	}

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

	reconstruct_font();

	CButton::PreSubclassWindow();
}


void CGdiButton::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	//SetFont(&m_font, true);

	ASSERT(bCreated);
}


BOOL CGdiButton::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
			case VK_SPACE :
				redraw_window();
				Wait(50);
				::SendMessage(GetParent()->m_hWnd, Message_CGdiButton, (WPARAM) & (CGdiButtonMessage(this, GetDlgCtrlID(), WM_LBUTTONDOWN)), 0);
				break;
		}
	}
	else if (pMsg->message == WM_KEYUP)
	{
		switch (pMsg->wParam)
		{
			case VK_SPACE:
				redraw_window();
				Wait(50);
				::SendMessage(GetParent()->m_hWnd, Message_CGdiButton, (WPARAM) & (CGdiButtonMessage(this, GetDlgCtrlID(), WM_LBUTTONUP)), 0);
				break;
		}
	}

	return CButton::PreTranslateMessage(pMsg);
}

//#define MAP_STYLE(src, dest) if(dwStyle & (src)) dwText |= (dest)

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
	Gdiplus::Color	cr_text = ::GetSysColor(COLOR_BTNTEXT);
	Gdiplus::Color	cr_back = ::GetSysColor(COLOR_3DFACE);
	DWORD		dwStyle = GetStyle();
	DWORD		dwText = 0;

	GetClientRect(rc);
	GetWindowText(text);
	
	//for test
	if (text == _T("진단 시작"))
		text = _T("진단 시작");

	rText = rc;

	CMemoryDC				dc(pDC1, &rc);
	Gdiplus::Graphics		g(dc.m_hDC, rc);
	Gdiplus::GraphicsPath	roundPath;

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	get_round_rect_path(&roundPath, CRect2GpRect(rc), m_round);

	bool is_down = lpDIS->itemState & ODS_SELECTED;
	bool is_disabled = (lpDIS->itemState & ODS_DISABLED);

	//TRACE(_T("%s\n"), __function__);

	int idx = MIN(m_idx, m_image.size()-1);
	if (idx < 0)
		return;


	//만약 parent에 배경색이나 배경 그림이 있고
	//그려지는 이미지가 배경이 투명한 PNG라면 투명하게 그리기 위해.
	//
	if (m_transparent)
	{
		//버튼의 위치를 얻어온다.
		CRect rbutton;
		GetWindowRect(&rbutton);
		CWnd* pParent = GetParent();
		ASSERT(pParent);
		pParent->ScreenToClient(&rbutton);  //convert our corrdinates to our parents

		//투명 png button일 경우 또는 round 버튼일 경우는 그 배경이 parent의 배경 이미지 또는 parent의 배경색으로 먼저 칠해져야 한다.
		if (m_img_parent != NULL && m_img_parent->is_valid())
		{
		}
		else if (m_image.size() > 0)
		{
			//dc.FillSolidRect(rc, m_cr_parent_back.ToCOLORREF());
			CDC* pDC = pParent->GetDC();
			CDC MemDC;
			CBitmap bmp;
			MemDC.CreateCompatibleDC(pDC);
			bmp.CreateCompatibleBitmap(pDC, rbutton.Width(), rbutton.Height());
			CBitmap* pOldBmp = MemDC.SelectObject(&bmp);
			MemDC.BitBlt(0, 0, rbutton.Width(), rbutton.Height(), pDC, rbutton.left, rbutton.top, SRCCOPY);
			dc.BitBlt(0, 0, rbutton.Width(), rbutton.Height(), &MemDC, 0, 0, SRCCOPY);
			MemDC.SelectObject(pOldBmp);
			pParent->ReleaseDC(pDC);
			MemDC.DeleteDC();
		}
		else if (m_round > 0)
		{
			dc.FillSolidRect(rc, m_cr_parent_back.ToCOLORREF());
		}
		/*
		cr_back = Gdiplus::Color::Transparent;

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
#ifdef _DEBUG
		save(CGdiplusBitmap((HBITMAP)bmp.GetSafeHandle()), _T("d:\\parent_bmp.bmp"));
#endif
		MemDC.SelectObject(pOldBmp);
		pParent->ReleaseDC(pDC);
		MemDC.DeleteDC();
		*/
	}

	//check or radio인데 m_image.size()가 1뿐이라면
	//checked = normal을, unchecked = disabled를 표시한다.
	bool use_disabled_image = false;
	if (!is_button_style(BS_PUSHBUTTON, BS_DEFPUSHBUTTON) && m_image.size() == 1 && m_idx == 0)
		use_disabled_image = true;

	if (m_cr_text.size() == 4)
	{
		if (is_disabled)
			cr_text = m_cr_text[3];
		else if (is_down)
			cr_text = m_cr_text[2];
		else if (m_use_hover && m_is_hover)
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
		else if (m_use_hover && m_is_hover)
			cr_back = m_cr_back[1];
		else
			cr_back = m_cr_back[0];

		if (m_round == 0)
		{
			dc.FillSolidRect(rc, cr_back.ToCOLORREF());
		}
		else
		{
			Gdiplus::SolidBrush brush(cr_back);
			g.FillPath(&brush, &roundPath);
		}
	}

	//배경 이미지가 지정되어 있다면 배경 이미지를 그리고 없다면 배경색으로 칠한다.
	if (!m_transparent)
	{
		if (m_back_img.is_valid())
			g.DrawImage(m_back_img, 0, 0);
		else if (m_cr_back.size())
			dc.FillSolidRect(rc, cr_back.ToCOLORREF());
	}

	//이미지가 있다면 이미지를 먼저 그려주고
	if (m_image.size() > 0 && m_image[idx]->img[0] != NULL)
	{
		Gdiplus::RectF				grect;
		Gdiplus::ImageAttributes	ia;

		//원본 화소를 거의 유지하지만 일부 화소는 사라짐. 그래서 더 거친 느낌
		//g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);

		//부드럽게 resize되지만 약간 뿌옇게 변함
		g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);

		//중간 느낌
		//g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

		//이미지를 그리기 전에 shadow를 먼저 그려준다.
		if (m_draw_drop_shadow)
		{
			//m_down_offset = CPoint(1, 1);
			CGdiplusBitmap img_shadow;
			m_image[idx]->img[0].deep_copy(&img_shadow);
#ifdef _DEBUG
			img_shadow.save(_T("d:\\0.origin.png"));
#endif
			//img_shadow.resize(rc.Width(), rc.Height());
			//img_shadow.blur(20, TRUE);
			img_shadow.blur(m_drop_shadow_blur_sigma);
#ifdef _DEBUG
			img_shadow.save(_T("d:\\1.blur.png"));
#endif
			img_shadow.gray(m_drop_shadow_weight);
			//img_shadow.apply_effect_rgba(0.4f, 0.4f, 0.4f);
#ifdef _DEBUG
			img_shadow.save(_T("d:\\2.gray.png"));
#endif
			CRect rc_shadow = rc;
			rc_shadow.OffsetRect(2, 2);
			img_shadow.draw(g, rc_shadow, CGdiplusBitmap::draw_mode_zoom);
		}
		else
		{
			//m_down_offset = CPoint(1, 3);
		}

		if (m_bAsStatic)
		{
			pImage = &m_image[idx]->img[0];
		}
		else
		{
			if (is_disabled)
			{
				pImage = &m_image[idx]->img[3];
			}
			//다운 이미지. 반드시 hovering보다 먼저 체크되어야 한다.
			else if (is_down)
			{
				pImage = (use_disabled_image ? &m_image[idx]->img[3] : &m_image[idx]->img[2]);
				pt = m_down_offset;
			}
			//
			else if (m_use_hover && m_is_hover)
			{
				pImage = (use_disabled_image ? &m_image[idx]->img[3] : &m_image[idx]->img[1]);
			}
			else
			{
				pImage = (use_disabled_image ? &m_image[idx]->img[3] : &m_image[idx]->img[0]);
			}
		}

		if (pImage == NULL)
			pImage = &m_image[idx]->img[0];

		//down 작은 크기로 shrink시키면서 그릴 경우
		//g.DrawImage(*pImage, pt.x, pt.y, rc.Width() - pt.x * 2, rc.Height() - pt.y * 2);

		//down 시 입체적으로 눌리는 느낌(위치가 변경되는 느낌)을 주려면 set_down_offset(1, 1)과 같이 줘야 한다.
		//g.DrawImage(*pImage, pt.x, pt.y, rc.Width(), rc.Height());


		//down 효과없이 그릴 경우
		pImage->draw(g, rc, CGdiplusBitmap::draw_mode_origin);
	}
	//설정된 이미지가 없는 경우 버튼의 이미지를 그려주고
	//기본 텍스트도 출력한다.
	else
	{
		int		size = 6;
		CRect	r = rc;

		if (is_button_style(BS_CHECKBOX, BS_AUTOCHECKBOX))
		{
			r.left += 2;
			r.right = r.left + size * 2 + 1;
			r.top = r.CenterPoint().y - size - 1;
			r.bottom = r.top + size * 2 + 1;
			draw_rectangle(&dc, r, cr_text, Gdiplus::Color::White);

			Gdiplus::Pen pen(Gdiplus::Color(255, 32, 32, 32), 1.51);

			int check_state = GetCheck();

			if (check_state == BST_CHECKED)
			{
				g.DrawLine(&pen, r.left + 1, r.CenterPoint().y - 1, r.left + 4, r.CenterPoint().y + 3);
				g.DrawLine(&pen, r.left + 4, r.CenterPoint().y + 3, r.right - 3, r.top + 3);
			}
			else if (check_state == BST_INDETERMINATE)
			{
				CRect inner = r;
				inner.DeflateRect(3, 3);
				draw_rectangle(&dc, inner, cr_text, cr_text);
			}

			rText = r;

			rText.left = rText.right + 4;
			rText.right = rc.right;
			//rText.bottom -= 1;
		}
		else if (is_button_style(BS_RADIOBUTTON, BS_AUTORADIOBUTTON))
		{
			if (is_button_style(BS_PUSHLIKE))
			{
				if (!is_button_style(BS_FLAT))
				{
					dc.Draw3dRect(rc,
						GetCheck() ? GRAY160 : white,
						GetCheck() ? white : GRAY128);
				}
			}
			else
			{
				//dc.DrawFrameControl(r, DFC_BUTTON, DFCS_BUTTONRADIO);
				r.left += 2;
				r.right = r.left + size * 2 + 0;
				r.top = r.CenterPoint().y - size - 0;
				r.bottom = r.top + size * 2 + 0;

				Gdiplus::Pen pen(cr_text, 1.0);
				Gdiplus::SolidBrush br(cr_text);
				g.DrawEllipse(&pen, r.left, r.top, r.Width(), r.Height());

				if (GetCheck())
				{
					r.DeflateRect(2, 2);
					g.FillEllipse(&br,
						(Gdiplus::REAL)(r.left) + 0.2f,
						(Gdiplus::REAL)(r.top) + 0.2f,
						(Gdiplus::REAL)(r.Width()) - 0.4f,
						(Gdiplus::REAL)(r.Height()) - 0.4f);
					r.InflateRect(2, 2);
				}

				rText = r;
				rText.left = rText.right + 4;
				rText.right = rc.right;
				rText.bottom += 1;
			}
		}
		else
		{
			if (m_round == 0 && m_b3DRect && !is_button_style(BS_FLAT))
			{
				dc.Draw3dRect(rc,
					is_down ? GRAY160 : white,
					is_down ? white : GRAY128
					//down_state ? GetSysColor(COLOR_3DSHADOW) : GetSysColor(COLOR_3DLIGHT),
					//down_state ? GetSysColor(COLOR_3DLIGHT) : GetSysColor(COLOR_3DSHADOW)
				);
				//if (is_down)
				//	r.OffsetRect(1, 1);
			}
			//SC_HELP, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE는 텍스트는 없지만 도형으로 그려준다.
			else if (m_button_cmd > 0)
			{
				Gdiplus::Pen pen(cr_text, 1.5F);
				Gdiplus::Pen pen_pin(Gdiplus::Color(45, 122, 190), 17.0F);
				Gdiplus::SolidBrush br_back(cr_back);
				Gdiplus::SolidBrush br_pin(Gdiplus::Color(255, 255, 255, 255));

				CPoint cp = rc.CenterPoint();

				switch (m_button_cmd)
				{
					case SC_MINIMIZE :
						g.DrawLine(&pen, cp.x - 5, cp.y + 3, cp.x + 5, cp.y + 3);
						break;
					case SC_MAXIMIZE :
						if (GetParent()->IsZoomed())
						{
							g.DrawRectangle(&pen, cp.x - 3, cp.y - 5, 8, 8);
							g.FillRectangle(&br_back, cp.x - 6, cp.y - 2, 8, 8);
							g.DrawRectangle(&pen, cp.x - 6, cp.y - 2, 8, 8);
						}
						else
						{
							g.DrawRectangle(&pen, cp.x - 5, cp.y - 6, 10, 10);
						}
						break;
					case SC_CLOSE:
						g.DrawLine(&pen, cp.x - 5, cp.y - 6, cp.x + 5, cp.y + 4);
						g.DrawLine(&pen, cp.x - 5, cp.y + 4, cp.x + 5, cp.y - 6);
						break;
					//case SC_PIN :
					//	g.DrawLine(&pen_pin, cp.x - 7, cp.y, cp.x + 7, cp.y);
					//	g.FillEllipse(&br_pin, cp.x + (is_top_most(GetParent()->GetSafeHwnd()) ? 1 : -11), cp.y - 5, 10, 10);
					//	break;
					//case SC_HELP :
					//	draw_text(g, rc, _T("?"), 12, 0, 1, 1.0f, _T("맑은 고딕"), cr_text);// , m_theme.cr_text, DT_CENTER | DT_VCENTER | DT_SINGELINE);
					//	break;
				}
			}

			rText = r;
		}
	}

	//focus_rect, border를 그려준다.
	//if (((lpDIS->itemAction & ODA_FOCUS) || (lpDIS->itemAction & ODA_DRAWENTIRE)) && (lpDIS->itemState & ODS_FOCUS))	//ok
	//if (lpDIS->itemState & ODS_FOCUS)	//ok
	//if (GetFocus() == this)	//ok
	if (m_draw_focus_rect && m_bHasFocus)	//옵션처리하기 위해 이 코드를 사용함
	{
		CRect rfocus = rc;
		rfocus.DeflateRect(1, 1, 2, 2);
		//TRACE(_T("draw focus rect\n"));
		//dc.DrawFocusRect(rfocus);
		Gdiplus::Pen	pen(m_cr_focus_rect, (Gdiplus::REAL)m_focus_rect_width);
		pen.SetDashStyle(Gdiplus::DashStyleDot);
		if (m_round <= 0)
			g.DrawRectangle(&pen, rc.left, rc.top, rc.Width(), rc.Height());
		else
			draw_round_rect(&g, CRect2GpRect(rfocus), m_cr_focus_rect, Gdiplus::Color::Transparent, m_round, m_focus_rect_width);
	}
	else if (m_use_hover && m_draw_hover_rect && m_is_hover)
	{
		if (m_round > 0)
			draw_round_rect(&g, CRect2GpRect(rc), m_hover_rect_color, Gdiplus::Color::Transparent, m_round, m_hover_rect_thick);
		else
			draw_rectangle(g, rc, m_cr_border);
	}
	else if (m_draw_border)// && !m_is_hover)
	{
		TRACE(_T("draw_border\n"));
		if (m_round > 0)
			//draw_round_rect(&g, CRect2GpRect(rc), m_cr_border, Gdiplus::Color::Transparent, m_round, m_border_thick);
			//draw_round_rect(&g, CRect2GpRect(rc), Gdiplus::Color::Red, Gdiplus::Color::Transparent, m_round, m_border_thick);
			g.DrawPath(&Gdiplus::Pen(m_cr_border), &roundPath);
		else
			draw_rectangle(g, rc, m_cr_border);
	}



	//이미지 버튼이면 텍스트는 출력하지 않는다.
	if (!m_draw_own_text && (m_image.size() > 0 || text.IsEmpty()))
		return;

	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	CFont* pOldFont = dc.SelectObject(&m_font);

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(cr_text.ToCOLORREF());

	//align은 resource editor에서 설정한 속성을 그대로 반영하고자 했으나 정확히 얻어오지 못한다.
	//우선 기본값으로 pushbutton은 DT_CENTER, checkbox와 radio는 DT_LEFT를 기본값으로 한다.
#if 1
	//dwStyle = GetStyle();
	//if (dwStyle & BS_CENTER == BS_CENTER)
	//	dwText = DT_CENTER;
	//else if (dwStyle & BS_RIGHT == BS_RIGHT)
	//	dwText = DT_RIGHT;
	//else
	//	dwText = DT_LEFT;

	//TRACE(_T("%s = %d\n"), text, dwText);


	if (is_button_style(BS_PUSHBUTTON, BS_DEFPUSHBUTTON) || is_button_style(BS_PUSHLIKE))
		dwText = DT_CENTER;
	else if (is_button_style(BS_CHECKBOX, BS_AUTOCHECKBOX) || (is_button_style(BS_RADIOBUTTON, BS_AUTORADIOBUTTON)))
		dwText = DT_LEFT;

	//{
	//	//dwStyle = GetButtonStyle();
	//	LONG_PTR style = GetWindowLong(m_hWnd, GWL_STYLE);
	//	if (style & BS_RIGHT)
	//		dwText = DT_RIGHT;
	//	else if (style & BS_LEFT)
	//		dwText = DT_LEFT;
	//	else//if (dwStyle & BS_CENTER)
	//		dwText = DT_CENTER;
	//}
	////else
	////{
	////	dwText |= DT_LEFT;
	////}
#else		
	MAP_STYLE(BS_LEFT,	 DT_LEFT);
	MAP_STYLE(BS_CENTER, DT_CENTER);
	MAP_STYLE(BS_RIGHT,	 DT_RIGHT);
#endif
	dwText |= (DT_SINGLELINE | DT_VCENTER);

	CSize sz_text = dc.GetTextExtent(text);

	//m_img_header 유무에 따라 텍스트 정렬도 재정의된다.
	if (m_img_header.is_valid())
	{
		int x, y;
		int gap = m_img_header_gap;	//header image와 text사이의 간격. 이미지의 여백 정도에 따라 다르게 표시될 수 있다. 이미지의 여백까지 검사해서 정해져야 정확하다.
		int margin = 8;	//왼쪽 또는 오른쪽 마진
		int img_height = (float)rc.Height() * m_img_header_ratio;
		int img_width = (double)m_img_header.height * (double)img_height / (double)m_img_header.width;

		if (m_img_header_align & DT_CENTER)
			x = (rc.Width() - (img_width + gap + sz_text.cx)) / 2;
		else if (m_img_header_align & DT_RIGHT)
			x = rc.right - margin - img_width - gap - img_width;
		else
			x = rc.left + gap;

		if (m_img_header_align & DT_VCENTER)
			y = (rc.Height() - img_height) / 2;
		else if (m_img_header_align & DT_BOTTOM)
			y = rc.bottom - 4 - img_height;
		else
			y = rc.top + 4;


		m_img_header.draw(g, x, y, img_width, img_height);
		rText.MoveToX(x + img_width + gap);
		dwText = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
	}

//#ifdef _UNICODE
//	DrawShadowText(dc.GetSafeHdc(), text, text.GetLength(), rText,
//		DT_CENTER | DT_TOP | DT_NOCLIP, cr_text.ToCOLORREF(), 0, 2, 1);
//#else
	dc.DrawText(text, rText, dwText);
//#endif
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
	//TRACE(_T("mouse move\n"));
	if (m_use_hover && !m_is_hover)//m_bIsTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE | TME_HOVER;
		tme.dwHoverTime = 1;
		m_is_hover = true;
		m_bIsTracking = _TrackMouseEvent(&tme);
		RedrawWindow();
	}

	CButton::OnMouseMove(nFlags, point);
}


void CGdiButton::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//TRACE(_T("mouse hover\n"));

	if (!m_use_hover)
		return;

	m_is_hover = true;
	redraw_window();

	//TRACE(_T("hover\n"));
	//::PostMessage()로 전달하면 쓰레기값이 전달된다.
	::SendMessage(GetParent()->m_hWnd, Message_CGdiButton, (WPARAM)&(CGdiButtonMessage(this, GetDlgCtrlID(), WM_MOUSEHOVER)), 0);

	CButton::OnMouseHover(nFlags, point);
}


void CGdiButton::OnMouseLeave()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//TRACE(_T("mouse leave\n"));

	if (!m_use_hover || !m_is_hover)
		return;

	m_bIsTracking = false;
	m_is_hover = false;
	redraw_window();

	//TRACE(_T("leave\n"));
	//::PostMessage()로 전달하면 쓰레기값이 전달된다.
	::SendMessage(GetParent()->m_hWnd, Message_CGdiButton, (WPARAM)&(CGdiButtonMessage(this, GetDlgCtrlID(), WM_MOUSELEAVE)), 0);

	CButton::OnMouseLeave();
}

void CGdiButton::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_blink)
	{
		m_blink_status = !m_blink_status;
		ShowWindow(m_blink_status ? SW_SHOW : SW_HIDE);

		KillTimer(timer_blink);

		if (m_blink_status)
			SetTimer(timer_blink, m_blink_time1, NULL);
		else
			SetTimer(timer_blink, m_blink_time0, NULL);
	}
	else if (nIDEvent == timer_auto_repeat)
	{
		if ((GetState() & BST_PUSHED) == 0)
			return;

		SetTimer(timer_auto_repeat, m_repeat_delay, NULL);
		GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
		m_sent_once_auto_repeat_click_message++;
	}


	CButton::OnTimer(nIDEvent);
}


void CGdiButton::OnSetFocus(CWnd* pOldWnd)
{
	CButton::OnSetFocus(pOldWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//TRACE("set focus\n");
	m_bHasFocus = true;
	redraw_window();
}


void CGdiButton::OnKillFocus(CWnd* pNewWnd)
{
	CButton::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//TRACE("kill focus\n");
	m_bHasFocus = false;
	redraw_window();
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
			//dx = rParentRect.right - m_width - m_nAnchorMarginX;
			dx = rParentRect.right - width() - m_nAnchorMarginX;

		if (m_nAnchor & ANCHOR_BOTTOM)
			dy = rParentRect.bottom - height() - m_nAnchorMarginY;

		if (m_nAnchor & ANCHOR_HCENTER)
			dx = rParentRect.CenterPoint().x - width() / 2;

		if (m_nAnchor & ANCHOR_VCENTER)
			dy = rParentRect.CenterPoint().y - height() / 2;
	
		SetWindowPos(NULL, dx, dy, width(), height(), SWP_NOZORDER | SWP_NOSIZE);
		//set_back_imagem_pBackOrigin);
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

	//set_back_imagem_pBackOrigin);
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
	if (m_back_img.is_valid() && m_back_img_origin.is_valid())
	{
		m_back_img_origin.deep_copy(&m_back_img);
		m_back_img.sub_image(rc);
	}

	SetWindowPos(&wndNoTopMost, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER);	
	redraw_window();
}

void CGdiButton::set_round(int round, Gdiplus::Color cr_border, Gdiplus::Color	gcr_parent_back)
{
	if (round < 0)
		round = 0;

	m_round = round;

	if (cr_border.GetValue() != Gdiplus::Color::Transparent)
	{
		m_draw_border = true;
		m_cr_border = cr_border;
	}

	if (m_round > 0)
		set_transparent(true, gcr_parent_back);
	else
		redraw_window();
}

int CGdiButton::GetCheck()
{
	return m_idx;
}

void CGdiButton::redraw_window(bool bErase)
{
	Invalidate();
	//투명이미지인 경우에는 parent에서 invalidate한 후 이 버튼이 그려져야하므로 GetParent()->InvalidateRect()을 호출했는데
	//이로인해 깜빡임이 발생한다.
	//GetParent()->InvalidateRect()호출하지 않을 경우 투명png 버튼일 경우 투명 영역이 계속 쌓이면서 투명으로 보이지 않게 된다.
	/*
	if (m_transparent)
	{
		CRect rc;

		GetWindowRect(&rc);

		GetParent()->ScreenToClient(&rc);
		GetParent()->InvalidateRect(rc, bErase);
	}
	else
	{
		Invalidate();
	}
	*/
}

void CGdiButton::Toggle()
{
	if (is_3state())
	{
		m_idx++;
		Cycle(m_idx, BST_UNCHECKED, BST_INDETERMINATE);
	}
	else
	{
		m_idx = !m_idx;
	}

	TRACE(_T("idx = %d\n"), m_idx);
	redraw_window();
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

void CGdiButton::set_blink_time(int nTime0 /*= 500*/, int nTime1 /*= 500*/)
{
	KillTimer(timer_blink);

	m_blink_time0		= nTime0;
	m_blink_time1		= nTime1;

	set_blink(m_blink);
}

void CGdiButton::set_blink(bool blink /*= TRUE*/)
{
	m_blink = blink;
	m_blink_status = FALSE;

	if (m_blink)
	{
		SetTimer(timer_blink, m_blink_time0, NULL);
	}
	else
	{
		m_blink = false;
		KillTimer(timer_blink);
		ShowWindow(SW_SHOW);
		//update_surface();
	}
}

void CGdiButton::set_font(CFont* font)
{
	font->GetLogFont(&m_lf);
	reconstruct_font();
}

void CGdiButton::set_font_size(int size)
{
	m_lf.lfHeight = get_pixel_size_from_font_size(GetParent()->GetSafeHwnd(), size);
	reconstruct_font();
}

void CGdiButton::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();
}

void CGdiButton::set_font_bold(int weight)
{
	m_lf.lfWeight = weight;
	reconstruct_font();
}

//m_bDown과 같은 플래그를 둬서 다운 상태를 판단하게 하는것은
//불편하거나 위험한 코드가 될 수 있다.
//다운 상태에서 마우스가 나가는 경우 등등 체크할 것이 많을수도 있다.
void CGdiButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	redraw_window();

	CRect rc;
	GetClientRect(rc);

	//down되면 SetCapture가 자동으로 들어가서 버튼 밖에서 up되었을때도
	//이 함수가 호출된다.
	//밖에서 up되는 경우는 스킵시킨다.
	if (rc.PtInRect(point))
	{
		if (m_use_auto_repeat)
		{
			KillTimer(timer_auto_repeat);

			if (GetCapture() != NULL)
			{
				ReleaseCapture();
				if (m_sent_once_auto_repeat_click_message == 0 && (GetState() & BST_PUSHED) != 0)
					GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
			}

			return;
		}

		SetCapture();
		::SendMessage(GetParent()->m_hWnd, Message_CGdiButton, (WPARAM) & (CGdiButtonMessage(this, GetDlgCtrlID(), WM_LBUTTONDOWN)), 0);
	}

	CButton::OnLButtonDown(nFlags, point);
}


void CGdiButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (m_use_auto_repeat)
	{
		//SetCapture();
		SetTimer(timer_auto_repeat, m_initial_delay, NULL);
		m_sent_once_auto_repeat_click_message = 0;
	}
	else
	{
		ReleaseCapture();

		CRect rc;
		GetClientRect(rc);

		if (rc.PtInRect(point))
		{
			if (is_button_style(BS_CHECKBOX, BS_AUTOCHECKBOX))
			{
				Toggle();
			}
			else if (is_button_style(BS_RADIOBUTTON, BS_AUTORADIOBUTTON))
			{
				SetCheck((m_idx = 1));
			}

			//TRACE(_T("GetDlgCtrlID() = %d\n"), GetDlgCtrlID());
			GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
			::SendMessage(GetParent()->m_hWnd, Message_CGdiButton, (WPARAM) & (CGdiButtonMessage(this, GetDlgCtrlID(), WM_LBUTTONUP)), 0);
		}
	}

	CButton::OnLButtonUp(nFlags, point);
}

void CGdiButton::use_hover(bool use)
{
	m_use_hover = use;
}

void CGdiButton::draw_hover_rect(bool draw, int thick, int round, Gdiplus::Color cr)
{
	m_use_hover = true;
	m_draw_hover_rect = true;

	if (cr.GetValue() != Gdiplus::Color::Transparent)
		m_hover_rect_color = cr;

	if (thick > 0)
		m_hover_rect_thick = thick;
	else if (draw && thick == 0)
		m_hover_rect_thick = 1;

	if (round > 0)
		set_round(round);

	Invalidate();
}

void CGdiButton::set_hover_rect_thick(int thick)
{
	m_hover_rect_thick = thick;
	Invalidate();
}

void CGdiButton::set_hover_rect_color(Gdiplus::Color cr)
{
	m_hover_rect_color = cr;
	Invalidate();
}

//border
void CGdiButton::draw_border(bool draw, int thick, int round, Gdiplus::Color cr_border)
{
	m_draw_border = draw;

	if (cr_border.GetValue() != Gdiplus::Color::Transparent)
		m_cr_border = cr_border;

	if (thick > 0)
		m_border_thick = thick;
	else if (draw && thick)
		m_border_thick = 1;

	if (round > 0)
		set_round(round);

	Invalidate();
}

//투명 버튼의 경우 그림자를 표시한다.
//shadow_weight가 1.0보다 크면 밝은, 작으면 어두운 그림자가 그려진다.
//blur_sigma가 크면 클수록 그림자의 blur가 강해짐
//만약 draw_shadow()를 단 한번만 호출하는 경우라면 문제없으나
//옵션값에 따라 shadow정도를 보기위함 등 여러번 호출할 경우
//
void CGdiButton::draw_drop_shadow(bool draw, float shadow_weight, float blur_sigma)
{
	m_draw_drop_shadow = draw;

	if (shadow_weight >= 0.0f)
		m_drop_shadow_weight = shadow_weight;

	if (blur_sigma >= 0.0f)
		m_drop_shadow_blur_sigma = blur_sigma;

	Invalidate();
	//RedrawWindow();
}

void CGdiButton::draw_back_shadow(bool draw, float shadow_weight, float blur_sigma)
{
	m_draw_back_shadow = draw;

	if (shadow_weight >= 0.0f)
		m_back_shadow_weight = shadow_weight;

	if (blur_sigma >= 0.0f)
		m_back_shadow_blur_sigma = blur_sigma;

	//Invalidate();
	RedrawWindow();
}

void CGdiButton::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CButton::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	redraw_window();
}


void CGdiButton::OnSize(UINT nType, int cx, int cy)
{
	CButton::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//if (m_fit2image)
	//{
	//}
	//else
	//{
	//	m_width = cx;
	//	m_height = cy;
	//	resize_control(cx, cy);
	//}
}

int	CGdiButton::width()
{
	if (m_image.size() == 0)
	{
		CRect rc;
		GetClientRect(rc);
		return rc.Width();
	}

	return m_image[0]->img->width;
}

int	CGdiButton::height()
{
	if (m_image.size() == 0)
	{
		CRect rc;
		GetClientRect(rc);
		return rc.Height();
	}

	return m_image[0]->img->height;
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
			m_image[i]->img[0].m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->img[0].replace_color(oldColor, newColor);
			m_image[i]->img[1].m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->img[1].replace_color(oldColor, newColor);
			m_image[i]->img[2].m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->img[2].replace_color(oldColor, newColor);
		}
		else if (state_index == 0)
		{
			m_image[i]->img[0].m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->img[0].replace_color(oldColor, newColor);
		}
		else if (state_index == 1)
		{
			m_image[i]->img[1].m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->img[1].replace_color(oldColor, newColor);
		}
		else if (state_index == 2)
		{
			m_image[i]->img[2].m_pBitmap->GetPixel(x, y, &oldColor);
			m_image[i]->img[2].replace_color(oldColor, newColor);
		}
	}

	redraw_window();
}

void CGdiButton::apply_effect_hsl(int state_index, int hue, int sat, int light)
{
	if (m_image.size() == 0)
		return;

	int start_index = 0;
	int end_index = start_index + 1;

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
			m_image[i]->img[0].adjust_hsl(hue, sat, light);
			m_image[i]->img[1].adjust_hsl(hue, sat, light);
			m_image[i]->img[2].adjust_hsl(hue, sat, light);
		}
		else if (state_index == 0)
		{
			m_image[i]->img[0].adjust_hsl(hue, sat, light);
		}
		else if (state_index == 1)
		{
			m_image[i]->img[1].adjust_hsl(hue, sat, light);
		}
		else if (state_index == 2)
		{
			m_image[i]->img[2].adjust_hsl(hue, sat, light);
		}
	}

	redraw_window();
}

void CGdiButton::apply_effect_rgba(int state_index, float r, float g, float b, float a)
{
	if (m_image.size() == 0)
		return;

	int start_index = 0;
	int end_index = start_index + 1;

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
			m_image[i]->img[0].adjust_rgba(r, g, b, a);
			m_image[i]->img[1].adjust_rgba(r, g, b, a);
			m_image[i]->img[2].adjust_rgba(r, g, b, a);
		}
		else if (state_index == 0)
		{
			m_image[i]->img[0].adjust_rgba(r, g, b, a);
		}
		else if (state_index == 1)
		{
			m_image[i]->img[1].adjust_rgba(r, g, b, a);
		}
		else if (state_index == 2)
		{
			m_image[i]->img[2].adjust_rgba(r, g, b, a);
		}
	}

	redraw_window();
}

void CGdiButton::apply_effect_blur(int state_index, float radius, BOOL expandEdge)
{
	if (m_image.size() == 0)
		return;

	int start_index = 0;
	int end_index = start_index + 1;

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
			m_image[i]->img[0].blur(radius, expandEdge);
			m_image[i]->img[1].blur(radius, expandEdge);
			m_image[i]->img[2].blur(radius, expandEdge);
		}
		else if (state_index == 0)
		{
			m_image[i]->img[0].blur(radius, expandEdge);
		}
		else if (state_index == 1)
		{
			m_image[i]->img[1].blur(radius, expandEdge);
		}
		else if (state_index == 2)
		{
			m_image[i]->img[2].blur(radius, expandEdge);
		}
	}

	redraw_window();
}


void CGdiButton::set_auto_repeat(bool use)
{
	m_use_auto_repeat = use;
}

void CGdiButton::set_auto_repeat_delay(int initial_delay, int repeat_delay)
{
	m_initial_delay = initial_delay;
	m_repeat_delay = repeat_delay;
}
#if 0
bool CGdiButton::is_push_button()
{
	/*
	if ((m_button_type & BS_PUSHBUTTON) == BS_PUSHBUTTON)
		return true;
	else if ((m_button_type & BS_DEFPUSHBUTTON) == BS_DEFPUSHBUTTON)
		return true;
	*/
	if (m_button_type == BS_PUSHBUTTON || m_button_type == BS_DEFPUSHBUTTON)
		return true;

	return false;
}

bool CGdiButton::is_radio_button()
{
	//if ((m_button_type & BS_RADIOBUTTON) == BS_RADIOBUTTON)
	//	return true;
	//else if ((m_button_type & BS_AUTORADIOBUTTON) == BS_AUTORADIOBUTTON)
	//	return true;

	if (m_button_type == BS_RADIOBUTTON || m_button_type == BS_AUTORADIOBUTTON)
		return true;

	return false;
}

bool CGdiButton::is_check_box()
{
	//if ((m_button_type & BS_CHECKBOX) == BS_CHECKBOX)
	//	return true;
	//else if ((m_button_type & BS_AUTOCHECKBOX) == BS_AUTOCHECKBOX)
	//	return true;

	if (m_button_type == BS_CHECKBOX || m_button_type == BS_AUTOCHECKBOX)
		return true;

	return false;
}

bool CGdiButton::is_push_like()
{
	if ((m_button_style & BS_PUSHLIKE) == BS_PUSHLIKE)
		return true;
	return false;
}
#endif

void CGdiButton::set_header_image(UINT id, float ratio, UINT align)
{
	m_img_header.load(id);
	m_img_header_ratio = ratio;
	m_img_header_align = align;

	CRect r = m_img_header.get_transparent_rect();
}

//이미지와 레이블 사이의 간격 픽셀 크기. m_img_header_gap. default = 4;
void CGdiButton::set_header_image_gap(int gap)
{
	m_img_header_gap = gap;
}

//기능 구현 중
void CGdiButton::OnNcPaint()
{
	if (!m_draw_back_shadow)
		return;

	Gdiplus::SolidBrush br(Gdiplus::Color::Blue);

	CString text;
	GetWindowText(text);

	//dc(this)라고 선언하고 g(dc)를 구해서 g에 그리면 이 버튼 영역 외에는 그림이 그려지지 않는다.
	//dc(this)라고 선언했어도 dc.FillSolidRect(r);로 그리면 r이 이 버튼 영역을 벗어나도 그림이 그려진다.
	//따라서 back shadow는 이 버튼 영역 밖이므로 dc(GetParent())로 구하고
	//실제 그릴때는 parent 기준의 이 버튼 좌표 영역에 그려줘야 한다.
	CClientDC  dc(GetParent());
	CRect rc;

	GetClientRect(rc);
	Gdiplus::Graphics g(dc);

	//버튼 이미지가 있다면 이미지 모양대로 back shadow를 만들어야 하고
	//버튼 이미지가 없어서 Gdi로 그려진 버튼이라면 기본 사각형을 바탕으로 back shadow를 만들어준다.
	CGdiplusBitmap img_shadow;

	if (m_image.size() && m_image[0]->img[0].is_valid())
	{
		m_image[0]->img[0].create_back_shadow_image(&img_shadow, 2.0f);
	}
	else
	{
		img_shadow.create(rc.Width(), rc.Height());
		Gdiplus::Graphics g(img_shadow.m_pBitmap);
		g.FillEllipse(&br, Gdiplus::Rect(4, 4, rc.Width() - 8, rc.Height() - 8));
	}
#ifdef _DEBUG
	img_shadow.save(_T("d:\\back_shadow.png"));
#endif

	//parent의 dc를 구해서 그리는 것이므로 좌표 또한 parent 기준의 상대좌표로 그려줘야 한다.
	CRect rw;
	GetWindowRect(rw);
	GetParent()->ScreenToClient(rw);

	//버튼 영역이 모니터를 나갔다가 다시 들어올 때 제대로 갱신되지 않는다.
	//GetParent()->InvalidateRect(rw);	//이 코드를 쓰면 계속 그리는 현상 발생
	Invalidate();						//이 코드를 쓰면 잔상은 덜하나 여전히 발생
	img_shadow.draw(&dc, rw.left + 14, rw.top - 0);
}
