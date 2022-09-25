// RichEditCtrlEx.cpp : 구현 파일입니다.
//

//#include "stdafx.h"
#include "RichEditCtrlEx.h"

#include <strsafe.h>	//for StringCchCopyN

#define ID_MENU_RICHEDIT_TOGGLE_LOG		9900
#define ID_MENU_RICHEDIT_CLEARL_LOG		9901
#define ID_MENU_RICHEDIT_LINE_SPACE10	9902
#define ID_MENU_RICHEDIT_LINE_SPACE15	9903
#define ID_MENU_RICHEDIT_LINE_SPACE20	9904
#define ID_MENU_RICHEDIT_TOGGLE_TIME	9905

#define TIMER_CLEAR_LOG					0

// CRichEditCtrlEx

IMPLEMENT_DYNAMIC(CRichEditCtrlEx, CRichEditCtrl)

CRichEditCtrlEx::CRichEditCtrlEx()
{
	m_crText			= ::GetSysColor(COLOR_GRAYTEXT);
	m_crBack			= RGB( 255, 255, 255 );
	m_bShowLog			= true;
	m_bShowTime			= true;
	m_nClearLogInterval	= 0;
	m_nMaxCharLimit		= 0;
	m_nScrollSize		= 2;

	memset( &m_lf, 0, sizeof( LOGFONT ) );
}

CRichEditCtrlEx::~CRichEditCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CRichEditCtrlEx, CRichEditCtrl)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_TIMER()
	ON_COMMAND( ID_MENU_RICHEDIT_TOGGLE_LOG, ToggleShowLog )
	ON_COMMAND( ID_MENU_RICHEDIT_CLEARL_LOG, ClearAll )
	ON_COMMAND( ID_MENU_RICHEDIT_TOGGLE_TIME, ToggleShowTime )
	ON_COMMAND_RANGE( ID_MENU_RICHEDIT_LINE_SPACE10, ID_MENU_RICHEDIT_LINE_SPACE20, OnCommandLineSpacing )
	ON_WM_MOUSEHWHEEL()
END_MESSAGE_MAP()



// CRichEditCtrlEx 메시지 처리기입니다.



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
int CRichEditCtrlEx::AppendToLog(CString str, COLORREF color /*= -1*/, BOOL bAddNewLine /*= TRUE*/ )
{
	if ( m_hWnd == NULL )
		return 0;

	if ( !m_bShowLog )
		return 0;

	if ( bAddNewLine && ( str.Right( 1 ) != "\n" ) )
		str += "\n";

	int			nOldLines = 0, nNewLines = 0, nScroll = 0;
	long		nInsertionPoint = 0;
	CHARFORMAT	cf;

	nOldLines = GetLineCount();
	nInsertionPoint = GetWindowTextLength();

	if ( m_hWnd == NULL )
		return 0;

	SetSel(nInsertionPoint, nInsertionPoint);

	if ( m_bShowTime )
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
	
	//텍스트 전체 크기가 특정 크기를 넘어가면 클리어
	if ( m_nMaxCharLimit > 0 && nInsertionPoint >= m_nMaxCharLimit )
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

	SCROLLINFO	scrollInfo;
	scrollInfo.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_VERT, &scrollInfo);
	if (scrollInfo.nPos < scrollInfo.nMax)
		return 0;
/*
	TRACE( "pos = %d, trackpos = %d, max = %d\n",
			scrollInfo.nPos,
			scrollInfo.nTrackPos,
			scrollInfo.nMax );
*/
	// Get new line count
	nNewLines = GetLineCount();

	// Scroll by the number of lines just inserted
   	nScroll	= nNewLines - nOldLines;
   	LineScroll(nScroll);

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

	AppendToLog(szBuffer, m_crText, false);
}

void CRichEditCtrlEx::Append( COLORREF cr, LPCTSTR lpszFormat, ... )
{
	TCHAR szBuffer[512];

	size_t cb = 0;
	va_list args;
	va_start(args, lpszFormat);
	::StringCchVPrintfEx(szBuffer, 512, NULL, &cb, 0, lpszFormat, args);
	va_end(args);

	AppendToLog(szBuffer, cr, false);
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

	if ( !m_bShowLog )
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
	CMenu	menu;

	menu.CreatePopupMenu();

	menu.AppendMenu( MF_STRING, ID_MENU_RICHEDIT_CLEARL_LOG, _T("Clear all logs(&C)") );
	menu.AppendMenu( MF_SEPARATOR);
	menu.AppendMenu( MF_STRING, ID_MENU_RICHEDIT_TOGGLE_LOG, _T("Display logs(&S)") );
	menu.AppendMenu( MF_STRING, ID_MENU_RICHEDIT_TOGGLE_TIME, _T("Display time info(&D)") );
	menu.AppendMenu( MF_SEPARATOR);
	menu.AppendMenu( MF_STRING, ID_MENU_RICHEDIT_LINE_SPACE10, _T("1.0 line space(&1)") );
	menu.AppendMenu( MF_STRING, ID_MENU_RICHEDIT_LINE_SPACE15, _T("1.5 line space(&2)") );
	menu.AppendMenu( MF_STRING, ID_MENU_RICHEDIT_LINE_SPACE20, _T("2.0 line space(&3)") );

	//현재 라인간격값을 얻어온다.
	PARAFORMAT2	paraFormat;
	GetParaFormat( paraFormat );
	paraFormat.dwMask = PFM_LINESPACING;
	BYTE nLineSpacing = paraFormat.bLineSpacingRule;	//줄간격을 1.5배로 한다. 0=1.0, 1=1.5, 2=2.0

	menu.CheckMenuItem( ID_MENU_RICHEDIT_TOGGLE_LOG, m_bShowLog ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( ID_MENU_RICHEDIT_TOGGLE_TIME, m_bShowTime ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( ID_MENU_RICHEDIT_LINE_SPACE10, nLineSpacing == 0 ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( ID_MENU_RICHEDIT_LINE_SPACE15, nLineSpacing == 1 ? MF_CHECKED : MF_UNCHECKED );
	menu.CheckMenuItem( ID_MENU_RICHEDIT_LINE_SPACE20, nLineSpacing == 2 ? MF_CHECKED : MF_UNCHECKED );

	menu.EnableMenuItem( ID_MENU_RICHEDIT_TOGGLE_TIME, m_bShowLog ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( ID_MENU_RICHEDIT_LINE_SPACE10, m_bShowLog ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( ID_MENU_RICHEDIT_LINE_SPACE15, m_bShowLog ? MF_ENABLED : MF_DISABLED );
	menu.EnableMenuItem( ID_MENU_RICHEDIT_LINE_SPACE20, m_bShowLog ? MF_ENABLED : MF_DISABLED );

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
	m_bShowLog = !m_bShowLog;

	if ( m_bShowLog )
	{
		AppendToLog(_T("\n") );
		AppendToLog(_T("로그를 디스플레이합니다."), GetComplementaryColor( m_crBack ) );
	}
	else
	{
		m_bShowLog = true;
		AppendToLog(_T("\n") );
		AppendToLog(_T("로그 디스플레이 옵션을 해제하였습니다."), GetComplementaryColor( m_crBack ) );
		AppendToLog(_T("로그를 디스플레이 하려면 오른쪽 버튼을 누른 후 \"Display logs\" 옵션을 선택하세요."), GetComplementaryColor( m_crBack ) );
		m_bShowLog = false;
	}
}

void CRichEditCtrlEx::ToggleShowTime()
{
	m_bShowTime = !m_bShowTime;
}

void CRichEditCtrlEx::OnCommandLineSpacing( UINT nLineSpace )
{
	SetLineSpacing( nLineSpace - ID_MENU_RICHEDIT_LINE_SPACE10 );
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
