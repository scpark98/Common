#pragma once

#include <deque>
#include <Afxwin.h>
#include <gdiplus.h>

//특정 색상값을 [색상없음]의 의미로 사용하기 위해.
#define listctrlex_unused_color	Gdiplus::Color(17,235,53)

//한 라인에 대한 정보를 담는 클래스
class CListCtrlData
{
public:
	//0번 컬럼에 들어갈 _text 항목과 column의 갯수를 입력받는데
	//리스트는 맨 처음 프로그램 시작시에 몇개의 column을 이용할지는 이미 정해놓고 사용한다.
	//각 리스트 데이터 라인마다 그 컬럼수를 달리할 일도 없다.
	CListCtrlData(CString _text, int _img_idx, int _max_column = -1)
	{
		ASSERT(max_column > 0 || _max_column > 0);

		//이 리스트 항목이 맨 처음 그 크기가 결정되는 코드.
		//그 다음부터는 -1이 넘어오니 스킵된다.
		if (_max_column > 0)
		{
			max_column = _max_column;
			text.resize(max_column);
			crText.resize(max_column);
			crText.assign(max_column, listctrlex_unused_color);
			crBack.resize(max_column);
			crBack.assign(max_column, listctrlex_unused_color);
		}

		text[0] = _text;
		img_idx = _img_idx;
		checked = false;
		selected = 0;
	}

	std::deque<CString> text;
	int img_idx = -1;
	int max_column = 0;
	bool checked = false;
	int selected = false;
	//HAS_STRING, OWNER_DRAW_FIXED 속성을 가지면 Get/SetItemData() 함수를 사용할 수 없다.
	//이 두 함수를 사용할 수 있도록 data 멤버를 추가한다.
	DWORD_PTR data = 0;

	//이 벡터가 NULL이면 CVtListCtrlEx의 멤버인 m_cr_text, m_cr_back을 사용하고
	//색상값이 특정색이 아니면 해당 색으로 그 컬럼을 표시한다.
	std::deque<Gdiplus::Color> crText;
	std::deque<Gdiplus::Color> crBack;

	bool compare(const CListCtrlData &a, int index) const
	{
		return text[index] < a.text[index];
	}

	void set_text(int subItem, CString _text)
	{
		ASSERT(subItem < max_column);
		text[subItem] = _text;
	}
};
