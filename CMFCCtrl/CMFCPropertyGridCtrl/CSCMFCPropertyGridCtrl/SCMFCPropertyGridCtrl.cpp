// MFCPropertyGridCtrlEx.cpp : 구현 파일입니다.
//

#include "SCMFCPropertyGridCtrl.h"
#include "../../../colors.h"

// CSCMFCPropertyGridCtrl

IMPLEMENT_DYNAMIC(CSCMFCPropertyGridCtrl, CMFCPropertyGridCtrl)

CSCMFCPropertyGridCtrl::CSCMFCPropertyGridCtrl()
{
	m_nLeftColumnWidth1 = 100;
}

CSCMFCPropertyGridCtrl::~CSCMFCPropertyGridCtrl()
{

}


BEGIN_MESSAGE_MAP(CSCMFCPropertyGridCtrl, CMFCPropertyGridCtrl)
	//ON_WM_LBUTTONDOWN()
	//ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()



// CSCMFCPropertyGridCtrl 메시지 처리기입니다.

int CSCMFCPropertyGridCtrl::GetGroupIndex(CString sName)		//find group(property by name
{
	int		i;
	CString	sPropertyName;

	sName.MakeLower();

	for (i = 0; i < GetPropertyCount(); i++)
	{
		sPropertyName = GetProperty(i)->GetName();
		sPropertyName.MakeLower();

		if (sPropertyName.Find(sName) >= 0)
			return i;
	}

	return -1;
}

CMFCPropertyGridProperty* CSCMFCPropertyGridCtrl::GetGroupProperty(CString sName)		//find group(property by name
{
	int		nIndex = GetGroupIndex(sName);

	if (nIndex >= 0)
		return GetProperty(nIndex);

	return NULL;
}

CMFCPropertyGridProperty* CSCMFCPropertyGridCtrl::GetItemProperty(DWORD dwData)
{
	CMFCPropertyGridProperty* pProperty = FindItemByData(dwData);
	return pProperty;
	/*
	int		i, j;
	CMFCPropertyGridProperty* pGroup;
	CMFCPropertyGridProperty* pSubItem;

	for (i = 0; i < GetPropertyCount(); i++)
	{
		pGroup = GetProperty(i);

		for (j = 0; j < pGroup->GetSubItemsCount(); j++)
		{
			pSubItem = pGroup->GetSubItem(j);
			if (pSubItem->GetData() == dwData)
				return pSubItem;
		}
	}
	*/

	return NULL;
}

CMFCPropertyGridProperty* CSCMFCPropertyGridCtrl::GetItemProperty(CString sGroup, CString sName)
{
	int		nGroup = GetGroupIndex(sGroup);
	int		nIndex = GetItemIndex(nGroup, sName);

	return GetProperty(nGroup)->GetSubItem(nIndex);
}

CString	CSCMFCPropertyGridCtrl::GetItemText(int nGroup, int nItem)
{
	return GetProperty(nGroup)->GetSubItem(nItem)->GetValue();
}

CString	CSCMFCPropertyGridCtrl::GetItemText(int nGroup, CString sName)		//find by name and return text
{
	return GetItemText(nGroup, GetItemIndex(nGroup, sName));
}

CString CSCMFCPropertyGridCtrl::GetItemText(CString sGroup, CString sName)	//find by name and return text(value)
{
	return GetItemText(GetGroupIndex(sGroup), sName);
}

int CSCMFCPropertyGridCtrl::GetItemIndex(int nGroup, CString sName)
{
	int		i;
	CString sItemName;
	CMFCPropertyGridProperty* pProp = GetProperty(nGroup);

	if (!pProp)
		return -1;

	sName.MakeLower();

	for (i = 0; i < pProp->GetSubItemsCount(); i++)
	{
		sItemName = pProp->GetSubItem(i)->GetName();
		sItemName.MakeLower();

		//full matching이 아닌 넘어온 문자열을 포함하는 경우도 리턴해준다.
		if (sItemName.Find(sName) >= 0)
			return i;
	}

	return -1;
}

void CSCMFCPropertyGridCtrl::SetItemText(int nGroup, int nItem, CString sText /*= ""*/)
{
	GetProperty(nGroup)->GetSubItem(nItem)->SetValue(sText);
}

void CSCMFCPropertyGridCtrl::SetItemText(int nGroup, CString sName, CString sText)
{
	CString sItemName;
	CMFCPropertyGridProperty* pProp = GetProperty(nGroup);
	
	if (!pProp)
		return;

	SetItemText(nGroup, GetItemIndex(nGroup, sName), sText);
}

void CSCMFCPropertyGridCtrl::SetItemText(CString sGroup, CString sName, CString sText)
{
	int		nGroup = GetGroupIndex(sGroup);
	SetItemText(nGroup, GetItemIndex(nGroup, sName), sText);
}

void CSCMFCPropertyGridCtrl::SetCurSel(int nGroup, int nName, int nOption)	//for combo style
{
	CString	sOption	= GetProperty(nGroup)->GetSubItem(nName)->GetOption(nOption);
	SetItemText(nGroup, nName, sOption);
}

void CSCMFCPropertyGridCtrl::SetCurSel(CString sGroup, CString sName, int nOption)	//for combo style
{
	int		nGroup	= GetGroupIndex(sGroup);
	int		nIndex	= GetItemIndex(nGroup, sName);
	CString	sOption	= GetProperty(nGroup)->GetSubItem(nIndex)->GetOption(nOption);
	SetItemText(nGroup, nIndex, sOption);
}

void CSCMFCPropertyGridCtrl::SetCurSel(DWORD dwData, int nOption)
{
	CString sOption = GetItemProperty(dwData)->GetOption(nOption);
	GetItemProperty(dwData)->SetValue(sOption);
}

int	CSCMFCPropertyGridCtrl::GetCurSel(int nGroup, int nItem)
{
	int		i;
	int		nOption = GetProperty(nGroup)->GetSubItem(nItem)->GetOptionCount();
	CString	sOption;
	CString sCurrentOption = GetProperty(nGroup)->GetSubItem(nItem)->GetValue();

	for (i = 0; i < nOption; i++)
	{
		sOption = GetProperty(nGroup)->GetSubItem(nItem)->GetOption(i);
		if (sOption == sCurrentOption)
			return i;
	}

	return -1;
}

int	CSCMFCPropertyGridCtrl::GetCurSel(DWORD dwData)
{
	int		i;
	int		nOption = GetItemProperty(dwData)->GetOptionCount();
	CString	sOption;
	CString sCurrentOption = GetItemProperty(dwData)->GetValue();

	for (i = 0; i < nOption; i++)
	{
		sOption = GetItemProperty(dwData)->GetOption(i);
		if (sOption == sCurrentOption)
			return i;
	}

	return -1;
}

void CSCMFCPropertyGridCtrl::EnableItem(int nGroup, int nItem, BOOL bEnable)
{
	GetProperty(nGroup)->GetSubItem(nItem)->Enable(bEnable);
}

void CSCMFCPropertyGridCtrl::EnableItem(int nGroup, CString sName, BOOL bEnable)
{
	EnableItem(nGroup, GetItemIndex(nGroup, sName), bEnable);
}

void CSCMFCPropertyGridCtrl::EnableItem(CString sGroup, CString sName, BOOL bEnable)
{
	EnableItem(GetGroupIndex(sGroup), sName, bEnable);
}

CMFCPropertyGridProperty* CSCMFCPropertyGridCtrl::AddChildItem(CMFCPropertyGridProperty* pParent, CString sName, _variant_t varValue /*= ""*/,
												CString sDescription /* = ""*/, bool bEnable /*= FALSE*/, DWORD dwData /*= NULL*/,
												bool bUseSpin, int min, int max)
{
	CMFCPropertyGridProperty* pProperty = new CMFCPropertyGridProperty(sName, varValue, sDescription, dwData);
	pProperty->Enable(bEnable);

	if (bUseSpin)
		pProperty->EnableSpinControl(true, min, max);

	pParent->AddSubItem(pProperty);

	return pProperty;
}

BOOL CSCMFCPropertyGridCtrl::AddBoolItem(CMFCPropertyGridProperty* pParent, CString sName, bool bDefault, CString sDescription, bool bEnable, DWORD dwData)
{
	auto pProperty = new CMFCPropertyGridProperty(sName, (_variant_t)(bDefault ? true : false), sDescription, dwData);
	pProperty->Enable(bEnable);
	return pParent->AddSubItem(pProperty);
}
/*
BOOL CSCMFCPropertyGridCtrl::AddOptionItem(CMFCPropertyGridProperty* pParent, CString sName, CString sDefault, CString sDescription, bool bAllowEdit, bool bEnable, DWORD dwData, int nOption, CString lpszFormat, ...)
{
	CString*	pString = new CString[nOption];
	LPCTSTR		sOption;
	va_list		ap;

	va_start(ap, lpszFormat);
	
	pString[0] = lpszFormat;

	for (int i = 1; i < nOption; i++)
	{
		sOption = va_arg(ap, LPCTSTR);
		pString[i] = sOption;
	}

	va_end(ap);

	AddOptionItem(pParent, sName, sDefault, sDescription, bAllowEdit, bEnable, dwData, nOption, pString);

	delete [] pString;

	return true;
}
*/
BOOL CSCMFCPropertyGridCtrl::AddOptionItem(CMFCPropertyGridProperty* pParent, CString sName, CString sDefault, CString sDescription,
											bool bAllowEdit, bool bEnable, DWORD dwData, int nOption, CString* sOptionArray)
{
	CMFCPropertyGridProperty* pProp = new CMFCPropertyGridProperty(sName, sDefault, sDescription, dwData);

	int		i;

	for (i = 0; i < nOption; i++)
	{
		pProp->AddOption((LPCTSTR)sOptionArray[i]);
	}

	pProp->AllowEdit(bAllowEdit);
	pProp->Enable(bEnable);

	return pParent->AddSubItem(pProp);
}

bool CSCMFCPropertyGridCtrl::AddFileSelect(CMFCPropertyGridProperty* pParent, CString sName, const _variant_t& varValue,
											    TCHAR* sFilter, CString sDescription, DWORD dwData)
{
	CMFCPropertyGridFileProperty* pFileSelect = new CMFCPropertyGridFileProperty(sName, TRUE, varValue, 0, 0, sFilter, sDescription, dwData);

	/*
	USES_CONVERSION;
	//wchar_t wbuf[1024];
	COleVariant rVariant = pFileSelect->GetValue();
	VARIANT v = rVariant.Detach();
	VARTYPE vt = v.vt;

	ASSERT(vt == VT_BSTR);
	//swprintf_s(wbuf, L"%s", OLE2W(v.bstrVal));
	//*sSelected = wbuf;
	*/

	if (pParent)
		pParent->AddSubItem(pFileSelect);
	else
		AddProperty(pFileSelect);

	return true;
}


bool CSCMFCPropertyGridCtrl::AddFolderSelect(	CMFCPropertyGridProperty* pParent, CString sName,
												CString sInitialFolder, CString sDescription,
												bool bEnable, DWORD dwData)
{
	CMFCPropertyGridFileProperty* pFolderSelect = new CMFCPropertyGridFileProperty(sName, sInitialFolder, dwData, sDescription);
	pFolderSelect->Enable(bEnable);

	if (pParent)
		pParent->AddSubItem(pFolderSelect);
	else
		AddProperty(pFolderSelect);

	return true;
}

bool CSCMFCPropertyGridCtrl::AddSliderItem(CMFCPropertyGridProperty* pParent, CString sName, int min, int max, int value, DWORD dwData)
{
	CPropGridSlider* prop = new CPropGridSlider(sName, value, sName, min, max, max - min, dwData);
	return pParent->AddSubItem(prop);
}

bool CSCMFCPropertyGridCtrl::AddFontSelect(CMFCPropertyGridProperty* parent, CString name, LOGFONT lf, DWORD id, CString desc, bool enable)
{
	CMFCPropertyGridProperty* prop = new CMFCPropertyGridFontProperty(name, lf, CF_EFFECTS | CF_SCREENFONTS, desc, id);
	prop->Enable(enable);
	return parent->AddSubItem(prop);
}

bool CSCMFCPropertyGridCtrl::AddColorSelect(CMFCPropertyGridProperty* parent, CString name, COLORREF crDefault, DWORD id, CString desc, bool enable)
{
	CMFCPropertyGridColorProperty* prop = new CMFCPropertyGridColorProperty(name, crDefault, NULL, desc, id);
	prop->Enable(enable);
	return parent->AddSubItem(prop);
}

/*
bool CSCMFCPropertyGridCtrl::addProperty_File(char* strTitle, char* strDesc, DWORD dwID, CMFCPropertyGridProperty* pParent, char* strFilter, CString* strOut)
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
bool CSCMFCPropertyGridCtrl::addProperty_File(wchar_t* strTitle, wchar_t* strDesc, DWORD dwID, CMFCPropertyGridProperty* pParent, wchar_t* strFilter, CString* strOut)
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
bool CSCMFCPropertyGridCtrl::addProperty_Folder(char* strTitle, DWORD dwID, CMFCPropertyGridProperty* pParent, char* strInitialFolder, CString* pFolderOut)
{
	PropertyPage_Assert(strTitle);
	PropertyPage_Assert(strInitialFolder);
	PropertyPage_Assert(pFolderOut);
	if(strTitle == 0 || strInitialFolder == 0 || pFolderOut == 0) return false;

	CStringW strTitleW(strTitle);
	CStringW strInitialFolderW(strInitialFolder);
	return addProperty_Folder(strTitleW.GetBuffer(), dwID, pParent, strInitialFolderW.GetBuffer(), pFolderOut);
}

bool CSCMFCPropertyGridCtrl::addProperty_Folder(wchar_t* strTitle, DWORD dwID, CMFCPropertyGridProperty* pParent, wchar_t* strInitialFolder, CString* pFolderOut)
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

void CSCMFCPropertyGridCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CMFCPropertyGridProperty* pProp = GetCurSel();

	if (pProp == NULL)
	{
		printf("selected = null\n");
		return;
	}

	printf("selected = %s\n", pProp->GetName());

	CMFCPropertyGridCtrl::OnLButtonDown(nFlags, point);
}
*/



// 속성값 변경 이벤트를 이 컨트롤 자체에서 수행하고자 할 땐 아래 함수를 이용한다.
// void CSCMFCPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
// {
// 	AfxMessageBox(pProp->GetName());
// }



void CSCMFCPropertyGridCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CMFCPropertyGridCtrl::OnSetFocus(pOldWnd);

	//TRACE("SetFocus\n");
}


void CSCMFCPropertyGridCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CMFCPropertyGridCtrl::OnKillFocus(pNewWnd);

	//TRACE("KillFocus\n");
}


int CSCMFCPropertyGridCtrl::OnDrawProperty(CDC* pDC, CMFCPropertyGridProperty* pProp) const
{
	// TODO: Add your specialized code here and/or call the base class
	//pDC->SetTextColor(RGB(64, 0, 255));

	return CMFCPropertyGridCtrl::OnDrawProperty(pDC, pProp);
}

//예전에는 m_nLeftColumnWidth값만 변경해주면 자동으로 너비가 조정되었었으나
//어느 패치 이후부터는 m_nLeftColumnWidth값이 항상 width / 2.0으로 복원되는 현상이 발생하여
//(WM_SIZE and WM_LBUTTONDBLCLK 등의 메시지로 인해)
//m_nLeftColumnWidth1 이라는 별도의 멤버변수에 저장하여 사용한다.
void CSCMFCPropertyGridCtrl::SetLeftColumnWidth(int cx)
{
	m_nLeftColumnWidth1 = cx;
	AdjustLayout();
}


void CSCMFCPropertyGridCtrl::AdjustLayout()
{
	m_nLeftColumnWidth = m_nLeftColumnWidth1;
	return CMFCPropertyGridCtrl::AdjustLayout();
}

//clrBackground,
//clrText,
//clrGroupBackground,
//clrGroupText,
//clrDescriptionBackground,
//clrDescriptionText,
//clrLine
void CSCMFCPropertyGridCtrl::set_color_theme(int theme)
{
	switch (theme)
	{
		case theme_default :
		{
			SetCustomColors(
				(COLORREF)-1,
				(COLORREF)-1,
				(COLORREF)-1,
				(COLORREF)-1,
				(COLORREF)-1,
				(COLORREF)-1,
				(COLORREF)-1
			);
			break;
		}
		case theme_gray:
		{
			SetCustomColors(
				::GetSysColor(COLOR_3DFACE),
				::GetSysColor(COLOR_BTNTEXT),
				::GetSysColor(COLOR_3DFACE),
				::GetSysColor(COLOR_BTNTEXT),
				::GetSysColor(COLOR_3DFACE),
				::GetSysColor(COLOR_BTNTEXT),
				get_color(::GetSysColor(COLOR_3DFACE), -8));
			break;
		}
		case theme_dark:
		{
			SetCustomColors(
				RGB(37, 37, 38),
				RGB(192, 192, 192),
				RGB(45, 45, 48),
				RGB(192, 192, 192),
				RGB(37, 37, 38),
				RGB(192, 192, 192),
				RGB(52, 52, 56));
			break;
		}
	}

	Invalidate();
}

