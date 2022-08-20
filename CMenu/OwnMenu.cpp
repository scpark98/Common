// OwnMenu.cpp: Implementierung der Klasse COwnMenu.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OwnMenu.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

LRESULT CALLBACK MenuProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
WNDPROC oldMenuProc;


COwnMenu::COwnMenu()
{
	deleteItem.clear();
	deleteMenu.clear();
}

COwnMenu::~COwnMenu()
{
	for (int i = 0; i < deleteItem.size(); i++)
	{
		delete ((MenuObject*)deleteItem[i]);
	}

	for (int i = 0; i < deleteMenu.size(); i++)
	{
		delete ((COwnMenu*)deleteMenu[i]);
	}
}

void COwnMenu::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CRect rectFull(lpDrawItemStruct->rcItem);
	CRect rectIcon(rectFull.left, rectFull.top, rectFull.left + 20, rectFull.top + 20);
	CRect rectText(rectIcon.right, rectFull.top, rectFull.right, rectFull.bottom);

	COLORREF IconRectLeft = COLORREF(RGB(0, 0, 255));
	COLORREF IconRectRight = COLORREF(RGB(0, 255, 0));
	COLORREF TextRect = RGB(255, 128, 128);
	CRect rectBorder = rectFull;
	//rectBorder.left -= 3;
	rectBorder.right -= 1;
	CRect rectFill = rectBorder;
	rectFill.left += 1;
	rectFill.right -= 1;
	rectFill.top += 1;
	rectFill.bottom -= 1;

	if (((MenuObject*)lpDrawItemStruct->itemData)->bFirstMenu)
	{
		ZeroMemory(&rectIcon, sizeof(CRect));
		rectText = rectFull;
		TextRect = RGB(255, 255, 128);//GetSysColor(COLOR_BTNFACE);// COLORREF(RGB(192,192,192));
	}

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	FillFluentRect(pDC->GetSafeHdc(),
		rectIcon, 255, 0, 0, 0, 255, 0);
	//::FillRect(pDC->GetSafeHdc(), &rectIcon, 
		   //         CreateSolidBrush(IconRectRight));

	pDC->FillSolidRect(&rectText,
		TextRect);





	//if the menu item is selected
	if ((lpDrawItemStruct->itemState & ODS_SELECTED) &&
		(lpDrawItemStruct->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		//draw XP Rect
		TextRect = COLORREF(RGB(255, 0, 255));
		pDC->FillSolidRect(&rectBorder, COLORREF(RGB(0, 255, 255)));
		pDC->FillSolidRect(&rectFill, TextRect);
	}




	MenuObject *pData = (MenuObject*)lpDrawItemStruct->itemData;
	//if (pData->m_hIcon == NULL)
	if (true)
	{
		pDC->SetBkColor(TextRect);
		rectText.left += 5;
		rectText.top += 1;
		rectText.bottom += 1;
		pDC->DrawText(((MenuObject*)lpDrawItemStruct->itemData)->m_strCaption, rectText,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
	}
}

void COwnMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	MenuObject *pData = (MenuObject*)lpMeasureItemStruct->itemData;
	//if (pData->m_hIcon == NULL)
	if (true)
	{
		lpMeasureItemStruct->itemHeight = 32;
		lpMeasureItemStruct->itemWidth = ((MenuObject*)lpMeasureItemStruct->itemData)->m_strCaption.GetLength() * 8;
	}
	else
	{
		lpMeasureItemStruct->itemHeight = 10;
		lpMeasureItemStruct->itemWidth = 50;//((MenuObject*)lpMeasureItemStruct->itemData)->m_strCaption.GetLength() * 8;
	}
}

void COwnMenu::MakeItemsOwnDraw(BOOL bFirst)
{
	int iMaxItems = GetMenuItemCount();
	for (int i = 0; i < iMaxItems; i++)
	{
		CString nameHolder;
		MenuObject* pObject = new MenuObject;
		deleteItem.push_back((DWORD)pObject);
		pObject->m_hIcon = NULL;
		pObject->bFirstMenu = bFirst;
		GetMenuString(i, pObject->m_strCaption, MF_BYPOSITION);
		//if (pObject->m_strCaption.IsEmpty())
			//continue;

		MENUITEMINFO mInfo;
		ZeroMemory(&mInfo, sizeof(MENUITEMINFO));
		UINT uID = mInfo.wID; //I dont use GetMenuItemID because it doesn't return 0/-1 when it's a Popup (so the MSDN is wrong)
		ModifyMenu(i, MF_BYPOSITION | MF_OWNERDRAW,
			uID, (LPCTSTR)pObject);


		if (GetSubMenu(i))
		{
			COwnMenu* pSubMenu = new COwnMenu;
			deleteMenu.push_back((DWORD)pSubMenu);
			pSubMenu->Attach(GetSubMenu(i)->GetSafeHmenu());
			pSubMenu->MakeItemsOwnDraw();
		}
	}
}

void COwnMenu::FillFluentRect(HDC hDC, RECT rect, byte r1, byte g1, byte b1, byte r2, byte g2, byte b2)
{
	int iWidth = rect.right - rect.left;
	int iHeight = rect.bottom - rect.top;

	short rDif = r2 - r1;
	short gDif = g2 - g1;
	short bDif = b2 - b1;

	for (int i = 0; i < iWidth; i++)
	{
		byte rCur, gCur, bCur;
		rCur = r1 + (short)(float)(((float)rDif / (float)iWidth)*(float)i);
		gCur = g1 + (short)(float)(((float)gDif / (float)iWidth)*(float)i);
		bCur = b1 + (short)(float)(((float)bDif / (float)iWidth)*(float)i);

		for (int y = 0; y < iHeight; y++)
			SetPixel(hDC, rect.left + i, rect.top + y,
				RGB(rCur, gCur, bCur));

	}
}

LRESULT CALLBACK MenuProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_CREATE:
		MessageBox(0, _T("hallo"), 0, 0);
		break;

	case WM_SIZE:
		MessageBox(0, _T("hallO"), 0, 0);
		break;

	case WM_RBUTTONDOWN:
		MessageBox(0, _T("bla"), 0, 0);

		return 0;
		break;
	}
	return ::CallWindowProc(oldMenuProc, hWnd, message, wParam, lParam);
}