#include "SCMenuButton.h"

BEGIN_MESSAGE_MAP(CSCMenuButton, CGdiButton)
	ON_REGISTERED_MESSAGE(Message_CSCMenu, &CSCMenuButton::on_message_SCMenu)
END_MESSAGE_MAP()

bool CSCMenuButton::create(CWnd* parent, CString _caption, CRect r, UINT menu_button_id)
{
	m_parent = parent;

	BOOL res = CGdiButton::create(_caption, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_FLAT,
					r, m_parent, menu_button_id);

	text_color(m_cr_text);
	back_color(m_cr_back);

	m_menu.create(this);

	return (res == TRUE);
}

void CSCMenuButton::add(int menu_id, CString _caption, UINT icon, CString hotkey)
{
	m_menu.add(menu_id, _caption, icon, hotkey);
}

void CSCMenuButton::add_sub_button(int menu_id, UINT sub_button_image_id)
{
	m_menu.add_sub_button(menu_id, sub_button_image_id);
}

void CSCMenuButton::popup_menu(int x, int y)
{
	if (x == -1 && y == -1)
	{
		CRect rw;
		GetWindowRect(rw);
		x = rw.left;
		y = rw.bottom + 1;
	}

	m_menu.popup_menu(x, y);
}

LRESULT CSCMenuButton::on_message_SCMenu(WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_parent->m_hWnd, Message_CSCMenu, wParam, lParam);
	return 0;
}