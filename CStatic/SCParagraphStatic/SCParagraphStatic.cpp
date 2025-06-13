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

	//m_sc_font는 PreSubclassWindow()에서 초기화되었는데
	//그 후 이 set_text()를 호출하기 전에 색상, 폰트관련 설정이 변경되었을 수 있으므로
	//설정값을 m_sc_font에 갱신시켜줘야 한다.
	update_sc_font();


	CSCParagraph::build_paragraph_str(text, m_para, &m_sc_font);

	//"<b><cr = Red>This</b></cr > is a <cr =Blue><i>sample</i> <b>paragraph</b>."
	CClientDC dc(this);
	CRect rc;
	GetClientRect(&rc);
	CSCParagraph::calc_text_rect(rc, &dc, m_para, DT_LEFT | DT_CENTER);
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

	Gdiplus::Graphics g(dc);
	draw_text(g, m_para);
}

void CSCParagraphStatic::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CSCStatic::PreSubclassWindow();

	update_sc_font();
}

void CSCParagraphStatic::update_sc_font()
{
	m_sc_font.name = m_lf.lfFaceName;
	m_sc_font.size = -MulDiv(m_lf.lfHeight, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72);

	if (m_lf.lfWeight >= FW_BOLD)
		m_sc_font.style |= Gdiplus::FontStyleBold;
	if (m_lf.lfItalic)
		m_sc_font.style |= Gdiplus::FontStyleItalic;
	if (m_lf.lfUnderline)
		m_sc_font.style |= Gdiplus::FontStyleUnderline;
	if (m_lf.lfStrikeOut)
		m_sc_font.style |= Gdiplus::FontStyleStrikeout;

	m_sc_font.cr_text = m_cr_text;
	m_sc_font.cr_back = m_cr_back;
}

void CSCParagraphStatic::OnSize(UINT nType, int cx, int cy)
{
	CSCStatic::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	CRect rc;
	GetClientRect(rc);
	CSCParagraph::calc_text_rect(rc, GetDC(), m_para, DT_CENTER | DT_VCENTER);
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
