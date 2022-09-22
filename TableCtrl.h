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

class CItem
{
public:
	CItem();
	CItem(CString t, bool b = false)
	{
		text = t;
		bold = b;
	}

	CString text;
	bool bold;
};

class CCellText
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

	std::vector<std::vector<CItem>> m;
	void set_size(int cx, int cy);

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


