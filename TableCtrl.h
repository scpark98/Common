#pragma once

/*
* m.w = {100, 200, 50};
* m[0][0] = table(str)
* m[1][0] = table(str);
* m[1][1] = table(str);
* 
* m[i]에 대해...
* m[i][j]를 그리는데
* m[i][0]의 텍스트 출력, rect는 r(sx, sy, sx + m.w[0], sy + line_height);
* sx += m.w[0];
* m[i][0]과 m[i][1] 사이의 세로선을 그리고
* 
*/

#include <afxwin.h>
#include <gdiplus.h>
#include <deque>
#include <map>
// CTableCtrl

using namespace Gdiplus;

#define GRAY_COLOR(i) ((i) == 8 ? Color(255, 255, 255, 255) : Color(255, 32 * i, 32 * i, 32 * i))

class CTableItem
{
public:
	CTableItem() {};

	CTableItem(CString _text,
		bool _bold = false,
		float _line_thick = 1.0,
		Color _text_color = GRAY_COLOR(1),
		Color _line_color = GRAY_COLOR(7),
		int _y_margin = 0)
	{
		text = _text;
		text_color = _text_color;
		text_bold = _bold;
		line_thick = _line_thick;
		line_color = _line_color;
		y_margin = _y_margin;
	}

	CString text = _T("");
	Color text_color = GRAY_COLOR(1);
	bool text_bold = false;
	float line_thick = 1.0;
	Color line_color = GRAY_COLOR(7);
	int y_margin = 0;
};

class CTableItemText
{
public:
	CString text;
};

using namespace Gdiplus;

class CTableCtrl : public CWnd
{
	DECLARE_DYNAMIC(CTableCtrl)

public:
	CTableCtrl();
	virtual ~CTableCtrl();

	std::deque<std::deque<CTableItem>> m;

	//table의 크기를 설정 또는 재조정한다.
	void resize(int cx, int cy, bool invalidate);

	void add_line(int num, ...);
	//void add_line(CString text, ...);

	void set_width(int num, ...);
	void line_height(int height) { m_line_height = height; Invalidate(); }
	
	void draw_first_line(bool draw) { m_draw_first_line = draw; }
	void draw_end_line(bool draw) { m_draw_end_line = draw; }

	//item_text에 해당하는 항목의 셀 인덱스를 얻어온다.
	CPoint find_string(CString find_str, bool discard_blank = true);

	//item_text에 해당하는 셀을 찾아 현재 셀 또는 그 오른쪽의 셀에 new_text를 입력한다.
	//표 항목의 이름을 주고 그 오른쪽에 실제 데이터를 넣는 용도로 사용할 수 있다.
	void set_text(CString find_str, CString new_text, bool on_right_cell = true, bool discard_blank = true);

protected:
	int m_line_height = 28;
	std::deque<int> m_width;
	bool m_draw_first_line = true;
	bool m_draw_end_line = true;

	Color m_cr_text = GRAY_COLOR(1);
	Color m_cr_line = GRAY_COLOR(7);

protected:
	BOOL			RegisterWindowClass();

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};


