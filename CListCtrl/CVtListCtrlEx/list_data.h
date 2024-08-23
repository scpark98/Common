#pragma once

#include <deque>
#include <Afxwin.h>

//Ư�� ������ [�������]�� �ǹ̷� ����ϱ� ����.
#define listctrlex_unused_color	RGB(17,235,53)

//�� ���ο� ���� ������ ��� Ŭ����
class CListCtrlData
{
public:
	//0�� �÷��� �� _text �׸�� column�� ������ �Է¹޴µ�
	//����Ʈ�� �� ó�� ���α׷� ���۽ÿ� ��� column�� �̿������� �̹� ���س��� ����Ѵ�.
	//�� ����Ʈ ������ ���θ��� �� �÷����� �޸��� �ϵ� ����.
	CListCtrlData(CString _text, int _img_idx, int _max_column = -1)
	{
		ASSERT(max_column > 0 || _max_column > 0);

		//�� ����Ʈ �׸��� �� ó�� �� ũ�Ⱑ �����Ǵ� �ڵ�.
		//�� �������ʹ� -1�� �Ѿ���� ��ŵ�ȴ�.
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
	int max_column;
	bool checked = false;
	int selected = false;

	//�� ���Ͱ� NULL�̸� CVtListCtrlEx�� ����� m_crText, m_crBack�� ����ϰ�
	//������ Ư������ �ƴϸ� �ش� ������ �� �÷��� ǥ���Ѵ�.
	std::deque<COLORREF> crText;
	std::deque<COLORREF> crBack;

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
