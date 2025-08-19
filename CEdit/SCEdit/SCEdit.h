// scpark. 20240103.
// DimEditCtrl, EditTrans 기능 추가 진행 중...
//
#if !defined(AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_)
#define AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SCEdit.h : header file
//

#include <Afxwin.h>
#include "../../SCGdiplusBitmap.h"

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

//ES_MULTILINE 속성이 있어야만 정상 동작하므로 속성에 반드시 멀티라인 속성을 설정해야 한다.
//ES_MULTILINE 속성은 생성후에는 변경할 수 없는 속성이므로 리소스 에디터에서 설정해야 한다.
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
	void					set_text(CString text) { CEdit::SetWindowText(text); /*set_line_align(m_valign);*/ }
	void					SetWindowText(CString text) { set_text(text); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCEdit)
	//}}AFX_VIRTUAL

	bool					set_read_only(bool readonly = true);

	//editbox의 오른쪽에 액션버튼을 표시하여 특정 기능을 실행할 수 있다.
	//ex)action_find : 돋보기 그림을 그려주고 클릭하면 검색으로 사용
	//클릭되면 parent에게 message_scedit_action_button_down 혹은 up 메시지를 전송한다.
	void					set_action_button(int action);

	enum BUTTON_ACTION
	{
		action_none = 0,
		action_find,
	};

	//Test_GdiButton 프로젝트에서는 CSCEdit이 투명하게 잘 표시되나
	//Test_SCThemeDialog 프로젝트에서는 투명하게 표시되지 않는다.
	//차이점은 전자의 경우 parent가 순수 CDialog를 상속받은 dlg이고
	//후자는 SCThemeDlg를 상속받은 dlg라는 점이다. 수정 필요.
	virtual CSCEdit&		set_transparent(bool transparent = true);
	virtual CSCEdit&		set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back);
	virtual CSCEdit&		set_text_color(Gdiplus::Color crText); // This Function is to set the Color for the Text.
	virtual CSCEdit&		set_back_color(Gdiplus::Color crBack); // This Function is to set the BackGround Color for the Text and the Edit Box.
	//아직 set_text_color_disabled()는 효과가 적용되고 있지 않다. 수정 필요.
	virtual CSCEdit&		set_text_color_disabled(Gdiplus::Color cr_text_disabled);
	virtual CSCEdit&		set_back_color_disabled(Gdiplus::Color cr_back_disabled);

	virtual	CSCEdit&		set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CSCEdit&		set_font_size(int nSize);
	virtual CSCEdit&		set_font_bold(int weight = FW_BOLD);
	virtual CSCEdit&		set_auto_font_size(bool bAuto = true, double ratio = 0.6);	//resize font depending on control's height, not width.
	void					recalc_font_size();						//recalculate font height when control size is changed.
	int						get_font_size(bool pixel_size = false);

	//CEdit::SetRect()를 이용해서 상하좌우 크기를 조정할 수 있는데
	//ES_MULTILINE 속성이 있어야만 동작하므로 속성에 반드시 멀티라인 속성을 설정해야 한다.
	//ES_MULTILINE 속성은 생성후에는 변경할 수 없는 속성이다.
	//https://forums.codeguru.com/showthread.php?361420-Want-to-set-quot-ES_MULTILINE-quot-property-of-Edit-object-externally
	//생성후에도 SetWindowLong()을 이용하여 변경할 수 있는 속성들
	//(ES_LOWERCASE, ES_NUMBER, ES_OEMCONVERT, ES_UPPERCASE, ES_WANTRETURN)
	//CDC::DrawText()의 define을 사용한다.(DT_TOP, DT_VCENTER, DT_BOTTOM)
	virtual CSCEdit&		set_line_align(DWORD align = DT_VCENTER);
	int						get_line_align() { return m_valign; }
//dim text
	//m_cr_dim_text의 기본값은 Gdiplus::Color::LightGray이며
	//이 함수를 호출할 때 Gdiplus::Color::Transparent라는 값일 경우는 dim_text 파라미터만 변경하고자 하는 의미일 것이다.
	virtual CSCEdit&		set_dim_text(CString dim_text, Gdiplus::Color cr_dim_text = Gdiplus::Color::Transparent);

	enum BORDER_TYPE
	{
		border_type_disregard = -1,
		border_type_none,
		border_type_sunken,	//default
		border_type_raised,
		border_type_flat,
		border_type_bevel,
	};

	//border를 설정할 때 set_draw_border();를 호출하면 모든 설정값은 기본 멤버변수값대로 설정된다.
	//즉, border width는 m_border_width 값이 사용된다.
	//border를 해제하기 위해 set_draw_border(false);를 호출하면 border를 그리지 않을 뿐 기본 설정값들은 유지된다.
	//다시 border를 그리기 위해 set_draw_border();를 호출하면 전에 설정된 세팅값대로 그릴 수 있다.
	void			set_draw_border(bool draw = true, int border_width = -1, Gdiplus::Color cr_border = Gdiplus::Color::Transparent, int border_type = border_type_disregard);
	bool			get_draw_border() { return m_draw_border; }
	int				get_border_width() { return m_border_width; }
	Gdiplus::Color	get_border_color() { return m_cr_border; }
	int				get_border_type() { return m_border_type; }

	// Generated message map functions
protected:
	bool			m_transparent = false;

//vertical align
	DWORD			m_valign = DT_VCENTER;	//vertical align이므로 DT_CENTER가 아닌 DT_VCENTER로 줘야 한다.

//border
	bool			m_draw_border = false;
	int				m_border_width = 1;	//border width
	int				m_border_type = border_type_sunken;	//border radius
	Gdiplus::Color	m_cr_border = Gdiplus::Color::LightGray;	//border color

	//editbox의 오른쪽에 액션버튼을 표시하여 특정 기능을 실행할 수 있다.
	//ex)돋보기 그림을 그려주고 클릭하면 검색으로 사용
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
	//마우스가 액션버튼내에 있는지 판별
	bool			mouse_in_action_button(CPoint pt = CPoint(0, 0));

	Gdiplus::Color	m_cr_text;
	Gdiplus::Color	m_cr_back;
	Gdiplus::Color	m_cr_text_disabled;	//배경은 변경되나 text색상은 COLOR_GREYTEXT로 고정된듯하다. 현재로는 변경 불가.
	Gdiplus::Color	m_cr_back_disabled = Gdiplus::Color::LightGray;	//간혹 disabled일때 윈도우 기본 회색이 아닌 특정색으로 표현해야 할 필요가 있다.
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
