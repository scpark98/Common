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
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CSCToolTipCtrl::on_custom_draw)
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

BOOL CSCToolTipCtrl::Create(CWnd* pParentWnd, DWORD dwStyle)
{
	return CToolTipCtrl::Create(pParentWnd, dwStyle | TTS_NOPREFIX);
}

void CSCToolTipCtrl::set_color_theme(const CSCColorTheme& theme)
{
	//20260831 by claude. operator= 가 아니라 copy_colors_from 을 쓴다. 다른 SC 컨트롤과 같은 규칙이다.
	//operator= 는 src 의 m_parent / m_cur_theme 까지 덮어써서, 이후 default 테마를 다시 적용할 때
	//툴팁이 자기 부모가 아니라 색을 준 컨트롤 기준으로 배경을 고르게 된다(colors.h 의 copy_colors_from 주석 참조).
	m_theme.copy_colors_from(theme);

	//이미 떠 있는 툴팁은 다음에 뜰 때 새 색으로 그려진다 — 크기·배경 계산이 TTN_SHOW / CDDS_PREPAINT 에서
	//일어나기 때문이다. 테마를 바꾼 그 순간 화면에 떠 있던 것은 내려서 다음 표시부터 확실히 새 색이 되게 한다.
	if (::IsWindow(m_hWnd))
		Pop();
}

CString CSCToolTipCtrl::escape_tags(const CString& text)
{
	CString out = text;

	//'&' 를 먼저 바꿔야 한다. get_tag_str 의 디코딩이 &amp; 를 *마지막* 에 풀기 때문에,
	//원문에 있던 "&lt;" 는 여기서 "&amp;lt;" 가 되었다가 디코딩 후 다시 "&lt;" 로 정확히 복원된다.
	//순서를 바꾸면 원문의 엔티티가 태그 기호로 둔갑한다.
	out.Replace(_T("&"), _T("&amp;"));
	out.Replace(_T("<"), _T("&lt;"));
	out.Replace(_T(">"), _T("&gt;"));

	return out;
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

	//20260828 by claude. <al=center|right> 는 넘겨받은 rc 폭을 기준으로 라인을 민다.
	//툴팁은 내용에 맞춰 줄어드는 창이라 첫 패스의 rc(=m_max_width)를 그대로 두면 가운데 정렬한 줄이
	//m_max_width 한가운데로 밀려 (a) 툴팁이 그만큼 넓어지고 (b) on_custom_draw 는 para 좌표를 0 기준으로
	//보고 그리므로 본문이 그 offset 만큼 오른쪽으로 벗어난다. 그래서 실제 내용 폭으로 한 번 더 계산해
	//정렬 기준이 툴팁 자신이 되게 한다. <al> 을 쓰지 않은 툴팁은 이 분기에 들어오지 않는다.
	bool has_line_align = false;

	for (auto& line : m_para)
	{
		if (!line.empty() && line[0].line_h_align != (DWORD)-1)
		{
			has_line_align = true;
			break;
		}
	}

	if (has_line_align)
	{
		//라인의 내용 폭은 정렬 offset 이 라인 전체에 같은 값으로 더해지므로 첫 패스 뒤에도 그대로 남아 있다.
		//<indent>/<hang> 은 그 offset 에 섞여 폭에서 빠지므로 여기서 도로 더한다.
		int content_width = 0;

		for (auto& line : m_para)
		{
			if (line.empty())
				continue;

			int width = line.back().r.right - line.front().r.left + (int)line.front().line_indent;

			if (line.front().wrap_continuation)
				width += (int)line.front().line_hang;

			content_width = max(content_width, width);
		}

		//같은 폭으로 다시 계산하므로 word-wrap 위치는 바뀌지 않는다 — 폭이 가장 넓은 라인의 실제 폭이고
		//wrap 판정이 '>' 라 딱 맞는 라인은 쪼개지지 않는다.
		CSCParagraph::calc_text_rect(CRect(0, 0, content_width, 0), dc, m_para, DT_NOCLIP, content_width);
	}

	//calc_text_rect 직후는 줄 간격 1.0 상태다. 이 호출이 m_line_spacing 을 적용하고 <ls=값> 태그도 여기서 반영된다.
	//CSCStatic 은 rebuild_layout 에서 같은 일을 reapply_line_spacings 로 한다(그쪽은 라인별 override 를 지원하기 때문).
	CSCParagraph::set_line_spacing(m_para, m_line_spacing);

	//20260828 by claude. on_custom_draw 는 para 좌표계의 원점이 (0,0) 이라고 보고 padding 만큼만 옮겨 그린다.
	//그런데 실제로 그려지는 영역의 원점은 0 이 아닐 수 있다 — <box> 의 배경은 run 의 r 밖으로 box_pad 만큼 번져
	//그려지고, <sup>·<tab>·<al=right> 도 원점을 옮긴다. 그대로 두면 그 차이만큼 내용이 창 밖으로 나가 잘렸다
	//(<box> 로 시작하는 툴팁에서 그 배경이 왼쪽·위 테두리를 뚫고 나갔던 것이 이 경우다).
	//그래서 문단 전체를 원점으로 당겨놓고 크기만 돌려준다 — 그리는 쪽은 항상 (0,0) 을 믿어도 된다.
	//set_line_spacing 의 반환값을 쓰지 않는 이유도 같다. 그쪽은 "가장 넓은 라인의 첫/마지막 run" 기준이라
	//라인마다 시작 x 가 다르거나 배경이 번지는 경우를 담지 못한다.
	CRect bounds = CSCParagraph::get_bounding_rect(m_para);

	//20260828 by claude. <glow>/<ts>/<sd> 는 글자 바깥으로 번지는데 레이아웃은 그 공간을 잡지 않는다.
	//툴팁은 내용에 딱 맞춰 줄어드는 창이라 그대로 두면 번진 부분이 테두리에서 잘려, 사방으로 고르게
	//퍼져야 할 glow 가 한쪽만 잘린 것처럼 보인다.
	//창 전체를 일률적으로 넓히면 안 된다 — 그림자가 없는 툴팁과 여백이 달라 보인다.
	//번지는 run 의 rect 만 부풀려 union 에 넣어, 실제로 테두리에 닿는 쪽만 넓어지게 한다.
	//반경 = offset + spread 의 절반(펜이 path 중앙 정렬이라 바깥으로는 절반만 나간다) + blur 1.5σ.
	//1.5σ 인 이유 : 이 blur 는 3-pass box blur 라 지원 반경이 2σ 근처인데(σ=4 → 3+3+4=10px),
	//바깥 절반은 알파가 10% 아래라 눈에 띄지 않는다. 2σ 를 다 잡으면 창만 커진다.
	//그리고 그 반경을 통째로 더하면 안 된다 — 번짐은 이미 비어 있는 padding 위로 퍼져도 되므로
	//padding 을 넘어서는 만큼만 확보한다. 안 그러면 아래쪽이 reach + padding 이 되어 위아래가 크게 어긋난다.
	std::vector<CSCTextShadow> shadows;

	for (auto& line : m_para)
	{
		for (auto& run : line)
		{
			run.get_shadow_list(shadows);

			for (auto& s : shadows)
			{
				if (s.color.GetA() == 0 || run.text.IsEmpty())
					continue;

				float dx = (s.dx < 0.0f) ? -s.dx : s.dx;
				float dy = (s.dy < 0.0f) ? -s.dy : s.dy;
				int reach = (int)(((dx > dy) ? dx : dy) + s.spread / 2.0f + s.blur * 1.5f + 0.5f);

				CRect r = run.r;
				r.InflateRect(max(0, reach - m_padding_cx), max(0, reach - m_padding_cy));

				bounds.left = min(bounds.left, r.left);
				bounds.top = min(bounds.top, r.top);
				bounds.right = max(bounds.right, r.right);
				bounds.bottom = max(bounds.bottom, r.bottom);
			}
		}
	}

	for (auto& line : m_para)
	{
		for (auto& run : line)
			run.r.OffsetRect(-bounds.left, -bounds.top);
	}

	return CRect(0, 0, bounds.Width(), bounds.Height());
}

void CSCToolTipCtrl::on_show(NMHDR* nmhdr, LRESULT* result)
{
	//comctl32 는 fade 애니메이션을 위해 툴팁 창에 WS_EX_LAYERED 를 붙인다.
	//20260826 by claude. 한때 이 스타일이 ClearType 을 죽인다고 보고 표시 후 벗겼다가 되붙이는 처리를 했었다.
	//픽셀로 측정해보니 사실이 아니다 — layered 상태로 그린 글자와 벗기고 다시 그린 글자의 서브픽셀 프린지 수가
	//완전히 동일했다(281/199 vs 281/199, 469/351 vs 469/351 …). 즉 상수 알파 layered 는 글자 품질과 무관하다.
	//(문서화된 제약은 per-pixel alpha 표면(UpdateLayeredWindow)에 ClearType 을 못 쓴다는 것이고 이 경로가 아니다.)
	//그래서 layered 는 건드리지 않는다. fade 는 comctl32 가 처음부터 끝까지 알아서 하고,
	//우리가 스타일을 토글하며 만들던 여분의 재그리기(어두운 테마에서 펄럭임으로 보이던)도 함께 사라진다.
	//fade 를 끄고 싶을 때만 표시 직전에 벗긴다 — layered 가 없으면 comctl32 는 알파를 올릴 대상이 없어 즉시 뜬다.
	if (!m_fade)
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

	//20260826 by claude. SWP_NOCOPYBITS 가 없으면 시스템이 *기존 클라이언트 내용을 복사해* 새 위치에 되붙인다.
	//툴팁은 창을 새로 만들지 않고 같은 창을 옮겨 쓰므로, 이미 떠 있는 상태에서 다른 컨트롤로 옮겨갈 때
	//직전 툴팁의 픽셀이 새 위치·새 크기로 한 프레임 보였다가 우리 그리기에 덮인다 — 그게 펄럭임이다.
	//복사를 막고 전체를 무효화해 새 내용으로만 한 번 그리게 한다(내용은 위 build 에서 이미 새것으로 바뀌어 있다).
	SetWindowPos(NULL, tip.left, tip.top, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);

	Invalidate(FALSE);

	apply_drop_shadow();

	//20260826 by claude. 모서리 라운드는 우리가 그리지 않고 DWM 에 맡긴다.
	//이 창은 불투명한 사각형이라 라운드 사각형만 그리면 모서리 바깥 픽셀을 칠할 방법이 없고
	//(뒤 배경을 알 수 없다), 그 자리에 직전 표시 때의 내용이 남아 노이즈로 보인다.
	//SetWindowRgn 으로 잘라내는 방법은 클리핑이 하드 에지라 안티에일리어싱된 테두리가 같이 잘려 계단이 진다.
	//DWM 은 합성 단계에서 알파와 함께 잘라내므로 잔상도 계단도 없다 — Win11 네이티브 툴팁과 같은 방식.
	//Win11 미만은 false 를 돌려주고 창은 각진 그대로다(그 OS 의 기본 툴팁과 같은 모양).
	bool dwm_round = (m_round > 0) && win_compat::dwm::set_window_corner_round(m_hWnd, true);

	//20260826 by claude. DWM 은 모서리를 자르면서 자기 테두리도 1px 그린다(실측: 우리 테두리 *바깥* 에 밝은 선이
	//하나 더 있어 이중선으로 보였다). 그 색을 우리 테마 색으로 지정하고 우리는 그리지 않는다 —
	//라운드를 따라 정확히 한 줄만 남고, 우리가 그릴 때 생기던 곡선-클리핑 어긋남도 없다.
	m_dwm_border = dwm_round && win_compat::dwm::set_border_color(m_hWnd, m_theme.cr_tooltip_border.ToCOLORREF());

	//TRUE 를 돌려주면 시스템이 크기·위치를 다시 잡지 않는다.
	*result = TRUE;
}

void CSCToolTipCtrl::apply_drop_shadow()
{
	//20260826 by claude. 툴팁이 어떤 배경 위에서도 분리돼 보이는 진짜 이유는 배경색이 아니라 그림자다.
	//배경색만으로 띄우면 테마마다 대비를 다시 맞춰야 하지만 그림자는 배경과 무관하게 성립한다.
	//CS_DROPSHADOW 는 창이 아니라 *클래스* 속성이라 한 번만 켜면 되고, 이 프로세스가 공유하는
	//tooltips_class32 의 다른 툴팁(표준 CToolTipCtrl 포함)에도 함께 적용된다 — 윈도우 기본 툴팁이
	//원래 이 스타일을 쓰므로 그쪽도 네이티브에 가까워질 뿐 어긋나지 않는다.
	//시스템 설정(SPI_GETDROPSHADOW)이 꺼져 있으면 스타일이 있어도 그려지지 않으니 그때는 건드리지 않는다.
	//XP 부터 있는 스타일이라 XP 호환에 문제 없다.
	if (m_shadow_applied || m_hWnd == NULL)
		return;

	m_shadow_applied = true;

	BOOL shadow_enabled = FALSE;
	if (!::SystemParametersInfo(SPI_GETDROPSHADOW, 0, &shadow_enabled, 0) || !shadow_enabled)
		return;

	ULONG_PTR style = ::GetClassLongPtr(m_hWnd, GCL_STYLE);
	if ((style & CS_DROPSHADOW) == 0)
		::SetClassLongPtr(m_hWnd, GCL_STYLE, (LONG_PTR)(style | CS_DROPSHADOW));
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

	//DWM 이 테두리를 그려주는 환경(Win11+)에서는 우리가 그리면 그 안쪽에 겹쳐 이중선이 된다(on_show 참조).
	//DWM 이 안 그리는 OS 에서는 창이 각지므로 사각 테두리를 직접 그린다.
	if (!m_dwm_border)
		draw_rect(g, rc, m_theme.cr_tooltip_border);

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

		//20260826 by claude. [진단] "layered 면 ClearType 이 죽는다" 를 눈짐작이 아니라 픽셀로 확정한다.
		//ClearType 은 서브픽셀 커버리지를 R/G/B 에 나눠 쓰므로 글리프 가장자리에 R!=G!=B 인 색 프린지가 남는다.
		//grayscale AA 면 모든 안티에일리어싱 픽셀이 R==G==B 다. 그래서 fringe 개수가 그대로 판정이 된다.
		//2026-08-26 측정 결과 : layered=1 과 layered=0 의 프린지 수가 완전히 동일 → layered 는 글자 품질과 무관.
		//visible / rc 도 같이 찍으므로 "보이지 않는 그리기" 나 "빈 프레임" 추적에도 그대로 쓸 수 있다.
		//주석 처리 상태 유지 — 재조사 시 이 블록과 파일 상단 SCLog include 만 풀면 된다.
		//{
		//	HDC hdc = custom_draw->nmcd.hdc;
		//	int fringe = 0;
		//	int aa = 0;
		//	COLORREF cr_bg = m_theme.cr_tooltip_back.ToCOLORREF();
		//	for (int y = m_padding_cy; y < min(m_padding_cy + 20, rc.bottom); y++)
		//	{
		//		for (int x = m_padding_cx; x < min(m_padding_cx + 260, rc.right); x++)
		//		{
		//			COLORREF cr = ::GetPixel(hdc, x, y);
		//			if (cr == cr_bg || cr == CLR_INVALID)
		//				continue;
		//			aa++;
		//			if (GetRValue(cr) != GetGValue(cr) || GetGValue(cr) != GetBValue(cr))
		//				fringe++;
		//		}
		//	}
		//	logWrite(_T("[tooltip] layered=%d visible=%d rc=%dx%d 비배경=%d 색프린지=%d"),
		//		(::GetWindowLong(m_hWnd, GWL_EXSTYLE) & WS_EX_LAYERED) ? 1 : 0,
		//		IsWindowVisible() ? 1 : 0, rc.Width(), rc.Height(), aa, fringe);
		//
		//}
	}

	//배경부터 본문까지 전부 직접 그렸으므로 기본 그리기를 막는다.
	*result = CDRF_SKIPDEFAULT;
}
