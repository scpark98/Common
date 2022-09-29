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

#define DEFAULT_TEXT_COLOR Color(255, 32, 32, 32)
#define DEFAULT_LINE_COLOR Color(255, 192, 192, 192)

class CTableItem
{
public:
	CTableItem() {};

	CTableItem(CString _text,
		bool _bold = false,
		float _line_thick = 1.0,
		Color _text_color = DEFAULT_TEXT_COLOR,
		Color _line_color = DEFAULT_LINE_COLOR)
	{
		text = _text;
		text_color = _text_color;
		text_bold = _bold;
		line_thick = _line_thick;
		line_color = _line_color;
	}

	CString text = _T("");
	Color text_color = DEFAULT_TEXT_COLOR;
	bool text_bold = false;
	float line_thick = 1.0;
	Color line_color = DEFAULT_LINE_COLOR;
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
	void draw_end_line(bool draw) { m_draw_end_line = draw; }

	CPoint find_string(CString src);

protected:
	int m_line_height = 28;
	std::deque<int> m_width;
	bool m_draw_end_line = true;

	Color m_cr_text = DEFAULT_TEXT_COLOR;
	Color m_cr_line = DEFAULT_LINE_COLOR;

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


