#pragma once

#include <afxwin.h>
#include <afxpropertygridctrl.h>
#include "PropGridSlider.h"

#define PropertyPage_Assert(x)	if(!(x)) {__asm{int 3}}

// CMFCPropertyGridCtrlEx

class CMFCPropertyGridCtrlEx : public CMFCPropertyGridCtrl
{
	DECLARE_DYNAMIC(CMFCPropertyGridCtrlEx)

public:
	CMFCPropertyGridCtrlEx();
	virtual ~CMFCPropertyGridCtrlEx();

	int		GetGroupIndex( CString sGroup );							//find group(property) index by name
	CMFCPropertyGridProperty*
			GetGroupProperty( CString sGroup );							//find group(property) pointer by name

	CMFCPropertyGridProperty*
			GetItemProperty( DWORD dwData );							//find property pointer by dwData

	CMFCPropertyGridProperty*
			GetItemProperty( CString sGroup, CString sName );
	int		GetItemIndex( int nGroup, CString sName );

	CString	GetItemText( int nGroup, int nItem );
	CString	GetItemText( int nGroup, CString sName );					//find by name and return text(value)
	CString	GetItemText( CString sGroup, CString sName );				//find by name and return text(value)

	void	SetItemText( int nGroup, int nItem, CString sText );		//
	void	SetItemText( int nGroup, CString sName, CString sText );	//find by name and set text
	void	SetItemText( CString sGroup, CString sName, CString sText );

	//for combo style
	void	SetCurSel( int nGroup, int nItem, int nOption );
	void	SetCurSel( CString sGroup, CString sName, int nOption );
	void	SetCurSel( DWORD dwData, int nOption );
	int		GetCurSel( int nGroup, int nItem );
	int		GetCurSel( DWORD dwData );

	void	EnableItem(  int nGroup, int nItem, BOOL bEnable );
	void	EnableItem(  int nGroup, CString sName, BOOL bEnable );
	void	EnableItem(  CString sGroup, CString sName, BOOL bEnable );

	CMFCPropertyGridProperty*
			AddChildItem(	CMFCPropertyGridProperty* pParent, const CString& sName, const _variant_t& varValue, CString sDescription = _T(""),
							bool bEnable = FALSE, DWORD dwData = NULL,
							bool bUseSpin = false, int min = 0, int max = 10 );
/*	CMFCPropertyGridProperty*
			AddChildItemInt(CMFCPropertyGridProperty* pParent, const CString& sName, const _variant_t& varValue, CString sDescription = "",
							bool bEnable = FALSE, DWORD dwData = NULL,
							bool bUseSpin = false, bool bEnableSpin = true, int min = 0, int max = 10 );
*/	BOOL	AddBoolItem(	CMFCPropertyGridProperty* pParent, CString sName, BOOL bDefault, CString sDescription );
	BOOL	AddOptionItem( CMFCPropertyGridProperty* pParent, CString sName, CString sDefault, CString sDescription, BOOL bAllowEdit, BOOL bEnable, DWORD dwData, int nOption, CString* sOptionArray );
	BOOL	AddOptionItem( CMFCPropertyGridProperty* pParent, CString sName, CString sDefault, CString sDescription, BOOL bAllowEdit, BOOL bEnable, DWORD dwData, int nOption, LPCTSTR lpszFormat, ... );
	bool	AddFileSelect( CMFCPropertyGridProperty* pParent, const CString& sName, const _variant_t& varValue, TCHAR* sFilter, CString sDescription = _T(""), DWORD dwData = NULL );
	bool	AddFolderSelect( CMFCPropertyGridProperty* pParent, const CString& sName, CString sInitialFolder, CString sDescription = _T(""), BOOL bEnable = true, DWORD dwData = NULL );
	bool	AddSliderItem(CMFCPropertyGridProperty* pParent, CString sName, int min, int max, int value);

	//bool	addProperty_File(char* strTitle, char* strDesc, DWORD dwID, CMFCPropertyGridProperty* pParent, char* strFilter, CString* strOut)
	//bool	addProperty_File(wchar_t* strTitle, wchar_t* strDesc, DWORD dwID, CMFCPropertyGridProperty* pParent, wchar_t* strFilter, CString* strOut)
	//bool	addProperty_Folder(char* strTitle, DWORD dwID, CMFCPropertyGridProperty* pParent, char* strInitialFolder, CString* pFolderOut);
	//bool	addProperty_Folder(wchar_t* strTitle, DWORD dwID, CMFCPropertyGridProperty* pParent, wchar_t* strInitialFolder, CString* pFolderOut);


	//컬럼의 너비를 직접 세팅하는 함수가 지원되지 않으므로 직접 선언하여 사용했다.
	void	SetLeftColumnWidth( int cx );

	void	SetControlColor( COLORREF clrText, COLORREF clrBackground )
	{
		m_clrText		= clrText;
		m_clrBackground	= clrBackground;
		//AdjustLayout();
	}

	void	SetGroupColor( COLORREF clrText, COLORREF clrBackground )
	{
		m_clrGroupText			= clrText;
		m_clrGroupBackground	= clrBackground;
		//AdjustLayout();
	}

protected:
	int	m_nLeftColumnWidth1;
	//virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
	//afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	virtual int OnDrawProperty(CDC* pDC, CMFCPropertyGridProperty* pProp) const;
	virtual void AdjustLayout();
};


