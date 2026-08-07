#include "SCToolTipCtrl.h"
#include "../../Functions.h"
//20260807 by claude. [진단] 글자 선명도 추적용. 원인 확정되어 주석 처리 — 재조사 시 이 줄과
//on_custom_draw 의 [tooltip] logWrite 만 풀면 된다. (이 모듈을 쓰는 RemoteSDKDemo 등에는
//SCLog.cpp 가 없어 활성화한 채로 두면 pLog 미해결로 링크가 깨진다.)
//#include "../../log/SCLog/SCLog.h"

IMPLEMENT_DYNAMIC(CSCToolTipCtrl, CToolTipCtrl)

BEGIN_MESSAGE_MAP(CSCToolTipCtrl, CToolTipCtrl)
	//툴팁은 자신의 알림을 부모에게 보내므로 파생 클래스에서 받으려면 reflect 를 써야 한다.
	ON_NOTIFY_REFLECT(TTN_SHOW, &CSCToolTipCtrl::on_show)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CSCToolTipCtrl::on_custom_draw)
END_MESSAGE_MAP()

CSCToolTipCtrl::CSCToolTipCtrl()
{
	m_sz_content = CSize(0, 0);

	//기본 폰트는 Segoe UI. 태그로 지정하지 않은 구간에 적용된다.
	_tcscpy_s(m_text_prop.name, _countof(m_text_prop.name), _T("Segoe UI"));
	m_text_prop.size = 9;
	m_text_prop.style = Gdiplus::FontStyleRegular;
}

CSCToolTipCtrl::~CSCToolTipCtrl()
{
}

void CSCToolTipCtrl::set_color_theme(CSCColorTheme theme)
{
	m_theme = theme;
}

void CSCToolTipCtrl::set_padding(int cx, int cy)
{
	m_padding_cx = cx;
	m_padding_cy = cy;
}

void CSCToolTipCtrl::set_max_width(int width)
{
	m_max_width = width;
}

void CSCToolTipCtrl::set_font(LPCTSTR font_name, int font_size)
{
	_tcscpy_s(m_text_prop.name, _countof(m_text_prop.name), font_name);
	m_text_prop.size = font_size;
}

void CSCToolTipCtrl::set_round(int radius)
{
	m_round = radius;
}

void CSCToolTipCtrl::set_line_spacing(float spacing)
{
	m_line_spacing = spacing;
}

void CSCToolTipCtrl::set_auto_font_quality(bool on, int AA_from_pt)
{
	m_auto_font_quality = on;
	m_AA_from_pt = AA_from_pt;
}

void CSCToolTipCtrl::relay_message(MSG* pMsg)
{
	if (m_hWnd == NULL || pMsg == NULL)
		return;

	//20260807 by claude. pMsg 를 직접 고쳐 쓰면 이 함수 이후의 dispatch 가 엉뚱한 hwnd/lParam 으로 진행되어 오동작한다.
	//반드시 복사본에만 쓴다.
	MSG msg = *pMsg;
	msg.hwnd = (HWND)SendMessage(TTM_WINDOWFROMPOINT, 0, (LPARAM)&(msg.pt));

	CPoint pt = msg.pt;

	if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
		::ScreenToClient(msg.hwnd, &pt);

	msg.lParam = MAKELONG(pt.x, pt.y);

	SendMessage(TTM_RELAYEVENT, 0, (LPARAM)&msg);
}

CString CSCToolTipCtrl::get_current_text()
{
	TOOLINFO ti = { 0, };
	ti.cbSize = sizeof(TOOLINFO);

	//지금 표시하려는 도구를 먼저 찾는다. hwnd / uId 가 채워져야 TTM_GETTEXT 가 그 도구의 문자열을 준다.
	if (!SendMessage(TTM_GETCURRENTTOOL, 0, (LPARAM)&ti))
		return _T("");

	TCHAR buffer[4096] = { 0, };
	ti.lpszText = buffer;

	SendMessage(TTM_GETTEXT, _countof(buffer), (LPARAM)&ti);

	return buffer;
}

CRect CSCToolTipCtrl::build(CString text, CDC* dc)
{
	m_para.clear();

	if (text.IsEmpty())
		return CRect(0, 0, 0, 0);

	m_text_prop.cr_text = m_theme.cr_text;
	m_text_prop.cr_back = m_theme.cr_back;

	//CSCParagraph 는 <br> 만 줄바꿈으로 인식한다. 표준 CToolTipCtrl 은 개행문자를 쓰므로,
	//기존 툴팁 문자열을 고치지 않고 그대로 넘길 수 있도록 여기서 바꿔준다.
	text.Replace(_T("\r\n"), _T("<br>"));
	text.Replace(_T("\n"), _T("<br>"));
	text.Replace(_T("\r"), _T("<br>"));

	CSCParagraph::build_paragraph_str(text, m_para, &m_text_prop);

	//m_max_width 를 넘으면 자동 줄바꿈. rc 의 높이는 calc_text_rect 가 채우므로 0 으로 둔다.
	CRect rc(0, 0, m_max_width, 0);

	CSCParagraph::calc_text_rect(rc, dc, m_para, DT_NOCLIP, m_max_width);

	//calc_text_rect 직후는 줄 간격 1.0 상태다. 이 호출이 m_line_spacing 을 적용하고 <ls=값> 태그도 여기서 반영된다.
	//CSCStatic 은 rebuild_layout 에서 같은 일을 reapply_line_spacings 로 한다(그쪽은 라인별 override 를 지원하기 때문).
	return CSCParagraph::set_line_spacing(m_para, m_line_spacing);
}

void CSCToolTipCtrl::on_show(NMHDR* nmhdr, LRESULT* result)
{
	//comctl32 는 fade 애니메이션을 위해 툴팁 창에 WS_EX_LAYERED 를 붙인다.
	//layered 창에는 GDI/GDI+ 가 ClearType 을 쓸 수 없어(서브픽셀이 알파 합성과 충돌) 시스템이
	//grayscale AA 로 강등시키고, 그 결과 작은 글자가 흐려진다. 표시 직전에 벗겨 ClearType 을 되살린다.
	//대가는 fade-in 애니메이션 소실뿐이다.
	ModifyStyleEx(WS_EX_LAYERED, 0);

	CString text = get_current_text();

	CClientDC dc(this);
	CRect rc = build(text, &dc);

	m_sz_content = CSize(rc.Width(), rc.Height());

	CSize size(m_sz_content.cx + m_padding_cx * 2, m_sz_content.cy + m_padding_cy * 2);

	CRect window;
	GetWindowRect(window);

	CPoint cursor;
	::GetCursorPos(&cursor);

	//시스템이 잡아둔 위치는 태그를 포함한 원문 기준(태그·개행이 전부 한 줄로 계산된다)의 잘못된 크기로
	//화면 경계 보정까지 끝낸 값이다. 그 보정이 걸리면 left 가 "모니터 오른쪽 - 잘못된 폭" 으로 고정되어
	//커서 위치와 무관한 곳에 뜬다. 그래서 크기만 바꾸지 않고 위치도 실제 크기로 다시 잡는다.
	//커서와의 세로 간격만 시스템 값을 빌려 쓴다 — 커서 크기·DPI 에 따라 달라지므로 상수로 박지 않는다.
	//시스템이 이미 커서 위로 뒤집어 놓았다면 간격은 커서와 아래변 사이에 있다.
	//뒤집을지 말지는 아래에서 실제 높이로 우리가 다시 판단한다.
	int gap = (window.top > cursor.y) ? window.top - cursor.y : cursor.y - window.bottom;

	CRect tip(CPoint(cursor.x, cursor.y + gap), size);

	//보정 기준은 커서가 놓인 모니터의 작업영역. 멀티모니터에서 옆 모니터로 넘어가지 않게 한다.
	int index = get_monitor_index(cursor.x, cursor.y);
	CRect work = (index >= 0) ? g_monitors[index].rWork : get_monitor_rect();

	if (tip.right > work.right)
		tip.OffsetRect(work.right - tip.right, 0);
	//폭이 모니터보다 넓으면(set_max_width(0) 로 줄바꿈을 끈 경우) 왼쪽을 살려 글 첫머리가 보이게 한다.
	if (tip.left < work.left)
		tip.OffsetRect(work.left - tip.left, 0);

	//아래로 넘치면 커서 위로 뒤집는다. 커서를 가리지 않도록 아래일 때와 같은 간격을 둔다.
	if (tip.bottom > work.bottom)
		tip.OffsetRect(0, -(size.cy + gap * 2));
	if (tip.top < work.top)
		tip.OffsetRect(0, work.top - tip.top);

	SetWindowPos(NULL, tip.left, tip.top, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE);

	//TRUE 를 돌려주면 시스템이 크기·위치를 다시 잡지 않는다.
	*result = TRUE;
}

void CSCToolTipCtrl::on_custom_draw(NMHDR* nmhdr, LRESULT* result)
{
	NMTTCUSTOMDRAW* custom_draw = (NMTTCUSTOMDRAW*)nmhdr;

	if (custom_draw->nmcd.dwDrawStage != CDDS_PREPAINT)
	{
		*result = CDRF_DODEFAULT;
		return;
	}

	CRect rc;
	GetClientRect(rc);

	Gdiplus::Graphics g(custom_draw->nmcd.hdc);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	draw_round_rect(&g, Gdiplus::Rect(rc.left, rc.top, rc.Width(), rc.Height()),
		m_theme.cr_border_inactive, m_theme.cr_back, m_round);

	if (!m_para.empty())
	{
		//para 의 좌표는 (0,0) 기준 상대좌표라 여백만큼 옮겨서 그린다.
		g.TranslateTransform((Gdiplus::REAL)m_padding_cx, (Gdiplus::REAL)m_padding_cy);

		//다크 배경에서는 ClearType subpixel fringe 가 거슬리므로 draw_text 가 grayscale AA 를 쓰게 한다.
		//기준(get_luminance < 128)은 CSCStatic 의 단락 그리기와 동일하게 맞춘다.
		bool dark_background = (get_luminance(m_theme.cr_back) < 128);

		//20260807 by claude. [진단] 선명도 저하 원인 확정용 (layered 여부 + GDI+ 기본 hint).
		//측정 결과: 수정 전 exstyle=0x00080080 layered=1 hint=0(SystemDefault) → 수정 후 0x00000080 layered=0.
		//logWrite(_T("[tooltip] exstyle=0x%08X layered=%d default_hint=%d font=%s size=%.0f dpi=%.1f"),
		//	::GetWindowLong(m_hWnd, GWL_EXSTYLE),
		//	(::GetWindowLong(m_hWnd, GWL_EXSTYLE) & WS_EX_LAYERED) ? 1 : 0,
		//	(int)g.GetTextRenderingHint(),
		//	m_text_prop.name, (double)m_text_prop.size, g.GetDpiY());

		//자동 결정이 OFF 일 때만 단일 hint 를 강제한다. ON 이면 draw_text 가 run 별로 고른다.
		//CSCStatic::OnPaint 의 단락 분기와 같은 처리 — 서식뿐 아니라 렌더 품질도 CSCStatic 과 일치시킨다.
		//(AA_from_pt 를 0 으로 넘기면 draw_text 가 hint 를 건드리지 않아 GDI+ 기본값으로 흐리게 그려진다)
		if (!m_auto_font_quality)
			g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

		CSCParagraph::draw_text(g, m_para, m_auto_font_quality ? m_AA_from_pt : 0, dark_background);
	}

	//배경부터 본문까지 전부 직접 그렸으므로 기본 그리기를 막는다.
	*result = CDRF_SKIPDEFAULT;
}
