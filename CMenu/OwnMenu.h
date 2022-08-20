// OwnMenu.h: Schnittstelle f? die Klasse COwnMenu.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OWNMENU_H__9BC986A8_8079_4A58_BECA_2E156BBCFA4F__INCLUDED_)
#define AFX_OWNMENU_H__9BC986A8_8079_4A58_BECA_2E156BBCFA4F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>

struct MenuObject
{
	HICON m_hIcon;
	CString m_strCaption;
	BOOL bFirstMenu;
};

class COwnMenu : public CMenu  
{
public:
	CWnd cwnd;
	CEdit m_edit;
	void FillFluentRect(HDC hDC, RECT rect, byte r1, byte g1, byte b1, byte r2, byte g2, byte b2);
	void MakeItemsOwnDraw(BOOL bFirst = FALSE);
	void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	COwnMenu();
	virtual ~COwnMenu();
	std::vector<DWORD> deleteItem;
	std::vector<DWORD> deleteMenu;

};

#endif // !defined(AFX_OWNMENU_H__9BC986A8_8079_4A58_BECA_2E156BBCFA4F__INCLUDED_)
