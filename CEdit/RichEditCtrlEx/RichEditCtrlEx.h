#pragma once

#include <Afxwin.h>
#include <afxcmn.h>
#include <deque>

#include "../../colors.h"

/*
* 시간표시를 하지 않는 경우는 관계없으나 시간표시를 하는 경우는
  text의 앞 또는 뒤에 \n이 있을 경우 부가적인 처리가 필요하다.

  - 앞에 있을 경우 

*/
//scpark.
//call AfxInitRichEdit2() at App()::InitInstance() for using RECHED20.dll


static const UINT Message_CRichEditCtrlEx = ::RegisterWindowMessage(_T("MessageString_CRichEditCtrlEx"));

class CSCKeywordFormat
{
public:
	CSCKeywordFormat()
	{

	}

	CSCKeywordFormat(CString _keyword, COLORREF _cr, bool _bold = false, bool _italic = false, bool _underline = false, bool _strikeout = false, bool _whole_word = false)
	{
		keyword = _keyword;
		cr = _cr;
		bold = _bold;
		italic = _italic;
		underline = _underline;
		strikeout = _strikeout;
		whole_word = _whole_word;
	}

	CString		keyword;
	COLORREF	cr;
	bool		bold = false;
	bool		italic = false;
	bool		underline = false;
	bool		strikeout = false;
	bool		whole_word = false;
};

class CRichEditCtrlExMessage
{
public:
	CRichEditCtrlExMessage(CWnd* _this, int _message)
	{
		pThis = _this;
		message = _message;
	}

	enum ENUM_MESSAGES
	{
		msg_hscroll = 0,
		msg_vscroll,
	};

	CWnd* pThis = nullptr;
	int		message;
};

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

	CSCColorTheme		m_theme = CSCColorTheme(this);
	void				set_color_theme(int color_theme, bool invalidate = false); //apply current m_theme colors to the control.

	void				set_default_text_color(Gdiplus::Color cr_text) { m_theme.cr_text = cr_text; }

	//back color를 변경한다.
	void				set_back_color(Gdiplus::Color cr_back);
	void				set_clear_log_interval(int interval);
	void				set_max_length_limit(int max_length) { m_max_length = max_length; }

	void				clear_all();

	void				toggle_show_log();
	void				show_log(bool bShow = true) { m_show_log = bShow; }
	bool				is_show_log() { return m_show_log; }

	void				toggle_show_time();
	void				show_time_info(bool bShow = true) { m_show_time = bShow; }
	bool				is_show_time() { return m_show_time; }

	void				use_popup_menu(bool use) { m_use_popup_menu = use; }

	UINT				get_line_spacing();
	//줄간격. 0=1줄, 1=1.5줄, 2=2.0줄
	void				set_line_spacing(UINT nLineSpace);

	//20231004. Append~로 시작되는 4개의 함수를 1개로 간소화한다.
	//맨 끝에 "\n"을 자동으로 붙여주지 않으므로 필요하다면 addl()함수를 사용한다.
	//간혹 한 라인에 추가적인 로그를 표시할 필요가 있으므로 add(), addl()를 분리함.
	CString				add(COLORREF cr, LPCTSTR lpszFormat, ...);
	//addl은 add line의 약자로 맨 끝에 "\n"을 추가할 뿐 add와 동일하다.
	//단, resource의 속성에서 multiline이 체크되어 있어야 한다.
	CString				addl(COLORREF cr, LPCTSTR lpszFormat, ...);

	int					AppendToLog(CString str, COLORREF color = -1, BOOL bAddNewLine = TRUE);	//color가 -1이면 기본 컬러를 사용한다.(m_crText)
	void				Append(LPCTSTR lpszFormat, ...);
	void				Append(COLORREF cr, LPCTSTR lpszFormat, ...);
	int					AppendToLogAndScroll(CString str, COLORREF color = -1, BOOL bAddNewLine = TRUE);

	//한줄씩 deque에 저장된 내용을 모두 합쳐서 rich의 내용을 update한다.
	void				set_text(std::deque<CString>* dqlist);


	int					GetNumVisibleLines();

	void				set_auto_scroll(bool auto_scroll) { m_auto_scroll = auto_scroll; }
	void				set_scroll_size(int nLines) { m_scroll_size = nLines; }

	int					get_align() { return m_align; }
	//PFA_LEFT(1), PFA_RIGHT(2), PFA_CENTER(3)
	void				set_align(int align);

	//기본 CWnd::SetFont() override
	void				SetFont(CFont* font, BOOL bRedraw = TRUE);
	void				set_font_name(TCHAR *sfontname);
	void				set_font_size(int nSize);
	void				set_font_weight(int weight);

	void				highlight_current_line();
	void				select_line(int line);

//file
	bool				load(CString path);
	bool				save(CString path);

//keyword highlight
	void				add_keyword_format(CSCKeywordFormat kf);
	void				clear_keyword_format();

protected:
	bool		m_show_log;
	bool		m_show_time;
	bool		m_use_popup_menu = true;

	int			m_clear_log_interval;	//KIOSK에서 메모리 증가를 막기 위해 주기적으로 로그 내용을 지워주는 타이머 세팅(단위.초, 0이면 동작 안함)
	int			m_max_length;		//정해진 문자수 이상이면 모두 지우고 새로 쓴다.
	int			m_scroll_size;
	bool		m_auto_scroll = true;	//default = true;

	int			m_align = PFA_LEFT;

	void		OnPopupMenu(UINT menuID);

	LOGFONT		m_lf;
	CFont		m_font;
	int			m_nDefaultHeight;
	void		UpdateSurface();
	void		reconstruct_font();

	std::deque<CSCKeywordFormat> m_keyword_formats;

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
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
};
