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
	m_show_log			= true;
	m_show_time			= AfxGetApp()->GetProfileInt(_T("setting\\rich_edit"), _T("show time"), true);
	m_clear_log_interval	= 0;
	m_max_length		= 0;
	m_scroll_size		= 2;

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
	ON_COMMAND_RANGE(id_menu_richedit_auto_scroll, id_menu_richedit_toggle_time, OnPopupMenu)
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
	ON_MESSAGE(WM_PASTE, &CRichEditCtrlEx::on_paste)
END_MESSAGE_MAP()



// CRichEditCtrlEx 메시지 처리기입니다.
CString CRichEditCtrlEx::addl(Gdiplus::Color cr, LPCTSTR lpszFormat, ...)
{
	//CString으로 변환
	CString new_text;
	va_list args;
	va_start(args, lpszFormat);
	new_text.FormatV(lpszFormat, args);

	return add(cr, new_text + _T("\n"));
}

CString CRichEditCtrlEx::add(Gdiplus::Color cr, LPCTSTR lpszFormat, ...)
{
	CString ret;

	if (m_hWnd == nullptr)
		return ret;

	if (!m_show_log)
		return ret;

	if (cr.GetValue() == Gdiplus::Color::Transparent)
		cr = m_theme.cr_text;

	//20260908 by claude. 붙이기 *전* 의 보기 상태를 기록해 둔다. 자동 스크롤 여부를 이 값으로 정한다.
	//반드시 첫 SetSel / ReplaceSel 보다 앞이어야 한다 — 그 호출들이 캐럿을 끝으로 옮기고 화면도 함께 끌고 간다.
	//
	//판단 기준은 캐럿이 아니라 *스크롤 위치* 다(VS 출력 창과 같다). 캐럿은 이 함수가 글자를 넣으면서
	//어차피 끝으로 옮기므로 기준으로 삼을 수 없다. 대신 사용자가 찍어둔 선택 영역은 아래에서 되돌려 준다.
	//리치에디트는 세로 스크롤을 픽셀 단위로 보고한다. 맨 아래에서 nPos + nPage == nMax 다.
	//스크롤바가 없으면(내용이 한 화면보다 짧다) 맨 아래를 보고 있는 것과 같다.
	SCROLLINFO	si = { 0 };
	const bool	was_at_bottom = !GetScrollInfo(SB_VERT, &si) || (si.nPos + (int)si.nPage >= si.nMax);
	const int	old_first_line = GetFirstVisibleLine();

	long	old_sel_min = 0;
	long	old_sel_max = 0;
	GetSel(old_sel_min, old_sel_max);

	//캐럿이 마지막 줄에 있는지. 끝 위치를 GetWindowTextLength 로 재면 안 된다 — 그쪽은 줄바꿈을
	//CRLF 두 글자로 세는데 선택 위치는 한 글자로 세어, 캐럿이 맨 끝(Ctrl+End)이어도 "중간" 으로 잘못 잡힌다.
	//줄 번호로 비교하면 그 차이를 타지 않는다.
	const bool	caret_on_last_line = (old_sel_min == old_sel_max) && (LineFromChar(old_sel_min) >= GetLineCount() - 1);

	//사용자가 중간을 클릭해 캐럿을 옮겨 두었으면 붙인 뒤 그 자리로 되돌린다.
	const bool	restore_selection = m_user_moved_caret && !caret_on_last_line;

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

	CHARFORMAT	cf;
	ZeroMemory(&cf, sizeof(cf));

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
		//cf.dwEffects = 0;	// To disable CFE_AUTOCOLOR
		cf.crTextColor = RGB(128, 128, 128);
		SetSelectionCharFormat(cf);

		ReplaceSel(sTime);
	}

	// Initialize character format structure
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT;

	//cr 은 함수 앞부분에서 Transparent 면 m_theme.cr_text 로 이미 치환되므로 여기선 그대로 변환한다.
	cf.crTextColor = cr.ToCOLORREF();

	//텍스트 전체 크기가 특정 크기를 넘어가면 클리어
	if (m_max_length > 0 && nInsertionPoint >= m_max_length)
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
				cf.crTextColor = m_keyword_formats[i].cr.ToCOLORREF();
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

	cf.crTextColor = m_theme.cr_text;
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

	//20260908 by claude. 자동 스크롤. 기준은 캐럿이 아니라 *붙이기 전의 스크롤 위치* 다(VS 출력 창과 같다).
	//맨 아래를 보고 있었으면 새 줄을 따라가고, 위로 올려 둔 상태였으면 보던 자리를 지킨다.
	//글자를 넣는 과정에서 SetSel / ReplaceSel 이 화면을 끝으로 끌고 가므로 어느 쪽이든 여기서 되돌려야 한다.
	//
	//예전 코드가 어긋났던 이유 둘 —
	//  - 포커스가 있으면(this == GetFocus()) 스크롤하지 않았다. 한 번 클릭하면 그 뒤로 따라가지 않았다.
	//  - LineScroll(INT_MAX) 로 끝을 지나쳐 간 뒤 GetNumVisibleLines() 만큼 되돌아왔는데, 그 함수는
	//    화면 수용량이 아니라 *현재 표시된 줄 수* 라 내용이 짧으면 덜 되돌아와 아래에 빈 공간이 남았다.
	if (restore_selection)
		SetSel(old_sel_min, old_sel_max);

	//캐럿을 위쪽 줄에 찍어 두면 화면이 맨 아래여도 따라가지 않는다(VS 출력 창과 같다).
	//한 번도 캐럿을 옮긴 적이 없으면(m_user_moved_caret == false) 캐럿은 0번에 있으므로 조건에서 뺀다 —
	//그러지 않으면 앱 시작 직후부터 자동 스크롤이 전혀 되지 않는다.
	if (m_auto_scroll && was_at_bottom && (!m_user_moved_caret || caret_on_last_line))
	{
		//SB_BOTTOM 은 끝을 지나치지 않는다 — 마지막 줄이 정확히 아래에 붙는다.
		SendMessage(WM_VSCROLL, SB_BOTTOM);
	}
	else
	{
		const int new_first_line = GetFirstVisibleLine();

		if (old_first_line != new_first_line)
			LineScroll(old_first_line - new_first_line);
	}

	return new_text;
}

//한줄씩 deque에 저장된 내용을 모두 합쳐서 rich의 내용을 update한다.
void CRichEditCtrlEx::set_text(std::deque<CString>* dqlist)
{
	if (!dqlist)
		return;

	SetRedraw(FALSE);

	clear_all();

	//str으로 합쳐서 한번에 넣으면 좋지만 간혹 dqlist의 합이 64K를 넘으면 CString에 담을 수 없다.
	//한줄씩 넣어야 한다.
	//CString str;

	for (int i = 0; i < dqlist->size(); i++)
		add(-1, dqlist->at(i));

	SetRedraw(TRUE);
	Invalidate();
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

	menu.AppendMenu(MF_STRING, id_menu_richedit_auto_scroll, _T("Auto Scroll(&A)"));
	menu.AppendMenu(MF_SEPARATOR);
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

	menu.CheckMenuItem(id_menu_richedit_auto_scroll, m_auto_scroll ? MF_CHECKED : MF_UNCHECKED);
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

void CRichEditCtrlEx::clear_all()
{
	//SetSel(0, -1);
	//ReplaceSel("");
	SetWindowText(_T(""));
}

void CRichEditCtrlEx::toggle_show_log()
{
	m_show_log = !m_show_log;

	if (m_show_log)
	{
		addl(get_complementary_gcolor(m_theme.cr_back), _T("\n로그를 디스플레이합니다."));
	}
	else
	{
		//add() 는 m_show_log 가 false 면 아무 것도 하지 않는다. 이 안내만은 보여야 하므로 잠시 켠다.
		m_show_log = true;
		addl(get_complementary_gcolor(m_theme.cr_back), _T("\n로그 디스플레이 옵션을 해제하였습니다."));
		addl(get_complementary_gcolor(m_theme.cr_back), _T("로그를 디스플레이 하려면 오른쪽 버튼을 누른 후 \"Display logs\" 옵션을 선택하세요."));
		m_show_log = false;
	}
}

void CRichEditCtrlEx::toggle_show_time()
{
	m_show_time = !m_show_time;
	AfxGetApp()->WriteProfileInt(_T("setting\\rich_edit"), _T("show time"), m_show_time);
}

void CRichEditCtrlEx::OnPopupMenu(UINT menuID)
{
	switch (menuID)
	{
		case id_menu_richedit_auto_scroll :
			m_auto_scroll = !m_auto_scroll;
			break;
		case id_menu_richedit_toggle_log :
			toggle_show_log();
			break;
		case id_menu_richedit_clearl_log:
			clear_all();
			break;
		case id_menu_richedit_toggle_time:
			toggle_show_time();
			break;
		case id_menu_richedit_line_space10 :
			set_line_spacing(1.0f);
			break;
		case id_menu_richedit_line_space15 :
			set_line_spacing(1.5f);
			break;
		case id_menu_richedit_line_space20 :
			set_line_spacing(2.0f);
			break;
	}
}

void CRichEditCtrlEx::set_line_spacing(float spacing)
{
	if (spacing <= 0.0f)
		spacing = 1.0f;

	PARAFORMAT2	paraFormat;

	GetParaFormat(paraFormat);
	paraFormat.dwMask = PFM_LINESPACING;

	//20260908 by claude. bLineSpacingRule 5 = "dyLineSpacing / 20 이 줄 수". 20=1.0줄, 30=1.5줄, 40=2.0줄.
	//예전에 쓰던 rule 0/1/2 는 1.0/1.5/2.0 세 가지뿐인데다 인자 값과 의미가 달라(1 을 주면 1.5줄) 혼동이 컸다.
	paraFormat.bLineSpacingRule = 5;
	paraFormat.dyLineSpacing = (LONG)(spacing * 20.0f + 0.5f);

	SetSel(0, -1);
	SetParaFormat(paraFormat);
	SetSel(-1, -1);
}

float CRichEditCtrlEx::get_line_spacing()
{
	PARAFORMAT2	paraFormat;

	GetParaFormat(paraFormat);

	//예전 규약(rule 0/1/2)으로 설정된 문단도 읽을 수 있어야 한다.
	//rule 3/4 는 twips 절대값이라 배수로 환산할 수 없다 — 그때는 1.0 으로 답한다.
	switch (paraFormat.bLineSpacingRule)
	{
		case 0:
			return 1.0f;
		case 1:
			return 1.5f;
		case 2:
			return 2.0f;
		case 5:
			return paraFormat.dyLineSpacing / 20.0f;
	}

	return 1.0f;
}

void CRichEditCtrlEx::set_back_color(Gdiplus::Color cr_back)
{
	//bSysColor = false일때만 crBack값이 유효하다.
	SetBackgroundColor(false, cr_back.ToCOLORREF());

	m_theme.cr_back = cr_back;
}

void CRichEditCtrlEx::set_clear_log_interval(int interval)
{
	KillTimer(TIMER_CLEAR_LOG);

	if (interval > 0)
	{
		m_clear_log_interval = interval;
		SetTimer(TIMER_CLEAR_LOG, m_clear_log_interval * 1000, nullptr);
	}
}

void CRichEditCtrlEx::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_CLEAR_LOG)
	{
		clear_all();
		addl(get_complementary_gcolor(m_theme.cr_back), _T("로그를 주기적으로 Clear 합니다."));
	}
}

BOOL CRichEditCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_MOUSEWHEEL)
	{
		//TRACE("wheel on richedit ptm: %d\n", pMsg->wParam);
		if ((int)(pMsg->wParam) > 0)
			LineScroll(-m_scroll_size);
		else
			LineScroll(m_scroll_size);
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
	//20260908 by claude. 클릭했다고 자동 스크롤을 끄지 않는다. 예전에는 여기서 껐다가 우클릭 메뉴로만
	//되살릴 수 있어, 한 번 클릭한 뒤로는 캐럿이 맨 끝에 있어도 따라가지 않았다.
	//따라갈지 말지는 add() 가 붙이기 직전의 스크롤 위치로 매번 판단한다.
	//m_auto_scroll 은 사용자가 우클릭 메뉴나 set_auto_scroll() 로 끄는 주 스위치로만 남는다.
	//20260908 by claude. RichEdit 2.0 은 Ctrl+V 를 내부에서 처리해 WM_PASTE 를 자기 자신에게 보내지 않는다.
	//붙여넣기 처리를 on_paste 한 곳에 모으기 위해 키 입력을 여기서 WM_PASTE 로 바꿔 보낸다.
	else if (pMsg->message == WM_KEYDOWN
		&& ((pMsg->wParam == 'V' && (::GetKeyState(VK_CONTROL) & 0x8000))
		 || (pMsg->wParam == VK_INSERT && (::GetKeyState(VK_SHIFT) & 0x8000))))
	{
		SendMessage(WM_PASTE);
		return TRUE;
	}

	return CRichEditCtrl::PreTranslateMessage(pMsg);
}

//20260908 by claude. 붙여넣기 서식을 클립보드 형식에 따라 갈라 처리한다.
//  RTF 있음  → 원본 서식 그대로. VS 편집기 등에서 복사한 코드의 색·굵기가 유지된다.
//  텍스트만  → set_default_text_color 로 정한 기본 글자색을 입혀서 넣는다.
//기본 붙여넣기에 맡기지 않고 직접 넣는 이유 — 삽입 지점 서식을 따르는지에 기대지 않기 위해서다.
//add() 가 쓰는 "SetSelectionCharFormat 후 ReplaceSel" 경로를 그대로 재사용한다.
LRESULT CRichEditCtrlEx::on_paste(WPARAM wParam, LPARAM lParam)
{
	//PasteSpecial / ReplaceSel 은 readonly 를 검사하지 않는다. 기본 처리와 같아지도록 여기서 막는다.
	if (GetStyle() & ES_READONLY)
		return 0;

	//"Rich Text Format" 은 RTF 의 표준 클립보드 형식 이름이다. XP 의 RichEdit 2.0 에도 있다.
	static const UINT cf_rtf = ::RegisterClipboardFormat(_T("Rich Text Format"));

	if (cf_rtf != 0 && ::IsClipboardFormatAvailable(cf_rtf))
	{
		PasteSpecial(cf_rtf);
		return 0;
	}

	if (!::IsClipboardFormatAvailable(CF_UNICODETEXT) && !::IsClipboardFormatAvailable(CF_TEXT))
		return 0;

	if (!OpenClipboard())
		return 0;

	CString text;

	//CF_TEXT 만 올려둔 앱이어도 OS 가 CF_UNICODETEXT 로 자동 변환해 준다.
	HANDLE clipboard_data = ::GetClipboardData(CF_UNICODETEXT);

	if (clipboard_data != nullptr)
	{
		LPCWSTR p = (LPCWSTR)::GlobalLock(clipboard_data);

		if (p != nullptr)
		{
			text = p;
			::GlobalUnlock(clipboard_data);
		}
	}

	::CloseClipboard();

	if (text.IsEmpty())
		return 0;

	CHARFORMAT cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;	// To disable CFE_AUTOCOLOR
	cf.crTextColor = m_theme.cr_text.ToCOLORREF();
	SetSelectionCharFormat(cf);

	ReplaceSel(text, TRUE);

	return 0;
}


void CRichEditCtrlEx::PreSubclassWindow()
{
	//세로 스크롤바·자동 스크롤은 리소스에서 빠뜨려도 런타임에 보강한다.
	ModifyStyle(0, ES_AUTOVSCROLL | WS_VSCROLL, SWP_FRAMECHANGED);

	//ES_MULTILINE 은 richedit 가 생성 시점(WM_NCCREATE)에만 읽으므로 ModifyStyle 로 런타임 전환이 안 된다.
	//(서브클래싱 도중 DestroyWindow→재생성하는 우회는 MFC SubclassWindow 의 공유 super-wndproc 를 깨뜨려 불가.)
	//리소스에서 Multiline=True 를 빠뜨리면 addl() 의 개행이 동작하지 않으므로 debug 에서 즉시 알린다.
	ASSERT((GetStyle() & ES_MULTILINE) && "CRichEditCtrlEx: resource control must have Multiline=True for addl() to break lines.");

	CFont* font = GetFont();
	if (font == nullptr)
		font = GetParent()->GetFont();

	if (font != nullptr)
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
	clear_all();

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
	//20260908 by claude. 클릭으로 자동 스크롤을 끄지 않는다 — PreTranslateMessage 의 같은 자리 주석 참조.
	//다만 이 클릭으로 캐럿이 옮겨지므로, 이후로는 add() 가 캐럿 위치도 함께 본다(m_user_moved_caret 선언부).
	m_user_moved_caret = true;

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

	//20260908 by claude. 키로도 캐럿이 움직인다(Ctrl+End, 방향키 등). 클릭과 같은 이유로 표시해 둔다.
	m_user_moved_caret = true;

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

void CRichEditCtrlEx::set_color_theme(int color_theme, bool invalidate)
{
	m_theme.set_color_theme(color_theme);

	SetBackgroundColor(false, m_theme.cr_back.ToCOLORREF());

	if (invalidate && m_hWnd)
		Invalidate();// RedrawWindow();
}
