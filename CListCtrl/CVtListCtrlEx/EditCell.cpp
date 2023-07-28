// EditCell.cpp : implementation file
//

//#include "stdafx.h"
#include "EditCell.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// gxEditCell

gxEditCell::gxEditCell (CVtListCtrlEx* pCtrl, int iItem, int iSubItem, CString sInitText)
:   bEscape (FALSE)
{
    m_pListCtrl = pCtrl;
    Item = iItem;
    SubItem = iSubItem;
    InitText = sInitText;
}

gxEditCell::~gxEditCell()
{
}

BEGIN_MESSAGE_MAP(gxEditCell, CEdit)
    //{{AFX_MSG_MAP(gxEditCell)
    ON_WM_KILLFOCUS()
    ON_WM_NCDESTROY()
    ON_WM_CHAR()
    ON_WM_CREATE()
    ON_WM_GETDLGCODE()
	ON_WM_KEYUP()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_WM_NCHITTEST()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_ACTIVATE()
	ON_WM_ACTIVATEAPP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// gxEditCell message handlers

void gxEditCell::SetListText()
{
	//이 함수가 두번 이상 호출된다.
	//두 번째는 이미 이 EditCell이 종료되는 시점에서 호출되어
	//윈도우 핸들이 유효하지 않다.
	if (!IsWindow(m_hWnd))
		return;

    CString Text;
    GetWindowText (Text);

    // Send Notification to parent of ListView ctrl
    LV_DISPINFO dispinfo;
    dispinfo.hdr.hwndFrom = m_pListCtrl->m_hWnd;
    dispinfo.hdr.idFrom = GetDlgCtrlID();
    dispinfo.hdr.code = LVN_ENDLABELEDIT;

    dispinfo.item.mask = LVIF_TEXT;
    dispinfo.item.iItem = Item;
    dispinfo.item.iSubItem = SubItem;
    dispinfo.item.pszText = bEscape ? NULL : LPTSTR ((LPCTSTR) Text);
    dispinfo.item.cchTextMax = Text.GetLength();

	m_pListCtrl->GetParent()->SendMessage (WM_NOTIFY, m_pListCtrl->GetDlgCtrlID(), (LPARAM) &dispinfo);
	//위와 같이 호출해야 main dlg에 메시지가 전달된다. 아래와 같이하면 main dlg에 안간다.
	//ListCtrl에서는 이와 관계없이 LVN_ENDLABELEDIT 이벤트 핸들러가 호출된다.
	//m_pListCtrl->SendMessage (WM_NOTIFY, m_pListCtrl->GetDlgCtrlID(), (LPARAM) &dispinfo);
}

BOOL gxEditCell::PreTranslateMessage (MSG* pMsg) 
{
    if (pMsg->message == WM_KEYDOWN)
    {
	    if (
			/*pMsg->wParam == VK_RETURN || pMsg->wParam == VK_DELETE || 
			pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT ||
			pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_TAB || 
			pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN ||
			pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT ||
			GetKeyState (VK_CONTROL)
			*/
			true)
			{
				::TranslateMessage (pMsg);
				::DispatchMessage (pMsg);
				return TRUE;		    	// DO NOT process further
			}
    }

    return CEdit::PreTranslateMessage (pMsg);
}

void gxEditCell::OnKillFocus (CWnd* pNewWnd) 
{
    CEdit::OnKillFocus(pNewWnd);

	//아래 한줄 SetListText();를 호출하면 편집중에 다른 곳을 클릭하여 포커스를 잃었을때
	//편집 완료로 할 것인지 편집 취소로 할 것인지 그 동작을 선택할 수 있다.
	//SetListText();를 호출해주면 포커스를 잃었을 때 편집 완료로 처리된다.(엑셀처럼)
	//단, 아래 TRACE에서 확인한 것 처럼 편집이 시작된 컨트롤과 새로 포커스를 가지게 되는 컨트롤이 다를 경우에는
	//editbox의 값이 새로 포커스를 가진 컨트롤의 값으로 할당되는 오류가 있으므로
	//두 컨트롤이 같을 경우에만 자동 입력 완료되도록 해야 한다.
	//TRACE("m_pListCtrl = %p, pNewWnd = %p\n", m_pListCtrl, pNewWnd);

	//if (m_pListCtrl == pNewWnd)
	{
		//편집모드일때만 편집완료 및 종료플래그 변경을 해야한다.
		//그렇지 않으면 OnLvnEndlabeledit()이 두번씩 호출된다.
		if (m_pListCtrl->is_in_editing())
		{
			SetListText();
			m_pListCtrl->set_flag_in_editing(false);
		}
		DestroyWindow();
	}
	//else
	//{

	//	CString Text;
	//	GetWindowText(Text);
	//	bool b = m_pListCtrl->SetItemText(Item, SubItem, Text);
	//	CString str = m_pListCtrl->GetItemText(Item, SubItem);
	//	m_pListCtrl->Update(Item);
	//	m_pListCtrl->SetFlagInEditing(false);
	//	DestroyWindow();
	//}
}

void gxEditCell::OnNcDestroy() 
{
    CEdit::OnNcDestroy();
    
    delete this;
}

void gxEditCell::OnKeyDown (UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void gxEditCell::OnKeyUp (UINT nChar, UINT nRepCnt, UINT nFlags) 
{
 	/*
   switch (nChar)
    {
		case VK_NEXT :
		{
			int Count = m_pListCtrl->GetItemCount();
			int NewItem = Item + m_pListCtrl->GetCountPerPage();
			if (Count > NewItem)
				m_pListCtrl->edit_item (NewItem, SubItem);
			else
				m_pListCtrl->edit_item (Count - 1, SubItem);
			return;
		}
		case VK_PRIOR :
		{
			int NewItem = Item - m_pListCtrl->GetCountPerPage();
			if (NewItem > 0)
				m_pListCtrl->edit_item (NewItem, SubItem);
			else
				m_pListCtrl->edit_item (0, SubItem);
			return;
		}
    }
	*/
    
    CEdit::OnKeyUp (nChar, nRepCnt, nFlags);
}

void gxEditCell::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    bool Shift = GetKeyState (VK_SHIFT) < 0;
    switch (nChar)
    {
		//편집 취소
		case VK_ESCAPE :
		{
			if (nChar == VK_ESCAPE)
				bEscape = TRUE;
			if (::IsWindow(m_hWnd))
				DestroyWindow();
			return;
		}
		//편집 완료
		case VK_RETURN :
		{
			//return키를 치면 편집을 마치고 edit box를 종료시킨다.
			SetListText();
			if (::IsWindow(m_hWnd))
				DestroyWindow();
			//m_pListCtrl->EditSubItem (Item + 1, 0);
			return;
		}

		/*
		//탭을 이용하여 이전, 다음 항목의 편집을 이어가는 기능이지만
		//멀티선택 후 편집 결과를 선택된 모든 항목에 적용해주는 기능과 충돌된다.
		//해결전까지 주석처리다.
		//다음 또는 이전 항목 편집
		case VK_TAB :
		{
			if (Shift)
			{
				if (SubItem > 0)
					m_pListCtrl->edit_item (Item, SubItem - 1);
				else if (Item > 0)
					m_pListCtrl->edit_item (Item - 1, m_pListCtrl->get_column_count() - 1);
			}
			else
			{
				if (SubItem < m_pListCtrl->get_column_count() - 1)
					m_pListCtrl->edit_item (Item, SubItem + 1);
				else if (Item < m_pListCtrl->GetItemCount() - 1)
					m_pListCtrl->edit_item (Item + 1, 0);
				else
					m_pListCtrl->edit_item (0, 0);
			}
			return;
		}
		*/
    }

    CEdit::OnChar(nChar, nRepCnt, nFlags);

    // Resize edit control if needed

    // Get text extent
    CString Text;

    GetWindowText(Text);
    CWindowDC DC(this);
    CFont *pFont = GetParent()->GetFont();
    CFont *pFontDC = DC.SelectObject(pFont);
    CSize Size = DC.GetTextExtent(Text);
    DC.SelectObject (pFontDC);
    Size.cx += 5;			   	// add some extra buffer

    // Get client rect
    CRect Rect, ParentRect;
    GetClientRect(&Rect);
    GetParent()->GetClientRect(&ParentRect);

    // Transform rect to parent coordinates
    ClientToScreen(&Rect);
    GetParent()->ScreenToClient(&Rect);

    // Check whether control needs to be resized and whether there is space to grow
    if (Size.cx > Rect.Width())
    {
		if (Size.cx + Rect.left < ParentRect.right)
			Rect.right = Rect.left + Size.cx;
		else
			Rect.right = ParentRect.right;
		MoveWindow(&Rect);
    }
}

int gxEditCell::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CEdit::OnCreate (lpCreateStruct) == -1)
		return -1;

    // Set the proper font
    CFont* Font = GetParent()->GetFont();
    SetFont (Font);

    SetWindowText (InitText);
    SetFocus();

	//만약 해당 셀의 값이 파일명이라면 확장자를 뺀 부분만 선택되게 하자.
    SetSel (0, -1);
    return 0;
}

UINT 
gxEditCell::OnGetDlgCode() 
{
    return CEdit::OnGetDlgCode() | DLGC_WANTARROWS | DLGC_WANTTAB;
}

LRESULT gxEditCell::OnNcHitTest(CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	return CEdit::OnNcHitTest(point);
}


void gxEditCell::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CEdit::OnNcLButtonDown(nHitTest, point);
}


void gxEditCell::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CEdit::OnActivate(nState, pWndOther, bMinimized);

	// TODO: Add your message handler code here
	TRACE("OnActivate : pWndOther = %p\n", pWndOther);
}


void gxEditCell::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	CEdit::OnActivateApp(bActive, dwThreadID);

	// TODO: Add your message handler code here
}
