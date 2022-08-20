#pragma once

//#include "
// CColorDialogEx

class CColorDialogEx : public CColorDialog
{
	DECLARE_DYNAMIC(CColorDialogEx)

public:
	CColorDialogEx(COLORREF clrInit = 0, DWORD dwFlags = 0, CWnd* pParentWnd = NULL);
	virtual ~CColorDialogEx();

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual INT_PTR DoModal();
	virtual BOOL OnColorOK();
};


