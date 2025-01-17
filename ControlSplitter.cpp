// ControlSplitter.cpp : �C���v�������e?�V���� �t?�C��
//

#include "ControlSplitter.h"
#include "MemoryDC.h"

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
	m_type = CS_NONE;	
	m_hCursor = NULL;
	m_rectMax.SetRectEmpty();
	
	m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_3DFACE));
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
// CControlSplitter ���b�Z?�W �n���h��

void CControlSplitter::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: ���̈ʒu�Ƀ��b�Z?�W �n���h���p�̃R?�h��ǉ����邩�܂��̓f�t�H���g�̏������Ăяo���Ă�������
	
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

		//parent�� resize�� ���� �� ���� �ٽ� ���Ž������ �Ѵ�.
		GetParent()->GetWindowRect(m_rectMax);
		GetParent()->ScreenToClient(m_rectMax);

		//parent�� resize�� ��� m_move_limit�� �� ������ ���� ��쵵 �����Ѵ�. ��������� �Ѵ�.
		//if (m_move_limit.left > m_rectMax.left)
		//	m_move_limit.left = m_rectMax.left;
		//if (m_move_limit.top > m_rectMax.top)
		//	m_move_limit.top = m_rectMax.top;

		//if (m_move_limit.right > m_rectMax.right)
		//	m_move_limit.right = m_rectMax.right;
		//if (m_move_limit.bottom > m_rectMax.bottom)
		//	m_move_limit.bottom = m_rectMax.bottom;
	}

	CButton::OnLButtonDown(nFlags, point);
}

void CControlSplitter::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: ���̈ʒu�Ƀ��b�Z?�W �n���h���p�̃R?�h��ǉ����邩�܂��̓f�t�H���g�̏������Ăяo���Ă�������
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

		TRACE(_T("diff = %d, %d, move = %d, %d, rect w = %d, h = %d\n"), sizeDiff.cx, sizeDiff.cy, sizeMove.cx, sizeMove.cy, rect.Width(), rect.Height());

		rect.OffsetRect(sizeMove);

		if(m_type == CS_HORZ)
		{							
			rect.OffsetRect(0, sizeDiff.cy);
		}
		else
		{
			rect.OffsetRect(sizeDiff.cx,0);
		}

		std::vector<CControlItem>::iterator it;

		GetParent()->ScreenToClient(&rect);

		//scpark.
		//splitter �̵� �� � ��Ʈ���� w, h�� min, max�� ����� �ʴ���, splitter�� ����� ��Ʈ���� ������ ħ���ϴ��� ���� ���� �����ϰ�
		//����ٸ� �ƿ� �Ʒ� �ڵ���� ������Ѽ��� �ȵȴ�.
		bool succeed = true;

		for (it = m_vtTopLeftControls.begin(); it < m_vtTopLeftControls.end(); it++)
		{
			CWnd* pCtrl = it->pWnd;
			UINT nFlag = it->flag;

			if (pCtrl == NULL)
				continue;

			CRect rectCtrl;
			pCtrl->GetWindowRect(&rectCtrl);

			if (m_type == CS_HORZ)
			{
				if (nFlag & SPF_BOTTOM)
				{
					rectCtrl.bottom += sizeMove.cy;
					rectCtrl.bottom += sizeDiff.cy;
				}
				if (!(nFlag & SPF_TOP))
				{
					rectCtrl.top += sizeMove.cy;
					rectCtrl.top += sizeDiff.cy;
				}

				if ((it->min_cx > 0 && rectCtrl.Width() < it->min_cx) ||
					(it->min_cy > 0 && rectCtrl.Height() < it->min_cy))
				{
					succeed = false;
					break;
				}
			}
			else
			{
				if ((nFlag & SPF_RIGHT))
				{
					rectCtrl.right += sizeMove.cx;
					rectCtrl.right += sizeDiff.cx;
				}
				if (0 == (nFlag & SPF_LEFT))
				{
					rectCtrl.left += sizeMove.cx;
					rectCtrl.left += sizeDiff.cx;
				}

				if ((it->min_cx > 0 && rectCtrl.Width() < it->min_cx) ||
					(it->min_cy > 0 && rectCtrl.Height() < it->min_cy))
				{
					succeed = false;
					break;
				}
			}
		}

		for (it = m_vtBottomRightControls.begin(); it < m_vtBottomRightControls.end(); it++)
		{
			CWnd* pCtrl = it->pWnd;
			UINT nFlag = it->flag;

			if (pCtrl == NULL)
				continue;

			CRect rectCtrl;
			pCtrl->GetWindowRect(&rectCtrl);

			if (m_type == CS_HORZ)
			{
				if (nFlag & SPF_TOP)
				{
					rectCtrl.top += sizeMove.cy;
					rectCtrl.top += sizeDiff.cy;
				}

				if (!(nFlag & SPF_BOTTOM))
				{
					rectCtrl.bottom += sizeMove.cy;
					rectCtrl.bottom += sizeDiff.cy;
				}

				if ((it->min_cx > 0 && rectCtrl.Width() < it->min_cx) ||
					(it->min_cy > 0 && rectCtrl.Height() < it->min_cy))
				{
					succeed = false;
					break;
				}
			}
			else
			{
				if (nFlag & SPF_LEFT)
				{
					rectCtrl.left += sizeMove.cx;
					rectCtrl.left += sizeDiff.cx;
				}

				if (!(nFlag & SPF_RIGHT))
				{
					rectCtrl.right += sizeMove.cx;
					rectCtrl.right += sizeDiff.cx;
				}

				if ((it->min_cx > 0 && rectCtrl.Width() < it->min_cx) ||
					(it->min_cy > 0 && rectCtrl.Height() < it->min_cy))
				{
					succeed = false;
					break;
				}
			}
		}


		//if((m_type == CS_HORZ) && ((m_rectMax.top >= rect.top) || (m_rectMax.bottom <= rect.bottom)))// || (rect.top < m_move_limit.top) || (rect.bottom > m_move_limit.bottom)))
		if ((m_type == CS_HORZ) && ((m_rectMax.top >= rect.top) || (m_rectMax.bottom <= rect.bottom)))
		{
			return;
		}
		else if((m_type == CS_VERT) && ((m_rectMax.left >= rect.left) || (m_rectMax.right <= rect.right)))
		{
			return;
		}

		//TRACE("Max(%d, %d, %d, %d)\n", m_rectMax.left, m_rectMax.top, m_rectMax.right, m_rectMax.bottom);
		//TRACE("%d, %d, %d, %d\n", rect.left, rect.top, rect.right, rect.bottom);

		if (!succeed)
			return;

		for (it = m_vtTopLeftControls.begin(); it < m_vtTopLeftControls.end(); it++)
		{
			CWnd* pCtrl = it->pWnd;
			UINT nFlag = it->flag;

			if (pCtrl == NULL)
				continue;

			CRect rectCtrl;
			pCtrl->GetWindowRect(&rectCtrl);

			if(m_type == CS_HORZ)
			{
				//TRACE(_T("before rectCtrl %d, %d, w = %d, h = %d\n"), rectCtrl.left, rectCtrl.top, rectCtrl.Width(), rectCtrl.Height());

				//if (rectCtrl.Height() < 50)
				//	return;
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

				//TRACE(_T("after  rectCtrl %d, %d, w = %d, h = %d\n"), rectCtrl.left, rectCtrl.top, rectCtrl.Width(), rectCtrl.Height());
			}
			else
			{
				//TRACE(_T("before rectCtrl %d, %d, w = %d, h = %d\n"), rectCtrl.left, rectCtrl.top, rectCtrl.Width(), rectCtrl.Height());
				//if (rectCtrl.Width() < 50)
				//	return;
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
				//TRACE(_T("after  rectCtrl %d, %d, w = %d, h = %d\n"), rectCtrl.left, rectCtrl.top, rectCtrl.Width(), rectCtrl.Height());
			}

			GetParent()->ScreenToClient(&rectCtrl);		
			pCtrl->MoveWindow(rectCtrl);
		}

		for (it = m_vtBottomRightControls.begin(); it < m_vtBottomRightControls.end(); it++)
		{
			CWnd* pCtrl = it->pWnd;
			UINT nFlag = it->flag;

			if (pCtrl == NULL)
				continue;

			CRect rectCtrl;
			pCtrl->GetWindowRect(&rectCtrl);

			if (m_type == CS_HORZ)
			{
				//TRACE(_T("before rectCtrl %d, %d, w = %d, h = %d\n"), rectCtrl.left, rectCtrl.top, rectCtrl.Width(), rectCtrl.Height());
				//if (rectCtrl.Height() < 50)
				//	return;
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
				//TRACE(_T("after  rectCtrl %d, %d, w = %d, h = %d\n"), rectCtrl.left, rectCtrl.top, rectCtrl.Width(), rectCtrl.Height());
			}
			else
			{
				//TRACE(_T("before rectCtrl %d, %d, w = %d, h = %d\n"), rectCtrl.left, rectCtrl.top, rectCtrl.Width(), rectCtrl.Height());
				//if (rectCtrl.Width() < 50)
				//	return;
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
				//TRACE(_T("after  rectCtrl %d, %d, w = %d, h = %d\n"), rectCtrl.left, rectCtrl.top, rectCtrl.Width(), rectCtrl.Height());
			}

			GetParent()->ScreenToClient(&rectCtrl);		
			pCtrl->MoveWindow(rectCtrl);
		}
		
		MoveWindow(rect);
		GetParent()->SendMessage(Message_CControlSplitter,(WPARAM)m_hWnd,(LPARAM)MAKELONG(point.x,point.y));
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
	if(m_type == CS_HORZ)
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

	//������ SetCursor�� return true�� ���� �Ʒ� �� ������ �츮��
	//splitter���� Ŀ���� ns�Ǵ� we�� �ٲ� ��
	//���̾�α��� �ٸ� ��ư �������� ns�Ǵ� we�� �ٲ�� ������ �߻��Ѵ�.
	//������ SetCursor�ϰ� ��������.
	//SetClassLong(m_hWnd,GCL_HCURSOR,(LONG)m_hCursor);		
		
	return CButton::OnSetCursor(pWnd, nHitTest, message);
}


void CControlSplitter::set_type(UINT nType, bool draw_dots, Gdiplus::Color cr_dots)
{
	m_type = nType;
	m_draw_dots = draw_dots;
	m_cr_dots = cr_dots;
}

//� ��Ʈ���� ���ø��Ϳ� �Բ� �������� �Ѵ�. ���� ��� ����Ѵ�.
void CControlSplitter::AddToBoth(CControlItem ctrl)
{
	AddToTopOrLeftCtrls(ctrl);
	AddToBottomOrRightCtrls(ctrl);
}

void CControlSplitter::AddToBoth(UINT id, int min_cx, int min_cy, UINT flag)
{
	AddToTopOrLeftCtrls(id, min_cx, min_cy, flag);
	AddToBottomOrRightCtrls(id, min_cx, min_cy, flag);
}

void CControlSplitter::AddToTopOrLeftCtrls(UINT id, int min_cx, int min_cy, UINT flag)
{
	m_vtTopLeftControls.push_back(CControlItem(GetParent()->GetDlgItem(id), min_cx, min_cy, flag));
}

void CControlSplitter::AddToTopOrLeftCtrls(CControlItem ctrl)
{
	ASSERT(m_type);
	m_vtTopLeftControls.push_back(ctrl);
	if(m_rectMax.IsRectEmpty())
	{
		GetParent()->GetWindowRect(m_rectMax);
		/*
		TRACE3("I-%d(%d,%d,",nCtrlId,m_rectMax.top,m_rectMax.left);
		TRACE2("%d,%d)\n",m_rectMax.bottom,m_rectMax.right);
		*/
	}
	CWnd * pCtrl = ctrl.pWnd;
	
	if(pCtrl != NULL)
	{
		CRect rect;
		pCtrl->GetWindowRect(&rect);
		//GetParent()->ClientToScreen(rect);
		if((m_type == CS_HORZ) &&(m_rectMax.top < rect.top))
		{
			m_rectMax.top = rect.top;
		}
		if((m_type == CS_VERT) && (m_rectMax.left < rect.left))
		{
			m_rectMax.left = rect.left;
		}
	}
	/*
	TRACE3("M-%d(%d,%d,",nCtrlId,m_rectMax.top,m_rectMax.left);
	TRACE2("%d,%d)\n",m_rectMax.bottom,m_rectMax.right);
	*/
}

void CControlSplitter::AddToBottomOrRightCtrls(UINT id, int min_cx, int min_cy, UINT flag)
{
	m_vtBottomRightControls.push_back(CControlItem(GetParent()->GetDlgItem(id), min_cx, min_cy, flag));
}

void CControlSplitter::AddToBottomOrRightCtrls(CControlItem ctrl)
{
	ASSERT(m_type);
	m_vtBottomRightControls.push_back(ctrl);
	if(m_rectMax.IsRectEmpty())
	{
		GetParent()->GetWindowRect(m_rectMax);
		GetParent()->ClientToScreen(m_rectMax);
		/*
		TRACE3("I-%d(%d,%d,",nCtrlId,m_rectMax.top,m_rectMax.left);
		TRACE2("%d,%d)\n",m_rectMax.bottom,m_rectMax.right);
		*/
	}
	CWnd* pCtrl = ctrl.pWnd;
	if(pCtrl != NULL)
	{
		CRect rect;
		pCtrl->GetWindowRect(&rect);
		GetParent()->ClientToScreen(rect);
		if((m_type == CS_HORZ) &&(m_rectMax.bottom > rect.bottom))
		{
			m_rectMax.bottom = rect.bottom;
		}
		if((m_type == CS_VERT) &&(m_rectMax.right > rect.right))
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
	CPaintDC dc1(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CButton::OnPaint() for painting messages
	CRect	rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);

	dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());
	//dc.FillSolidRect(rc, RGB(255, 0, 0));

	if (m_draw_dots)
		draw_dots(&dc, &rc, m_type == CS_HORZ, 6, 2, m_cr_dots);
}

void CControlSplitter::draw_dots(CDC* pDC, CRect const* rect, bool horz, int number, int size, Gdiplus::Color cr)
{
	if (horz)
	{
		int x = rect->CenterPoint().x - (size * (number + number - 1)) / 2;
		const int y = rect->CenterPoint().y - size / 2;
		// 
		for (; number-- > 0; )
		{
			pDC->FillSolidRect(x, y, size, size, cr.ToCOLORREF());
			x += 2 * size;
		}
	}
	else
	{
		const int x = rect->CenterPoint().x - size / 2 + 1;
		int y = rect->CenterPoint().y - (size * (number + number - 1)) / 2;
		// 
		for (; number-- > 0; )
		{
			pDC->FillSolidRect(x, y, size, size, cr.ToCOLORREF());
			y += 2 * size;
		}
	}
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
