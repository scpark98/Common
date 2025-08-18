#pragma once

/*
* menu�� sub popup menu���� �����Ϸ��� tree ������ �ڷᱸ���� �ʿ��ϴ�.
* �켱 1������ �޴� ���¸� �������.
*/

#include <afxwin.h>
#include <afxdialogex.h>
#include <deque>
#include "../../CButton/GdiButton/GdiButton.h"
#include "../../colors.h"

static const UINT Message_CSCMenu = ::RegisterWindowMessage(_T("MessageString_CSCMenu"));

class CSCMenuItem;

class CSCMenuMessage
{
public:
	CSCMenuMessage(CWnd* _this, int _message, CSCMenuItem* _menu_item, int _button_index = -1, int _button_state = 0);

	CWnd*		m_pWnd = NULL;
	int			m_message;
	CSCMenuItem	*m_menu_item;
	int			m_button_index;
	int			m_button_state;
};

//�� �޴� �׸񳻿� ���� �ɼ� ǥ�ÿ� SubButton
//�����ư�� �ϳ���� check�� �����ϰ� 2�� �̻��̶�� radio�� �����ϹǷ�
//subButton����� ���� CGdiButton���� �������� ����.
class CSCMenuSubButton
{
public:
	CSCMenuSubButton(UINT _id, int menu_height);
	~CSCMenuSubButton();

	CSCGdiplusBitmap* m_button_image[2] = { NULL, };	//0:unselect, 1:select, (2:over, 3:down �߰� ����??)
	CRect	m_r = CRect(0, 0, 0, 0);
	//radio�� �Ѵٸ� group �Ӽ������� ����ؾ������� �׷��Ա��� �޴��� �����ϰ� �������� ����.
	//�� �ʿ��ϴٸ� �޴� �׸��� ������.
	//BS_CHECKBOX �߰� �ʿ伺�� ������ �ʿ�� �ִ�.
	int		m_style = BS_RADIOBUTTON;
	int		m_state = 0;		//0:unchecked, 1:checked
};

class CSCMenuItem
{
public:
	CSCMenuItem(int id, CString caption, UINT icon_id = 0, CString hot_key = _T(""), int menu_height = -1);
	~CSCMenuItem();

	//�ϳ��� �޴� �������� �߰��ϸ鼭 resource�� png�� ��ϵ� sub button���� �߰��� �� �ִ�.
	//m_menu.add(control_start, _T("�������� �����ϱ�"), IDB_CONTROL_START, _T(""), IDB_MENU_CHECK);
	//m_menu.add(control_mode, _T("������"), IDB_CONTROL_MODE, _T(""), IDB_MENU_DRIVE_MODE, IDB_MENU_GDI_MODE);
	template <typename ... Types> CSCMenuItem(int id, CString caption, UINT icon_id = 0, CString hot_key = _T(""), int menu_height = -1, Types... args)
	{
		m_id = _id;
		m_caption = _caption;
		m_hot_key = _hot_key;

		set_icon(icon_id);

		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
		{
			CSCMenuSubButton* btn = new CSCMenuSubButton(id);
			m_buttons.push_back(btn);
		}
	}

	int				m_id;	//0 ���ϴ� separator. Resouce�� Menu�� separator�� 0���� ����Ѵ�.
	bool			m_is_separator = false;
	CRect			m_r = CRect(0, 0, 0, 0);
	CString			m_caption;
	CString			m_hot_key;
	int				m_access_key = 0;	//�޴� ĸ�ǿ� (&N)�� ���� ���ڿ��� �ִٸ� �̸� �����ؼ� nŰ�� ����Ű�� ����Ѵ�.
	int				m_menu_height;

	//icon �̹����� ���� png�� �����ϰ� m_line_height�� ���̿� ������ ���簢�� ũ��� �����ؾ� �Ѵ�.
	//�� ũ�Ⱑ 32x32��� ���� �̹����� �� �ȿ� �۰� ǥ�õ� ������ �׷��� �̹����̾�� �Ѵ�.
	//32x32 ũ�⿡ ������ �׷��� �̹����� ����ؼ��� �ȵȴ�.
	CSCGdiplusBitmap	m_icon;
	std::deque<CSCMenuSubButton*> m_buttons;

	void			set_icon(UINT icon_id);
	void			add_button(UINT button_id, bool reset = false);

	//��ư�� 1���� ���� check <-> uncheck toggle�̰�
	//2�� �̻��� ���� radio�� �����Ѵ�.
	void			set_check(int idx, bool _check = true);

	int				get_check(int idx);
	int				get_checked_button_index();
};


// CSCMenu

class CSCMenu : public CDialogEx
{
	DECLARE_DYNAMIC(CSCMenu)

public:
	CSCMenu();
	~CSCMenu();

	enum MESSAGES
	{
		message_scmenu_selchanged = 0,
		message_scmenu_button_state_changed,
		message_scmenu_hide,
	};

	bool			create(CWnd* parent, int width = 220);

	//load from menu resource
	void			load(UINT resource_id, int menu_index);

	//add menu item manually. _id < 0 = separator
	void			add(int _id, CString _caption = _T(""), UINT icon_id = 0, CString _hot_key = _T(""));

	//sub button�� �������� �� args�� �����Ͽ� ȣ��.
	template <typename ... Types> void add(int _id, CString _caption = _T(""), UINT icon_id = 0, CString _hot_key = _T(""), Types... args)
	{
		CSCMenuItem* item = new CSCMenuItem(_id, _caption, icon_id, _hot_key);

		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
		{
			CSCMenuSubButton* button = new CSCMenuSubButton(id);
			item->m_buttons.push_back(button);
		}

		m_items.push_back(item);
		recalc_items_rect();
	}

	template <typename ... Types> void	set_icon_and_buttons(UINT sub_menu_id, UINT icon_id, Types... button_ids)
	{
		CSCMenuItem* menu_item = get_menu_item(sub_menu_id);
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

	void			add_sub_button_by_menu_index(int index, UINT id);
	void			add_sub_button_by_menu_id(int menu_id, UINT id);

	void			set_check(UINT menu_id, int sub_button_index, bool check);


	//����� ������ ����� �ʵ��� ������ �� ǥ���ؾ� �Ѵ�.
	void			popup_menu(int x, int y);
	CSCMenuItem*	get_menu_item(int menu_id);
	int				get_menu_count() { return m_items.size(); }

	//���� ����
	int				get_line_height() { return m_line_height; }
	void			set_line_height(int _line_height);

	void			use_over(bool use = true) { m_use_over = use; }
	int				get_over_item() { return (m_use_over ? m_over_item : -1); }

	void			set_back_image(CSCGdiplusBitmap* img);

	virtual			CSCMenu& set_font(LOGFONT& lf);
	virtual			CSCMenu& set_font_name(CString sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual			CSCMenu& set_font_size(int nSize);
	virtual			CSCMenu& set_font_bold(bool bBold = true);

	void			set_color_theme(int theme) { m_theme.set_color_theme(theme); if (!m_hWnd) return; Invalidate(); }

protected:
	CSCColorTheme	m_theme = CSCColorTheme(this);

	CWnd*			m_parent = NULL;
	CSCGdiplusBitmap	m_img_back;
	std::deque<CSCMenuItem*> m_items;

	bool		m_use_over = true;			//hover hilighted
	int			m_over_item = -1;
	int			get_item_index(CPoint pt);

	LOGFONT		m_lf;
	CFont		m_font;
	void		ReconstructFont();

	//���� ���̴� �۲� ���̿� ���� �ڵ� ���ȴ�.
	//���� �������� ���� ������ �����Ϸ��� set_line_height(32); �� ���� �Լ��� �̿��ؾ� �Ѵ�.
	//font_size�� �����ߴµ� ���� ������ ������� �ʴ� �͵� ������ �Ǵ� �ʿ��� ���
	//�ش� �Լ��� ���� ��������.
	int			m_line_height;
	//�׸��� �߰�/�����ǰų� m_line_height�� ����Ǹ� �ݵ�� rect������ ��������� �Ѵ�.
	//���� ������ font size�� ���� �ڵ� �����ǰ� �� ���� ������ ������� �ִ�.
	//�ϴ��� ������ ó���ǵ��� �Ѵ�.
	void		recalc_items_rect();

protected:
	DECLARE_MESSAGE_MAP()
public:
	//virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//afx_msg BOOL OnLbnSelchange();
	afx_msg void OnPaint();
};


