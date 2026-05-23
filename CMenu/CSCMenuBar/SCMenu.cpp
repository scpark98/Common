// SCMenu.cpp : implementation file
//

#include "SCMenu.h"

#include "../../Functions.h"
#include "../../colors.h"
#include "../../MemoryDC.h"
#include "../../log/SCLog/SCLog.h"		//logWrite — *.rc placeholder / caption 매칭 진단용.
												//※ 메뉴 작업 정상 완료 후 logWrite 호출은 모두 제거 또는 TRACE 로 변경 예정.

//timer ID 와 hover/scroll 상수들 — set_over_item / OnMouseMove / OnTimer 등 여러 함수에서 사용. 파일 상단으로 이동.
static const UINT_PTR timer_submenu_hover = 0x10001;
static const UINT_PTR timer_scroll_auto = 0x10002;
static const int submenu_hover_delay_ms = 300;
static const int scroll_auto_interval_ms = 30;
static const int scroll_auto_step_px = 4;

//WS_EX_NOACTIVATE 팝업이라 KillFocus 만으로 외부 클릭 dismiss 가 불완전 — main dlg client 의 빈 영역을 클릭하면
//focus 변동 없이 active 상태도 그대로라 KillFocus 가 안 불린다. thread-local mouse hook 으로 능동 감지.
static HHOOK s_menu_mouse_hook = NULL;
static CSCMenu* s_menu_root = NULL;

LRESULT CALLBACK CSCMenu::menu_mouse_hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && s_menu_root)
	{
		const bool is_press =
			wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN ||
			wParam == WM_NCLBUTTONDOWN || wParam == WM_NCRBUTTONDOWN || wParam == WM_NCMBUTTONDOWN;
		if (is_press)
		{
			MOUSEHOOKSTRUCT* p = (MOUSEHOOKSTRUCT*)lParam;
			HWND hwnd_at = ::WindowFromPoint(p->pt);
			if (!s_menu_root->contains_descendant_hwnd(hwnd_at))
			{
				CSCMenu* root = s_menu_root;
				root->hide_visible_descendants();
				if (::IsWindow(root->m_hWnd))
					root->ShowWindow(SW_HIDE);
				if (root->m_parent && ::IsWindow(root->m_parent->m_hWnd))
					::SendMessage(root->m_parent->m_hWnd, Message_CSCMenu,
						(WPARAM)&CSCMenuMessage(root, message_scmenu_hide, NULL), 0);
			}
		}
	}
	return ::CallNextHookEx(NULL, nCode, wParam, lParam);
}

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
	m_hot_key = hot_key;
	m_menu_height = (menu_height > 0 ? menu_height : 22);

	//caption 의 access-key 마커 처리:
	//- '&&' → '&' literal
	//- '&X' (X 가 alphanumeric, 최초 1회) → X 추출 후 m_access_key = X 위치
	//- '&' 다음이 alphanumeric 이 아닌 글자 (space, punctuation 등) → literal '&' (GetMenuItemInfo 가 '&&' 를
	//  '&' 로 collapse 하는 케이스 방어 — 예: '팬 && 스캔' → '팬 & 스캔' → '& ' 가 access-key 가 아닌 literal '&').
	//- 끝의 단독 '&' → literal '&'
	m_access_key = -1;
	CString cleaned;
	int n = caption.GetLength();
	for (int i = 0; i < n; i++)
	{
		TCHAR c = caption[i];
		if (c == _T('&'))
		{
			if (i + 1 < n && caption[i + 1] == _T('&'))
			{
				cleaned += _T('&');
				i++;	//두 번째 '&' skip
			}
			else if (i + 1 < n && _istalnum((_TUCHAR)caption[i + 1]) && m_access_key < 0)
			{
				m_access_key = cleaned.GetLength();
				cleaned += caption[i + 1];
				i++;	//access key 글자는 이미 추가했으므로 한 번 더 skip
			}
			else
			{
				cleaned += _T('&');	//literal '&' — 다음이 non-alnum 이거나 끝.
			}
		}
		else
		{
			cleaned += c;
		}
	}
	m_caption = cleaned;

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

void CSCMenu::remove_item(int menu_id)
{
	for (auto it = m_items.begin(); it != m_items.end(); ++it)
	{
		if ((*it)->m_id != menu_id)
			continue;
		for (auto* btn : (*it)->m_buttons)
			delete btn;
		(*it)->m_buttons.clear();
		delete *it;
		m_items.erase(it);
		recalc_items_rect();
		return;
	}
}

void CSCMenu::move_item_after(int id, int after_id)
{
	int src = -1, dst = -1;
	for (size_t i = 0; i < m_items.size(); i++)
	{
		if (m_items[i]->m_id == id) src = (int)i;
		if (m_items[i]->m_id == after_id) dst = (int)i;
	}
	if (src < 0 || dst < 0 || src == dst)
		return;

	CSCMenuItem* item = m_items[src];
	m_items.erase(m_items.begin() + src);
	//src 가 dst 앞이었으면 erase 후 dst index 가 한 칸 당겨짐.
	if (src < dst)
		dst--;
	m_items.insert(m_items.begin() + dst + 1, item);
	recalc_items_rect();
}

void CSCMenu::move_item_before(int id, int before_id)
{
	int src = -1, dst = -1;
	for (size_t i = 0; i < m_items.size(); i++)
	{
		if (m_items[i]->m_id == id) src = (int)i;
		if (m_items[i]->m_id == before_id) dst = (int)i;
	}
	if (src < 0 || dst < 0 || src == dst)
		return;

	CSCMenuItem* item = m_items[src];
	m_items.erase(m_items.begin() + src);
	if (src < dst)
		dst--;
	m_items.insert(m_items.begin() + dst, item);
	recalc_items_rect();
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
	ON_WM_NCCALCSIZE()
	ON_WM_SHOWWINDOW()
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
	if (pMsg->message == WM_KEYDOWN)
	{
		UINT key = (UINT)pMsg->wParam;
		switch (key)
		{
			case VK_ESCAPE:
				ShowWindow(SW_HIDE);
				if (m_parent && ::IsWindow(m_parent->m_hWnd))
					::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
						(WPARAM)&CSCMenuMessage(this, message_scmenu_hide, NULL), 0);
				return TRUE;

			case VK_DOWN:
				navigate_over(+1);
				return TRUE;

			case VK_UP:
				navigate_over(-1);
				return TRUE;

			case VK_RIGHT:
				//over_item 이 submenu 면 펼치고 그 안으로 focus 이동. 그 외엔 무시.
				if (m_over_item >= 0 && m_over_item < (int)m_items.size() &&
					m_items[m_over_item]->m_type == CSCMenuItem::item_submenu)
				{
					activate_over_item();
				}
				return TRUE;

			case VK_LEFT:
				//parent_menu 가 있으면 자기 hide 후 parent 로 focus 회수 — sub 닫기 동작.
				if (m_parent_menu && ::IsWindow(m_parent_menu->m_hWnd))
				{
					m_suppress_cascade_hide = true;
					ShowWindow(SW_HIDE);
					m_parent_menu->SetFocus();
				}
				return TRUE;

			case VK_RETURN:
			case VK_SPACE:
				activate_over_item();
				return TRUE;

			default:
				//access-key 매칭 — 영문/숫자 키만. case-insensitive.
				if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9'))
				{
					if (handle_access_key((TCHAR)key))
						return TRUE;
				}
				break;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

LRESULT CSCMenu::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	//WS_THICKFRAME 사용 시 NC 영역 활성/비활성 전환 micro-flash 차단.
	//lParam = -1 = title bar 전환 그리기 skip (Endorphin2Dlg 와 동일 트릭). WS_EX_NOACTIVATE 와 함께 적용.
	if (message == WM_NCACTIVATE)
		return DefWindowProc(WM_NCACTIVATE, wParam, -1);

	//WS_THICKFRAME 의 부작용 — 가장자리에서 resize 커서 + drag-resize 가능. popup 메뉴는 resize 불가여야 함.
	//resize 관련 hit-test 결과는 모두 HTCLIENT 로 강제 — 커서 변경·drag 시작 양쪽 다 차단.
	//WS_THICKFRAME 자체는 유지 (invisible NC border 가 popup 레이아웃 계산에 이미 반영됨).
	if (message == WM_NCHITTEST)
	{
		LRESULT hit = DefWindowProc(WM_NCHITTEST, wParam, lParam);
		switch (hit)
		{
			case HTLEFT: case HTRIGHT: case HTTOP: case HTBOTTOM:
			case HTTOPLEFT: case HTTOPRIGHT: case HTBOTTOMLEFT: case HTBOTTOMRIGHT:
			case HTGROWBOX:		//= HTSIZE (alias)
				return HTCLIENT;
		}
		return hit;
	}

	return CDialogEx::WindowProc(message, wParam, lParam);
}


BOOL CSCMenu::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
	//return CDialogEx::OnEraseBkgnd(pDC);
}


void CSCMenu::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	//root popup 이 hide 될 때 mouse hook 해제. 다음 popup 시 popup_menu 가 재설치.
	if (!bShow && !m_parent_menu && this == s_menu_root)
	{
		if (s_menu_mouse_hook)
		{
			::UnhookWindowsHookEx(s_menu_mouse_hook);
			s_menu_mouse_hook = NULL;
		}
		s_menu_root = NULL;
	}
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
	if (::IsWindow(m_hWnd))
		ShowWindow(SW_HIDE);
	hide_visible_ancestors();		//focus 가 외부로 나가면 전체 체인 종료.

	if (m_parent && ::IsWindow(m_parent->m_hWnd))
		::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
			(WPARAM)&CSCMenuMessage(this, message_scmenu_hide, NULL), 0);
}

bool CSCMenu::is_in_menu_chain(CWnd* pWnd)
{
	if (!pWnd || !pWnd->m_hWnd)
		return false;

	//프로그램 종료 시 ancestor 체인의 일부 menu window 가 먼저 destroy 되어 m_hWnd 가 NULL 인 채로
	//C++ 객체만 살아있을 수 있다. 이 경우 ::IsWindow guard 로 traversal 중단.
	CSCMenu* top = this;
	while (top->m_parent_menu && ::IsWindow(top->m_parent_menu->m_hWnd))
		top = top->m_parent_menu;

	return top->contains_descendant_hwnd(pWnd->m_hWnd);
}

bool CSCMenu::contains_descendant_hwnd(HWND hwnd)
{
	if (!::IsWindow(m_hWnd))
		return false;

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
		//종료 단계 등 sub_menu window 가 이미 destroy 된 경우 (m_hWnd=NULL) IsWindowVisible/ShowWindow
		//호출 시 MFC ASSERT — ::IsWindow 로 우회.
		if (!::IsWindow(item->m_sub_menu->m_hWnd))
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
		//종료 단계에 ancestor menu 가 이미 destroy 됐으면 (m_hWnd=NULL) IsWindowVisible/ShowWindow ASSERT.
		if (::IsWindow(p->m_hWnd) && p->IsWindowVisible())
		{
			p->m_suppress_cascade_hide = true;
			p->ShowWindow(SW_HIDE);
		}
		p = p->m_parent_menu;
	}
}

//over_index 의 항목이 submenu 면 부모 메뉴의 window 우측 edge 너머에 popup.
//Win10/11 의 WS_THICKFRAME 은 좌/우/하에 ~7 px invisible border 가 있어 GetWindowRect 가 실제 visible edge
//보다 큰 rect 반환. get_window_real_rect (DwmGetWindowAttribute DWMWA_EXTENDED_FRAME_BOUNDS) 로
//actual visible bounds 사용. Windows 탐색기 / 기본 CMenu 와 동일하게 부모 우측 border 와 ~3 px 겹쳐 표시.
void CSCMenu::popup_submenu_for(int over_index)
{
	if (over_index < 0 || over_index >= (int)m_items.size())
		return;
	CSCMenuItem* item = m_items[over_index];
	if (!item->m_enabled)
		return;	//disabled 항목은 hover/click/keyboard 모든 경로에서 sub popup 차단.
	if (item->m_type != CSCMenuItem::item_submenu || !item->m_sub_menu)
		return;
	if (item->m_sub_menu->IsWindowVisible())
		return;

	CRect rItem = item->m_r;
	rItem.OffsetRect(0, view_dy());
	ClientToScreen(rItem);

	CRect rWin = get_window_real_rect(this);
	item->m_sub_menu->popup_menu(rWin.right - 11, rItem.top);

	m_submenu_ever_opened = true;
}

void CSCMenu::set_over_item(int idx)
{
	if (idx == m_over_item)
		return;

	m_over_item = idx;
	if (m_hWnd)
		Invalidate(FALSE);

	//형제 submenu dismiss — mouse hover 와 동일 처리. 자동 popup 은 안 함 (키보드는 RIGHT/Enter 로 명시 popup).
	KillTimer(timer_submenu_hover);
	for (auto* item : m_items)
	{
		if (item->m_type != CSCMenuItem::item_submenu || !item->m_sub_menu)
			continue;
		if (!::IsWindow(item->m_sub_menu->m_hWnd))
			continue;
		if (!item->m_sub_menu->IsWindowVisible())
			continue;
		if (idx < 0 || m_items[idx] != item)
		{
			item->m_sub_menu->hide_visible_descendants();
			item->m_sub_menu->m_suppress_cascade_hide = true;
			item->m_sub_menu->ShowWindow(SW_HIDE);
		}
	}
}

void CSCMenu::navigate_over(int delta)
{
	int n = (int)m_items.size();
	if (n == 0)
		return;

	//시작 위치 — over_item 이 없으면 delta>0 일 때 -1 (다음이 0), delta<0 일 때 0 (다음이 n-1).
	int idx = (m_over_item < 0) ? (delta > 0 ? -1 : 0) : m_over_item;
	for (int step = 0; step < n; step++)
	{
		idx = (idx + delta + n) % n;
		if (m_items[idx]->m_id <= 0)		//separator skip
			continue;
		if (!m_items[idx]->m_enabled)		//disabled skip
			continue;
		set_over_item(idx);
		m_kb_priority = true;	//mouse 가 실제로 움직일 때까지 hover 갱신 차단.
		return;
	}
}

void CSCMenu::activate_over_item()
{
	if (m_over_item < 0 || m_over_item >= (int)m_items.size())
		return;
	CSCMenuItem* item = m_items[m_over_item];
	if (!item->m_enabled)
		return;

	//submenu 면 펼치고 sub 의 첫 항목 hover 로 focus 이동.
	if (item->m_type == CSCMenuItem::item_submenu && item->m_sub_menu)
	{
		popup_submenu_for(m_over_item);
		CSCMenu* sub = item->m_sub_menu;
		if (sub && ::IsWindow(sub->m_hWnd))
		{
			sub->SetFocus();
			sub->navigate_over(+1);
		}
		return;
	}

	//일반 항목 — parent 에 selchanged 통지 + cascade close.
	if (m_parent && ::IsWindow(m_parent->m_hWnd))
		::SendMessage(m_parent->m_hWnd, Message_CSCMenu,
			(WPARAM)&CSCMenuMessage(this, message_scmenu_selchanged, item), 0);
	ShowWindow(SW_HIDE);
	hide_visible_ancestors();
}

bool CSCMenu::handle_access_key(TCHAR ch)
{
	TCHAR lower = (TCHAR)_totlower(ch);
	for (int i = 0; i < (int)m_items.size(); i++)
	{
		if (m_items[i]->m_access_key < 0)
			continue;
		if (!m_items[i]->m_enabled)
			continue;
		if (m_items[i]->m_access_key >= m_items[i]->m_caption.GetLength())
			continue;
		TCHAR ak = m_items[i]->m_caption[m_items[i]->m_access_key];
		if ((TCHAR)_totlower(ak) == lower)
		{
			set_over_item(i);
			m_kb_priority = true;
			activate_over_item();
			return true;
		}
	}
	return false;
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
				popup_submenu_for(i);
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

void CSCMenu::OnMouseMove(UINT nFlags, CPoint point)
{
	//keyboard nav 직후 동일 좌표 WM_MOUSEMOVE (Invalidate 후 hover refresh 등) 또는 사용자가 키 누르며
	//발생한 hand jitter 가 keyboard hover 를 덮어 m_over_item 이 마우스 아래 항목으로 되돌아가는 증상 차단.
	//- 동일 좌표면 무조건 skip
	//- m_kb_priority 가 set 이면 mouse 가 실제로 움직일 때까지 hover 갱신 안 함. 한 번 다른 좌표가 들어오면 release.
	if (point == m_last_mouse_pt)
		return;
	if (m_kb_priority)
	{
		m_kb_priority = false;	//mouse 가 실제로 움직였으니 mouse 모드로 복귀.
		m_last_mouse_pt = point;
		return;					//이번 한 번은 무시 — 다음 mouse move 부터 정상 처리.
	}
	m_last_mouse_pt = point;

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
				if (!::IsWindow(item->m_sub_menu->m_hWnd))
					continue;
				if (!item->m_sub_menu->IsWindowVisible())
					continue;
				if (over_item < 0 || m_items[over_item] != item)
				{
					item->m_sub_menu->hide_visible_descendants();
					item->m_sub_menu->m_suppress_cascade_hide = true;
					item->m_sub_menu->ShowWindow(SW_HIDE);
					dismissed_any = true;
				}
			}
			if (dismissed_any)
				SetFocus();

			if (over_item >= 0 && m_items[over_item]->m_type == CSCMenuItem::item_submenu)
			{
				//이번 popup session 에서 sub 가 한 번이라도 열렸으면 (m_submenu_ever_opened) 모든 후속 hover 는
				//즉시 전환 — 사용자가 sub 열린 → non-sub 거쳐 → 다른 sub 로 이동해도 delay 없음.
				//첫 hover (sub 가 아직 한 번도 안 열림) 만 hover_delay_ms 후 popup.
				if (dismissed_any || m_submenu_ever_opened)
					popup_submenu_for(over_item);
				else
					SetTimer(timer_submenu_hover, submenu_hover_delay_ms, NULL);
			}
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CSCMenu::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == timer_submenu_hover)
	{
		KillTimer(timer_submenu_hover);
		popup_submenu_for(m_over_item);
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

	DWORD dwStyle = WS_POPUP | WS_THICKFRAME;// WS_BORDER;
	//WS_EX_NOACTIVATE: popup 표시 시 부모 dlg 가 deactivate 되지 않아야 함 (deactivation 시 부모의 NC paint
	//가 흰색 막대 등을 잠시 노출시키는 증상 차단). focus 는 SetFocus 로 명시적 전달 (KillFocus 기반 cascade close 유지).
	//WS_EX_TOOLWINDOW: 작업표시줄 미표시 + 작은 caption 영역 (popup 메뉴에 적합).
	//WS_EX_COMPOSITED: OS-level 더블버퍼링. hbrBackground=NULL 만으로는 DWM 이 backing store 를
	//transparent/garbage 로 잡았다가 OnPaint 가 늦게 덮으면 회색/잔상 flash 가 보이는 경우가 있다.
	//COMPOSITED 는 paint 완료 후 한 번에 compose 해서 그 단계를 사용자에게 노출하지 않는다.
	DWORD dwExStyle = WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_COMPOSITED;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCMenu");
	wc.hbrBackground = NULL;	//#32770 의 default brush (COLOR_3DFACE = 시스템 회색) 제거.
								//OS 가 popup 표시 시 default brush 로 먼저 fill 한 후 OnPaint 가 theme 색으로 덮어 회색 flash 가 보이는 증상 회피.
								//OnEraseBkgnd 가 FALSE 반환하므로 자동 erase 도 안 일어나 flicker 없음.
	AfxRegisterClass(&wc);

	//CString class_name = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)::GetStockObject(WHITE_BRUSH), NULL);

	//CreateEx()에서 첫번째 파라미터를 0으로 준 경우는 class_name도 NULL로 줘도 동작했으나 WS_EX_...스타일이 있을 경우는 NULL로 하면 에러 발생.
	//height는 자동 recalc됨.
	bool res = CreateEx(dwExStyle, wc.lpszClassName, _T("CSCMenu"), dwStyle, CRect(0, 0, width, 300), parent, 0);

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
void CSCMenu::load(UINT resource_id, int idx0, int idx1, bool include_popup_placeholder)
{
	CMenu menu;
	menu.LoadMenu(resource_id);
	CMenu* pMenu = menu.GetSubMenu(idx0);
	if (idx1 >= 0 && pMenu)
		pMenu = pMenu->GetSubMenu(idx1);
	load_from_menu(pMenu, include_popup_placeholder);
}

void CSCMenu::set_thumbnail_size(int w, int h)
{
	m_thumb_w = w;
	m_thumb_h = h;
	recalc_items_rect();
	if (m_hWnd)
		Invalidate();
}

//caption 정규화 — `&` access-key marker 제거 + 좌우 공백 trim. `&&` literal 은 single `&` 로 보존.
//.rc 의 *즐겨찾기(&F)* / *즐겨찾기* / *즐겨찾기(&f)* 모두 동일 매칭 키 (*즐겨찾기*) 가 되도록.
static CString normalize_menu_caption(LPCTSTR src)
{
	CString out;
	if (!src)
		return out;

	CString s(src);
	s.Trim();

	const int len = s.GetLength();
	int i = 0;
	while (i < len)
	{
		TCHAR c = s[i];
		if (c == _T('&'))
		{
			if (i + 1 < len && s[i + 1] == _T('&'))
			{
				out += _T('&');		//`&&` literal — single `&` 로 축약
				i += 2;
			}
			else if (i + 1 >= len || s[i + 1] == _T(' ') || s[i + 1] == _T('\t'))
			{
				//공백 / EOL 앞 `&` — literal (access-key 가 아님).
				out += _T('&');
				i += 1;
			}
			else
			{
				//access-key marker — `&X` (X 가 non-space). `&` skip, X 는 다음 iteration 에서 보존.
				i += 1;
			}
		}
		else
		{
			out += c;
			i += 1;
		}
	}
	return out;
}

//caption 매칭으로 HMENU 의 자식 중 sub-menu (POPUP) HMENU 반환. 못 찾으면 NULL.
static HMENU find_popup_by_caption(HMENU hMenu, LPCTSTR caption)
{
	if (!hMenu || !caption)
		return NULL;

	const CString target = normalize_menu_caption(caption);
	const int n = ::GetMenuItemCount(hMenu);
	for (int i = 0; i < n; i++)
	{
		HMENU hSub = ::GetSubMenu(hMenu, i);
		if (!hSub)
			continue;

		TCHAR buf[256] = { 0 };
		::GetMenuString(hMenu, i, buf, _countof(buf), MF_BYPOSITION);
		CString raw(buf);
		int tab = raw.Find(_T('\t'));
		if (tab >= 0)
			raw = raw.Left(tab);

		if (normalize_menu_caption(raw).Compare(target) == 0)
			return hSub;
	}
	return NULL;
}

void CSCMenu::load_from_menu(CMenu* pMenu, bool include_popup_placeholder)
{
	if (!pMenu)
		return;

	int i, nCount = GetMenuItemCount(pMenu->m_hMenu);
	UINT menu_id;
	CString menu_caption;

	for (i = 0; i < nCount; i++)
	{
		HMENU hSub = ::GetSubMenu(pMenu->m_hMenu, i);
		const bool is_popup = (hSub != NULL);

		//VS resource editor 가 자식 없는 POPUP (Popup=True 지만 빈 BEGIN/END) 을 MENUITEM id=0xFFFF 로
		//down-grade 해서 저장. 사용자 의도는 POPUP 이므로 0xFFFF 도 placeholder 로 인식한다.
		get_menu_item_info(pMenu->m_hMenu, i, &menu_id, &menu_caption, TRUE);
		const bool is_empty_popup_downgrade = !is_popup && (menu_id == 0xFFFF);

		if ((is_popup || is_empty_popup_downgrade) && include_popup_placeholder)
		{
			CString raw;
			if (is_popup)
			{
				TCHAR buf[256] = { 0 };
				::GetMenuString(pMenu->m_hMenu, i, buf, _countof(buf), MF_BYPOSITION);
				raw = buf;
			}
			else
			{
				raw = menu_caption;
			}

			std::deque<CString> token;
			get_token_str(raw, token, _T("\t"));
			CString caption = (token.size() >= 1 ? token[0] : raw);

			//placeholder 용 unique negative id — 일반 menu_id (0x8000~0xDFFF) 와 separator(0) 모두와 충돌 회피.
			//attach_submenu_by_caption 으로 m_sub_menu set 되기 전엔 click 시 m_sub_menu==nullptr 가드로 no-op.
			int placeholder_id = -10000 - (int)m_items.size();
			CSCMenuItem* item = new CSCMenuItem(placeholder_id, caption);
			item->m_type = CSCMenuItem::item_submenu;
			item->m_sub_menu = nullptr;
			m_items.push_back(item);
		}
		else if (!is_popup)
		{
			//seperator와 일반 메뉴항목.
			if (menu_id == 0 || (menu_id >= 0x8000 && menu_id <= 0xDFFF))
			{
				std::deque<CString> token;
				get_token_str(menu_caption, token, _T("\t"));

				CString caption = (token.size() >= 1 ? token[0] : menu_caption);
				CString hot_key = (token.size() >= 2 ? token[1] : CString());
				add(menu_id, caption, 0, hot_key);
			}
		}
		//else: POPUP 인데 include_popup_placeholder=false → skip (기존 동작).
	}
	recalc_items_rect();
}

void CSCMenu::load_by_caption(UINT resource_id, LPCTSTR parent_caption, LPCTSTR target_caption,
							  bool include_popup_placeholder)
{
	CMenu menu;
	if (!menu.LoadMenu(resource_id))
	{
		logWrite(_T("[SCMenu] load_by_caption: LoadMenu(%u) failed"), resource_id);
		return;
	}

	HMENU hTop = menu.m_hMenu;
	HMENU hTarget = NULL;

	if (parent_caption && parent_caption[0] != _T('\0'))
	{
		HMENU hParent = find_popup_by_caption(hTop, parent_caption);
		if (!hParent)
		{
			logWrite(_T("[SCMenu] load_by_caption: parent '%s' not found in resource %u"),
				parent_caption, resource_id);
			return;
		}
		hTarget = find_popup_by_caption(hParent, target_caption);
	}
	else
	{
		hTarget = find_popup_by_caption(hTop, target_caption);
	}

	if (!hTarget)
	{
		logWrite(_T("[SCMenu] load_by_caption: target '%s' not found under '%s'"),
			target_caption, parent_caption ? parent_caption : _T("(top)"));
		return;
	}

	CMenu menu_wrap;
	menu_wrap.Attach(hTarget);
	load_from_menu(&menu_wrap, include_popup_placeholder);
	menu_wrap.Detach();
}

bool CSCMenu::attach_submenu_by_caption(LPCTSTR caption, CSCMenu* sub_menu, int new_id)
{
	const CString target = normalize_menu_caption(caption);
	for (auto* item : m_items)
	{
		if (item->m_type == CSCMenuItem::item_submenu &&
			item->m_sub_menu == nullptr &&
			normalize_menu_caption(item->m_caption).Compare(target) == 0)
		{
			item->m_sub_menu = sub_menu;
			if (sub_menu)
				sub_menu->m_parent_menu = this;
			if (new_id != 0)
				item->m_id = new_id;
			return true;
		}
	}
	logWrite(_T("[SCMenu] attach_submenu_by_caption: '%s' not found among placeholders"), caption);
	return false;
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
	m_submenu_ever_opened = false;	//새 popup session — 첫 hover 는 delay, 이후엔 즉시 전환.
	m_last_mouse_pt = CPoint(-1, -1);
	m_kb_priority = false;
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

	//Pre-paint — ShowWindow 전에 hidden 상태에서 OnPaint 강제 실행해 backbuffer 를 theme 색으로 채움.
	//그래야 ShowWindow 시 첫 화면부터 정상 theme 색으로 보이고, 빈 backbuffer 가 잠시 노출되는 flicker 회피.
	Invalidate();
	UpdateWindow();

	//SW_SHOWNA: WS_EX_NOACTIVATE 와 일관 — activation 안 일어나 부모의 NC paint 트리거 안 함.
	//SetFocus: cascade close 가 KillFocus 기반이므로 focus 는 명시적으로 메뉴에 부여.
	ShowWindow(SW_SHOWNA);
	SetFocus();

	//top-level popup 만 hook 설치 — submenu 들은 root 가 hook 으로 한꺼번에 관리.
	if (!m_parent_menu)
	{
		s_menu_root = this;
		if (!s_menu_mouse_hook)
			s_menu_mouse_hook = ::SetWindowsHookEx(WH_MOUSE,
				menu_mouse_hook_proc, NULL, ::GetCurrentThreadId());
	}
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
	int sy = 4;	//top margin (bottom 의 +4 와 대칭)
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

		//item rect 는 client 좌표 (CPaintDC 는 client 기준). client_right = rw.right - 2*border.cx 이므로
		//우측 4 px 마진 = rw.right - 2*border.cx - 4. 이전 식 (rw.right - 5 - border.cx) 은 WS_BORDER (~1px)
		//에선 거의 맞았지만 WS_THICKFRAME (~7~8px) 에선 item 이 client 영역을 넘어 잘려 우측 마진이 사라졌음.
		m_items[i]->m_r = CRect(4, sy, rw.right - 2 * border.cx - 4, sy + item_h);
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
				dc.SetTextColor(m_theme.cr_text_selected.ToCOLORREF());
				dc.FillSolidRect(view_r, m_theme.cr_back_selected.ToCOLORREF());
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
					dc.DrawText(m_items[i]->m_caption, rTextArea, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				}
				else
				{
					CRect rPrimary = rTextArea;
					rPrimary.bottom = rTextArea.top + rTextArea.Height() / 2;
					dc.DrawText(m_items[i]->m_caption, rPrimary, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

					CRect rSecondary = rTextArea;
					rSecondary.top = rPrimary.bottom;
					COLORREF prev_text_color = dc.GetTextColor();
					dc.SetTextColor(m_theme.cr_text_dim.ToCOLORREF());
					dc.DrawText(m_items[i]->m_sub_caption, rSecondary, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
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

			//메뉴 캡션 표시 — DT_NOPREFIX 로 자동 '&' 처리 차단 (caption 은 ctor 에서 이미 cleaned).
			dc.DrawText(m_items[i]->m_caption, rText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

			//access-key 위치 글자 아래 수동 underline. ctor 에서 m_access_key 에 위치 저장됨.
			if (m_items[i]->m_access_key >= 0)
			{
				int pos = m_items[i]->m_access_key;
				CString prefix = m_items[i]->m_caption.Left(pos);
				CString key_ch = m_items[i]->m_caption.Mid(pos, 1);
				CSize sz_prefix = dc.GetTextExtent(prefix);
				CSize sz_key = dc.GetTextExtent(key_ch);
				CSize sz_full = dc.GetTextExtent(m_items[i]->m_caption);

				int text_top = rText.top + (rText.Height() - sz_full.cy) / 2;
				int underline_y = text_top + sz_full.cy - 2;
				int x1 = rText.left + sz_prefix.cx;
				int x2 = x1 + sz_key.cx;

				COLORREF cr_line = (i == m_over_item)
					? m_theme.cr_text_hover.ToCOLORREF()
					: (m_items[i]->m_enabled ? m_theme.cr_text.ToCOLORREF() : m_theme.cr_text_dim.ToCOLORREF());
				dc.FillSolidRect(CRect(x1, underline_y, x2, underline_y + 1), cr_line);
			}

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
					rText.right = r_view.right - 10;
				//sub button이 있을 경우
				else
					rText.right = r_view.left - 12;

				dc.DrawText(m_items[i]->m_hot_key, rText, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			}

			if (m_items[i]->m_type == CSCMenuItem::item_submenu)
			{
				//chevron 두 직선 — 우측 꼭짓점에서 45° 로 만남. text '>' 글리프가 폰트 따라 깨져 보이는 증상 회피.
				Gdiplus::Pen pen(m_items[i]->m_enabled ? m_theme.cr_text : m_theme.cr_text_dim, 1.5f);
				pen.SetStartCap(Gdiplus::LineCapRound);
				pen.SetEndCap(Gdiplus::LineCapRound);

				const int half = 4;
				int tip_x = view_r.right - 14;
				int base_x = tip_x - half;
				int cy = view_r.CenterPoint().y;

				g.DrawLine(&pen, base_x, cy - half, tip_x, cy);
				g.DrawLine(&pen, base_x, cy + half, tip_x, cy);
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

void CSCMenu::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (bCalcValidRects)
	{
		lpncsp->rgrc[0].top -= 6;
	}

	CDialogEx::OnNcCalcSize(bCalcValidRects, lpncsp);
}
