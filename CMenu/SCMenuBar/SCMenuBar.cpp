// SCMenuBar.cpp: 구현 파일
//

#include "SCMenuBar.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

// CSCMenuBar 대화 상자

IMPLEMENT_DYNAMIC(CSCMenuBar, CDialogEx)

CSCMenuBar::~CSCMenuBar()
{
	for (int i = 0; i < m_menu_button.size(); i++)
	{
		m_menu_button[i]->DestroyWindow();
		delete m_menu_button[i];
	}
}

void CSCMenuBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCMenuBar, CDialogEx)
	ON_COMMAND_RANGE(MENU_BUTTON_ID, MENU_BUTTON_ID + 100, &CSCMenuBar::on_menu_button_clicked)
	ON_REGISTERED_MESSAGE(Message_CSCMenu, &CSCMenuBar::on_message_SCMenu)
	ON_REGISTERED_MESSAGE(Message_CGdiButton, &CSCMenuBar::on_message_GdiButton)
END_MESSAGE_MAP()


// CSCMenuBar 메시지 처리기
void CSCMenuBar::iterate_menu(HMENU hMenu, CSCMenuButton* menu_button)
{
	UINT menu_id;
	CString menu_caption;
	//MENUITEMINFO mii;
	int i, nCount = GetMenuItemCount(hMenu);

	for (i = 0; i < nCount; i++)
	{
		/*
		memset(&mii, 0, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_FTYPE | MIIM_ID; // | MIIM_STATE
		mii.dwTypeData = NULL;

		if (!GetMenuItemInfo(hMenu, i, TRUE, &mii))
			continue;

		pcmi = (PCHARMENUITEM)LocalAlloc(LPTR,
			sizeof(*pcmi) + mii.cch * sizeof(*pcmi->szItemText));

		pcmi->cchItemText = mii.cch;
		mii.dwTypeData = pcmi->szItemText;
		mii.cch++;

		if (!GetMenuItemInfo(hMenu, i, TRUE, &mii))
		{
			continue;
		}
		*/
		get_menu_item_info(hMenu, i, &menu_id, &menu_caption, TRUE);

		TRACE(_T("ID = %u, string = %s\n"), menu_id, menu_caption);

		//seperator와 일반 메뉴항목만 허용(서브 팝업메뉴 항목은 배제)
		if (menu_id == 0 || (menu_id >= 0x8000 && menu_id <= 0xDFFF))
		{
			std::deque<CString> token;
			get_token_string(menu_caption, token, _T("\t"));
			if (token.size() >= 2)
				menu_button->add(menu_id, token[0], 0, token[1]);
			else
				menu_button->add(menu_id, token[0]);
		}

		//메뉴 항목이 팝업인 경우는 우선 배제한다.
		//if (mii.hSubMenu != NULL)
		//	iterate_menu(mii.hSubMenu, NULL); // ** recursive **
	}
}

bool CSCMenuBar::create()
{
	DWORD dwStyle = WS_CHILD | WS_VISIBLE;

	bool res = CreateEx(0, NULL, _T("CSCMenuBar"), dwStyle, CRect(m_menubar_sx, m_menubar_sy, m_menubar_sx + m_menu_button_width, m_menubar_sy + m_menu_button_height), m_parent, 0);

	//LONG_PTR wStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE); // Get the current style
	//wStyle &= ~WS_BORDER;   // Here, we mask out the style bit(s) we want to remove
	//SetWindowLongPtr(m_hWnd, GWL_STYLE, wStyle);

	if (res)
	{
		CFont* font = GetFont();

		if (font == NULL)
			font = AfxGetMainWnd()->GetFont();

		if (font != NULL)
			font->GetObject(sizeof(m_lf), &m_lf);
		else
			GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

		reconstruct_font();
	}

	return res;
}

void CSCMenuBar::init(CWnd* parent, UINT resource_menu_id, int x, int y, int menu_button_width, int menu_button_height)
{
	m_parent = parent;
	m_menubar_sx = x;
	m_menubar_sy = y;
	m_menu_button_width = MAX(menu_button_width, m_menu_button_width);
	m_menu_button_height = MAX(menu_button_height, m_menu_button_height);

	create();

	int sx, sy;
	BOOL bOk;
	CMenu menu;
	CMenu* pMenu;


	menu.LoadMenu(resource_menu_id);

	sx = 0;
	sy = 0;
	int count = menu.GetMenuItemCount();

	//각 메뉴 항목들에 대해서 버튼 생성 및 메뉴 항목들을 추가.
	//iterate_menu(menu.m_hMenu);를 호출하여 코드를 간결하게 작성할 수 있었으나
	//최상위 메뉴 항목과 서브메뉴가 존재하는 팝업메뉴 항목을 구분하는 방법이 현재로서는 모호함.
	//우선 최상위 항목 처리 루프에서 하위 항목만 iterate_menu()를 사용하도록 한다.
	for (int i = 0; i < count; i++)
	{
		UINT menu_id;
		CString menu_caption;
		get_menu_item_info(menu.m_hMenu, i, &menu_id, &menu_caption, TRUE);

		CSCMenuButton* menu_button = new CSCMenuButton(menu_caption);
		bOk = menu_button->create(this, menu_button->caption, CRect(sx, sy, sx + m_menu_button_width, sy + m_menu_button_height), MENU_BUTTON_ID + i);
		//menu_button->set_tooltip_text(menu_button->caption);
		sx += m_menu_button_width;

		pMenu = menu.GetSubMenu(i);

		//실제 서브 메뉴항목이 있을 경우에만 처리.
		if (pMenu)
		{
			iterate_menu(pMenu->m_hMenu, menu_button);
		}

		m_menu_button.push_back(menu_button);
	}

	//모든 메뉴 항목들을 추가했다면 menubar를 resize해준다.
	SetWindowPos(NULL, m_menubar_sx, m_menubar_sy, m_menu_button_width * m_menu_button.size(), m_menu_button_height, SWP_NOZORDER);
}


void CSCMenuBar::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	ASSERT(bCreated);
}

void CSCMenuBar::create_menu_buttons()
{
	int x = 0;
	int y = 0;
	BOOL bOk = FALSE;

	CRect rw;
	m_parent->GetWindowRect(rw);
	m_parent->ScreenToClient(rw);
	//MoveWindow(m_menu_pos.x, m_menu_pos.y, m_menu_width * m_menu_button.size(), m_menu_height);
	
	SetWindowPos(NULL, m_menubar_sx, m_menubar_sy, m_menu_button_width * m_menu_button.size(), m_menu_button_height, SWP_NOZORDER);

	for (int i = 0; i < m_menu_button.size(); i++)
	{
		bOk = m_menu_button[i]->create(this, m_menu_button[i]->caption, CRect(x, y, x + m_menu_button_width, y + m_menu_button_height), MENU_BUTTON_ID + i);
		x += m_menu_button_width;
	}
}

void CSCMenuBar::on_menu_button_clicked(UINT id)
{
	TRACE(_T("m_cur_menu = %d, id = %d, MENU_BUTTON_ID = %d, %d\n"), m_cur_menu, id, MENU_BUTTON_ID, id - MENU_BUTTON_ID);

	//현재 보여지는 메뉴 항목이면 hide
	//메뉴 버튼이 클릭되는 순간 CSCMenu 입장에서는 killfocus가 되므로 메뉴는 사라지고 m_cur_menu도 -1로 리셋되므로
	//현재는 아래 코드가 수행되지 않는 구조다.
	if (m_cur_menu == id - (UINT)MENU_BUTTON_ID)
	{
		m_menu_button[m_cur_menu]->popup_hide();
		m_cur_menu = -1;
		return;
	}

	m_cur_menu = id - (UINT)MENU_BUTTON_ID;

	CRect rw;

	m_menu_button[m_cur_menu]->GetWindowRect(rw);
	m_menu_button[m_cur_menu]->popup_menu();
}


LRESULT	CSCMenuBar::on_message_SCMenu(WPARAM wParam, LPARAM lParam)
{
	CSCMenuMessage* msg = (CSCMenuMessage*)wParam;

	::SendMessage(m_parent->m_hWnd, Message_CSCMenu, wParam, m_cur_menu);

	if (msg->m_message == CSCMenu::message_scmenu_hide ||
		msg->m_message == CSCMenu::message_scmenu_selchanged)
	{
		m_cur_menu = -1;
	}

	return 0;
}

//최상위 메뉴 버튼에 대한 메시지 처리
LRESULT	CSCMenuBar::on_message_GdiButton(WPARAM wParam, LPARAM lParam)
{
	CGdiButtonMessage* msg = (CGdiButtonMessage*)wParam;
	TRACE(_T("btn = %p, msg = %d\n"), msg->m_pWnd, msg->m_message);

	if (m_cur_menu >= 0 && msg->m_message == WM_MOUSEHOVER)
	{
		TRACE(_T("hover\n"));

		int new_menu = get_menu_button_index_by_instance(msg->m_pWnd);
		if (new_menu == m_cur_menu)
			return 0;

		m_menu_button[m_cur_menu]->popup_hide();

		m_cur_menu = get_menu_button_index_by_instance(msg->m_pWnd);
		((CSCMenuButton*)(msg->m_pWnd))->popup_menu();
	}
	else if (msg->m_message == WM_LBUTTONUP)
	{
		TRACE(_T("m_cur_menu = %d, id = %d, MENU_BUTTON_ID = %d, id - MENU_BUTTON_ID = %d\n"), m_cur_menu, msg->m_ctrl_id, MENU_BUTTON_ID, msg->m_ctrl_id - MENU_BUTTON_ID);
		//메뉴버튼이 눌려지면 on_menu_button_clicked()에서 처리하므로
		//CGdiButton이 보내오는 WM_LBUTTONUP에서는 별도 처리하지 않는다.
		/*
		//현재 보여지는 메뉴 항목이면 hide
		//메뉴 버튼이 클릭되는 순간 CSCMenu 입장에서는 killfocus가 되므로 메뉴는 사라지고 m_cur_menu도 -1로 리셋되므로
		//현재는 아래 코드가 수행되지 않는 구조다.
		if (m_cur_menu == (msg->m_ctrl_id - (UINT)MENU_BUTTON_ID))
		{
			m_menu_button[m_cur_menu]->popup_hide();
			m_cur_menu = -1;
			return 0;
		}

		m_cur_menu = msg->m_ctrl_id - (UINT)MENU_BUTTON_ID;

		//m_menu_button[msg->m_ctrl_id - MENU_BUTTON_ID]->ShowWindow(SW_SHOW);

		CRect rw;

		m_menu_button[msg->m_ctrl_id - MENU_BUTTON_ID]->GetWindowRect(rw);
		m_menu_button[msg->m_ctrl_id - MENU_BUTTON_ID]->popup_menu();
		m_cur_menu = msg->m_ctrl_id - MENU_BUTTON_ID;
		*/
	}

	return 0;
}

int	CSCMenuBar::get_menu_button_index_by_instance(CWnd* pWnd)
{
	for (int i = 0; i < m_menu_button.size(); i++)
	{
		if (pWnd == m_menu_button[i])
			return i;
	}

	return -1;
}

int	CSCMenuBar::get_menu_button_index_by_caption(CString caption)
{
	for (int i = 0; i < m_menu_button.size(); i++)
	{
		if (caption == m_menu_button[i]->caption)
			return i;
	}

	return -1;
}

void CSCMenuBar::set_check(int menu_index, UINT menu_id, int sub_button_index, bool check)
{
	CSCMenuItem* menu_item = m_menu_button[menu_index]->get_menu_item(menu_id);
	
	ASSERT(menu_item != NULL);

	if (menu_item == 0)
	{
		TRACE(_T("error : no exist menu_id(%d) in menu_index = %d\n"), menu_id, menu_index);
	}

	menu_item->set_check(sub_button_index, check);
}

void CSCMenuBar::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);
	
	if (!m_hWnd)
		return;
	
	Invalidate();

	for (int i = 0; i < m_menu_button.size(); i++)
	{
		//메뉴 버튼들의 색상 설정
		m_menu_button[i]->set_text_color(m_theme.cr_text);
		m_menu_button[i]->set_back_color(m_theme.cr_back);

		//각 메뉴버튼에 연결된 팝업메뉴 항목들도 색상 설정
		m_menu_button[i]->get_menu()->set_color_theme(theme);
	}
}

void CSCMenuBar::set_text_color(Gdiplus::Color cr_text)
{
	m_theme.cr_text = cr_text;

	for (int i = 0; i < m_menu_button.size(); i++)
		m_menu_button[i]->set_text_color(cr_text);
}

void CSCMenuBar::set_back_color(Gdiplus::Color cr_back)
{
	m_theme.cr_back = cr_back;

	for (int i = 0; i < m_menu_button.size(); i++)
		m_menu_button[i]->set_back_color(cr_back);
}

//메뉴가 표시되는 영역을 반투명으로 표시
void CSCMenuBar::use_aero_effect(bool use)
{
	for (int i = 0; i < m_menu_button.size(); i++)
		m_menu_button[i]->use_aero_effect(use);
}


BOOL CSCMenuBar::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		return false;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSCMenuBar::set_menu_line_height(int height)
{
	for (int i = 0; i < m_menu_button.size(); i++)
		m_menu_button[i]->get_menu()->set_line_height(height);
}
