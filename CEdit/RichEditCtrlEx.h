#pragma once

#include <Afxwin.h>
#include <afxcmn.h>

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

	void		SetDefaultTextColor( COLORREF crText ) { m_crText = crText; }
	void		SetBackColor( bool bSysColor, COLORREF crBack );
	void		SetClearLogInterval( int nInterval );
	void		SetMaxCharLimit( int nMaxChar ) { m_nMaxCharLimit = nMaxChar; }

	void		ClearAll();

	void		ToggleShowLog();
	void		ShowLog( bool bShow = true ) { m_bShowLog = bShow; }
	bool		IsShowLog() { return m_bShowLog; }

	void		ToggleShowTime();
	void		ShowTimeInfo( bool bShow = true ) { m_bShowTime = bShow; }
	bool		IsShowTime() { return m_bShowTime; }

	void		use_popup_menu(bool use) { m_use_popup_menu = use; }

	UINT		GetLineSpacing();
	void		SetLineSpacing( UINT nLineSpace );
	int			AppendToLog( CString str, COLORREF color = -1, BOOL bAddNewLine = TRUE );	//color가 -1이면 기본 컬러를 사용한다.(m_crText)
	void		Append( LPCTSTR lpszFormat, ... );
	void		Append( COLORREF cr, LPCTSTR lpszFormat, ... );
	int			AppendToLogAndScroll( CString str, COLORREF color = -1, BOOL bAddNewLine = TRUE );
	int			GetNumVisibleLines();

	COLORREF	GetComplementaryColor( COLORREF crColor );

	void		auto_scroll(bool auto_scroll) { m_auto_scroll = auto_scroll; }
	void		SetScrollSize( int nLines ) { m_nScrollSize = nLines; }

	int			get_align() { return m_align; }
	//PFA_LEFT(1), PFA_RIGHT(2), PFA_CENTER(3)
	void		set_align(int align);

	virtual		CRichEditCtrlEx& SetFontName( TCHAR *sfontname );
	virtual		CRichEditCtrlEx& SetFontSize( int nSize );
	virtual		CRichEditCtrlEx& SetFontBold( bool bBold = true );

protected:
	COLORREF	m_crText;
	COLORREF	m_crBack;
	bool		m_bShowLog;
	bool		m_bShowTime;
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
