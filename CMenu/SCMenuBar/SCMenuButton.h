#pragma once

#include <afxwin.h>
#include "../../colors.h"
#include "../../CButton/GdiButton/GdiButton.h"
#include "SCMenu.h"

//�޴� �׸� ��ư�� �ش� ��ư�� ������ �� �������� �˾��޴��� ������
class CSCMenuButton : public CGdiButton
{
public:
	CSCMenuButton(CString _caption);
	~CSCMenuButton();

	bool		create(CWnd* parent, CString _caption, CRect r, UINT menu_button_id);
	void		add(int menu_id, CString _caption, UINT icon = 0, CString hotkey = _T(""));
	void		add_sub_button(int menu_id, UINT sub_button_image_id);
	void		popup_menu();
	CRect		get_menu_rect();	//�޴��� ǥ�õ� ������ �̸� ����ؼ� �� �� �ִ�.

	void		popup_hide() { m_menu.ShowWindow(SW_HIDE); }

	//�޴��� ǥ�õǴ� ������ ���������� ǥ��
	void		use_aero_effect(bool use) { m_use_aero_effect = use; }


	void		set_text_color(COLORREF cr_text);
	void		set_back_color(COLORREF cr_back);

	CString		caption;
	LRESULT		on_message_SCMenu(WPARAM wParam, LPARAM lParam);

	CSCMenu*	get_menu() { return &m_menu; }
	CSCMenuItem* get_menu_item(UINT menu_id) { return m_menu.get_menu_item(menu_id); }

protected:
	CWnd*		m_parent;
	CSCMenu		m_menu;
	COLORREF	m_cr_text = GRAY(212);
	COLORREF	m_cr_back = RGB(64, 73, 88);
	bool		m_use_aero_effect = false;

	DECLARE_MESSAGE_MAP()
};

