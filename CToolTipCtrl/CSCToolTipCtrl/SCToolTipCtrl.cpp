#include "SCToolTipCtrl.h"
#include "../../Functions.h"

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

	return CSCParagraph::calc_text_rect(rc, dc, m_para, DT_NOCLIP, m_max_width);
}

void CSCToolTipCtrl::on_show(NMHDR* nmhdr, LRESULT* result)
{
	CString text = get_current_text();

	CClientDC dc(this);
	CRect rc = build(text, &dc);

	m_sz_content = CSize(rc.Width(), rc.Height());

	//시스템은 태그를 모른 채 원문 기준으로 크기를 잡으므로 여기서 다시 정해야 한다.
	CRect window;
	GetWindowRect(window);

	SetWindowPos(NULL,
		window.left, window.top,
		m_sz_content.cx + m_padding_cx * 2,
		m_sz_content.cy + m_padding_cy * 2,
		SWP_NOZORDER | SWP_NOACTIVATE);

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

		CSCParagraph::draw_text(g, m_para, 0, dark_background);
	}

	//배경부터 본문까지 전부 직접 그렸으므로 기본 그리기를 막는다.
	*result = CDRF_SKIPDEFAULT;
}
