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

gxEditCell::gxEditCell (CListCtrlEx* pCtrl, int iItem, int iSubItem, CString sInitText)
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
	//�� �Լ��� �ι� �̻� ȣ��ȴ�.
	//�� ��°�� �̹� �� EditCell�� ����Ǵ� �������� ȣ��Ǿ�
	//������ �ڵ��� ��ȿ���� �ʴ�.
	if ( !IsWindow( m_hWnd ) )
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
			true )
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

	//�Ʒ� ���� SetListText();�� ȣ���ϸ� �����߿� �ٸ� ���� Ŭ���Ͽ� ��Ŀ���� �Ҿ�����
	//���� �Ϸ�� �� ������ ���� ��ҷ� �� ������ �� ������ ������ �� �ִ�.
	//SetListText();�� ȣ�����ָ� ��Ŀ���� �Ҿ��� �� ���� �Ϸ�� ó���ȴ�.(����ó��)
	//��, �Ʒ� TRACE���� Ȯ���� �� ó�� ������ ���۵� ��Ʈ�Ѱ� ���� ��Ŀ���� ������ �Ǵ� ��Ʈ���� �ٸ� ��쿡��
	//editbox�� ���� ���� ��Ŀ���� ���� ��Ʈ���� ������ �Ҵ�Ǵ� ������ �����Ƿ�
	//�� ��Ʈ���� ���� ��쿡�� �ڵ� �Է� �Ϸ�ǵ��� �ؾ� �Ѵ�.
	//TRACE("m_pListCtrl = %p, pNewWnd = %p\n", m_pListCtrl, pNewWnd);

	//if (m_pListCtrl == pNewWnd)
	{
		SetListText();
		m_pListCtrl->SetFlagInEditing(false);
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

void 
gxEditCell::OnNcDestroy() 
{
    CEdit::OnNcDestroy();
    
    delete this;
}

void gxEditCell::OnKeyDown (UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
	return;
    // Up and down are in the OnKeyDown so that the user can hold down the arrow
    // keys to scroll through the entries.
    bool Control = GetKeyState(VK_CONTROL) < 0;
    switch (nChar)
    {
		case VK_UP :
		{
			SetListText();
			//if (Item > 0)
				//m_pListCtrl->EditSubItem (Item - 1, SubItem);
			return;
		}
		case VK_DOWN :
		{
			SetListText();
			//if ( Item < m_pListCtrl->GetItemCount() - 1 )
				//m_pListCtrl->EditSubItem (Item + 1, SubItem);
			return;
		}
		case VK_LEFT :
		{
			SetListText();
			//if ( SubItem > 0 )
				//m_pListCtrl->EditSubItem( Item, SubItem - 1 );
			//else if ( Item > 0 )
				//m_pListCtrl->EditSubItem( Item - 1, m_pListCtrl->GetColumnCount() - 1 );
			return;
		}
		case VK_RIGHT :
		{
			SetListText();
			if ( SubItem < m_pListCtrl->GetColumnCount() - 1 )
				m_pListCtrl->EditSubItem( Item, SubItem + 1 );
			else if ( Item < m_pListCtrl->GetItemCount() - 1 )
				m_pListCtrl->EditSubItem( Item + 1, 0 );
			return;
		}
		case VK_HOME :
		{
			if (!Control)
				break;

			m_pListCtrl->EditSubItem (0, SubItem);
			return;
		}
		case VK_END :
		{
			if (!Control)
				break;

			int Count = m_pListCtrl->GetItemCount() - 1;
			m_pListCtrl->EditSubItem (Count, SubItem);
			return;
		}
    }
    
    CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void 
gxEditCell::OnKeyUp (UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    switch (nChar)
    {
		case VK_NEXT :
		{
			int Count = m_pListCtrl->GetItemCount();
			int NewItem = Item + m_pListCtrl->GetCountPerPage();
			if (Count > NewItem)
				m_pListCtrl->EditSubItem (NewItem, SubItem);
			else
				m_pListCtrl->EditSubItem (Count - 1, SubItem);
			return;
		}
		case VK_PRIOR :
		{
			int NewItem = Item - m_pListCtrl->GetCountPerPage();
			if (NewItem > 0)
				m_pListCtrl->EditSubItem (NewItem, SubItem);
			else
				m_pListCtrl->EditSubItem (0, SubItem);
			return;
		}
    }
    
    CEdit::OnKeyUp (nChar, nRepCnt, nFlags);
}

void 
gxEditCell::OnChar (UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    bool Shift = GetKeyState (VK_SHIFT) < 0;
    switch (nChar)
    {
		//���� ���
		case VK_ESCAPE :
		{
			if (nChar == VK_ESCAPE)
				bEscape = TRUE;
			if ( ::IsWindow( m_hWnd ) )
				DestroyWindow();
			return;
		}
		//���� �Ϸ�
		case VK_RETURN :
		{
			//returnŰ�� ġ�� ������ ��ġ�� edit box�� �����Ų��.
			SetListText();
			if ( ::IsWindow( m_hWnd ) )
				DestroyWindow();
			//m_pListCtrl->EditSubItem (Item + 1, 0);
			return;
		}
		//���� �Ǵ� ���� �׸� ����
		case VK_TAB :
		{
			if ( Shift )
			{
				if (SubItem > 0)
					m_pListCtrl->EditSubItem (Item, SubItem - 1);
				else if (Item > 0)
					m_pListCtrl->EditSubItem (Item - 1, m_pListCtrl->GetColumnCount() - 1 );
			}
			else
			{
				if ( SubItem < m_pListCtrl->GetColumnCount() - 1 )
					m_pListCtrl->EditSubItem (Item, SubItem + 1);
				else if ( Item < m_pListCtrl->GetItemCount() - 1 )
					m_pListCtrl->EditSubItem (Item + 1, 0);
				else
					m_pListCtrl->EditSubItem (0, 0);
			}
			return;
		}
    }

    CEdit::OnChar (nChar, nRepCnt, nFlags);

    // Resize edit control if needed

    // Get text extent
    CString Text;

    GetWindowText (Text);
    CWindowDC DC (this);
    CFont *pFont = GetParent()->GetFont();
    CFont *pFontDC = DC.SelectObject (pFont);
    CSize Size = DC.GetTextExtent (Text);
    DC.SelectObject (pFontDC);
    Size.cx += 5;			   	// add some extra buffer

    // Get client rect
    CRect Rect, ParentRect;
    GetClientRect (&Rect);
    GetParent()->GetClientRect (&ParentRect);

    // Transform rect to parent coordinates
    ClientToScreen (&Rect);
    GetParent()->ScreenToClient (&Rect);

    // Check whether control needs to be resized and whether there is space to grow
    if (Size.cx > Rect.Width())
    {
		if (Size.cx + Rect.left < ParentRect.right )
			Rect.right = Rect.left + Size.cx;
		else
			Rect.right = ParentRect.right;
		MoveWindow (&Rect);
    }
}

int 
gxEditCell::OnCreate (LPCREATESTRUCT lpCreateStruct) 
{
    if (CEdit::OnCreate (lpCreateStruct) == -1)
		return -1;

    // Set the proper font
    CFont* Font = GetParent()->GetFont();
    SetFont (Font);

    SetWindowText (InitText);
    SetFocus();
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	return CEdit::OnNcHitTest(point);
}


void gxEditCell::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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
