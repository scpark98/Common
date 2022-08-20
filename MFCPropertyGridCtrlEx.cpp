// MFCPropertyGridCtrlEx.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "MFCPropertyGridCtrlEx.h"


// CMFCPropertyGridCtrlEx

IMPLEMENT_DYNAMIC(CMFCPropertyGridCtrlEx, CMFCPropertyGridCtrl)

CMFCPropertyGridCtrlEx::CMFCPropertyGridCtrlEx()
{
	m_nLeftColumnWidth1 = 100;
}

CMFCPropertyGridCtrlEx::~CMFCPropertyGridCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CMFCPropertyGridCtrlEx, CMFCPropertyGridCtrl)
	//ON_WM_LBUTTONDOWN()
	//ON_REGISTERED_MESSAGE( AFX_WM_PROPERTY_CHANGED, OnPropertyChanged )
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()



// CMFCPropertyGridCtrlEx 메시지 처리기입니다.

int CMFCPropertyGridCtrlEx::GetGroupIndex( CString sName )		//find group(property by name
{
	int		i;
	CString	sPropertyName;

	sName.MakeLower();

	for ( i = 0; i < GetPropertyCount(); i++ )
	{
		sPropertyName = GetProperty( i )->GetName();
		sPropertyName.MakeLower();

		if ( sPropertyName.Find( sName ) >= 0 )
			return i;
	}

	return -1;
}

CMFCPropertyGridProperty* CMFCPropertyGridCtrlEx::GetGroupProperty( CString sName )		//find group(property by name
{
	int		nIndex = GetGroupIndex( sName );

	if ( nIndex >= 0 )
		return GetProperty( nIndex );

	return NULL;
}

CMFCPropertyGridProperty* CMFCPropertyGridCtrlEx::GetItemProperty( DWORD dwData )
{
	return FindItemByData( dwData );
	/*
	int		i, j;
	CMFCPropertyGridProperty* pGroup;
	CMFCPropertyGridProperty* pSubItem;

	for ( i = 0; i < GetPropertyCount(); i++ )
	{
		pGroup = GetProperty( i );

		for ( j = 0; j < pGroup->GetSubItemsCount(); j++ )
		{
			pSubItem = pGroup->GetSubItem( j );
			if ( pSubItem->GetData() == dwData )
				return pSubItem;
		}
	}
	*/

	return NULL;
}

CMFCPropertyGridProperty* CMFCPropertyGridCtrlEx::GetItemProperty( CString sGroup, CString sName )
{
	int		nGroup = GetGroupIndex( sGroup );
	int		nIndex = GetItemIndex( nGroup, sName );

	return GetProperty( nGroup )->GetSubItem( nIndex );
}

CString	CMFCPropertyGridCtrlEx::GetItemText( int nGroup, int nItem )
{
	return GetProperty( nGroup )->GetSubItem( nItem )->GetValue();
}

CString	CMFCPropertyGridCtrlEx::GetItemText( int nGroup, CString sName )		//find by name and return text
{
	return GetItemText( nGroup, GetItemIndex( nGroup, sName ) );
}

CString CMFCPropertyGridCtrlEx::GetItemText( CString sGroup, CString sName )	//find by name and return text(value)
{
	return GetItemText( GetGroupIndex( sGroup ), sName );
}

int CMFCPropertyGridCtrlEx::GetItemIndex( int nGroup, CString sName )
{
	int		i;
	CString sItemName;
	CMFCPropertyGridProperty* pProp = GetProperty( nGroup );

	if ( !pProp )
		return -1;

	sName.MakeLower();

	for ( i = 0; i < pProp->GetSubItemsCount(); i++ )
	{
		sItemName = pProp->GetSubItem( i )->GetName();
		sItemName.MakeLower();

		//full matching이 아닌 넘어온 문자열을 포함하는 경우도 리턴해준다.
		if ( sItemName.Find( sName ) >= 0 )
			return i;
	}

	return -1;
}

void CMFCPropertyGridCtrlEx::SetItemText( int nGroup, int nItem, CString sText /*= ""*/ )
{
	GetProperty( nGroup )->GetSubItem( nItem )->SetValue( sText );
}

void CMFCPropertyGridCtrlEx::SetItemText( int nGroup, CString sName, CString sText )
{
	CString sItemName;
	CMFCPropertyGridProperty* pProp = GetProperty( nGroup );
	
	if ( !pProp )
		return;

	SetItemText( nGroup, GetItemIndex( nGroup, sName ), sText );
}

void CMFCPropertyGridCtrlEx::SetItemText( CString sGroup, CString sName, CString sText )
{
	int		nGroup = GetGroupIndex( sGroup );
	SetItemText( nGroup, GetItemIndex( nGroup, sName ), sText );
}

void CMFCPropertyGridCtrlEx::SetCurSel( int nGroup, int nName, int nOption )	//for combo style
{
	CString	sOption	= GetProperty( nGroup )->GetSubItem( nName )->GetOption( nOption );
	SetItemText( nGroup, nName, sOption );
}

void CMFCPropertyGridCtrlEx::SetCurSel( CString sGroup, CString sName, int nOption )	//for combo style
{
	int		nGroup	= GetGroupIndex( sGroup );
	int		nIndex	= GetItemIndex( nGroup, sName );
	CString	sOption	= GetProperty( nGroup )->GetSubItem( nIndex )->GetOption( nOption );
	SetItemText( nGroup, nIndex, sOption );
}

void CMFCPropertyGridCtrlEx::SetCurSel( DWORD dwData, int nOption )
{
	CString sOption = GetItemProperty( dwData )->GetOption( nOption );
	GetItemProperty( dwData )->SetValue( sOption );
}

int	CMFCPropertyGridCtrlEx::GetCurSel( int nGroup, int nItem )
{
	int		i;
	int		nOption = GetProperty( nGroup )->GetSubItem( nItem )->GetOptionCount();
	CString	sOption;
	CString sCurrentOption = GetProperty( nGroup )->GetSubItem( nItem )->GetValue();

	for ( i = 0; i < nOption; i++ )
	{
		sOption = GetProperty( nGroup )->GetSubItem( nItem )->GetOption( i );
		if ( sOption == sCurrentOption )
			return i;
	}

	return -1;
}

int	CMFCPropertyGridCtrlEx::GetCurSel( DWORD dwData )
{
	int		i;
	int		nOption = GetItemProperty( dwData )->GetOptionCount();
	CString	sOption;
	CString sCurrentOption = GetItemProperty( dwData )->GetValue();

	for ( i = 0; i < nOption; i++ )
	{
		sOption = GetItemProperty( dwData )->GetOption( i );
		if ( sOption == sCurrentOption )
			return i;
	}

	return -1;
}

void CMFCPropertyGridCtrlEx::EnableItem(  int nGroup, int nItem, BOOL bEnable )
{
	GetProperty( nGroup )->GetSubItem( nItem )->Enable( bEnable );
}

void CMFCPropertyGridCtrlEx::EnableItem(  int nGroup, CString sName, BOOL bEnable )
{
	EnableItem( nGroup, GetItemIndex( nGroup, sName ), bEnable );
}

void CMFCPropertyGridCtrlEx::EnableItem(  CString sGroup, CString sName, BOOL bEnable )
{
	EnableItem( GetGroupIndex( sGroup ), sName, bEnable );
}

CMFCPropertyGridProperty* CMFCPropertyGridCtrlEx::AddChildItem(  CMFCPropertyGridProperty* pParent, const CString& sName, const _variant_t& varValue /*= ""*/,
												CString sDescription /* = ""*/, bool bEnable /*= FALSE*/, DWORD dwData /*= NULL*/,
												bool bUseSpin, int min, int max )
{
	CMFCPropertyGridProperty* pProperty = new CMFCPropertyGridProperty( sName, varValue, sDescription, dwData );
	pProperty->Enable( bEnable );

	if ( bUseSpin )
		pProperty->EnableSpinControl( true, min, max );

	pParent->AddSubItem( pProperty );

	return pProperty;
}

BOOL CMFCPropertyGridCtrlEx::AddBoolItem(	CMFCPropertyGridProperty* pParent, CString sName, BOOL bDefault, CString sDescription )
{
	return pParent->AddSubItem( new CMFCPropertyGridProperty( sName, (_variant_t)(bDefault ? true : false) , sDescription ) );
}

BOOL CMFCPropertyGridCtrlEx::AddOptionItem( CMFCPropertyGridProperty* pParent, CString sName, CString sDefault, CString sDescription, BOOL bAllowEdit, BOOL bEnable, DWORD dwData, int nOption, LPCTSTR lpszFormat, ... )
{
	CString*	pString = new CString[nOption];
	LPCTSTR		sOption;
	va_list		ap;

	va_start( ap, lpszFormat );
	
	pString[0] = lpszFormat;

	for ( int i = 1; i < nOption; i++ )
	{
		sOption = va_arg(ap, LPCTSTR);
		pString[i] = sOption;
	}

	va_end( ap );

	AddOptionItem( pParent, sName, sDefault, sDescription, bAllowEdit, bEnable, dwData, nOption, pString );

	delete [] pString;

	return true;
}

BOOL CMFCPropertyGridCtrlEx::AddOptionItem( CMFCPropertyGridProperty* pParent, CString sName, CString sDefault, CString sDescription,
												BOOL bAllowEdit, BOOL bEnable, DWORD dwData, int nOption, CString* sOptionArray )
{
	CMFCPropertyGridProperty* pProp = new CMFCPropertyGridProperty( sName, sDefault, sDescription, dwData );

	int		i;

	for ( i = 0; i < nOption; i++ )
	{
		pProp->AddOption( (LPCTSTR)sOptionArray[i] );
	}

	pProp->AllowEdit( bAllowEdit );
	pProp->Enable( bEnable );

	return pParent->AddSubItem( pProp );
}

bool CMFCPropertyGridCtrlEx::AddFileSelect( CMFCPropertyGridProperty* pParent, const CString& sName, const _variant_t& varValue,
											    TCHAR* sFilter, CString sDescription, DWORD dwData )
{
	CMFCPropertyGridFileProperty* pFileSelect = new CMFCPropertyGridFileProperty(sName, TRUE, _T(""), 0, 0, sFilter, sDescription);
	pFileSelect->SetData(dwData);

	USES_CONVERSION;
	//wchar_t wbuf[1024];
	COleVariant rVariant = pFileSelect->GetValue();
	VARIANT v = rVariant.Detach();
	VARTYPE vt = v.vt;

	ASSERT(vt == VT_BSTR);
	//swprintf_s(wbuf, L"%s", OLE2W(v.bstrVal));
	//*sSelected = wbuf;

	if ( pParent )
		pParent->AddSubItem(pFileSelect);
	else
		AddProperty(pFileSelect);

	return true;
}


bool CMFCPropertyGridCtrlEx::AddFolderSelect(	CMFCPropertyGridProperty* pParent, const CString& sName,
												CString sInitialFolder, CString sDescription,
												BOOL bEnable, DWORD dwData )
{
	CMFCPropertyGridFileProperty* pFolderSelect = new CMFCPropertyGridFileProperty(sName, sInitialFolder);
	pFolderSelect->SetData(dwData);

	USES_CONVERSION;
	//wchar_t wbuf[1024];
	COleVariant rVariant = pFolderSelect->GetValue();
	VARIANT v = rVariant.Detach();
	VARTYPE vt = v.vt;

	ASSERT(vt == VT_BSTR);
	//swprintf_s(wbuf, L"%s", OLE2W(v.bstrVal));
	//pFolderSelected = wbuf;

	pFolderSelect->Enable( bEnable );

	if ( pParent )
		pParent->AddSubItem(pFolderSelect);
	else
		AddProperty(pFolderSelect);

	return true;
}

/*
bool CMFCPropertyGridCtrlEx::addProperty_File(char* strTitle, char* strDesc, DWORD dwID, CMFCPropertyGridProperty* pParent, char* strFilter, CString* strOut)
{
	// check params
	PropertyPage_Assert(strTitle);
	PropertyPage_Assert(strOut);
	PropertyPage_Assert(strFilter);
	if(strTitle == 0 || strOut == 0 || strFilter == 0) return false;

	CStringW strTitleW(strTitle);
	CStringW strDescW(strDesc);
	CStringW strFilterW(strFilter);
	return addProperty_File(strTitleW.GetBuffer(), strDescW.GetBuffer(), dwID, pParent, strFilterW.GetBuffer(), strOut);
}
bool CMFCPropertyGridCtrlEx::addProperty_File(wchar_t* strTitle, wchar_t* strDesc, DWORD dwID, CMFCPropertyGridProperty* pParent, wchar_t* strFilter, CString* strOut)
{
	// check params
	PropertyPage_Assert(strTitle);
	PropertyPage_Assert(strOut);
	PropertyPage_Assert(strFilter);
	if(strTitle == 0 || strOut == 0 || strFilter == 0) return false;

	CMFCPropertyGridFileProperty* pFileSelect1 = new CMFCPropertyGridFileProperty(strTitle, TRUE, _T(""), 0, 0, strFilter, strDesc);
	pFileSelect1->SetData(dwID);

	USES_CONVERSION;
	wchar_t wbuf[1024];
	COleVariant rVariant = pFileSelect1->GetValue();
	VARIANT v = rVariant.Detach();
	VARTYPE vt = v.vt;

	assert(vt == VT_BSTR);
	swprintf_s(wbuf, L"%s", OLE2W(v.bstrVal));
	*strOut = wbuf;

	if(pParent)	{pParent->AddSubItem(pFileSelect1);}
	else		{m_Property.AddProperty(pFileSelect1);}

	return true;
}
*/
/*
bool CMFCPropertyGridCtrlEx::addProperty_Folder(char* strTitle, DWORD dwID, CMFCPropertyGridProperty* pParent, char* strInitialFolder, CString* pFolderOut)
{
	PropertyPage_Assert(strTitle);
	PropertyPage_Assert(strInitialFolder);
	PropertyPage_Assert(pFolderOut);
	if(strTitle == 0 || strInitialFolder == 0 || pFolderOut == 0) return false;

	CStringW strTitleW(strTitle);
	CStringW strInitialFolderW(strInitialFolder);
	return addProperty_Folder(strTitleW.GetBuffer(), dwID, pParent, strInitialFolderW.GetBuffer(), pFolderOut);
}

bool CMFCPropertyGridCtrlEx::addProperty_Folder(wchar_t* strTitle, DWORD dwID, CMFCPropertyGridProperty* pParent, wchar_t* strInitialFolder, CString* pFolderOut)
{
	PropertyPage_Assert(strTitle);
	PropertyPage_Assert(strInitialFolder);
	PropertyPage_Assert(pFolderOut);
	if(strTitle == 0 || strInitialFolder == 0 || pFolderOut == 0) return false;

	CMFCPropertyGridFileProperty* pFolderSelect1 = new CMFCPropertyGridFileProperty(strTitle, strInitialFolder);
	pFolderSelect1->SetData(dwID);

	USES_CONVERSION;
	wchar_t wbuf[1024];
	COleVariant rVariant = pFolderSelect1->GetValue();
	VARIANT v = rVariant.Detach();
	VARTYPE vt = v.vt;

	assert(vt == VT_BSTR);
	swprintf_s(wbuf, L"%s", OLE2W(v.bstrVal));
	*pFolderOut = wbuf;

	if(pParent)	{pParent->AddSubItem(pFolderSelect1);}
	else		{AddProperty(pFolderSelect1);}

	return true;
}
*/
/*

void CMFCPropertyGridCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CMFCPropertyGridProperty* pProp = GetCurSel();

	if ( pProp == NULL )
	{
		printf( "selected = null\n" );
		return;
	}

	printf( "selected = %s\n", pProp->GetName() );

	CMFCPropertyGridCtrl::OnLButtonDown(nFlags, point);
}
*/



// 속성값 변경 이벤트를 이 컨트롤 자체에서 수행하고자 할 땐 아래 함수를 이용한다.
// void CMFCPropertyGridCtrlEx::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
// {
// 	AfxMessageBox( pProp->GetName() );
// }



void CMFCPropertyGridCtrlEx::OnSetFocus(CWnd* pOldWnd)
{
	CMFCPropertyGridCtrl::OnSetFocus(pOldWnd);

	//TRACE( "SetFocus\n" );
}


void CMFCPropertyGridCtrlEx::OnKillFocus(CWnd* pNewWnd)
{
	CMFCPropertyGridCtrl::OnKillFocus(pNewWnd);

	//TRACE( "KillFocus\n" );
}


int CMFCPropertyGridCtrlEx::OnDrawProperty(CDC* pDC, CMFCPropertyGridProperty* pProp) const
{
	// TODO: Add your specialized code here and/or call the base class
	//pDC->SetTextColor( RGB( 64, 0, 255 ) );

	return CMFCPropertyGridCtrl::OnDrawProperty(pDC, pProp);
}

//예전에는 m_nLeftColumnWidth값만 변경해주면 자동으로 너비가 조정되었었으나
//어느 패치 이후부터는 m_nLeftColumnWidth값이 항상 width / 2.0으로 복원되는 현상이 발생하여
//(WM_SIZE and WM_LBUTTONDBLCLK 등의 메시지로 인해)
//m_nLeftColumnWidth1 이라는 별도의 멤버변수에 저장하여 사용한다.
void CMFCPropertyGridCtrlEx::SetLeftColumnWidth( int cx )
{
	m_nLeftColumnWidth1 = cx;
	AdjustLayout();
}


void CMFCPropertyGridCtrlEx::AdjustLayout()
{
	m_nLeftColumnWidth = m_nLeftColumnWidth1;
	return CMFCPropertyGridCtrl::AdjustLayout();
}
