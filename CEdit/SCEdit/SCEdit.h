// This file was created on March 21st 2001. By Robert Brault
// scpark. 20240103.
// DimEditCtrl, EditTrans ��� �߰� ���� ��...
//
#if !defined(AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_)
#define AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SCEdit.h : header file
//

#include <Afxwin.h>
#include "../../GdiplusBitmap.h"

static const UINT Message_CSCEdit = ::RegisterWindowMessage(_T("MessageString_CSCEdit"));

class CSCEditMessage
{
public:
	CSCEditMessage(CWnd* _this, int _message)
	{
		pThis = _this;
		message = _message;
	}

	CWnd* pThis = NULL;
	int		message;
};

/////////////////////////////////////////////////////////////////////////////
// CSCEdit window
//color, font

class CSCEdit : public CEdit
{
// Construction
public:
	CSCEdit();
	virtual ~CSCEdit();

	enum MESSAGES
	{
		message_scedit_killfocus = 0,
		message_scedit_action_button_down,
		message_scedit_action_button_up,
	};

	CString					get_text() { CString text; GetWindowText(text); return text; }
	void					set_text(CString text) { CEdit::SetWindowText(text); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCEdit)
	//}}AFX_VIRTUAL

	bool					set_read_only(bool readonly = true);

	//editbox�� �����ʿ� �׼ǹ�ư�� ǥ���Ͽ� Ư�� ����� ������ �� �ִ�.
	//ex)action_find : ������ �׸��� �׷��ְ� Ŭ���ϸ� �˻����� ���
	//Ŭ���Ǹ� parent���� message_scedit_action_button_down Ȥ�� up �޽����� �����Ѵ�.
	void					set_action_button(int action);

	enum BUTTON_ACTION
	{
		action_none = 0,
		action_find,
	};

	//Test_GdiButton ������Ʈ������ CSCEdit�� �����ϰ� �� ǥ�õǳ�
	//Test_SCThemeDialog ������Ʈ������ �����ϰ� ǥ�õ��� �ʴ´�.
	//�������� ������ ��� parent�� ���� CDialog�� ��ӹ��� dlg�̰�
	//���ڴ� SCThemeDlg�� ��ӹ��� dlg��� ���̴�. ���� �ʿ�.
	virtual CSCEdit&		set_transparent(bool transparent = true);

	virtual CSCEdit&		set_text_color(Gdiplus::Color crText); // This Function is to set the Color for the Text.
	virtual CSCEdit&		set_back_color(Gdiplus::Color crBack); // This Function is to set the BackGround Color for the Text and the Edit Box.
	//���� set_text_color_disabled()�� ȿ���� ����ǰ� ���� �ʴ�. ���� �ʿ�.
	virtual CSCEdit&		set_text_color_disabled(Gdiplus::Color cr_text_disabled);
	virtual CSCEdit&		set_back_color_disabled(Gdiplus::Color cr_back_disabled);

	virtual	CSCEdit&		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CSCEdit&		set_font_size(int nSize);
	virtual CSCEdit&		set_font_bold(int weight = FW_BOLD);
	virtual CSCEdit&		set_auto_font_size(bool bAuto = true, double ratio = 0.6);	//resize font depending on control's height, not width.
	void					recalc_font_size();						//recalculate font height when control size is changed.
	int						get_font_size(bool pixel_size = false);

	//CEdit::SetRect()�� �̿��ؼ� �����¿� ũ�⸦ ������ �� �ִµ�
	//ES_MULTILINE �Ӽ��� �־�߸� �����ϹǷ� �Ӽ��� �ݵ�� ��Ƽ���� �Ӽ��� �����ؾ� �Ѵ�.
	//ES_MULTILINE �Ӽ��� �����Ŀ��� ������ �� ���� �Ӽ��̴�.
	//https://forums.codeguru.com/showthread.php?361420-Want-to-set-quot-ES_MULTILINE-quot-property-of-Edit-object-externally
	//�����Ŀ��� SetWindowLong()�� �̿��Ͽ� ������ �� �ִ� �Ӽ���
	//(ES_LOWERCASE, ES_NUMBER, ES_OEMCONVERT, ES_UPPERCASE, ES_WANTRETURN)
	//CDC::DrawText()�� define�� ����Ѵ�.(DT_TOP, DT_VCENTER, DT_BOTTOM)
	virtual CSCEdit&		set_line_align(DWORD align = DT_VCENTER);

//dim text
	//m_cr_dim_text�� �⺻���� Gdiplus::Color::LightGray�̸�
	//�� �Լ��� ȣ���� �� Gdiplus::Color::Transparent��� ���� ���� dim_text �Ķ���͸� �����ϰ��� �ϴ� �ǹ��� ���̴�.
	virtual CSCEdit&		set_dim_text(CString dim_text, Gdiplus::Color cr_dim_text = Gdiplus::Color::Transparent);

	enum BORDER_TYPE
	{
		border_type_none = 0,
		border_type_sunken,	//default
		border_type_raised,
		border_type_flat,
		border_type_bevel,
	};
	void					set_draw_border(bool draw = true, int border_width = 1, int border_type = border_type_sunken, Gdiplus::Color cr_border = Gdiplus::Color::Transparent);

	// Generated message map functions
protected:
	bool			m_transparent = false;

//border
	bool			m_draw_border = true;
	int				m_border_width = 1;	//border width
	int				m_border_type = border_type_sunken;	//border radius
	Gdiplus::Color	m_cr_border = Gdiplus::Color::DimGray;	//border color

	//editbox�� �����ʿ� �׼ǹ�ư�� ǥ���Ͽ� Ư�� ����� ������ �� �ִ�.
	//ex)������ �׸��� �׷��ְ� Ŭ���ϸ� �˻����� ���
	int				m_action_button = 0;
	bool			m_action_button_down = false;
	Gdiplus::Color	m_cr_button_back;
	Gdiplus::Color	m_cr_button_back_hover;
	Gdiplus::Color	m_cr_button_back_down;

//dim text
	CString			m_dim_text;
	Gdiplus::Color	m_cr_dim_text = Gdiplus::Color::LightGray;
	DWORD			m_dwStyle;
	void			draw_dim_text();


	//
	CSize			m_sz_action_button;
	//���콺�� �׼ǹ�ư���� �ִ��� �Ǻ�
	bool			mouse_in_action_button(CPoint pt = CPoint(0, 0));

	Gdiplus::Color	m_cr_text;
	Gdiplus::Color	m_cr_back;
	Gdiplus::Color	m_cr_text_disabled;	//����� ����ǳ� text������ COLOR_GREYTEXT�� �����ȵ��ϴ�. ����δ� ���� �Ұ�.
	Gdiplus::Color	m_cr_back_disabled;	//��Ȥ disabled�϶� ������ �⺻ ȸ���� �ƴ� Ư�������� ǥ���ؾ� �� �ʿ䰡 �ִ�.
	CBrush			m_br_back;
	CBrush			m_br_back_disabled;

	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	bool			m_auto_resize_font;	//default = false
	double			m_auto_resize_ratio;
	int				m_default_height;
	void			reconstruct_font();
	void			update_ctrl();

	CRect			m_rect_NCbottom;
	CRect			m_rect_NCtop;

	//{{AFX_MSG(CSCEdit)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor); // This Function Gets Called Every Time Your Window Gets Redrawn.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
public:
	afx_msg void OnPaint();
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnNcPaint();
	//afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEnKillfocus();
	afx_msg BOOL OnEnUpdate();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg BOOL OnEnSetfocus();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg BOOL OnEnChange();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_)
