// ResizeCtrl.h: interface for the CResizeCtrl class.
//
//
// Written by Herbert Menke (h.menke@gmx.de)
// Copyright (c) 2000.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. If 
// the source code in  this file is used in any commercial application 
// then acknowledgement must be made to the author of this file 
// (in whatever form you wish).
//
// This file is provided "as is" with no expressed or implied warranty.
//
// Expect bugs.
// 
// Please use and enjoy. Please let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into this
// file. 
//
//////////////////////////////////////////////////////////////////////


/*usage
1.	in .h file
#include "ResizeCtrl.h"
	CResizeCtrl		m_resize;

2.	in cpp...
	m_resize.Create(this);
	m_resize.Add(IDC_EDIT1,   0,   0, 100,  50);
	m_resize.SetMinimumTrackingSize();

//	*�Ķ���Ͱ� l, t, r, b�� �ƴ� l, t, w, h�ӿ� ��������. ���� �� ���� resize�Ǵ� ũ�⿡ ���� �����̴�.

//  divided into 3 parts with the horizontal
// 	m_resize.Create(this, true, 1000);
// 	m_resize.Add(IDC_STATIC_IMAGE, 0, 0, 333, 1000);
// 	m_resize.Add(IDC_STATIC_RESULT, 333, 0, 333, 1000);
// 	m_resize.Add(IDC_STATIC_FINAL, 666, 0, 333, 1000);

����! : m_resize���� � ��Ʈ���� �������� ������ ��쿡�� �ݵ�� ������ �� �ٽ� �־���� ����� �����Ѵ�.

* scpark 2023041010
https://stackoverflow.com/questions/39731497/create-window-without-titlebar-with-resizable-border-and-without-bogus-6px-whit
������10������ DWM��å�� ���� ���������ΰ� Ÿ��Ʋ�ٰ� ���� resizable dialog�� ����� ��ܿ� ��� ������ ǥ�õȴ�.
�̴� ���׶�� �� �� ���� DWM��å�� ���� ���ܳ� ���ۿ��̶� �Ѵ�.

�̸� �����ϴ� ����� 2������ �ִµ�

1.�� ��α�ó�� �ذ��ϴ� ���
	- ���� : �׳��� �������� ó�� ���
	- ���� : ������10�� 7 & Vista�� ó���ڵ尡 �ణ �޶�����.

2.border�� none�̳� thin���� �ϰ� SetCursor�� OnMouseMove, OnLButtonDown�� �̿��Ͽ� ����
	- ���� : resizable ������ ũ�⸦ ���ϴ� ��ŭ �� �� �ִ�.
	- ���� : CResizeCtrl�� ���� Dialog�� �ڵ����� WM_THICKFRAME �Ӽ��� �ο��ǹǷ� ���� ��ܿ� ��� ������ ����Ƿ� �� ����� �� �� ����.
*/
//////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESIZECTRL_H__3CD44893_48A2_11D4_880C_00902755BD88__INCLUDED_)
#define AFX_RESIZECTRL_H__3CD44893_48A2_11D4_880C_00902755BD88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Afxwin.h>
#include <Afxdisp.h>

class CResizeArray;

struct CResizeInfo
{
	int ctlID;  // Specifies the identifier of the control 
	int left;   // Specifies the  change in the position of the left edge 
				// of the object relative to the total change in the parent window�s width. 
	int top;    // Specifies the  change in the position of the top 
				// of the object relative to the total change in the parent window�s height.
	int width;  // Specifies the  change in the width of the object 
				// relative to the total change in the parent window�s width.
	int height; // Specifies the  change in the height of the object 
				// relative to the total change in the parent window�s height.
};

class CResizeCtrl
{
public:

	//
	// Add Method
	//

	// Adds a control window to the list to be resized

	// left, top, width and height determines how the size and position of the control window
	// will change when the size of the parent window changes.

	// In general, the formula is	
	//
	//    newValue = oldValue + ((deltaValueParent * partValue) / maxPart);
	//
	// newValue          - new left or top position or new width or height
	// oldValue          - old left or top position or old width or height 
	// deltaValueParent  - changes in parent width or height
	// partValue         - left, top, width or height value specified in the Add Method
	// maxPart           - value specified by the maxPart parameter of the constructor
	//                     or the Create method

	//
	// left   - Specifies the change in the position of the left edge 
	//          of the object relative to the total change in the parent window�s width. 
	//          For example: suppose that left is 50 and the width of the window increases by 200 pixels. 
	//          Then the left edge of the object moves right by 100 pixels (50% of 200).
	// top    - Specifies the change in the top position
	//          of the object relative to the total change in the parent window�s height.
	// width  - Specifies the  change in the width of the object 
	//          relative to the total change in the parent window�s width.
	//          For example: suppose that width is zero. Then the width of the object does not change, 
	//                       regardless of how much the width of the parent window changes.
	//                       suppose that width is 100 and the width of the window decreases by 50 pixels. 
	//                       Then the width of the object also decreases by 50 pixels (100% of 50).
	// height - Specifies the  change in the height of the object 
	//          relative to the total change in the parent window�s height.

	BOOL Add(HWND hWndCtl, int ctrlID, int left, int top, int width, int height);
	BOOL Add(int ctrlID, int left, int top, int width, int height);
	BOOL Add(CWnd * wndCtl, int ctrlID, int left, int top, int width, int height);

	// resizeInfo is a null terminated array of CResizeInfo

	BOOL Add(const CResizeInfo * resizeInfo);

	//
	// Remove Method
	//

	// Removes a control window from the list to be resized

	BOOL Remove(HWND hWndCtl);
	BOOL Remove(int ctlID);
	BOOL Remove(CWnd * wndCtl);
	BOOL RemoveAll();

	// enabled
	// TRUE    returns the position and size before resizing was enabled
	// FALSE   returns the last position and size before resizing was disabled

	BOOL GetWindowRect(RECT * rect);

	//
	// Construction
	//

	BOOL Create(HWND hWndParent, BOOL enabled = TRUE, int maxPart = 100);
	BOOL Create(CWnd * wndParent, BOOL enabled = TRUE, int maxPart = 100);

	CResizeCtrl();
	CResizeCtrl(HWND hWndParent, BOOL enabled = TRUE, int maxPart = 100);
	CResizeCtrl(CWnd * wndParent, BOOL enabled = TRUE, int maxPart = 100);
	virtual ~CResizeCtrl();

	//
	// Enabled Property
	//

	// enable or disable resizing

	BOOL SetEnabled(BOOL enable);
	BOOL GetEnabled() const;

	//
	// GripEnabled Property
	//

	BOOL SetGripEnabled(BOOL showGrip);
	BOOL GetGripEnabled() const;

	///////////////////////////////////////////////////////////////////////
	//
	// MinMax Support
	//
	BOOL  SetMinimumTrackingSize(const CSize & size);
	BOOL  SetMinimumTrackingSize();
	CSize GetMinimumTrackingSize();

	BOOL  SetMaximumTrackingSize(const CSize & size);
	CSize GetMaximumTrackingSize();

private:
	void GetGripRect(RECT & rect, BOOL redraw);
	// Resize the controls
	void Resize(int cx, int cy);
	// processes the messages for resizing
	BOOL ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT & result);
	// adds or removes style WS_THICKFRAME of the aprent window
	void ChangeStyle(BOOL enable);
	// initialise the control
	// calculates the new left, top, width or height 
	BOOL CalcValue(int delta, int part, int & pending, long & position, BOOL isSize);
	// subclass window proc
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	BOOL                              m_hasResizingBorder; // TRUE if the parent has a resizing border
	WNDPROC                           m_prevWndProc;       // previous Window Proc 
	BOOL                              m_enabled;           // parent is subclassed
	int                               m_maxPart;           // max PartValue
	HWND                              m_hWndParent;        // handle of the paretn window
	CSize                             m_size;              // width and heigth of the parent window
	CResizeArray                    * m_array;             // parameters of registered controls
	/////////////////////////////////////////////////
	CSize                             m_maxTracking;       // parameters for max Tracking
	CSize                             m_minTracking;       // parameters for min Tracking
  /////////////////////////////////////////////////
	BOOL                              m_inResize;          // flag to prevent recursion in WM_SIZE
  /////////////////////////////////////////////////
	int                               m_hitCode;           // stored hitCode from WM_NCLBUTTONDOWN
	CSize                             m_delta;             // stored delta Size from WM_NCLBUTTONDOWN 
	BOOL                              m_inMouseMove;       // flag to prevent recursion in WM_MOUSEMOVE
	/////////////////////////////////////////////////
	CRect                             m_gripRect;          // position and size of the grip rect
	BOOL                              m_gripEnabled;       // TRUE, if grip is enabled
	/////////////////////////////////////////////////
	CRect                             m_windowRect;        // last Window position and Size
};

#endif // !defined(AFX_RESIZECTRL_H__3CD44893_48A2_11D4_880C_00902755BD88__INCLUDED_)
