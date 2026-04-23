#include "SCParagraph.h"
#include "../../Functions.h"

CSCParagraph::CSCParagraph()
{

}

CSCParagraph::~CSCParagraph()
{
}

//text占쏙옙 占승그몌옙 占식쏙옙占싹울옙 占쏙옙 占쏙옙占쏙옙占쏙옙 占쌈쇽옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙 para占쏙옙 占쏙옙占쏙옙占싼댐옙.
//cr_text, cr_back占쏙옙 占쏙옙占쏙옙, 占쏙옙占?占썩본占쏙옙
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

	//占쏙옙占쏙옙 占승그몌옙 占쏙옙占쏙옙占쏙옙 占쌈쇽옙占쏙옙 占쏙옙占쏙옙占싹곤옙
	//占쌤쇽옙 占쌔쏙옙트占쏙옙 占쏙옙占쏙옙占쏙옙 占쌔댐옙 占쌈쇽옙占쏙옙 占쌉뀐옙 paragraph占쏙옙 push占싹곤옙
	//占쏙옙 占승그몌옙 占쏙옙占쏙옙占쏙옙 占쏙옙 占승그곤옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싹곤옙 占쏙옙 占승그몌옙 占쏙옙占쏙옙占쏙옙 占쏙옙 占쌈쇽옙占쏙옙 占쏙옙占쏙옙占싼댐옙. 
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
			_tcscpy_s(para_temp.text_prop.name, str_font);
		}
		else if (tags[i].Find(_T("<font=")) >= 0)
		{
			CString str_font = tags[i].Mid(6, tags[i].GetLength() - 7);
			_tcscpy_s(para_temp.text_prop.name, str_font);
		}
		else if (tags[i].Find(_T("<name=")) >= 0)
		{
			CString str_font = tags[i].Mid(6, tags[i].GetLength() - 7);
			_tcscpy_s(para_temp.text_prop.name, str_font);
		}
		else if (tags[i].Find(_T("<font_name=")) >= 0)
		{
			CString str_font = tags[i].Mid(11, tags[i].GetLength() - 12);
			_tcscpy_s(para_temp.text_prop.name, str_font);
		}
		else if (tags[i].Find(_T("</f>")) >= 0 || tags[i].Find(_T("</font>")) >= 0 || tags[i].Find(_T("</name>")) >= 0 || tags[i].Find(_T("</font_name>")) >= 0)
		{
			_tcscpy_s(para_temp.text_prop.name, basic_para.text_prop.name);
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
			//<br>占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쌩곤옙占쏙옙 占쏙옙占?
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

			//占쏙옙체 占쏙옙占쏙옙占쏙옙 cr_back占쏙옙 Transparent占쏙옙 占싣댐옙 占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙 para[][].cr_back占쏙옙 cr_back占쏙옙 占쏙옙 占쏙옙占?占쌩븝옙占쏙옙占쏙옙 占쌓뤄옙占쏙옙占쏙옙 占싫댐옙.
			//cr_back占쏙옙 占쏙옙占쏙옙占쏙옙占싱몌옙 占쏙옙占쏙옙消?占쌓뤄옙占쏙옙占쏙옙 표占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占싱몌옙 占쏙옙占쏙옙占쏙옙 占쌓뤄옙占쏙옙占쏙옙 占싫댐옙.
			if (para_temp.text_prop.cr_back.GetValue() == basic_para.text_prop.cr_back.GetValue())
				para_temp.text_prop.cr_back = Gdiplus::Color::Transparent;

			para_line.push_back(para_temp);
		}
	}

	if (para_line.size())
		para.push_back(para_line);
}

//paragraph text 占쏙옙占쏙옙占쏙옙 dc占쏙옙 占쏙옙占쏙옙占?占쏙옙 占쏙옙占?크占썩를 占쏙옙占쏙옙構占?占쏙옙 占쌔쏙옙트占쏙옙 占쏙옙쨉占?占쏙옙치占쏙옙占쏙옙 CSCParagraph 占쏙옙占쏙옙占?占쏙옙占쏙옙占싼댐옙.
CRect CSCParagraph::calc_text_rect(CRect rc, CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, DWORD align)
{
	if (para.empty())
		return CRect();

	int i, j;
	int sx = 0;
	int sy = 0;				//占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙치(占쏙옙占싱곤옙 占쏙옙占쏙옙)
	int total_text_height;
	CRect rect_text;
	CFont font;

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

			//GetTextExtent()占쏙옙 DrawText(DT_CALCRECT)占쏙옙 占쏙옙占쏙옙 크占쏙옙占?占쏙옙占쏙옙占싹몌옙 italic占쏙옙 占썅간 占쌩몌옙.
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

			//"text...    "占쏙옙 占쏙옙占쏙옙 占쌘울옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙占?占쏙옙占쏙옙占쏙옙 占쏙옙占시되억옙 占쏙옙쨉占쏙옙占?占십는댐옙.
			//占쏙옙占?. 占쏙옙 占쏙옙占쏙옙 "|"占쏙옙 占쏙옙占쏙옙 占쏙옙占쌘몌옙 占쌍억옙 占쏙옙占쏙옙占?占쏙옙 "|"占쏙옙 width占쏙옙 占폗占쏙옙.
			//占쏙옙占?. SetMeasurableCharacterRanges(), MeasureCharacterRanges() 占쏙옙占쏙옙 占싱울옙占싹댐옙 占쏙옙占쏙옙 占쏙옙 占쏙옙占쏙옙
			//占쎌선 占쏙옙占쏙옙占싹곤옙 1占쏙옙 占쏙옙占쏙옙占?占쏙옙占쏙옙磯占?
			g.MeasureString(CStringW(para[i][j].text + _T("|")), -1, font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &boundRect);
			g.MeasureString(L"|", -1, font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &boundRect_temp);

			if (boundRect.IsEmptyArea())
			{
				boundRect.Width = 1;
				boundRect.Height = 40;
			}

			//stroke 占싸뀐옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 크占썩여占쏙옙 占싼댐옙.
			sz.cx = boundRect.Width - boundRect_temp.Width + para[i][j].text_prop.thickness;// *2.0f;
			sz.cy = boundRect.Height + para[i][j].text_prop.thickness;// *2.0f;
			para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
#endif
			//TRACE(_T("[%d][%d] text = %s, sz = %dx%d, r = %s\n"), i, j, para[i][j].text, sz.cx, sz.cy, get_rect_info_string(para[i][j].r));
			sz_text.cx += sz.cx;

			//占쏙옙 占쏙옙占싸울옙占쏙옙 占쏙옙占쏙옙 cy占쏙옙 큰 占쏙옙占쏙옙 占쏙옙占쏙옙킨占쏙옙.
			sz_text.cy = MAX(sz_text.cy, sz.cy);
		}

		//占쏙옙 占쏙옙占싸듸옙 占쌩울옙占쏙옙 占쌍댐옙 占십븝옙 占쏙옙占싼댐옙.
		if (sz_text.cx > max_width)
		{
			max_width = sz_text.cx;
			max_width_line = i;
		}

		//占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙치占쏙옙 占쏙옙占쏙옙占싫댐옙.
		sy += sz_text.cy;
	}

	total_text_height = sy;

	font.DeleteObject();

	//占쏙옙 占쏙옙占싸놂옙占쏙옙占쏙옙 height占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙 占쌓몌옙占쏙옙占쏙옙 占쏙옙占싹쏙옙키占쏙옙占쏙옙? 占싣니몌옙 占쏙옙占쏙옙占쏙옙 占쏙옙占싱몌옙 占쌓댐옙占?占쏙옙占쏙옙占싹댐옙占쏙옙...
	//for (i = 0; i < m_paragraph.size(); i++)
	//{
	//	m_paragraph[i].r.bottom = m_paragraph[i].r.top + m_sz_text.cy;
	//}

	//align 占심션울옙 占쏙옙占쏙옙 占쏙옙占쏙옙
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

	//align占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙
	if (align & DT_CENTER)
	{
		//占쏙옙 占쏙옙占싸몌옙占쏙옙 total_width占쏙옙 占쏙옙占싹곤옙
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싹울옙 center占쏙옙 표占쏙옙占쏙옙 占쏙옙, 占쌔쏙옙트占쏙옙 center占쏙옙 표占쏙옙占쏙옙 占쏙옙...
			//if (m_hIcon)
			//	total_width -= (m_sz_icon.cx + 4);

			//cx占쏙옙占쏙옙 total_width/2占쏙옙 占쏙옙 占쏙옙치占쏙옙 첫 占쏙옙째 占쌓몌옙占쏙옙 sx占싱므뤄옙 占쏙옙 占쏙옙큼 shift占쏙옙키占쏙옙 占싫댐옙.
			sx = rc.CenterPoint().x - total_width / 2;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else if (align & DT_RIGHT)
	{
		//占쏙옙 占쏙옙占싸몌옙占쏙옙 total_width占쏙옙 占쏙옙占싹곤옙
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//rc.right占쏙옙占쏙옙 total_width占쏙옙 占쏙옙 占쏙옙치占쏙옙 첫 占쏙옙째 占쌓몌옙占쏙옙 sx占싱므뤄옙 占쏙옙 占쏙옙큼 shift占쏙옙키占쏙옙 占싫댐옙.
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
		//占쏙옙체 占쏙옙占싱울옙占쏙옙 占쏙옙체 占쌔쏙옙트 占쏙옙占쏙옙 占쌌계를 占쏙옙 1/2 占쏙옙큼 shift 占쏙옙킨占쏙옙.
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
		//占쏙옙占쏙옙占쏙옙占쏙옙 top 占쏙옙占쏙옙占싹댐옙占쏙옙, 占쏙옙占?占쏙옙占쏙옙占쏙옙 vcenter占쏙옙 占쏙옙占쏙옙占싹댐옙占쏙옙...
		//m_pt_icon.y = m_para[0][0].r.top;

		rect_text.left = para[max_width_line][0].r.left;	//占쌍댐옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 0占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙 left
		rect_text.top = para[0][0].r.top;					//占쌍삼옙占?占쌓몌옙占쏙옙 top
		rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;	//占쌍댐옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쌓몌옙占쏙옙 right
		rect_text.bottom = para[0][0].r.top + total_text_height;	//占쌍삼옙占?占쌓몌옙占쏙옙 top + 占쏙옙체 占쌔쏙옙트 占쏙옙占쏙옙
	}
	else
	{
		//m_pt_icon.x = sx - m_sz_icon.cx;
		//m_pt_icon.y = sy - m_sz_icon.cy / 2;

		//m_rect_text = make_rect(m_pt_icon.x, m_pt_icon.y, m_sz_icon.cx, m_sz_icon.cy);
	}

	//text 크占썩에 占쏙옙占쏙옙 占쏙옙트占쏙옙占쏙옙 크占썩를 占쏙옙占쏙옙占싹댐옙 占쏙옙占쏙옙 占쌔댐옙 占쏙옙占쏙옙占쎌에占쏙옙 처占쏙옙占쏙옙 占쏙옙占싱댐옙.
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

//calc_text_rect()濡?媛?paragraph??r??寃곗젙???댄썑???몄텧.
//?쇱씤 ?ъ씠 媛꾧꺽??議곗젙?쒕떎. 湲???먯껜 ?ш린??洹몃?濡??먭퀬 ?ㅼ쓬 ?쇱씤???쒖옉 ?꾩튂留??꾨옒濡?諛?대궦??
//spacing = 1.0f?대㈃ 蹂寃??놁쓬, 1.5f?대㈃ ?쇱씤 i??max height??0.5諛곕쭔???댄썑 ?쇱씤遺???꾩쟻 shift?쒕떎.
//spacing < 1.0f???덉슜?섎굹 ?뚯닔 shift濡??쇱씤??寃뱀튌 ???덈떎.
CRect CSCParagraph::set_line_spacing(std::deque<std::deque<CSCParagraph>>& para, float spacing)
{
	if (para.empty())
		return CRect();

	int i, j;

	//媛??쇱씤??max height瑜?誘몃━ 援ы븳??
	std::deque<int> line_heights;

	for (i = 0; i < para.size(); i++)
	{
		int line_h = 0;

		for (j = 0; j < para[i].size(); j++)
			line_h = MAX(line_h, para[i][j].r.Height());

		line_heights.push_back(line_h);
	}

	//?쇱씤 0? 洹몃?濡??먭퀬 ?쇱씤 1遺???꾩쟻 shift瑜??곸슜.
	//?쇱씤 i??異붽? shift = sum(line_heights[0..i-1]) * (spacing - 1.0)
	int shift_y = 0;

	for (i = 1; i < (int)para.size(); i++)
	{
		shift_y += (int)((float)line_heights[i - 1] * (spacing - 1.0f));

		for (j = 0; j < para[i].size(); j++)
			para[i][j].r.OffsetRect(0, shift_y);
	}

	//?꾩껜 ?곸뿭 ?곗텧. calc_text_rect()? ?숈씪??諛⑹떇?쇰줈 max_width_line 湲곗?.
	CRect rect_text;
	int max_width_line = get_max_width_line(para);

	if (max_width_line < 0)
		max_width_line = 0;

	rect_text.left = para[max_width_line][0].r.left;
	rect_text.top = para[0][0].r.top;
	rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;

	//bottom? 留덉?留??쇱씤??媛????bottom (shift ?곸슜 寃곌낵 洹몃?濡?.
	int last_line = (int)para.size() - 1;
	int max_bottom = para[last_line][0].r.bottom;

	for (j = 1; j < para[last_line].size(); j++)
		max_bottom = MAX(max_bottom, para[last_line][j].r.bottom);

	rect_text.bottom = max_bottom;

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

	std::unique_ptr<Gdiplus::Font> ff;

	if (fontFamily.IsAvailable())
		ff = std::make_unique<Gdiplus::Font>(&fontFamily, emSize, text_prop.style);
	else
		ff = std::make_unique<Gdiplus::Font>(Gdiplus::FontFamily::GenericSansSerif(), emSize, text_prop.style);

	*font = ff->Clone();
}

/*
//ex. add("<font size=12><font bold = 1>first line") 占쌉력듸옙 占쏙옙
//add("second line")占싱몌옙 占쏙옙 占쏙옙 占쏙옙타占쏙옙占쏙옙 占쏙옙 占쏙옙타占싹곤옙占쏙옙 占쌓댐옙占?占쏙옙占쏙옙占싼댐옙.
void CSCParagraph::add(CString data)
{
	CSCParagraphData	para_data;
	int font_size = get_value(data, _T("font size"));
}

//tag_name占쏙옙 占쏙옙占쏙옙 占쏙옙占쌘울옙 占쏙옙占쏙옙占싶울옙占쏙옙 占쏙옙占쏙옙 占쏙옙 占쏙옙占쏙옙占실뤄옙 占쌥듸옙占?<, > 占쏙옙호占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙 확占쏙옙占쌔억옙 占싼댐옙.
DWORD CSCParagraph::get_value(CString data, CString tag_name, int start)
{
	int tag_pos = data.Find(tag_name, start);
	if (tag_pos < 0)
		return -1;

	//< font size=1>占쏙옙 占쏙옙占쏙옙 '<'占쏙옙 tag_name 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙
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

	//占쏙옙 占승그븝옙占쏙옙 占쏙옙占쏙옙占쏙옙 '='占쏙옙호占쏙옙占쏙옙 찾占싣곤옙占쏙옙.
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

	Gdiplus::Unit unit = g.GetPageUnit();
	float fDpiX = g.GetDpiX();
	float fDpiY = g.GetDpiY();

	int logPixelsY = ::GetDeviceCaps(NULL, LOGPIXELSY);
	//Gdiplus::REAL emSize = (Gdiplus::REAL)MulDiv(font_size, 96, logPixelsY);

	//g占쏙옙 占싱울옙占쌔쇽옙 pDC占쏙옙 占쏙옙占쌔쇽옙 占쏙옙占쏙옙求占?占쏙옙占쏙옙 g.ReleaseHDC(hdc);占쏙옙 호占쏙옙占싹깍옙 占쏙옙占쏙옙占쏙옙占쏙옙 g占쏙옙 占쏘떤 占쌉쇽옙 占쏙옙逾?占쏙옙占쏙옙 占십아억옙 占싼댐옙.
	// Make GDI calls, but don't call any methods
	// on g until after the call to ReleaseHDC.
#ifdef USING_HDC
	HDC hdc = g.GetHDC();
	CDC* pDC = CDC::FromHandle(hdc);
	pDC->SetBkMode(TRANSPARENT);
#endif
	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

	//dc.SetBkColor()占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싹몌옙 占쏙옙占쏙옙占쏙옙占쏙옙
	//占쏙옙占쏙옙 占쌈쇽옙占쏙옙 占쏙옙占쏙옙 占쏙옙 占쏙옙占쏙옙占싱곤옙 占쌕몌옙 占쏙옙理?占쌍댐옙.
	//占쏙옙占쏙옙 calc_text_rect()占쏙옙占쏙옙 max height占쏙옙 占쏙옙占?paragraph占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙
	//占쏙옙占썩서占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙 칠占쏙옙 占쏙옙 占쌔쏙옙트占쏙옙 표占쏙옙占싼댐옙.

	for (i = 0; i < para.size(); i++)
	{
		//占쏙옙 占쌓몌옙占쏙옙 占쏙옙占쏙옙溝占?占쌘울옙占쏙옙占쏙옙占쏙옙 占쏙옙쩍占신뀐옙占?
		//占싱댐옙 italic占쏙옙 占쏙옙占?占쏙옙占쏙옙 占쌓몌옙 占쏙옙占쏙옙 占싹븝옙 占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싹깍옙 占쏙옙占쏙옙占싱댐옙.
		for (j = para[i].size() - 1; j >= 0; j--)
		{
#ifdef USING_HDC
			pOldFont = select_paragraph_font(pDC, para, i, j, lf, &font);

			//text 占쏙옙占쏙옙占쏙옙 칠占싹곤옙
			if (para[i][j].cr_back.GetA() != 0)
				pDC->FillSolidRect(para[i][j].r, para[i][j].cr_back.ToCOLORREF());

			//text占쏙옙 占쏙옙占쏙옙磯占?
			pDC->SetTextColor(para[i][j].cr_text.ToCOLORREF());
			pDC->DrawText(para[i][j].text, para[i][j].r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
			//draw_text(g, m_para[i][j].r, m_para[i][j].cr_text, m_para[i][j].text, m_para[i][j].size, m_para[i][j].);
			//g.DrawString(CStringW(m_para[i][j].text), m_para[i][j].text.GetLength(), font, Gdiplus::PointF((Gdiplus::REAL)m_para[i][j].r.left, (Gdiplus::REAL)m_para[i][j].r.top), &sf);
			pDC->SelectObject(pOldFont);
#else
			//text 占쏙옙占쏙옙占쏙옙 칠占싹곤옙
			draw_rect(g, para[i][j].r, Gdiplus::Color::Transparent, para[i][j].text_prop.cr_back);

			Gdiplus::FontFamily* fontFamily = new Gdiplus::FontFamily((WCHAR*)(const WCHAR*)CStringW(para[i][j].text_prop.name));

			//占시쏙옙占쌜울옙 占쏙옙溝占쏙옙占?占쏙옙占쏙옙 占쏙옙트占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占?ff占쏙옙 null占싱므뤄옙 占썩본 占쏙옙트占싸띰옙 占쏙옙체占쏙옙占싼억옙 占싼댐옙.
			if (!fontFamily->IsAvailable())
			{
				delete fontFamily;
				fontFamily = Gdiplus::FontFamily::GenericSansSerif()->Clone();
			}

			Gdiplus::Font* font = NULL;
			para[i][j].get_paragraph_font(g, &font);
			//float emSize = fDpiY * para[i][j].text_prop.size / 96.0;
			//Gdiplus::Font font(&ff, emSize, para[i][j].text_prop.style);


			//text占쏙옙 占쏙옙占쏙옙磯占?
			Gdiplus::SolidBrush text_brush(para[i][j].text_prop.cr_text);

			//GraphicsPath占쏙옙 占싱울옙占싹몌옙 stroke, shadow 占쏙옙 占쌕억옙占쏙옙 효占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙 占쏙옙占쏙옙占쏙옙
			//DrawString()占쏙옙占쏙옙 占쏙옙占쌘곤옙 占쏙옙占쏙옙占싹곤옙 占쏙옙占쏙옙占쏙옙 占십댐옙 占쏙옙占쏙옙占쏙옙 占쌍댐옙.
			//stroke, shadow占쏙옙 占싣울옙 占쏙옙占쏙옙占쏙옙占?占쏙옙占쏙옙 占쏙옙占쏙옙 DrawString()占쏙옙占쏙옙 占쌓뤄옙占쌔댐옙.
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

				//占쏙옙치占쏙옙 占싸븝옙占쏙옙 占쏙옙占쏙옙占쏙옙키占쏙옙 占십는댐옙. FillModeAlternate占쏙옙 占쏙옙占쏙옙占쏙옙킴.
				str_path.SetFillMode(Gdiplus::FillModeWinding);
				shadow_path.SetFillMode(Gdiplus::FillModeWinding);

				//AddString() 占식띰옙占쏙옙占?占쏙옙 占쏙옙占쏙옙占식∽옙占?占쏙옙 占쏙옙 Gdiplus::Rect() 占실댐옙 Gdiplus::Point()占쏙옙 占쏙옙 占쏙옙 占쌍는듸옙
				//stroke 占실댐옙 shadow占쏙옙 占쌩곤옙占실억옙 r占쏙옙 占쏙옙占쏙옙占쏙옙 占쌔쏙옙트占쏙옙 占쏙옙쨉占쏙옙占?占십댐옙 占쏙옙占쏙옙占쏙옙 占쌍댐옙.
				//r占쏙옙 占쏙옙확占쏙옙 占쏙옙占쏙옙求占?占쏙옙占쏙옙 占쏙옙占쏙옙占싱놂옙 占쏙옙占쏙옙 r占쏙옙 占쏙옙占쏙옙 占십곤옙 Gdiplus::Point()占쏙옙 占쌍몌옙 占쏙옙占쏙옙占쏙옙占쏙옙 占십는댐옙.
				CRect r = para[i][j].r;
				str_path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), fontFamily,
					para[i][j].text_prop.style, emSize, Gdiplus::Point(r.left, r.top), sf.GenericTypographic());

				//占쌓몌옙占쏙옙占쏙옙 占쏙옙占싱댐옙 占쌔쏙옙트 height占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙構占?stroke占쏙옙 thickness 占쏙옙占쏙옙占싶듸옙 占쏙옙占쏙옙占쌍댐옙
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

				//thickness占쏙옙 0.0f占싱몌옙 g.DrawPath()占쏙옙 占싣댐옙 g.DrawString()占쏙옙占쏙옙 占쌓몌옙占쏙옙 占실곤옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙 占쌓뤄옙占쏙옙占쏙옙占쏙옙
				//占쏙옙占쏙옙 占심쇽옙占쏙옙 틀占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쌩삼옙占싹울옙 占쎌선 占싣뤄옙占쏙옙 占쏙옙占쏙옙 占쏙옙占실울옙 占쏙옙占쏙옙 g.DrawPath()占쏙옙 占쏙옙占쏙옙占싹듸옙占쏙옙 占싼댐옙.
				if (para[i][j].text_prop.thickness > 0.0f)
					g.DrawPath(&pen, &str_path);

				g.FillPath(&brush, &str_path);
			}
#endif

			//占쏙옙 para 占쏙옙占쏙옙 확占싸울옙 占쌘듸옙
#ifdef _DEBUG
			//"\n"占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙 占썹각占쏙옙占쏙옙 占쏙옙占쏙옙 표占쏙옙占쏙옙占쏙옙 占십는댐옙.
			//if (para[i][j].r.Width() > 2)
				//draw_rect(g, para[i][j].r, Gdiplus::Color::Blue);// , Gdiplus::Color(255, 255, 0, 0));
#endif
			if (font)
				delete font;

			if (fontFamily)
				delete fontFamily;
		}
	}

	//占쌔쏙옙트 占쏙옙占?占쏙옙占쏙옙 확占싸울옙
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

//calc_text_rect()占쏙옙占쏙옙 占싱뱄옙 占쏙옙 paragraph占쏙옙 r占쏙옙 align占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙 占싱몌옙 占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싹곤옙占쏙옙 占쏙옙 占쏙옙占?호占쏙옙.
CRect CSCParagraph::set_text_align(CRect rc, std::deque<std::deque<CSCParagraph>>& para, DWORD align)
{
	int i, j;
	int sx, sy;
	int total_text_height = 0;
	CRect margin;	//占쏙옙占쏙옙 static 占쏙옙占쏙옙占쏙옙占쏙옙占?占싻몌옙占쏙옙 占쏙옙占쏙옙

	if (align & DT_CENTER)
	{
		//占쏙옙 占쏙옙占싸몌옙占쏙옙 total_width占쏙옙 占쏙옙占싹곤옙
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싹울옙 center占쏙옙 표占쏙옙占쏙옙 占쏙옙, 占쌔쏙옙트占쏙옙 center占쏙옙 표占쏙옙占쏙옙 占쏙옙...
			//if (m_hIcon)
			//	total_width -= (m_sz_icon.cx + 4);

			//cx占쏙옙占쏙옙 total_width/2占쏙옙 占쏙옙 占쏙옙치占쏙옙 첫 占쏙옙째 占쌓몌옙占쏙옙 sx占싱므뤄옙 占쏙옙 占쏙옙큼 shift占쏙옙키占쏙옙 占싫댐옙.
			sx = rc.CenterPoint().x - total_width / 2;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else if (align & DT_RIGHT)
	{
		//占쏙옙 占쏙옙占싸몌옙占쏙옙 total_width占쏙옙 占쏙옙占싹곤옙
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//rc.right占쏙옙占쏙옙 total_width占쏙옙 占쏙옙 占쏙옙치占쏙옙 첫 占쏙옙째 占쌓몌옙占쏙옙 sx占싱므뤄옙 占쏙옙 占쏙옙큼 shift占쏙옙키占쏙옙 占싫댐옙.
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
		//占쏙옙체 占쏙옙占싱울옙占쏙옙 占쏙옙체 占쌔쏙옙트 占쏙옙占쏙옙 占쌌계를 占쏙옙 1/2 占쏙옙큼 shift 占쏙옙킨占쏙옙.
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
		rect_text.left = para[max_width_line][0].r.left;	//占쌍댐옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 0占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙 left
		rect_text.top = para[0][0].r.top;					//占쌍삼옙占?占쌓몌옙占쏙옙 top
		rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;	//占쌍댐옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쌓몌옙占쏙옙 right
		rect_text.bottom = para[0][0].r.top + total_text_height;	//占쌍삼옙占?占쌓몌옙占쏙옙 top + 占쏙옙체 占쌔쏙옙트 占쏙옙占쏙옙
	}

	return rect_text;
}

//占쌔쏙옙트 占쏙옙占쏙옙占승울옙 占쏙옙占쏙옙
//void CSCParagraph::set_margin(std::deque<std::deque<CSCParagraph>>& para, float margin)
//{
//	para.text_prop.
//}

//para占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쌘울옙占쏙옙 占쏙옙占쏙옙占싼댐옙.
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
				get_rect_info_str(para[i][j].r),
				para[i][j].text_prop.name,
				para[i][j].text_prop.size,
				para[i][j].text_prop.style);

			info += str;
		}
	}

	TRACE(_T("%s\n"), info);
	return info;
}
