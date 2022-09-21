#pragma once

/*
* m_table[m][n]
* 1000000100100
* 1001000100100
* 
* m_text[0].x = 1;
* m_text[0].y = 2;
* m_text[0].text = "sample text;
*/

#include <afxwin.h>
#include <gdiplus.h>
#include <vector>
#include <map>
// CTableCtrl

class CTable
{

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
	std::vector<std::vector<int>> m_tb;
	void set_size(int cx, int cy);

	struct CELL
	{
		int x;
		int y;
	};
	std::map<CELL, CCellText> m_text;

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


