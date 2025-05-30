/*
* 개발 배경 :
	- 타이틀바가 없는 themeUI의 dlg를 쉽게 상속받아 사용하기 위해 제작.
	- 타이틀바의 유무는 resource editor에서 속성 변경 또는 method로 동적 변경 가능.
	- resizable도 사용자가 정한대로 동작되며 동적 변경도 가능해야 한다.
	  (현재는 동적 변경 후 resize하면 잔상이 남는 버그가 있다.
	   일단은 resize가 필요한 dlg일 경우는 resource editor에서 Resizing속성을 주고 시작하자)
	- 윈도우 기본 타이틀바를 사용하지 않거나 필요한 경우 커스터마이즈 된 타이틀바를 표시한다.
	  (m_titlebar_height가 0보다 크면 동작)
	- 시스템 버튼은 SC_HELP, SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE 에서 원하는 버튼들을 추가할 수 있다.
	  (이미지로 세팅할 수도 있고 이미지를 별도로 지정하지 않으면 DC에 심플하게 그려준다)
	- 배경 이미지를 설정하면 이 클래스를 상속받은 모든 dlg는 그 배경으로 표시된다.
	  (별도 설정도 물론 가능해야 한다)

* Dialog의 Resource 속성
	- resource의 제목표시줄 유무, 테두리 속성등에 무관하게 동작되도록 구현해야 하지만
	  뭔가 모든 경우의 수에 대한 처리가 되지 않은 듯하다.
	  따라서 우선 속성을 제목표시줄 = 없음, 테두리 = Resizing 으로 설정할 것.
	  만약 resize를 지원하지 않는 dlg일 경우는 테두리 = Dialog Frame, 제목표시줄 = 없음 으로 설정.

	  또한 main dlg의 OnInitDialog()에서 다음의 코드를 추가해줘야 한다.
		//이 코드를 넣어야 작업표시줄에서 클릭하여 minimize, restore된다.
		//작업표시줄에서 해당 앱을 shift+우클릭하여 SYSMENU를 표시할 수 있다.
		//또한 CResizeCtrl을 이용하면 resize할 때 모든 컨트롤들의 레이아웃을 자동으로 맞춰주는데
		//아래 코드를 사용하지 않으면 타이틀바가 없는 dlg는 상단에 흰색 여백 공간이 생기는 부작용이 생긴다.
		//resize가 가능한 dlg일 경우는 WS_THICKFRAME을 넣어주면 윈도우에서 기본 제공하는 shadow도 표시되므로 CWndShadow와 같은
		//별도의 클래스를 이용하지 않아도 된다.
		//단, resize가 불필요한 dlg일 경우는 WS_THICKFRAME을 넣어주면 안되는데 이럴 경우는 부작용이 발생한다.
		//좀 더 방법을 찾아야 한다.
		SetWindowLong(m_hWnd, GWL_STYLE, WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

* main dlg의 OnBnClickedCancel(), PreTranslateMessage()를 추가한 경우 기본 처리 함수는
	CSCThemeDlg::OnCancel();이 아닌 CDialogEx::OnCancel();이어야 하고
	PreTranslateMessage()에서도
	return CSCThemeDlg::PreTranslateMessage(pMsg);가 아닌 return CDialogEx::PreTranslateMessage(pMsg);로 되어 있어야 한다.
	따라서 만약 main dlg에서 PreTranslateMessage()를 추가하지 않으면
	기본적으로 CSCThemeDlg::PreTranslateMessage()가 호출되므로
	main dlg의 OnBnClickedCancel() 함수가 호출되지 않는다.
*/

#pragma once

#include <afxwin.h>
#include <afxdialogex.h>

#include "../../GdiplusBitmap.h"
#include "../../CButton/SCSystemButtons/SCSystemButtons.h"
#include "../../CWnd/WndShadow/WndShadow.h"

// CSCThemeDlg 대화 상자
class CSCThemeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSCThemeDlg)

public:
	CSCThemeDlg(CWnd* pParent = nullptr, int left = 0, int top = 0, int right = 0, int bottom = 0);   // 표준 생성자입니다.
	CSCThemeDlg(UINT nResourceID, CWnd* pParent = nullptr);   // 표준 생성자입니다.
	//CSCThemeDlg(CString message, CString headline);
	virtual ~CSCThemeDlg();


	//윈도우 기본 타이틀바가 있는 dlg일 경우는 타이틀바와 관계된 모든 옵션은 무시된다.
	//void	SetWindowText(CString title) { set_title(title); }
	//void	SetWindowText(CString title) { CWnd::SetWindowText(title); m_title = title; Invalidate(); }
	void	set_title_bold(bool bold = true);
	bool	get_title_bold() { return (m_title_lf.lfWeight == FW_BOLD); }

	void	set_title_height(int height);
	int		get_title_height() { return m_titlebar_height; }

	void	set_title_text_color(Gdiplus::Color cr);
	Gdiplus::Color get_title_text_color() { return m_theme.cr_title_text; }
	void	set_title_back_color(Gdiplus::Color cr);
	Gdiplus::Color get_title_back_color() { return m_theme.cr_title_back; }
	void	set_title_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void	set_title_font_size(int size);
	void	set_title_font_bold(bool bold) { m_titlebar_bold = bold; m_title_lf.lfWeight = (bold ? FW_BOLD : FW_NORMAL); }
	//
	void	set_title_icon(UINT icon_id, int cx = 16, int cy = 16);
	void	set_title_icon(HICON hIcon);
	void	set_logo(UINT png_id) { m_img_logo.load(png_id); }
	void	show_titlebar_logo(bool show_logo = true) { m_show_logo = show_logo; Invalidate(); }

	void	set_titlebar_movable(bool movable) { m_titlebar_movable = movable; }

	//WS_THICKFRAME이 있어야만 윈도우 테두리에 그림자가 생기고 모니터 자동 레이아웃도 지원된다.
	//이 속성을 주면 resize가 가능해지는데 만약 이를 막고 싶다면 set_use_resizable(false)를 호출한다.
	void	set_use_resizable(bool use_resizable = true) { m_use_resizable = use_resizable; }

	//parent창이 resize 될 때 호출해줘야만 m_sys_buttons가 위치를 바로잡는다.
	void	adjust_sys_buttons();

	//필요한 시스템 버튼들을 추가해준다. 이 함수를 호출하지 않으면 기본 닫기 버튼만 사용된다.
	//set_system_buttons(SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE);	//항상 위에 버튼까지 필요한 경우
	//set_system_buttons(SC_CLOSE);	//닫기 버튼만 필요한 경우
	template <typename ... Types> void	set_system_buttons(Types... args)
	{
		int button_width = -1;
		if (m_sys_buttons.m_hWnd)
		{
			button_width = m_sys_buttons.get_button_width();
			m_sys_buttons.DestroyWindow();
			m_sys_buttons.create(this);
		}

		int n = sizeof...(args);
		int arg[] = { args... };

		for (int i = 0; i < n; i++)
			m_sys_buttons.insert_button(-1, arg[i]);

		m_sys_buttons.set_button_width(button_width);
		m_sys_buttons.set_button_height(m_titlebar_height - 2);
	}

	void	set_back_color(Gdiplus::Color cr) { m_theme.cr_back = cr; }

	//배경에 그림 표시, zoom? stretch? original size?, 
	void	set_back_image(CString imgType, UINT nResourceID, int draw_mode = CGdiplusBitmap::draw_mode_stretch);
	void	set_back_image(CString img_path, int draw_mode = CGdiplusBitmap::draw_mode_stretch);
	void	set_back_image(UINT resource_id, int draw_mode = CGdiplusBitmap::draw_mode_stretch);

	void	set_draw_border(bool draw, int width = 1, Gdiplus::Color cr = Gdiplus::Color::DimGray);
	void	set_border_color(Gdiplus::Color cr) { m_theme.cr_border = cr; }

	void	set_color_theme(int theme);
	void	enable_resize(bool resizable);

	//init_shadow()를 CSCThemeDlg::OnInitDialog()에서 호출하지 않고 실행한 후에 하려니 적용되지 않는다.
	//init_shadow();는 우선 호출해두고 동적으로 그림자 효과를 해제하거나 적용하려면 다음 함수를 호출한다.
	void	set_use_shadow(bool use_shadow = true);

	virtual BOOL OnInitDialog();
	virtual INT_PTR DoModal();
	virtual void OnOK() {};
	virtual void OnCancel() {};

protected:
	//CSCThemeDlg를 상속받아 만든 dlg들에서도 TIMER_ID를 지정할 수 있고 일반적으로 0번부터 지정하므로
	//여기서 0번부터 타이머 ID를 주면 위험하다. 다른 파생 dlg에서 사용하지 않을 법한 id부터 시작하자.

	bool				m_use_shadow = true;	//default = true
	CWndShadow			m_shadow;
	void				init_shadow();

	HICON				m_hIcon = NULL;

	CRect				m_border_thickness;		//resize 처리를 위한 기본 윈도우 테두리 두께. OnPaint()에서 그리는 테두리는 m_border_width.
	//이 변수는 DwmExtendFrameIntoClientArea()과 관련된 변수로 true/false에 따라 동작하게 하려 했으나 false일 경우는 부작용이 많다. 우선 true로 고정한다.
	//메인에서 WS_THICKFRAME을 주면 
	bool				m_has_thickframe = true;

	bool				m_use_resizable = true;

	int					m_titlebar_height = 32;// GetSystemMetrics(SM_CYCAPTION);
	bool				m_titlebar_bold = false;
	bool				m_titlebar_movable = true;

	CSCColorTheme		m_theme = CSCColorTheme(this);
	//Gdiplus::Color		m_cr_titlebar_text = ::GetSysColor(COLOR_CAPTIONTEXT);
	//Gdiplus::Color		m_cr_titlebar_back = gRGB(31, 31, 31);// ::GetSysColor(COLOR_ACTIVECAPTION);
	//Gdiplus::Color		m_cr_sys_buttons_back_hover;
	//Gdiplus::Color		m_cr_text;
	//Gdiplus::Color		m_cr_back;

	//Gdiplus::Color		m_cr_border = Gdiplus::Color::DimGray;
	bool				m_draw_border = false;
	int					m_border_width = 1;


	//프로그램 로고 아이콘 표시 여부. 기본값 true
	bool				m_show_logo = true;
	//프로그램 로고(png만 허용, 이 값이 없다면 기본 앱 아이콘을 사용한다)
	CGdiplusBitmap		m_img_logo;

	LOGFONT				m_title_lf;
	CFont				m_title_font;
	void				reconstruct_title_font();

	CGdiplusBitmap		m_img_back;
	int					m_img_back_draw_mode = CGdiplusBitmap::draw_mode_stretch;	//default = CGdiplusBitmap::draw_mode_stretch
	Gdiplus::Color		m_cr_out_of_back_img;		//배경 이미지를 zoom or origin크기로 그릴 경우 남은 영역의 색 채우기

	CSCSystemButtons	m_sys_buttons;

	//IDD 리소스를 사용하지 않고 동적 생성할 경우
	CWnd*				m_parent = NULL;
	bool				create(CWnd* parent, int left, int top, int right, int bottom);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg BOOL OnNcActivate(BOOL bActive);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
