#if !defined(AFX_SMSLISTCTRL_H__5A4426BC_B484_4B47_A0D2_E5AF67CA28D5__INCLUDED_)
#define AFX_SMSLISTCTRL_H__5A4426BC_B484_4B47_A0D2_E5AF67CA28D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SmsListCtrl.h : header file
//



/////////////////////////////////////////////////////////////////////////////
// CImageListCtrl command target
#include "ItemListCtrl.h"

class CImageItem;

class CImageListCtrl : public CItemListCtrl
{
public:
	CImageListCtrl();           // protected constructor used by dynamic creation
	virtual ~CImageListCtrl();

public:
	virtual int GetItemWidth();
	virtual int GetItemHeight();

	virtual int GetItemVGap();
	virtual int GetItemHGap();

	virtual int GetVGap();
	virtual int GetHGap();

public:
	virtual bool LoadPath(CString strPath);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImageListCtrl)
	//}}AFX_VIRTUAL

// Implementation

	// Generated message map functions
	//{{AFX_MSG(CImageListCtrl)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SMSLISTCTRL_H__5A4426BC_B484_4B47_A0D2_E5AF67CA28D5__INCLUDED_)
