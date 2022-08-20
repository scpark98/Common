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
	CListCtrlData(CString _text, int _column = -1)
	{
		ASSERT(column > 0 || _column > 0);

		//�� ����Ʈ �׸��� �� ó�� �� ũ�Ⱑ �����Ǵ� �ڵ�.
		//�� �������ʹ� -1�� �Ѿ���� ��ŵ�ȴ�.
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
		ASSERT(subItem < column);
		text[subItem] = _text;
	}

};
