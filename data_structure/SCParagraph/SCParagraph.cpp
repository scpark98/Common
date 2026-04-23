#include "SCParagraph.h"
#include "../../Functions.h"

CSCParagraph::CSCParagraph()
{

}

CSCParagraph::~CSCParagraph()
{
}

//text�� �±׸� �Ľ��Ͽ� �� ������ �Ӽ��� ������ �� para�� �����Ѵ�.
//cr_text, cr_back�� ����, ��� �⺻��
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

	//���� �±׸� ������ �Ӽ��� �����ϰ�
	//�ܼ� �ؽ�Ʈ�� ������ �ش� �Ӽ��� �Բ� paragraph�� push�ϰ�
	//�� �±׸� ������ �� �±װ� ���������� �����ϰ� �� �±׸� ������ �� �Ӽ��� �����Ѵ�. 
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
			//<br>�� ���� ���� ������ �߰��� ���
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

			//��ü ������ cr_back�� Transparent�� �ƴ� �� ������ ������ �������� ���� para[][].cr_back�� cr_back�� �� ��� �ߺ����� �׷����� �ȴ�.
			//cr_back�� �������̸� ����Ἥ �׷����� ǥ�� ���� ������ �������̸� ������ �׷����� �ȴ�.
			if (para_temp.text_prop.cr_back.GetValue() == basic_para.text_prop.cr_back.GetValue())
				para_temp.text_prop.cr_back = Gdiplus::Color::Transparent;

			para_line.push_back(para_temp);
		}
	}

	if (para_line.size())
		para.push_back(para_line);
}

//paragraph text ������ dc�� ����� �� ��� ũ�⸦ ����ϰ� �� �ؽ�Ʈ�� ��µ� ��ġ���� CSCParagraph ����� �����Ѵ�.
CRect CSCParagraph::calc_text_rect(CRect rc, CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, DWORD align)
{
	if (para.empty())
		return CRect();

	int i, j;
	int sx = 0;
	int sy = 0;				//�� ������ ���� ��ġ(���̰� ����)
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

			//GetTextExtent()�� DrawText(DT_CALCRECT)�� ���� ũ��� �����ϸ� italic�� �ణ �߸�.
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

			//"text...    "�� ���� �ڿ� ������ ���� ��� ������ ���õǾ� ��µ��� �ʴ´�.
			//���1. �� ���� "|"�� ���� ���ڸ� �־� ����� �� "|"�� width�� �A��.
			//���2. SetMeasurableCharacterRanges(), MeasureCharacterRanges() ���� �̿��ϴ� ���� �� ����
			//�켱 �����ϰ� 1�� ����� ����Ѵ�.
			g.MeasureString(CStringW(para[i][j].text + _T("|")), -1, font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &boundRect);
			g.MeasureString(L"|", -1, font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &boundRect_temp);

			if (boundRect.IsEmptyArea())
			{
				boundRect.Width = 1;
				boundRect.Height = 40;
			}

			//stroke �β����� ������ ũ�⿩�� �Ѵ�.
			sz.cx = boundRect.Width - boundRect_temp.Width + para[i][j].text_prop.thickness;// *2.0f;
			sz.cy = boundRect.Height + para[i][j].text_prop.thickness;// *2.0f;
			para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
#endif
			//TRACE(_T("[%d][%d] text = %s, sz = %dx%d, r = %s\n"), i, j, para[i][j].text, sz.cx, sz.cy, get_rect_info_string(para[i][j].r));
			sz_text.cx += sz.cx;

			//�� ���ο��� ���� cy�� ū ���� ����Ų��.
			sz_text.cy = MAX(sz_text.cy, sz.cy);
		}

		//�� ���ε� �߿��� �ִ� �ʺ� ���Ѵ�.
		if (sz_text.cx > max_width)
		{
			max_width = sz_text.cx;
			max_width_line = i;
		}

		//�� ���� ���� ��ġ�� �����ȴ�.
		sy += sz_text.cy;
	}

	total_text_height = sy;

	font.DeleteObject();

	//�� ���γ����� height�� ���� ���� �׸����� ���Ͻ�Ű����? �ƴϸ� ������ ���̸� �״�� �����ϴ���...
	//for (i = 0; i < m_paragraph.size(); i++)
	//{
	//	m_paragraph[i].r.bottom = m_paragraph[i].r.top + m_sz_text.cy;
	//}

	//align �ɼǿ� ���� ����
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

	//align�� ���� ����
	if (align & DT_CENTER)
	{
		//�� ���θ��� total_width�� ���ϰ�
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//�������� �����Ͽ� center�� ǥ���� ��, �ؽ�Ʈ�� center�� ǥ���� ��...
			//if (m_hIcon)
			//	total_width -= (m_sz_icon.cx + 4);

			//cx���� total_width/2�� �� ��ġ�� ù ��° �׸��� sx�̹Ƿ� �� ��ŭ shift��Ű�� �ȴ�.
			sx = rc.CenterPoint().x - total_width / 2;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else if (align & DT_RIGHT)
	{
		//�� ���θ��� total_width�� ���ϰ�
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//rc.right���� total_width�� �� ��ġ�� ù ��° �׸��� sx�̹Ƿ� �� ��ŭ shift��Ű�� �ȴ�.
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
		//��ü ���̿��� ��ü �ؽ�Ʈ ���� �հ踦 �� 1/2 ��ŭ shift ��Ų��.
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
		//�������� top �����ϴ���, ��� ������ vcenter�� �����ϴ���...
		//m_pt_icon.y = m_para[0][0].r.top;

		rect_text.left = para[max_width_line][0].r.left;	//�ִ� ���� ������ 0�� �������� left
		rect_text.top = para[0][0].r.top;					//�ֻ�� �׸��� top
		rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;	//�ִ� ���� ������ ������ �׸��� right
		rect_text.bottom = para[0][0].r.top + total_text_height;	//�ֻ�� �׸��� top + ��ü �ؽ�Ʈ ����
	}
	else
	{
		//m_pt_icon.x = sx - m_sz_icon.cx;
		//m_pt_icon.y = sy - m_sz_icon.cy / 2;

		//m_rect_text = make_rect(m_pt_icon.x, m_pt_icon.y, m_sz_icon.cx, m_sz_icon.cy);
	}

	//text ũ�⿡ ���� ��Ʈ���� ũ�⸦ �����ϴ� ���� �ش� �����쿡�� ó���� ���̴�.
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

//calc_text_rect()로 각 paragraph의 r이 결정된 이후에 호출.
//라인 사이 간격을 조정한다. 글자 자체 크기는 그대로 두고 다음 라인의 시작 위치만 아래로 밀어낸다.
//spacing = 1.0f이면 변경 없음, 1.5f이면 라인 i의 max height의 0.5배만큼 이후 라인부터 누적 shift된다.
//spacing < 1.0f도 허용하나 음수 shift로 라인이 겹칠 수 있다.
CRect CSCParagraph::set_line_spacing(std::deque<std::deque<CSCParagraph>>& para, float spacing)
{
	if (para.empty())
		return CRect();

	int i, j;

	//각 라인의 max height를 미리 구한다.
	std::deque<int> line_heights;

	for (i = 0; i < para.size(); i++)
	{
		int line_h = 0;

		for (j = 0; j < para[i].size(); j++)
			line_h = MAX(line_h, para[i][j].r.Height());

		line_heights.push_back(line_h);
	}

	//라인 0은 그대로 두고 라인 1부터 누적 shift를 적용.
	//라인 i의 추가 shift = sum(line_heights[0..i-1]) * (spacing - 1.0)
	int shift_y = 0;

	for (i = 1; i < (int)para.size(); i++)
	{
		shift_y += (int)((float)line_heights[i - 1] * (spacing - 1.0f));

		for (j = 0; j < para[i].size(); j++)
			para[i][j].r.OffsetRect(0, shift_y);
	}

	//전체 영역 산출. calc_text_rect()와 동일한 방식으로 max_width_line 기준.
	CRect rect_text;
	int max_width_line = get_max_width_line(para);

	if (max_width_line < 0)
		max_width_line = 0;

	rect_text.left = para[max_width_line][0].r.left;
	rect_text.top = para[0][0].r.top;
	rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;

	//bottom은 마지막 라인의 가장 큰 bottom (shift 적용 결과 그대로).
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
//ex. add("<font size=12><font bold = 1>first line") �Էµ� ��
//add("second line")�̸� �� �� ��Ÿ���� �� ��Ÿ�ϰ��� �״�� �����Ѵ�.
void CSCParagraph::add(CString data)
{
	CSCParagraphData	para_data;
	int font_size = get_value(data, _T("font size"));
}

//tag_name�� ���� ���ڿ� �����Ϳ��� ���� �� �����Ƿ� �ݵ�� <, > ��ȣ�� ���������� Ȯ���ؾ� �Ѵ�.
DWORD CSCParagraph::get_value(CString data, CString tag_name, int start)
{
	int tag_pos = data.Find(tag_name, start);
	if (tag_pos < 0)
		return -1;

	//< font size=1>�� ���� '<'�� tag_name ������ ���� ����
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

	//�� �±׺��� ������ '='��ȣ���� ã�ư���.
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

	//g�� �̿��ؼ� pDC�� ���ؼ� ����ϴ� ���� g.ReleaseHDC(hdc);�� ȣ���ϱ� �������� g�� � �Լ� ��뵵 ���� �ʾƾ� �Ѵ�.
	// Make GDI calls, but don't call any methods
	// on g until after the call to ReleaseHDC.
#ifdef USING_HDC
	HDC hdc = g.GetHDC();
	CDC* pDC = CDC::FromHandle(hdc);
	pDC->SetBkMode(TRANSPARENT);
#endif
	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

	//dc.SetBkColor()�� ������ ������ �����ϸ� ��������
	//���� �Ӽ��� ���� �� �����̰� �ٸ� ��쵵 �ִ�.
	//���� calc_text_rect()���� max height�� ��� paragraph�� ����������
	//���⼭�� �������� ĥ�� �� �ؽ�Ʈ�� ǥ���Ѵ�.

	for (i = 0; i < para.size(); i++)
	{
		//�� �׸��� ����ϵ� �ڿ������� ��½�Ų��.
		//�̴� italic�� ��� ���� �׸� ���� �Ϻ� �������� ������ �����ϱ� �����̴�.
		for (j = para[i].size() - 1; j >= 0; j--)
		{
#ifdef USING_HDC
			pOldFont = select_paragraph_font(pDC, para, i, j, lf, &font);

			//text ������ ĥ�ϰ�
			if (para[i][j].cr_back.GetA() != 0)
				pDC->FillSolidRect(para[i][j].r, para[i][j].cr_back.ToCOLORREF());

			//text�� ����Ѵ�.
			pDC->SetTextColor(para[i][j].cr_text.ToCOLORREF());
			pDC->DrawText(para[i][j].text, para[i][j].r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
			//draw_text(g, m_para[i][j].r, m_para[i][j].cr_text, m_para[i][j].text, m_para[i][j].size, m_para[i][j].);
			//g.DrawString(CStringW(m_para[i][j].text), m_para[i][j].text.GetLength(), font, Gdiplus::PointF((Gdiplus::REAL)m_para[i][j].r.left, (Gdiplus::REAL)m_para[i][j].r.top), &sf);
			pDC->SelectObject(pOldFont);
#else
			//text ������ ĥ�ϰ�
			draw_rect(g, para[i][j].r, Gdiplus::Color::Transparent, para[i][j].text_prop.cr_back);

			Gdiplus::FontFamily* fontFamily = new Gdiplus::FontFamily((WCHAR*)(const WCHAR*)CStringW(para[i][j].text_prop.name));

			//�ý��ۿ� ��ϵ��� ���� ��Ʈ�� ������ ��� ff�� null�̹Ƿ� �⺻ ��Ʈ�ζ� ��ü���Ѿ� �Ѵ�.
			if (!fontFamily->IsAvailable())
			{
				delete fontFamily;
				fontFamily = Gdiplus::FontFamily::GenericSansSerif()->Clone();
			}

			Gdiplus::Font* font = NULL;
			para[i][j].get_paragraph_font(g, &font);
			//float emSize = fDpiY * para[i][j].text_prop.size / 96.0;
			//Gdiplus::Font font(&ff, emSize, para[i][j].text_prop.style);


			//text�� ����Ѵ�.
			Gdiplus::SolidBrush text_brush(para[i][j].text_prop.cr_text);

			//GraphicsPath�� �̿��ϸ� stroke, shadow �� �پ��� ȿ���� ������ �� ������
			//DrawString()���� ���ڰ� �����ϰ� ������ �ʴ� ������ �ִ�.
			//stroke, shadow�� �ƿ� ������� ���� ���� DrawString()���� �׷��ش�.
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

				//��ġ�� �κ��� ������Ű�� �ʴ´�. FillModeAlternate�� ������Ŵ.
				str_path.SetFillMode(Gdiplus::FillModeWinding);
				shadow_path.SetFillMode(Gdiplus::FillModeWinding);

				//AddString() �Ķ���� �� �����ġ�� �� �� Gdiplus::Rect() �Ǵ� Gdiplus::Point()�� �� �� �ִµ�
				//stroke �Ǵ� shadow�� �߰��Ǿ� r�� ������ �ؽ�Ʈ�� ��µ��� �ʴ� ������ �ִ�.
				//r�� ��Ȯ�� ����ϴ� ���� �����̳� ���� r�� ���� �ʰ� Gdiplus::Point()�� �ָ� �������� �ʴ´�.
				CRect r = para[i][j].r;
				str_path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), fontFamily,
					para[i][j].text_prop.style, emSize, Gdiplus::Point(r.left, r.top), sf.GenericTypographic());

				//�׸����� ���̴� �ؽ�Ʈ height�� ���� ����ϰ� stroke�� thickness �����͵� �����ִ�
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

				//thickness�� 0.0f�̸� g.DrawPath()�� �ƴ� g.DrawString()���� �׸��� �ǰ� ���� ������ �� �׷�������
				//���� �ɼ��� Ʋ���� ������ �߻��Ͽ� �켱 �Ʒ��� ���� ���ǿ� ���� g.DrawPath()�� �����ϵ��� �Ѵ�.
				if (para[i][j].text_prop.thickness > 0.0f)
					g.DrawPath(&pen, &str_path);

				g.FillPath(&brush, &str_path);
			}
#endif

			//�� para ���� Ȯ�ο� �ڵ�
#ifdef _DEBUG
			//"\n"�� ���� ���� ������ ���� �簢���� ���� ǥ������ �ʴ´�.
			//if (para[i][j].r.Width() > 2)
				//draw_rect(g, para[i][j].r, Gdiplus::Color::Blue);// , Gdiplus::Color(255, 255, 0, 0));
#endif
			if (font)
				delete font;

			if (fontFamily)
				delete fontFamily;
		}
	}

	//�ؽ�Ʈ ��� ���� Ȯ�ο�
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

//calc_text_rect()���� �̹� �� paragraph�� r�� align�� ���� ���������� �̸� �������� �����ϰ��� �� ��� ȣ��.
CRect CSCParagraph::set_text_align(CRect rc, std::deque<std::deque<CSCParagraph>>& para, DWORD align)
{
	int i, j;
	int sx, sy;
	int total_text_height = 0;
	CRect margin;	//���� static ��������� �и��� ����

	if (align & DT_CENTER)
	{
		//�� ���θ��� total_width�� ���ϰ�
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//�������� �����Ͽ� center�� ǥ���� ��, �ؽ�Ʈ�� center�� ǥ���� ��...
			//if (m_hIcon)
			//	total_width -= (m_sz_icon.cx + 4);

			//cx���� total_width/2�� �� ��ġ�� ù ��° �׸��� sx�̹Ƿ� �� ��ŭ shift��Ű�� �ȴ�.
			sx = rc.CenterPoint().x - total_width / 2;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else if (align & DT_RIGHT)
	{
		//�� ���θ��� total_width�� ���ϰ�
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//rc.right���� total_width�� �� ��ġ�� ù ��° �׸��� sx�̹Ƿ� �� ��ŭ shift��Ű�� �ȴ�.
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
		//��ü ���̿��� ��ü �ؽ�Ʈ ���� �հ踦 �� 1/2 ��ŭ shift ��Ų��.
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
		rect_text.left = para[max_width_line][0].r.left;	//�ִ� ���� ������ 0�� �������� left
		rect_text.top = para[0][0].r.top;					//�ֻ�� �׸��� top
		rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;	//�ִ� ���� ������ ������ �׸��� right
		rect_text.bottom = para[0][0].r.top + total_text_height;	//�ֻ�� �׸��� top + ��ü �ؽ�Ʈ ����
	}

	return rect_text;
}

//�ؽ�Ʈ �����¿� ����
//void CSCParagraph::set_margin(std::deque<std::deque<CSCParagraph>>& para, float margin)
//{
//	para.text_prop.
//}

//para�� ������ ���ڿ��� �����Ѵ�.
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
