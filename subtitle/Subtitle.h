#pragma once

#include <Afxwin.h>

#include <deque>
#include <map>
#include <vector>

#include "../Functions.h"

#define BRACKET_OPEN	_T("<")
#define BRACKET_CLOSE	_T(">")

//한 단위의 자막에 한 문장씩. 색상이 다를 수 있어서 별도로 정의.
//line_index: 같은 cue 안 <br> 그룹 id. 같은 line_index = 한 line 의 색 segment 들 (예: line 안에서
//<font> 색 변화). 다른 line_index = <br> 로 분리된 다른 line. save 시 line_index 변화 = <br> 삽입.
class CSentence
{
public:
	CSentence()
	{
		sentence.Empty();
		color.Empty();
		line_index = 0;
	}

	CSentence(CString _sentence, CString _cr = _T(""), int _line_index = 0)
	{
		sentence = _sentence;
		color = _cr;
		line_index = _line_index;
	}

	CString sentence;
	CString color;
	int line_index;
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
		alignment = 0;
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
	//ASS \an<n> numpad alignment (1~9). 0 = default (m_subCfg.pos_x/pos_y 기반).
	//   7 8 9  (top)
	//   4 5 6  (middle)
	//   1 2 3  (bottom)
	int alignment = 0;
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
	//외부에서 자막 파일이 rename 된 후 표기 경로만 갱신 — 내부 cue 데이터는 그대로.
	void	set_subtitle_file(const CString& sfile) { m_sfile = sfile; }

	//SMI 의 P Class 별 분리 저장. KRCC/ENCC/JPCC 등. srt 는 단일 키 "" 로 저장.
	//load_smi 가 채우고 rebuild_active_view 가 m_active_classes 의 합집합으로 m_subtitle 재구성.
	std::map<CString, std::deque<CCaption>> m_tracks;

	//현재 활성 Class 목록. 다중 가능 (KRCC + ENCC 동시 → 한영 동시 표시).
	//비어있으면 모든 트랙 활성 (기존 동작 호환).
	std::vector<CString> m_active_classes;

	//트랙 keys 반환 — 메뉴 빌더용. 정렬 순서: KRCC, ENCC, JPCC, 기타.
	std::vector<CString> get_classes() const;

	//활성 트랙 변경 후 m_subtitle 재구성. UI 에서 메뉴 선택 시 호출.
	void set_active_classes(const std::vector<CString>& classes);

	//m_tracks + m_active_classes → m_subtitle 재구성 (start 정렬 + 동일 start sentences merge).
	void rebuild_active_view();

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
		m_tracks.clear();
		m_active_classes.clear();
	}

protected:
	CString m_header;
	CString m_sfile;
	CString m_sLanguage;	//KRCC or JPCC or ENCC...

	//한 sync단위를 입력받아 필요한 정보를 파싱한 후
	//m_tracks[Class] 에 넣어준다. Class 가 없으면 default_class 사용.
	void parse_subtltle(CString source, const CString& default_class = _T("KRCC"));

	bool load_smi(CString sfile);
	bool load_srt(CString sfile);
	bool load_ass(CString sfile);	//ASS / SSA — Dialogue 라인 + Format header 기반 parsing.
	bool save_smi(CString sfile);
	bool save_srt(CString sfile);
	//ASS 단순 skeleton + Dialogue lines. style 정보 보존 X — text 만. 호출자가 export 시 사용.
	bool save_ass(CString sfile);
};
