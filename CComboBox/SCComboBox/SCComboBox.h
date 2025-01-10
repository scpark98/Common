#pragma once

/*
- ���ҽ� �Ӽ����� Has String = true, Owner Draw = Fixed�� ����.
*/

#include <afxwin.h>

// CColorComboBox

class CSCComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CSCComboBox)

public:
	CSCComboBox();
	virtual ~CSCComboBox();

	//���� �Էµ� �ؽ�Ʈ�� �о���� �׸� �������� ������ �߰���Ų��. ������Ʈ������ �����Ѵ�.
	int				add(CString text = _T(""));

	COLORREF		m_crText;
	COLORREF		m_crBack;
	COLORREF		m_crHighlightText;
	COLORREF		m_crHighlightTextBack;

	void			SetTextColor( COLORREF crText ) { m_crText = crText; RedrawWindow(); }
	void			SetBackColor( COLORREF crBack ) { m_crBack = crBack; RedrawWindow(); }
	void			SetHighlightTextColor( COLORREF cr ) { m_crHighlightText = cr; RedrawWindow(); }
	void			SetHighlightTextBackColor( COLORREF cr ) { m_crHighlightTextBack = cr; RedrawWindow(); }

	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	//-1 : reduce, +1 : enlarge
	void			set_font_size(int font_size);
	//void	enlarge_font_size(bool enlarge);
	//void	set_font_bold(bool bold = true);
	//void	set_font_italic(bool italic = true);
	//LOGFONT	get_log_font() { return m_lf; }
	//void	set_log_font(LOGFONT lf);

	void			load_history(CWinApp* app, CString section);
	void			save_history(CWinApp* app, CString section);

	//src���� �����ϴ� �޺��ڽ� �������� �ε����� ����.
	int				find_string(CString src);

//���� ����
	void			edit_end(bool valid);
	LRESULT			on_message_CSCEdit(WPARAM wParam, LPARAM lParam);


protected:

//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

//���� ����
	bool		m_use_edit = true;		//���� �׸� �̿��� ���� Ŭ���� ���� ��������� ����� ������
	CEdit*		m_pEdit = NULL;
	CString		m_old_text;				//�����Ǳ� ���� ���� �ؽ�Ʈ
	CRect		m_edit_margin;			//edit box ���� ����(���η� ��� ���ĵǰ� ǥ���ϱ� ����)
	void		repos_edit();			//resize�� �ϸ� ������ ���µǹǷ� ��ġ�� ������ �ٽ� ���

//���ã�� ����
	CString			m_reg_section;		//load or save�� �� �Ѿ�� section���� ����س��´�.

	DECLARE_MESSAGE_MAP()
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT /*lpCompareItemStruct*/);
	//afx_msg void OnNcPaint();
	//afx_msg void OnPaint();
	//afx_msg void OnEditUpdate();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCbnDropdown();
	virtual void PreSubclassWindow();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnCbnSetfocus();
	afx_msg void OnCbnKillfocus();
	afx_msg void OnCbnSelchange();
	afx_msg void OnCbnSelendok();
	afx_msg void OnCbnSelendcancel();
	afx_msg HBRUSH CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


