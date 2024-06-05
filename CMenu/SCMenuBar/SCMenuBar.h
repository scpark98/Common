#pragma once

#include <afxwin.h>
#include <afxdialogex.h>
#include <deque>

#include "SCMenuButton.h"

#define MENU_BUTTON_ID	WM_USER + 100


// CSCMenuBar 대화 상자
class CSCMenuBar : public CDialogEx
{
	DECLARE_DYNAMIC(CSCMenuBar)

public:
	CSCMenuBar();   // 표준 생성자입니다.
	~CSCMenuBar();

	//CMenu를 이용하여 resource의 menu를 읽어오는 경우
	void	init(CWnd* parent, UINT resource_menu_id, int x = 0, int y = 0, int menu_item_width = 0, int menu_item_height = 0);
	void	set_check(int menu_index, UINT menu_id, int sub_button_index, bool check);
	void	set_color_theme(int theme);
	void	set_menu_text_color(COLORREF cr_text);
	void	set_menu_back_color(COLORREF cr_back);

	//메뉴가 표시되는 영역을 반투명으로 표시
	void	use_aero_effect(bool use);

	//메뉴 아이콘 및 서브 버튼들 설정
	void	set_icon_and_buttons(int menu_index, UINT menu_id, UINT icon_id = 0)
	{
		set_icon_and_buttons(menu_index, menu_id, icon_id, 0);
	}

	template <typename ... Types> void	set_icon_and_buttons(int menu_index, UINT menu_id, UINT icon_id, Types... button_ids)
	{
		if (menu_index < 0 || menu_id <= 0)
			return;

		CSCMenuItem* menu_item = m_menu_button[menu_index]->get_menu_item(menu_id);
		menu_item->set_icon(icon_id);

		int n = sizeof...(button_ids);
		if (n == 0)
			return;

		UINT arg[] = { button_ids... };
		for (auto button_id : arg)
		{
			if (button_id > 0)
				menu_item->add_button(button_id);
		}
	}

//resource의 menu를 이용하지 않고 직접 추가하는 경우
	/*
	template <typename ... Types> void init(CWnd* parent, int x, int y, int total_width, int menu_width, Types... args)
	{
		m_parent = parent;
		m_menu_pos = CPoint(x, y);

		create();

		int n = sizeof...(args);
		CString arg[] = { args... };

		if (total_width > 0 && menu_width < 0)
		{
			m_menu_width = total_width / n;
		}
		else if (total_width < 0 && menu_width > 0)
		{
			m_menu_width = menu_width;
		}

		for (auto caption : arg)
		{
			CSCMenuButton* button = new CSCMenuButton(caption);
			m_menu_button.push_back(button);
		}

		create_menu_buttons();
	}
	*/

	//sub_index = -1이면 separator
	void add(int index, int sub_index = -1, CString _caption = _T(""), UINT _icon_id = 0, CString _hotkey = _T(""))
	{
		m_menu_button[index]->add(sub_index, _caption, _icon_id, _hotkey);
	}

	template <typename ... Types> void add(int index, int sub_index, CString _caption, UINT _icon_id, CString _hotkey, Types... sub_buttons)
	{
		m_menu_button[index]->add(sub_index, _caption, _icon_id, _hotkey);

		UINT args[] = { sub_buttons... };
		for (auto sub_button : args)
		{
			m_menu_button[index]->add_sub_button(sub_index, sub_button);
		}
	}
////////////////////////////////////////////////////////////////////////

	//최상위 메뉴 버튼에 대한 메시지 처리
	LRESULT		on_message_GdiButton(WPARAM wParam, LPARAM lParam);

	//어떤 메뉴 항목에 발생한 클릭, 선택등의 메시지 처리
	LRESULT		on_message_SCMenu(WPARAM wParam, LPARAM lParam);

	CSCMenuItem* get_menu_item(int menu_id);

protected:
	bool		create();

	void		iterate_menu(HMENU hMenu, CSCMenuButton* button);
	int			get_menu_button_index_by_instance(CWnd* pWnd);
	int			get_menu_button_index_by_caption(CString caption);

	CWnd*		m_parent = NULL;
	std::deque<CSCMenuButton*>	m_menu_button;
	int			m_cur_menu = -1;

	int			m_menu_sx = 0;
	int			m_menu_sy = 0;
	int			m_total_width = -1;
	int			m_menu_width = 80;
	int			m_menu_height = 28;

	void		create_menu_buttons();
	void		set_menu_total_width(int total_width);
	void		set_menu_width(int width);
	void		set_menu_height(int height);
	void		set_menu_pos(int x, int y);

	void		on_button_clicked(UINT id);

	LOGFONT		m_lf;
	CFont		m_font;
	void		reconstruct_font();

	COLORREF	m_cr_text;
	COLORREF	m_cr_text_selected;
	COLORREF	m_cr_back;
	COLORREF	m_cr_back_selected;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
