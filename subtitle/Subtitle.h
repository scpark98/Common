#pragma once

#include <Afxwin.h>

#include <deque>

#include "../../Common/Functions.h"

#define BRACKET_OPEN	_T("<")
#define BRACKET_CLOSE	_T(">")

//한 단위의 자막에 한 문장씩. 색상이 다를 수 있어서 별도로 정의.
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

//한 단위의 자막
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

	//여러줄의 한 캡션을 한 문자열로 합쳐서 리턴한다.
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
	int start;	//자막 시작 시간
	int end;	//자막 끝 시간
};

//하나의 자막 파일을 읽고 전체 자막 데이터를 관리.
class CSubtitle
{
public:
	CSubtitle();

	bool load_subtitle_file(CString sfile = _T(""));
	bool save_subtitle_file(CString sfile = _T(""));

	std::deque<CCaption> m_subtitle;
	CString get_subtitle_file() { return m_sfile; }

	//pos가 포함된 자막 구간의 시작과 끝값을 구하고 그 자막 인덱스를 리턴한다.
	int	get_subtitle_range(int pos, int &start, int &end);


//add, delete, edit
	
	//sub_index 항목뒤에 시작 시간값이 있는 새로운 자막 항목을 추가한다.
	void add_subtitle(int sub_index, int start, int end = -1, CCaption caption = CCaption());

	//sub_index 자막 항목의 sentence_index 다음에 문장을 추가시킨다.
	//start시간이 다른 새로운 자막을 추가하는 것은 add_subtitle함수를 이용한다.
	void add_sentence(int sub_index, int sentence_index, CSentence sentence = CSentence());

	//field : 0(start), 1(end), 2(color), 3(caption)
	void modify(int sub_index, int sentence_index, int field, CString text);

	//라인 단위로 삭제한다. sentence_index가 < 0이면 sub_index를 삭제한다.
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

	//처리 대상이 아닌 태그들을 삭제한다.
	//ex. <font face>, </font>, <b>, <i>
	void remove_other_tags(CString &str);

	//한 sync단위를 입력받아 필요한 정보를 파싱한 후
	//m_subtitle에 넣어준다.
	void parse_subtltle(CString source);

	bool load_smi(CString sfile);
	bool load_srt(CString sfile);
	bool save_smi(CString sfile);
	bool save_srt(CString sfile);
};
