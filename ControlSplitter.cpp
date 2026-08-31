// ControlSplitter.cpp : 귽깛긵깏긽깛긡?긘깈깛 긲?귽깑
//

#include "ControlSplitter.h"
#include "MemoryDC.h"
#include "Functions.h"		//20260831 by claude. move_windows_together.

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
// CControlSplitter 긽긞긜?긙 긪깛긤깋

//20260831 by claude. splitter 와 링크된 컨트롤을 한 번에 옮기고 그 자리에서 동기 갱신한다.
//실제 구현은 Functions.h 의 move_windows_together — 창 리사이즈(CResizeCtrl)에서도 같은 것이 필요해
//그쪽으로 옮겼다. 왜 셋(DeferWindowPos + SWP_NOCOPYBITS + RDW_UPDATENOW)을 함께 해야 하는지는 거기 주석 참조.
void CControlSplitter::apply_moves(std::vector<split_move>& moves)
{
	std::vector<sc_window_move> wm;
	wm.reserve(moves.size());
	for (auto& m : moves)
	{
		if (m.pWnd == NULL)
			continue;
		wm.push_back({ m.pWnd->GetSafeHwnd(), m.rect });
	}

	move_windows_together(GetSafeHwnd() ? GetParent()->GetSafeHwnd() : NULL, wm);
}

void CControlSplitter::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 궞궻댧뭫궸긽긞긜?긙 긪깛긤깋뾭궻긓?긤귩믁돿궥귡궔귏궫궼긢긲긅깑긣궻룉뿚귩뚁귂뢯궢궲궘궬궠궋
	
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

		//parent가 resize된 경우는 이 값을 다시 갱신시켜줘야 한다.
		GetParent()->GetWindowRect(m_rectMax);
		GetParent()->ScreenToClient(m_rectMax);

		//20260831 by claude. 드래그 동안은 창 리사이즈와 같은 상태다. 리스트/트리의 스크롤바 재계산
		//(SWP_FRAMECHANGED 유발)을 재워 마우스 이동당 비용을 줄인다. 연결은 app 몫 — 헤더 주석 참조.
		if (m_live_resize_hook)
			m_live_resize_hook(true);

		//parent가 resize된 경우 m_move_limit이 그 범위를 넘을 경우도 존재한다. 보정해줘야 한다.
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
	// TODO: 궞궻댧뭫궸긽긞긜?긙 긪깛긤깋뾭궻긓?긤귩믁돿궥귡궔귏궫궼긢긲긅깑긣궻룉뿚귩뚁귂뢯궢궲궘궬궠궋
	//TRACE0("OnLButtonUp");
	if(m_pOldDragCapture != NULL)
    {		
		ReleaseCapture();
        m_pOldDragCapture = NULL;
    }
	bool was_dragging = m_bDragging;
	m_bDragging = false;
	//GetParent()->Invalidate(false);
	//GetParent()->SendMessage(MSG_CONTROL_SPLITTER_MOVED,(WPARAM)m_hWnd,(LPARAM)MAKELONG(point.x,point.y));
	CButton::OnLButtonUp(nFlags, point);

	//20260831 by claude. 재워뒀던 컨트롤을 깨우고 최종 상태로 한 번 다시 그린다.
	//hook 해제만 하면 스크롤바가 드래그 시작 시점의 상태로 남으므로 갱신까지 해야 한다.
	if (was_dragging && m_live_resize_hook)
	{
		m_live_resize_hook(false);
		if (CWnd* pParent = GetParent())
			pParent->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
	}

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

		//TRACE(_T("diff = %d, %d, move = %d, %d, rect w = %d, h = %d\n"), sizeDiff.cx, sizeDiff.cy, sizeMove.cx, sizeMove.cy, rect.Width(), rect.Height());

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
		//splitter 이동 시 어떤 컨트롤의 w, h가 min, max를 벗어나지 않는지, splitter가 연결된 컨트롤의 영역을 침범하는지 등을 먼저 조사하고
		//벗어난다면 아예 아래 코드들을 진행시켜서는 안된다.
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

		//20260831 by claude. 예전엔 여기서 컨트롤마다 MoveWindow 를 따로 불렀다. 이제는 새 위치만 모아서
		//apply_moves 에 한 번에 넘긴다 — 개별 이동이 드래그 잔상의 원인이었다(apply_moves 주석 참조).
		std::vector<split_move> moves;
		moves.reserve(m_vtTopLeftControls.size() + m_vtBottomRightControls.size() + 1);

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
			moves.push_back({ pCtrl, rectCtrl });
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
			moves.push_back({ pCtrl, rectCtrl });
		}

		//splitter 자신도 같은 배치에 넣어야 컨트롤과 한 프레임에 움직인다. rect 는 이미 parent client 좌표.
		moves.push_back({ this, rect });

		apply_moves(moves);
		GetParent()->SendMessage(Message_CControlSplitter,(WPARAM)m_hWnd,(LPARAM)MAKELONG(point.x,point.y));
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

	//위에서 SetCursor와 return true를 막고 아래 한 문장을 살리면
	//splitter에서 커서를 ns또는 we로 바꾼 후
	//다이얼로그의 다른 버튼 위에서도 ns또는 we로 바뀌는 문제가 발생한다.
	//위에서 SetCursor하고 리턴하자.
	//SetClassLong(m_hWnd,GCL_HCURSOR,(LONG)m_hCursor);		
		
	return CButton::OnSetCursor(pWnd, nHitTest, message);
}


void CControlSplitter::set_type(UINT nType, bool draw_dots, Gdiplus::Color cr_dots)
{
	m_type = nType;
	m_draw_dots = draw_dots;
	m_cr_dots = cr_dots;
}

//어떤 컨트롤은 스플리터와 함께 움직여야 한다. 양쪽 모두 줘야한다.
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

void CControlSplitter::AddToBoth(CWnd* pWnd, int min_cx, int min_cy, UINT flag)
{
	AddToTopOrLeftCtrls(pWnd, min_cx, min_cy, flag);
	AddToBottomOrRightCtrls(pWnd, min_cx, min_cy, flag);
}

void CControlSplitter::AddToTopOrLeftCtrls(UINT id, int min_cx, int min_cy, UINT flag)
{
	m_vtTopLeftControls.push_back(CControlItem(GetParent()->GetDlgItem(id), min_cx, min_cy, flag));
}

// ID 없는 동적 컨트롤 등록용(예: 런타임 생성한 CSCThumbCtrl). CControlItem 경로로 보내 m_rectMax 보정까지 수행.
void CControlSplitter::AddToTopOrLeftCtrls(CWnd* pWnd, int min_cx, int min_cy, UINT flag)
{
	AddToTopOrLeftCtrls(CControlItem(pWnd, min_cx, min_cy, flag));
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

// ID 없는 동적 컨트롤 등록용. CControlItem 경로로 보내 m_rectMax 보정까지 수행(AddToTopOrLeftCtrls(CWnd*) 와 대칭).
void CControlSplitter::AddToBottomOrRightCtrls(CWnd* pWnd, int min_cx, int min_cy, UINT flag)
{
	AddToBottomOrRightCtrls(CControlItem(pWnd, min_cx, min_cy, flag));
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

int CControlSplitter::get_split_offset() const
{
	if (!::IsWindow(m_hWnd))
		return -1;
	CWnd* pParent = const_cast<CControlSplitter*>(this)->GetParent();
	if (!pParent)
		return -1;

	CRect rect;
	GetWindowRect(&rect);
	pParent->ScreenToClient(&rect);
	return (m_type == CS_HORZ) ? rect.top : rect.left;
}

bool CControlSplitter::set_split_offset(int new_offset)
{
	if (!::IsWindow(m_hWnd) || !GetParent() || m_type == CS_NONE)
		return false;

	CRect rect;
	GetWindowRect(&rect);
	GetParent()->ScreenToClient(&rect);

	int current = (m_type == CS_HORZ) ? rect.top : rect.left;
	int delta = new_offset - current;
	if (delta == 0)
		return true;

	return apply_split_delta(m_type == CS_VERT ? delta : 0,
	                         m_type == CS_HORZ ? delta : 0);
}

bool CControlSplitter::apply_split_delta(int dcx, int dcy)
{
	if (m_type == CS_NONE)
		return false;
	if (dcx == 0 && dcy == 0)
		return true;

	//OnMouseMove 와 동일 산식 — top_left vs bottom_right + flag 조합으로 ctrl rect 변형.
	auto compute = [&](const CControlItem& it, bool top_left, CRect& out) -> bool {
		out = CRect();
		it.pWnd->GetWindowRect(&out);
		if (m_type == CS_HORZ)
		{
			if (top_left)
			{
				if (it.flag & SPF_BOTTOM)   out.bottom += dcy;
				if (!(it.flag & SPF_TOP))   out.top    += dcy;
			}
			else
			{
				if (it.flag & SPF_TOP)      out.top    += dcy;
				if (!(it.flag & SPF_BOTTOM)) out.bottom += dcy;
			}
		}
		else
		{
			if (top_left)
			{
				if (it.flag & SPF_RIGHT)    out.right += dcx;
				if (!(it.flag & SPF_LEFT))  out.left  += dcx;
			}
			else
			{
				if (it.flag & SPF_LEFT)     out.left  += dcx;
				if (!(it.flag & SPF_RIGHT)) out.right += dcx;
			}
		}
		if ((it.min_cx > 0 && out.Width()  < it.min_cx) ||
		    (it.min_cy > 0 && out.Height() < it.min_cy))
			return false;
		return true;
	};

	struct Pending { CWnd* pCtrl; CRect rect; };
	std::vector<Pending> pending;

	for (const auto& it : m_vtTopLeftControls)
	{
		if (!it.pWnd) continue;
		CRect r;
		if (!compute(it, true, r)) return false;
		pending.push_back({ it.pWnd, r });
	}
	for (const auto& it : m_vtBottomRightControls)
	{
		if (!it.pWnd) continue;
		CRect r;
		if (!compute(it, false, r)) return false;
		pending.push_back({ it.pWnd, r });
	}

	//splitter 자체 위치 + parent 경계 체크.
	CRect rectSplit;
	GetWindowRect(&rectSplit);
	rectSplit.OffsetRect(dcx, dcy);
	CRect rectSplitClient = rectSplit;
	GetParent()->ScreenToClient(&rectSplitClient);

	GetParent()->GetWindowRect(m_rectMax);
	GetParent()->ScreenToClient(m_rectMax);
	if (m_type == CS_HORZ)
	{
		if (rectSplitClient.top < m_rectMax.top || rectSplitClient.bottom > m_rectMax.bottom)
			return false;
	}
	else
	{
		if (rectSplitClient.left < m_rectMax.left || rectSplitClient.right > m_rectMax.right)
			return false;
	}

	//20260831 by claude. 드래그 경로(OnMouseMove)와 같은 배치 이동을 쓴다. 개별 MoveWindow 는
	//SetWindowPos 의 비트 복사 때문에 위치 의존 콘텐츠에 잘못된 픽셀을 남긴다(apply_moves 주석 참조).
	std::vector<split_move> moves;
	moves.reserve(pending.size() + 1);

	for (auto& m : pending)
	{
		CRect rc = m.rect;
		GetParent()->ScreenToClient(&rc);
		moves.push_back({ m.pCtrl, rc });
	}
	moves.push_back({ this, rectSplitClient });

	apply_moves(moves);
	GetParent()->SendMessage(Message_CControlSplitter, (WPARAM)m_hWnd, 0);
	return true;
}
