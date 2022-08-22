#pragma once

/*
scpark 2020_03_30_13
full customize�� ���� ���� ����.

[������ ����]
-rename
-move, copy?
-horizontal scroll

[������ ����]

[Ʈ�� �׸� ����]
-[Ȯ����ҹ�ư][üũ�ڽ�][������][�ؽ�Ʈ] 4�� �κ����� �����ȴ�.
-Ȯ����Ҵ� ǥ�� ��Ʈ�ѿ����� +, - ��ư���� ǥ�������� ������ >��ȣ�� ����ϰ�
 üũ�ڽ�, ������ 3�� ��� 16x16ũ��� ǥ�õȴ�.
-���� ǥ�� ����� Ȯ����ҹ�ư���� ���η� �׸��� �����Ѵ�.
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

	//����� ������ ���� ���� ��� ������ ǥ�õ� ���ڿ������� �ε��� �����ϴ�.
	void			load_from_string(CString data);
	//������ �鿩����� �ؽ�Ʈ ���Ͽ��� �о���ų� �ؽ�Ʈ ���Ϸ� �����Ѵ�.
	bool			load_from_file(bool load, CString sfile);
	//üũ�ڽ� Ʈ������. default = false
	void			use_check_box(bool use = true);
	//��Ʈ �׸񿡵� üũ�ڽ��� ǥ���� ������. default = false
	void			use_check_box_at_root(bool use = true);
	void			set_check_box_size(int size = 16);

	void			always_show_selection(bool show = true);

	//findText�� ""�̸� ��� ��带 ��ȸ�ؼ� check�� 0�Ǵ� 1�̶�� üũ ����.
	//findText�� ""�� �ƴϸ� �ش� �����۸� ã�Ƽ� check.
	//��, check�� 0�Ǵ� 1�� �ƴ϶�� �ܼ��� findText�� ��带 ã���� �ϴ� �������� �̿�ȴ�.
	tree<CTreeItem>::pre_order_iterator find_item(const CString& findText, int check = -1, int select = -1);
	tree<CTreeItem>::pre_order_iterator find_item(const CString& parent, const CString& findText, int check = -1, int select = -1);
	tree<CTreeItem>::pre_order_iterator get_selected_item(tree<CTreeItem>::pre_order_iterator it = NULL);
	void			check_item(tree<CTreeItem>::pre_order_iterator it, int check);
	//üũ�� �׸��� �����Ѵ�.
	std::deque<CTreeItem> get_checked_item(tree<CTreeItem>::pre_order_iterator it,
		bool include_current_node = true, bool only_current_group = true, bool include_sub_nodes = true);
	std::deque<CString> CTreeCtrlEx::get_checked_item_text(tree<CTreeItem>::pre_order_iterator it,
		bool include_current_node = true, bool only_current_group = true, bool include_sub_nodes = true);

	//üũ�� �׸� ������ �����Ѵ�.
	int				get_checked_count(tree<CTreeItem>::pre_order_iterator it = NULL, bool include_current_node = true, bool only_current_group = true, bool include_sub_nodes = true);
	void			select_item(tree<CTreeItem>::pre_order_iterator it, bool single_select = true);
	CRect			get_item_rect(tree<CTreeItem>::pre_order_iterator it, bool text_only);
	CString			get_text(tree<CTreeItem>::pre_order_iterator it);
	void			ensure_visible(tree<CTreeItem>::pre_order_iterator it);

	void			use_expand_button(bool use = true);
	//expand (1:expand, 0:collapse, -1:toggle)
	void			expand(tree<CTreeItem>::pre_order_iterator it, int expand = 1);

	//��Ʈ ����
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

	//�ؽ�Ʈ ������ �о������ �����ڵ� ���ο� ���� ���嵵 �����ϰ� ����. default = true
	//bool			m_unicode_file;

	//����Ű �� Ű���� �Է��� ó���� �� �ƴϸ� parent���� �׳� �ѱ���. default = true;
	bool			m_use_keyboard;
	//Ȯ��, ��� ��ư�� ǥ���� ������. default = true;
	bool			m_use_expand_button;
	int				m_expand_button_size;

	//üũ�ڽ� Ʈ������
	bool			m_use_check_box;
	//��Ʈ���� üũ�ڽ��� ǥ���� ������
	bool			m_check_box_at_root;
	//checkbox width
	int				m_check_box_size;
	//üũ�ڽ��� �ؽ�Ʈ ������ ���� ũ��
	int				m_check_box_text_gap;
	//�̹�������Ʈ�� ����� ������. default = false
	bool			m_use_image_list;
	//�̹�������Ʈ�� �⺻ �̹��� ũ��. ����� �̹����� ������ �ƴ�. default = 16
	int				m_image_list_size;
	//���� �׸��� �׻� ���̵��� �� ������. default = true;
	bool			m_always_show_selection;
	//multi selection. default = false;
	bool			m_use_multiple_selection;

	//Ʈ�� ����(���� ��� �� �ƴ϶� ������, �ϴܵ� ����Ǿ�� �Ѵ�)
	CSize			m_margin;

	//indent size in pixel
	int				m_indent_size;

	//�� ����� �ּ����� ����
	int				m_line_gap;
	//�� ����� ���� = m_line_gap, m_lf.lfHeight, icon_size�� ���� ���� �����ǰ� �̸� �̿��� ��ũ�� ũ�⵵ �������ȴ�.
	int				m_line_height;
	//���� �ڵ����� ������ ���̸� �������� �ʰ� �����ϰ� �ʹٸ�
	bool			m_auto_line_height;	//default = true;
	void			recalculate_line_height();
	//��ũ�� ũ�⸦ �����Ѵ�.
	void			recalculate_scroll_size();

//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	//��ܺ��� �������� ���� ������ �������� ã�´�.
	tree<CTreeItem>::pre_order_iterator get_item_by_index(int index);
	//Ȯ��, ��Ҹ� ����Ͽ� ���� �׸��� �����Ѵ�. ���� ��尡 ��ҵ� ���¶�� ���� ���� ���� ����� ���ϵ尡 �ƴ� sibling�� �ȴ�.
	tree<CTreeItem>::pre_order_iterator get_next_visible_item(tree<CTreeItem>::pre_order_iterator it);
	//Ȯ��, ��Ҹ� ����Ͽ� ���� �׸��� �����Ѵ�. ���� ����� �ٷ� ������ ã���� �� ���� collapsed�� �� �ִ�.
	//���� ����� prev_sibling�� ã�Ƽ� �������鼭 get_next_visible_item�� ã�ƾ��Ѵ�.
	tree<CTreeItem>::pre_order_iterator get_previous_visible_item(tree<CTreeItem>::pre_order_iterator it);
	//��ҵ� ��带 �����ϰ� ȭ�鿡 ǥ���ؾ� �ϴ� �� ����� ����. ��ũ�� ũ�� ����ÿ� ���ȴ�.
	int				get_visible_item_count();

	COLORREF		m_crBack;
	COLORREF		m_crText;
	COLORREF		m_crCheck;
	COLORREF		m_crTextSelected;
	COLORREF		m_crBackSelected;

	//��� ���� ����
	bool			m_use_node_line;	//default = true
	COLORREF		m_node_line_color;
	int				m_node_line_style;
	int				m_node_line_width;

	//��ũ�ѹٸ� ������� �ʰ� ��ũ���ϱ�. �� �Ǵ� Ű����θ�.
	bool			m_use_scrollbar;	//default = true;
	int				m_scroll_total;
	int				m_scroll_pos;
	int				m_scroll_unit;		//PgUp, PgDn, �� ��ũ�� �� �� ���ξ� ��ũ������). default = 1

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
