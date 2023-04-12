#if !defined(AFX_COLORLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
#define AFX_COLORLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxwin.h>
#include <deque>
#include "../../Functions.h"

//ROOT_LABEL�� PathCtrl���� �ֻ����� ǥ���ϱ� ���� �뵵��.
//#define ROOT_LABEL _T("_r")

// ColorListBox.h : header file

//-------------------------------------------------------------------
//
//	CColorListBox class - 
//		A CListBox-derived class with optional colored items.
//
//		Version: 1.0	01/10/1998 Copyright ?Patrice Godard
//
//		Version: 2.0	09/17/1999 Copyright ?Paul M. Meidinger
//
//-------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// CColorListBox window

class CColorListBox : public CListBox
{
// Construction
public:
	CColorListBox();

// Attributes
	enum USER_MESSAGE
	{
		wm_message_colorlistbox_selchange = WM_USER + 2724,
	};

// Operations
public:
	int			AddString(CString text, COLORREF crText = ::GetSysColor(COLOR_WINDOWTEXT), COLORREF crBack = ::GetSysColor(COLOR_WINDOW), bool invalidate = false);
	int			add_string(CString text, COLORREF crText = ::GetSysColor(COLOR_WINDOWTEXT), COLORREF crBack = ::GetSysColor(COLOR_WINDOW), bool invalidate = false);
	int			add_string(CString lpszItem, COLORREF rgb, bool invalidate = true);					// Adds a colored string to the list box
	int			add_string(std::deque<CString> *lists, bool invalidate = false);
	int			insert_string(int nIndex, CString lpszItem);				// Inserts a string to the list box
	int			insert_string(int nIndex, CString lpszItem, COLORREF rgb);	// Inserts a colored string to the list box
	void		set_item_color(int nIndex, COLORREF rgb, bool invalidate = true);	// Sets the color of an item in the list box
	COLORREF	get_item_color(int nIndex);

	CSize		resizeToFit(bool bHori = true, bool bVert = true);			//����� ũ�⸦ �����Ѵ�.
	void		set_minimum_lines(int lines) { m_nMinimumLines = lines; }
	int			get_line_height() { return m_line_height; }

	void		UseColor(bool bUse = true) { m_bUseColor = bUse; }
	void		UseHover(bool bUse = true) { m_bUseHover = bUse; }
	int			get_hover_item() { return (m_bUseHover ? m_nOverItem : -1); }

	void		set_back_color(COLORREF cr) { m_crBack = cr; Invalidate(); }

	int			GetGutterCharNumber() { return m_nGutterCharNumber; }
	void		SetGutterCharNumber(int chars) { m_nGutterCharNumber = chars; }

	//folder list�� ���۽�Ų��.
	void		set_parent(HWND hWnd) { m_hParentWnd = hWnd; }
	//root��� ������ ���� �������� ����Ʈ�� ǥ���Ѵ�.
	int			set_path(CString root, CString selected_text = _T(""));
	//lists�� NULL�Ͽ� ȣ���ϸ� ��������� m_folder_list�� ������ ǥ���Ѵ�.
	int			set_folder_list(std::deque<CString>* lists = NULL, CString selected_text = _T(""));
	CImageList	m_imagelist_small;

	virtual		CColorListBox&	set_font(LOGFONT& lf);
	virtual		CColorListBox&	set_font_name(CString sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual		CColorListBox&	set_font_size(int nSize);
	virtual		CColorListBox&	set_font_bold(bool bBold = true);

	//color setting
	enum listctrlex_color_theme
	{
		color_theme_default = 0,
		color_theme_explorer,
		//color_theme_light_blue,
		//color_theme_navy_blue,
		//color_theme_dark_blue,
		//color_theme_dark_gray,
	};

	void	set_color_theme(int theme, bool apply_now = true);

protected:
	//���������� ��� GetParent�����ε� parent�� �������� �ʰ� OnNotify()�� �������� �ʾƼ� �������� �����ϱ� ����.
	HWND		m_hParentWnd = NULL;

	bool		m_bUseColor;		//default = false;
	bool		m_bUseHover;		//default = false;
	BOOL		m_bOutside;
	int			m_nOverItem;
	bool		m_as_popup;			//�˾����� �����ϴ� ����Ʈ�ڽ��� ���� killfocus�̸� ��������.
	bool		m_as_folder_list;	//��������� ǥ���ϴ� �������� �����ϴ� ���
	std::deque<CString> m_folder_list;

	COLORREF	m_crText;					//�⺻ ���ڻ�
	COLORREF	m_crTextSelected;			//���� �׸��� Ȱ��ȭ(active) ���ڻ�
	COLORREF	m_crTextSelectedInactive;	//���� �׸��� ��Ȱ��ȭ(inactive) ���ڻ�
	COLORREF	m_crTextOver;
	COLORREF	m_crBack;					//�⺻ ����
	COLORREF	m_crBackSelected;
	COLORREF	m_crBackSelectedInactive;
	COLORREF	m_crBackOver;


	LOGFONT		m_lf;
	CFont		m_font;
	void		ReconstructFont();

	int			m_line_height;


	//vert fit���� �ּ� ǥ�� ���μ��� ���س��´�.default = -1(������ ���� ���)
	int			m_nMinimumLines;

	//
	//int			m_nItemHeight?


	//�׸� ���ʿ� �ε����� ǥ���ϴ� gutter ���� ����
	//������ �ȼ��� �ƴ� ���� ������. default = 0;
	int			m_nGutterCharNumber;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorListBox)
	public:
	//afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorListBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorListBox)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual void PreSubclassWindow();
	afx_msg void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	//afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnLbnSelchange();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORLISTBOX_H__5529A6B1_584A_11D2_A41A_006097BD277B__INCLUDED_)
