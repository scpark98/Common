#pragma once

/*
* menu를 sub popup menu까지 구현하려면 tree 형태의 자료구조가 필요하다.
* 우선 1레벨의 메뉴 형태만 고려하자.
*/

#include <afxwin.h>
#include <afxdialogex.h>
#include <deque>
#include "../../CButton/GdiButton/GdiButton.h"

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

//한 메뉴 항목내에 선택 옵션 표시용 SubButton
//서브버튼이 하나라면 check로 동작하고 2개 이상이라면 radio로 동작하므로
//subButton들까지 굳이 CGdiButton으로 구현하진 않음.
class CSCMenuSubButton
{
public:
	CSCMenuSubButton(UINT _id, int menu_height);
	~CSCMenuSubButton();

	CGdiplusBitmap* m_button_image[2] = { NULL, };	//0:unselect, 1:select, (2:over, 3:down 추가 예정??)
	CRect	m_r = CRect(0, 0, 0, 0);
	//radio로 한다면 group 속성까지도 고려해야하지만 그렇게까지 메뉴를 복잡하게 구성하진 말자.
	//정 필요하다면 메뉴 항목을 나누자.
	//BS_CHECKBOX 추가 필요성도 검토할 필요는 있다.
	int		m_style = BS_RADIOBUTTON;
	int		m_state = 0;		//0:unchecked, 1:checked
};

class CSCMenuItem
{
public:
	CSCMenuItem(int id, CString caption, UINT icon_id = 0, CString hot_key = _T(""), int menu_height = -1);
	~CSCMenuItem();

	//하나의 메뉴 아이템을 추가하면서 resource의 png로 등록된 sub button들을 추가할 수 있다.
	//m_menu.add(control_start, _T("원격제어 시작하기"), IDB_CONTROL_START, _T(""), IDB_MENU_CHECK);
	//m_menu.add(control_mode, _T("제어모드"), IDB_CONTROL_MODE, _T(""), IDB_MENU_DRIVE_MODE, IDB_MENU_GDI_MODE);
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

	int				m_id;	//-1이면 separator
	bool			m_is_separator = false;
	CRect			m_r = CRect(0, 0, 0, 0);
	CString			m_caption;
	CString			m_hot_key;
	int				m_menu_height;

	//icon 이미지는 투명 png로 제작하고 m_line_height의 높이와 동일한 정사각형 크기로 제작해야 한다.
	//그 크기가 32x32라면 실제 이미지는 그 안에 작게 표시될 정도로 그려진 이미지이어야 한다.
	//32x32 크기에 꽉차게 그려진 이미지를 사용해서는 안된다.
	CGdiplusBitmap	m_icon;
	std::deque<CSCMenuSubButton*> m_buttons;

	void			set_icon(UINT icon_id);
	void			add_button(UINT button_id, bool reset = false);

	//버튼이 1개인 경우는 check <-> uncheck toggle이고
	//2개 이상일 경우는 radio로 동작한다.
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

	bool create(CWnd* parent);
	void add(int _id, CString _caption = _T(""), UINT icon_id = 0, CString _hot_key = _T(""));

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

	void			add_sub_button(int index, UINT id);

	//모니터 영역을 벗어나지 않도록 보정한 후 표시해야 한다.
	void			popup_menu(int x, int y);
	CSCMenuItem*	get_menu_item(int menu_id);
	int				get_menu_count() { return m_items.size(); }

	//라인 간격
	int				get_line_height() { return m_line_height; }
	void			set_line_height(int _line_height);

	void			use_over(bool use = true) { m_use_over = use; }
	int				get_over_item() { return (m_use_over ? m_over_item : -1); }

	void			set_back_image(CGdiplusBitmap* img);

	virtual			CSCMenu& set_font(LOGFONT& lf);
	virtual			CSCMenu& set_font_name(CString sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual			CSCMenu& set_font_size(int nSize);
	virtual			CSCMenu& set_font_bold(bool bBold = true);

	//color setting
	enum CSCMENU_COLOR_THEME
	{
		color_theme_default = 0,
		color_theme_dark_gray,
		color_theme_linkmemine,
	};

	void			set_color_theme(int theme, bool apply_now = true);

protected:
	CWnd*			m_parent = NULL;
	CGdiplusBitmap	m_img_back;
	std::deque<CSCMenuItem*> m_items;

	bool		m_use_over = true;			//hover hilighted
	int			m_over_item = -1;
	int			get_item_index(CPoint pt);

	COLORREF	m_cr_text = ::GetSysColor(COLOR_WINDOWTEXT);	//기본 글자색
	COLORREF	m_cr_text_selected = m_cr_text;					//선택 항목의 활성화(active) 글자색
	COLORREF	m_cr_text_selected_inactive = m_cr_text;		//선택 항목의 비활성화(inactive) 글자색
	COLORREF	m_cr_text_over = m_cr_text;
	COLORREF	m_cr_back = ::GetSysColor(COLOR_3DFACE);		//기본 배경색
	COLORREF	m_cr_back_selected = ::GetSysColor(COLOR_3DHIGHLIGHT);	//선택 항목 배경색
	COLORREF	m_cr_back_selected_border;						//선택 항목 테두리(focus()가 있을 경우에만)
	COLORREF	m_cr_back_selected_inactive;
	COLORREF	m_cr_back_over = ::GetSysColor(COLOR_3DHIGHLIGHT);

	LOGFONT		m_lf;
	CFont		m_font;
	void		ReconstructFont();

	//라인 높이는 글꼴 높이에 따라 자동 계산된다.
	//만약 수동으로 라인 간격을 변경하려면 set_line_height(32); 함수를 이용해야 한다.
	//font_size를 증감했는데 라인 간격이 변경되지 않는 것도 문제가 되니 필요할 경우
	//해당 함수를 통해 설정하자.
	int			m_line_height;
	//항목이 추가/삭제되거나 m_line_height가 변경되면 반드시 rect정보를 갱신해줘야 한다.
	//라인 간격은 font size에 따라 자동 조절되게 할 수도 있지만 장단점이 있다.
	//일단은 별개로 처리되도록 한다.
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


