#pragma once

#include <afxwin.h>
#include <afxpropertygridctrl.h>
#include "PropGridSlider.h"

#define PropertyPage_Assert(x)	if(!(x)) {__asm{int 3}}

/*
OnGridPropertyChanged()���� lparam���� �Ʒ��� ���� ����� �׸��� ���ϰ�
dwData�� switch������ �ش� �׸��� �о�� �� �ִ�.
CString ������ �� �� �ƴ϶� int, float���� �⺻ Ÿ�Կܿ���
CMFCPropertyGridFontProperty, CMFCPropertyGridColorProperty ��� ����
CMFCPropertyGridProperty�� ��ӹ޾� ������� Ÿ�Ե鵵 �ش� ���� ���� �� �ִ�.
operator�� �߰��ϸ� ��� Ÿ�Կ� ���� ������ getPropValue(pProperty);��� ȣ�������� ��� ���� �� �ִ�.
����Ÿ������ �Ǵ��Ͽ� operator���߿��� �ش� �ڵ尡 ����ȴ�.

	CMFCPropertyGridProperty* pProperty = (CMFCPropertyGridProperty*)lparam;
	DWORD	dwData = pProperty->GetData();
	switch (dwData)
	{
		case id_input_text :
		{
			CString text = getPropValue(pProperty);
			break;
		}
		case id_font_select :
		{
			LOGFONT* lf = getPropValue(pProperty);
			break;
		}
		...
	}
*/

class getPropValue
{
public:
	getPropValue(CMFCPropertyGridProperty* pProp) :m_pProp(pProp) {};
	operator CString() { return (LPCTSTR)(_bstr_t)m_pProp->GetValue(); }
	operator int() { return m_pProp->GetValue().iVal; }
	operator unsigned int() { return (unsigned int)m_pProp->GetValue().iVal; }
	operator float() { return m_pProp->GetValue().fltVal; }
	operator double() { return m_pProp->GetValue().dblVal; }
	operator bool() { return m_pProp->GetValue().boolVal == VARIANT_TRUE; }
	operator LOGFONT*() { return ((CMFCPropertyGridFontProperty*)m_pProp)->GetLogFont(); }
	operator COLORREF() { return ((CMFCPropertyGridColorProperty*)m_pProp)->GetColor(); }
private:
	CMFCPropertyGridProperty* m_pProp;
};


// CSCMFCPropertyGridCtrl
class CSCMFCPropertyGridCtrl : public CMFCPropertyGridCtrl
{
	DECLARE_DYNAMIC(CSCMFCPropertyGridCtrl)

public:
	CSCMFCPropertyGridCtrl();
	virtual ~CSCMFCPropertyGridCtrl();

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
			AddChildItem(	CMFCPropertyGridProperty* pParent, CString sName, _variant_t varValue, CString sDescription = _T(""),
							bool bEnable = FALSE, DWORD dwData = NULL,
							bool bUseSpin = false, int min = 0, int max = 10 );
/*	CMFCPropertyGridProperty*
			AddChildItemInt(CMFCPropertyGridProperty* pParent, const CString& sName, const _variant_t& varValue, CString sDescription = "",
							bool bEnable = FALSE, DWORD dwData = NULL,
							bool bUseSpin = false, bool bEnableSpin = true, int min = 0, int max = 10 );
*/	BOOL	AddBoolItem(CMFCPropertyGridProperty* pParent, CString sName, bool bDefault, CString sDescription = _T(""), bool bEnable = true, DWORD dwID = NULL);
	BOOL	AddOptionItem( CMFCPropertyGridProperty* pParent, CString sName, CString sDefault, CString sDescription, bool bAllowEdit, bool bEnable, DWORD dwID, int nOption, CString* sOptionArray );

	template <typename ... T> bool AddOptionItem(CMFCPropertyGridProperty* pParent, CString sName, int default_index, CString sDescription, bool bAllowEdit, bool bEnable, DWORD dwID, T... options)
	{
		int n = sizeof...(options);
		if (n <= 0)
			return false;

		CString args[] = { options... };

		CMFCPropertyGridProperty* pProp = new CMFCPropertyGridProperty(sName, args[default_index], sDescription, dwID);

		for (auto option : args)
		{
			pProp->AddOption(option);
		}

		pProp->AllowEdit(bAllowEdit);
		pProp->Enable(bEnable);

		return pParent->AddSubItem(pProp);
	}

	bool	AddFileSelect( CMFCPropertyGridProperty* pParent, CString sName, const _variant_t& varValue, TCHAR* sFilter, CString sDescription = _T(""), DWORD dwID = NULL );
	bool	AddFolderSelect( CMFCPropertyGridProperty* pParent, CString sName, CString sInitialFolder, CString sDescription = _T(""), bool bEnable = true, DWORD dwID = NULL );
	bool	AddSliderItem(CMFCPropertyGridProperty* pParent, CString sName, int min, int max, int value, DWORD dwData);
	bool	AddFontSelect(CMFCPropertyGridProperty* parent, CString name, LOGFONT lf, DWORD id, CString desc = _T(""), bool enable = true);
	bool	AddColorSelect(CMFCPropertyGridProperty* parent, CString name, COLORREF crDefault, DWORD id, CString desc = _T(""), bool enable = true);

	//bool	addProperty_File(char* strTitle, char* strDesc, DWORD dwID, CMFCPropertyGridProperty* pParent, char* strFilter, CString* strOut)
	//bool	addProperty_File(wchar_t* strTitle, wchar_t* strDesc, DWORD dwID, CMFCPropertyGridProperty* pParent, wchar_t* strFilter, CString* strOut)
	//bool	addProperty_Folder(char* strTitle, DWORD dwID, CMFCPropertyGridProperty* pParent, char* strInitialFolder, CString* pFolderOut);
	//bool	addProperty_Folder(wchar_t* strTitle, DWORD dwID, CMFCPropertyGridProperty* pParent, wchar_t* strInitialFolder, CString* pFolderOut);


	//�÷��� �ʺ� ���� �����ϴ� �Լ��� �������� �����Ƿ� ���� �����Ͽ� ����ߴ�.
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

	void	set_color_theme(int theme);
	enum COLOR_THEMES
	{
		theme_default = 0,
		theme_gray,
		theme_dark,
	};

protected:

	//�������� m_nLeftColumnWidth���� �������ָ� �ڵ����� �ʺ� �����Ǿ�������
	//��� ��ġ ���ĺ��ʹ� m_nLeftColumnWidth���� �׻� width / 2.0���� �����Ǵ� ������ �߻��Ͽ�
	//(WM_SIZE and WM_LBUTTONDBLCLK ���� �޽����� ����)
	//m_nLeftColumnWidth1 �̶�� ������ ��������� �����Ͽ� ����Ѵ�.
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


