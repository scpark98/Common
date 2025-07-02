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

	//"<b><cr = Red>This</b></cr > is a <cr =Blue><i>sample</i> <b>paragraph</b>."
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
		else if (tags[i].Find(_T("<fontname=")) >= 0)
		{
			CString str_font = tags[i].Mid(10, tags[i].GetLength() - 11);
			para_temp.text_prop.name = str_font;
		}
		else if (tags[i].Find(_T("</f>")) >= 0 || tags[i].Find(_T("</font>")) >= 0 || tags[i].Find(_T("</fontname>")) >= 0)
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
		else if (tags[i].Find(_T("<fontsize=")) >= 0)
		{
			CString str_size = tags[i].Mid(10, tags[i].GetLength() - 11);
			para_temp.text_prop.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("</sz>")) >= 0 || tags[i].Find(_T("</size>")) >= 0 || tags[i].Find(_T("</fontsize>")) >= 0)
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

void CSCParagraph::get_paragraph_font(Gdiplus::Graphics& g, Gdiplus::Font** font)
{
	Gdiplus::Unit unit = g.GetPageUnit();
	float fDpiX = g.GetDpiX();
	float fDpiY = g.GetDpiY();

	int logPixelsY = ::GetDeviceCaps(NULL, LOGPIXELSY);
	float emSize = fDpiY * text_prop.size / 96.0;

	Gdiplus::FontFamily fontFamily((WCHAR*)(const WCHAR*)CStringW(text_prop.name));

	Gdiplus::Font ff(&fontFamily, emSize, text_prop.style);
	*font = ff.Clone();
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