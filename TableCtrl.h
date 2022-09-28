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
#include <vector>
#include <map>
// CTableCtrl

class CTableItem
{
public:
	CTableItem() {};
	CTableItem(CString _text, COLORREF _text_color = ::GetSysColor(COLOR_BTNTEXT), bool _bold = false, float _line_thick = 1.0, COLORREF _line_color = RGB(255,192,192))
	{
		text = _text;
		text_color = _text_color;
		text_bold = _bold;
		line_thick = _line_thick;
		line_color = _line_color;
	}

	CString text = _T("");
	COLORREF text_color = ::GetSysColor(COLOR_BTNTEXT);
	bool text_bold;
	float line_thick = 1.0;
	COLORREF line_color = RGB(255, 192, 192);
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
	std::vector<int> m_width;
	int m_line_height = 40;

	std::vector<std::vector<CTableItem>> m;

	//table의 크기를 설정 또는 재조정한다.
	void resize(int cx, int cy, bool invalidate);

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


