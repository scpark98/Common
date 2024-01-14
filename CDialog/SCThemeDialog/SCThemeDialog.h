/*
* 개발 배경 :
	- 타이틀바가 없는 themeUI의 dlg를 쉽게 상속받아 사용하기 위해 제작.
	- 타이틀바의 유무는 resource editor에서 속성 변경 또는 method로 동적 변경 가능.
	- resizable도 사용자가 정한대로 동작되며 동적 변경도 가능해야 한다.
	  (현재는 동적 변경 후 resize하면 잔상이 남는 버그가 있다.
	   일단은 resize가 필요한 dlg일 경우는 resource editor에서 Resizing속성을 주고 시작하자)
	- 윈도우 기본 타이틀바를 사용하지 않거나 필요한 경우 커스터마이즈 된 타이틀바를 표시한다.
	  (m_titlebar_height가 0보다 크면 동작)
	- 시스템 버튼은 SC_PIN, SC_MINIMIZE, SC_MAXIMIZE, SC_CLOSE 에서 원하는 버튼들을 추가할 수 있다.
	  (이미지로 세팅할 수도 있고 이미지를 별도로 지정하지 않으면 DC에 심플하게 그려준다)
	- 배경 이미지를 설정하면 이 클래스를 상속받은 모든 dlg는 그 배경으로 표시된다.
	  (별도 설정도 물론 가능해야 한다)
	- 
*/

#pragma once

#include <afxwin.h>
#include <afxdialogex.h>

#include "../../CButton/SCSystemButtons/SCSystemButtons.h"

// CSCThemeDialog 대화 상자
class CSCThemeDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSCThemeDialog)

public:
	CSCThemeDialog(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	CSCThemeDialog(UINT nResourceID, CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CSCThemeDialog();

	//윈도우 기본 타이틀바가 있는 dlg일 경우는 타이틀바와 관계된 모든 옵션은 무시된다.
	//void	SetWindowText(CString title) { set_title(title); }
	void	set_title(CString title) { m_title = title; }
	void	set_titlebar_height(int height);
	void	set_titlebar_text_color(COLORREF cr) { m_cr_titlebar_text = cr; }
	void	set_titlebar_back_color(COLORREF cr) { m_cr_titlebar_back = cr; }
	void	set_back_color(COLORREF cr) { m_cr_back = cr; }

	enum COLOR_THEME
	{
		color_theme_window = 0,
		color_theme_visualstudio,
		color_theme_gray,
	};

	void	set_color_theme(int theme);

	void	enable_resize(bool resizable);

	virtual BOOL OnInitDialog();
	virtual INT_PTR DoModal();
	virtual void OnOK() {};
	virtual void OnCancel() {};

protected:
	CRect		m_border_thickness;
	bool		m_is_resizable = true;

	int			m_titlebar_height = 32;// GetSystemMetrics(SM_CYCAPTION);
	COLORREF	m_cr_titlebar_text = ::GetSysColor(COLOR_CAPTIONTEXT);
	COLORREF	m_cr_titlebar_back = RGB(31, 31, 31);// ::GetSysColor(COLOR_ACTIVECAPTION);
	CString		m_title;
	CFont*		m_font;

	//프로그램 로고(png만 허용, 이 값이 없다면 기본 앱 아이콘을 사용한다)
	CGdiplusBitmap	m_app_logo;

	COLORREF	m_cr_back = ::GetSysColor(COLOR_3DFACE);
	CSCSystemButtons	m_sys_buttons;

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
};
