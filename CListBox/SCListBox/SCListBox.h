#if !defined(AFX_SCLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
#define AFX_SCLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_


/*
* [���� ���]
* �α� ���� ����ϴ� �뵵�� ���� CRichEditCtrlEx�� ���������
* �� �������� SetSel�ϰ� �̸� ReplaceSel�� �Ͽ� ����ϴ� ����̴ٺ���
* �αװ� ��� �߰��Ǵ� ���¿����� ��ũ�� �� �ؽ�Ʈ ���� ���� �ſ� �����ߴ�.
* �׷��� CStatic�� ��ӹ��� COutputStatic�� �����غ�����
  CStatic�� Ű���� �Է��� ó������ �ʰ� ��ũ�� ���� ó���� ���ŷӴ�.
  CListBox�� ��ӹ޾� �� �������� ���ֱ� ���� ������.

* DWORD�� �÷����� SetItemData()�� �����Ͽ� �÷� �ؽ�Ʈ�� ���.
* log ��µ CRichEditCtrlEx�� ����ϱ� ���� ����.
* (���� �׸��� Ŭ���ϸ� auto_scroll�� off�ǰ� Ctrl+End � ���� auto_scroll�� on��.
* CPathCtrl���� ���� �˾� ����Ʈâ���� ǥ���ϱ� ���ؼ��� ����.
* 
* [warning]
* - ��Ʈ���� �ʺ�, ���̰� ��� �̻�Ǹ� ǥ�� �ӵ��� ������ ��������.
* 
* [setting]
* - �ʼ� : Has String = true, Owner Draw = Fixed, No Integral Height = false
* - ���� : Multiline, Sort
* 
* [������ ����]
* - text color�� SetItemData()�� �̿��Ͽ� �����ϰ� ������ image index ���� �����Ϸ���
*   struct�� ����ؾ� �Ѵ�.
*   ��, �� Ŭ������ ����ϴ� ��� ������Ʈ���� �̷��� �� ��� ���ʿ��� ���ҽ��� �Ҵ��ؾ� �ϹǷ�
*   flagó���� �� �ʿ��� ��쿡�� ����ϵ��� �����Ǿ�� �Ѵ�.
*/



#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxwin.h>
#include <deque>
#include "../../Functions.h"
#include "../../system/ShellImageList/ShellImageList.h"
#include "../../GdiplusBitmap.h"


//ROOT_LABEL�� PathCtrl���� �ֻ����� ǥ���ϱ� ���� �뵵��.
//#define ROOT_LABEL _T("_r")

// SCListBox.h : header file

//-------------------------------------------------------------------
//
//	CSCListBox class - 
//		A CListBox-derived class with optional colored items.
//
//		Version: 1.0	01/10/1998 Copyright ?Patrice Godard
//
//		Version: 2.0	09/17/1999 Copyright ?Paul M. Meidinger
//
//-------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// CSCListBox window

static const UINT Message_CSCListBox = ::RegisterWindowMessage(_T("MessageString_CSCListBox"));

class CSCListBoxMessage
{
public:
	CSCListBoxMessage(CWnd* _this, int _message)
	{
		pThis = _this;
		message = _message;
	}

	CWnd*	pThis = NULL;
	int		message;
};

class CSCListBox : public CListBox
{
// Construction
public:
	CSCListBox();
	virtual ~CSCListBox();

	enum MESSAGES
	{
		message_sclistbox_selchanged = 0,
	};

// Attributes

// Operations
public:
	//�⺻ ���ڻ����� �� �� �߰�
	int			add(LPCTSTR lpszFormat, ...);

	//���� ���ڻ����� �� �� �߰�
	int			add(COLORREF cr, LPCTSTR lpszFormat, ...);

	//������ ������ �����Ϳ� �ؽ�Ʈ append
	int			append(LPCTSTR lpszFormat, ...);

	//�������� ���ڿ��� �ѹ��� �߰�(shell_listbox�� ���)
	int			add(std::deque<CString>* lists, COLORREF cr = -1);

	int			insert_string(int nIndex, CString lpszItem);				// Inserts a string to the list box
	int			insert_string(int nIndex, CString lpszItem, COLORREF rgb = -1);	// Inserts a colored string to the list box
	void		set_item_color(int nIndex, COLORREF rgb, bool invalidate = true);	// Sets the color of an item in the list box
	COLORREF	get_item_color(int nIndex);

	CString		get_text(int index);
	void		set_text(int index, CString text, COLORREF cr = -1);

	void		clear_all() { ResetContent(); Invalidate(); }

	CSize		resizeToFit(bool bHori = true, bool bVert = true);			//����� ũ�⸦ �����Ѵ�.
	void		set_minimum_lines(int lines) { m_nMinimumLines = lines; }

	//���� ����
	//���õ� �׸� ����Ʈ �� ���õ� ������ ����
	int			get_selected_items(std::deque<int>* selected = NULL);

	//�˾� �޴�
	enum CONTEXT_MENU
	{
		menu_selected_count = WM_USER + 1234,
		menu_show_log,
		menu_show_date,
		menu_show_time,
		menu_auto_scroll,
		menu_clear_all,
		menu_copy_selected_to_clipboard,
		menu_copy_all_to_clipboard,
		menu_save_selected,				//���� �α׸� ���Ϸ� ����
		menu_save_all,					//��ü �α� ���Ϸ� ����
	};

	bool		m_use_popup_menu = true;
	void		use_popup_menu(bool use) { m_use_popup_menu = use; }
	void		OnPopupMenu(UINT nID);

	//1:show, 0:hide, -1:no change
	void		show_date_time(int date = -1, int time = -1);

	//���� ����
	int			get_line_height() { return m_line_height; }
	void		set_line_height(int _line_height);

	void		use_over(bool use = true) { m_use_over = use; }
	int			get_over_item() { return (m_use_over ? m_over_item : -1); }

	//����
	void		set_text_color(COLORREF cr) { m_cr_text = cr; Invalidate(); }
	void		set_back_color(COLORREF cr) { m_cr_back = cr; Invalidate(); }

	int			GetGutterCharNumber() { return m_nGutterCharNumber; }
	void		SetGutterCharNumber(int chars) { m_nGutterCharNumber = chars; }

	//folder list�� ���۽�Ų��.
	void		set_as_folder_list();
	void		set_parent(HWND hWnd) { m_hParentWnd = hWnd; }
	//root��� ������ ���� �������� ����Ʈ�� ǥ���Ѵ�.
	int			set_path(CString root, CString selected_text = _T(""));
	//lists�� NULL�Ͽ� ȣ���ϸ� ��������� m_folder_list�� ������ ǥ���Ѵ�.
	int			set_folder_list(std::deque<CString>* lists = NULL, CString selected_text = _T(""));


	virtual		CSCListBox&	set_font(LOGFONT& lf);
	virtual		CSCListBox&	set_font_name(CString sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual		CSCListBox&	set_font_size(int nSize);
	virtual		CSCListBox&	set_font_bold(bool bBold = true);

	//color setting
	enum SCLISTBOX_COLOR_THEME
	{
		color_theme_default = 0,
		color_theme_explorer,
		color_theme_popup_folder_list,
		//color_theme_navy_blue,
		//color_theme_dark_blue,
		//color_theme_dark_gray,
	};

	void		set_color_theme(int theme, bool apply_now = true);

	CShellImageList* m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList) { m_pShellImageList = pShellImageList; }

	//���� �Ұ� ����Ʈ�� ����. disable�� �� �� ������� �����ȴ�� ǥ���Ѵ�.
	//�̷��� �ؾ� Ű����, ���콺 �̺�Ʈ���� ���� ������ �� �ִ�.
	void		set_as_static(bool as_static = true) { EnableWindow(FALSE); m_as_static = as_static; }

	//���� ����
	bool		get_use_edit() { return m_use_edit; }
	void		set_use_edit(bool use = true, bool readonly = false) { m_use_edit = use; m_edit_readonly = readonly; }
	//index == -1�̸� ���õ� ù��° �׸� ����
	void		edit(int index = -1);
	//modify�� true�̸� ������ �ؽ�Ʈ�� ����, �׷��� ������ ���� �ؽ�Ʈ ����.
	void		edit_end(bool modify = true);

	//use own imagelist
	//png �̹����� label�� �տ� ǥ���Ѵ�. 2�� �̻��� ��� altȿ���� �� �� �ִ�. id�� 0�̸� clear()�� �����Ѵ�.
	void		add_to_imagelist(UINT id);
	//png �̹����� label�� �տ� ǥ���Ѵ�. 2�� �̻��� ��� altȿ���� �� �� �ִ�.
	template <typename ... T> void set_imagelist(T... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//set_�̹Ƿ� ���� �׸���� ��� ������ �� �߰��ؾ� �Ѵ�.
		//�׳� �ϳ��� �߰��Ѵٸ� add_header_image()�� ����Ѵ�.
		for (int i = 0; i < m_imagelist.size(); i++)
		{
			CGdiplusBitmap* img = m_imagelist[i];
			delete img;
		}

		m_imagelist.clear();

		for (auto id : arg)
		{
			//���⼭ ���� new�� �Ҵ�ް� load�Ͽ� deque�� ������������
			//���⼭�� �� ��ó�� �������� ���� OnPaint()���� ���� deque�� ����־���.
			//template�̶� �׷��� ���⼭ �ٷδ� ���� �ʴ´�.
			//����Լ��� ���� �־�� �Ѵ�.
			add_to_imagelist(id);
		}
	}

protected:
	//���������� ��� GetParent�����ε� parent�� �������� �ʰ� OnNotify()�� �������� �ʾƼ� �������� �����ϱ� ����.
	HWND		m_hParentWnd = NULL;

	bool		m_as_static = false;		//true�� ��� Ű����, ���콺�� ���� ���� �Ұ�
	bool		m_use_over = false;			//hover hilighted
	int			m_over_item = -1;
	bool		m_as_popup = false;			//�˾����� �����ϴ� ����Ʈ�ڽ��� ���� killfocus�̸� ��������.
	bool		m_as_folder_list = false;	//��������� ǥ���ϴ� �������� �����ϴ� ���
	std::deque<CString> m_folder_list;

	bool		m_show_log = true;
	bool		m_show_date = false;
	bool		m_show_time = true;
	bool		m_dim_time_str = true;		//�ð� ���ڿ��� ���� ȸ������ �����ǵ��� ǥ��

	COLORREF	m_cr_text;					//�⺻ ���ڻ�
	COLORREF	m_cr_textSelected;			//���� �׸��� Ȱ��ȭ(active) ���ڻ�
	COLORREF	m_cr_textSelectedInactive;	//���� �׸��� ��Ȱ��ȭ(inactive) ���ڻ�
	COLORREF	m_cr_textOver;
	COLORREF	m_cr_back;					//�⺻ ����
	COLORREF	m_cr_backSelected;			//���� �׸� ����
	COLORREF	m_cr_backSelectedRect;		//���� �׸� �׵θ�(focus()�� ���� ��쿡��)
	COLORREF	m_cr_backSelectedInactive;
	COLORREF	m_cr_backOver;


	LOGFONT		m_lf;
	CFont		m_font;
	void		ReconstructFont();

	//���� ���̴� �۲� ���̿� ���� �ڵ� ���ȴ�.
	//���� �������� ���� ������ �����Ϸ��� set_line_height(32); �Լ��� �̿��ؾ� �Ѵ�.
	//font_size�� �����ߴµ� ���� ������ ������� �ʴ� �͵� ������ �Ǵ� �ʿ��� ���
	//�ش� �Լ��� ���� ��������.
	int			m_line_height;

	//���� ����
	bool		m_use_edit = false;
	bool		m_in_editing = false;
	bool		m_edit_readonly = false;
	int			m_edit_index = -1;
	CEdit*		m_pEdit = NULL;

	//���� ����
	//std::deque<int>	 m_selected;
	//int			m_last_selected = -1;	//������ ������ �׸�(���� ���ý� �ʿ�)

	//vert fit���� �ּ� ǥ�� ���μ��� ���س��´�.default = -1(������ ���� ���)
	int			m_nMinimumLines;



	//�׸� ���ʿ� �ε����� ǥ���ϴ� gutter ���� ����
	//������ �ȼ��� �ƴ� ���� ������. default = 0;
	int			m_nGutterCharNumber = 0;

	//��ũ�� ��� ����
	bool		m_auto_scroll = true;
	bool		m_is_thumb_tracking = false;
	int			m_max_horizontal_extent = 0;

	CString		get_all_text(bool selected_only = false);
	void		copy_selected_to_clipboard();
	void		copy_all_to_clipboard();
	void		save_selected_to_file();
	void		save_all_to_file();

	std::deque<CGdiplusBitmap*> m_imagelist;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCListBox)
	public:
	//afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM);
	//}}AFX_VIRTUAL


	// Generated message map functions
protected:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual void PreSubclassWindow();
	afx_msg void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	afx_msg void OnPaint();
	//afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnLbnSelchange();
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	//	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)