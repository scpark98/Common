#pragma once

#include <deque>
#include <Afxwin.h>
#include "list_data.h"
#include "HeaderCtrlEx.h"
#include "../../Functions.h"

/*
CListCtrl�� �� ������ CListCtrlData��� Ŭ������ �ν��Ͻ��� �����Ͽ�
virtual list ����� �̿��ϵ��� �ۼ��� Ŭ����.
resource editor����
virtual list ����� �̿��ϱ� ���� Owner Data�� true��,
DrawItem�� �̿��ؼ� ���� �׸��� ���� Owner Draw Fixed�� true�� ����� �Ѵ�.

<����!>
�� ������ �����Ͱ� m_list_db��� deque�� ����ǰ� �̸� �̿��Ͽ� virtual list�� �����ϴ� ���̹Ƿ�
������ �߰�, �����ÿ� �ݵ�� m_list_db ���� ���� ����Ǿ�� �Ѵ�.
���� ���� CListCtrl�� InsertItem, DeleteItem, DeleteAllItems��� ���� �Լ���
�״�� ����ؼ��� �ȵȴ�.
�ݵ�� ���� ������ insert_item, add_item, delete_item, delete_all_items�� ���� �Լ��� �̿��ؼ�
�߰�, ������ �Ǿ�� �Ѵ�.
���� GetItemText�� ������ �ؽ�Ʈ�� �о���� �Լ��̹Ƿ� �ᵵ ���������
SetItemText�� ���� ����Ʈ���� ���� ���� m_list_db���� ������ �ʴ� �Լ��̴�
�ݵ�� set_text �Լ��� �̿��ؼ� ��� �Ѵ�.

<����!>
����Ʈ ��Ʈ���� column�� header�� ��� ���� ������ �� ������
text align�� ���� ����Ʈ�� ���� ��İ� header�� ���� ����� �ٸ��� ������ �� �ֵ���
����Ʈ ��Ʈ���� column�� ���õ� ���� column����,
��� ��Ʈ���� column�� ���õ� ���� header��� Ű���带 �������.

[���� �Ϸ� ��]
-��Ƽ ���� ���¿��� ���� �Ϸ�� �� shift�� ����ä�� �Ϸ�Ǹ� ��Ƽ �׸� ��� ������ �������� ����ȴ�.

<���� ����>
m_list.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FLATSB | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
m_list.UsePopupMenu(false);
m_list.AllowSort();
m_list.set_color_theme(CVtListCtrlEx::color_theme_dark_gray);
m_list.set_progress_color(RGB(128, 255, 128));
m_list.set_headings(_T("����,200;������,60;�з�,60;����,60;ũ��,200;�޸�,100;���,300;��¥,100"));
m_list.set_header_height(24);
*/
// CVtListCtrlEx

#define LCSB_CLIENTDATA 1
#define LCSB_NCOVERRIDE 2

#define MESSAGE_VTLISTCTRLEX	WM_USER + 0x7FFF - 0x7464

class CVtListCtrlEx : public CListCtrl
{
	DECLARE_DYNAMIC(CVtListCtrlEx)

public:
	CVtListCtrlEx();
	virtual ~CVtListCtrlEx();

	BOOL NCOverride;
	int Who;

	BOOL m_bHorizLine = TRUE;
	void HideScrollBars(int Type, int Which = SB_BOTH);

	enum VtListCtrlExMsgs
	{
		message_progress_pos = 0,
	};


//�÷� ����
	//ex. "No,20;Item1,50;Item2,50"
	//�÷� ������ ���õ� ������ ���� ũ��� �÷��� ���� �����Ǹ� ��������.
	//���� set_headings()�Լ��� ���� ���� ȣ���ϰ� �� �Ŀ� �÷� ������ �ϵ��� ����.
	bool		set_headings(const CString& strHeadings);
	
	int			get_column_count();

	//default = LVCFMT_LEFT
	void		set_column_text_align(int column, int format = LVCFMT_LEFT, bool set_to_header = true);
	int			get_column_text_align(int column);

	CString		get_header_text(int column);
	void		set_header_text(int column, CString text);
	//default = HDF_LEFT
	void		set_header_text_align(int header, int format = HDF_LEFT);
	int			get_header_text_align(int header);

	enum
	{
		column_data_type_unknown = 0,
		column_data_type_text,
		column_data_type_numeric,
		//�������� �������� �׷����� ����׷����� �׷���. 100%�� �ִ밪�̹Ƿ� 100�Ѵ� ���� 100%�� ǥ�õȴ�.
		//���� �ִ밪�� �����ϰų� ���� �ٸ� ó���� �ʿ��ϴٸ� �ΰ����� �ڵ尡 �ʿ��ϴ�.
		//�ϴ� �� �÷��� ������ Ÿ���� percentage�� ����׷����� �׷��ִ� ���� �����̹Ƿ�
		//���� 0~100������ ������ �ƿ� �־�����. 110���� �־ �������. �ٸ� 100%�� �׷��� ���̴�.
		column_data_type_percentage_bar,
		column_data_type_percentage_grid,
	};
	int			get_column_data_type(int column);
	void		set_column_data_type(int column, int nType = column_data_type_text, bool invalidate = false);

//�߰� ����
	//index ��ġ�� 0�� �÷��� text�� ������ �߰��Ѵ�.(-1�̸� �� �������� �߰�)
	int			add_item(CString text = _T(""), bool ensureVisible = true, bool invalidate = true);
	int			add_line_string_item(CString line_string, CString separator = _T("|"), bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, CString text = _T(""), bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, std::deque<CString> dqText, bool ensureVisible = true, bool invalidate = true);

	//�������� ���ڸ� args�� �ִ� ��� ��ϵ��� �ʴ� ������ �ִ�. Ȯ�� �ʿ�!!
	template <typename ... Types> int insert_item(int index, Types... args)
	{
		int n = sizeof...(args);
		CString arg[] = { args... };

		int subItem = 0;
		if (index < 0)
			index = size();

		for (auto item : arg)
		{
			if (subItem == 0)
				index = insert_item(index, item, false, false);
			else
				set_text(index, subItem, item, false);
			subItem++;
		}

		return index;
	}

	int			size();

//���� ����
	//����� �� Ȯ��â�� ȣ���ƾ���� ó���ؾ� ��
	void		delete_selected_items();
	void		delete_item(int index);
	void		delete_all_items();
	void		DeleteAllItems() { AfxMessageBox(_T("use delete_all_items() function in virtual list control.")); }

//�ؽ�Ʈ ����
	CString		get_text(int item, int subItem);
	void		set_text(int item, int subItem, CString text, bool invalidate = true);
	//from ~ to �÷������� �ؽ�Ʈ�� �ϳ��� ���ļ� �������ش�.
	CString		get_line_text(int index, int from = 0, int to = -1, CString sep = _T("|"));
	CString		get_line_text(int index, std::deque<int>* dqColumn = NULL, CString sep = _T("|"));
	std::deque<CString> get_line_text_list(int index, int from = 0, int to = -1);
	std::deque<CString> get_line_text_list(int index, std::deque<int>* dqColumn = NULL);
	

//���� ����
	//start���� �� ó�� ���õ� �׸��� �ε����� �����Ѵ�.
	int			get_selected_index(int start = 0);
	//�� ���������� ���õ� �� �׸��� �ε����� �����Ѵ�.
	int			get_last_selected_item();
	//���õ� �׸���� dqSelected�� ��´�. dqSelected�� null�̸� �׳� ���� ������ ���Ϲ޾� ����Ѵ�.
	int			get_selected_items(std::deque<int> *dqSelected = NULL);
	//index = -1 : ��ü����
	void		select_item(int index, bool select = true, bool after_unselect = false);
	void		unselect_selected_item();

//�˻� ����
	//0�� �÷������� �����͸� ã�´�.
	int			find_string(CString str, int start = -1, bool bWholeWord = false, bool bCaseSensitive = false);
	//separator�� ""�� �ƴ� ���� �Ǵ� �ٸ� ���ڿ��� ����
	//str�� �и��ؼ� ��� ã�´�.
	void		find_string(CString find_target, std::deque<int>* result,
							int start_idx = 0, int end_idx = -1, int start_column = 0, int end_column = -1,
							bool result_reset = true, bool bWholeWord = false, bool bCaseSensitive = false, bool select = false);
	int			find_string(CString find_target, std::deque<int>* result,
							int start_idx = 0, int end_idx = -1,
							std::deque<int>* dqColumn = NULL, bool stop_first_found = false);

//���� ����
	enum listctrlex_sort_type
	{
		sort_none = -1,
		sort_descending,
		sort_ascending,
	};
	void		allow_sort(bool allow) { m_allow_sort = allow; }
	void		sort(int column, int ascending);
	void		sort_by_text_color(int column, int ascending);
	int			last_sorted_column_index() { return m_last_sorted_column; }

//�÷� ����
	enum listctrlex_color_theme
	{
		color_theme_default = 0,
		color_theme_light_blue,
		color_theme_navy_blue,
		color_theme_dark_blue,
		color_theme_dark_gray,
	};

	void	set_color_theme(int theme, bool apply_now = true);
	COLORREF get_text_color(int item, int subItem);
	COLORREF get_back_color(int item, int subItem);
	//Ư�� �׸��� ���ڻ� ����. erase�� true�̸� crText ���ڸ� �����ϰ� �⺻ ���ڻ����� �ǵ�����.
	void	set_text_color(int item, int subItem, COLORREF crText, bool erase = false, bool invalidate = true);
	//Ư�� �׸��� ���� ����. erase�� true�̸� crText ���ڸ� �����ϰ� �⺻ ���ڻ����� �ǵ�����.
	void	set_back_color(int item, int subItem, COLORREF crBack, bool erase = false);
	void	set_item_color(int item, int subItem, COLORREF crText, COLORREF crBack);
	void	set_default_item_color(COLORREF crText, COLORREF crBack);	//�⺻ ���ڻ�, ������ �����Ѵ�.
	//�÷��� percentage�� ǥ���ϴ� �÷��� ��� �� ǥ�� ������ �����Ѵ�.
	void	set_progress_color(COLORREF crProgress);

//���� ����
	//���ҽ��� ListCtrl �Ӽ����� "���̺� ����" �Ӽ��� default���� false�� �ϰ� �� �Լ��� ���� ���� ���θ� �����ؾ� �Ѵ�.
	void	allow_edit(bool allow_edit = true, bool one_click_edit = true);
	void	allow_one_click_edit(bool allow_one_click_edit) { m_allow_one_click_edit = allow_one_click_edit; }
	void	allow_edit_column(int column, bool allow_edit = true);
	bool	is_in_editing()	{ return m_in_editing; }	//����������
	void	set_flag_in_editing(bool in_editing) { edit_end(); m_in_editing = in_editing; }
	int		get_recent_edit_item() { return m_edit_item; }
	int		get_recent_edit_subitem() { return m_edit_subItem; }
	void	set_recent_edit_subitem(int subItem) { m_edit_subItem = subItem; }
	std::deque<int>* get_selected_list_for_edit() { return &m_dqSelected_list_for_edit; }
	std::deque<int> m_dqSelected_list_for_edit;
	CString get_old_text() { return m_old_text; }
	CEdit*	edit_item(int item, int subItem);
	void	edit_end(bool valid = true);
	void	undo_edit_label();		//���� ���� �ؽ�Ʈ�� �ǵ�����.(���� ��� ���� ���̺��� ���ϸ��̰� ���ϸ� ������ ������ ��� �� �� �ִ�.)
	bool	is_modified() { return m_modified; }
	void	reset_modified_flag() { m_modified = false; }

//��Ʈ ����
	int		get_font_size();
	void	set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	//-1 : reduce, +1 : enlarge
	void	set_font_size(int font_size);
	void	enlarge_font_size(bool enlarge);
	void	set_font_bold(bool bold = true);
	void	set_font_italic(bool italic = true);
	LOGFONT	get_log_font() { return m_lf; }
	void	set_log_font(LOGFONT lf);

//����, Ŭ������ ����
	//Ctrl+CŰ�� ���õ� �׸��� Ŭ������� ������ �� �ִ�. shift�� �����ϸ� ������� ���Եȴ�.
	bool	list_copy_to_clipboard(bool selected_only = true, CString sSeparator = _T("|"), bool bHead = false);
	//void	paste_from_clipboard();
	bool	save(CString file, CString separator = _T("|"), bool selected_only = false);
	bool	load(CString sfile, CString separator = _T("|"), bool add_index = false, bool match_column_count = true, bool reset_before_load = true);

//��Ÿ ���̾ƿ� ����
	//header_height�� ���Ŭ�������� HDM_LAYOUT �޽����� �߻��ؾ� ����Ǵµ�
	//�׷��� �Ǳ� ���ؼ��� SetFont / MoveWindow / SetWindowPos ���� �ʿ��ϹǷ�
	//VtListCtrlEx���� set_font_name() ȣ��ÿ� ����� SetFont�� �����Ͽ� �ذ���.
	void	set_header_height(int height);

	//line height�� �����ϴ� ����� ������ �̹�������Ʈ�� �̿��ϴ� �����
	//(���� ����� �̹�������Ʈ�� �ִ� ���� �� ����� ����� �� ����)
	//MeasureItem�� �̿��ϴ� ���(OwnerDrawFixed only)�� �ִ�.
	//(���� ���� ��� ���� ���ذ��̸� resize�� �ϸ� �ڵ� ����ǹǷ�
	//�켱�� mainDlg���� �� �Լ� ȣ���Ŀ� RestoreWindowPosition()��� ����
	//CVtListCtrlEx�� WM_SIZE�� �߻��ϵ��� �ϴ� �Լ��� ����ϸ� �ȴ�)
	//CVtListCtrlEx�� �ƴ� �׳� mainDlg�� resize�Ѵٰ� �ؼ� ���� �ʴ´�.
	//��ü imagelist�� ������� �ʴ� ��Ʈ���̶�� ���� lineheight�� ������ �� �ִ�.
	bool	use_own_imagelist = true;
	void	set_use_own_imagelist(bool use) { use_own_imagelist = use; }
	void	set_line_height(int height);

	void	set_column_width(int nCol, int cx);
	void	load_column_width(CWinApp* pApp, CString sSection);
	void	save_column_width(CWinApp* pApp, CString sSection);
	CRect	get_item_rect(int item, int subItem);

//scroll
	enum CLISTCTRLEX_ENSURE_VISIBLE_MODE
	{
		visible_first = 0,
		visible_center,
		visible_last,
	};
	//� �׸��� Ư�� ��ġ�� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode�� visible_first�̰� offset�� 3�̸� ������ 3+1�� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode�� visible_center�̰� offset�� 0�̸� �߾� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode�� visible_last�̰� offset�� 3�̸� �Ʒ����� -3-1�� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode default = visible_center
	void			ensure_visible(int index, int mode = visible_center, int offset = 0);
	//original IsItemVisible method does not check partial. only check vertical axis.
	bool			is_item_visible(int index, bool bPartial = false);

	void random();

	DWORD index_from_point(int x, int y);

protected:
//���� ������
	std::deque<CListCtrlData> m_list_db;

//�÷� ����
	CHeaderCtrlEx	m_HeaderCtrlEx;
	std::deque<int> m_column_data_type;
	std::deque<int> m_column_text_align;

	int				m_line_height = 16;

//���� ����
	bool			m_allow_sort;				//default = true
	std::deque<int> m_column_sort_type;			//asceding or descending
	int				m_last_sorted_column;		//������ ���ĵ� �÷� �ε���(���� ������ ����)

//�÷� ����
	COLORREF		m_crText;					//�⺻ ���ڻ�
	COLORREF		m_crTextSelected;			//���� �׸��� Ȱ��ȭ(active) ���ڻ�
	COLORREF		m_crTextSelectedInactive;	//���� �׸��� ��Ȱ��ȭ(inactive) ���ڻ�
	COLORREF		m_crBack;					//�⺻ ����
	COLORREF		m_crBackSelected;
	COLORREF		m_crBackSelectedInactive;
	COLORREF		m_crHeaderBack;
	COLORREF		m_crHeaderText;
	COLORREF		m_crProgress;				//percentage bar graph color

//������� ����
	bool			m_allow_edit;			//�׸� ������ ��������... �⺻ false
	bool			m_allow_one_click_edit;	//�׸��� Ŭ���� �� ������ �ð����� �ٽ� Ŭ���� �� �������� ����
	std::deque<bool> m_allow_edit_column;	//�� �÷����� �������� ���θ� ������ �� �ִ�. ��, m_allow_edit�� ����Ǹ� ��� �� ������ ����ʿ� ����.
	bool			m_modified;				//�׸��� ���� �Ǵ� ������ ������ ��� true
	bool			m_in_editing;			//����������
	int				m_edit_item;			//�������� ������ �ε���
	int				m_edit_subItem;			//�������� ������ �����ε���
	CString			m_old_text;				//���� �� �ؽ�Ʈ
	CEdit*			m_pEdit;			//
	long			m_last_clicked;

//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();


protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnOdfinditem(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnOdcachehint(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLvnOdstatechanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnNMClickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	//afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
//	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
};


