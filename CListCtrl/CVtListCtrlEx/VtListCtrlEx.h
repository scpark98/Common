#pragma once

#include <deque>
#include <Afxwin.h>
#include "list_data.h"
#include "HeaderCtrlEx.h"
//#include "../../GdiplusBitmap.h"
#include "../../CButton/GdiButton/GdiButton.h"
#include "../../Functions.h"
#include "../../colors.h"
#include "../../system/ShellImageList/ShellImageList.h"
#include "../../CEdit/SCEdit/SCEdit.h"

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

<��Ƽ���� ���� ���� ����>
- ��Ȥ Ʈ��+����Ʈ�� ���·� ��Ƽ������ ������ ��Ʈ�ѵ� ������ �� ��Ʈ�Ѹ����� ������ �������� ��Ƽ������ �����ϰ��� ��
  (Ȯ��, ��� ��� ��������)
- CListCtrlData�� level ����� �߰��Ͽ� �ش� ������ ������ ǥ���Ѵ�.
  0����
	1����
		2����
			3����
- �� ������ ���� üũ�ڽ��� ���� �鿩���� �� ��, üũ�ڽ��� �׻� �� ���ʿ��� ǥ�������� ���� �ʿ�.
- DrawItem()���� level�� ���� �鿩���⸦ �����Ѵ�.

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
	CVtListCtrlExMessage(CWnd* _this, int _message, CWnd* _pTarget = NULL, CString _param0 = _T(""), CString _param1 = _T(""), int _reserved = 0)
	{
		pThis = _this;
		message = _message;
		pTarget = _pTarget;
		param0 = _param0;
		param1 = _param1;
		reserved = _reserved;
	}

	CWnd*	pThis = NULL;
	CWnd*	pTarget = NULL;
	int		message;
	CString param0 = _T("");
	CString param1 = _T("");
	int		reserved = 0;
};

class CVtFileInfo
{
public:
	CVtFileInfo()
	{
		memset(&data, 0, sizeof(data));
		is_remote = false;
	};

	CVtFileInfo(WIN32_FIND_DATA _data, bool _is_remote = false)
	{
		data = _data;
		is_remote = _is_remote;
	}

	CString get_file_time_str()
	{
		return ::get_file_time_str(data.ftLastWriteTime);
	}

	WIN32_FIND_DATA	data;
	//sort�� lamda����� �ε����� �����ϵ��� �ϱ� ���� text[3];�� ���� �迭�� �����߾�����
	//3���ۿ� ���� �ʰ� ��Ȯ�� ����ϰ��� Ÿ�Կ� �°� ������
	//CString			path;
	//ULARGE_INTEGER	filesize;
	//FILETIME		filetime;
	bool			is_remote = false;
	//bool			is_folder = false;
};


class CVtListCtrlEx : public CListCtrl
{
	DECLARE_DYNAMIC(CVtListCtrlEx)

public:
	CVtListCtrlEx();
	virtual ~CVtListCtrlEx();

	//virtual list ����� ����ϱ� ���ؼ��� Owner Data, Owner Draw Fixed�� ��� true�� �����ؾ� �Ѵ�.
	//�� �ɼ��� �������� ���� �Ұ��ϰ� WM_DRAWITEM �ڵ鷯�� OnDrawItem()���� �׸��� �ȴ�.
	//��, virtual list���� checkbox�� �ùٸ��� ǥ�õ��� �ʴ� ������ �־����� check = !check�� m_list_db���� ó���ϰ�
	//OnDrawItem()���� üũ���ο� ���� ���� �׸����� �����Ͽ���.
	//��Ȥ OnDrawItem()���� ���� �׸��� ���� ������ ��ɵ��� ������� �ʰ� �⺻ CListCtrl�� ���� ���۽�Ű�� �ʹٸ�
	//Owner Data, Owner Draw Fixed�� ��� false�� �����ϰ� �ظ��� method�� ���������� ��� �����ϴ�.
	void			set_use_virtual_list(bool use_virtual = true) { m_use_virtual_list = use_virtual; }

	CSCColorTheme	m_theme = CSCColorTheme(this);

	void			modify_style();

	BOOL NCOverride;
	int Who;

	BOOL			m_bHorizLine = TRUE;
	void			HideScrollBars(int Type, int Which = SB_BOTH);

	enum CVtListCtrlExMsgs
	{
		message_progress_pos = 0,

		//OnLvnGetdispinfo(), OnLvnItemchanged() ��� ���� ���ø� ���ָ�
		//virtual list�϶��� OnLvnItemChanged()���� üũ�� ����ɶ��� �̺�Ʈ�� ó���� �� �������ѵ�
		//������ �� �ذ�å�� ã�� ���ߴ�.
		//�켱 üũ ���� �� ����� ���� �޽����� parent�� �˸���.
		message_checked_item,

		message_drag_and_drop,
		message_path_changed,
		message_rename_duplicated,
		message_request_rename,
		message_request_new_folder,
		message_request_new_folder_index,
		message_get_remote_free_space,
		message_get_remote_total_space,
		message_list_processing,		//C:\Windows, C:\Windows\WinSxS ��� ���� ������ �� ������ �����Ƿ� parent���� �̸� �˸���.
	};


//ShellList�� ���۽�Ű��
	bool		m_is_shell_listctrl = false;
	bool		m_is_local = true;
	bool		is_shell_listctrl() { return m_is_shell_listctrl; }
	bool		is_local() { return m_is_local; }
	//�� ó�� �� ����� �ָ� ��� ���ð� ������ ShellListCtrl�� ���۵ȴ�.
	//���� ���� ShellList�� �ƴ� ���·� ���۽�Ű�� ���� ������� �ʴ´�.
	//is_local�� true�̸� ���ϸ���� ���� ���ͼ� ǥ��������
	//false, �� remote�� ���� ���ϸ���� �޾Ƽ� ǥ���ؾ� �Ѵ�.
	void		set_as_shell_listctrl(CShellImageList* pShellImageList, bool is_local = true, CString default_path = _T("C:\\"));
	
	//list�� index�� �ָ� fullpath�� �����Ѵ�. -1�̸� ���� path�� �����Ѵ�.
	//���� ���õ� �׸��� �����̰� �ش� ���������� ��θ� ���Ѵٸ� get_selected_path()�� ����ؾ� �Ѵ�.
	CString		get_path(int index = -1);

	//���� ���õ� �׸��� �����̸� �ش� ��α����� fullpath��, �����̶�� ���� ����Ʈ�� ��θ� �����Ѵ�.('\' ����)
	CString		get_selected_path();

	//�ش� �ε����� ����/������ WIN32_FIND_DATA ���� �����Ѵ�.
	WIN32_FIND_DATA	get_win32_find_data(int index);
	//�ش� �ε����� ����/������ WIN32_FIND_DATA ���� �����Ѵ�.
	void		set_win32_find_data(int index, WIN32_FIND_DATA data);

	//path�� �޾� m_path�� �����ϰ� refresh_list()�� ȣ���Ѵ�.
	//local�� ���� ��θ� �ָ� �ڵ����� ������� ǥ��
	//m_path�� ������ �� set_path(m_path)�� ���� ���� ȣ������ ����. set_path()���ο��� �����Ϸ��� path�� m_path�� ������ ��ŵ�ǹǷ�.
	void		set_path(CString path, bool refresh = true);
	//remote�� ���� ���ϸ���� �޾Ƽ� ǥ���Ѵ�.
	void		set_filelist(std::deque<CVtFileInfo>* pFolderList, std::deque<CVtFileInfo>* pFileList);

	//reload�� true��� local�� ���ϸ���� �ٽ� �ҷ������� false��� ����� �ٽ� ����Ʈ�� ǥ���Ѵ�
	//(sort�� ��� ����/���� ����� sort�Ͽ� ������ �� ���ϸ���� reload���� �ʴ´�)
	//�ٸ� �ڵ忡 ���� �̹� m_cur_folders/m_cur_files �� ä�����ٸ� reload�� false�� ȣ���Ѵ�.
	void		refresh_list(bool reload = true);

	//m_cur_folders�� m_cur_files�� ä���� ������� ����Ʈ�� ��½�Ų��.
	void		display_filelist(CString cur_path);


	//����Ʈ�� ����/������ �и��Ǿ� ǥ��/���ĵǹǷ� ������ ������Ѿ� �Ѵ�.
	//�߰�/�����ÿ��� �� ��Ͽ� �߰�/�������Ѿ� �ϴ� �������� �ִ�.
	std::deque<CVtFileInfo> m_cur_folders;
	std::deque<CVtFileInfo> m_cur_files;
	//remote�� ���� fullpath�� �ش� ������ WIN32_FIND_DATA���� ���� �� ��찡 �ִ�.
	void		get_remote_file_info(CString fullpath, WIN32_FIND_DATA* data);

	//���� �Ǵ� ������ �ش��ϴ� ��� ����Ʈ�� �߰��Ѵ�.
	//local�� ��� ũ��� ��¥���� ����ִٸ� �ڵ� ä���ְ� remote��� ��������� �ȵȴ�.
	//void		add_file(CString path, uint64_t size, CString date = _T(""), bool is_remote = false, bool is_folder = false);
	//local�� �ƴ� remote�� ��� �Ѱܹ��� ����ü������ ����� �߰��Ѵ�.
	void		add_file(WIN32_FIND_DATA* data, bool is_remote);

	//���� ������ ����Ʈ�� �߰��Ѵ�. �ϳ��� ���� ������ ������ �ٷ� ����Ʈ�� �����ϱ� ���� ���.
	//list���� �߰��� �� m_cur_folders, m_cur_files���� �߰����� �ʴ´�.
	//void		add_file(WIN32_FIND_DATA* data, bool is_remote);

	enum SHELL_LIST_COLUMN
	{
		col_filename = 0,
		col_filesize,
		col_filedate,
	};

	CString		m_path;
	CShellImageList*	m_pShellImageList = NULL;
	CShellImageList*	get_shell_imagelist() { return m_pShellImageList; }
	void				set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

	void				move_parent_folder();

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

	//column = -1�̸� ��ü ����� �ؽ�Ʈ�� '|'�� �����ڷ� �Ͽ� �����Ѵ�.
	CString		get_header_text(int column = -1);
	void		set_header_text(int column, CString text);
	//default = HDF_LEFT
	void		set_header_text_align(int header, int format = HDF_LEFT);
	int			get_header_text_align(int header);

	//WM_SIZE�ÿ� Ư�� �÷��� �� �ʺ� �������� ó���Ѵ�.
	void		set_fixed_width_column(int column) { m_fixed_width_column = column; }

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
		column_data_type_combo,
	};
	int			get_column_data_type(int column);
	void		set_column_data_type(int column, int nType = column_data_type_text, bool invalidate = false);

//�߰� ����
	//index ��ġ�� 0�� �÷��� text�� ������ �߰��Ѵ�.(-1�̸� �� �������� �߰�)
	int			add_item(CString text = _T(""), int image_index = -1, bool ensureVisible = true, bool invalidate = true);
	int			add_item(std::deque<CString> dqText, int image_index = -1, bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, CString text = _T(""), int img_idx = -1, bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, std::deque<CString> dqText, int img_idx = -1, bool ensureVisible = true, bool invalidate = true);
	int			insert_item(int index, WIN32_FIND_DATA data, bool ensureVisible = true, bool invalidate = true);
	int			update_item(int index, WIN32_FIND_DATA data, bool ensureVisible = true, bool invalidate = true);
	//default separator = _T("|")
	int			insert_line(int index, CString line_string, CString separator = _T("|"), int img_idx = -1, bool ensureVisible = true, bool invalidate = true);

	//���� ������ ������ �߰��Ѵ�.
	int			insert_folder(int index, CString new_folder_name, bool is_remote);

	//���� ������ �� ������ �����ϰ� �������� ǥ���Ѵ�. ������ �������� �����Ѵ�.
	//"�� ����" or "New Folder" �� �ٱ������ ����Ͽ� Ÿ��Ʋ�� �޴´�.
	CString		new_folder(CString &new_folder_title);

	//���� �������� "�� ����" ���� �� �ε����� ���Ѵ�. ex. "�� ���� (2)"
	int			get_file_index(CString path, CString new_folder_title);

	//�������� ���ڸ� args�� �ִ� ��� ��ϵ��� �ʴ� ������ �ִ�. Ȯ�� �ʿ�!!
	template <typename ... Types> int insert_item(int index, int img_idx, Types... args)
	{
		int n = sizeof...(args);
		CString arg[] = { args... };

		int subItem = 0;
		if (index < 0)
			index = size();

		for (auto item : arg)
		{
			if (subItem == 0)
				index = insert_item(index, item, img_idx, false, false);
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

	//shell_listctrl�̶�� ���� ���� ������ �� �Ķ���͸� �� �� �ְ� ���� ������ ���� �Ŀ� ����Ʈ������ �����ؾ� �Ѵ�.
	bool		delete_item(int index, bool delete_physical_file = false);
	bool		delete_item(CString label);

	//���� ��ü�� �����̸� �ش� ������ �����Ѵ�.
	void		delete_empty_lines();
	void		delete_all_items(bool delete_file_list = true);
	void		DeleteAllItems() { delete_all_items(); }

//�ؽ�Ʈ ����
	CString		get_text(int item, int subItem);
	void		set_text(int item, int subItem, CString text, bool invalidate = true);

	//�� ������ �� �÷����� separator�� ���еǾ� �ִ� ���� ���� �����ͷ� index��ġ���� ����Ʈ�� ä���.
	//reset = true�̸� ��� �����͸� ������ �� ���� ä���. �� �� index�� 0���� ���µȴ�.
	void		set_text(int index, CString text, CString separator = _T("|"), bool reset = true);

	//from ~ to �÷������� �ؽ�Ʈ�� �ϳ��� ���ļ� �������ش�.
	CString		get_line_text(int index, int from = 0, int to = -1, CString sep = _T("|"));
	CString		get_line_text(int index, std::deque<int>* dqColumn = NULL, CString sep = _T("|"));
	std::deque<CString> get_line_text_list(int index, int from = 0, int to = -1);
	std::deque<CString> get_line_text_list(int index, std::deque<int>* dqColumn = NULL);
	//txt ��� �� �÷� ���� ������ �°� ����ϵ��� ������ �����Ͽ� ����
	//CString�� �ִ� 64K������ ���� �� �����Ƿ� vector�� �̿���.
	void		get_line_text_list(std::vector<CString> *vt);

	//����Ʈ�� ǥ���� �׸��� ���� ��� ǥ���� �ؽ�Ʈ ����
	void		set_text_on_empty(CString text, int font_size = 9, Gdiplus::Color cr = Gdiplus::Color::DimGray);

	//shell_listctrl�� �� ������ Ž���⿡�� ����/������ ���̺��� �����ϴ� �̺�Ʈ�� �߻��ϸ�
	//main���� �� �Լ��� ȣ���Ͽ� ���̺��� �����Ѵ�.
	//shell_listctrl�� �ƴϾ ��� �����ϳ� ���̺�� ���ϹǷ� ������ �׸��� ���� ���� ���� ó������ �����Ƿ�
	//������� �� ��!
	void		rename(CString old_text, CString new_text);

//���� ����
	//start���� �� ó�� ���õ� �׸��� �ε����� �����Ѵ�.
	int			get_selected_index(int start = 0);
	//�� ���������� ���õ� �� �׸��� �ε����� �����Ѵ�.
	int			get_last_selected_item();
	int			get_selected_count() { return GetSelectedCount(); }
	//���õ� �׸���� dq�� ��´�. dqSelected�� null�̸� �׳� ���� ������ ���Ϲ޾� ����Ѵ�.
	int			get_selected_items(std::deque<int> *dq);
	//���õ� �׸���� ����� dq�� ��´�. shelllist�� ��� is_fullpath = true�̸� �� �׸��� ��ü��θ� ��´�.
	int			get_selected_items(std::deque<CString>* dq, bool is_fullpath = false);
	//���õ� �׸���� ����� dq�� ��´�. shelllist������ ���Ǹ� cFileName�� �̹� ��ü��θ� ������ �ִ�.
	int			get_selected_items(std::deque<WIN32_FIND_DATA>* dq);

	//index = -1 : ��ü����
	void		select_item(int index, bool select = true, bool after_unselect = false, bool insure_visible = true);
	void		unselect_selected_item();

	int			get_check(int index);
	//CListCtrl::GetCheck() override.
	int			GetCheck(int index) { return get_check(index); }

	//index = -1 : ��ü����
	void		set_check(int index, bool check = true);
	void		set_check(std::deque<int> indices, bool check);
	//CListCtrl::SetCheck() override.
	void		SetCheck(int index, BOOL bCheck) { set_check(index, bCheck); }


	//check�� �׸� ����Ʈ�� dq�� ��� üũ�� �׸��� ������ �����Ѵ�.
	int			get_checked_items(std::deque<int>* dq = NULL);

	//�������� ���°��� Ư�� ���°��� �׸� �Ǵ� �� ���� ���ϱ�
	//LVIS_DROPHILITED or LVIS_SELECTED �׸��� ���� �� �ִ�.
	//drag ���߿� ���콺�� �ٸ� �� �������� ������ WM_LBUTTONUP �� ��� drophilited ���·� �������� ���� ������ �����ϱ� ����.
	int			get_items_by_state(UINT state, std::deque<int>* dq = NULL);
	//dq ����� �����۵��� state ����. dq�� null�̸� ��� �׸� ���� ����
	//���� : set_items_with_state(LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED, dq);
	//���� : set_items_with_state(0, LVIS_SELECTED|LVIS_FOCUSED, dq);
	int			set_items_with_state(UINT state, UINT mask, std::deque<int>* dq = NULL);

//�˻� ����
	//�⺻ �˻��Լ��� FindItem()�� �̿��ؼ� 0�� �÷������� �����͸� ã�´�. virtual list�̹Ƿ� OnLvnOdfinditem() �Լ� ���� �ʼ�.
	int			find(CString str, int start = -1, bool whole_word_olny = false, bool case_sensitive = false);
	//separator�� ""�� �ƴ� ���� �Ǵ� �ٸ� ���ڿ��� ����
	//str�� �и��ؼ� ��� ã�´�.
	int			find(CString find_target, std::deque<int>* result,
					 int start_idx = 0, int end_idx = -1,
					 std::deque<int>* dqColumn = NULL, bool stop_first_found = false, bool whole_word_olny = false, bool case_sensitive = false);
#if 1
	//target_columns�� �˻� ��� �÷����� �������ش�.
	template <typename ... T> int find(CString find_target, std::deque<int>* result, int start_idx = 0, int end_idx = -1,
					bool stop_first_found = false, bool whole_word_olny = false, bool case_sensitive = false,
					T... target_columns)
	{
		int n = sizeof...(target_columns);
		int arg[] = { target_columns... };

		std::deque<int> dqColumn;
		for (auto column : arg)
			dqColumn.push_back(column);

		return find(find_target, result, start_idx, end_idx, &dqColumn, stop_first_found, whole_word_olny, case_sensitive);
	}
#endif

//���� ����
	enum listctrlex_sort_type
	{
		sort_none = -1,
		sort_descending,
		sort_ascending,
	};
	void			allow_sort(bool allow);
	void			sort(int column, int ascending);
	void			sort_by_text_color(int column, int ascending, bool text_sort_on_same_color = true);
	int				cur_sorted_column_index() { return m_cur_sort_column; }

//�÷� ����
	void			set_color_theme(int theme);
	Gdiplus::Color	get_text_color(int item, int subItem);
	Gdiplus::Color	get_back_color(int item, int subItem);
	//Ư�� ���� �ƴ� �⺻ ������ ����.
	Gdiplus::Color	get_back_color() { return m_theme.cr_back; }
	Gdiplus::Color	get_back_alternate_color(int item, int subItem) { return m_theme.cr_back_alternate; }
	void			set_back_alternate_color(bool use = true, Gdiplus::Color cr = Gdiplus::Color::Transparent);

	//Ư�� �׸��� ���ڻ� ����. erase�� true�̸� crText ���ڸ� �����ϰ� �⺻ ���ڻ����� �ǵ�����.
	//item�� -1�̸� ��� ���ο�, subItem�� -1�̸� ��� �÷��� ����.
	//set_default_text_color(), set_default_back_color()�� ���� �⺻ ���ڻ��� ������ ������ �� �ִ�.
	void		set_text_color(int item, int subItem, Gdiplus::Color crText, bool erase = false, bool invalidate = true);
	//Ư�� �׸��� ���� ����. erase�� true�̸� crText ���ڸ� �����ϰ� �⺻ ���ڻ����� �ǵ�����.
	void		set_back_color(int item, int subItem, Gdiplus::Color crBack, bool erase = false);
	//���ڻ��� ���� ���� ����
	void		set_item_color(int item, int subItem, Gdiplus::Color crText, Gdiplus::Color crBack);

	//�⺻ ���ڻ��� �����Ѵ�.
	void		set_default_text_color(Gdiplus::Color cr_text) { m_theme.cr_text = cr_text; Invalidate(); }
	//�⺻ ������ �����Ѵ�.
	void		set_default_back_color(Gdiplus::Color cr_back) { m_theme.cr_back = cr_back; Invalidate(); }

	//�÷��� percentage�� ǥ���ϴ� �÷��� ��� �� ǥ�� ������ �����Ѵ�.
	void		set_progress_color(Gdiplus::Color crProgress);
	template <typename ... Types> void set_percentage_gradient(Types... args)
	{
		int n = sizeof...(args);
		Gdiplus::Color arg[] = { args... };

		m_cr_percentage_bar.clear();

		for (auto item : arg)
		{
			m_cr_percentage_bar.push_back(item);
		}
	}


//���� ����
	//���ҽ��� ListCtrl �Ӽ����� "���̺� ����" �Ӽ��� default���� false�� �ϰ� �� �Լ��� ���� ���� ���θ� �����ؾ� �Ѵ�.
	void		allow_edit(bool allow_edit = true, bool one_click_edit = true);
	void		allow_one_click_edit(bool allow_one_click_edit) { m_allow_one_click_edit = allow_one_click_edit; }
	void		allow_edit_column(int column, bool allow_edit = true);
	void		set_edit_readonly(bool readonly = true) { m_edit_readonly = readonly; }
	bool		is_in_editing()	{ return m_in_editing; }	//����������
	void		set_flag_in_editing(bool in_editing) { edit_end(); m_in_editing = in_editing; }
	int			get_recent_edit_item() { return m_edit_item; }
	int			get_recent_edit_subitem() { return m_edit_subItem; }
	void		set_recent_edit_subitem(int subItem) { m_edit_subItem = subItem; }
	std::deque<int>* get_selected_list_for_edit() { return &m_dqSelected_list_for_edit; }
	std::deque<int> m_dqSelected_list_for_edit;
	CString		get_edit_old_text() { return m_edit_old_text; }
	CString		get_edit_new_text() { return m_edit_new_text; }
	//item = -1�̸� ���õ� �׸���, subItem = -1�̸� 0�� �÷��� ���� �����Ѵ�.
	//return�Ǵ� CEdit*�� �̿��� ��쵵 �ʿ��Ͽ� ������.
	CEdit*		edit_item(int item = -1, int subItem = -1);
	void		edit_end(bool valid = true);
	void		undo_edit_label();		//���� ���� �ؽ�Ʈ�� �ǵ�����.(���� ��� ���� ���̺��� ���ϸ��̰� ���ϸ� ������ ������ ��� �� �� �ִ�.)
	bool		is_modified() { return m_modified; }
	void		reset_modified_flag() { m_modified = false; }
	LRESULT		on_message_CSCEdit(WPARAM wParam, LPARAM lParam);

//��Ʈ ����. �ݵ�� set_headings() �Ŀ� ȣ���Ұ�.
	void		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	//-1 : reduce, +1 : enlarge
	int			get_font_size();
	//�ݵ�� set_headings() �Ŀ� ȣ���Ұ�.
	void		set_font_size(int font_size);
	void		enlarge_font_size(bool enlarge);
	void		set_font_bold(int weight = FW_BOLD);
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
	int			get_header_height();
	//header_height�� ���Ŭ�������� HDM_LAYOUT �޽����� �߻��ؾ� ����Ǵµ�
	//�׷��� �Ǳ� ���ؼ��� SetFont / MoveWindow / SetWindowPos ���� �ʿ��ϹǷ�
	//VtListCtrlEx���� set_font_name() ȣ��ÿ� ����� SetFont�� �����Ͽ� �ذ���.
	//=> SetFont()�� ���ؼ� header�� height�� �����Ѵ�.
	void		set_header_height(int height);

	LRESULT		on_message_CHeaderCtrlEx(WPARAM wParam, LPARAM lParam);

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

	int			get_line_height() { return m_line_height; }
	void		set_line_height(int height);

	void		set_column_width(int nCol, int cx);
	void		load_column_width(CWinApp* pApp, CString sSection);
	void		save_column_width(CWinApp* pApp, CString sSection);
	CRect		get_item_rect(int item, int subItem);
	//Ŭ����ġ�� ���� item�� �ùٸ��� �Ǻ��ǳ� subItem�� �׷��� �ʾƼ�(���콺 �̺�Ʈ �ڵ鷯 �Լ�����) ���� �߰���.
	//return value : checkbox Ŭ�� �� LVHT_ONITEMSTATEICON, �̹��� Ŭ�� �� LVHT_ONITEMICON, �� �ܿ��� LVHT_ONITEMLABEL�� �����Ѵ�.
	int			hit_test(CPoint pt, int& item, int& subItem, bool include_icon);

	void		show_progress_text(bool show = true) { m_show_progress_text = show; Invalidate(); }
	//void		set_progress_text_color(Gdiplus::Color cr) { m_theme.cr_progress_text = cr; }

	//cr_border�� Transparent��� cr_border�� ���� �������� �ʴ´ٴ� �ǹ̴�. �⺻ m_theme.cr_selected_border ������ ����Ѵ�.
	//��, set_use_distinct_border_color(true);�� �����Ǹ� ������ cr_border�� �����ϰ� �ش� �������� ������ ���еǴ� ������ �׸���.(black or white)
	//selected_border_width <= 0�̸� m_selected_border_width�� �������� �ʴ´ٴ� �ǹ̴�. �⺻ m_selected_border_width ���� ���ȴ�.
	void		set_draw_selected_border(bool draw, Gdiplus::Color cr_border = Gdiplus::Color::Transparent, int selected_border_width = 0, int pen_style = -1);
	//border color�� Ư������ �ƴ� ������ ���еǴ� ������ �׸���.
	void		set_use_distinct_border_color(bool use_distinct = true) { m_use_distinct_border_color = use_distinct; }

	//�� ���ο� ���� ��� ���� ǥ�� ���� ����. m_draw_top_line
	void		draw_top_line(bool draw, Gdiplus::Color cr = Gdiplus::Color::LightGray) { m_draw_top_line = draw; m_cr_top_line = cr; }
	//�� ���ο� ���� �ϴ� ���� ǥ�� ���� ����. m_draw_top_line
	void		draw_bottom_line(bool draw, Gdiplus::Color cr = Gdiplus::Color::LightGray) { m_draw_bottom_line = draw; m_cr_bottom_line = cr; }

//scroll
	enum CLISTCTRLEX_ENSURE_VISIBLE_MODE
	{
		visible_first = 0,
		visible_center,
		visible_last,
	};
	//� �׸��� Ư�� ��ġ�� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode�� visible_first(0)�̰� offset�� 3�̸� ������ 3+1�� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode�� visible_center(1)�̰� offset�� 0�̸� �߾� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode�� visible_last(2)�̰� offset�� 3�̸� �Ʒ����� -3-1�� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
	//mode default = visible_center
	void			ensure_visible(int index, int mode = visible_center, int offset = 0);
	//original IsItemVisible method does not check partial. only check vertical axis.
	bool			is_item_visible(int index, bool bPartial = false);

	//�׸��� �߰��Ǹ� auto scroll������ Ư�� �׸��� �����ϸ� false�� �ȴ�.
	//scroll_end_button�� m_auto_scroll = false �϶��� ȭ�鿡 ǥ�õȴ�.
	bool			m_auto_scroll = true;
	bool			m_show_auto_scroll_button = false;
	void			show_auto_scroll_button(bool show = true);
	bool			get_auto_scroll() { return m_auto_scroll; }
	//0:auto scroll off, 1:auto scroll on, -1:toggle auto scroll
	void			set_auto_scroll(int auto_scroll);
	LRESULT			on_message_CGdiButton(WPARAM wParam, LPARAM lParam);

	//����Ʈ �׸���� �������� ���´�.
	void			shuffle();

	//x, y��ġ�� item, sub_item �ε����� ���� �� �ִ�. item�� �ε������� �ʿ�� �� ���� ���ϰ��� �̿��ϸ� �ȴ�.
	int				index_from_point(int x, int y, int* item = NULL, int* sub_item = NULL);

	//Drag&Drop �巡�� ����
	template <typename ... Types> void add_drag_images(Types... args) //(�������Ͽ� �̹���, �̱����Ͽ� �̹����� ���ʴ�� �ְ� drag�Ǵ� ������ ���� �´� �̹����� ����Ѵ�)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
			m_drag_images_id.push_back(id);
	}

	bool			get_use_drag_and_drop() { return m_use_drag_and_drop; }
	void			set_use_drag_and_drop(bool use_drag = true) { m_use_drag_and_drop = use_drag; }
	int				get_drop_index() { return m_nDropIndex; }

	//text = "     some text"�� ��� ���� ������ �鿩���� �뵵�� ����Ѵ�.
	//"     " ��ŭ �鿩�Ἥ "some text"�� ����Ѵ�. �����ܵ� �Բ� ����ȴ�.
	void			use_indent_from_prefix_space(bool use_indent) { m_use_indent_from_prefix_space = use_indent; }

	//HAS_STRING, OWNER_DRAW_FIXED �Ӽ��� ������ Get/SetItemData() �Լ��� ����� �� ����.
	//�� �� �Լ��� ����� �� �ֵ��� CListCtrlData�� data ����� �߰��ϰ� ���� �Լ����� override�Ͽ� ������.
	DWORD_PTR		GetItemData(int index);
	BOOL			SetItemData(int index, DWORD_PTR dwData);

	CHeaderCtrlEx*	get_header_ctrl() { return &m_HeaderCtrlEx; }

protected:
	bool			m_use_virtual_list = true;

//���� ������
	std::deque<CListCtrlData> m_list_db;
	
	CString			m_text_on_empty;
	int				m_text_on_empty_size = 9;
	Gdiplus::Color	m_text_on_empty_color = Gdiplus::Color::DimGray;

//�÷� ����
	CHeaderCtrlEx	m_HeaderCtrlEx;
	std::deque<int> m_column_data_type;
	std::deque<int> m_column_text_align;
	int				m_fixed_width_column = -1;
	int				m_line_height = 16;

//���� ����
	bool			m_allow_sort = true;
	std::deque<int> m_column_sort_type;				//asceding or descending
	int				m_cur_sort_column = 0;			//���ĵ� �÷� �ε���(���� ������ ����)

//�÷� ����
	bool			m_use_alternate_back_color = false;

//������� ����
	bool			m_allow_edit = false;			//�׸� ������ ��������...
	bool			m_allow_one_click_edit = false;	//�׸��� Ŭ���� �� ������ �ð����� �ٽ� Ŭ���� �� �������� ����
	std::deque<bool> m_allow_edit_column;			//�� �÷����� �������� ���θ� ������ �� �ִ�. ��, m_allow_edit�� ����Ǹ� ��� �� ������ ����ʿ� ����.
	bool			m_modified = false;				//�׸��� ���� �Ǵ� ������ ������ ��� true
	bool			m_in_editing = false;			//����������
	int				m_edit_item = -1;				//�������� ������ �ε���
	int				m_edit_subItem = -1;			//�������� ������ �����ε���
	bool			m_edit_readonly = false;		//�������δ� ������ �����ؼ��� �ȵǴ� �������� ��� set_edit_readonly(true);�� ȣ���Ѵ�.
	CString			m_edit_old_text;				//���� �� �ؽ�Ʈ
	CString			m_edit_new_text;				//���� �� �ؽ�Ʈ
	CSCEdit*		m_pEdit = NULL;
	long			m_last_clicked_time = 0;		//one_click���� ������� ���� �� ������ Ŭ�� �ð�
	int				m_last_clicked_index = -1;		//one_click���� ������� ���� �� ������ Ŭ�� �ε���

//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	bool			m_show_progress_text = true;	//default = true

	//���콺�� ��Ʈ�� �ȿ� ���� ��� true
	bool			m_is_hovering = false;

//Drag&Drop �巡�� ����
	bool			m_use_drag_and_drop = false;//default = false
	CWnd*			m_pDragWnd = NULL;			//Which ListCtrl we are dragging FROM
	CWnd*			m_pDropWnd = NULL;			//Which ListCtrl we are dropping ON
	CImageList*		m_pDragImage = NULL;		//For creating and managing the drag-image
	bool			m_bDragging = false;		//T during a drag operation
	int				m_nDragIndex = -1;			//drag�Ǵ� ��Ʈ���� CListCtrl�� �� �� �ε���(drag�� ������ ��Ʈ���� ������� �����, ��ӵ� Ŭ�������� ������� ����)
	int				m_nDropIndex = -1;			//drop�� ��Ʈ���� CListCtrl�� �� �� �ε���(drag�� ������ ��Ʈ���� ������� �����, ��ӵ� Ŭ�������� ������� ����)
	std::deque<UINT> m_drag_images_id;			//drag�� �� ����ϴ� �̹������� resource id ����(�������Ͽ� �̹���, �̱����Ͽ� �̹����� ���ʴ�� �ְ� drag�Ǵ� ������ ���� �´� �̹����� ����Ѵ�)
	void			DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd);

	//https://jiniya.net/tt/594/
	//�� �Լ��� �巡�� �̹����� ���� �������ִ� �ڵ����� ������� ���� �ڵ��̹Ƿ� ���� �Ұ�.
	CImageList* create_drag_image(CListCtrl* pList, LPPOINT lpPoint);

	bool			m_use_indent_from_prefix_space = false;

	bool			m_draw_selected_border = true;
	bool			m_use_distinct_border_color = false;
	int				m_selected_border_width = 1;
	int				m_selected_border_style = Gdiplus::DashStyleSolid;

	bool			m_has_focus = false;

	bool			m_draw_top_line = false;
	Gdiplus::Color	m_cr_top_line = Gdiplus::Color::LightGray;

	bool			m_draw_bottom_line = false;
	Gdiplus::Color	m_cr_bottom_line = Gdiplus::Color::LightGray;

//��ũ�� ��ư
	CGdiButton*		m_button_scroll_to_end = NULL;
	int				m_auto_scroll_button_size = 24;
	void			init_auto_scroll_button();

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
	afx_msg BOOL OnNMClick(NMHDR* pNMHDR, LRESULT* pResult);
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
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnLvnItemchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLvnBeginScroll(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnEndScroll(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};


