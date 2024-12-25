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
	- 
*/

#pragma once

#include <afxwin.h>
#include <afxdialogex.h>

#include "../../GdiplusBitmap.h"
#include "../../CButton/SCSystemButtons/SCSystemButtons.h"

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
	void	set_title(CString title) { m_title = title; Invalidate(); }
	void	set_title_bold(bool bold = true);
	void	set_titlebar_height(int height);
	void	set_titlebar_text_color(Gdiplus::Color cr);
	void	set_titlebar_back_color(Gdiplus::Color cr);
	void	set_title_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void	set_title_font_size(int size);

	void	set_logo(UINT icon_id) { m_img_logo.load(icon_id); }
	void	show_titlebar_logo(bool show_logo = true) { m_show_logo = show_logo; Invalidate(); }



	//parent창이 resize 될 때 호출해줘야만 m_sys_buttons가 위치를 바로잡는다.
	void	adjust();

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
		m_sys_buttons.set_button_height(m_titlebar_height);
	}

	void	set_back_color(Gdiplus::Color cr) { m_cr_back = cr; }

	//배경에 그림 표시, zoom? stretch? original size?, 
	void	set_back_image(CString imgType, UINT nResourceID, int draw_mode = CGdiplusBitmap::draw_mode_stretch);
	void	set_back_image(CString img_path, int draw_mode = CGdiplusBitmap::draw_mode_stretch);
	void	set_back_image(UINT resource_id, int draw_mode = CGdiplusBitmap::draw_mode_stretch);

	void	set_border_color(Gdiplus::Color cr) { m_cr_border = cr; }

	enum THEMES
	{
		theme_default = 0,
		theme_visualstudio,
		theme_gray,
		theme_linkmemine,
	};

	void	set_color_theme(int theme);
	void	enable_resize(bool resizable);

	virtual BOOL OnInitDialog();
	virtual INT_PTR DoModal();
	virtual void OnOK() {};
	virtual void OnCancel() {};

protected:
	CRect				m_border_thickness;		//resize를 위한 기본 윈도우 테두리 두께
	bool				m_is_resizable = true;

	CString				m_title;
	int					m_titlebar_height = GetSystemMetrics(SM_CYCAPTION);

	Gdiplus::Color		m_cr_titlebar_text = ::GetSysColor(COLOR_CAPTIONTEXT);
	Gdiplus::Color		m_cr_titlebar_back = gRGB(31, 31, 31);// ::GetSysColor(COLOR_ACTIVECAPTION);
	Gdiplus::Color		m_cr_sys_buttons_back_hover;
	Gdiplus::Color		m_cr_text;
	Gdiplus::Color		m_cr_back;

	Gdiplus::Color		m_cr_border = Gdiplus::Color::DimGray;
	int					m_border_width = 1;		//그려질 테두리 두께


	//프로그램 로고 아이콘 표시 여부. 기본값 true
	bool				m_show_logo = true;
	//프로그램 로고(png만 허용, 이 값이 없다면 기본 앱 아이콘을 사용한다)
	CGdiplusBitmap		m_img_logo;

	LOGFONT				m_title_lf;
	CFont				m_title_font;
	void				reconstruct_title_font();

	CGdiplusBitmap		m_img_back;
	int					m_img_back_mode = CGdiplusBitmap::draw_mode_stretch;	//default = CGdiplusBitmap::draw_mode_stretch
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
};
