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
	CSCParagraph::build_paragraph_str(text, m_para, &m_lf, m_cr_text, m_cr_back);

	//"<b><cr = Red>This</b></cr > is a <cr =Blue><i>sample</i> <b>paragraph</b>."
	CClientDC dc(this);
	CRect rc;
	GetClientRect(&rc);
	::calc_text_size(rc, &dc, m_para, &m_lf, DT_LEFT | DT_CENTER);
	Invalidate();

	return m_rect_text;
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
	dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());

	draw_text(&dc, m_para, &m_lf);
	/*
	CFont font, *pOldFont = NULL;

	Gdiplus::Graphics g(dc.m_hDC, rc);
	Gdiplus::StringFormat sf;

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

	//dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());
	draw_rectangle(g, rc, Gdiplus::Color::Transparent, m_cr_back);

	//dc.SetBkColor()로 지정된 배경색을 설정하면 편하지만
	//글자 속성에 따라 그 높낮이가 다른 경우도 있다.
	//따라서 recalc_text_size()에서 max height를 모든 paragraph에 적용했으며
	//여기서도 배경색으로 칠한 뒤 텍스트를 표시한다.
	dc.SetBkMode(TRANSPARENT);

	if (m_hIcon)
	{
		draw_icon(&dc, m_hIcon, make_rect(m_pt_icon.x, m_pt_icon.y, m_sz_icon.cx, m_sz_icon.cy));
	}

	
	for (i = 0; i < m_para.size(); i++)
	{
		for (j = 0; j < m_para[i].size(); j++)
		{
#if 1
			pOldFont = select_paragraph_font(m_para, i, j, &dc, &m_lf, &font);

			//text 배경색을 칠하고
			if (m_para[i][j].cr_back.GetValue() != Gdiplus::Color::Transparent)
				//dc.FillSolidRect(m_para[i][j].r, m_para[i][j].cr_back.ToCOLORREF());
				draw_rectangle(g, m_para[i][j].r, Gdiplus::Color::Transparent, m_para[i][j].cr_back);

			//text를 출력한다.
			dc.SetTextColor(m_para[i][j].cr_text.ToCOLORREF());
			dc.DrawText(m_para[i][j].text, m_para[i][j].r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
			//draw_text(g, m_para[i][j].r, m_para[i][j].cr_text, m_para[i][j].text, m_para[i][j].size, m_para[i][j].);
			//g.DrawString(CStringW(m_para[i][j].text), m_para[i][j].text.GetLength(), font, Gdiplus::PointF((Gdiplus::REAL)m_para[i][j].r.left, (Gdiplus::REAL)m_para[i][j].r.top), &sf);
			dc.SelectObject(pOldFont);
#else
			Gdiplus::Font* font;
			get_paragraph_font(i, j, g, &font);

			//text 배경색을 칠하고
			draw_rectangle(g, m_para[i][j].r, Gdiplus::Color::Transparent, m_para[i][j].cr_back);

			//text를 출력한다.
			Gdiplus::SolidBrush text_brush(m_para[i][j].cr_text);
			g.DrawString(CStringW(m_para[i][j].text), m_para[i][j].text.GetLength(), font, Gdiplus::PointF((Gdiplus::REAL)m_para[i][j].r.left, (Gdiplus::REAL)m_para[i][j].r.top), &sf, &text_brush);
#endif

			if (m_draw_word_hover_rect && CPoint(i, j) == m_pos_word_hover)
			{
				draw_rectangle(g, m_para[i][j].r, Gdiplus::Color::Red, Gdiplus::Color(96, 255, 255, 255));
			}
		}
	}

	//텍스트 출력 영역 확인용
#ifdef _DEBUG
	//draw_rectangle(g, m_rect_text, Gdiplus::Color::Blue, Gdiplus::Color::Transparent, 1);
#endif
	TRACE(_T("m_rect_text = %s\n"), get_rect_info_string(m_rect_text));

	font.DeleteObject();
	dc.SelectObject(pOldFont);
	*/
}

void CSCParagraphStatic::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CSCStatic::PreSubclassWindow();
}

void CSCParagraphStatic::OnSize(UINT nType, int cx, int cy)
{
	CSCStatic::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	CRect rc;
	GetClientRect(rc);
	calc_text_size(rc, GetDC(), m_para, &m_lf, DT_LEFT | DT_VCENTER);
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
					TRACE(_T("m_pos_word_hover = %d, %d\n"), i, j);
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
