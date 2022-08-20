#pragma once

#include <deque>
#include <Afxwin.h>

//특정 색상값을 [색상없음]의 의미로 사용하기 위해.
#define listctrlex_unused_color	RGB(17,235,53)

//한 라인에 대한 정보를 담는 클래스
class CListCtrlData
{
public:
	//0번 컬럼에 들어갈 _text 항목과 column의 갯수를 입력받는데
	//리스트는 맨 처음 프로그램 시작시에 몇개의 column을 이용할지는 이미 정해놓고 사용한다.
	//각 리스트 데이터 라인마다 그 컬럼수를 달리할 일도 없다.
	CListCtrlData(CString _text, int _column = -1)
	{
		ASSERT(column > 0 || _column > 0);

		//이 리스트 항목이 맨 처음 그 크기가 결정되는 코드.
		//그 다음부터는 -1이 넘어오니 스킵된다.
		if (_column > 0)
		{
			column = _column;
			text.resize(column);
			crText.resize(column);
			crText.assign(column, listctrlex_unused_color);
			crBack.resize(column);
			crBack.assign(column, listctrlex_unused_color);
		}

		text[0] = _text;
		checked = false;
		selected = 0;
	}

	std::deque<CString> text;
	int column;
	bool checked;
	int selected;

	//이 벡터가 NULL이면 CVtListCtrlEx의 멤버인 m_crText, m_crBack을 사용하고
	//색상값이 특정색이 아니면 해당 색으로 그 컬럼을 표시한다.
	std::deque<COLORREF> crText;
	std::deque<COLORREF> crBack;

	bool compare(const CListCtrlData &a, int index) const
	{
		return text[index] < a.text[index];
	}

	void set_text(int subItem, CString _text)
	{
		ASSERT(subItem < column);
		text[subItem] = _text;
	}

};
