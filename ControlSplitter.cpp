// ControlSplitter.cpp : インプリメンテ?ション フ?イル
//

#include "ControlSplitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CControlSplitter

CControlSplitter::CControlSplitter()
{
	m_pOldDragCapture = NULL;
    m_bDragging = false;
	m_nType = CS_NONE;	
	m_hCursor = NULL;
	m_rectMax.SetRectEmpty();
	
	m_crBack = ::GetSysColor(COLOR_WINDOW);//::GetSysColor(COLOR_3DFACE);//RGB(255, 249, 228);
}

CControlSplitter::~CControlSplitter()
{
	DestroyCursor(m_hCursor);
}


BEGIN_MESSAGE_MAP(CControlSplitter, CButton)
	//{{AFX_MSG_MAP(CControlSplitter)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CControlSplitter メッセ?ジ ハンドラ

void CControlSplitter::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: この位置にメッセ?ジ ハンドラ用のコ?ドを追加するかまたはデフォルトの処理を呼び出してください
	
	//if(DragDetect(m_hWnd,point))
	{        
        m_bDragging = true;
        if(m_pOldDragCapture != NULL)
		{
			ReleaseCapture();
			m_pOldDragCapture = NULL;
		}
		//TRACE0("Drag");
		m_pOldDragCapture = SetCapture();        
		GetCursorPos(&m_ptStartDrag);
		CRect rect;
		GetWindowRect(&rect);
		m_ptStartPos = rect.TopLeft();

		//parentｰ｡ resizeｵﾈ ｰ豼�ｴﾂ ﾀﾌ ｰｪﾀｻ ｴﾙｽﾃ ｰｻｽﾅｽﾃﾄﾑﾁ狎ﾟ ﾇﾑｴﾙ.
		GetParent()->GetWindowRect(m_rectMax);
		GetParent()->ScreenToClient(m_rectMax);
	}
	CButton::OnLButtonDown(nFlags, point);
}

void CControlSplitter::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: この位置にメッセ?ジ ハンドラ用のコ?ドを追加するかまたはデフォルトの処理を呼び出してください
	//TRACE0("OnLButtonUp");
	if(m_pOldDragCapture != NULL)
    {		
		ReleaseCapture();
        m_pOldDragCapture = NULL;
    }
	m_bDragging = false;
	//GetParent()->Invalidate(false);
	//GetParent()->SendMessage(MSG_CONTROL_SPLITTER_MOVED,(WPARAM)m_hWnd,(LPARAM)MAKELONG(point.x,point.y));
	CButton::OnLButtonUp(nFlags, point);

	Invalidate(false);
}

void CControlSplitter::OnMouseMove(UINT nFlags, CPoint point) 
{		
	if(m_bDragging)
	{
		CRect rect;
		CPoint ptMouse;

		::GetWindowRect(m_hWnd,&rect);
		GetCursorPos(&ptMouse);

		CSize sizeDiff = ptMouse - m_ptStartDrag;
		CSize sizeMove = m_ptStartPos-rect.TopLeft();

		rect.OffsetRect(sizeMove);

		if(m_nType == CS_HORZ)
		{							
			rect.OffsetRect(0, sizeDiff.cy);
		}
		else
		{
			rect.OffsetRect(sizeDiff.cx,0);
		}

		std::vector<DWORD>::iterator it;

		GetParent()->ScreenToClient(&rect);

		if((m_nType == CS_HORZ) && ((m_rectMax.top >= rect.top) || (m_rectMax.bottom <= rect.bottom)))
		{
			return;
		}
		else if((m_nType == CS_VERT) && ((m_rectMax.left >= rect.left) || (m_rectMax.right <= rect.right)))
		{
			return;
		}

		//TRACE("Max(%d, %d, %d, %d)\n", m_rectMax.left, m_rectMax.top, m_rectMax.right, m_rectMax.bottom);
		//TRACE("Cur(%d, %d, %d, %d)\n", rect.left, rect.top, rect.right, rect.bottom);

		for (it = m_vtTopLeftControls.begin(); it < m_vtTopLeftControls.end(); it++)
		{
			CWnd* pCtrl = GetParent()->GetDlgItem(LOWORD(*it));
			UINT nFlag = HIWORD(*it);

			if (pCtrl == NULL)
				continue;

			CRect rectCtrl;
			pCtrl->GetWindowRect(&rectCtrl);

			if(m_nType == CS_HORZ)
			{
				if (nFlag & SPF_BOTTOM)
				{
					rectCtrl.bottom +=  sizeMove.cy;
					rectCtrl.bottom += sizeDiff.cy;
				}
				if (!(nFlag & SPF_TOP))
				{
					rectCtrl.top +=  sizeMove.cy;
					rectCtrl.top += sizeDiff.cy;
				}
			}
			else
			{
				if ((nFlag & SPF_RIGHT))
				{
					rectCtrl.right +=  sizeMove.cx;
					rectCtrl.right += sizeDiff.cx;
				}
				if (0 == (nFlag & SPF_LEFT))
				{
					rectCtrl.left +=  sizeMove.cx;
					rectCtrl.left += sizeDiff.cx;
				}
			}			
			GetParent()->ScreenToClient(&rectCtrl);		
			pCtrl->MoveWindow(rectCtrl);
		}

		for (it = m_vtBottomRightControls.begin(); it < m_vtBottomRightControls.end(); it++)
		{
			CWnd* pCtrl = GetParent()->GetDlgItem(LOWORD(*it));
			UINT nFlag = HIWORD(*it);

			if (pCtrl == NULL)
				continue;

			CRect rectCtrl;
			pCtrl->GetWindowRect(&rectCtrl);

			if (m_nType == CS_HORZ)
			{
				if (nFlag & SPF_TOP)
				{
					rectCtrl.top +=  sizeMove.cy;
					rectCtrl.top += sizeDiff.cy;
				}

				if(!(nFlag & SPF_BOTTOM))
				{
					rectCtrl.bottom +=  sizeMove.cy;
					rectCtrl.bottom += sizeDiff.cy;
				}
			}
			else
			{
				if (nFlag & SPF_LEFT)
				{
					rectCtrl.left +=  sizeMove.cx;
					rectCtrl.left += sizeDiff.cx;
				}

				if(!(nFlag & SPF_RIGHT))
				{
					rectCtrl.right +=  sizeMove.cx;
					rectCtrl.right += sizeDiff.cx;
				}
			}

			GetParent()->ScreenToClient(&rectCtrl);		
			pCtrl->MoveWindow(rectCtrl);
		}
		
		MoveWindow(rect);
		GetParent()->SendMessage(MSG_CONTROL_SPLITTER_MOVED,(WPARAM)m_hWnd,(LPARAM)MAKELONG(point.x,point.y));
		Invalidate();
	}
	
	CButton::OnMouseMove(nFlags, point);
}

BOOL CControlSplitter::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{	
	if(m_hCursor != NULL)
	{
		DestroyCursor(m_hCursor);
	}
	if(m_nType == CS_HORZ)
	{
		m_hCursor = ::LoadCursor(NULL,IDC_SIZENS);
		::SetCursor( m_hCursor );
		return true;
	}else
	{
		m_hCursor = ::LoadCursor(NULL,IDC_SIZEWE);
		::SetCursor( m_hCursor );
		return true;
	}

	//ﾀｧｿ｡ｼｭ SetCursorｿﾍ return trueｸｦ ｸｷｰ� ｾﾆｷ｡ ﾇﾑ ｹｮﾀ蠡ｻ ｻ�ｸｮｸ�
	//splitterｿ｡ｼｭ ﾄｿｼｭｸｦ nsｶﾇｴﾂ weｷﾎ ｹﾙｲﾛ ﾈﾄ
	//ｴﾙﾀﾌｾ�ｷﾎｱﾗﾀﾇ ｴﾙｸ･ ｹ�ﾆｰ ﾀｧｿ｡ｼｭｵｵ nsｶﾇｴﾂ weｷﾎ ｹﾙｲ�ｴﾂ ｹｮﾁｦｰ｡ ｹﾟｻ�ﾇﾑｴﾙ.
	//ﾀｧｿ｡ｼｭ SetCursorﾇﾏｰ� ｸｮﾅﾏﾇﾏﾀﾚ.
	//SetClassLong(m_hWnd,GCL_HCURSOR,(LONG)m_hCursor);		
		
	return CButton::OnSetCursor(pWnd, nHitTest, message);
}


void CControlSplitter::SetType(UINT nType)
{
	m_nType = nType;
}

//ｾ�ｶｲ ﾄﾁﾆｮｷﾑﾀｺ ｽｺﾇﾃｸｮﾅﾍｿﾍ ﾇﾔｲｲ ｿ�ﾁ�ｿｩｾﾟ ﾇﾑｴﾙ. ｾ酊ﾊ ｸ�ｵﾎ ﾁ狎ﾟﾇﾑｴﾙ.
void CControlSplitter::AddToBoth(UINT nCtrlId, WORD nFlags)
{
	AddToTopOrLeftCtrls(nCtrlId, nFlags);
	AddToBottomOrRightCtrls(nCtrlId, nFlags);
}

void CControlSplitter::AddToTopOrLeftCtrls(UINT nCtrlId, WORD nFlags)
{
	ASSERT(m_nType);
	m_vtTopLeftControls.push_back(MAKELONG(nCtrlId,nFlags));
	if(m_rectMax.IsRectEmpty())
	{
		GetParent()->GetWindowRect(m_rectMax);
		/*
		TRACE3("I-%d(%d,%d,",nCtrlId,m_rectMax.top,m_rectMax.left);
		TRACE2("%d,%d)\n",m_rectMax.bottom,m_rectMax.right);
		*/
	}
	CWnd * pCtrl = GetParent()->GetDlgItem(nCtrlId);
	
	if(pCtrl != NULL)
	{
		CRect rect;
		pCtrl->GetWindowRect(&rect);
		//GetParent()->ClientToScreen(rect);
		if((m_nType == CS_HORZ) &&(m_rectMax.top < rect.top))
		{
			m_rectMax.top = rect.top;
		}
		if((m_nType == CS_VERT) && (m_rectMax.left < rect.left))
		{
			m_rectMax.left = rect.left;
		}
	}
	/*
	TRACE3("M-%d(%d,%d,",nCtrlId,m_rectMax.top,m_rectMax.left);
	TRACE2("%d,%d)\n",m_rectMax.bottom,m_rectMax.right);
	*/
}

void CControlSplitter::AddToBottomOrRightCtrls(UINT nCtrlId, WORD nFlags)
{
	ASSERT(m_nType);
	m_vtBottomRightControls.push_back(MAKELONG(nCtrlId,nFlags));
	if(m_rectMax.IsRectEmpty())
	{
		GetParent()->GetWindowRect(m_rectMax);
		GetParent()->ClientToScreen(m_rectMax);
		/*
		TRACE3("I-%d(%d,%d,",nCtrlId,m_rectMax.top,m_rectMax.left);
		TRACE2("%d,%d)\n",m_rectMax.bottom,m_rectMax.right);
		*/
	}
	CWnd * pCtrl = GetParent()->GetDlgItem(nCtrlId);
	if(pCtrl != NULL)
	{
		CRect rect;
		pCtrl->GetWindowRect(&rect);
		GetParent()->ClientToScreen(rect);
		if((m_nType == CS_HORZ) &&(m_rectMax.bottom > rect.bottom))
		{
			m_rectMax.bottom = rect.bottom;
		}
		if((m_nType == CS_VERT) &&(m_rectMax.right > rect.right))
		{
			m_rectMax.right = rect.right;
		}
	}
	/*
	TRACE3("M-%d(%d,%d,",nCtrlId,m_rectMax.top,m_rectMax.left);
	TRACE2("%d,%d)\n",m_rectMax.bottom,m_rectMax.right);
	*/
}


void CControlSplitter::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CButton::OnPaint() for painting messages
	CRect	rc;
	GetClientRect( rc );
	dc.FillSolidRect( rc, m_crBack );
}


BOOL CControlSplitter::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
	return CButton::OnEraseBkgnd(pDC);
}


BOOL CControlSplitter::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP)
		return false;

	return CButton::PreTranslateMessage(pMsg);
}
