// SCParagraphStatic.cpp: 구현 파일
//

#include "SCParagraphStatic.h"
#include "../../Functions.h"
#include "../../MemoryDC.h"

// CSCParagraphStatic

IMPLEMENT_DYNAMIC(CSCParagraphStatic, CSCStatic)

CSCParagraphStatic::CSCParagraphStatic()
{
	memset(&m_lf, 0, sizeof(LOGFONT));
}

CSCParagraphStatic::~CSCParagraphStatic()
{
}


BEGIN_MESSAGE_MAP(CSCParagraphStatic, CSCStatic)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()



// CSCParagraphStatic 메시지 처리기
CRect CSCParagraphStatic::set_text(CString text)
{
	m_para.clear();

	//m_text_prop는 PreSubclassWindow()에서 초기화되었는데
	//그 후 이 set_text()를 호출하기 전에 색상, 폰트관련 설정이 변경되었을 수 있으므로
	//설정값을 m_text_prop에 갱신시켜줘야 한다.
	update_text_property();

	CSCParagraph::build_paragraph_str(text, m_para, &m_text_prop);

	//halign/valign, line_spacing 은 rebuild_layout() 에서 한 번에 적용된다.
	rebuild_layout();

	return m_rect_text;
}

void CSCParagraphStatic::set_textf(LPCTSTR format, ...)
{
	va_list args;
	va_start(args, format);

	CString text;
	text.FormatV(format, args);

	set_text(text);
}

void CSCParagraphStatic::set_line_spacing(float spacing)
{
	m_line_spacing = spacing;
	rebuild_layout();
}

//line = 1에서 1은 인덱스이므로 두번째 라인(인덱스=1)의 윗 간격을 조정하는 것이다.
//즉 해당 라인이 시작될 때 위에서 얼마나 간격을 넓힐것인지를 설정하는 것이다. line >= 1 이어야 한다.
//line = 0이면 0번 라인이 출력되기 전의 라인 간격이라고 할 수 있으나
//그건 top margin 에 해당한다.
//line=3, spacing=2.0f 라고 하면 index=3, 즉 4번째 라인 앞의 공백이 두배가 되는 것이고
//이는 결국 3번째 라인 다음에 2배 간격이 생기는 것과 같다.
void CSCParagraphStatic::set_line_spacing(int line, float spacing)
{
	//line == 0 은 "위쪽 간격" 개념이 없으므로 무시.
	if (line < 1)
		return;

	m_line_spacings[line] = spacing;
	rebuild_layout();
}

float CSCParagraphStatic::get_line_spacing(int line)
{
	auto it = m_line_spacings.find(line);

	if (it != m_line_spacings.end())
		return it->second;

	return m_line_spacing;
}

CRect CSCParagraphStatic::reapply_line_spacings()
{
	//호출 시점: m_para 의 각 r 는 calc_text_rect() 직후의 baseline(spacing=1.0) 위치여야 한다.
	if (m_para.empty())
		return m_rect_text;

	int i, j;
	int shift_y = 0;

	//line 0 은 기준이므로 shift 0. line >= 1 부터 누적 shift 를 가산.
	for (i = 1; i < (int)m_para.size(); i++)
	{
		int line_above_h = 0;

		for (j = 0; j < (int)m_para[i - 1].size(); j++)
		{
			int h = m_para[i - 1][j].r.Height();

			if (h > line_above_h)
				line_above_h = h;
		}

		auto it = m_line_spacings.find(i);
		float spacing = (it != m_line_spacings.end()) ? it->second : m_line_spacing;

		shift_y += (int)((float)line_above_h * (spacing - 1.0f));

		if (shift_y == 0)
			continue;

		for (j = 0; j < (int)m_para[i].size(); j++)
			m_para[i][j].r.OffsetRect(0, shift_y);
	}

	//m_rect_text 재계산. CSCParagraph::set_line_spacing() 의 최종 블록과 동일한 공식.
	CRect rect_text;
	int max_width_line = CSCParagraph::get_max_width_line(m_para);

	if (max_width_line < 0)
		max_width_line = 0;

	rect_text.left = m_para[max_width_line][0].r.left;
	rect_text.top = m_para[0][0].r.top;
	rect_text.right = m_para[max_width_line][m_para[max_width_line].size() - 1].r.right;

	int last_line = (int)m_para.size() - 1;
	int max_bottom = m_para[last_line][0].r.bottom;

	for (j = 1; j < (int)m_para[last_line].size(); j++)
	{
		if (m_para[last_line][j].r.bottom > max_bottom)
			max_bottom = m_para[last_line][j].r.bottom;
	}

	rect_text.bottom = max_bottom;

	return rect_text;
}

void CSCParagraphStatic::set_text_align(DWORD align)
{
	//halign 추출.
	if (align & DT_CENTER)
		m_halign = DT_CENTER;
	else if (align & DT_RIGHT)
		m_halign = DT_RIGHT;
	else
		m_halign = DT_LEFT;

	//valign 추출. DT_TOP 은 0 이므로 bit 검사로는 잡히지 않아 else 에서 fallback.
	if (align & DT_VCENTER)
		m_valign = DT_VCENTER;
	else if (align & DT_BOTTOM)
		m_valign = DT_BOTTOM;
	else
		m_valign = DT_TOP;

	rebuild_layout();
}

void CSCParagraphStatic::set_halign(DWORD halign)
{
	m_halign = halign;
	rebuild_layout();
}

void CSCParagraphStatic::set_halign(int line, DWORD halign)
{
	if (line < 0)
		return;

	m_line_haligns[line] = halign;
	rebuild_layout();
}

void CSCParagraphStatic::set_valign(DWORD valign)
{
	m_valign = valign;
	rebuild_layout();
}

DWORD CSCParagraphStatic::get_halign(int line)
{
	auto it = m_line_haligns.find(line);

	if (it != m_line_haligns.end())
		return it->second;

	return m_halign;
}

void CSCParagraphStatic::rebuild_layout()
{
	if (m_para.empty())
	{
		Invalidate();
		return;
	}

	CClientDC dc(this);
	CRect rc;
	GetClientRect(&rc);

	//DT_NOCLIP 만 넘겨 baseline 배치만 얻는다 — halign/valign 은 별도 단계로 적용.
	m_rect_text = CSCParagraph::calc_text_rect(rc, &dc, m_para, DT_NOCLIP);
	m_rect_text = reapply_line_spacings();
	m_rect_text = apply_halign();
	m_rect_text = apply_valign();

	Invalidate();
}

CRect CSCParagraphStatic::apply_halign()
{
	//baseline 은 모든 라인이 x=0 에서 시작. 각 라인별로 target_left 을 구해 현재 left 와의 차이만큼 shift.
	if (m_para.empty())
		return m_rect_text;

	CRect rc;
	GetClientRect(&rc);

	int i, j;

	for (i = 0; i < (int)m_para.size(); i++)
	{
		if (m_para[i].empty())
			continue;

		auto it = m_line_haligns.find(i);
		DWORD halign = (it != m_line_haligns.end()) ? it->second : m_halign;

		int total_width = 0;
		for (j = 0; j < (int)m_para[i].size(); j++)
			total_width += m_para[i][j].r.Width();

		int target_left;

		if (halign & DT_CENTER)
			target_left = rc.CenterPoint().x - total_width / 2;
		else if (halign & DT_RIGHT)
			target_left = rc.right - total_width;
		else
			target_left = rc.left;

		int shift_x = target_left - m_para[i][0].r.left;

		if (shift_x == 0)
			continue;

		for (j = 0; j < (int)m_para[i].size(); j++)
			m_para[i][j].r.OffsetRect(shift_x, 0);
	}

	//라인별 halign 이 다를 수 있으므로 rect_text.left/right 는 모든 라인에 걸친 min/max 로 구한다.
	CRect rect_text = m_rect_text;
	rect_text.left = m_para[0][0].r.left;
	rect_text.right = m_para[0][m_para[0].size() - 1].r.right;

	for (i = 1; i < (int)m_para.size(); i++)
	{
		if (m_para[i].empty())
			continue;

		if (m_para[i][0].r.left < rect_text.left)
			rect_text.left = m_para[i][0].r.left;

		int line_right = m_para[i][m_para[i].size() - 1].r.right;

		if (line_right > rect_text.right)
			rect_text.right = line_right;
	}

	return rect_text;
}

CRect CSCParagraphStatic::apply_valign()
{
	if (m_para.empty())
		return m_rect_text;

	CRect rc;
	GetClientRect(&rc);

	int first_top = m_para[0][0].r.top;
	int last_line = (int)m_para.size() - 1;
	int last_bottom = m_para[last_line][0].r.bottom;
	int j;

	for (j = 1; j < (int)m_para[last_line].size(); j++)
	{
		if (m_para[last_line][j].r.bottom > last_bottom)
			last_bottom = m_para[last_line][j].r.bottom;
	}

	int total_h = last_bottom - first_top;

	int target_top;

	if (m_valign & DT_VCENTER)
		target_top = rc.top + (rc.Height() - total_h) / 2;
	else if (m_valign & DT_BOTTOM)
		target_top = rc.bottom - total_h;
	else
		target_top = rc.top;

	int shift_y = target_top - first_top;

	if (shift_y != 0)
	{
		for (int i = 0; i < (int)m_para.size(); i++)
		{
			for (j = 0; j < (int)m_para[i].size(); j++)
				m_para[i][j].r.OffsetRect(0, shift_y);
		}
	}

	CRect rect_text = m_rect_text;
	rect_text.top = target_top;
	rect_text.bottom = target_top + total_h;

	return rect_text;
}

#if 0
CRect CSCParagraphStatic::calc_text_size()
{
	if (m_para.empty())
	{
		Invalidate();
		return CRect();
	}

	int i, j;
	int sx = 0;
	int sy = 0;				//각 라인의 시작 위치(높이값 누적)
	int total_text_height;
	CFont font, * pOldFont;
	CClientDC dc(this);
	LOGFONT lf;

	Gdiplus::Graphics g(dc.m_hDC);
	Gdiplus::StringFormat sf;

	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);
	sf.SetFormatFlags(Gdiplus::StringFormatFlagsMeasureTrailingSpaces);

	m_max_width = 0;
	m_max_width_line = 0;

	memcpy(&lf, &m_lf, sizeof(LOGFONT));

	for (i = 0; i < m_para.size(); i++)
	{
		CSize sz_text = CSize(0, 0);

		for (j = 0; j < m_para[i].size(); j++)
		{
			CSize sz;
#if 1
			pOldFont = ::select_paragraph_font(m_para, i, j, &dc, &m_lf, &font);

			//GetTextExtent()와 DrawText(DT_CALCRECT)로 구한 크기는 동일하며 italic은 약간 잘림.
			sz = dc.GetTextExtent(m_para[i][j].text);
			//GetTextExtentExPoint(dc.m_hDC, m_paragraph[i].text, m_paragraph[i].text.GetLength(), 0, NULL, NULL, &sz);

			if (m_para[i][j].italic)
			{
				TEXTMETRIC tm;
				GetTextMetrics(dc.m_hDC, &tm);
				if (tm.tmOverhang > 0)
					sz.cx += tm.tmOverhang;
				else
					sz.cx += (dc.GetTextExtent(_T("M")).cx / 4);
			}

			m_para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
#else
			Gdiplus::Font* font = NULL;
			get_paragraph_font(i, j, g, &font);
			Gdiplus::RectF boundRect;
			//g.MeasureString(CStringW(m_para[i][j].text), -1, font, Gdiplus::PointF(0, 0), &boundRect);
			//g.MeasureString(CStringW(m_para[i][j].text), -1, font, Gdiplus::PointF(0, 0), &sf, &boundRect);
			Gdiplus::Region rgn[] = Gdiplus::Region[1];
			g.MeasureCharacterRanges(CStringW(m_para[i][j].text), -1, font, boundRect, &sf, 1, &rgn);
			boundRect = rgn[0].GetBounds(g);
			sz.cx = boundRect.Width;
			sz.cy = boundRect.Height;
			m_para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
#endif
			TRACE(_T("[%d][%d] text = %s, sz = %dx%d, r = %s\n"), i, j, m_para[i][j].text, sz.cx, sz.cy, get_rect_info_string(m_para[i][j].r));
			sz_text.cx += sz.cx;

			//한 라인에서 가장 cy가 큰 값을 기억시킨다.
			sz_text.cy = max(sz_text.cy, sz.cy);
		}

		//각 라인들 중에서 최대 너비를 구한다.
		if (sz_text.cx > m_max_width)
		{
			m_max_width = sz_text.cx;
			m_max_width_line = i;
		}

		//각 라인 시작 위치는 누적된다.
		sy += sz_text.cy;
	}

	total_text_height = sy;

	font.DeleteObject();

	//한 라인내에서 height가 가장 높은 항목으로 통일시키느냐? 아니면 각자의 높이를 그대로 유지하느냐...
	//for (i = 0; i < m_paragraph.size(); i++)
	//{
	//	m_paragraph[i].r.bottom = m_paragraph[i].r.top + m_sz_text.cy;
	//}

	//align 옵션에 따른 보정
	DWORD dwStyle = GetStyle();
	DWORD dwText = DT_NOCLIP;// | DT_WORDBREAK;

	if (m_dwStyle == 0)
	{
		MAP_STYLE(SS_LEFT, DT_LEFT);
		MAP_STYLE(SS_RIGHT, DT_RIGHT);
		MAP_STYLE(SS_CENTER, DT_CENTER);
		MAP_STYLE(SS_NOPREFIX, DT_NOPREFIX);
		MAP_STYLE(SS_WORDELLIPSIS, DT_WORD_ELLIPSIS);
		MAP_STYLE(SS_ENDELLIPSIS, DT_END_ELLIPSIS);
		MAP_STYLE(SS_PATHELLIPSIS, DT_PATH_ELLIPSIS);
	}

	CRect rc;
	GetClientRect(&rc);

	//align에 따른 보정
	if (dwStyle & SS_CENTER)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < m_para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < m_para[i].size(); j++)
				total_width += m_para[i][j].r.Width();

			//아이콘을 포함하여 center에 표시할 지, 텍스트만 center에 표시할 지...
			//if (m_hIcon)
			//	total_width -= (m_sz_icon.cx + 4);

			//cx에서 total_width/2를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.CenterPoint().x - total_width / 2;
			for (j = 0; j < m_para[i].size(); j++)
				m_para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else if (dwStyle & SS_RIGHT)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < m_para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < m_para[i].size(); j++)
				total_width += m_para[i][j].r.Width();

			//rc.right에서 total_width를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.right - m_margin.right - total_width;
			for (j = 0; j < m_para[i].size(); j++)
				m_para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else //SS_LEFT (default)
	{
		if (m_hIcon)
		{
			for (i = 0; i < m_para.size(); i++)
			{
				for (j = 0; j < m_para[i].size(); j++)
				{
					m_para[i][j].r.OffsetRect(m_margin.left + m_sz_icon.cx + 4, 0);
				}
			}
		}
	}

	if (dwStyle & SS_CENTERIMAGE)
	{
		//전체 높이에서 전체 텍스트 높이 합계를 뺀 1/2 만큼 shift 시킨다.
		sy = (rc.Height() - total_text_height) / 2;
		for (i = 0; i < m_para.size(); i++)
		{
			for (j = 0; j < m_para[i].size(); j++)
			{
				m_para[i][j].r.OffsetRect(0, sy);
			}
		}
	}
	else //top align
	{
		for (i = 0; i < m_para.size(); i++)
		{
			for (j = 0; j < m_para[i].size(); j++)
			{
				m_para[i][j].r.OffsetRect(0, m_margin.top);
			}
		}
	}


	if (m_para.size() > 0)
	{
		m_pt_icon.x = m_para[m_max_width_line][0].r.left - m_sz_icon.cx - 4;
		//아이콘을 top 정렬하느냐, 모든 라인의 vcenter에 정렬하느냐...
		m_pt_icon.y = m_para[0][0].r.top;
		//m_pt_icon.y = m_para[0][0].r.CenterPoint().y - m_sz_icon.cy / 2;

		m_rect_text.left = m_para[m_max_width_line][0].r.left;	//최대 넓이 라인의 0번 아이템의 left
		m_rect_text.top = m_para[0][0].r.top;					//최상단 항목의 top
		m_rect_text.right = m_para[m_max_width_line][m_para[m_max_width_line].size() - 1].r.right;	//최대 넓이 라인의 마지막 항목의 right
		m_rect_text.bottom = m_para[0][0].r.top + total_text_height;	//최상단 항목의 top + 전체 텍스트 높이
	}
	else
	{
		m_pt_icon.x = sx - m_sz_icon.cx;
		m_pt_icon.y = sy - m_sz_icon.cy / 2;

		m_rect_text = make_rect(m_pt_icon.x, m_pt_icon.y, m_sz_icon.cx, m_sz_icon.cy);
	}

	if (m_auto_ctrl_size)
	{
		CRect rc;
		GetClientRect(rc);
		if (m_rect_text.Width() > rc.Width() || m_rect_text.Height() > rc.Height())
		{
			//MoveWindow(m_rect_text);
			SetWindowPos(NULL, 0, 0, m_rect_text.Width(), m_rect_text.Height(), SWP_NOMOVE | SWP_NOZORDER);
		}
	}

	return m_rect_text;
}

CFont* CSCParagraphStatic::select_paragraph_font(int line, int index, CDC* pDC, CFont* font)
{
	font->DeleteObject();

	LOGFONT lf;
	memcpy(&lf, &m_lf, sizeof(LOGFONT));

	_tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), m_para[line][index].name);

	lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, m_para[line][index].size);////-MulDiv(m_para[line][index].size, GetDeviceCaps(pDC->m_hDC, LOGPIXELSY), 72);
	lf.lfWeight = m_para[line][index].bold ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = m_para[line][index].italic;
	lf.lfUnderline = m_para[line][index].underline;
	lf.lfStrikeOut = m_para[line][index].strike;

	font->CreateFontIndirect(&lf);
	return (CFont*)pDC->SelectObject(font);
}
#endif

//마우스가 hover된 음절에 사각형 표시
void CSCParagraphStatic::draw_word_hover_rect(bool draw, Gdiplus::Color cr_rect)
{
	m_draw_word_hover_rect = draw;

	if (cr_rect.GetValue() != Gdiplus::Color::Transparent)
		m_cr_word_hover_rect = cr_rect;
}

void CSCParagraphStatic::OnPaint()
{
	int i, j;
	CPaintDC dc1(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CSCStatic::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;

	GetClientRect(&rc);
	CMemoryDC dc(&dc1, &rc);

	//draw_rectangle(dc, rc, Gdiplus::Color::Transparent, m_cr_back);
	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	Gdiplus::Graphics g(dc);

	//AntiAlias, TextRenderingHint 등은 미리 g에 세팅되서 넘어와야 한다.
	//무조건 여기서 이를 고정하여 출력하게 되면 일부 텍스트는 antiAlias로 인해 흐리게 표시된다.
	if (m_font_antialiasing)
	{
		g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
		g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	}

	CSCParagraph::draw_text(g, m_para);
}

void CSCParagraphStatic::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CSCStatic::PreSubclassWindow();

	update_text_property();

	//다이얼로그 리소스의 SS_* 스타일로부터 초기 halign/valign 추출.
	//사용자가 이후 set_halign()/set_valign()/set_text_align() 을 호출하면 덮어쓴다.
	DWORD dwStyle = GetStyle();
	DWORD dwText = 0;

	if (m_dwStyle == 0)
	{
		MAP_STYLE(SS_LEFT, DT_LEFT);
		MAP_STYLE(SS_RIGHT, DT_RIGHT);
		MAP_STYLE(SS_CENTER, DT_CENTER);
		MAP_STYLE(SS_CENTERIMAGE, DT_VCENTER);
	}

	if (dwText & DT_CENTER)
		m_halign = DT_CENTER;
	else if (dwText & DT_RIGHT)
		m_halign = DT_RIGHT;
	else
		m_halign = DT_LEFT;

	if (dwText & DT_VCENTER)
		m_valign = DT_VCENTER;
	else
		m_valign = DT_TOP;
}

void CSCParagraphStatic::update_text_property()
{
	_tcscpy_s(m_text_prop.name, _countof(m_text_prop.name), m_lf.lfFaceName);
	m_text_prop.size = get_font_size_from_pixel_size(m_hWnd, m_lf.lfHeight);

	if (m_lf.lfWeight >= FW_BOLD)
		m_text_prop.style |= Gdiplus::FontStyleBold;
	if (m_lf.lfItalic)
		m_text_prop.style |= Gdiplus::FontStyleItalic;
	if (m_lf.lfUnderline)
		m_text_prop.style |= Gdiplus::FontStyleUnderline;
	if (m_lf.lfStrikeOut)
		m_text_prop.style |= Gdiplus::FontStyleStrikeout;

	m_text_prop.cr_text = m_theme.cr_text;
	m_text_prop.cr_back = m_theme.cr_back;
}

void CSCParagraphStatic::OnSize(UINT nType, int cx, int cy)
{
	CSCStatic::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	rebuild_layout();
}

void CSCParagraphStatic::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_draw_word_hover_rect)
	{
		for (int i = 0; i < m_para.size(); i++)
		{
			for (int j = 0; j < m_para[i].size(); j++)
			{
				if (m_para[i][j].r.PtInRect(point))
				{
					m_pos_word_hover = CPoint(i, j);
					TRACE(_T("m_pos_word_hover = %d, %d, %s\n"), i, j, m_para[i][j].text);
					Invalidate();
					return;
				}
			}
		}

		m_pos_word_hover = CPoint(-1, -1);
		Invalidate();
	}

	CSCStatic::OnMouseMove(nFlags, point);
}
