#pragma once

#include <Afxwin.h>

#include <deque>

#include "../../Common/Functions.h"

#define BRACKET_OPEN	_T("<")
#define BRACKET_CLOSE	_T(">")

//�� ������ �ڸ��� �� ���徿. ������ �ٸ� �� �־ ������ ����.
class CSentence
{
public:
	CSentence()
	{
		sentence.Empty();
		color.Empty();
	}

	CSentence(CString _sentence, CString _cr = _T(""))
	{
		sentence = _sentence;
		color = _cr;
	}

	CString sentence;
	CString color;
};

//�� ������ �ڸ�
class CCaption
{
public:
	CCaption()
	{
		reset();
	}

	CCaption(int _start, int _end = -1, CString _sentence = _T(""), CString _cr = _T(""))
	{
		start = _start;
		end = _end;
		sentences.push_back(CSentence(_sentence, _cr));
	}

	void reset()
	{
		sentences.clear();
		start = -1;
		end = -1;
	}

	//�������� �� ĸ���� �� ���ڿ��� ���ļ� �����Ѵ�.
	CString merge_caption()
	{
		if (sentences.size() == 0)
			return _T("");

		CString caption;
		for (int i = 0; i < sentences.size(); i++)
			caption += (sentences[i].sentence + _T("\n"));

		return caption;
	}

	bool is_valid()
	{
		return ((start >= 0) && (sentences.size() > 0));
	}

	std::deque<CSentence> sentences;
	int start;	//�ڸ� ���� �ð�
	int end;	//�ڸ� �� �ð�
};

//�ϳ��� �ڸ� ������ �а� ��ü �ڸ� �����͸� ����.
class CSubtitle
{
public:
	CSubtitle();

	bool load_subtitle_file(CString sfile = _T(""));
	bool save_subtitle_file(CString sfile = _T(""));

	std::deque<CCaption> m_subtitle;
	CString get_subtitle_file() { return m_sfile; }

	//pos�� ���Ե� �ڸ� ������ ���۰� ������ ���ϰ� �� �ڸ� �ε����� �����Ѵ�.
	int	get_subtitle_range(int pos, int &start, int &end);


//add, delete, edit
	
	//sub_index �׸�ڿ� ���� �ð����� �ִ� ���ο� �ڸ� �׸��� �߰��Ѵ�.
	void add_subtitle(int sub_index, int start, int end = -1, CCaption caption = CCaption());

	//sub_index �ڸ� �׸��� sentence_index ������ ������ �߰���Ų��.
	//start�ð��� �ٸ� ���ο� �ڸ��� �߰��ϴ� ���� add_subtitle�Լ��� �̿��Ѵ�.
	void add_sentence(int sub_index, int sentence_index, CSentence sentence = CSentence());

	//field : 0(start), 1(end), 2(color), 3(caption)
	void modify(int sub_index, int sentence_index, int field, CString text);

	//���� ������ �����Ѵ�. sentence_index�� < 0�̸� sub_index�� �����Ѵ�.
	void delete_subtitle(int sub_index, int sentence_index);

	void reset()
	{
		m_sfile.Empty();
		m_header.Empty();
		m_sLanguage.Empty();
		m_subtitle.clear();
	}

protected:
	CString m_header;
	CString m_sfile;
	CString m_sLanguage;	//KRCC or JPCC or ENCC...

	FILE	*m_fp;
	bool	m_unicode;

	//ó�� ����� �ƴ� �±׵��� �����Ѵ�.
	//ex. <font face>, </font>, <b>, <i>
	void remove_other_tags(CString &str);

	//�� sync������ �Է¹޾� �ʿ��� ������ �Ľ��� ��
	//m_subtitle�� �־��ش�.
	void parse_subtltle(CString source);

	bool load_smi(CString sfile);
	bool load_srt(CString sfile);
	bool save_smi(CString sfile);
	bool save_srt(CString sfile);
};
