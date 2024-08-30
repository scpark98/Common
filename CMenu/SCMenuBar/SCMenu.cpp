// SCMenu.cpp : implementation file
//

#include "SCMenu.h"

#include "../../Functions.h"
#include "../../colors.h"
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

	//���� ��ư �̹��� �÷����� cyan ���̾����� �ڵ�󿡼� �ٸ� ���� ������ ���� ����
	m_button_image[1]->apply_effect_hsl(70);

	//��ư �̹����� ũ��� menu_height�� 80% ũ���� ���̸� �������� �����Ѵ�.
	float img_height = (float)menu_height * (float)0.95;
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
		delete m_button_image[0];
	}

	if (m_button_image[1])
	{
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

	m_buttons.clear();
}

void CSCMenuItem::set_icon(UINT icon_id)
{
	m_icon.release();

	if (icon_id > 0)
	{
		m_icon.load(icon_id);

		//menu_height�� 90% ũ���� ���̸� �������� �����Ѵ�.
		float ico_height = (float)m_menu_height * (float)0.95;
		float ratio = ico_height / (float)(m_icon.height);
		m_icon.resize(ratio, ratio);
	}
}

void CSCMenuItem::add_button(UINT button_id, bool reset)
{
	CSCMenuSubButton* btn = new CSCMenuSubButton(button_id, m_menu_height);
	m_buttons.push_back(btn);
}

//��ư�� 1���� ���� check <-> uncheck toggle�̰�
//2�� �̻��� ���� radio�� �����Ѵ�.
void CSCMenuItem::set_check(int idx, bool _check)
{
	//�������� �ʴ� �޴��׸��� �����ư�� �����Ϸ��ϸ� this�� NULL��.
	if (this == NULL)
		return;

	ASSERT(m_buttons.size() > 0 && idx >= 0 && idx < m_buttons.size());

	//��ư�� 1���� ���� check <-> uncheck toggle�̰�
	if (m_buttons.size() == 1)
	{
		m_buttons[idx]->m_state = !m_buttons[idx]->m_state;
	}
	//2�� �̻��� ���� radio�� �����Ѵ�.
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
}

CSCMenu::~CSCMenu()
{
	for (int i = 0; i < m_items.size(); i++)
	{
		for (int j = 0; j < m_items[i]->m_buttons.size(); j++)
		{
			delete m_items[i]->m_buttons[j];
		}

		m_items[i]->m_buttons.clear();

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

void CSCMenu::add_sub_button_by_menu_index(int index, UINT id)
{
	CSCMenuSubButton* button = new CSCMenuSubButton(id, m_line_height);
	m_items[index]->m_buttons.push_back(button);
}

void CSCMenu::add_sub_button_by_menu_id(int menu_id, UINT id)
{
	CSCMenuItem* menu_item = get_menu_item(menu_id);
	menu_item->add_button(id);
}

void CSCMenu::set_check(UINT menu_id, int sub_button_index, bool check)
{
	CSCMenuItem* menu_item = get_menu_item(menu_id);

	ASSERT(menu_item != NULL);

	if (menu_item == 0)
	{
		TRACE(_T("error : no exist menu_id(%d)\n"), menu_id);
	}

	menu_item->set_check(sub_button_index, check);
}

//resource editor���� �� Ŭ���� ��Ʈ���� �߰��ؼ� ����ϴ� ���� �ƴ�
//�������� create�ؼ� ����ϹǷ� ���⿡ �ʱ�ȭ �ڵ带 �־ �ǹ̾���.
//create() �� �Ŀ� �ʱ�ȭ �ڵ带 �־���� �Ѵ�.
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
	//Ŭ���� ��ġ�� �޴��� sub button�� ���
	bool found = false;

	for (int i = 0; i < m_items.size(); i++)
	{
		if (m_items[i]->m_id <= 0)
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
					//list�׸��� ���õȰ� �ƴϰ� ��ư�� Ŭ���� ���� �޴��� ������ �ʴ´�.
					//ShowWindow(SW_HIDE);
					Invalidate();
					return;
				}

				found = true;
				break;
			}
		}

		//sub button�� ���� ���
		if (found)
		{
			break;
		}
		//�޴� �׸��� ���� ���
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
		if (m_items[i]->m_id > 0 && m_items[i]->m_r.PtInRect(pt))
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

bool CSCMenu::create(CWnd* parent, int width)
{
	m_parent = parent;

	DWORD dwStyle = WS_POPUP | WS_BORDER;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCMenu");
	AfxRegisterClass(&wc);

	//CString class_name = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)::GetStockObject(WHITE_BRUSH), NULL);

	//CreateEx()���� ù��° �Ķ���͸� 0���� �� ���� class_name�� NULL�� �൵ ���������� WS_EX_...��Ÿ���� ���� ���� NULL�� �ϸ� ���� �߻�.
	//height�� �ڵ� recalc��.
	bool res = CreateEx(NULL, wc.lpszClassName, _T("CSCMenu"), dwStyle, CRect(0, 0, width, 300), parent, 0);

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

//load from menu resource
void CSCMenu::load(UINT resource_id, int menu_index)
{
	CMenu menu;
	CMenu* pMenu;

	menu.LoadMenu(resource_id);
	pMenu = menu.GetSubMenu(menu_index);

	int i, nCount = GetMenuItemCount(pMenu->m_hMenu);
	UINT menu_id;
	CString menu_caption;

	for (i = 0; i < nCount; i++)
	{
		get_menu_item_info(pMenu->m_hMenu, i, &menu_id, &menu_caption, TRUE);

		TRACE(_T("ID = %u, string = %s\n"), menu_id, menu_caption);

		//seperator�� �Ϲ� �޴��׸� ���(���� �˾��޴� �׸��� ����)
		if (menu_id == 0 || (menu_id >= 0x8000 && menu_id <= 0xDFFF))
		{
			std::deque<CString> token;
			get_token_string(menu_caption, token, _T("\t"));

			add(menu_id, menu_caption, 0, (token.size() == 2 ? token[1] : _T("")));
		}
	}
}

//����� ������ ����� �ʵ��� ������ �� ǥ���ؾ� �Ѵ�.
void CSCMenu::popup_menu(int x, int y)
{
	if (m_items.size() == 0)
		return;

	m_over_item = -1;

	recalc_items_rect();


	//�˾��޴��� ǥ�õ� ������ �Ź� ����� ������ ���� �о���� �Ѵ�.
	//�׷��� ��Ƽ�����, ����� ���� ���� ���� ������ ���� �ùٸ��� ǥ�õȴ�.
	enum_display_monitors();

	CRect rw, rMonitor;

	GetWindowRect(rw);

	int idx = get_monitor_index(x, y);
	if (idx < 0)
	{
		//�� ����� ����Ǵ� ���� ����� �Ѵ�.
		return;
	}

	rw.MoveToXY(x, y);
	rMonitor = g_dqMonitors[idx];

	//����� ������ ����� ��� ������� right or bottom���� 10������ ����߸���.
	rMonitor.DeflateRect(10, 10);

	adjust_rect_range(rw, rMonitor, true, true);

	MoveWindow(rw);

	//CenterWindow();
	ShowWindow(SW_SHOW);
}

CSCMenuItem* CSCMenu::get_menu_item(int menu_id)
{
	if (menu_id <= 0)
		return NULL;

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

	m_line_height = -m_lf.lfHeight + 10;
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
/*
void CSCMenu::set_color_theme(int theme, bool apply_now)
{
	switch (theme)
	{
	case color_theme_default:
		m_cr_text = RGB2gpColor(::GetSysColor(COLOR_BTNTEXT));
		m_cr_text_selected = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		m_cr_text_selected_inactive = RGB2gpColor(::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
		m_cr_text_over = m_cr_text;

		m_cr_back = RGB2gpColor(::GetSysColor(COLOR_3DFACE));
		m_cr_back_selected = RGB2gpColor(RGB(174, 215, 247));
		m_cr_back_selected_border = get_color(m_cr_back_selected, -32);// RGB(0, 120, 215);
		m_cr_back_selected_inactive = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
		m_cr_back_over = m_cr_back_selected; //RGB(235, 245, 255); //m_crBackOver = ::GetSysColor(COLOR_HIGHLIGHT);
		break;
	case color_theme_dark_gray:
		m_cr_text = RGB2gpColor(RGB(210, 220, 221));
		m_cr_text_selected = m_cr_text;
		m_cr_text_selected_inactive = RGB2gpColor(::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
		m_cr_text_over = RGB2gpColor(RGB(255, 255, 255));

		m_cr_back = RGB2gpColor(RGB(45, 45, 48)); //RGB(242, 242, 242);// ::GetSysColor(COLOR_WINDOW);
		m_cr_back_selected = RGB2gpColor(RGB(51, 51, 52));// ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_selected_border = get_color(m_cr_back_selected, 64);// RGB(0, 120, 215);
		m_cr_back_selected_inactive = RGB2gpColor(::GetSysColor(COLOR_HIGHLIGHT));
		m_cr_back_over = m_cr_back_selected;
		break;
	case color_theme_linkmemine:
		m_cr_text = RGB2gpColor(RGB(208, 211, 220));
		m_cr_text_selected = m_cr_text;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_cr_text_selected_inactive = RGB2gpColor(::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
		m_cr_text_over = m_cr_text;

		m_cr_back = RGB2gpColor(RGB(64, 73, 88));// ::GetSysColor(COLOR_WINDOW);
		m_cr_back_selected = RGB2gpColor(RGB(45, 51, 51));// ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_selected_border = get_color(m_cr_back_selected, +32);// RGB(0, 120, 215);
		m_cr_back_selected_inactive = m_cr_back_selected;
		m_cr_back_over = m_cr_back_selected;
		break;
	}

	if (apply_now)
		Invalidate();
}
*/
//���õ� �޴�ID�� �����Ѵ�.
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

//���� ������ font size�� ���� �ڵ� �����ǰ� �� ���� ������ ������� �ִ�.
//�ϴ��� ������ ó���ǵ��� �Ѵ�.
void CSCMenu::recalc_items_rect()
{
	int sy = 2;
	bool is_separator;
	int total_height = 0;	//��� �޴� �׸��� ǥ���ϴ� �� ����
	int max_width = 0;		//���� �� �޴��׸� �ʺ�
	CSize	border;
	CRect	rw, rc;

	//GetWindowRect()�� �ƴ� GetClientRect()�� �� �� �ؿ��� MoveWindow()�ϸ�
	//������ ũ��� ���� �پ��� �ȴ�. �������� border ȿ�� ������
	//���̴� ũ�⺸�� ���� ������ �� ũ��.
	GetWindowRect(rw);
	GetClientRect(rc);
	border = CSize((rw.Width() - rc.Width()) / 2, (rw.Height() - rc.Height()) / 2);

	ScreenToClient(rw);
	rw.MoveToXY(0, 0);

	//item rect�� ũ��� border ������ ���� �ٸ� �� �ִ�.
	for (int i = 0; i < m_items.size(); i++)
	{
		is_separator = (m_items[i]->m_id <= 0);
		m_items[i]->m_r = CRect(4, sy, rw.right - 5 - border.cx, sy + (is_separator ? 5 : m_line_height));
		sy = m_items[i]->m_r.bottom + 2;

		total_height = m_items[i]->m_r.bottom;
	}

	rw.bottom = rw.top + total_height + 4 + border.cy * 2;	//2=margin(equal to top margin), 8 = border height? * 2

	SetWindowPos(&wndTopMost, 0, 0, rw.Width(), rw.Height(), SWP_NOMOVE);
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

	//dc.FillSolidRect(rc, m_cr_back);
	if (m_img_back.is_valid())
		m_img_back.draw(g, rc);
	else
		dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(m_theme.cr_text.ToCOLORREF());

	CFont* pOldFont = (CFont*)dc.SelectObject(&m_font);

	for (int i = 0; i < m_items.size(); i++)
	{
		//separator�� ���
		if (m_items[i]->m_id <= 0)
		{
			draw_line(&dc, m_items[i]->m_r.left, m_items[i]->m_r.CenterPoint().y, m_items[i]->m_r.right, m_items[i]->m_r.CenterPoint().y, gGRAY(214));
		}
		//�Ϲ� �޴� �׸��� ���
		else
		{
			//���õ� �׸� ǥ��
			if (i == m_over_item)
			{
				dc.SetTextColor(m_theme.cr_text_hover.ToCOLORREF());
				dc.FillSolidRect(m_items[i]->m_r, m_theme.cr_back_hover.ToCOLORREF());

				//Gdiplus::Color gcr_over_stroke = Gdiplus::Color(128, GetRValue(m_cr_back_selected_border), GetGValue(m_cr_back_selected_border), GetBValue(m_cr_back_selected_border));
				//Gdiplus::Color gcr_over_fill = Gdiplus::Color(128, GetRValue(m_cr_back_over), GetGValue(m_cr_back_over), GetBValue(m_cr_back_over));
				Gdiplus::Color gcr_over_stroke = m_theme.cr_back_selected_border;
				Gdiplus::Color gcr_over_fill = m_theme.cr_back_hover;

				//gcr_over_stroke.SetFromCOLORREF(m_cr_back_selected_border);
				//gcr_over_stroke.SetFromCOLORREF(m_cr_back_selected_border);
				//gcr_over_fill.SetFromCOLORREF(m_cr_back_over);
				//draw_round_rect(&g, CRect2GpRect(m_items[i]->m_r), gcr_over_stroke, gcr_over_fill, 0);
				//draw_round_rect(&g, CRect2GpRect(m_items[i]->m_r), Gdiplus::Color(255,225,0,0), gcr_over_fill, 2);

				//CGdiplusBitmap img;
				//img.round_shadow_rect(m_items[i]->r.Width(), m_items[i]->r.Height(), 4);
				//img.draw(&g, m_items[i]->r, true);
			}
			else
			{
				dc.SetTextColor(m_theme.cr_text.ToCOLORREF());
			}

			CRect rText = m_items[i]->m_r;

			//�޴� �������� �׸���
			if (m_items[i]->m_icon.is_valid())
			{
				CRect rIcon = m_items[i]->m_r;
				rIcon.right = rIcon.left + 28;
				rIcon = get_center_rect(rIcon, m_items[i]->m_icon.width, m_items[i]->m_icon.height);
				m_items[i]->m_icon.draw(g, rIcon.left, rIcon.top);
			}

			//�޴� ������ ������ ������� ������ ������ŭ ������ �ش�.
			rText.left = rText.left + 28 + 4;

			//�޴� ĸ�� ǥ��
			dc.DrawText(m_items[i]->m_caption, rText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			//sub button���� �����ϸ� �׷��ش�.
			CRect r = m_items[i]->m_r;
			//r.top += 2;
			//�ϳ��� �޴� �׸� ǥ�õǴ� sub button���� r�� ���⼭ ����ؼ� �׸���?
			//���� ������ �������� �� �� �̹� ����� �����ϴ�.
			for (int j = m_items[i]->m_buttons.size() - 1; j >= 0; j--)
			{
				r.right = r.right - 6;
				r.left = r.right - m_items[i]->m_buttons[j]->m_r.Width();
				//sub button�� ������ left, right�� ��Ȯ�� ��������� top, bottom�� �޴��׸�� �����ϰ� �Ѵ�.
				//�׷��� ��ư�� Ŭ���� �� ��ư���� �ణ �� �Ǵ� �Ʒ��� ������ Ŭ������ ó���ǹǷ�
				//����ڰ� ��ư �̹����� ��Ȯ�� Ŭ���ؾ��ϴ� �δ��� ����.
				m_items[i]->m_buttons[j]->m_r = r;
				m_items[i]->m_buttons[j]->m_button_image[m_items[i]->m_buttons[j]->m_state]->draw(g, r);

				//���� sub button �տ� ���� �ٸ� ��ư�� �����Ѵٸ� r�� �������� shift��Ű�� ��� �׷��ش�.
				if (j > 0)
					r.OffsetRect(-m_items[i]->m_buttons[j]->m_r.Width() - 2, 0);
			}

			//����Ű ������ �ִٸ� ǥ���ϰ�
			if (!m_items[i]->m_hot_key.IsEmpty())
			{
				//sub button�� ���� �޴��׸��� ���
				if (r == m_items[i]->m_r)
					rText.right = r.right - 6;
				//sub button�� ���� ���
				else
					rText.right = r.left - 8;

				//DrawRectangle(&dc, rText);
				dc.DrawText(m_items[i]->m_hot_key, rText, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			}
		}
	}

	dc.SelectObject(pOldFont);
}

void CSCMenu::set_back_image(CGdiplusBitmap* img)
{
	m_img_back.release();
	if (img == NULL)
		return;

	img->deep_copy(&m_img_back);
	Invalidate();
}
