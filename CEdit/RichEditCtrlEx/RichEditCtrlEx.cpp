// RichEditCtrlEx.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "RichEditCtrlEx.h"
#include "../../Functions.h"

//#include <stdarg.h>
#include <strsafe.h>	//for StringCchCopyN

#define TIMER_CLEAR_LOG					0

// CRichEditCtrlEx

IMPLEMENT_DYNAMIC(CRichEditCtrlEx, CRichEditCtrl)

CRichEditCtrlEx::CRichEditCtrlEx()
{
	m_crText			= ::GetSysColor(COLOR_GRAYTEXT);
	m_crBack			= RGB(255, 255, 255);
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
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	//ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//ON_WM_KEYDOWN()
	//ON_WM_PAINT()
	//ON_WM_ERASEBKGND()
	ON_WM_KEYUP()
	ON_NOTIFY_REFLECT(EN_SELCHANGE, &CRichEditCtrlEx::OnEnSelchange)
END_MESSAGE_MAP()



// CRichEditCtrlEx 메시지 처리기입니다.
CString CRichEditCtrlEx::addl(COLORREF cr, LPCTSTR lpszFormat, ...)
{
	//CString으로 변환
	CString new_text;
	va_list args;
	va_start(args, lpszFormat);
	new_text.FormatV(lpszFormat, args);

	return add(cr, new_text + _T("\n"));
}

CString CRichEditCtrlEx::add(COLORREF cr, LPCTSTR lpszFormat, ...)
{
	CString ret;

	if (m_hWnd == NULL)
		return ret;

	if (!m_show_log)
		return ret;

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
	ZeroMemory(&cf, sizeof(cf));

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
		//cf.dwMask = CFM_COLOR;
		//cf.dwEffects = 0;	// To disable CFE_AUTOCOLOR
		cf.crTextColor = RGB(128, 128, 128);
		SetSelectionCharFormat(cf);

		ReplaceSel(sTime);
	}

	// Save number of lines before insertion of new text
	nOldLines = GetLineCount();

	// Initialize character format structure
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT;

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

	//텍스트를 추가하고
	nInsertionPoint = GetWindowTextLength();
	SetSel(nInsertionPoint, nInsertionPoint);
	SetSelectionCharFormat(cf);
	ReplaceSel(new_text);

	//조건에 따라 텍스트 일부 색상을 변경한다.
	//라인이 추가되면서 pos의 위치가 하나씩 밀리는 현상이 왜 발생하는지 모르겠으나
	//total_lines 변수를 이용해서 보정한다.

	int total_lines = GetLineCount() - 2;

	//우선 add()함수에서 이를 for루프로 처리하고 있으나 속도를 개선하기 위해 thread로 분리시켜야 한다.
	for (int i = 0; i < m_keyword_formats.size(); i++)
	{
		std::deque<int> results;
		find_all(results, new_text, m_keyword_formats[i].keyword, false, true);

		for (int j = 0; j < results.size(); j++)
		{
			int pos = results[j];
			if (pos >= 0)
			{
				cf.crTextColor = m_keyword_formats[i].cr;
				cf.dwEffects = 0;
				if (m_keyword_formats[i].bold)
					cf.dwEffects |= CFE_BOLD;
				if (m_keyword_formats[i].italic)
					cf.dwEffects |= CFE_ITALIC;
				if (m_keyword_formats[i].underline)
					cf.dwEffects |= CFE_UNDERLINE;
				if (m_keyword_formats[i].strikeout)
					cf.dwEffects |= CFE_STRIKEOUT;
				SetSel(nInsertionPoint + pos - total_lines, nInsertionPoint + pos - total_lines + m_keyword_formats[i].keyword.GetLength());
				SetSelectionCharFormat(cf);
			}
		}
	}

	/*
	CString find_str = _T("][");
	int pos = new_text.Find(find_str);
	if (pos >= 0)
	{
		cf.crTextColor = RGB(255, 0, 0);
		SetSel(nInsertionPoint + pos - total_lines, nInsertionPoint + pos - total_lines + find_str.GetLength());
		SetSelectionCharFormat(cf);
	}

	cf.crTextColor = m_crText;
	if (pos >= 0)
		SetSel(nInsertionPoint + pos - total_lines + find_str.GetLength(), GetWindowTextLength());
	else
		SetSel(nInsertionPoint, GetWindowTextLength());
	SetSelectionCharFormat(cf);
	*/

	// Replace selection. Because we have nothing selected, this will simply insert
	// the string at the current caret position.
	//SetRedraw(FALSE);
	//SetRedraw(TRUE);

#ifdef _DEBUG
	//TRACE(new_text);
#endif

	//여기서부터의 코드는 맨 마지막 라인으로 스크롤시킬지 말지를 결정.
	//스크롤 위치가 맨 끝이면 항상 마지막 라인으로 자동 스크롤하고
	//스크롤 위치가 중간이면 자동 스크롤시키지 않도록 구현하려 했으나 아직 미완성.
	/*
	TRACE(_T("m_auto_scroll = %d, pos = %d, trackpos = %d, page = %d, min = %d, max = %d, sel = %d, %d\n"),
		m_auto_scroll,
		si.nPos,
		si.nTrackPos,
		si.nPage,
		si.nMin,
		si.nMax,
		lMinSel, lMaxSel);
	*/
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
#if 1
	if (!m_auto_scroll)
		return new_text;

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

	return new_text;

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

	return new_text;
}

//한줄씩 deque에 저장된 내용을 모두 합쳐서 rich의 내용을 update한다.
void CRichEditCtrlEx::set_text(std::deque<CString>* dqlist)
{
	if (!dqlist)
		return;

	SetRedraw(FALSE);

	ClearAll();

	//str으로 합쳐서 한번에 넣으면 좋지만 간혹 dqlist의 합이 64K를 넘으면 CString에 담을 수 없다.
	//한줄씩 넣어야 한다.
	//CString str;

	for (int i = 0; i < dqlist->size(); i++)
		add(-1, dqlist->at(i));

	SetRedraw(TRUE);
	Invalidate();
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
int CRichEditCtrlEx::AppendToLog(CString str, COLORREF color /*= -1*/, BOOL bAddNewLine /*= TRUE*/)
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
	if (bAddNewLine)// && (str.Right(1) != "\n"))
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

		::GetLocalTime(&t);
		sTime.Format(_T("%d-%02d-%02d %02d:%02d:%02d(%03d) "), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

		cf.cbSize		= sizeof(CHARFORMAT);
		cf.dwMask		= CFM_COLOR;
		cf.dwEffects	= 0;	// To disable CFE_AUTOCOLOR
		cf.crTextColor	= RGB(128, 128, 128);
		SetSelectionCharFormat(cf);

		ReplaceSel(sTime);
	}

	// Save number of lines before insertion of new text
	nOldLines		= GetLineCount();

	// Initialize character format structure
	cf.cbSize		= sizeof(CHARFORMAT);
	cf.dwMask		= CFM_COLOR;
	cf.dwEffects	= 0;	// To disable CFE_AUTOCOLOR

	if (color == -1)
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
		ReplaceSel(_T(""));
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

void CRichEditCtrlEx::Append(LPCTSTR lpszFormat, ...)
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
	if (m_hWnd == NULL)
		return 0;

	if (!m_show_log)
		return 0;

	long nVisible = 0;
	long nInsertionPoint = 0;
	CHARFORMAT cf;
	
	if (bAddNewLine && (str.Right(1) != "\n"))
		str += _T("\n");

	// Initialize character format structure
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0; // To disable CFE_AUTOCOLOR

	if (color == -1)
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


	if (this == GetFocus())
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
	{
		CRichEditCtrl::OnRButtonUp(nFlags, point);
		return;
	}

	CMenu	menu;

	menu.CreatePopupMenu();

	menu.AppendMenu(MF_STRING, id_menu_richedit_clearl_log, _T("Clear all logs(&C)"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, id_menu_richedit_toggle_log, _T("Display logs(&S)"));
	menu.AppendMenu(MF_STRING, id_menu_richedit_toggle_time, _T("Display time info(&D)"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, id_menu_richedit_line_space10, _T("1.0 line space(&1)"));
	menu.AppendMenu(MF_STRING, id_menu_richedit_line_space15, _T("1.5 line space(&2)"));
	menu.AppendMenu(MF_STRING, id_menu_richedit_line_space20, _T("2.0 line space(&3)"));

	//현재 라인간격값을 얻어온다.
	PARAFORMAT2	paraFormat;
	GetParaFormat(paraFormat);
	paraFormat.dwMask = PFM_LINESPACING;
	BYTE nLineSpacing = paraFormat.bLineSpacingRule;	//줄간격을 1.5배로 한다. 0=1.0, 1=1.5, 2=2.0

	menu.CheckMenuItem(id_menu_richedit_toggle_log, m_show_log ? MF_CHECKED : MF_UNCHECKED);
	menu.CheckMenuItem(id_menu_richedit_toggle_time, m_show_time ? MF_CHECKED : MF_UNCHECKED);
	menu.CheckMenuItem(id_menu_richedit_line_space10, nLineSpacing == 0 ? MF_CHECKED : MF_UNCHECKED);
	menu.CheckMenuItem(id_menu_richedit_line_space15, nLineSpacing == 1 ? MF_CHECKED : MF_UNCHECKED);
	menu.CheckMenuItem(id_menu_richedit_line_space20, nLineSpacing == 2 ? MF_CHECKED : MF_UNCHECKED);

	menu.EnableMenuItem(id_menu_richedit_toggle_time, m_show_log ? MF_ENABLED : MF_DISABLED);
	menu.EnableMenuItem(id_menu_richedit_line_space10, m_show_log ? MF_ENABLED : MF_DISABLED);
	menu.EnableMenuItem(id_menu_richedit_line_space15, m_show_log ? MF_ENABLED : MF_DISABLED);
	menu.EnableMenuItem(id_menu_richedit_line_space20, m_show_log ? MF_ENABLED : MF_DISABLED);

	SetMenu(&menu);

	ClientToScreen(&point);
	menu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);

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
	//SetSel(0, -1);
	//ReplaceSel("");
	SetWindowText(_T(""));
}

void CRichEditCtrlEx::ToggleShowLog()
{
	m_show_log = !m_show_log;

	if (m_show_log)
	{
		AppendToLog(_T("\n"));
		AppendToLog(_T("로그를 디스플레이합니다."), GetComplementaryColor(m_crBack));
	}
	else
	{
		m_show_log = true;
		AppendToLog(_T("\n"));
		AppendToLog(_T("로그 디스플레이 옵션을 해제하였습니다."), GetComplementaryColor(m_crBack));
		AppendToLog(_T("로그를 디스플레이 하려면 오른쪽 버튼을 누른 후 \"Display logs\" 옵션을 선택하세요."), GetComplementaryColor(m_crBack));
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

//줄간격. 0=1줄, 1=1.5줄, 2=2.0줄
void CRichEditCtrlEx::SetLineSpacing(UINT nLineSpace)
{
	PARAFORMAT2	paraFormat;

	GetParaFormat(paraFormat);
	paraFormat.dwMask = PFM_LINESPACING;
	paraFormat.bLineSpacingRule = (BYTE)nLineSpace;		//줄간격. 0=1.0, 1=1.5, 2=2.0
	
	SetSel(0, -1);
	SetParaFormat(paraFormat);
	SetSel(-1, -1);
}

UINT CRichEditCtrlEx::GetLineSpacing()
{
	PARAFORMAT2	paraFormat;

	GetParaFormat(paraFormat);

	return paraFormat.bLineSpacingRule;
}

void CRichEditCtrlEx::SetBackColor(COLORREF crBack)
{
	//bSysColor = false일때만 crBack값이 유효하다.
	SetBackgroundColor(false, crBack);

	m_crBack = crBack;
}

void CRichEditCtrlEx::SetClearLogInterval(int nInterval)
{
	KillTimer(TIMER_CLEAR_LOG);

	if (nInterval > 0)
	{
		m_nClearLogInterval = nInterval;
		SetTimer(TIMER_CLEAR_LOG, m_nClearLogInterval * 1000, NULL);
	}
}

void CRichEditCtrlEx::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_CLEAR_LOG)
	{
		ClearAll();
		AppendToLog(_T("로그를 주기적으로 Clear 합니다."), GetComplementaryColor(m_crBack));
	}
}

COLORREF CRichEditCtrlEx::GetComplementaryColor(COLORREF crColor)
{
	int	r = 255 - GetRValue(crColor);
	int	g = 255 - GetGValue(crColor);
	int	b = 255 - GetBValue(crColor);

/*
	Clamp(r, 0, 255);
	Clamp(g, 0, 255);
	Clamp(b, 0, 255);
*/

	return RGB(r, g, b);
}

BOOL CRichEditCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_MOUSEWHEEL)
	{
		//TRACE("wheel on richedit ptm: %d\n", pMsg->wParam);
		if ((int)(pMsg->wParam) > 0)
			LineScroll(-m_nScrollSize);
		else
			LineScroll(m_nScrollSize);
		//TRACE(_T("CRichEditCtrlEx::PreTranslateMessage(). WM_MOUSEWHEEL\n"));
		//GetParent()->SendMessage(Message_CRichEditCtrlEx, (WPARAM)&CRichEditCtrlExMessage(this, WM_MOUSEWHEEL), 0);
		return FALSE;
	}
	else if (pMsg->message == WM_MOUSEHWHEEL)
	{
		//TRACE(_T("CRichEditCtrlEx::PreTranslateMessage(). WM_MOUSEHWHEEL\n"));
		//GetParent()->SendMessage(Message_CRichEditCtrlEx, (WPARAM)&CRichEditCtrlExMessage(this, WM_MOUSEHWHEEL), 0);
		return FALSE;
	}
	else if (pMsg->message == WM_HSCROLL)
	{
		//TRACE(_T("CRichEditCtrlEx::PreTranslateMessage(). WM_HSCROLL\n"));
		//GetParent()->SendMessage(Message_CRichEditCtrlEx, (WPARAM)&CRichEditCtrlExMessage(this, WM_HSCROLL), 0);
		return FALSE;
	}
	else if (pMsg->message == WM_VSCROLL)
	{
		//TRACE(_T("CRichEditCtrlEx::PreTranslateMessage(). WM_VSCROLL\n"));
		//GetParent()->SendMessage(Message_CRichEditCtrlEx, (WPARAM)&CRichEditCtrlExMessage(this, WM_VSCROLL), 0);
		return FALSE;
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

void CRichEditCtrlEx::reconstruct_font()
{
	m_font.DeleteObject();
	m_lf.lfCharSet = DEFAULT_CHARSET;
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	CRichEditCtrl::SetFont(&m_font, true);

	ASSERT(bCreated);
}

void CRichEditCtrlEx::SetFont(CFont* font, BOOL bRedraw)
{
	font->GetObject(sizeof(m_lf), &m_lf);
	reconstruct_font();
}

void CRichEditCtrlEx::set_font_name(TCHAR *sfontname)
{
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sfontname);
	reconstruct_font();
}

void CRichEditCtrlEx::set_font_size(int nSize)
{
	m_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, nSize);
	reconstruct_font();
}

void CRichEditCtrlEx::set_font_weight(int weight)
{
	m_lf.lfWeight = weight;
	reconstruct_font();
}

void CRichEditCtrlEx::set_align(int align)
{
	m_align = align;
}

bool CRichEditCtrlEx::load(CString path)
{
	ClearAll();

	CString str = read(path);
	add(-1, str);

	return true;
}

bool CRichEditCtrlEx::save(CString path)
{
	CString str;
	GetWindowText(str);

	str.Replace(_T("\r\n"), _T("\n"));

	return ::save(path, str);
}

void CRichEditCtrlEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//GetParent()->SendMessage(WM_HSCROLL, MAKEWPARAM(nSBCode, nPos), (LPARAM)pScrollBar->GetSafeHwnd());
	//GetParent()->SendMessage(Message_CRichEditCtrlEx, (WPARAM)&CRichEditCtrlExMessage(this, WM_HSCROLL), 0);
	CRichEditCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRichEditCtrlEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//GetParent()->SendMessage(WM_VSCROLL, MAKEWPARAM(nSBCode, nPos), (LPARAM)pScrollBar->GetSafeHwnd());
	//GetParent()->SendMessage(Message_CRichEditCtrlEx, (WPARAM)&CRichEditCtrlExMessage(this, WM_VSCROLL), 0);
	CRichEditCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CRichEditCtrlEx::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// 이 기능을 사용하려면 Windows Vista 이상이 있어야 합니다.
	// _WIN32_WINNT 기호는 0x0600보다 크거나 같아야 합니다.
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TRACE("wheel on richedit omh: %d\n", zDelta);
	CRichEditCtrl::OnMouseHWheel(nFlags, zDelta, pt);
}

BOOL CRichEditCtrlEx::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//GetParent()->SendMessage(WM_VSCROLL, MAKEWPARAM(nSBCode, nPos), (LPARAM)pScrollBar->GetSafeHwnd());

	return CRichEditCtrl::OnMouseWheel(nFlags, zDelta, pt);
}

void CRichEditCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//Trace_func();
	//CPoint pt = GetCaretPos();
	//TRACE(_T("%d, %d\n"), pt.x, pt.y);


	CRichEditCtrl::OnLButtonDown(nFlags, point);
}

void CRichEditCtrlEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	highlight_current_line();

	CRichEditCtrl::OnLButtonUp(nFlags, point);
}

void CRichEditCtrlEx::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CRichEditCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CRichEditCtrlEx::OnPaint()
{
	CRichEditCtrl::OnPaint();

	/*
	CPaintDC dc(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CRichEditCtrl::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	GetClientRect(rc);

	dc.FillSolidRect(0, 0, 100, 100, red);
	*/
}

BOOL CRichEditCtrlEx::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return CRichEditCtrl::OnEraseBkgnd(pDC);
}

void CRichEditCtrlEx::highlight_current_line()
{
	return;
	int line = LineFromChar(LineIndex(-1));
	int begin = LineIndex(line);
	int length = LineLength(begin);
	TRACE(_T("line = %d, begin = %d, length = %d\n"), line, begin, length);

	SetRedraw(FALSE);

	//원래색으로 복원하고
	//SetSel(0, -1);	//이럴 경우 맨위로 스크롤되는	문제가 있다.

	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR | CFM_BACKCOLOR;
	//cf.crBackColor = m_crBack;
	//cf.dwEffects &= ~CFE_AUTOBACKCOLOR;
	//SetSelectionCharFormat(cf);


	SetSel(begin, begin + length);
	cf.crTextColor = white;
	cf.crBackColor = royalblue;
	cf.dwEffects &= ~CFE_AUTOBACKCOLOR;
	SetSelectionCharFormat(cf);

	SetSel(begin, begin);

	SetRedraw(TRUE);

	Invalidate();
}

void CRichEditCtrlEx::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	highlight_current_line();

	CRichEditCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CRichEditCtrlEx::OnEnSelchange(NMHDR* pNMHDR, LRESULT* pResult)
{
	SELCHANGE* pSelChange = reinterpret_cast<SELCHANGE*>(pNMHDR);
	// TODO:  컨트롤은 IParam 마스크와 OR 연산하여 설정된 ENM_CORRECTTEXT 플래그를 사용하여
	// EM_SETEVENTMASK 메시지를 컨트롤로 보내도록 CRichEditCtrl::OnInitDialog() 함수를 재지정하지 않으면
	// ENM_SELCHANGE가 있는 컨트롤에는 IParam 마스크에 ORed를 플래그합니다.

	// TODO:  여기에 컨트롤 알림 처리기 코드를 추가합니다.

	*pResult = 0;
}

void CRichEditCtrlEx::select_line(int line)
{
	int lineIdx = LineIndex(line);
	int lineLen = LineLength(lineIdx);
	SetSel(lineIdx, lineIdx + lineLen);
}

void CRichEditCtrlEx::add_keyword_format(CSCKeywordFormat kf)
{
	m_keyword_formats.push_back(kf);
}

void CRichEditCtrlEx::clear_keyword_format()
{
	m_keyword_formats.clear();
}
