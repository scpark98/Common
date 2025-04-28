#include "SCParagraph.h"
#include "../../Functions.h"

CSCParagraph::CSCParagraph()
{

}

CSCParagraph::~CSCParagraph()
{
}

void CSCParagraph::build_paragraph_str(CString text, std::deque<std::deque<CSCParagraph>>& para, LOGFONT* lf, Gdiplus::Color cr_text, Gdiplus::Color cr_back)
{
	int i;
	CString cr_str;
	CString str;
	std::deque<CString> tags;
	int line = 0;

	//"<b><cr = Red>This</b></cr > is a <cr =Blue><i>sample</i> <b>paragraph</b>."
	get_tag_str(text, tags);

	CSCParagraph basic_para, para_temp;
	basic_para.name = lf->lfFaceName;
	basic_para.bold = (lf->lfWeight == FW_BOLD);
	basic_para.size = get_font_size_from_pixel_size(NULL, lf->lfHeight);
	basic_para.italic = lf->lfItalic;
	basic_para.strike = lf->lfStrikeOut;
	basic_para.underline = lf->lfUnderline;
	basic_para.cr_text = cr_text;
	basic_para.cr_back = cr_back;

	para_temp = basic_para;

	std::deque<CSCParagraph> para_line;

	//���� �±׸� ������ �Ӽ��� �����ϰ�
	//�ܼ� �ؽ�Ʈ�� ������ �ش� �Ӽ��� �Բ� paragraph�� push�ϰ�
	//�� �±׸� ������ �� �±װ� ���������� �����ϰ� �� �±׸� ������ �� �Ӽ��� �����Ѵ�. 
	for (i = 0; i < tags.size(); i++)
	{
		if (tags[i] == _T("<b>"))
		{
			para_temp.bold = true;
		}
		else if (tags[i] == _T("</b>"))
		{
			para_temp.bold = basic_para.bold;
		}
		else if (tags[i] == _T("<i>"))
		{
			para_temp.italic = true;
		}
		else if (tags[i] == _T("</i>"))
		{
			para_temp.italic = basic_para.italic;
		}
		else if (tags[i] == _T("<u>"))
		{
			para_temp.underline = true;
		}
		else if (tags[i] == _T("</u>"))
		{
			para_temp.underline = basic_para.underline;
		}
		else if (tags[i] == _T("<s>"))
		{
			para_temp.strike = true;
		}
		else if (tags[i] == _T("</s>"))
		{
			para_temp.strike = basic_para.strike;
		}
		else if (tags[i].Find(_T("<cr=")) >= 0 || tags[i].Find(_T("<ct=")) >= 0)
		{
			cr_str = tags[i].Mid(4, tags[i].GetLength() - 5);
			para_temp.cr_text = get_color(cr_str);
		}
		else if (tags[i].Find(_T("</cr>")) >= 0 || tags[i].Find(_T("</ct>")) >= 0)
		{
			para_temp.cr_text = basic_para.cr_text;
		}
		else if (tags[i].Find(_T("<cb=")) >= 0)
		{
			cr_str = tags[i].Mid(4, tags[i].GetLength() - 5);
			para_temp.cr_back = get_color(cr_str);
		}
		else if (tags[i].Find(_T("<crb=")) >= 0)
		{
			cr_str = tags[i].Mid(5, tags[i].GetLength() - 6);
			para_temp.cr_back = get_color(cr_str);
		}
		else if (tags[i].Find(_T("</cb>")) >= 0 || tags[i].Find(_T("</crb>")) >= 0)
		{
			para_temp.cr_back = basic_para.cr_back;
		}
		else if (tags[i].Find(_T("<f=")) >= 0)
		{
			CString str_font = tags[i].Mid(3, tags[i].GetLength() - 4);
			para_temp.name = str_font;
		}
		else if (tags[i].Find(_T("<font=")) >= 0)
		{
			CString str_font = tags[i].Mid(6, tags[i].GetLength() - 7);
			para_temp.name = str_font;
		}
		else if (tags[i].Find(_T("<fontname=")) >= 0)
		{
			CString str_font = tags[i].Mid(10, tags[i].GetLength() - 11);
			para_temp.name = str_font;
		}
		else if (tags[i].Find(_T("</f>")) >= 0 || tags[i].Find(_T("</font>")) >= 0 || tags[i].Find(_T("</fontname>")) >= 0)
		{
			para_temp.name = basic_para.name;
		}
		else if (tags[i].Find(_T("<sz=")) >= 0)
		{
			CString str_size = tags[i].Mid(4, tags[i].GetLength() - 5);
			para_temp.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("<size=")) >= 0)
		{
			CString str_size = tags[i].Mid(6, tags[i].GetLength() - 7);
			para_temp.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("<fontsize=")) >= 0)
		{
			CString str_size = tags[i].Mid(10, tags[i].GetLength() - 11);
			para_temp.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("</sz>")) >= 0 || tags[i].Find(_T("</size>")) >= 0 || tags[i].Find(_T("</fontsize>")) >= 0)
		{
			para_temp.size = basic_para.size;
		}
		else if (tags[i] == _T("<br>"))
		{
			para.push_back(para_line);
			para_line.clear();
			line++;
		}
		else
		{
			para_temp.text = tags[i];
			para_line.push_back(para_temp);
		}
	}

	if (para_line.size())
		para.push_back(para_line);
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