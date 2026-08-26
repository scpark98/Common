#include "SCToolTipCtrl.h"
#include "../../Functions.h"
#include "../../win_compat/dwm.h"
//20260807 by claude. [진단] 글자 선명도 추적용. 원인 확정되어 주석 처리 — 재조사 시 이 줄과
//on_custom_draw 의 [tooltip] logWrite 만 풀면 된다. (이 모듈을 쓰는 RemoteSDKDemo 등에는
//SCLog.cpp 가 없어 활성화한 채로 두면 pLog 미해결로 링크가 깨진다.)
//#include "../../log/SCLog/SCLog.h"

IMPLEMENT_DYNAMIC(CSCToolTipCtrl, CToolTipCtrl)

BEGIN_MESSAGE_MAP(CSCToolTipCtrl, CToolTipCtrl)
	//툴팁은 자신의 알림을 부모에게 보내므로 파생 클래스에서 받으려면 reflect 를 써야 한다.
	ON_NOTIFY_REFLECT(TTN_SHOW, &CSCToolTipCtrl::on_show)
	ON_NOTIFY_REFLECT(TTN_POP, &CSCToolTipCtrl::on_pop)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CSCToolTipCtrl::on_custom_draw)
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
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

void CSCToolTipCtrl::set_fade(bool on)
{
	m_fade = on;
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
	m_text_prop.cr_back = m_theme.cr_tooltip_back;

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
	//20260826 by claude. layered 인 동안 그린 글자는 흐리게 나온다(실측). 확실한 제약은 per-pixel alpha 표면
	//(UpdateLayeredWindow)에 ClearType 을 못 쓴다는 것이고, 상수 알파인 이 경로까지 강등되는지는 문서로 확인 못 했다.
	//다만 원인이 무엇이든 결과는 같다 — NM_CUSTOMDRAW 는 표시 시작 시 1회뿐이라, 그때 그린 픽셀이 계속 화면에 남는다.
	//따라서 layered 를 벗기는 것만으로는 부족하고 반드시 다시 그려야 한다(아래 OnTimer 의 Invalidate).
	//표시 직전에 벗기면 글자는 또렷해지지만 fade 가 통째로 사라져 툴팁이 "쑥" 나타난다.
	//그래서 fade 가 켜져 있으면 지금은 그대로 두고, fade 가 끝날 시점에 타이머로 벗긴다 —
	//애니메이션은 시스템 것을 그대로 쓰고 정지 상태에서만 ClearType 을 되찾는다.
	if (m_fade)
	{
		m_fade_elapsed = 0;
		SetTimer(timer_unlayer, fade_poll_interval, NULL);
	}
	else
	{
		ModifyStyleEx(WS_EX_LAYERED, 0);
	}

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

	//20260826 by claude. 모서리 라운드는 우리가 그리지 않고 DWM 에 맡긴다.
	//이 창은 불투명한 사각형이라 라운드 사각형만 그리면 모서리 바깥 픽셀을 칠할 방법이 없고
	//(뒤 배경을 알 수 없다), 그 자리에 직전 표시 때의 내용이 남아 노이즈로 보인다.
	//SetWindowRgn 으로 잘라내는 방법은 클리핑이 하드 에지라 안티에일리어싱된 테두리가 같이 잘려 계단이 진다.
	//DWM 은 합성 단계에서 알파와 함께 잘라내므로 잔상도 계단도 없다 — Win11 네이티브 툴팁과 같은 방식.
	//Win11 미만은 false 를 돌려주고 창은 각진 그대로다(그 OS 의 기본 툴팁과 같은 모양).
	m_dwm_round = (m_round > 0) && win_compat::dwm::set_window_corner_round(m_hWnd, true);

	//TRUE 를 돌려주면 시스템이 크기·위치를 다시 잡지 않는다.
	*result = TRUE;
}

void CSCToolTipCtrl::on_pop(NMHDR* nmhdr, LRESULT* result)
{
	KillTimer(timer_unlayer);

	//20260826 by claude. fade-in 이 끝나며 벗겨둔 WS_EX_LAYERED 를 되돌려준다 — 이게 없으면
	//comctl32 가 알파를 낮출 대상이 없어 사라질 때만 "툭" 끊긴다.
	//붙이기만 하면 알파가 미정이라 창이 사라져 보이므로 곧바로 불투명으로 고정한다.
	if (m_fade && (GetExStyle() & WS_EX_LAYERED) == 0)
	{
		ModifyStyleEx(0, WS_EX_LAYERED);
		::SetLayeredWindowAttributes(m_hWnd, 0, 255, LWA_ALPHA);
	}

	*result = 0;
}

void CSCToolTipCtrl::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == timer_unlayer)
	{
		m_fade_elapsed += fade_poll_interval;

		//20260826 by claude. fade 가 끝났는지를 시간이 아니라 창의 현재 알파로 판정한다.
		//고정 시간으로 끊으면 아직 알파가 올라가는 중에 벗겨져 그 순간 불투명으로 튀어(깜빡) 보인다.
		//알파를 읽을 수 없는 경우(comctl32 가 SetLayeredWindowAttributes 가 아닌 방식으로 애니메이션)는
		//최대 대기시간에서 끊는다 — 그때까지 못 끝났으면 애초에 알파 기반이 아니라는 뜻이다.
		BYTE alpha = 255;
		DWORD flags = 0;
		COLORREF cr_key = 0;
		bool alpha_readable = (::GetLayeredWindowAttributes(m_hWnd, &cr_key, &alpha, &flags) != FALSE) && (flags & LWA_ALPHA);

		if (alpha_readable && alpha < 255 && m_fade_elapsed < fade_max_wait)
			return;

		KillTimer(timer_unlayer);

		//fade 가 끝났으므로 이제 벗겨서 ClearType 을 되찾는다. 이미 알파 255 라 밝기 변화는 없고 글자만 또렷해진다.
		//SWP_FRAMECHANGED 를 주면 프레임 재계산과 함께 시스템이 한 번 더 그려 그 프레임이 깜빡임으로 보인다.
		//exstyle 만 지우면 충분하므로 flags 를 주지 않는다.
		ModifyStyleEx(WS_EX_LAYERED, 0);

		//Invalidate() 는 (1) 기본값이 배경 지우기라 툴팁 기본 배경이 한 프레임 스치고
		//(2) WM_PAINT 를 큐에 넣기만 해서, 그리기 전까지 layered 를 막 벗긴 창이 빈 상태로 보인다.
		//지우지 않고(RDW_NOERASE) 이 자리에서 즉시(RDW_UPDATENOW) 다시 그려 그 틈을 없앤다.
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
		return;
	}

	//comctl32 가 이 창에서 쓰는 내부 타이머(표시 지연·자동 감춤 등)는 그대로 넘겨야 한다.
	CToolTipCtrl::OnTimer(nIDEvent);
}

BOOL CSCToolTipCtrl::OnEraseBkgnd(CDC* pDC)
{
	//20260826 by claude. on_custom_draw 가 클라이언트 전체를 칠하므로 배경 지우기는 순수한 손해다.
	//지우고 나서 우리가 다시 칠하는 사이가 다른 색으로 한 프레임 보이는 것이 깜빡임으로 인지된다.
	return TRUE;
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

	//20260826 by claude. 클라이언트 전체를 먼저 채운다. CDRF_SKIPDEFAULT 로 기본 그리기를 막았으므로
	//우리가 칠하지 않은 픽셀에는 이 창이 직전에 떴을 때의 내용이 그대로 남는다 — 라운드 사각형만 그리면
	//네 모서리 바깥이 정확히 그 상태가 되어 잔상 노이즈로 보였다. 모서리는 DWM 이 잘라낸다(on_show 참조).
	Gdiplus::SolidBrush brush(m_theme.cr_tooltip_back);
	g.FillRectangle(&brush, rc.left, rc.top, rc.Width(), rc.Height());

	//DWM 이 잘라줄 때만 라운드 테두리를 그린다 — 잘라내지 않는 OS 에서 라운드로 그리면 모서리 바깥에
	//채움색이 그대로 남아 테두리만 안쪽으로 들어간 이상한 모양이 된다.
	if (m_dwm_round)
	{
		draw_round_rect(&g, Gdiplus::Rect(rc.left, rc.top, rc.Width() - 1, rc.Height() - 1),
			m_theme.cr_border_inactive, Gdiplus::Color::Transparent, m_round);
	}
	else
	{
		draw_rect(g, rc, m_theme.cr_border_inactive);
	}

	if (!m_para.empty())
	{
		//para 의 좌표는 (0,0) 기준 상대좌표라 여백만큼 옮겨서 그린다.
		g.TranslateTransform((Gdiplus::REAL)m_padding_cx, (Gdiplus::REAL)m_padding_cy);

		//다크 배경에서는 ClearType subpixel fringe 가 거슬리므로 draw_text 가 grayscale AA 를 쓰게 한다.
		//기준(get_luminance < 128)은 CSCStatic 의 단락 그리기와 동일하게 맞춘다.
		bool dark_background = (get_luminance(m_theme.cr_tooltip_back) < 128);

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
