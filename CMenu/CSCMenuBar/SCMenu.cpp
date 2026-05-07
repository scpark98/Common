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
	m_button_image[0] = new CSCGdiplusBitmap(_T("PNG"), _id);	//normal
	m_button_image[1] = new CSCGdiplusBitmap(_T("PNG"), _id);	//selected
	m_button_image[0]->gray();

	//기존 버튼 이미지 컬러톤이 cyan 톤이었으나 코드상에서 다른 색상 톤으로 변경 가능
	m_button_image[1]->adjust_hsl(70);

	//버튼 이미지의 크기는 menu_height의 80% 크기의 높이를 가지도록 조정한다.
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

		//menu_height의 90% 크기의 높이를 가지도록 조정한다.
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
}

CSCMenu::~CSCMenu()
{
	clear();
}

void CSCMenu::clear()
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
	m_items.clear();
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
	ON_WM_TIMER()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

void CSCMenu::add(int _id, CString _caption, UINT icon_id, CString _hot_key)
{
	CSCMenuItem* item = new CSCMenuItem(_id, _caption, icon_id, _hot_key);
	m_items.push_back(item);
	recalc_items_rect();
}

void CSCMenu::add_thumbnail_item(int _id, CString _primary, CString _secondary,
								 CString thumbnail_res_type, UINT thumbnail_res_id)
{
	CSCMenuItem* item = new CSCMenuItem(_id, _primary);
	item->m_type = CSCMenuItem::item_thumbnail;
	item->m_sub_caption = _secondary;
	if (thumbnail_res_id > 0)
		item->m_thumbnail.load(thumbnail_res_type, thumbnail_res_id);
	m_items.push_back(item);
	recalc_items_rect();
}

void CSCMenu::add_thumbnail_item(int _id, CString _primary, CString _secondary,
								 CSCGdiplusBitmap* thumbnail)
{
	CSCMenuItem* item = new CSCMenuItem(_id, _primary);
	item->m_type = CSCMenuItem::item_thumbnail;
	item->m_sub_caption = _secondary;
	if (thumbnail)
		thumbnail->deep_copy(&item->m_thumbnail);
	m_items.push_back(item);
	recalc_items_rect();
}

void CSCMenu::add_submenu_item(int _id, CString _caption, CSCMenu* sub_menu)
{
	CSCMenuItem* item = new CSCMenuItem(_id, _caption);
	item->m_type = CSCMenuItem::item_submenu;
	item->m_sub_menu = sub_menu;
	if (sub_menu)
		sub_menu->m_parent_menu = this;
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

void CSCMenu::enable_item(int menu_id, bool enabled)
{
	CSCMenuItem* item = get_menu_item(menu_id);
	if (!item)
		return;
	item->m_enabled = enabled;
	if (m_hWnd)
		Invalidate();
}

void CSCMenu::check_item(int menu_id, bool checked)
{
	CSCMenuItem* item = get_menu_item(menu_id);
	if (!item)
		return;
	item->m_checked = checked;
	if (m_hWnd)
		Invalidate();
}

void CSCMenu::check_radio_item(int first_id, int last_id, int selected_id)
{
	for (auto* item : m_items)
	{
		if (item->m_id < first_id || item->m_id > last_id)
			continue;
		item->m_checked = (item->m_id == selected_id);
	}
	if (m_hWnd)
		Invalidate();
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
		TRACE(_T("CSCMenu::PreTranslateMessage() WM_KEYDOWN\n"));
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

	//부모가 우리를 강제 hide 시킨 경우 — cascade/message 까지 가면 부모도 닫혀버리므로 skip.
	if (m_suppress_cascade_hide)
	{
		m_suppress_cascade_hide = false;
		return;
	}

	//focus 가 우리 메뉴 체인 내부 (자식 popup, 부모 복귀 등) 로 이동했으면 그대로 유지.
	if (is_in_menu_chain(pNewWnd))
		return;

	//외부로 focus 가 갔으면 우리 + 보이는 자식 + 조상 모두 닫는다.
	hide_visible_descendants();
	ShowWindow(SW_HIDE);
	hide_visible_ancestors();		//focus 가 외부로 나가면 전체 체인 종료.

	::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
		(WPARAM)&CSCMenuMessage(this, message_scmenu_hide, NULL), 0);
}

bool CSCMenu::is_in_menu_chain(CWnd* pWnd)
{
	if (!pWnd || !pWnd->m_hWnd)
		return false;

	CSCMenu* top = this;
	while (top->m_parent_menu)
		top = top->m_parent_menu;

	return top->contains_descendant_hwnd(pWnd->m_hWnd);
}

bool CSCMenu::contains_descendant_hwnd(HWND hwnd)
{
	if (m_hWnd == hwnd)
		return true;

	for (auto* item : m_items)
	{
		if (item->m_type == CSCMenuItem::item_submenu && item->m_sub_menu)
		{
			if (item->m_sub_menu->contains_descendant_hwnd(hwnd))
				return true;
		}
	}
	return false;
}

void CSCMenu::hide_visible_descendants()
{
	for (auto* item : m_items)
	{
		if (item->m_type != CSCMenuItem::item_submenu || !item->m_sub_menu)
			continue;
		if (!item->m_sub_menu->IsWindowVisible())
			continue;

		item->m_sub_menu->hide_visible_descendants();
		item->m_sub_menu->m_suppress_cascade_hide = true;
		item->m_sub_menu->ShowWindow(SW_HIDE);
	}
}

void CSCMenu::hide_visible_ancestors()
{
	CSCMenu* p = m_parent_menu;
	while (p)
	{
		if (p->IsWindowVisible())
		{
			p->m_suppress_cascade_hide = true;
			p->ShowWindow(SW_HIDE);
		}
		p = p->m_parent_menu;
	}
}


void CSCMenu::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	//스크롤 화살표 영역 클릭 — 한 페이지의 절반 만큼 이동.
	if (m_scrollable)
	{
		CRect rc;
		GetClientRect(rc);
		int viewport_h = rc.Height() - 2 * scroll_arrow_h;
		int page_step = max(m_line_height, viewport_h / 2);

		if (get_top_arrow_rect().PtInRect(point))
		{
			scroll_by(-page_step);
			return;
		}
		if (get_bottom_arrow_rect().PtInRect(point))
		{
			scroll_by(page_step);
			return;
		}
	}

	//항목 hit-test 는 컨텐츠 좌표로.
	CPoint pt = client_to_content(point);

	//클릭된 위치가 메뉴의 sub button일 경우
	bool found = false;

	for (int i = 0; i < m_items.size(); i++)
	{
		if (m_items[i]->m_id <= 0)
			continue;

		for (int j = 0; j < m_items[i]->m_buttons.size(); j++)
		{
			if (m_items[i]->m_buttons[j]->m_r.PtInRect(pt))
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
		else if (m_items[i]->m_r.PtInRect(pt))
		{
			//disabled 항목은 아무 동작 안 함.
			if (!m_items[i]->m_enabled)
				return;

			if (m_items[i]->m_type == CSCMenuItem::item_submenu && m_items[i]->m_sub_menu)
			{
				CRect rItem = m_items[i]->m_r;
				rItem.OffsetRect(0, view_dy());		//submenu 는 보이는 항목 위치 옆에서 popup.
				ClientToScreen(rItem);
				m_items[i]->m_sub_menu->popup_menu(rItem.right - 4, rItem.top);
				return;
			}

			::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
				(WPARAM)&CSCMenuMessage(this, message_scmenu_selchanged, m_items[i]), 0);
			ShowWindow(SW_HIDE);
			hide_visible_ancestors();		//다단 nested 메뉴 — root 까지 모두 닫음.
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
	//pt 는 컨텐츠 좌표.
	for (int i = 0; i < m_items.size(); i++)
	{
		if (m_items[i]->m_id > 0 && m_items[i]->m_r.PtInRect(pt))
			return i;
	}

	return -1;
}

static const UINT_PTR timer_submenu_hover = 0x10001;
static const UINT_PTR timer_scroll_auto = 0x10002;
static const int submenu_hover_delay_ms = 300;
static const int scroll_auto_interval_ms = 30;
static const int scroll_auto_step_px = 4;

void CSCMenu::OnMouseMove(UINT nFlags, CPoint point)
{
	//스크롤 가능 + 화살표 위 hover → 자동 스크롤 타이머. 그 외는 타이머 종료.
	if (m_scrollable)
	{
		int dir = 0;
		if (get_top_arrow_rect().PtInRect(point))
			dir = -1;
		else if (get_bottom_arrow_rect().PtInRect(point))
			dir = 1;

		if (dir != m_auto_scroll_dir)
		{
			m_auto_scroll_dir = dir;
			KillTimer(timer_scroll_auto);
			if (dir != 0)
				SetTimer(timer_scroll_auto, scroll_auto_interval_ms, NULL);
		}

		if (dir != 0)
		{
			//화살표 위에선 hover 항목 없음.
			if (m_over_item != -1)
			{
				m_over_item = -1;
				Invalidate(false);
			}
			CDialogEx::OnMouseMove(nFlags, point);
			return;
		}
	}

	if (m_use_over)
	{
		CPoint pt = client_to_content(point);
		int over_item = get_item_index(pt);
		if (over_item != m_over_item)
		{
			m_over_item = over_item;
			Invalidate(false);

			KillTimer(timer_submenu_hover);

			//hover 가 형제 항목으로 이동했으면 떠있던 자식 submenu 닫기.
			//자식이 hide 되며 focus 가 test dialog 로 빠져나가면 부모(자기)도 외부 클릭 시 KillFocus 를 못 받아 안 닫힌다 — SetFocus 로 회수.
			bool dismissed_any = false;
			for (auto* item : m_items)
			{
				if (item->m_type != CSCMenuItem::item_submenu || !item->m_sub_menu)
					continue;
				if (!item->m_sub_menu->IsWindowVisible())
					continue;
				if (over_item < 0 || m_items[over_item] != item)
				{
					item->m_sub_menu->m_suppress_cascade_hide = true;
					item->m_sub_menu->ShowWindow(SW_HIDE);
					dismissed_any = true;
				}
			}
			if (dismissed_any)
				SetFocus();

			if (over_item >= 0 && m_items[over_item]->m_type == CSCMenuItem::item_submenu)
				SetTimer(timer_submenu_hover, submenu_hover_delay_ms, NULL);
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CSCMenu::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == timer_submenu_hover)
	{
		KillTimer(timer_submenu_hover);

		if (m_over_item < 0)
			return;

		CSCMenuItem* item = m_items[m_over_item];
		if (item->m_type != CSCMenuItem::item_submenu || !item->m_sub_menu)
			return;
		if (item->m_sub_menu->IsWindowVisible())
			return;

		CRect rItem = item->m_r;
		rItem.OffsetRect(0, view_dy());
		ClientToScreen(rItem);
		item->m_sub_menu->popup_menu(rItem.right - 4, rItem.top);
		return;
	}

	if (nIDEvent == timer_scroll_auto)
	{
		if (m_auto_scroll_dir == 0)
		{
			KillTimer(timer_scroll_auto);
			return;
		}
		scroll_by(m_auto_scroll_dir * scroll_auto_step_px);
		//경계 도달 시 더 이상 스크롤 안 됨 — 타이머 유지하되 무해 (다음 mousemove 에서 정리).
		return;
	}

	CDialogEx::OnTimer(nIDEvent);
}

BOOL CSCMenu::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_scrollable)
	{
		//WHEEL_DELTA = 120 한 노치 당 약 3 라인.
		int dy = -zDelta * m_line_height * 3 / WHEEL_DELTA;
		scroll_by(dy);
		return TRUE;
	}
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}


void CSCMenu::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
}

bool CSCMenu::create(CWnd* parent, int width)
{
	m_parent = parent;
	m_min_width = width;

	DWORD dwStyle = WS_POPUP | WS_BORDER;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCMenu");
	AfxRegisterClass(&wc);

	//CString class_name = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)::GetStockObject(WHITE_BRUSH), NULL);

	//CreateEx()에서 첫번째 파라미터를 0으로 준 경우는 class_name도 NULL로 줘도 동작했으나 WS_EX_...스타일이 있을 경우는 NULL로 하면 에러 발생.
	//height는 자동 recalc됨.
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

		_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), _T("Segoe UI"));
		m_lf.lfCharSet = DEFAULT_CHARSET;

		ReconstructFont();
	}

	return res;
}

//load from menu resource
void CSCMenu::load(UINT resource_id, int idx0, int idx1)
{
	CMenu menu;
	menu.LoadMenu(resource_id);
	CMenu* pMenu = menu.GetSubMenu(idx0);
	if (idx1 >= 0 && pMenu)
		pMenu = pMenu->GetSubMenu(idx1);
	load_from_menu(pMenu);
}

void CSCMenu::set_thumbnail_size(int w, int h)
{
	m_thumb_w = w;
	m_thumb_h = h;
	recalc_items_rect();
	if (m_hWnd)
		Invalidate();
}

void CSCMenu::load_from_menu(CMenu* pMenu)
{
	if (!pMenu)
		return;

	int i, nCount = GetMenuItemCount(pMenu->m_hMenu);
	UINT menu_id;
	CString menu_caption;

	for (i = 0; i < nCount; i++)
	{
		get_menu_item_info(pMenu->m_hMenu, i, &menu_id, &menu_caption, TRUE);

		//seperator와 일반 메뉴항목만 허용(서브 팝업메뉴 항목은 배제 — 호출자가 add_submenu_item 으로 chain).
		if (menu_id == 0 || (menu_id >= 0x8000 && menu_id <= 0xDFFF))
		{
			std::deque<CString> token;
			get_token_str(menu_caption, token, _T("\t"));

			CString caption = (token.size() >= 1 ? token[0] : menu_caption);
			CString hot_key = (token.size() >= 2 ? token[1] : CString());
			add(menu_id, caption, 0, hot_key);
		}
	}
}

//모니터 영역을 벗어나지 않도록 보정한 후 표시해야 한다.
void CSCMenu::popup_menu(int x, int y)
{
	if (m_items.size() == 0)
		return;

	m_over_item = -1;
	m_scroll_offset = 0;
	m_scrollable = false;
	m_max_scroll = 0;
	m_auto_scroll_dir = 0;
	KillTimer(0x10002);		//timer_scroll_auto

	recalc_items_rect();


	//팝업메뉴가 표시될 때마다 매번 모니터 정보를 새로 읽어줘야 한다.
	//그래야 멀티모니터, 모니터 영역 변경 등의 설정에 맞춰 올바르게 표시된다.
	enum_display_monitors();

	CRect rw, rMonitor, rc;

	GetWindowRect(rw);
	GetClientRect(rc);
	CSize border((rw.Width() - rc.Width()) / 2, (rw.Height() - rc.Height()) / 2);

	int idx = get_monitor_index(x, y);
	if (idx < 0)
	{
		//이 블록은 실행되는 일이 없어야 한다.
		return;
	}

	rw.MoveToXY(x, y);
	rMonitor = g_monitors[idx].rMonitor;

	//모니터 영역을 벗어나는 경우 모니터의 right or bottom에서 10정도는 떨어뜨린다.
	rMonitor.DeflateRect(10, 10);

	//컨텐츠가 모니터 가용 높이보다 크면 윈도우를 모니터 높이로 클램프 + 스크롤 모드 활성.
	int max_h = rMonitor.Height();
	if (rw.Height() > max_h)
	{
		rw.bottom = rw.top + max_h;
		m_scrollable = true;
		int client_h = max_h - border.cy * 2;
		int viewport_h = client_h - 2 * scroll_arrow_h;
		m_max_scroll = max(0, m_content_h - viewport_h);
	}

	adjust_rect_range(rw, rMonitor, true, true);

	MoveWindow(rw);

	//CenterWindow();
	ShowWindow(SW_SHOW);
}

CRect CSCMenu::get_top_arrow_rect() const
{
	if (!m_scrollable)
		return CRect();
	CRect rc;
	GetClientRect(&rc);
	return CRect(0, 0, rc.right, scroll_arrow_h);
}

CRect CSCMenu::get_bottom_arrow_rect() const
{
	if (!m_scrollable)
		return CRect();
	CRect rc;
	GetClientRect(&rc);
	return CRect(0, rc.bottom - scroll_arrow_h, rc.right, rc.bottom);
}

void CSCMenu::scroll_by(int dy)
{
	if (!m_scrollable || dy == 0)
		return;
	int prev = m_scroll_offset;
	m_scroll_offset = max(0, min(m_max_scroll, m_scroll_offset + dy));
	if (m_scroll_offset != prev)
		Invalidate(false);
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
	CSize	border;
	CRect	rw, rc;

	//GetWindowRect()가 아닌 GetClientRect()를 한 후 밑에서 MoveWindow()하면
	//윈도우 크기는 점점 줄어들게 된다. 윈도우의 border 효과 때문에
	//보이는 크기보다 실제 영역은 더 크다.
	GetWindowRect(rw);
	GetClientRect(rc);
	border = CSize((rw.Width() - rc.Width()) / 2, (rw.Height() - rc.Height()) / 2);

	ScreenToClient(rw);
	rw.MoveToXY(0, 0);

	//width 측정 패스 — 가장 긴 항목 너비를 산출 후 윈도우 너비를 max(min_width, 측정+padding) 으로 확장.
	int max_content_w = 0;
	{
		CClientDC dc_m(this);
		CFont* old_f = dc_m.SelectObject(&m_font);

		for (int i = 0; i < m_items.size(); i++)
		{
			if (m_items[i]->m_id <= 0)
				continue;

			int item_w = 0;
			if (m_items[i]->m_type == CSCMenuItem::item_thumbnail)
			{
				CSize sz_p = dc_m.GetTextExtent(m_items[i]->m_caption);
				CSize sz_s = m_items[i]->m_sub_caption.IsEmpty() ? CSize(0, 0) : dc_m.GetTextExtent(m_items[i]->m_sub_caption);
				int text_w = max(sz_p.cx, sz_s.cx);
				item_w = 4 + m_thumb_w + 6 + text_w + 6;
			}
			else
			{
				CSize sz_c = dc_m.GetTextExtent(m_items[i]->m_caption);
				item_w = 28 + 4 + sz_c.cx;

				int trailing = 12;
				if (!m_items[i]->m_hot_key.IsEmpty())
				{
					CSize sz_h = dc_m.GetTextExtent(m_items[i]->m_hot_key);
					trailing = max(trailing, sz_h.cx + 16);
				}
				if (m_items[i]->m_type == CSCMenuItem::item_submenu)
					trailing = max(trailing, 28);

				int btn_block = 0;
				for (auto* btn : m_items[i]->m_buttons)
					btn_block += 6 + btn->m_button_image[0]->width;
				if (btn_block > 0)
					btn_block += 8;

				item_w += trailing + btn_block;
			}

			max_content_w = max(max_content_w, item_w);
		}

		dc_m.SelectObject(old_f);
	}

	//min_width 는 윈도우 전체 너비. 컨텐츠 너비 비교는 동일 기준으로.
	int needed_w = max(m_min_width, max_content_w + 8 + 8 + border.cx * 2);
	rw.right = rw.left + needed_w;

	//썸네일 항목 height = 위 padding(4) + 썸네일 height + 아래 padding(4).
	//크기 변경: set_thumbnail_size(w, h) 로 m_thumb_w/m_thumb_h 설정.
	const int thumbnail_item_h = 4 + m_thumb_h + 4;

	//item rect의 크기는 border 유무에 따라 다를 수 있다.
	for (int i = 0; i < m_items.size(); i++)
	{
		is_separator = (m_items[i]->m_id <= 0);

		int item_h;
		if (is_separator)
			item_h = 5;
		else if (m_items[i]->m_type == CSCMenuItem::item_thumbnail)
			item_h = thumbnail_item_h;
		else
			item_h = m_line_height;

		m_items[i]->m_r = CRect(4, sy, rw.right - 5 - border.cx, sy + item_h);
		sy = m_items[i]->m_r.bottom + 2;

		total_height = m_items[i]->m_r.bottom;
	}

	m_content_h = total_height;

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

	//스크롤 모드: 항목 그리기는 viewport 영역에 clip. (화살표 영역은 항목과 분리)
	int vp_top = m_scrollable ? scroll_arrow_h : 0;
	int vp_bot = m_scrollable ? rc.bottom - scroll_arrow_h : rc.bottom;
	int dy = view_dy();

	if (m_scrollable)
	{
		CRgn rgn_vp;
		rgn_vp.CreateRectRgn(0, vp_top, rc.right, vp_bot);
		dc.SelectClipRgn(&rgn_vp);
	}

	for (int i = 0; i < m_items.size(); i++)
	{
		CRect view_r = m_items[i]->m_r;
		view_r.OffsetRect(0, dy);

		//viewport 밖이면 skip (성능 + 인접 영역 침범 방지).
		if (view_r.bottom < vp_top || view_r.top > vp_bot)
			continue;

		//separator의 경우
		if (m_items[i]->m_id <= 0)
		{
			draw_line(&dc, view_r.left, view_r.CenterPoint().y, view_r.right, view_r.CenterPoint().y, GRAY(214));
		}
		//일반 메뉴 항목인 경우
		else
		{
			//disabled 항목은 hover 무시 + dim 색상.
			if (!m_items[i]->m_enabled)
			{
				dc.SetTextColor(m_theme.cr_text_dim.ToCOLORREF());
			}
			//선택된 항목 표시
			else if (i == m_over_item)
			{
				dc.SetTextColor(m_theme.cr_text_hover.ToCOLORREF());
				dc.FillSolidRect(view_r, m_theme.cr_back_hover.ToCOLORREF());
			}
			else
			{
				dc.SetTextColor(m_theme.cr_text.ToCOLORREF());
			}

			//썸네일 항목: 좌측 썸네일(80x45 영역 안에 center) + 우측 primary/secondary 2줄 텍스트.
			//hover 색은 위에서 이미 적용됨. sub button/hot_key 는 없으므로 continue.
			if (m_items[i]->m_type == CSCMenuItem::item_thumbnail)
			{
				CRect rThumb = view_r;
				rThumb.DeflateRect(4, 4);
				rThumb.right = rThumb.left + m_thumb_w;

				if (m_items[i]->m_thumbnail.is_valid())
					m_items[i]->m_thumbnail.draw(g, rThumb);

				CRect rTextArea = view_r;
				rTextArea.left = rThumb.right + 6;
				rTextArea.right -= 6;
				rTextArea.DeflateRect(0, 4);

				if (m_items[i]->m_sub_caption.IsEmpty())
				{
					dc.DrawText(m_items[i]->m_caption, rTextArea, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				}
				else
				{
					CRect rPrimary = rTextArea;
					rPrimary.bottom = rTextArea.top + rTextArea.Height() / 2;
					dc.DrawText(m_items[i]->m_caption, rPrimary, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

					CRect rSecondary = rTextArea;
					rSecondary.top = rPrimary.bottom;
					COLORREF prev_text_color = dc.GetTextColor();
					dc.SetTextColor(m_theme.cr_text_dim.ToCOLORREF());
					dc.DrawText(m_items[i]->m_sub_caption, rSecondary, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
					dc.SetTextColor(prev_text_color);
				}

				continue;
			}

			CRect rText = view_r;

			//메뉴 아이콘을 그리고
			if (m_items[i]->m_icon.is_valid())
			{
				CRect rIcon = view_r;
				rIcon.right = rIcon.left + 28;
				rIcon = get_center_rect(rIcon, m_items[i]->m_icon.width, m_items[i]->m_icon.height);
				m_items[i]->m_icon.draw(g, rIcon.left, rIcon.top);
			}
			//checked 표시 (icon 영역 한가운데). icon 과 m_checked 동시 사용 시에는 icon 이 먼저 그려진 후 dot 이 위에 덮어써짐 — 일반적으로 checkable 항목엔 icon 안 둠.
			else if (m_items[i]->m_checked)
			{
				CPoint cp = view_r.CenterPoint();
				cp.x = view_r.left + 14;
				Gdiplus::Color cr_check = m_items[i]->m_enabled ? m_theme.cr_text : m_theme.cr_text_dim;
				Gdiplus::SolidBrush br(cr_check);
				g.FillEllipse(&br, cp.x - 3, cp.y - 3, 6, 6);
			}

			//메뉴 아이콘 유무에 관계없이 아이콘 영역만큼 공백을 준다.
			rText.left = rText.left + 28 + 4;

			//submenu '>' 영역과 겹치지 않도록 caption right 제한.
			if (m_items[i]->m_type == CSCMenuItem::item_submenu)
				rText.right = view_r.right - 22;

			//메뉴 캡션 표시
			dc.DrawText(m_items[i]->m_caption, rText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			//sub button: 컨텐츠 좌표는 m_r 에 저장 (hit test용), 그리기는 view 좌표.
			CRect r_content = m_items[i]->m_r;
			CRect r_view = view_r;
			for (int j = m_items[i]->m_buttons.size() - 1; j >= 0; j--)
			{
				int btn_w = m_items[i]->m_buttons[j]->m_r.Width();
				r_content.right -= 6;
				r_view.right -= 6;
				r_content.left = r_content.right - btn_w;
				r_view.left = r_view.right - btn_w;
				//sub button의 영역은 left, right는 정확히 줘야하지만 top, bottom은 메뉴항목과 동일하게 한다.
				//그래야 버튼을 클릭할 때 버튼보다 약간 위 또는 아래를 눌러도 클릭으로 처리되므로
				//사용자가 버튼 이미지를 정확히 클릭해야하는 부담이 없다.
				m_items[i]->m_buttons[j]->m_r = r_content;
				m_items[i]->m_buttons[j]->m_button_image[m_items[i]->m_buttons[j]->m_state]->draw(g, r_view);

				if (j > 0)
				{
					r_content.OffsetRect(-btn_w - 2, 0);
					r_view.OffsetRect(-btn_w - 2, 0);
				}
			}

			//단축키 정보가 있다면 표시하고
			if (!m_items[i]->m_hot_key.IsEmpty())
			{
				//sub button이 없는 메뉴항목일 경우
				if (r_content == m_items[i]->m_r)
					rText.right = r_view.right - 6;
				//sub button이 있을 경우
				else
					rText.right = r_view.left - 8;

				dc.DrawText(m_items[i]->m_hot_key, rText, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			}

			if (m_items[i]->m_type == CSCMenuItem::item_submenu)
			{
				CRect rArrow = view_r;
				rArrow.right -= 6;
				dc.DrawText(_T(">"), rArrow, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			}
		}
	}

	if (m_scrollable)
	{
		dc.SelectClipRgn(NULL);

		//스크롤 화살표 영역 — back 색으로 채우고 위/아래 삼각형. 비활성 (boundary) 이면 dim.
		dc.FillSolidRect(0, 0, rc.right, scroll_arrow_h, m_theme.cr_back.ToCOLORREF());
		dc.FillSolidRect(0, rc.bottom - scroll_arrow_h, rc.right, rc.bottom, m_theme.cr_back.ToCOLORREF());

		bool top_active = m_scroll_offset > 0;
		bool bot_active = m_scroll_offset < m_max_scroll;

		Gdiplus::Color cr_top = top_active ? m_theme.cr_text : m_theme.cr_text_dim;
		Gdiplus::Color cr_bot = bot_active ? m_theme.cr_text : m_theme.cr_text_dim;

		Gdiplus::SolidBrush br_top(cr_top);
		Gdiplus::SolidBrush br_bot(cr_bot);

		int cx = rc.right / 2;
		int tri_w = 8, tri_h = 5;

		Gdiplus::PointF pts_top[3] = {
			Gdiplus::PointF((float)cx, (float)(scroll_arrow_h / 2 - tri_h / 2)),
			Gdiplus::PointF((float)(cx - tri_w), (float)(scroll_arrow_h / 2 + tri_h / 2)),
			Gdiplus::PointF((float)(cx + tri_w), (float)(scroll_arrow_h / 2 + tri_h / 2)),
		};
		g.FillPolygon(&br_top, pts_top, 3);

		int by = rc.bottom - scroll_arrow_h / 2;
		Gdiplus::PointF pts_bot[3] = {
			Gdiplus::PointF((float)cx, (float)(by + tri_h / 2)),
			Gdiplus::PointF((float)(cx - tri_w), (float)(by - tri_h / 2)),
			Gdiplus::PointF((float)(cx + tri_w), (float)(by - tri_h / 2)),
		};
		g.FillPolygon(&br_bot, pts_bot, 3);
	}

	dc.SelectObject(pOldFont);
}

void CSCMenu::set_back_image(CSCGdiplusBitmap* img)
{
	m_img_back.release();
	if (img == NULL)
		return;

	img->deep_copy(&m_img_back);
	Invalidate();
}
