#if !defined _SIDEBAR_H
#define _SIDEBAR_H

/*
********** Author : Mohammed Nisamudheen S.  **********
********** Email  : nizam_clt@rediffmail.com **********
********** Date   : 21.10.2004               **********
********** URL    : www.uslindia.com         **********
*/

/* README:

  This is one of the very simplest implementation of an owner 
  drawn popup menu with side-bar-with-text and other colorful features.
  To use this menu simply include the header file where ever you
  want and just call the simple methods along with other CMenu
  functions.

  ->IMPORTENT

  Dont forget to add the MF_OWNERDRAW flag while appending new
  items to the menu.


*/

#include "afxwin.h"

class CSideBarMenu : public CMenu{

public:

	
	CSideBarMenu();

    void SetSideBarText(LPCSTR text);
	void SetSideBarTextColor(COLORREF color);
    void SetSideBarColor(COLORREF start, COLORREF end);
    void SetMenuBkgColor(COLORREF color);
	void SetHiLitColor(COLORREF color);
	void SetMenuTextColor(COLORREF color);
	void SetTextHiLitColor(COLORREF color);

	virtual void MeasureItem( LPMEASUREITEMSTRUCT lpMeasureItemStruct );
    virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

protected:


	HBITMAP GetGradientBitmap(HDC hDC,COLORREF clStart,COLORREF clEnd,int nWidth,int nHeight);


	CString m_SideBarText;

	COLORREF m_SideBarTextColor;
	COLORREF m_SideBarStartColor;
    COLORREF m_SideBarEndColor;
	COLORREF m_MenuBkgColor;
	COLORREF m_HiLitColor;
	COLORREF m_TextColor;
	COLORREF m_HiLitTextColor;

};


#endif