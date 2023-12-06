// RichEditCtrlEx.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "RichEditCtrlEx.h"

//#include <stdarg.h>
#include <strsafe.h>	//for StringCchCopyN

#define TIMER_CLEAR_LOG					0

// CRichEditCtrlEx

IMPLEMENT_DYNAMIC(CRichEditCtrlEx, CRichEditCtrl)

CRichEditCtrlEx::CRichEditCtrlEx()
{
	m_crText			= ::GetSysColor(COLOR_GRAYTEXT);
	m_crBack			= RGB( 255, 255, 255 );
	m_show_log			= true;
	m_show_time			= AfxGetApp()->GetProfileInt(_T("setting\\rich_edit"), _T("show time"), true);
	m_nClearLogInterval	= 0;
	m_nMaxCharLimit		= 0;
	m_nScrollSize		= 2;

	memset(&m_lf, 0, sizeof(LOGFONT));
}

CRichEditCtrlEx::~CRichEditCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CRichEditCtrlEx, CRichEditCtrl)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_TIMER()
	ON_COMMAND_RANGE(id_menu_richedit_toggle_log, id_menu_richedit_toggle_time, OnPopupMenu)
	ON_WM_MOUSEHWHEEL()
END_MESSAGE_MAP()



// CRichEditCtrlEx 메시지 처리기입니다.
//void CRichEditCtrlEx::append(COLORREF cr, CString text, bool linefeed)
//{
//	append(cr, _T("%s%c"), text, (linefeed ? '\n' : '\0'));
//}

void CRichEditCtrlEx::append(COLORREF cr, LPCTSTR lpszFormat, ...)
{
	if (m_hWnd == NULL)
		return;

	if (!m_show_log)
		return;

	if (cr == -1)
		cr = m_crText;

	//CString으로 변환
	CString new_text;
	va_list args;
	va_start(args, lpszFormat);
	new_text.FormatV(lpszFormat, args);


	//만약 텍스트의 맨 앞에 \n이 붙어 있으면 이전 로그 라인과 라인을 구분하기 위함인데
	//그냥 기록하면 시간정보를 출력한 후 라인이 변경된다.
	//text의 맨 앞에 \n이 있다면 먼저 처리해준다.
	int i;
	int linefeed_count = 0;
	bool skip_time_info = false;

	for (i = 0; i < new_text.GetLength(); i++)
	{
		if (new_text[i] == '\n')
			linefeed_count++;
		else
			break;
	}

	//new_text의 앞부분에 있는 '\n'을 제외한 나머지 문자열
	new_text = new_text.Mid(linefeed_count);


	// Set insertion point to end of text
	int			nOldLines = 0, nNewLines = 0, nScroll = 0;
	long		nInsertionPoint = 0;

	nInsertionPoint = GetWindowTextLength();

	if (linefeed_count == 0 && new_text.IsEmpty())
	{
		SetSel(nInsertionPoint, -1);
		ReplaceSel(_T("\n"));
	}
	else
	{
		for (i = 0; i < linefeed_count; i++)
		{
			SetSel(nInsertionPoint, -1);
			ReplaceSel(_T("\n"));
			nInsertionPoint = GetWindowTextLength();
		}
	}

	nInsertionPoint = GetWindowTextLength();

	if (m_show_time)
	{
		//m_show_time = true일때 첫 컬럼인 경우에만 시간값을 출력한다.
		int total_lines = GetLineCount();
		int len = LineLength(total_lines - 1);
		if (len > 0)
		{
			TCHAR p[1024] = { 0, };
			GetLine(total_lines - 1, p, 1024);
			p[len] = '\0';
			CString cur_line(p);
			cur_line.Trim();
			skip_time_info = (cur_line.GetLength() > 0);
		}
	}

	int nOldFirstVisibleLine = GetFirstVisibleLine();
	long lMinSel, lMaxSel;
	GetSel(lMinSel, lMaxSel);


	CHARFORMAT	cf;

	CPoint pt = GetCaretPos();
	//TRACE(_T("%d, %d\n"), pt.x, pt.y);
	if (pt.x > 1)
		m_auto_scroll = false;

	SCROLLINFO	si;
	si.cbSize = sizeof(SCROLLINFO);
	ZeroMemory(&si, si.cbSize);
	GetScrollInfo(SB_VERT, &si);



	nOldLines = GetLineCount();
	nInsertionPoint = GetWindowTextLength();

	SetSel(nInsertionPoint, nInsertionPoint);

	PARAFORMAT2 pf;
	GetParaFormat(pf);
	pf.dwMask = PFM_ALIGNMENT;
	pf.wAlignment = m_align;
	SetParaFormat(pf);
	SendMessage(EM_SETMODIFY, (WPARAM)TRUE, 0L);

	if (m_show_time && !skip_time_info)
	{
		SYSTEMTIME	t;
		CString sTime;

		::GetLocalTime(&t);
		sTime.Format(_T("%d-%02d-%02d %02d:%02d:%02d(%03d) "), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = 0;	// To disable CFE_AUTOCOLOR
		cf.crTextColor = RGB(128, 128, 128);
		SetSelectionCharFormat(cf);

		ReplaceSel(sTime);
	}

	// Save number of lines before insertion of new text
	nOldLines = GetLineCount();

	// Initialize character format structure
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;	// To disable CFE_AUTOCOLOR

	if (cr == -1)
		cf.crTextColor = m_crText;
	else
		cf.crTextColor = cr;

	//텍스트 전체 크기가 특정 크기를 넘어가면 클리어
	if (m_nMaxCharLimit > 0 && nInsertionPoint >= m_nMaxCharLimit)
	{
		//clear
		SetSel(0, -1);
		ReplaceSel(_T(""));
	}

	nInsertionPoint = -1;
	SetSel(nInsertionPoint, -1);

	//  Set the character format
	SetSelectionCharFormat(cf);

	// Replace selection. Because we have nothing selected, this will simply insert
	// the string at the current caret position.
	//SetRedraw(FALSE);
	ReplaceSel(new_text);
	//SetRedraw(TRUE);

#ifdef _DEBUG
	TRACE(new_text);
#endif

	//여기서부터의 코드는 맨 마지막 라인으로 스크롤시킬지 말지를 결정.
	//스크롤 위치가 맨 끝이면 항상 마지막 라인으로 자동 스크롤하고
	//스크롤 위치가 중간이면 자동 스크롤시키지 않도록 구현하려 했으나 아직 미완성.
	TRACE(_T("m_auto_scroll = %d, pos = %d, trackpos = %d, page = %d, min = %d, max = %d, sel = %d, %d\n"),
		m_auto_scroll,
		si.nPos,
		si.nTrackPos,
		si.nPage,
		si.nMin,
		si.nMax,
		lMinSel, lMaxSel);
	if (si.nPos != si.nTrackPos)
		m_auto_scroll = false;

	//뭔가 의도대로 동작하지 않는다.
	//현재는 포커스를 가지면 항상 맨 마지막으로 자동 스크롤되고
	//포커스가 없으면 마지막 줄로 자동 스크롤되진 않는다.
	/*
	if (si.nPos + si.nPage == si.nMax + 1)
	{
		//SetScrollPos(SB_VERT, 10000000);
		int nVisible = GetNumVisibleLines();
		LineScroll(INT_MAX);
		LineScroll(1 - nVisible);
	}
	*/
#if 0
	//if (!m_auto_scroll)
	//	return 0;

	int nVisible = GetNumVisibleLines();

	// Now this is the fix of CRichEditCtrl's abnormal behaviour when used
	// in an application not based on dialogs. Checking the focus prevents
	// us from scrolling when the CRichEditCtrl does so automatically,
	// even though ES_AUTOxSCROLL style is NOT set.


	if (this != GetFocus())
	{
		LineScroll(INT_MAX);
		LineScroll(1 - nVisible);
	}

	return;

	CRect rc;
	GetClientRect(rc);
	// Get new line count
	if (false)//(si.nMax - si.nTrackPos) < rc.Height())
	{
		TRACE(_T("true\n"));
		nNewLines = GetLineCount();

		// Scroll by the number of lines just inserted
		nScroll = nNewLines - nOldLines;
		LineScroll(nScroll);
	}
	else// if (false)
	{
		TRACE(_T("false\n"));
		//SetSel(lMinSel, lMaxSel);

		int nNewFirstVisibleLine = GetFirstVisibleLine();

		if (nOldFirstVisibleLine != nNewFirstVisibleLine)
		{
			SetRedraw(TRUE);
			LineScroll(nOldFirstVisibleLine - nNewFirstVisibleLine);
		}

		SetRedraw(TRUE);
	}
#endif
}



//=============================================================================
// Generics
//=============================================================================


//-----------------------------------------------------------------------------
//  AppendToLog()
///
/// \brief	Add a string to the log window at the current position and scroll
///			by the number of inserted lines (the naive solution for
///			auto-scrolling).
///
/// The string is added to the log starting at the current position,
/// i.e. without starting a new line. Then the control scrolls down by the
/// number of lines inserted.
/// The string is displayed in the specified text color.
/// The string may be a multiline string using carriage return/line feed
/// (i.e. newline) characters to indicate a line breaks.
///
/// The scrolling mechanism used here is kind of naive, because it assumes
/// that the user did not touch the scroll bars and that the scroll position
/// is always the end of the text. However, this is not the general case.
/// In general, we need to assume that the current scrolling position is
/// unkown. A solution for that is shown in the AppendToLogAndScroll()
/// method.
///
/// \param [in]		str		The string to add to the message log.
/// \param [in]		color	The text color of the string. You may use the
///							RGB(r,g,b) macro to specify the color byte-wise.
/// \return					An integer indicating sucess or failure:
///							- 0, if the function succeeded.
///							- (-1), if the function failed.
///							(This function always returns 0, because no
///							parameter or failure checking is done.)
///
/// \remark
/// Support for adding multiline strings requires the ES_MULTILINE style
/// to be set.
/// If you are not using the Visual Studio Wizards but create the control
/// indirectly using the Create() method, you should use the following
/// style: WS_CHILD|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_READONLY.
///
/// \sa AppendToLogAndScroll()
//-----------------------------------------------------------------------------
/*
* vs처럼 내용을 보기 위해 스크롤하면 그 자리에서 멈춰있고
* 맨 마지막 라인으로 캐럿을 옮겨놓으면 자동 스크롤되도록 처리했으나
* 뭔가 깜빡임이 발생한다.
*/
int CRichEditCtrlEx::AppendToLog(CString str, COLORREF color /*= -1*/, BOOL bAddNewLine /*= TRUE*/ )
{
	if (m_hWnd == NULL)
		return 0;

	if (!m_show_log)
		return 0;


	//만약 로그 텍스트의 맨 앞에 \n이 붙어있으면 이전 로그 라인과 라인을 구분하기 위함인데
	//그냥 기록하면 시간부터 라인번호까지 출력한 후 라인이 변경된다.
	//log_text의 맨 앞에 \n이 있다면 먼저 처리해준다.
	int i;
	int linefeed_count = 0;

	for (i = 0; i < str.GetLength(); i++)
	{
		if (str[i] == '\n')
			linefeed_count++;
		else
			break;
	}

	if (linefeed_count > 0)
		str = str.Mid(linefeed_count);



	//SetRedraw(FALSE);
	int nOldFirstVisibleLine = GetFirstVisibleLine();
	long lMinSel, lMaxSel;
	GetSel(lMinSel, lMaxSel);

	//str의 끝에 \n이 있던 없던 옵션대로 추가할 건 추가한다.
	//str의 끝에 \n이 이미 있으면 스킵하려 했으나 의도적인 경우도 존재하므로
	//코드는 명시된 규칙 그대로 수행하는게 맞다.
	if (bAddNewLine)// && (str.Right(1) != "\n") )
		str += "\n";

	int			nOldLines = 0, nNewLines = 0, nScroll = 0;
	long		nInsertionPoint = 0;
	CHARFORMAT	cf;

	CPoint pt = GetCaretPos();
	//TRACE(_T("%d, %d\n"), pt.x, pt.y);
	if (pt.x > 1)
		m_auto_scroll = false;

	SCROLLINFO	si;
	si.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_VERT, &si);

	/*
	TRACE(_T("m_auto_scroll = %d, pos = %d, trackpos = %d, max = %d, sel = %d, %d\n"),
		m_auto_scroll,
		si.nPos,
		si.nTrackPos,
		si.nMax,
		lMinSel, lMaxSel);
	if (si.nPos != si.nTrackPos)
		m_auto_scroll = false;
	*/

	nOldLines = GetLineCount();
	nInsertionPoint = GetWindowTextLength();

	if (m_hWnd == NULL)
		return 0;

	SetSel(nInsertionPoint, nInsertionPoint);

	//TRACE(_T("nOldLines = %d\n"), nOldLines);
	
	PARAFORMAT2 pf;
	GetParaFormat(pf);
	pf.dwMask = PFM_ALIGNMENT;
	pf.wAlignment = m_align;
	SetParaFormat(pf);
	SendMessage(EM_SETMODIFY, (WPARAM)TRUE, 0L);
	

	if (m_show_time)
	{
		SYSTEMTIME	t;
		CString sTime;

		::GetLocalTime( &t );
		sTime.Format( _T("%d-%02d-%02d %02d:%02d:%02d(%03d) "), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds );

		cf.cbSize		= sizeof(CHARFORMAT);
		cf.dwMask		= CFM_COLOR;
		cf.dwEffects	= 0;	// To disable CFE_AUTOCOLOR
		cf.crTextColor	= RGB( 128, 128, 128 );
		SetSelectionCharFormat(cf);

		ReplaceSel( sTime );
	}

	// Save number of lines before insertion of new text
	nOldLines		= GetLineCount();

	// Initialize character format structure
	cf.cbSize		= sizeof(CHARFORMAT);
	cf.dwMask		= CFM_COLOR;
	cf.dwEffects	= 0;	// To disable CFE_AUTOCOLOR

	if ( color == -1 )
		cf.crTextColor	= m_crText;
	else
		cf.crTextColor	= color;

	// Set insertion point to end of text
	nInsertionPoint = GetWindowTextLength();

	if (linefeed_count == 0 && str.IsEmpty())
	{
		SetSel(nInsertionPoint, -1);
		ReplaceSel(_T("\n"));
	}
	else
	{
		for (i = 0; i < linefeed_count; i++)
		{
			SetSel(nInsertionPoint, -1);
			ReplaceSel(_T("\n"));
			nInsertionPoint = GetWindowTextLength();
		}
	}

	nInsertionPoint = GetWindowTextLength();
	
	//텍스트 전체 크기가 특정 크기를 넘어가면 클리어
	if (m_nMaxCharLimit > 0 && nInsertionPoint >= m_nMaxCharLimit)
	{
		//clear
		SetSel(0, -1);
		ReplaceSel( _T("") );
	}

	nInsertionPoint = -1;
	SetSel(nInsertionPoint, -1);


    //  Set the character format
    SetSelectionCharFormat(cf);

	// Replace selection. Because we have nothing selected, this will simply insert
	// the string at the current caret position.
	ReplaceSel(str);
#ifdef _DEBUG
	TRACE(str);
#endif


	//if (!m_auto_scroll)
	//	return 0;


	CRect rc;
	GetClientRect(rc);
	// Get new line count
	if ((si.nMax - si.nTrackPos) < rc.Height())
	{
		nNewLines = GetLineCount();

		// Scroll by the number of lines just inserted
		nScroll = nNewLines - nOldLines;
		LineScroll(nScroll);
	}
	else// if (false)
	{
		SetSel(lMinSel, lMaxSel);

		int nNewFirstVisibleLine = GetFirstVisibleLine();

		if (nOldFirstVisibleLine != nNewFirstVisibleLine)
		{
			SetRedraw(TRUE);
			LineScroll(nOldFirstVisibleLine - nNewFirstVisibleLine);
		}

		SetRedraw(TRUE);
	}


	return 0;
}

void CRichEditCtrlEx::Append( LPCTSTR lpszFormat, ... )
{
	TCHAR szBuffer[512];

	size_t cb = 0;
	va_list args;
	va_start(args, lpszFormat);
	::StringCchVPrintfEx(szBuffer, 512, NULL, &cb, 0, lpszFormat, args);
	va_end(args);

	//시간 표시가 true라고 하더라도 라인의 맨 처음 컬럼이 아니면 시간표시를 생략시킨다.
	if (m_show_time)
	{
		int total_lines = GetLineCount();
		int len = LineLength(total_lines);
		TCHAR sline[1024] = { 0, };
		GetLine(total_lines - 1, sline, 1024);

		if (len > 0)
		{
			m_show_time = false;
			AppendToLog(szBuffer, m_crText, false);
			m_show_time = true;
		}
		else
		{
			AppendToLog(szBuffer, m_crText, false);
		}
	}
	else
	{
		AppendToLog(szBuffer, m_crText, false);
	}
}

void CRichEditCtrlEx::Append(COLORREF cr, LPCTSTR lpszFormat, ...)
{
	TCHAR szBuffer[512];

	size_t cb = 0;
	va_list args;
	va_start(args, lpszFormat);
	::StringCchVPrintfEx(szBuffer, 512, NULL, &cb, 0, lpszFormat, args);
	va_end(args);

	//시간 표시가 true라고 하더라도 라인의 맨 처음 컬럼이 아니면 시간표시를 생략시킨다.
	if (m_show_time)
	{
		int total_lines = GetLineCount();
		int len = LineLength(total_lines);
		TCHAR sline[1024] = { 0, };
		GetLine(total_lines - 1, sline, 1024);

		if (len > 0)
		{
			m_show_time = false;
			AppendToLog(szBuffer, cr, false);
			m_show_time = true;
		}
		else
		{
			AppendToLog(szBuffer, cr, false);
		}
	}
	else
	{
		AppendToLog(szBuffer, cr, false);
	}
}

//-----------------------------------------------------------------------------
//  AppendToLogAndScroll()
///
/// \brief	Add a string to the serial log window at the current position,
///			then scroll to the end of the text such that the last line of
///			the text is shown at the bottom of the CRichEditCtrl.
///
/// The string is added to the message log starting at the current position,
/// i.e. without starting a new line. Then the control scrolls down to show
/// as much text as possible, including the last line of text at the very
/// bottom.
/// The string is displayed in the specified text color.
/// The string may be a multiline string using carriage return/line feed
/// (i.e. newline) characters to indicate a line breaks.
///
/// \param [in]		str		The string to add to the message log.
/// \param [in]		color	The text color of the string. You may use the
///							RGB(r,g,b) macro to specify the color byte-wise.
/// \return					An integer indicating sucess or failure:
///							- 0, if the function succeeded.
///							- (-1), if the function failed.
///							(This function always returns 0, because no
///							parameter or failure checking is done.)
///
/// \remark
/// The automatic scrolling function would be easy, if the MFC documentation
/// was correct. Unfortunetely, it is not as trivial as one might think.
/// If the CRichEditCtrl has the focus, it scrolls automatically if you
/// insert text programatically. If it does not have the focus, it does not
/// scroll automatically, so in that case you can use the LineScroll()
/// method and you get the results you would expect when reading the MFC docs.
/// This is true even if ES_AUTOxSCROLL style is NOT set.
///
/// So the point is to check in the AppendToLogAndScroll() method if the
/// affected CRichEditCtrl has the focus. If so, we must not call
/// LineScroll(). If not, it is safe to call LineSroll() to first scroll to
/// the very end, which means that the last line of text is shown at the top
/// of the CRichEditCtrl.
/// Then we call LineScroll() a second time, this time scrolling back by
/// the number of visible lines. This leads to having the last line of the
/// text being displayed at the bottom of CRichEditCtrl.
///
/// Please note that in this sample application, the CRichEditCtrl never has
/// the focus, because we always have to click a button in order to insert
/// text. However, if you are using the code in an application not based on
/// a dialog and that fills up the control where the user could have set focus
/// to the control first, this method would fail to scroll correctly without
/// checking the focus.
/// I used this code in an MDI application, and there the control claims
/// to have the focus if I click into the control before clicking a menu
/// command (whatever the reason might be why in that case the focus is
/// not lost to the menu command).
///
/// Please note that the code is written for maximum comprehension / good
/// readability, not for code or execution efficiency.
//-----------------------------------------------------------------------------
int CRichEditCtrlEx::AppendToLogAndScroll(CString str, COLORREF color /*= -1*/, BOOL bAddNewLine /*= TRUE*/)
{
	if ( m_hWnd == NULL )
		return 0;

	if ( !m_show_log )
		return 0;

	long nVisible = 0;
	long nInsertionPoint = 0;
	CHARFORMAT cf;
	
	if ( bAddNewLine && ( str.Right( 1 ) != "\n" ) )
		str += _T("\n");

	// Initialize character format structure
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0; // To disable CFE_AUTOCOLOR

	if ( color == -1 )
		cf.crTextColor = m_crText;
	else
		cf.crTextColor = color;

	// Set insertion point to end of text
	nInsertionPoint = GetWindowTextLength();
	SetSel(nInsertionPoint, -1);
	
	// Set the character format
	SetSelectionCharFormat(cf);

	// Replace selection. Because we have nothing 
	// selected, this will simply insert
	// the string at the current caret position.
	ReplaceSel(str);

	// Get number of currently visible lines or maximum number of visible lines
	// (We must call GetNumVisibleLines() before the first call to LineScroll()!)
	nVisible   = GetNumVisibleLines();

	// Now this is the fix of CRichEditCtrl's abnormal behaviour when used
	// in an application not based on dialogs. Checking the focus prevents
	// us from scrolling when the CRichEditCtrl does so automatically,
	// even though ES_AUTOxSCROLL style is NOT set.


	if ( this == GetFocus() )
	{
		LineScroll(INT_MAX);
		LineScroll(1 - nVisible);
	}

	return 0;
}


//-----------------------------------------------------------------------------
//  GetNumVisibleLines()
///
/// \brief	Returns the number of lines that are currently visible in the
///			client area of the given CRichEditCtrl.
///
/// 
///
/// \param [in]		pCtrl	Pointer to the CRichEditCtrl object to query.
///
/// \return					The number of currently visible lines.
///
/// \remark
/// The code is written for best comprehension / readability, not for code
/// or execution efficiency.
//-----------------------------------------------------------------------------
int CRichEditCtrlEx::GetNumVisibleLines()
{
	CRect rect;
	long nFirstChar, nLastChar;
	long nFirstLine, nLastLine;

	// Get client rect of rich edit control
	GetClientRect(rect);

	// Get character index close to upper left corner
	nFirstChar = CharFromPos(CPoint(0, 0));

	// Get character index close to lower right corner
	nLastChar = CharFromPos(CPoint(rect.right, rect.bottom));
	if (nLastChar < 0)
	{
		nLastChar = GetTextLength();
	}

	// Convert to lines
	nFirstLine = LineFromChar(nFirstChar);
	nLastLine  = LineFromChar(nLastChar);

	return (nLastLine - nFirstLine);
}

void CRichEditCtrlEx::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CRichEditCtrl::OnRButtonDown(nFlags, point);
}


void CRichEditCtrlEx::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_use_popup_menu)
		return;

	CMenu	menu;

	menu.CreatePopupMenu();

	menu.AppendMenu( MF_STRING, id_menu_richedit_clearl_log, _T("Clear all logs(&C)") );
	menu.AppendMenu( MF_SEPARATOR);
	menu.AppendMenu( MF_STRING, id_menu_richedit_toggle_log, _T("Display logs(&S)") );
	menu.AppendMenu( MF_STRING, id_menu_richedit_toggle_time, _T("Display time info(&D)") );
	menu.AppendMenu( MF_SEPARATOR);
	menu.AppendMenu( MF_STRING, id_menu_richedit_line_space10, _T("1.0 line space(&1)") );
	menu.AppendMenu( MF_STRING, id_menu_richedit_line_space15, _T("1.5 line space(&2)") );
	menu.AppendMenu( MF_STRING, id_menu_richedit_line_space20, _T("2.0 line space(&3)") );

	//현재 라인간격값을 얻어온다.
	PARAFORMAT2	paraFormat;
	GetParaFormat( paraFormat );
	paraFormat.dwMask = PFM_LINESPACING;
	BYTE nLineSpacing = paraFormat.bLineSpacingRule;	//줄간격을 1.5배로 한다. 0=1.0, 1=1.5, 2=2.0

	menu.CheckMenuItem( id_menu_richedit_toggle_log, m_show_log ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( id_menu_richedit_toggle_time, m_show_time ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( id_menu_richedit_line_space10, nLineSpacing == 0 ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( id_menu_richedit_line_space15, nLineSpacing == 1 ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( id_menu_richedit_line_space20, nLineSpacing == 2 ? MF_CHECKED : MF_UNCHECKED );

	menu.EnableMenuItem( id_menu_richedit_toggle_time, m_show_log ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_richedit_line_space10, m_show_log ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_richedit_line_space15, m_show_log ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( id_menu_richedit_line_space20, m_show_log ? MF_ENABLED : MF_DISABLED );

	SetMenu( &menu );

	ClientToScreen( &point );
	menu.TrackPopupMenu( TPM_LEFTALIGN, point.x, point.y, this );

	menu.DestroyMenu();

	//CRichEditCtrl::OnRButtonUp(nFlags, point);
}


void CRichEditCtrlEx::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CRichEditCtrl::OnRButtonDblClk(nFlags, point);
}

void CRichEditCtrlEx::ClearAll()
{
	//SetSel( 0, -1 );
	//ReplaceSel( "" );
	SetWindowText(_T("") );
}

void CRichEditCtrlEx::ToggleShowLog()
{
	m_show_log = !m_show_log;

	if ( m_show_log )
	{
		AppendToLog(_T("\n") );
		AppendToLog(_T("로그를 디스플레이합니다."), GetComplementaryColor( m_crBack ) );
	}
	else
	{
		m_show_log = true;
		AppendToLog(_T("\n") );
		AppendToLog(_T("로그 디스플레이 옵션을 해제하였습니다."), GetComplementaryColor( m_crBack ) );
		AppendToLog(_T("로그를 디스플레이 하려면 오른쪽 버튼을 누른 후 \"Display logs\" 옵션을 선택하세요."), GetComplementaryColor( m_crBack ) );
		m_show_log = false;
	}
}

void CRichEditCtrlEx::ToggleShowTime()
{
	m_show_time = !m_show_time;
	AfxGetApp()->WriteProfileInt(_T("setting\\rich_edit"), _T("show time"), m_show_time);
}

void CRichEditCtrlEx::OnPopupMenu(UINT menuID)
{
	switch (menuID)
	{
		case id_menu_richedit_toggle_log :
			ToggleShowLog();
			break;
		case id_menu_richedit_clearl_log:
			ClearAll();
			break;
		case id_menu_richedit_toggle_time:
			ToggleShowTime();
			break;
		case id_menu_richedit_line_space10 :
		case id_menu_richedit_line_space15 :
		case id_menu_richedit_line_space20 :
			SetLineSpacing(menuID - id_menu_richedit_line_space10);
			break;
	}
}

void CRichEditCtrlEx::SetLineSpacing( UINT nLineSpace )
{
	PARAFORMAT2	paraFormat;

	GetParaFormat( paraFormat );
	paraFormat.dwMask = PFM_LINESPACING;
	paraFormat.bLineSpacingRule = (BYTE)nLineSpace;		//줄간격. 0=1.0, 1=1.5, 2=2.0
	
	SetSel( 0, -1 );
	SetParaFormat( paraFormat );
	SetSel( -1, -1 );
}

UINT CRichEditCtrlEx::GetLineSpacing()
{
	PARAFORMAT2	paraFormat;

	GetParaFormat( paraFormat );

	return paraFormat.bLineSpacingRule;
}

void CRichEditCtrlEx::SetBackColor( bool bSysColor, COLORREF crBack )
{
	SetBackgroundColor( bSysColor, crBack );

	m_crBack = crBack;
}

void CRichEditCtrlEx::SetClearLogInterval( int nInterval )
{
	KillTimer( TIMER_CLEAR_LOG );

	if ( nInterval > 0 )
	{
		m_nClearLogInterval = nInterval;
		SetTimer( TIMER_CLEAR_LOG, m_nClearLogInterval * 1000, NULL );
	}
}

void CRichEditCtrlEx::OnTimer(UINT_PTR nIDEvent)
{
	if ( nIDEvent == TIMER_CLEAR_LOG )
	{
		ClearAll();
		AppendToLog(_T("로그를 주기적으로 Clear 합니다."), GetComplementaryColor( m_crBack ) );
	}
}

COLORREF CRichEditCtrlEx::GetComplementaryColor( COLORREF crColor )
{
	int	r = 255 - GetRValue( crColor );
	int	g = 255 - GetGValue( crColor );
	int	b = 255 - GetBValue( crColor );

/*
	Clamp( r, 0, 255 );
	Clamp( g, 0, 255 );
	Clamp( b, 0, 255 );
*/

	return RGB( r, g, b );
}


void CRichEditCtrlEx::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// 이 기능을 사용하려면 Windows Vista 이상이 있어야 합니다.
	// _WIN32_WINNT 기호는 0x0600보다 크거나 같아야 합니다.
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TRACE( "wheel on richedit omh: %d\n", zDelta );
	CRichEditCtrl::OnMouseHWheel(nFlags, zDelta, pt);
}


BOOL CRichEditCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if ( pMsg->message == WM_MOUSEWHEEL )
	{
		//TRACE( "wheel on richedit ptm: %d\n", pMsg->wParam );
		if ( (int)(pMsg->wParam) > 0 )
			LineScroll( -m_nScrollSize );
		else
			LineScroll( m_nScrollSize );
		return true;
	}

	return CRichEditCtrl::PreTranslateMessage(pMsg);
}


void CRichEditCtrlEx::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	CFont* font = GetFont();
	if (font == NULL)
		font = GetParent()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	CRichEditCtrl::PreSubclassWindow();
}

void CRichEditCtrlEx::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	SetFont( &m_font, true );

	ASSERT(bCreated);
}

CRichEditCtrlEx& CRichEditCtrlEx::SetFontName( TCHAR *sfontname )
{
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sfontname);
	ReconstructFont();

	return *this;
}

CRichEditCtrlEx& CRichEditCtrlEx::SetFontSize( int nSize )
{
	HDC hDC = GetDC()->GetSafeHdc();
	m_lf.lfHeight = -MulDiv(nSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	::ReleaseDC(m_hWnd, hDC);
	ReconstructFont();

	return *this;
}

CRichEditCtrlEx& CRichEditCtrlEx::SetFontBold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	ReconstructFont();

	return *this;
}

void CRichEditCtrlEx::set_align(int align)
{
	m_align = align;
}
