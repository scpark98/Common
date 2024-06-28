#pragma once

#include <Afxwin.h>
#include <afxcmn.h>

/*
* 시간표시를 하지 않는 경우는 관계없으나 시간표시를 하는 경우는
  text의 앞 또는 뒤에 \n이 있을 경우 부가적인 처리가 필요하다.

  - 앞에 있을 경우 

*/
//scpark.
//call AfxInitRichEdit2() at App()::InitInstance() for using RECHED20.dll

// CRichEditCtrlEx

class CRichEditCtrlEx : public CRichEditCtrl
{
	DECLARE_DYNAMIC(CRichEditCtrlEx)

public:
	CRichEditCtrlEx();
	virtual ~CRichEditCtrlEx();

	enum CRichEditCtrlExMenu
	{
		id_menu_richedit_toggle_log = 9900,
		id_menu_richedit_clearl_log,
		id_menu_richedit_line_space10,
		id_menu_richedit_line_space15,
		id_menu_richedit_line_space20,
		id_menu_richedit_toggle_time,
	};

	void		SetDefaultTextColor(COLORREF crText) { m_crText = crText; }

	//back color를 변경한다.
	void		SetBackColor(COLORREF crBack);
	void		SetClearLogInterval(int nInterval);
	void		SetMaxCharLimit(int nMaxChar) { m_nMaxCharLimit = nMaxChar; }

	void		ClearAll();

	void		ToggleShowLog();
	void		ShowLog(bool bShow = true) { m_show_log = bShow; }
	bool		IsShowLog() { return m_show_log; }

	void		ToggleShowTime();
	void		ShowTimeInfo(bool bShow = true) { m_show_time = bShow; }
	bool		IsShowTime() { return m_show_time; }

	void		use_popup_menu(bool use) { m_use_popup_menu = use; }

	UINT		GetLineSpacing();
	//줄간격. 0=1줄, 1=1.5줄, 2=2.0줄
	void		SetLineSpacing(UINT nLineSpace);

	//20231004. Append~로 시작되는 4개의 함수를 1개로 간소화한다.
	//맨 끝에 "\n"을 자동으로 붙여주지 않으므로 필요하다면 addl()함수를 사용한다.
	//간혹 한 라인에 추가적인 로그를 표시할 필요가 있으므로 add(), addl()를 분리함.
	CString		add(COLORREF cr, LPCTSTR lpszFormat, ...);
	//addl은 add line의 약자로 맨 끝에 "\n"을 추가할 뿐 add와 동일하다.
	CString		addl(COLORREF cr, LPCTSTR lpszFormat, ...);

	int			AppendToLog(CString str, COLORREF color = -1, BOOL bAddNewLine = TRUE);	//color가 -1이면 기본 컬러를 사용한다.(m_crText)
	void		Append(LPCTSTR lpszFormat, ...);
	void		Append(COLORREF cr, LPCTSTR lpszFormat, ...);
	int			AppendToLogAndScroll(CString str, COLORREF color = -1, BOOL bAddNewLine = TRUE);



	int			GetNumVisibleLines();

	COLORREF	GetComplementaryColor(COLORREF crColor);

	void		auto_scroll(bool auto_scroll) { m_auto_scroll = auto_scroll; }
	void		SetScrollSize(int nLines) { m_nScrollSize = nLines; }

	int			get_align() { return m_align; }
	//PFA_LEFT(1), PFA_RIGHT(2), PFA_CENTER(3)
	void		set_align(int align);

	virtual		CRichEditCtrlEx& SetFontName(TCHAR *sfontname);
	virtual		CRichEditCtrlEx& SetFontSize(int nSize);
	virtual		CRichEditCtrlEx& SetFontBold(bool bBold = true);

protected:
	COLORREF	m_crText;
	COLORREF	m_crBack;
	bool		m_show_log;
	bool		m_show_time;
	bool		m_use_popup_menu = true;

	int			m_nClearLogInterval;	//KIOSK에서 메모리 증가를 막기 위해 주기적으로 로그 내용을 지워주는 타이머 세팅(단위.초, 0이면 동작 안함)
	int			m_nMaxCharLimit;		//정해진 문자수 이상이면 모두 지우고 새로 쓴다.
	int			m_nScrollSize;
	bool		m_auto_scroll = true;

	int			m_align = PFA_LEFT;

	void		OnPopupMenu(UINT menuID);

	LOGFONT		m_lf;
	CFont		m_font;
	int			m_nDefaultHeight;
	void		UpdateSurface();
	void		ReconstructFont();


protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PreSubclassWindow();
};
