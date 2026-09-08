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

	//20260908 by claude. 붙이기 *전* 의 상태를 기록해 둔다. 반드시 첫 SetSel / ReplaceSel 보다 앞이어야 한다 —
	//그 호출들이 캐럿을 끝으로 옮기고 화면도 함께 끌고 간다.
	const int	old_first_line = GetFirstVisibleLine();

	long	old_sel_min = 0;
	long	old_sel_max = 0;
	GetSel(old_sel_min, old_sel_max);

	//따라갈지는 이미 정해져 있다(선언부 참조). 여기서 스크롤 위치를 다시 재서 판단하지 않는다.
	const bool	follow = is_auto_scrolling();

	//따라가지 않는 중이면 아래에서 사용자가 찍어 둔 캐럿·선택을 그대로 되돌린다.
	//이 함수는 글자를 넣으려고 SetSel 로 캐럿을 끝으로 옮기므로, 되돌리지 않으면 클릭한 자리가 매번 사라진다.
	//Ctrl+End 는 이 복원에 걸리지 않는다 — PreTranslateMessage 가 키를 받는 즉시 따라가기를 켜기 때문이다.

	//글자를 넣는 동안 SetSel 이 화면을 캐럿(맨 끝) 쪽으로 끌어온다. 따라가지 않는 중이면 그 뒤에 원래 자리로
	//되돌리므로 화면이 오가며 심하게 깜빡인다. 그래서 그때만 그리기를 묶는다.
	//
	//따라가는 중에는 묶지 않는다. 중간 이동의 목적지가 어차피 최종 목적지(맨 끝)와 같아 왕복이 없고,
	//무엇보다 SetRedraw 를 끄면 컨트롤이 캐럿을 감추는데 다시 켠다고 저절로 보여주지 않는다.
	//로그가 계속 들어오면 캐럿이 매번 지워져 깜빡이지 않는 것처럼 보인다(2026-09-08 실측).
	if (!follow)
		SetRedraw(FALSE);

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

	//스크롤은 반드시 그리기를 다시 켠 뒤에 한다. SetRedraw(FALSE) 동안에는 컨트롤이 스크롤 위치를
	//갱신하지 않아 여기서 보낸 스크롤 명령이 통째로 무시된다(2026-09-08 실측).
	if (!follow)
		SetRedraw(TRUE);

	//아래 스크롤은 우리가 하는 것이므로 OnVScroll 이 m_at_bottom 을 다시 정하지 않게 막는다.
	m_in_programmatic_scroll = true;

	if (follow)
	{
		scroll_to_bottom();
	}
	else
	{
		const int new_first_line = GetFirstVisibleLine();

		if (old_first_line != new_first_line)
			LineScroll(old_first_line - new_first_line);

		//20260908 by claude. 이 Invalidate() 는 반드시 있어야 한다.
		//위 LineScroll 은 화면 픽셀을 비트블릿으로 옮기는데, SetRedraw(FALSE) 동안 갱신되지 않은
		//낡은 픽셀이 그대로 옮겨져 같은 줄이 여러 번 찍힌다(2026-09-08 실측 — 500ms 로 늦추니 드러났다).
		//대신 이 때문에 50ms 처럼 잦은 삽입에서는 캐럿이 매번 지워져 깜빡이지 못한다. 둘은 이 구조에서
		//동시에 만족시킬 수 없다 — 선택을 건드리지 않고 삽입하는 방식(TOM)으로 가야 풀린다.
		Invalidate();

		//사용자가 찍어 둔 캐럿·선택을 되돌린다(복사하려던 것이 매번 풀리면 안 된다).
		//그리기를 켠 *뒤* 에 해야 감춰졌던 캐럿이 함께 살아난다.
		//캐럿은 방금 되돌린 화면 안에 있으므로 이 SetSel 이 화면을 다시 움직이지는 않는다.
		SetSel(old_sel_min, old_sel_max);
	}

	m_in_programmatic_scroll = false;

	return new_text;
}

//20260908 by claude. 마지막 줄이 화면 맨 아래에 오도록 스크롤한다. *포커스와 무관하게* 동작해야 한다.
//  - EM_SCROLLCARET 은 캐럿을 보이게 하는 명령이다. 포커스가 없으면 캐럿이 없어 아무 일도 하지 않는다(실측).
//    예전 코드의 `if (this != GetFocus())` 도 같은 벽을 반대편에서 만나 우회한 흔적이다.
//  - WM_VSCROLL/SB_BOTTOM 은 최대 스크롤 위치로 가는데, 컨트롤이 내용 끝 뒤에 두는 여유까지
//    함께 내려가 아래에 빈 공간이 남는다.
//  - LineScroll(INT_MAX) 는 마지막 줄을 맨 위로 올린다. 되돌리려면 화면에 들어가는 줄 수가 필요한데
//    GetNumVisibleLines() 는 수용량이 아니라 *현재 표시된 줄 수* 라 내용이 짧으면 덜 되돌아온다.
//그래서 줄 높이를 직접 재서 목표 위치를 계산하고 LineScroll 로 옮긴다. LineScroll 은 캐럿을 쓰지 않는다.
//20260908 by claude. 한 줄 높이(px). 연속한 두 줄의 y 차이로 잰다 — 폰트나 줄간격이 바뀌어도 따라온다.
//화면 밖의 줄이어도 좌표는 나오므로 스크롤 상태와 무관하다. 줄이 둘 미만이면 0.
int CRichEditCtrlEx::get_line_height()
{
	if (GetLineCount() < 2)
		return 0;

	return PosFromChar(LineIndex(1)).y - PosFromChar(LineIndex(0)).y;
}

//20260908 by claude. "맨 아래" 로 봤을 때의 첫 표시 줄 번호.
//스크롤하는 쪽과 판정하는 쪽이 *이 하나의 계산* 을 함께 써야 한다. 그러지 않으면 우리가 맞춰 둔 자리가
//판정에서는 "맨 아래가 아님" 으로 읽혀 Ctrl+End 나 휠로 내려도 재개가 됐다 안 됐다 한다(2026-09-08 실측).
//스크롤바의 픽셀 값(nPos + nPage >= nMax)으로 비교하면 반올림 한두 픽셀에 판정이 뒤집힌다.
int CRichEditCtrlEx::get_bottom_first_line()
{
	const int line_count = GetLineCount();
	const int line_height = get_line_height();

	if (line_count <= 1 || line_height <= 0)
		return 0;

	//addl() 은 텍스트 뒤에 "\n" 을 붙이므로 마지막 줄은 항상 빈 줄이다.
	//그 빈 줄을 화면 아래에 맞추면 한 줄이 통째로 비어 보인다. 글자가 있는 마지막 줄을 기준으로 삼는다.
	int last_line = line_count - 1;

	if (last_line > 0 && LineLength(LineIndex(last_line)) == 0)
		last_line--;

	CRect rc;
	GetClientRect(rc);

	const int visible_lines = rc.Height() / line_height;

	return max(0, last_line + 1 - visible_lines);
}

void CRichEditCtrlEx::scroll_to_bottom()
{
	LineScroll(get_bottom_first_line() - GetFirstVisibleLine());
}

bool CRichEditCtrlEx::is_scrolled_to_bottom()
{
	//20260908 by claude. scroll_to_bottom() 과 같은 계산을 쓴다 — 그래야 우리가 맞춰 둔 자리가
	//반드시 "맨 아래" 로 읽힌다. 스크롤바 픽셀 값으로 비교하던 것을 걷어냈다(get_bottom_first_line 주석 참조).
	//더 아래로 내려간 경우(끝의 빈 줄까지 보이도록)도 맨 아래로 본다.
	return (GetFirstVisibleLine() >= get_bottom_first_line());
}

bool CRichEditCtrlEx::is_caret_on_last_line()
{
	long sel_min = 0;
	long sel_max = 0;
	GetSel(sel_min, sel_max);

	if (sel_min != sel_max)
		return false;

	//끝 위치를 GetWindowTextLength 로 재면 안 된다 — 그쪽은 줄바꿈을 CRLF 두 글자로 세는데
	//선택 위치는 한 글자로 세어, 캐럿이 맨 끝(Ctrl+End)이어도 "중간" 으로 잘못 잡힌다.
	return (LineFromChar(sel_min) >= GetLineCount() - 1);
}

//사용자가 스크롤이나 캐럿을 움직인 직후에만 부른다.
//기준은 *스크롤 위치* 다 — 맨 아래로 내려오면 다시 따라간다(VS 출력 창과 같다).
//
//check_caret 은 클릭에서만 켠다. 클릭은 캐럿을 옮기므로 "중간 줄을 찍어 두면 멈춘다" 를 여기서 판단한다.
//스크롤바·휠·PgUp/PgDn 은 캐럿을 움직이지 않으므로 캐럿까지 따지면 안 된다 — 끝까지 내려도
//캐럿이 옛 줄에 남아 있어 영영 재개되지 않는다(2026-09-08 실측).
void CRichEditCtrlEx::update_at_bottom(bool check_caret)
{
	m_at_bottom = is_scrolled_to_bottom() && (!check_caret || is_caret_on_last_line());

	//20260908 by claude. 리치에디트는 내용 끝을 지나쳐 스크롤하는 것을 허용한다(메모장과 같다).
	//휠을 빠르게 굴려 끝을 넘어가면 그만큼 아래가 빈다. 맨 아래에 닿은 순간 정확한 자리로 되돌린다.
	//위로 올라가는 중에는 is_scrolled_to_bottom() 이 거짓이라 여기 걸리지 않는다.
	if (m_at_bottom && GetFirstVisibleLine() > get_bottom_first_line())
		scroll_to_bottom();
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

	//20260908 by claude. 실제로 따라가고 있는지를 그대로 보여준다. 위로 스크롤해 멈춘 상태에서
	//체크만 켜져 있으면 "체크돼 있는데 왜 안 따라가지" 가 된다.
	menu.CheckMenuItem(id_menu_richedit_auto_scroll, is_auto_scrolling() ? MF_CHECKED : MF_UNCHECKED);
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
			//20260908 by claude. 메뉴는 *실제로 따라가고 있는지* 를 켜고 끈다.
			//켤 때는 위로 스크롤해 둔 상태도 함께 풀고 즉시 맨 아래로 내려간다 — 안 그러면 체크만 되고
			//화면은 그대로여서 켜진 것으로 보이지 않는다.
			//끌 때는 주 스위치를 내린다. 그러면 맨 아래로 스크롤하거나 캐럿을 끝에 두어도 따라가지 않는다.
			if (is_auto_scrolling())
			{
				m_use_auto_scroll = false;
			}
			else
			{
				m_use_auto_scroll = true;
				m_at_bottom = true;
				scroll_to_bottom();
			}
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
	//m_use_auto_scroll 은 사용자가 우클릭 메뉴나 use_auto_scroll() 로 끄는 주 스위치로만 남는다.
	//20260908 by claude. RichEdit 2.0 은 Ctrl+V 를 내부에서 처리해 WM_PASTE 를 자기 자신에게 보내지 않는다.
	//붙여넣기 처리를 on_paste 한 곳에 모으기 위해 키 입력을 여기서 WM_PASTE 로 바꿔 보낸다.
	else if (pMsg->message == WM_KEYDOWN
		&& ((pMsg->wParam == 'V' && (::GetKeyState(VK_CONTROL) & 0x8000))
		 || (pMsg->wParam == VK_INSERT && (::GetKeyState(VK_SHIFT) & 0x8000))))
	{
		SendMessage(WM_PASTE);
		return TRUE;
	}
	//20260908 by claude. Ctrl+End 는 다른 앱에서도 "맨 끝으로" 인 공용키다. 누르는 즉시 따라가기를 켠다.
	//키를 소비하지는 않는다 — 캐럿·화면을 끝으로 옮기는 일은 컨트롤이 그대로 한다.
	//OnKeyUp 까지 미루면 안 된다. 그 사이에 들어온 add() 가 (아직 따라가지 않는 상태라)
	//캐럿과 화면을 원래 자리로 되돌려 버려, 정작 OnKeyUp 은 "맨 아래가 아니다" 로 읽는다.
	else if (pMsg->message == WM_KEYDOWN
		&& pMsg->wParam == VK_END
		&& (::GetKeyState(VK_CONTROL) & 0x8000))
	{
		m_at_bottom = true;
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

	//20260908 by claude. 사용자가 스크롤바를 움직였을 때만 다시 정한다.
	//add() 가 보내는 SB_BOTTOM 도 SendMessage 라 이 핸들러를 거치는데, 그것까지 반영하면
	//스크롤 범위가 아직 잡히지 않은 첫 add 에서 따라가기가 꺼져 버리고 다시는 켜지지 않는다(실측).
	if (!m_in_programmatic_scroll)
		update_at_bottom();
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

	const BOOL result = CRichEditCtrl::OnMouseWheel(nFlags, zDelta, pt);

	//20260908 by claude. 휠로 위를 보면 멈추고, 다시 맨 아래로 내려오면 따라간다.
	update_at_bottom();

	return result;
}

void CRichEditCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//Trace_func();
	//20260908 by claude. 클릭만으로 끄지 않는다. 클릭이 끝난 뒤(OnLButtonUp) 캐럿 위치를 보고 정한다.
	CRichEditCtrl::OnLButtonDown(nFlags, point);
}

void CRichEditCtrlEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	highlight_current_line();

	//20260908 by claude. 클릭이 끝나면 캐럿이 옮겨져 있다. 마지막 줄을 찍었으면 다시 따라가고,
	//중간 줄을 찍었으면 화면이 맨 아래여도 멈춘다. 캐럿을 보는 것은 이 경로뿐이다.
	update_at_bottom(true);

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

	//20260908 by claude. 키로도 화면과 캐럿이 움직인다(Ctrl+End, PgUp/PgDn, 방향키).
	update_at_bottom();

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
