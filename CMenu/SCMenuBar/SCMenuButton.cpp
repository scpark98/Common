#include "SCMenuButton.h"

#include "../../Functions.h"
#include "../../GdiplusBitmap.h"

BEGIN_MESSAGE_MAP(CSCMenuButton, CGdiButton)
	ON_REGISTERED_MESSAGE(Message_CSCMenu, &CSCMenuButton::on_message_SCMenu)
END_MESSAGE_MAP()

CSCMenuButton::CSCMenuButton(CString _caption)
{
	caption = _caption;
}

CSCMenuButton::~CSCMenuButton()
{
	m_menu.DestroyWindow();
}

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

void CSCMenuButton::add_sub_button_by_menu_id(int menu_id, UINT sub_button_image_id)
{
	m_menu.add_sub_button_by_menu_id(menu_id, sub_button_image_id);
}

void CSCMenuButton::add_sub_button_by_menu_index(int index, UINT sub_button_image_id)
{
	m_menu.add_sub_button_by_menu_index(index, sub_button_image_id);
}

void CSCMenuButton::popup_menu()
{
	CRect rw;

	if (m_use_aero_effect)
	{
		CRect rmenu = get_menu_rect();
		HBITMAP hBitmap = capture_screen_to_bitmap(&rmenu);
		CGdiplusBitmap img(hBitmap);
		img.apply_effect_blur(35.3, FALSE);
		m_menu.set_back_image(&img);
	}
	else
	{
		m_menu.set_back_image(NULL);
	}

	GetWindowRect(rw);
	m_menu.popup_menu(rw.left, rw.bottom + 1);
}

CRect CSCMenuButton::get_menu_rect()
{
	CRect rw, rmenu;
	GetWindowRect(rw);
	m_menu.GetWindowRect(rmenu);

	return CRect(rw.left, rw.bottom + 1, rw.left + rmenu.Width(), rw.bottom + 1 + rmenu.Height());
}

LRESULT CSCMenuButton::on_message_SCMenu(WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_parent->m_hWnd, Message_CSCMenu, wParam, lParam);
	return 0;
}

void CSCMenuButton::set_text_color(Gdiplus::Color cr_text)
{
	text_color(cr_text);
}

void CSCMenuButton::set_back_color(Gdiplus::Color cr_back)
{
	back_color(cr_back);
}

