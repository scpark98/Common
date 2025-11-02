#include "SCParagraph.h"
#include "../../Functions.h"

CSCParagraph::CSCParagraph()
{

}

CSCParagraph::~CSCParagraph()
{
}

//text의 태그를 파싱하여 각 구문의 속성을 설정한 후 para에 저장한다.
//cr_text, cr_back은 글자, 배경 기본값
void CSCParagraph::build_paragraph_str(CString& text, std::deque<std::deque<CSCParagraph>>& para, CSCTextProperty* text_prop)
{
	int i;
	CString cr_str;
	CString str;
	std::deque<CString> tags;
	int line = 0;

	//"<b><cr=red>This</b></cr> is a <cr=blue><i>sample</i> <b>paragraph</b>."
	get_tag_str(text, tags);

	CSCParagraph basic_para, para_temp;
	basic_para.text_prop = *text_prop;
	para_temp = basic_para;

	std::deque<CSCParagraph> para_line;

	//시작 태그를 만나면 속성을 세팅하고
	//단순 텍스트를 만나면 해당 속성과 함께 paragraph로 push하고
	//끝 태그를 만나면 끝 태그가 끝날때까지 진행하고 끝 태그를 만나면 그 속성을 해제한다. 
	for (i = 0; i < tags.size(); i++)
	{
		if (tags[i] == _T("<b>"))
		{
			para_temp.text_prop.style |= Gdiplus::FontStyleBold;
		}
		else if (tags[i] == _T("</b>"))
		{
			para_temp.text_prop.style &= ~Gdiplus::FontStyleBold;
		}
		else if (tags[i] == _T("<i>"))
		{
			para_temp.text_prop.style |= Gdiplus::FontStyleItalic;
		}
		else if (tags[i] == _T("</i>"))
		{
			para_temp.text_prop.style &= ~Gdiplus::FontStyleItalic;
		}
		else if (tags[i] == _T("<u>"))
		{
			para_temp.text_prop.style |= Gdiplus::FontStyleUnderline;
		}
		else if (tags[i] == _T("</u>"))
		{
			para_temp.text_prop.style &= ~Gdiplus::FontStyleUnderline;
		}
		else if (tags[i] == _T("<s>"))
		{
			para_temp.text_prop.style |= Gdiplus::FontStyleStrikeout;
		}
		else if (tags[i] == _T("</s>"))
		{
			para_temp.text_prop.style &= ~Gdiplus::FontStyleStrikeout;
		}
		else if (tags[i].Find(_T("<cr=")) >= 0 || tags[i].Find(_T("<ct=")) >= 0)
		{
			cr_str = tags[i].Mid(4, tags[i].GetLength() - 5);
			para_temp.text_prop.cr_text = get_color(cr_str);
		}
		else if (tags[i].Find(_T("</cr>")) >= 0 || tags[i].Find(_T("</ct>")) >= 0)
		{
			para_temp.text_prop.cr_text = basic_para.text_prop.cr_text;
		}
		else if (tags[i].Find(_T("<cb=")) >= 0)
		{
			cr_str = tags[i].Mid(4, tags[i].GetLength() - 5);
			para_temp.text_prop.cr_back = get_color(cr_str);
		}
		else if (tags[i].Find(_T("<crb=")) >= 0)
		{
			cr_str = tags[i].Mid(5, tags[i].GetLength() - 6);
			para_temp.text_prop.cr_back = get_color(cr_str);
		}
		else if (tags[i].Find(_T("</cb>")) >= 0 || tags[i].Find(_T("</crb>")) >= 0)
		{
			para_temp.text_prop.cr_back = basic_para.text_prop.cr_back;
		}
		else if (tags[i].Find(_T("<f=")) >= 0)
		{
			CString str_font = tags[i].Mid(3, tags[i].GetLength() - 4);
			para_temp.text_prop.name = str_font;
		}
		else if (tags[i].Find(_T("<font=")) >= 0)
		{
			CString str_font = tags[i].Mid(6, tags[i].GetLength() - 7);
			para_temp.text_prop.name = str_font;
		}
		else if (tags[i].Find(_T("<name=")) >= 0)
		{
			CString str_font = tags[i].Mid(6, tags[i].GetLength() - 7);
			para_temp.text_prop.name = str_font;
		}
		else if (tags[i].Find(_T("<font_name=")) >= 0)
		{
			CString str_font = tags[i].Mid(11, tags[i].GetLength() - 12);
			para_temp.text_prop.name = str_font;
		}
		else if (tags[i].Find(_T("</f>")) >= 0 || tags[i].Find(_T("</font>")) >= 0 || tags[i].Find(_T("</name>")) >= 0 || tags[i].Find(_T("</font_name>")) >= 0)
		{
			para_temp.text_prop.name = basic_para.text_prop.name;
		}
		else if (tags[i].Find(_T("<sz=")) >= 0)
		{
			CString str_size = tags[i].Mid(4, tags[i].GetLength() - 5);
			para_temp.text_prop.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("<size=")) >= 0)
		{
			CString str_size = tags[i].Mid(6, tags[i].GetLength() - 7);
			para_temp.text_prop.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("<font_size=")) >= 0)
		{
			CString str_size = tags[i].Mid(11, tags[i].GetLength() - 12);
			para_temp.text_prop.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("</sz>")) >= 0 || tags[i].Find(_T("</size>")) >= 0 || tags[i].Find(_T("</font_size>")) >= 0)
		{
			para_temp.text_prop.size = basic_para.text_prop.size;
		}
		else if (tags[i] == _T("<br>"))
		{
			//<br>에 의해 공백 라인이 추가된 경우
			if (para_line.size() == 0)
			{
				para_line.push_back(basic_para);
			}

			para.push_back(para_line);
			para_line.clear();
			line++;
		}
		else
		{
			para_temp.text = tags[i];

			//전체 배경색인 cr_back이 Transparent가 아닐 때 색상이 별도로 지정되지 않은 para[][].cr_back에 cr_back을 줄 경우 중복으로 그려지게 된다.
			//cr_back이 불투명이면 덮어써서 그려져서 표가 나지 않지만 반투명이면 겹쳐져 그려지게 된다.
			if (para_temp.text_prop.cr_back.GetValue() == basic_para.text_prop.cr_back.GetValue())
				para_temp.text_prop.cr_back = Gdiplus::Color::Transparent;

			para_line.push_back(para_temp);
		}
	}

	if (para_line.size())
		para.push_back(para_line);
}

//paragraph text 정보를 dc에 출력할 때 출력 크기를 계산하고 각 텍스트가 출력될 위치까지 CSCParagraph 멤버에 저장한다.
CRect CSCParagraph::calc_text_rect(CRect rc, CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, DWORD align)
{
	if (para.empty())
		return CRect();

	int i, j;
	int sx = 0;
	int sy = 0;				//각 라인의 시작 위치(높이값 누적)
	int total_text_height;
	CRect rect_text;
	CFont font, * pOldFont;

	Gdiplus::Graphics g(pDC->m_hDC);

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	Gdiplus::StringFormat sf;

	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);
	//sf.SetTrimming(Gdiplus::StringTrimmingNone);

	int max_width = 0;
	int max_width_line = 0;

	for (i = 0; i < para.size(); i++)
	{
		CSize sz_text = CSize(0, 0);

		for (j = 0; j < para[i].size(); j++)
		{
			CSize sz;
#if 0
			pOldFont = select_paragraph_font(pDC, para, i, j, lf, &font);

			//GetTextExtent()와 DrawText(DT_CALCRECT)로 구한 크기는 동일하며 italic은 약간 잘림.
			sz = pDC->GetTextExtent(para[i][j].text);
			//GetTextExtentExPoint(dc.m_hDC, m_paragraph[i].text, m_paragraph[i].text.GetLength(), 0, NULL, NULL, &sz);

			//if (para[i][j].italic)
			//{
			//	TEXTMETRIC tm;
			//	GetTextMetrics(pDC->m_hDC, &tm);
			//	if (tm.tmOverhang > 0)
			//		sz.cx += tm.tmOverhang;
			//	else
			//		sz.cx += (pDC->GetTextExtent(_T("M")).cx / 4);
			//}

			para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
#else
			Gdiplus::Font* font = NULL;
			para[i][j].get_paragraph_font(g, &font);
			Gdiplus::RectF boundRect;
			Gdiplus::RectF boundRect_temp;

			//"text...    "와 같이 뒤에 공백이 있을 경우 공백이 무시되어 출력되지 않는다.
			//방법1. 맨 끝에 "|"와 같은 문자를 넣어 계산한 후 "|"의 width를 뺸다.
			//방법2. SetMeasurableCharacterRanges(), MeasureCharacterRanges() 등을 이용하는 것이 더 좋음
			//우선 간단하게 1번 방식을 사용한다.
			g.MeasureString(CStringW(para[i][j].text + _T("|")), -1, font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &boundRect);
			g.MeasureString(L"|", -1, font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &boundRect_temp);

			if (boundRect.IsEmptyArea())
			{
				boundRect.Width = 1;
				boundRect.Height = 40;
			}

			//stroke 두께까지 포함한 크기여야 한다.
			sz.cx = boundRect.Width - boundRect_temp.Width + para[i][j].text_prop.thickness;// *2.0f;
			sz.cy = boundRect.Height + para[i][j].text_prop.thickness;// *2.0f;
			para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
#endif
			//TRACE(_T("[%d][%d] text = %s, sz = %dx%d, r = %s\n"), i, j, para[i][j].text, sz.cx, sz.cy, get_rect_info_string(para[i][j].r));
			sz_text.cx += sz.cx;

			//한 라인에서 가장 cy가 큰 값을 기억시킨다.
			sz_text.cy = MAX(sz_text.cy, sz.cy);
		}

		//각 라인들 중에서 최대 너비를 구한다.
		if (sz_text.cx > max_width)
		{
			max_width = sz_text.cx;
			max_width_line = i;
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
	/*
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
	*/

	CRect margin;

	//align에 따른 보정
	if (align & DT_CENTER)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//아이콘을 포함하여 center에 표시할 지, 텍스트만 center에 표시할 지...
			//if (m_hIcon)
			//	total_width -= (m_sz_icon.cx + 4);

			//cx에서 total_width/2를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.CenterPoint().x - total_width / 2;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else if (align & DT_RIGHT)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//rc.right에서 total_width를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.right - margin.right - total_width;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else //SS_LEFT (default)
	{
		if (false)//m_hIcon)
		{
			for (i = 0; i < para.size(); i++)
			{
				for (j = 0; j < para[i].size(); j++)
				{
					//para[i][j].r.OffsetRect(m_margin.left + m_sz_icon.cx + 4, 0);
				}
			}
		}
	}

	if (align & DT_VCENTER)// SS_CENTERIMAGE)
	{
		//전체 높이에서 전체 텍스트 높이 합계를 뺀 1/2 만큼 shift 시킨다.
		sy = (rc.Height() - total_text_height) / 2;
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, sy);
			}
		}
	}
	else //top align
	{
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, 0);// m_margin.top);
			}
		}
	}


	if (para.size() > 0)
	{
		//m_pt_icon.x = m_para[m_max_width_line][0].r.left - m_sz_icon.cx - 4;
		//아이콘을 top 정렬하느냐, 모든 라인의 vcenter에 정렬하느냐...
		//m_pt_icon.y = m_para[0][0].r.top;

		rect_text.left = para[max_width_line][0].r.left;	//최대 넓이 라인의 0번 아이템의 left
		rect_text.top = para[0][0].r.top;					//최상단 항목의 top
		rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;	//최대 넓이 라인의 마지막 항목의 right
		rect_text.bottom = para[0][0].r.top + total_text_height;	//최상단 항목의 top + 전체 텍스트 높이
	}
	else
	{
		//m_pt_icon.x = sx - m_sz_icon.cx;
		//m_pt_icon.y = sy - m_sz_icon.cy / 2;

		//m_rect_text = make_rect(m_pt_icon.x, m_pt_icon.y, m_sz_icon.cx, m_sz_icon.cy);
	}

	//text 크기에 맞춰 컨트롤의 크기를 조정하는 것은 해당 윈도우에서 처리할 일이다.
	/*
	if (false)//m_auto_ctrl_size)
	{
		if (rect_text.Width() > rc.Width() || rect_text.Height() > rc.Height())
		{
			//MoveWindow(m_rect_text);
			SetWindowPos(NULL, 0, 0, rect_text.Width(), rect_text.Height(), SWP_NOMOVE | SWP_NOZORDER);
		}
	}
	*/

	return rect_text;
}

int CSCParagraph::get_max_width_line(std::deque<std::deque<CSCParagraph>>& para)
{
	int i, j;
	int max_width = 0;
	int max_width_line = -1;

	for (i = 0; i < para.size(); i++)
	{
		int line_width = 0;

		for (j = 0; j < para[i].size(); j++)
			line_width += para[i][j].r.Width();

		if (line_width > max_width)
		{
			max_width = line_width;
			max_width_line = i;
		}
	}

	return max_width_line;
}

void CSCParagraph::get_paragraph_font(Gdiplus::Graphics& g, Gdiplus::Font** font)
{
	Gdiplus::Unit unit = g.GetPageUnit();
	float fDpiX = g.GetDpiX();
	float fDpiY = g.GetDpiY();

	int logPixelsY = ::GetDeviceCaps(NULL, LOGPIXELSY);
	float emSize = fDpiY * text_prop.size / 96.0;

	Gdiplus::FontFamily fontFamily((WCHAR*)(const WCHAR*)CStringW(text_prop.name));

	std::unique_ptr<Gdiplus::Font> ff;

	if (fontFamily.IsAvailable())
		ff = std::make_unique<Gdiplus::Font>(&fontFamily, emSize, text_prop.style);
	else
		ff = std::make_unique<Gdiplus::Font>(Gdiplus::FontFamily::GenericSansSerif(), emSize, text_prop.style);

	*font = ff->Clone();
}

/*
//ex. add("<font size=12><font bold = 1>first line") 입력된 후
//add("second line")이면 이 때 스타일은 전 스타일값을 그대로 유지한다.
void CSCParagraph::add(CString data)
{
	CSCParagraphData	para_data;
	int font_size = get_value(data, _T("font size"));
}

//tag_name이 실제 문자열 데이터에도 있을 수 있으므로 반드시 <, > 기호로 묶여있음을 확인해야 한다.
DWORD CSCParagraph::get_value(CString data, CString tag_name, int start)
{
	int tag_pos = data.Find(tag_name, start);
	if (tag_pos < 0)
		return -1;

	//< font size=1>과 같이 '<'과 tag_name 사이의 공백 제거
	int tag_start = tag_pos - 1;
	while (tag_start >= 0)
	{
		if (data[tag_start] == '<')
			break;
		else
			tag_start--;
	}

	if (tag_start < 0)
		return -1;

	int tag_end = data.Find('>', tag_start + 1);
	if (tag_end < 0)
		return -1;

	//끝 태그부터 역으로 '='기호까지 찾아간다.
	int value_start = tag_end;
	while (value_start >= 0)
	{
		if (data[value_start] == '=')
			break;
		else
			value_start--;
	}

	if (value_start <= 0)
		return -1;

	CString value;
	//value = data.Mid(value_start + 1, tag_end - )
}
*/

void CSCParagraph::draw_text(Gdiplus::Graphics& g, std::deque<std::deque<CSCParagraph>>& para)
{
	int i, j;
	CFont font, * pOldFont = NULL;

	Gdiplus::StringFormat sf;

	//AntiAlias, TextRenderingHint 등은 미리 g에 세팅되서 넘어와야 한다.
	//무조건 여기서 이를 고정하여 출력하게 되면 일부 텍스트는 antiAlias로 인해 흐리게 표시된다.
	//g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	//g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	//g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	Gdiplus::Unit unit = g.GetPageUnit();
	float fDpiX = g.GetDpiX();
	float fDpiY = g.GetDpiY();

	int logPixelsY = ::GetDeviceCaps(NULL, LOGPIXELSY);
	//Gdiplus::REAL emSize = (Gdiplus::REAL)MulDiv(font_size, 96, logPixelsY);

	//g를 이용해서 pDC를 구해서 사용하는 경우는 g.ReleaseHDC(hdc);를 호출하기 전까지는 g의 어떤 함수 사용도 하지 않아야 한다.
	// Make GDI calls, but don't call any methods
	// on g until after the call to ReleaseHDC.
#ifdef USING_HDC
	HDC hdc = g.GetHDC();
	CDC* pDC = CDC::FromHandle(hdc);
	pDC->SetBkMode(TRANSPARENT);
#endif
	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

	//dc.SetBkColor()로 지정된 배경색을 설정하면 편하지만
	//글자 속성에 따라 그 높낮이가 다른 경우도 있다.
	//따라서 calc_text_rect()에서 max height를 모든 paragraph에 적용했으며
	//여기서도 배경색으로 칠한 뒤 텍스트를 표시한다.

	for (i = 0; i < para.size(); i++)
	{
		//각 항목을 출력하되 뒤에서부터 출력시킨다.
		//이는 italic인 경우 다음 항목에 의해 일부 가려지는 현상을 방지하기 위함이다.
		for (j = para[i].size() - 1; j >= 0; j--)
		{
#ifdef USING_HDC
			pOldFont = select_paragraph_font(pDC, para, i, j, lf, &font);

			//text 배경색을 칠하고
			if (para[i][j].cr_back.GetA() != 0)
				pDC->FillSolidRect(para[i][j].r, para[i][j].cr_back.ToCOLORREF());

			//text를 출력한다.
			pDC->SetTextColor(para[i][j].cr_text.ToCOLORREF());
			pDC->DrawText(para[i][j].text, para[i][j].r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
			//draw_text(g, m_para[i][j].r, m_para[i][j].cr_text, m_para[i][j].text, m_para[i][j].size, m_para[i][j].);
			//g.DrawString(CStringW(m_para[i][j].text), m_para[i][j].text.GetLength(), font, Gdiplus::PointF((Gdiplus::REAL)m_para[i][j].r.left, (Gdiplus::REAL)m_para[i][j].r.top), &sf);
			pDC->SelectObject(pOldFont);
#else
			//text 배경색을 칠하고
			draw_rect(g, para[i][j].r, Gdiplus::Color::Transparent, para[i][j].text_prop.cr_back);

			Gdiplus::FontFamily* fontFamily = new Gdiplus::FontFamily((WCHAR*)(const WCHAR*)CStringW(para[i][j].text_prop.name));

			//시스템에 등록되지 않은 폰트를 설정할 경우 ff는 null이므로 기본 폰트로라도 대체시켜야 한다.
			if (!fontFamily->IsAvailable())
			{
				delete fontFamily;
				fontFamily = Gdiplus::FontFamily::GenericSansSerif()->Clone();
			}

			Gdiplus::Font* font = NULL;
			para[i][j].get_paragraph_font(g, &font);
			//float emSize = fDpiY * para[i][j].text_prop.size / 96.0;
			//Gdiplus::Font font(&ff, emSize, para[i][j].text_prop.style);


			//text를 출력한다.
			Gdiplus::SolidBrush text_brush(para[i][j].text_prop.cr_text);

			//GraphicsPath를 이용하면 stroke, shadow 등 다양한 효과를 구현할 수 있지만
			//DrawString()보다 글자가 선명하게 보이지 않는 단점이 있다.
			//stroke, shadow를 아예 사용하지 않을 경우는 DrawString()으로 그려준다.
			if (para[i][j].text_prop.shadow_depth == 0 && para[i][j].text_prop.thickness == 0)
			{
				g.DrawString(CStringW(para[i][j].text), -1, font,
					//Gdiplus::PointF((Gdiplus::REAL)para[i][j].r.left, (Gdiplus::REAL)para[i][j].r.top), &sf, &text_brush);
					Gdiplus::PointF((Gdiplus::REAL)para[i][j].r.left, (Gdiplus::REAL)para[i][j].r.top), sf.GenericTypographic(), &text_brush);
			}
			else
			{
				float emSize = fDpiY * para[i][j].text_prop.size / 72.0;
				Gdiplus::GraphicsPath str_path, shadow_path;

				//겹치는 부분을 반전시키지 않는다. FillModeAlternate는 반전시킴.
				str_path.SetFillMode(Gdiplus::FillModeWinding);
				shadow_path.SetFillMode(Gdiplus::FillModeWinding);

				//AddString() 파라미터 중 출력위치를 줄 때 Gdiplus::Rect() 또는 Gdiplus::Point()로 줄 수 있는데
				//stroke 또는 shadow가 추가되어 r이 작으면 텍스트가 출력되지 않는 현상이 있다.
				//r을 정확히 계산하는 것이 정석이나 굳이 r을 주지 않고 Gdiplus::Point()로 주면 문제되지 않는다.
				CRect r = para[i][j].r;
				str_path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), fontFamily,
					para[i][j].text_prop.style, emSize, Gdiplus::Point(r.left, r.top), sf.GenericTypographic());

				//그림자의 깊이는 텍스트 height에 따라 비례하고 stroke의 thickness 유무와도 관계있다
				Gdiplus::SolidBrush br_shadow(para[0][0].text_prop.cr_shadow);

				
				CPoint pt_shadow_offset;
				pt_shadow_offset.x = max((float)(para[i][j].r.Height()) / 30.0f, 2.0f);
				pt_shadow_offset.x = max(pt_shadow_offset.x, para[i][j].text_prop.thickness / 1.4f);
				pt_shadow_offset.y = max((float)(para[i][j].r.Height()) / 30.0f, 2.0f);
				pt_shadow_offset.y = max(pt_shadow_offset.y, para[i][j].text_prop.thickness / 1.4f);
				r.OffsetRect(pt_shadow_offset.x, pt_shadow_offset.y);

				shadow_path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), fontFamily,
					para[i][j].text_prop.style, emSize, Gdiplus::Point(r.left, r.top), sf.GenericTypographic());

				Gdiplus::Pen   pen(para[i][j].text_prop.cr_stroke, para[i][j].text_prop.thickness);
				Gdiplus::SolidBrush brush(para[i][j].text_prop.cr_text);

				//pen.SetLineJoin(Gdiplus::LineJoinMiter);
				pen.SetLineJoin(Gdiplus::LineJoinRound);

				g.FillPath(&br_shadow, &shadow_path);

				//thickness가 0.0f이면 g.DrawPath()가 아닌 g.DrawString()으로 그리면 되고 이전 버전은 잘 그려졌으나
				//뭔가 옵셋이 틀어진 현상이 발생하여 우선 아래와 같이 조건에 의해 g.DrawPath()를 실행하도록 한다.
				if (para[i][j].text_prop.thickness > 0.0f)
					g.DrawPath(&pen, &str_path);

				g.FillPath(&brush, &str_path);
			}
			/*
			Gdiplus::Pen   pen(para[i][j].text_prop.cr_stroke, para[i][j].text_prop.thickness);
			Gdiplus::SolidBrush text_brush(para[i][j].text_prop.cr_text);

			//g.DrawString(CStringW(para[i][j].text), para[i][j].text.GetLength(), &font,
			//			 Gdiplus::PointF((Gdiplus::REAL)para[i][j].r.left, (Gdiplus::REAL)para[i][j].r.top), sf.GenericTypographic(), &text_brush);
			g.DrawString(CStringW(para[i][j].text), -1, &font,
				Gdiplus::PointF((Gdiplus::REAL)para[i][j].r.left, (Gdiplus::REAL)para[i][j].r.top), sf.GenericTypographic(), &text_brush);
			*/
#endif

			//각 para 영역 확인용 코드
#ifdef _DEBUG
			//"\n"에 의한 공백 라인은 영역 사각형을 굳이 표시하지 않는다.
			//if (para[i][j].r.Width() > 2)
				//draw_rect(g, para[i][j].r, Gdiplus::Color::Blue);// , Gdiplus::Color(255, 255, 0, 0));
#endif
			if (font)
				delete font;

			if (fontFamily)
				delete fontFamily;
		}
	}

	//텍스트 출력 영역 확인용
#ifdef _DEBUG
	//draw_rect(g, m_rect_text, Gdiplus::Color::Blue, Gdiplus::Color::Transparent, 1);
#endif
	//TRACE(_T("m_rect_text = %s\n"), get_rect_info_string(m_rect_text));

#ifdef USING_HDC
	font.DeleteObject();
	pDC->SelectObject(pOldFont);

	g.ReleaseHDC(hdc);
#endif
}

//calc_text_rect()에서 이미 각 paragraph의 r이 align에 따라 정해지지만 이를 동적으로 변경하고자 할 경우 호출.
CRect CSCParagraph::set_text_align(CRect rc, std::deque<std::deque<CSCParagraph>>& para, DWORD align)
{
	int i, j;
	int sx, sy;
	int total_text_height = 0;
	CRect margin;	//추후 static 멤버변수로 분리할 예정

	if (align & DT_CENTER)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//아이콘을 포함하여 center에 표시할 지, 텍스트만 center에 표시할 지...
			//if (m_hIcon)
			//	total_width -= (m_sz_icon.cx + 4);

			//cx에서 total_width/2를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.CenterPoint().x - total_width / 2;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else if (align & DT_RIGHT)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//rc.right에서 total_width를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.right - margin.right - total_width;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else //SS_LEFT (default)
	{
		if (false)//m_hIcon)
		{
			for (i = 0; i < para.size(); i++)
			{
				for (j = 0; j < para[i].size(); j++)
				{
					//para[i][j].r.OffsetRect(m_margin.left + m_sz_icon.cx + 4, 0);
				}
			}
		}
	}

	if (align & DT_VCENTER)// SS_CENTERIMAGE)
	{
		//전체 높이에서 전체 텍스트 높이 합계를 뺀 1/2 만큼 shift 시킨다.
		sy = (rc.Height() - total_text_height) / 2;
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, sy);
			}
		}
	}
	else //top align
	{
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, 0);// m_margin.top);
			}
		}
	}

	CRect rect_text;
	int max_width_line = get_max_width_line(para);

	if (para.size() > 0)
	{
		rect_text.left = para[max_width_line][0].r.left;	//최대 넓이 라인의 0번 아이템의 left
		rect_text.top = para[0][0].r.top;					//최상단 항목의 top
		rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;	//최대 넓이 라인의 마지막 항목의 right
		rect_text.bottom = para[0][0].r.top + total_text_height;	//최상단 항목의 top + 전체 텍스트 높이
	}

	return rect_text;
}

//para의 정보를 문자열로 리턴한다.
CString	CSCParagraph::get_paragraph_info_string(std::deque<std::deque<CSCParagraph>>& para)
{
	int i, j;
	CString str;
	CString info;

	for (i = 0; i < para.size(); i++)
	{
		for (j = 0; j < para[i].size(); j++)
		{
			str.Format(_T("(%d, %d) : text = %s, r = %s, name = %s, size = %.1f, style = %d\n"),
				i, j,
				para[i][j].text,
				get_rect_info_string(para[i][j].r),
				para[i][j].text_prop.name,
				para[i][j].text_prop.size,
				para[i][j].text_prop.style);

			info += str;
		}
	}

	TRACE(_T("%s\n"), info);
	return info;
}
