/* scpark.
- vertical align을 설정하기 위해서는 ES_MULTILINE 속성이 있어야 한다.
  ES_MULTILINE 속성은 생성후에는 변경할 수 없으므로 리소스 에디터에서 설정해야 한다.
  set_line_align(DT_VCENTER); 와 같이 호출하여 수직 정렬을 설정할 수 있다.

[수정 내용]
- 20250821 text, back, border, readonly, disabled 등 설정 모두 가능
- readonly일 경우는 기본	대화상자의 배경색인 COLOR_3DFACE로 설정되지만
  다른 색상으로 설정할 수 있도록 set_back_color_readonly() 추가.
  또는 set_use_readonly_default_color(false)를 호출하여 readonly일 때도 배경색을 변경하지 않도록 할 수 있다.

[수정 예정]
- set_round(8) 너무 많은 부분을 수정해야 하므로 우선 보류한다.
- DimEditCtrl, EditTrans 기능 추가 진행 중...
*/

#if !defined(AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_)
#define AFX_SCEdit_H__E889B47D_AF6B_4066_B055_967508314A88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SCEdit.h : header file
//

#include <Afxwin.h>
#include "../../colors.h"
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

	enum CSCStaticMsgs
	{
		
	};

	CWnd*	pThis = NULL;
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

	//동적 생성 시 호출
	bool					create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	//기본 CEdit::Create() override. 동적 생성 시 font width가 잘못 세팅되는 문제 수정을 위해 override.
	BOOL					Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	CString					get_text() { CString text; GetWindowText(text); return text; }
	void					set_text(CString text = _T("")) { CEdit::SetWindowText(text); }
	void					set_text(int n) { CString text; text.Format(_T("%d"), n); CEdit::SetWindowText(text); }
	void					SetWindowText(CString text) { set_text(text); }

	//아래와 같이 템플릿으로 구현하려 했으나 typeid()가 제대로 동작하지 않는 문제가 있어 사용할 수 없음.
	//L"test string"을 인자로 넘기면 CString으로 기대했으나 typeid()가 const wchar_t[12]로 인식하는 등
	//모든 타입을 열거해 줄 수 없으므로 사용할 수 없음.
	/*
	template<class T> void set_text(T v)
	{
		CString text;

		if (typeid(v) == typeid(int) || typeid(v) == typeid(long) || typeid(v) == typeid(bool) || typeid(v) == typeid(BOOL) ||
			typeid(v) == typeid(short) || typeid(v) == typeid(unsigned int) || typeid(v) == typeid(unsigned long) ||
			typeid(v) == typeid(unsigned short) || typeid(v) == typeid(INT) || typeid(v) == typeid(UINT))
			text.Format(_T("%d"), v);
		else if (typeid(v) == typeid(float) || typeid(v) == typeid(double))// || typeid(v) == typeid(Gdiplus::REAL))
			text.Format(_T("%f"), v);
		else if (typeid(v) == typeid(CString))
			text.Format(_T("%s"), v);
		else
			TRACE(_T("warning. not defined type.\n"));

		CEdit::SetWindowText(text);
	}
	*/

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCEdit)
	//}}AFX_VIRTUAL

	//editbox의 오른쪽에 액션버튼을 표시하여 특정 기능을 실행할 수 있다.
	//ex)action_find : 돋보기 그림을 그려주고 클릭하면 검색으로 사용
	//클릭되면 parent에게 message_scedit_action_button_down 혹은 up 메시지를 전송한다.
	void					set_action_button(int action);

	enum BUTTON_ACTION
	{
		action_none = 0,
		action_find,
	};

	CSCColorTheme		m_theme = CSCColorTheme(this);
	void				set_color_theme(int color_theme, bool invalidate = false); //apply current m_theme colors to the control.

	//Test_GdiButton 프로젝트에서는 CSCEdit이 투명하게 잘 표시되나
	//Test_SCThemeDialog 프로젝트에서는 투명하게 표시되지 않는다.
	//차이점은 전자의 경우 parent가 순수 CDialog를 상속받은 dlg이고
	//후자는 SCThemeDlg를 상속받은 dlg라는 점이다. 수정 필요.
	void				set_transparent(bool transparent = true);
	void				set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back);
	Gdiplus::Color		get_text_color() { return m_theme.cr_text; }
	void				set_text_color(Gdiplus::Color cr_text); // This Function is to set the Color for the Text.
	Gdiplus::Color		get_back_color() { return m_theme.cr_back; }
	void				set_back_color(Gdiplus::Color cr_back); // This Function is to set the BackGround Color for the Text and the Edit Box.
	//아직 set_text_color_disabled()는 효과가 적용되고 있지 않다. 수정 필요.
	void				set_text_color_disabled(Gdiplus::Color cr_text_disabled);
	void				set_back_color_disabled(Gdiplus::Color cr_back_disabled);

	//read only일 때 배경색을 변경할 수 있다. 파라미터를 주지 않으면 윈도우 기본 readonly 배경색(COLOR_3DFACE)으로 설정된다.
	void				set_back_color_readonly(Gdiplus::Color cr_back_readonly = get_sys_color(COLOR_3DFACE));
	//readonly일 때 전용 배경색인 m_cr_back_readonly를 사용할 지, 무관하게 m_cr_back을 사용할 지.
	void				set_use_readonly_color(bool use_default = true);

	//기본 CWnd::SetFont() override
	void				SetFont(CFont* font, BOOL bRedraw = TRUE);
	void				set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void				set_font_size(int nSize);
	void				set_font_weight(int weight = FW_BOLD);
	void				set_font_antialias(bool antialias = true);
	void				set_auto_font_size(bool bAuto = true, double ratio = 0.6);	//resize font depending on control's height, not width.
	void				recalc_font_size();						//recalculate font height when control size is changed.
	int					get_font_size(bool pixel_size = false);

	//CEdit::SetRect()를 이용해서 상하좌우 크기를 조정할 수 있는데
	//ES_MULTILINE 속성이 있어야만 동작하므로 속성에 반드시 멀티라인 속성을 설정해야 한다.
	//ES_MULTILINE 속성은 생성후에는 변경할 수 없는 속성이다.
	//https://forums.codeguru.com/showthread.php?361420-Want-to-set-quot-ES_MULTILINE-quot-property-of-Edit-object-externally
	//생성후에도 SetWindowLong()을 이용하여 변경할 수 있는 속성들
	//(ES_LOWERCASE, ES_NUMBER, ES_OEMCONVERT, ES_UPPERCASE, ES_WANTRETURN)
	//CDC::DrawText()의 define을 사용한다.(DT_TOP, DT_VCENTER (DT_CENTER가 아님에 주의), DT_BOTTOM)
	void				set_line_align(DWORD align = DT_VCENTER);
	int					get_line_align() { return m_valign; }
//dim text
	//m_cr_dim_text의 기본값은 Gdiplus::Color::LightGray이며
	//이 함수를 호출할 때 Gdiplus::Color::Transparent라는 값일 경우는 dim_text 파라미터만 변경하고자 하는 의미일 것이다.
	void				set_dim_text(CString dim_text, Gdiplus::Color cr_dim_text = Gdiplus::Color::Transparent);

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
	void				set_draw_border(bool draw = true, int border_width = -1, Gdiplus::Color cr_border = Gdiplus::Color::Transparent, int border_type = border_type_disregard);
	bool				get_draw_border() { return m_draw_border; }
	int					get_border_width() { return m_border_width; }
	Gdiplus::Color		get_border_color() { return m_theme.cr_border; }
	void				set_border_color(Gdiplus::Color cr_border) { m_theme.cr_border = cr_border; }
	int					get_border_type() { return m_border_type; }
	void				set_dark_border_on_focus(bool dark_on_focus = true) { m_dark_border_on_focus = dark_on_focus; }

	// Generated message map functions
protected:
	bool				m_transparent = false;

	//create()으로 동적 생성했는지, 일반 dlg 등에서 정적으로 생성했는지에 따라
	//일부 메시지(ex. VK_RETURN)의 처리방식이 달라지므로 이를 구분하기 위한 플래그.
	//default = false. create()이 호출되면 true로 자동 변경됨.
	bool				m_is_dynamic_control = false;

//vertical align
	//vertical align이므로 DT_CENTER가 아닌 DT_VCENTER로 줘야 한다.
	//단, 한줄이 아닌 여러줄을 입력하기 위한 CEdit일 경우는 DT_TOP으로 시작되어야 한다.
	//여러줄이고 DT_VCENTER일 경우 어디가 입력필드이고 어디가 여백인지가 애매하다.
	DWORD				m_valign = DT_TOP;

//border
	bool				m_draw_border = false;
	bool				m_dark_border_on_focus = false;		//focus일 때 border를 좀 더 진하게 그릴 지 여부. default = false
	int					m_border_width = 1;	//border width
	int					m_border_type = border_type_sunken;	//border radius
	//Gdiplus::Color	m_cr_border = Gdiplus::Color::LightGray;	//border color

	//editbox의 오른쪽에 액션버튼을 표시하여 특정 기능을 실행할 수 있다.
	//ex)돋보기 그림을 그려주고 클릭하면 검색으로 사용
	int					m_action_button = 0;
	bool				m_action_button_down = false;
	Gdiplus::Color		m_cr_button_back;
	Gdiplus::Color		m_cr_button_back_hover;
	Gdiplus::Color		m_cr_button_back_down;

//dim text
	CString				m_dim_text;
	Gdiplus::Color		m_cr_dim_text = Gdiplus::Color::LightGray;
	DWORD				m_dwStyle;
	void				draw_dim_text();


	//
	CSize				m_sz_action_button;
	//마우스가 액션버튼내에 있는지 판별
	bool				mouse_in_action_button(CPoint pt = CPoint(0, 0));


	//Gdiplus::Color	m_cr_text;
	//Gdiplus::Color	m_cr_back;
	Gdiplus::Color		m_cr_text_disabled;	//배경은 변경되나 text색상은 COLOR_GREYTEXT로 고정된듯하다. 현재로는 변경 불가.
	Gdiplus::Color		m_cr_back_disabled = Gdiplus::Color::LightGray;	//간혹 disabled일때 윈도우 기본 회색이 아닌 특정색으로 표현해야 할 필요가 있다.

	//readonly일 때 m_cr_back_readonly를 사용할 지 지정된 배경인 m_cr_back을 사용할 지.
	//때로는 readonly일 때도 m_cr_back으로 표현해야 하는 경우도 있다.
	//default = true
	bool				m_use_readonly_color = true;
	Gdiplus::Color		m_cr_back_readonly = get_sys_color(COLOR_3DFACE);

	CBrush				m_br_back;
	CBrush				m_br_back_disabled;


	LOGFONT				m_lf;
	CFont				m_font;
	int					m_font_size;
	bool				m_auto_resize_font;	//default = false
	double				m_auto_resize_ratio;
	int					m_default_height;
	void				reconstruct_font();
	void				update_ctrl();

	CRect				m_rect_NCbottom;
	CRect				m_rect_NCtop;

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
