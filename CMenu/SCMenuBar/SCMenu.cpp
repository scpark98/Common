// SCMenu.cpp : implementation file
//

#include "SCMenu.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

CSCMenuMessage::CSCMenuMessage(CWnd* _this, int message, CSCMenuItem* menu_item, int button_index, int button_state)
{
	m_pWnd = _this;
	m_message = message;
	m_menu_item = menu_item;
	m_button_index = button_index;
	m_button_state = button_state;
}

CSCMenuSubButton::CSCMenuSubButton(UINT _id, int menu_height)
{
	m_button_image[0] = new CGdiplusBitmap(_T("PNG"), _id);	//normal
	m_button_image[1] = new CGdiplusBitmap(_T("PNG"), _id);	//selected
	m_button_image[0]->gray();

	//기존 버튼 이미지 컬러톤이 cyan 톤이었으나 코드상에서 blue톤으로 변경 가능
	m_button_image[1]->apply_effect_hsl(70);

	//menu_height의 80%크기의 높이를 가지도록 조정한다.
	float img_height = (float)menu_height * (float)0.9;
	float ratio = img_height / (float)(m_button_image[0]->height);
	m_button_image[0]->resize(ratio, ratio);
	m_button_image[1]->resize(ratio, ratio);

	m_r.right = m_button_image[0]->width;
	m_r.bottom = m_button_image[0]->height;
}

CSCMenuSubButton::~CSCMenuSubButton()
{
	if (m_button_image[0])
	{
		m_button_image[0]->release();
		delete m_button_image[0];
	}

	if (m_button_image[1])
	{
		m_button_image[1]->release();
		delete m_button_image[1];
	}
}

CSCMenuItem::CSCMenuItem(int id, CString caption, UINT icon_id, CString hot_key, int menu_height)
{
	m_id = id;
	m_caption = caption;
	m_hot_key = hot_key;
	m_menu_height = (menu_height > 0 ? menu_height : 22);

	set_icon(icon_id);
}

CSCMenuItem::~CSCMenuItem()
{
	for (int i = 0; i < m_buttons.size(); i++)
	{
		delete m_buttons[i];
	}
}

void CSCMenuItem::set_icon(UINT icon_id)
{
	m_icon.release();

	if (icon_id > 0)
	{
		m_icon.load(icon_id);

		//menu_height의 90% 크기의 높이를 가지도록 조정한다.
		float ico_height = (float)m_menu_height * (float)0.9;
		float ratio = ico_height / (float)(m_icon.height);
		m_icon.resize(ratio, ratio);
	}
}

void CSCMenuItem::add_button(UINT button_id, bool reset)
{
	CSCMenuSubButton* btn = new CSCMenuSubButton(button_id, m_menu_height);
	m_buttons.push_back(btn);
}

//버튼이 1개인 경우는 check <-> uncheck toggle이고
//2개 이상일 경우는 radio로 동작한다.
void CSCMenuItem::set_check(int idx, bool _check)
{
	//존재하지 않는 메뉴항목의 서브버튼을 조작하려하면 this가 NULL임.
	if (this == NULL)
		return;

	ASSERT(m_buttons.size() > 0 && idx >= 0 && idx < m_buttons.size());

	//버튼이 1개인 경우는 check <-> uncheck toggle이고
	if (m_buttons.size() == 1)
	{
		m_buttons[idx]->m_state = !m_buttons[idx]->m_state;
	}
	//2개 이상일 경우는 radio로 동작한다.
	else
	{
		for (int i = 0; i < m_buttons.size(); i++)
		{
			if (i == idx)
				m_buttons[i]->m_state = true;
			else
				m_buttons[i]->m_state = false;
		}
	}
}

int CSCMenuItem::get_check(int idx)
{
	if (m_buttons.size() <= 0)
		return -1;
	return m_buttons[idx]->m_state;
}

int CSCMenuItem::get_checked_button_index()
{
	for (int i = 0; i < m_buttons.size(); i++)
	{
		if (m_buttons[i]->m_state)
		{
			return i;
		}
	}

	return -1;
}

// CSCMenu

IMPLEMENT_DYNAMIC(CSCMenu, CDialogEx)

CSCMenu::CSCMenu()
{
	memset(&m_lf, 0, sizeof(LOGFONT));
	set_color_theme(color_theme_default, false);
}

CSCMenu::~CSCMenu()
{
	for (int i = 0; i < m_items.size(); i++)
	{
		delete m_items[i];
	}
}


BEGIN_MESSAGE_MAP(CSCMenu, CDialogEx)
	ON_WM_ERASEBKGND()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	//ON_CONTROL_REFLECT_EX(LBN_SELCHANGE, &CSCMenu::OnLbnSelchange)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CSCMenu::add(int _id, CString _caption, UINT icon_id, CString _hot_key)
{
	CSCMenuItem* item = new CSCMenuItem(_id, _caption, icon_id, _hot_key);
	m_items.push_back(item);
	recalc_items_rect();
}

void CSCMenu::add_sub_button(int index, UINT id)
{
	CSCMenuSubButton* button = new CSCMenuSubButton(id, m_line_height);
	m_items[index]->m_buttons.push_back(button);
}


//resource editor에서 이 클래스 컨트롤을 추가해서 사용하는 것이 아닌
//동적으로 create해서 사용하므로 여기에 초기화 코드를 넣어도 의미없다.
//create() 한 후에 초기화 코드를 넣어줘야 한다.
void CSCMenu::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialogEx::PreSubclassWindow();
}


BOOL CSCMenu::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
			case VK_ESCAPE :
				TRACE(_T("esc\n"));
				ShowWindow(SW_HIDE);
				::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
					(WPARAM)&CSCMenuMessage(this, message_scmenu_hide, NULL), 0);
				return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


BOOL CSCMenu::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
	//return CDialogEx::OnEraseBkgnd(pDC);
}


void CSCMenu::OnKillFocus(CWnd* pNewWnd)
{
	CDialogEx::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	ShowWindow(SW_HIDE);
	::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
		(WPARAM)&CSCMenuMessage(this, message_scmenu_hide, NULL), 0);
}


void CSCMenu::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	//클릭된 위치가 메뉴의 sub button일 경우
	bool found = false;

	for (int i = 0; i < m_items.size(); i++)
	{
		if (m_items[i]->m_id < 0)
			continue;

		for (int j = 0; j < m_items[i]->m_buttons.size(); j++)
		{
			if (m_items[i]->m_buttons[j]->m_r.PtInRect(point))
			{
				m_items[i]->set_check(j);
				TRACE(_T("%d, %d\n"), i, j);

				CString text;

				if (m_parent)
				{
					::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
						(WPARAM)&CSCMenuMessage(this, message_scmenu_button_state_changed, m_items[i], j, m_items[i]->get_check(j)),
						0);
					//list항목이 선택된게 아니고 버튼을 클릭한 경우는 메뉴를 숨기지 않는다.
					//ShowWindow(SW_HIDE);
					Invalidate();
					return;
				}

				found = true;
				break;
			}
		}

		//sub button이 눌린 경우
		if (found)
		{
			break;
		}
		//메뉴 항목이 눌린 경우
		else if (m_items[i]->m_r.PtInRect(point))
		{
			::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
				(WPARAM)&CSCMenuMessage(this, message_scmenu_selchanged, m_items[i]), 0);
			ShowWindow(SW_HIDE);
			return;
		}
	}

	if (found)
		Invalidate();

	CDialogEx::OnLButtonDown(nFlags, point);
}


void CSCMenu::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CDialogEx::OnLButtonUp(nFlags, point);
}

int CSCMenu::get_item_index(CPoint pt)
{
	for (int i = 0; i < m_items.size(); i++)
	{
		if (m_items[i]->m_id >= 0 && m_items[i]->m_r.PtInRect(pt))
			return i;
	}

	return -1;
}

void CSCMenu::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (m_use_over)
	{
		int over_item = get_item_index(point);
		if (over_item != m_over_item)
		{
			m_over_item = over_item;
			Invalidate(false);
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}


void CSCMenu::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
}

bool CSCMenu::create(CWnd* parent)
{
	m_parent = parent;

	DWORD dwStyle = WS_POPUP | WS_BORDER;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCMenu");
	AfxRegisterClass(&wc);

	//CString class_name = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)::GetStockObject(WHITE_BRUSH), NULL);

	//CreateEx()에서 첫번째 파라미터를 0으로 준 경우는 class_name도 NULL로 줘도 동작했으나 WS_EX_...스타일이 있을 경우는 NULL로 하면 에러 발생.
	bool res = CreateEx(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, wc.lpszClassName, _T("CSCMenu"), dwStyle, CRect(0, 0, 320, 300), parent, 0);

	if (res)
	{
		CFont* font = GetFont();

		if (font == NULL)
			font = AfxGetMainWnd()->GetFont();

		if (font != NULL)
			font->GetObject(sizeof(m_lf), &m_lf);
		else
			GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

		ReconstructFont();
	}

	return res;
}

//모니터 영역을 벗어나지 않도록 보정한 후 표시해야 한다.
void CSCMenu::popup_menu(int x, int y)
{
	if (m_items.size() == 0)
		return;

	m_over_item = -1;

	recalc_items_rect();


	//팝업메뉴가 표시될 때마다 매번 모니터 정보를 새로 읽어줘야 한다.
	//그래야 멀티모니터, 모니터 영역 변경 등의 설정에 맞춰 올바르게 표시된다.
	enum_display_monitors();

	CRect rw, rMonitor;

	GetWindowRect(rw);

	int idx = get_monitor_index(x, y);
	if (idx < 0)
	{
		//이 블록은 실행되는 일이 없어야 한다.
		return;
	}

	rw.MoveToXY(x, y);
	rMonitor = g_dqMonitors[idx];

	//모니터 영역을 벗어나는 경우 모니터의 right or bottom에서 10정도는 떨어뜨린다.
	rMonitor.DeflateRect(10, 10);

	adjust_rect_range(rw, rMonitor, true, true);

	MoveWindow(rw);

	//CenterWindow();
	ShowWindow(SW_SHOW);
}

CSCMenuItem* CSCMenu::get_menu_item(int menu_id)
{
	for (int i = 0; i < m_items.size(); i++)
	{
		if (m_items[i]->m_id == menu_id)
			return m_items[i];
	}

	return NULL;
}

void CSCMenu::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	m_line_height = -m_lf.lfHeight + 16;
	recalc_items_rect();

	ASSERT(bCreated);
}

void CSCMenu::set_line_height(int _line_height)
{
	if (_line_height < 8)
		_line_height = 8;

	m_line_height = _line_height;
	recalc_items_rect();
	Invalidate();
}

CSCMenu& CSCMenu::set_font(LOGFONT& lf)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	ReconstructFont();

	return *this;
}

CSCMenu& CSCMenu::set_font_name(CString sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	ReconstructFont();

	return *this;
}

CSCMenu& CSCMenu::set_font_size(int nSize)
{
	HDC hDC = GetDC()->GetSafeHdc();
	m_lf.lfHeight = -MulDiv(nSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	::ReleaseDC(m_hWnd, hDC);
	ReconstructFont();

	return *this;
}

CSCMenu& CSCMenu::set_font_bold(bool bBold)
{
	m_lf.lfWeight = (bBold ? FW_BOLD : FW_NORMAL);
	ReconstructFont();

	return *this;
}

void CSCMenu::set_color_theme(int theme, bool apply_now)
{
	switch (theme)
	{
	case color_theme_default:
		m_cr_text = ::GetSysColor(COLOR_BTNTEXT);
		m_cr_text_selected = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_cr_text_selected_inactive = ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_cr_text_over = m_cr_text;

		m_cr_back = ::GetSysColor(COLOR_3DFACE);
		m_cr_back_selected = RGB(174, 215, 247);
		m_cr_back_selected_border = get_color(m_cr_back_selected, -32);// RGB(0, 120, 215);
		m_cr_back_selected_inactive = ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_over = m_cr_back_selected; //RGB(235, 245, 255); //m_crBackOver = ::GetSysColor(COLOR_HIGHLIGHT);
		break;
	case color_theme_dark_gray:
		m_cr_text = RGB(210, 220, 221);
		m_cr_text_selected = m_cr_text;
		m_cr_text_selected_inactive = ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_cr_text_over = RGB(255, 255, 255);

		m_cr_back = RGB(27, 27, 28); //RGB(242, 242, 242);// ::GetSysColor(COLOR_WINDOW);
		m_cr_back_selected = RGB(51, 51, 52);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_selected_border = get_color(m_cr_back_selected, 64);// RGB(0, 120, 215);
		m_cr_back_selected_inactive = ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_over = m_cr_back_selected;
		break;
	case color_theme_linkmemine:
		m_cr_text = RGB(208, 211, 220);
		m_cr_text_selected = m_cr_text;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_cr_text_selected_inactive = ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_cr_text_over = m_cr_text;

		m_cr_back = RGB(64, 73, 88);// ::GetSysColor(COLOR_WINDOW);
		m_cr_back_selected = RGB(45, 51, 51);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_selected_border = get_color(m_cr_back_selected, +32);// RGB(0, 120, 215);
		m_cr_back_selected_inactive = m_cr_back_selected;
		m_cr_back_over = m_cr_back_selected;
		break;
	}

	if (apply_now)
		Invalidate();
}

//선택된 메뉴ID를 전달한다.
/*
BOOL CSCMenu::OnLbnSelchange()
{
	TRACE(_T("changed\n"));

	int index = -1;// GetCurSel();

	//if (index < 0 || index >= GetCount())
		return FALSE;

	CString text;

	//GetText(index, text);

	if (m_parent)
	{
		::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
			(WPARAM)&CSCMenuMessage(this, message_scmenu_selchanged, m_items[index]->id), 0);
		ShowWindow(SW_HIDE);
	}

	return FALSE;
}
*/

//라인 간격은 font size에 따라 자동 조절되게 할 수도 있지만 장단점이 있다.
//일단은 별개로 처리되도록 한다.
void CSCMenu::recalc_items_rect()
{
	int sy = 2;
	bool is_separator;
	int total_height = 0;	//모든 메뉴 항목을 표시하는 총 높이
	int max_width = 0;		//가장 긴 메뉴항목 너비
	CRect rc;

	//GetWindowRect()가 아닌 GetClientRect()를 한 후 밑에서 MoveWindow()하면
	//윈도우 크기는 점점 줄어들게 된다. 윈도우의 border 효과 때문에
	//보이는 크기보다 실제 영역은 더 크다.
	GetWindowRect(rc);
	ScreenToClient(rc);
	rc.MoveToXY(0, 0);

	for (int i = 0; i < m_items.size(); i++)
	{
		is_separator = (m_items[i]->m_id <= 0);
		m_items[i]->m_r = CRect(2, sy, rc.right - 9, sy + (is_separator ? 6 : m_line_height));
		sy = m_items[i]->m_r.bottom + 2;

		total_height = m_items[i]->m_r.bottom;
	}

	rc.bottom = rc.top + total_height + 2 + 8;	//2=margin(equal to top margin), 8 = border height? * 2

	SetWindowPos(&wndTopMost, 0, 0, rc.Width(), rc.Height(), SWP_NOMOVE);
}

void CSCMenu::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CDialogEx::OnPaint() for painting messages
	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);
	Gdiplus::Graphics g(dc.m_hDC, rc);

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	dc.FillSolidRect(rc, m_cr_back);

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(m_cr_text);

	CFont* pOldFont = (CFont*)dc.SelectObject(&m_font);

	for (int i = 0; i < m_items.size(); i++)
	{
		//separator의 경우
		if (m_items[i]->m_id <= 0)
		{
			DrawLine(&dc, m_items[i]->m_r.left, m_items[i]->m_r.CenterPoint().y, m_items[i]->m_r.right, m_items[i]->m_r.CenterPoint().y, RGB(214, 214, 214));
		}
		//일반 메뉴 항목인 경우
		else
		{
			//선택된 항목 표시
			if (i == m_over_item)
			{
				dc.SetTextColor(m_cr_text_over);
				//dc.FillSolidRect(m_items[i]->r, m_cr_back_over);

				Gdiplus::Color gcr_over_stroke;
				Gdiplus::Color gcr_over_fill;

				//gcr_over_stroke.SetFromCOLORREF(m_cr_back_selected_border);
				gcr_over_stroke.SetFromCOLORREF(m_cr_back_over);
				gcr_over_fill.SetFromCOLORREF(m_cr_back_over);
				draw_round_rect(&g, CRect2GpRect(m_items[i]->m_r), gcr_over_stroke, gcr_over_fill, 0);
				//draw_round_rect(&g, CRect2GpRect(m_items[i]->m_r), Gdiplus::Color(255,225,0,0), gcr_over_fill, 2);

				//CGdiplusBitmap img;
				//img.round_shadow_rect(m_items[i]->r.Width(), m_items[i]->r.Height(), 4);
				//img.draw(&g, m_items[i]->r, true);
			}
			else
			{
				dc.SetTextColor(m_cr_text);
			}

			CRect rText = m_items[i]->m_r;

			//메뉴 아이콘을 그리고
			if (m_items[i]->m_icon.is_valid())
			{
				CRect rIcon = m_items[i]->m_r;
				rIcon.right = rIcon.left + 32;
				rIcon = get_center_rect(rIcon, m_items[i]->m_icon.width, m_items[i]->m_icon.height);
				m_items[i]->m_icon.draw(g, rIcon.left, rIcon.top);
			}

			//메뉴 아이콘 유무에 관계없이 아이콘 영역만큼 공백을 준다.
			rText.left = rText.left + 32 + 4;

			//메뉴 캡션 표시
			dc.DrawText(m_items[i]->m_caption, rText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			//sub button들이 존재하면 그려준다.
			CRect r = m_items[i]->m_r;
			r.top += 2;
			//하나의 메뉴 항목에 표시되는 sub button들의 r은 여기서 계산해서 그린다?
			//라인 간격이 정해지면 그 때 이미 계산은 가능하다.
			for (int j = m_items[i]->m_buttons.size() - 1; j >= 0; j--)
			{
				r.right = r.right - 6;
				r.left = r.right - m_items[i]->m_buttons[j]->m_r.Width();
				r.top = r.top + (r.Height() - m_items[i]->m_buttons[j]->m_button_image[0]->height) / 2;
				r.bottom = r.top + m_items[i]->m_buttons[j]->m_button_image[0]->height;
				m_items[i]->m_buttons[j]->m_r = r;
				m_items[i]->m_buttons[j]->m_button_image[m_items[i]->m_buttons[j]->m_state]->draw(g, r);

				//현재 sub button 앞에 아직 다른 버튼이 존재한다면 r을 왼쪽으로 shift시키고 계속 그려준다.
				if (j > 0)
					r.OffsetRect(-m_items[i]->m_buttons[j]->m_r.Width() - 2, 0);
			}

			//단축키 정보가 있다면 표시하고
			if (!m_items[i]->m_hot_key.IsEmpty())
			{
				//sub button이 없는 메뉴항목일 경우
				if (r == m_items[i]->m_r)
					rText.right = r.right - 6;
				//sub button이 있을 경우
				else
					rText.right = r.left - 8;

				//DrawRectangle(&dc, rText);
				dc.DrawText(m_items[i]->m_hot_key, rText, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			}
		}
	}

	dc.SelectObject(pOldFont);
}
