#pragma once

/*
scpark 2020_03_30_13
full customize를 위해 직접 제작.

[수정될 내용]
-rename
-move, copy?
-horizontal scroll

[수정된 내용]

[트리 항목 구성]
-[확장축소버튼][체크박스][아이콘][텍스트] 4개 부분으로 구성된다.
-확장축소는 표준 컨트롤에서는 +, - 버튼으로 표시했지만 요즘은 >기호를 사용하고
 체크박스, 아이콘 3종 모두 16x16크기로 표시된다.
-라인 표시 모드라면 확장축소버튼에서 세로로 그리기 시작한다.
*/

#include "../data_structure/tree_node.h"

// CTreeCtrlEx
#define MESSAGE_TREECTRLEX_CHECK_CHANGED	WM_USER + 0x7FFF - 0x7762

class CTreeItem
{
public:
	CTreeItem()
		: text(_T("")), selected(false), checked(0), image_index(-1), expanded(1)
	{
	}

	CTreeItem(CString _text, bool _selected = false, int _checked = 0, int _image_index = -1, bool _expanded = true)
		: text(_text), selected(_selected), checked(_checked), image_index(_image_index), expanded(_expanded)
	{
	}

	CString text;
	bool	selected;
	int		checked;		//0:unchecked, 1:checked, 2:indeterminated
	int		image_index;
	bool	expanded;		//1:expand, 0:collapsed

	bool	get_selected() { return selected; }
	void	set_select(bool select) { selected = select; }
	void	toggle_select() { selected = !selected; }

	int		get_check() { return checked; }
	void	set_check(int check) { checked = check; }
	void	toggle_checked() { checked = !checked; }
};

class CTreeCtrlEx : public CWnd
{
	DECLARE_DYNAMIC(CTreeCtrlEx)

public:
	CTreeCtrlEx();
	virtual ~CTreeCtrlEx();

	void			use_keyboard(bool use) { m_use_keyboard = use; }

	tree<CTreeItem>	m_tr;

	//목록의 갯수가 많지 않을 경우 탭으로 표시된 문자열에서도 로딩이 가능하다.
	void			load_from_string(CString data);
	//탭으로 들여쓰기된 텍스트 파일에서 읽어오거나 텍스트 파일로 저장한다.
	bool			load_from_file(bool load, CString sfile);
	//체크박스 트리인지. default = false
	void			use_check_box(bool use = true);
	//루트 항목에도 체크박스를 표시할 것인지. default = false
	void			use_check_box_at_root(bool use = true);
	void			set_check_box_size(int size = 16);

	void			always_show_selection(bool show = true);

	//findText가 ""이면 모든 노드를 순회해서 check가 0또는 1이라면 체크 변경.
	//findText가 ""이 아니면 해당 아이템만 찾아서 check.
	//단, check가 0또는 1이 아니라면 단순히 findText의 노드를 찾고자 하는 목적으로 이용된다.
	tree<CTreeItem>::pre_order_iterator find_item(const CString& findText, int check = -1, int select = -1);
	tree<CTreeItem>::pre_order_iterator find_item(const CString& parent, const CString& findText, int check = -1, int select = -1);
	tree<CTreeItem>::pre_order_iterator get_selected_item(tree<CTreeItem>::pre_order_iterator it = NULL);
	void			check_item(tree<CTreeItem>::pre_order_iterator it, int check);
	//체크된 항목을 리턴한다.
	std::deque<CTreeItem> get_checked_item(tree<CTreeItem>::pre_order_iterator it,
		bool include_current_node = true, bool only_current_group = true, bool include_sub_nodes = true);
	std::deque<CString> CTreeCtrlEx::get_checked_item_text(tree<CTreeItem>::pre_order_iterator it,
		bool include_current_node = true, bool only_current_group = true, bool include_sub_nodes = true);

	//체크된 항목 개수를 리턴한다.
	int				get_checked_count(tree<CTreeItem>::pre_order_iterator it = NULL, bool include_current_node = true, bool only_current_group = true, bool include_sub_nodes = true);
	void			select_item(tree<CTreeItem>::pre_order_iterator it, bool single_select = true);
	CRect			get_item_rect(tree<CTreeItem>::pre_order_iterator it, bool text_only);
	CString			get_text(tree<CTreeItem>::pre_order_iterator it);
	void			ensure_visible(tree<CTreeItem>::pre_order_iterator it);

	void			use_expand_button(bool use = true);
	//expand (1:expand, 0:collapse, -1:toggle)
	void			expand(tree<CTreeItem>::pre_order_iterator it, int expand = 1);

	//폰트 관련
	int				get_font_size();
	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int font_size);
	void			enlarge_font_size(bool enlarge);
	void			set_font_bold(bool bold = true);
	void			set_font_italic(bool italic = true);
	LOGFONT			get_log_font() { return m_lf; }
	void			set_log_font(LOGFONT lf);

	int				get_line_height() { return m_line_height; }
	void			set_line_height(int line_height);
	int				get_line_gap() { return m_line_gap; }
	void			set_line_gap(int gap) { m_line_gap = gap; }

	enum COLOR_THEME
	{
		color_theme_default = 0,
		//color_theme_blue,
		//color_theme_dark_blue,
		color_theme_dark_gray,
	};
	void			set_color_theme(int theme, bool invalidate = true);

	void			set_back_color(COLORREF crBack);
	void			set_text_color(COLORREF crText);
	void			set_check_color(COLORREF crCheck);

	int				get_indent_size() { return m_indent_size; }
	void			set_indent_size(int indent) { m_indent_size = indent; Invalidate(); }
	void			draw_line_node(bool draw = true);

	void			use_scrollbar(bool use = true);
	void			set_scroll_unit(int unit) { m_scroll_unit = unit; }

	void			use_image_list(bool use) { m_use_image_list = use; }
	bool			use_image_list() { return m_use_image_list; }
	int				get_image_list_size() { return m_image_list_size; }
	void			set_image_list_size(int image_size);
	void			set_image_random();	//for test
	CImageList*		get_image_list() { return &m_image_list; }

protected:
	BOOL			RegisterWindowClass();

	//텍스트 파일을 읽어왔을때 유니코드 여부에 따라 저장도 동일하게 하자. default = true
	//bool			m_unicode_file;

	//방향키 등 키보드 입력을 처리할 지 아니면 parent에게 그냥 넘길지. default = true;
	bool			m_use_keyboard;
	//확장, 축소 버튼을 표시할 것인지. default = true;
	bool			m_use_expand_button;
	int				m_expand_button_size;

	//체크박스 트리인지
	bool			m_use_check_box;
	//루트에도 체크박스를 표시할 것인지
	bool			m_check_box_at_root;
	//checkbox width
	int				m_check_box_size;
	//체크박스와 텍스트 사이의 여백 크기
	int				m_check_box_text_gap;
	//이미지리스트를 사용할 것인지. default = false
	bool			m_use_image_list;
	//이미지리스트의 기본 이미지 크기. 저장된 이미지의 개수가 아님. default = 16
	int				m_image_list_size;
	//선택 항목을 항상 보이도록 할 것인지. default = true;
	bool			m_always_show_selection;
	//multi selection. default = false;
	bool			m_use_multiple_selection;

	//트리 여백(왼쪽 상단 뿐 아니라 오른쪽, 하단도 적용되어야 한다)
	CSize			m_margin;

	//indent size in pixel
	int				m_indent_size;

	//두 노드의 최소한의 간격
	int				m_line_gap;
	//한 노드의 높이 = m_line_gap, m_lf.lfHeight, icon_size에 의해 최종 결정되고 이를 이용해 스크롤 크기도 재조정된다.
	int				m_line_height;
	//만약 자동으로 라인의 높이를 조절하지 않고 고정하고 싶다면
	bool			m_auto_line_height;	//default = true;
	void			recalculate_line_height();
	//스크롤 크기를 재계산한다.
	void			recalculate_scroll_size();

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	//상단부터 순차적인 레벨 순서의 아이템을 찾는다.
	tree<CTreeItem>::pre_order_iterator get_item_by_index(int index);
	//확대, 축소를 고려하여 다음 항목을 리턴한다. 현재 노드가 축소된 상태라면 다음 노드는 현재 노드의 차일드가 아닌 sibling이 된다.
	tree<CTreeItem>::pre_order_iterator get_next_visible_item(tree<CTreeItem>::pre_order_iterator it);
	//확대, 축소를 고려하여 이전 항목을 리턴한다. 현재 노드의 바로 위부터 찾으면 그 노드는 collapsed일 수 있다.
	//현재 노드의 prev_sibling을 찾아서 내려가면서 get_next_visible_item을 찾아야한다.
	tree<CTreeItem>::pre_order_iterator get_previous_visible_item(tree<CTreeItem>::pre_order_iterator it);
	//축소된 노드를 제외하고 화면에 표시해야 하는 총 노드의 개수. 스크롤 크기 재계산시에 사용된다.
	int				get_visible_item_count();

	COLORREF		m_crBack;
	COLORREF		m_crText;
	COLORREF		m_crCheck;
	COLORREF		m_crTextSelected;
	COLORREF		m_crBackSelected;

	//노드 연결 라인
	bool			m_use_node_line;	//default = true
	COLORREF		m_node_line_color;
	int				m_node_line_style;
	int				m_node_line_width;

	//스크롤바를 사용하지 않고 스크롤하기. 휠 또는 키보드로만.
	bool			m_use_scrollbar;	//default = true;
	int				m_scroll_total;
	int				m_scroll_pos;
	int				m_scroll_unit;		//PgUp, PgDn, 휠 스크롤 시 몇 라인씩 스크롤할지). default = 1

	void			on_mouse_event(int mouse_event, CPoint point);

	CImageList		m_image_list;



protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PreSubclassWindow();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
};
