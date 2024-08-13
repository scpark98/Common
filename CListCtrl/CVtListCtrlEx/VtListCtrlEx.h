#pragma once

#include <deque>
#include <Afxwin.h>
#include "list_data.h"
#include "HeaderCtrlEx.h"
#include "../../GdiplusBitmap.h"
#include "../../Functions.h"
#include "../../system/ShellImageList/ShellImageList.h"

/*
* CVtListCtrlEx�� ����ϱ� ���� ������Ʈ�� �߰��ؾ� �ϴ� �ҽ���
* Common/CListCtrl/CVtListCtrlEx/*
* Common/Functions.*
* Common/colors.*
* Common/system/ShellImageList.*
* Common/GdiPlusBitmap.*
*/

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

<������ ����>
-� �÷��� �̹����� ǥ���� ��찡 �����Ƿ� column type�� �߰��ϰ� sort�� �׿� �°� ó���Ǿ�� �Ѵ�.
*/
// CVtListCtrlEx

#define LCSB_CLIENTDATA 1
#define LCSB_NCOVERRIDE 2

static const UINT Message_CVtListCtrlEx = ::RegisterWindowMessage(_T("MessageString_CVtListCtrlEx"));

class CVtListCtrlExMessage
{
public :
	CVtListCtrlExMessage(CWnd* _this, int _message, CWnd* _pTarget = NULL)
	{
		pThis = _this;
		message = _message;
		pTarget = _pTarget;
	}

	CWnd*	pThis = NULL;
	CWnd*	pTarget = NULL;
	int		message;
};

class CVtFileInfo
{
public:
	CVtFileInfo(CString _path, uint64_t _size = 0, CString _date = _T(""))
	{
		if (_size == 0)
		{
			if (PathIsDirectory(_path))
				_size = 0;
			else
				_size = get_file_size(_path);
		}

		if (_date.IsEmpty())
		{
			_date = get_datetime_string(GetFileLastModifiedTime(_path), 2, true, _T(" "), false, false);
		}

		path = _path;
		size = _size;
		date = _date;
	}

	//sort�� lamda����� �ε����� �����ϵ��� �ϱ� ���� text[3];�� ���� �迭�� �����߾�����
	//3���ۿ� ���� �ʰ� ��Ȯ�� ����ϰ��� Ÿ�Կ� �°� ������
	CString		path;
	uint64_t	size;
	CString		date;
};


class CVtListCtrlEx : public CListCtrl
{
	DECLARE_DYNAMIC(CVtListCtrlEx)

public:
	CVtListCtrlEx();
	virtual ~CVtListCtrlEx();

	void		modify_style();

	BOOL NCOverride;
	int Who;

	BOOL m_bHorizLine = TRUE;
	void HideScrollBars(int Type, int Which = SB_BOTH);

	enum CVtListCtrlExMsgs
	{
		message_progress_pos = 0,
		message_drag_and_drop,
	};


//ShellList�� ���۽�Ű��
	bool		m_is_shell_listctrl = false;
	bool		m_is_shell_listctrl_local = true;
	bool		is_shell_listctrl() { return m_is_shell_listctrl; }
	bool		is_shell_listctrl_local() { return m_is_shell_listctrl_local; }
	//�� ó�� �� ����� �ָ� ��� ���ð� ������ ShellListCtrl�� ���۵ȴ�.
	//���� ���� ShellList�� �ƴ� ���·� ���۽�Ű�� ���� ������� �ʴ´�.
	//is_local�� true�̸� ���ϸ���� ���� ���ͼ� ǥ��������
	//false, �� remote�� ���� ���ϸ���� �޾Ƽ� ǥ���ؾ� �Ѵ�.
	void		set_as_shell_listctrl(bool is_local = true);
	//local�� ���� ��θ� �ָ� �ڵ����� ������� ǥ��
	CString		get_path() { return m_path; }
	//path�� �޾� m_path�� �����ϰ� refresh_list()�� ȣ���Ѵ�.
	void		set_path(CString path, bool refresh = true);
	//local�� �ƴ� ���� ���ϸ���� �޾Ƽ� ǥ���Ѵ�.
	void		set_filelist(std::deque<CVtFileInfo>* pFolderList, std::deque<CVtFileInfo>* pFileList);

	//reload�� true��� local�� ���ϸ���� �ٽ� �ҷ������� false��� ����� �ٽ� ����Ʈ�� ǥ���Ѵ�
	//(sort�� ��� ����/���� ����� sort�Ͽ� ������ �� ���ϸ���� reload���� �ʴ´�)
	//�ٸ� �ڵ忡 ���� �̹� m_cur_folders/m_cur_files �� ä�����ٸ� reload�� false�� ȣ���Ѵ�.
	void		refresh_list(bool reload = true);

	//������ ������ ������ ó���� ������ ���Ľÿ� ���ϰ� ������ ���� ó���Ǳ� ����
	std::deque<CVtFileInfo> m_cur_folders;
	std::deque<CVtFileInfo> m_cur_files;
	//���� �Ǵ� ������ �ش��ϴ� ��� ����Ʈ�� �߰��Ѵ�.
	//local�� ��� ũ��� ��¥���� ����ִٸ� �ڵ� ä���ְ� remote��� ��������� �ȵȴ�.
	void		add_file(CString path, uint64_t size, CString date = _T(""), bool is_remote = false, bool is_folder = false);
	//local�� �ƴ� remote�� ��� �Ѱܹ��� ����ü������ ����� �߰��Ѵ�.
	void		add_file(WIN32_FIND_DATA* pFindFileData);

	enum SHELL_LIST_COLUMN
	{
		col_filename = 0,
		col_filesize,
		col_filedate,
	};

	CString		m_path;
	CShellImageList*	m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

//�÷� ����
	//ex. "No,20;Item1,50;Item2,50"
	//�÷� ������ ���õ� ������ ���� ũ��� �÷��� ���� �����Ǹ� ��������.
	//���� set_headings()�Լ��� ���� ���� ȣ���ϰ� �� �Ŀ� �÷� ������ �ϵ��� ����.
	bool		set_headings(const CString& strHeadings);
	
	int			get_column_count();

	//default = LVCFMT_LEFT
	void		set_column_text_align(int column, int format = LVCFMT_LEFT, bool set_to_header = true);
	int			get_column_text_align(int column);

	//�ش� �÷����� ���ڿ� ���� �ִ밪�� ����.
	//real_
	int			get_column_max_text_length(int column, bool real_bytes);
	void		get_column_max_text_length(std::vector<int>& col_len, bool real_bytes);
	//�ش� �÷����� ��½ÿ� width �ִ밪�� ����.(�̱���)
	int			get_column_max_text_width(int column);

	CString		get_header_text(int column);
	void		set_header_text(int column, CString text);
	//default = HDF_LEFT
	void		set_header_text_align(int header, int format = HDF_LEFT);
	int			get_header_text_align(int header);

	enum
	{
		column_data_type_unknown = 0,
		column_data_type_text,
		column_data_type_text_ip,		//ip�� text�� ���������� sort ����� �ٸ���.
		column_data_type_numeric,

		//�������� �������� �׷����� ����׷����� �׷���. 100%�� �ִ밪�̹Ƿ� %�� ��ȯ�Ͽ� ���� ��� ��.
		//100�Ѵ� ���� 100%�� ǥ�õȴ�.
		//���� �ִ밪�� �����ϰų� ���� �ٸ� ó���� �ʿ��ϴٸ� �ΰ����� �ڵ尡 �ʿ��ϹǷ� �켱 100%�� max�� ������.
		//�ϴ� �� �÷��� ������ Ÿ���� percentage�� ����׷����� �׷��ִ� ���� �����̹Ƿ�
		//���� 0~100������ ������ �ƿ� �־�����. 110���� �־ �������. �ٸ� 100%�� �׷��� ���̴�.
		column_data_type_percentage_bar,
		column_data_type_percentage_grid,
		//progress�� ����׷��� ����� �ƴ� ���� ������¸� ǥ���ϴ� ProgressBar�� ������.
		column_data_type_progress,
	};
	int			get_column_data_type(int column);
	void		set_column_data_type(int column, int nType = column_data_type_text, bool invalidate = false);

//�߰� ����
	//index ��ġ�� 0�� �÷��� text�� ������ �߰��Ѵ�.(-1�̸� �� �������� �߰�)
	int			add_item(CString text = _T(""), bool ensureVisible = true, bool invalidate = true);
	int			add_line_string_item(CString line_string, TCHAR separator = '|', bool ensureVisible = true, bool invalidate = true);
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
	//���� ��ü�� �����̸� �ش� ������ �����Ѵ�.
	void		delete_empty_lines();
	void		delete_all_items(bool delete_file_list = true);
	void		DeleteAllItems() { delete_all_items(); }

//�ؽ�Ʈ ����
	CString		get_text(int item, int subItem);
	void		set_text(int item, int subItem, CString text, bool invalidate = true);
	//from ~ to �÷������� �ؽ�Ʈ�� �ϳ��� ���ļ� �������ش�.
	CString		get_line_text(int index, int from = 0, int to = -1, CString sep = _T("|"));
	CString		get_line_text(int index, std::deque<int>* dqColumn = NULL, CString sep = _T("|"));
	std::deque<CString> get_line_text_list(int index, int from = 0, int to = -1);
	std::deque<CString> get_line_text_list(int index, std::deque<int>* dqColumn = NULL);
	//txt ��� �� �÷� ���� ������ �°� ����ϵ��� ������ �����Ͽ� ����
	//CString�� �ִ� 64K������ ���� �� �����Ƿ� vector�� �̿���.
	void		get_line_text_list(std::vector<CString> *vt);

	//����Ʈ�� ǥ���� �׸��� ���� ��� ǥ���� �ؽ�Ʈ ����
	void		set_text_on_empty(CString text) { m_text_on_empty = text; Invalidate(); };

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

	//�������� ���°��� Ư�� ���°��� �׸� �Ǵ� �� ���� ���ϱ�
	//LVIS_DROPHILITED or LVIS_SELECTED �׸��� ���� �� �ִ�.
	//drag ���߿� ���콺�� �ٸ� �� �������� ������ WM_LBUTTONUP �� ��� drophilited ���·� �������� ���� ������ �����ϱ� ����.
	int			get_items_state(UINT state, std::deque<int>* dq = NULL);
	//dq ����� �����۵��� state ����. dq�� null�̸� ��� �׸� ���� ����
	//���� : set_items_state(LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED, dq);
	//���� : set_items_state(0, LVIS_SELECTED|LVIS_FOCUSED, dq);
	int			set_items_state(UINT state, UINT mask, std::deque<int>* dq = NULL);

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
	void		sort_by_text_color(int column, int ascending, bool text_sort_on_same_color = true);
	int			cur_sorted_column_index() { return m_cur_sorted_column; }

//�÷� ����
	enum listctrlex_color_theme
	{
		color_theme_default = 0,
		color_theme_light_blue,
		color_theme_navy_blue,
		color_theme_dark_blue,
		color_theme_dark_gray,
		color_theme_dark,
	};

	void		set_color_theme(int theme, bool apply_now = true);
	COLORREF	get_text_color(int item, int subItem);
	COLORREF	get_back_color(int item, int subItem);
	//Ư�� ���� �ƴ� �⺻ ������ ����.
	COLORREF	get_back_color() { return m_crBack; }
	COLORREF	get_back_alt_color(int item, int subItem) { return m_crBackAlt; }
	void		set_back_alt_color(COLORREF cr) { m_crBackAlt = cr; }

	//Ư�� �׸��� ���ڻ� ����. erase�� true�̸� crText ���ڸ� �����ϰ� �⺻ ���ڻ����� �ǵ�����.
	//item�� -1�̸� ��� ���ο�, subItem�� -1�̸� ��� �÷��� ����.
	void		set_text_color(int item, int subItem, COLORREF crText, bool erase = false, bool invalidate = true);
	//Ư�� �׸��� ���� ����. erase�� true�̸� crText ���ڸ� �����ϰ� �⺻ ���ڻ����� �ǵ�����.
	void		set_back_color(int item, int subItem, COLORREF crBack, bool erase = false);
	void		set_item_color(int item, int subItem, COLORREF crText, COLORREF crBack);

	//�⺻ ���ڻ��� �����Ѵ�.
	void		set_default_text_color(COLORREF cr_text) { m_crText = cr_text; Invalidate(); }
	//�⺻ ������ �����Ѵ�.
	void		set_default_back_color(COLORREF cr_back) { m_crBack = cr_back; Invalidate(); }

	//�÷��� percentage�� ǥ���ϴ� �÷��� ��� �� ǥ�� ������ �����Ѵ�.
	void		set_progress_color(COLORREF crProgress);
	template <typename ... Types> void set_percentage_gradient(Types... args)
	{
		int n = sizeof...(args);
		COLORREF arg[] = { args... };

		m_crPercentage.clear();

		for (auto item : arg)
		{
			m_crPercentage.push_back(item);
		}
	}


//���� ����
	//���ҽ��� ListCtrl �Ӽ����� "���̺� ����" �Ӽ��� default���� false�� �ϰ� �� �Լ��� ���� ���� ���θ� �����ؾ� �Ѵ�.
	void		allow_edit(bool allow_edit = true, bool one_click_edit = true);
	void		allow_one_click_edit(bool allow_one_click_edit) { m_allow_one_click_edit = allow_one_click_edit; }
	void		allow_edit_column(int column, bool allow_edit = true);
	bool		is_in_editing()	{ return m_in_editing; }	//����������
	void		set_flag_in_editing(bool in_editing) { edit_end(); m_in_editing = in_editing; }
	int			get_recent_edit_item() { return m_edit_item; }
	int			get_recent_edit_subitem() { return m_edit_subItem; }
	void		set_recent_edit_subitem(int subItem) { m_edit_subItem = subItem; }
	std::deque<int>* get_selected_list_for_edit() { return &m_dqSelected_list_for_edit; }
	std::deque<int> m_dqSelected_list_for_edit;
	CString		get_old_text() { return m_old_text; }
	CEdit*		edit_item(int item, int subItem);
	void		edit_end(bool valid = true);
	void		undo_edit_label();		//���� ���� �ؽ�Ʈ�� �ǵ�����.(���� ��� ���� ���̺��� ���ϸ��̰� ���ϸ� ������ ������ ��� �� �� �ִ�.)
	bool		is_modified() { return m_modified; }
	void		reset_modified_flag() { m_modified = false; }

//��Ʈ ����. �ݵ�� set_headings() �Ŀ� ȣ���Ұ�.
	void		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	//-1 : reduce, +1 : enlarge
	int			get_font_size();
	//�ݵ�� set_headings() �Ŀ� ȣ���Ұ�.
	void		set_font_size(int font_size);
	void		enlarge_font_size(bool enlarge);
	void		set_font_bold(bool bold = true);
	void		set_font_italic(bool italic = true);
	LOGFONT		get_log_font() { return m_lf; }
	void		set_log_font(LOGFONT lf);

//����, Ŭ������ ����
	//Ctrl+CŰ�� ���õ� �׸��� Ŭ������� ������ �� �ִ�. shift�� �����ϸ� ������� ���Եȴ�.
	bool		list_copy_to_clipboard(bool selected_only = true, TCHAR separator = '|', bool bHead = false);
	//void	paste_from_clipboard();
	bool		save(CString file, TCHAR separator = '|', bool selected_only = false);
	bool		load(CString sfile, TCHAR separator = '|', bool add_index = false, bool match_column_count = true, bool reset_before_load = true);

//��Ÿ ���̾ƿ� ����
	//header_height�� ���Ŭ�������� HDM_LAYOUT �޽����� �߻��ؾ� ����Ǵµ�
	//�׷��� �Ǳ� ���ؼ��� SetFont / MoveWindow / SetWindowPos ���� �ʿ��ϹǷ�
	//VtListCtrlEx���� set_font_name() ȣ��ÿ� ����� SetFont�� �����Ͽ� �ذ���.
	//=> SetFont()�� ���ؼ� header�� height�� �����Ѵ�.
	void		set_header_height(int height);

	//line height�� �����ϴ� ����� ������ �̹�������Ʈ�� �̿��ϴ� �����
	//(���� ����� �̹�������Ʈ�� �ִ� ���� �� ����� ����� �� ����)
	//MeasureItem�� �̿��ϴ� ���(OwnerDrawFixed only)�� �ִ�.
	//(���� ���� ��� ���� ���ذ��̸� resize�� �ϸ� �ڵ� ����ǹǷ�
	//�켱�� mainDlg���� �� �Լ� ȣ���Ŀ� RestoreWindowPosition()��� ����
	//CVtListCtrlEx�� WM_SIZE�� �߻��ϵ��� �ϴ� �Լ��� ����ϸ� �ȴ�)
	//CVtListCtrlEx�� �ƴ� �׳� mainDlg�� resize�Ѵٰ� �ؼ� ���� �ʴ´�.
	//��ü imagelist�� ������� �ʴ� ��Ʈ���̶�� ���� lineheight�� ������ �� �ִ�.
	bool		m_use_own_imagelist = true;
	void		set_use_own_imagelist(bool use) { m_use_own_imagelist = use; }
	void		set_line_height(int height);

	void		set_column_width(int nCol, int cx);
	void		load_column_width(CWinApp* pApp, CString sSection);
	void		save_column_width(CWinApp* pApp, CString sSection);
	CRect		get_item_rect(int item, int subItem);
	//Ŭ����ġ�� ���� item�� �ùٸ��� �Ǻ��ǳ� subItem�� �׷��� �ʾƼ�(���콺 �̺�Ʈ �ڵ鷯 �Լ�����) ���� �߰���.
	bool		get_index_from_point(CPoint pt, int& item, int& subItem, bool include_icon);

	void		show_progress_text(bool show = true) { m_show_progress_text = show; Invalidate(); }
	void		progress_text_color(COLORREF cr) { m_crProgressText = cr; }

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

	//�׸��� �߰��Ǹ� auto scroll������ Ư�� �׸��� �����ϸ� false�� �ȴ�.
	bool			m_auto_scroll = true;


	void random();

	DWORD index_from_point(int x, int y);

	//Drag&Drop �巡�� ����
	template <typename ... Types> void add_drag_images(Types... args) //(�������Ͽ� �̹���, �̱����Ͽ� �̹����� ���ʴ�� �ְ� drag�Ǵ� ������ ���� �´� �̹����� ����Ѵ�)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			m_drag_images_id.push_back(id);
	}

	bool			use_drag_and_drop() { return m_use_drag_and_drop; }
	void			use_drag_and_drop(bool use_drag) { m_use_drag_and_drop = use_drag; }
	int				get_drop_index() { return m_nDropIndex; }
	void			capture_selected_items_to_bitmap(CGdiplusBitmap* bmp);

protected:

//���� ������
	std::deque<CListCtrlData> m_list_db;
	
	CString			m_text_on_empty;

//�÷� ����
	CHeaderCtrlEx	m_HeaderCtrlEx;
	std::deque<int> m_column_data_type;
	std::deque<int> m_column_text_align;

	int				m_line_height = 16;

//���� ����
	bool			m_allow_sort = true;
	std::deque<int> m_column_sort_type;			//asceding or descending
	int				m_cur_sorted_column = 0;	//���ĵ� �÷� �ε���(���� ������ ����)

//�÷� ����
	COLORREF		m_crText;					//�⺻ ���ڻ�
	COLORREF		m_crTextSelected;			//���� �׸��� Ȱ��ȭ(active) ���ڻ�
	COLORREF		m_crTextSelectedInactive;	//���� �׸��� ��Ȱ��ȭ(inactive) ���ڻ�
	COLORREF		m_crTextDropHilited;
	COLORREF		m_crBack;					//�⺻ ����
	COLORREF		m_crBackAlt;				//�� ���п�
	COLORREF		m_crBackSelected;
	COLORREF		m_crBackSelectedInactive;
	COLORREF		m_crBackDropHilited;
	COLORREF		m_crSelectedBorder;
	COLORREF		m_crHeaderBack;
	COLORREF		m_crHeaderText;
	std::deque<COLORREF> m_crPercentage;		//percentage bar graph color
	COLORREF		m_crProgress;				//progress bar
	COLORREF		m_crProgressText;			//progress text

//������� ����
	bool			m_allow_edit = false;			//�׸� ������ ��������...
	bool			m_allow_one_click_edit = false;	//�׸��� Ŭ���� �� ������ �ð����� �ٽ� Ŭ���� �� �������� ����
	std::deque<bool> m_allow_edit_column;			//�� �÷����� �������� ���θ� ������ �� �ִ�. ��, m_allow_edit�� ����Ǹ� ��� �� ������ ����ʿ� ����.
	bool			m_modified = false;				//�׸��� ���� �Ǵ� ������ ������ ��� true
	bool			m_in_editing = false;			//����������
	int				m_edit_item = -1;				//�������� ������ �ε���
	int				m_edit_subItem = -1;			//�������� ������ �����ε���
	CString			m_old_text = _T("");			//���� �� �ؽ�Ʈ
	CEdit*			m_pEdit = NULL;
	long			m_last_clicked_time = 0;		//one_click���� ������� ���� �� ������ Ŭ�� �ð�
	int				m_last_clicked_index = -1;		//one_click���� ������� ���� �� ������ Ŭ�� �ε���

//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	bool			m_show_progress_text = true;

//Drag&Drop �巡�� ����
	bool			m_use_drag_and_drop = false;
	CWnd*			m_pDragWnd = NULL;			//Which ListCtrl we are dragging FROM
	CWnd*			m_pDropWnd = NULL;			//Which ListCtrl we are dropping ON
	CImageList*		m_pDragImage = NULL;		//For creating and managing the drag-image
	bool			m_bDragging = false;		//T during a drag operation
	int				m_nDragIndex = -1;			//Index of selected item in the List we are dragging FROM
	int				m_nDropIndex = -1;			//Index at which to drop item in the List we are dropping ON(drag�� ������ ��Ʈ���� ������� �����, ��ӵ� Ŭ�������� ������� ����)
	std::deque<UINT> m_drag_images_id;			//drag�� �� ����ϴ� �̹������� resource id ����(�������Ͽ� �̹���, �̱����Ͽ� �̹����� ���ʴ�� �ְ� drag�Ǵ� ������ ���� �´� �̹����� ����Ѵ�)
	void			DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd);

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
	afx_msg void OnLvnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg BOOL OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
};


