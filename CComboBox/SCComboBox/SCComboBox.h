#pragma once

/*
- ���ҽ� �Ӽ����� Owner Draw = Fixed, Has String = true�� ����.
*/

#include <afxwin.h>

#include "../../colors.h"

// CColorComboBox

class CSCComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CSCComboBox)

public:
	CSCComboBox();
	virtual ~CSCComboBox();

	CSCColorTheme	m_theme = CSCColorTheme(this);
	void			set_color_theme(int theme);

	CString			get_cur_text() { CString text; GetLBText(GetCurSel(), text); return text; }

	//���� �Էµ� �ؽ�Ʈ�� �о���� �׸� �������� ������ �߰���Ų��. ������Ʈ������ �����Ѵ�.
	//������ ������ �������� ������ �⺻ cr_text�� ����Ѵ�.
	int				add(CString text = _T(""), Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

//design
	//�������� �ȼ� ������ �ƴ� logical_unit�̴�. �� set_font_size()�� ������ ���Ͻ�Ų��.
	//�ȼ� ũ��� ���̸� �����Ѵٸ� set_line_height_px()�� ȣ���Ѵ�.
	void			set_line_height(int height_logical_unit);
	void			set_line_height_px(int height_pixel_unit);

//font
	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int font_size);
	void			set_font_bold(int weight = FW_BOLD);
	LOGFONT			get_log_font() { return m_lf; }
	void			set_log_font(LOGFONT lf);

	void			load_history(CWinApp* app, CString section);
	void			save_history(CWinApp* app, CString section);

	//src���� �����ϴ� �޺��ڽ� �������� �ε����� ����.
	int				find_string(CString src);

//���� ����
	void			edit_end(bool valid);
	LRESULT			on_message_CSCEdit(WPARAM wParam, LPARAM lParam);


protected:
//design
	//-1�̸� ��Ʈũ�⿡ ���� �ڵ� ����
	int				m_line_height = -1;

//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

//���� ����
	bool			m_use_edit = true;		//���� �׸� �̿��� ���� Ŭ���� ���� ��������� ����� ������
	CEdit*			m_pEdit = NULL;
	CString			m_old_text;				//�����Ǳ� ���� ���� �ؽ�Ʈ
	CRect			m_edit_margin;			//edit box ���� ����(���η� ��� ���ĵǰ� ǥ���ϱ� ����)
	void			repos_edit();			//resize�� �ϸ� ������ ���µǹǷ� ��ġ�� ������ �ٽ� ���

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


