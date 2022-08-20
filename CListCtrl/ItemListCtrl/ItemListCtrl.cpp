// PhotoListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "Item.h"
#include "ItemListCtrl.h"
//#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CItemListCtrl::CItemListCtrl()
{
	CWinApp * pApp = AfxGetApp();

	m_hCursorHand	=	pApp->LoadStandardCursor(IDC_HAND);
	m_hCursorArrow	=	pApp->LoadStandardCursor(IDC_ARROW);

	m_ptStart = m_ptEnd = CPoint(-1,-1);

	m_pSelectedItem = NULL;

	m_nViewHeight = 0;
	m_nVPageSize = 0;
	m_nVScrollPos = 0;

	m_nLButtonDown = -1;
	m_nLButtonDblClk = -1;

//	m_brBackground.CreateSolidBrush(RGB(255,247,231));
	m_brBackground.CreateSolidBrush(RGB(198,198,198));
}

CItemListCtrl::~CItemListCtrl()
{
	::DeleteObject( m_hCursorHand );
	::DeleteObject( m_hCursorArrow );

	m_brBackground.DeleteObject();

	DeleteAllItems();
}


BEGIN_MESSAGE_MAP(CItemListCtrl, CWnd )
	//{{AFX_MSG_MAP(CItemListCtrl)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CItemListCtrl message handlers
//---------------------------------------------------------------------------
// 함수설명 : Photo ListCtrl을 위한 정보를 초기화한다.
//---------------------------------------------------------------------------
int CItemListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//---------------------------------------------------------
	// 실제 응용프로그램의 클라이언트 영역 사이즈를 얻어온다.
	//---------------------------------------------------------
	CRect rcClient;
	GetClientRect( rcClient );

	//---------------------------------------------------------
	// 스크롤링을 위한 변수들을 셋팅한다.
	//---------------------------------------------------------
	InitVirtualSpace(rcClient);
	
	return 0;
}

//---------------------------------------------------------------------------
// 함수설명 : 포토컨트롤을 초기화한다.
//---------------------------------------------------------------------------
void CItemListCtrl::InitItemList()
{
	m_ptStart = m_ptEnd = CPoint(-1,-1);

	m_pSelectedItem = NULL;

	m_nViewHeight = 0;

	m_nVPageSize = 0;
	m_nVScrollPos = 0;

	DeleteAllItems();
}

//---------------------------------------------------------------------------
// 함수설명 : 스크롤링을 위한 변수들을 셋팅한다.
//---------------------------------------------------------------------------
void CItemListCtrl::InitVirtualSpace(CRect rcClient)
{
	//---------------------------------------------------------
	// 수직방향의 Virtual Space 를 계산한다.
	//---------------------------------------------------------
	m_nViewHeight = CalcVerticalSpace(rcClient);

	//---------------------------------------------------------
	// 수직 스크롤을 위한 파라미터값을 지정한다.
	//---------------------------------------------------------
	m_nVScrollPos = m_nVPageSize = 0;

	if(	rcClient.Height() < m_nViewHeight )
	{
		m_nVPageSize = rcClient.Height();
		m_nVScrollPos = min( m_nVScrollPos , m_nViewHeight - m_nVPageSize - 1 );
	}
}

//---------------------------------------------------------------------------
// 함수설명 : 스크롤 사이즈를 조정한다.
//---------------------------------------------------------------------------
void CItemListCtrl::InitScrollSize()
{
	CRect rcClient;
	GetClientRect(&rcClient);

	//----------------------------------------------
	// 스크롤링을 위한 변수들을 셋팅한다.
	//----------------------------------------------
	InitVirtualSpace(rcClient);

	//----------------------------------------------
	// 수직 스크롤을 위한 파라미터값을 지정한다.
	//----------------------------------------------
	int nVScrollMax = 0;
	
	if( rcClient.Height() < m_nViewHeight )
	{
		nVScrollMax = m_nViewHeight - 1;
	}

	SCROLLINFO si;	

	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_TRACKPOS;
	si.nMin = 0;
	si.nMax = nVScrollMax;
	si.nPos = m_nVScrollPos;
	si.nPage = m_nVPageSize;

	SetScrollInfo( SB_VERT , &si , TRUE );
}

//---------------------------------------------------------------------------
// 함수설명 : 아이템들의 위치를 변경한다.
//---------------------------------------------------------------------------
void CItemListCtrl::InitItemPos()
{
	int nIndex = 0;

	//-----------------------------------------------------------------------
	// 기존에 있던 아이템들의 영역을 변경한다.
	//-----------------------------------------------------------------------
	POSITION pos = m_listItem.GetHeadPosition();

	while( pos != NULL )
	{
		CItem * pItem = (CItem *)m_listItem.GetNext( pos );	
		pItem->SetRect( CalcItemRect( nIndex++ ) );
	}
}

//---------------------------------------------------------------------------
// 함수설명 : Vertical Scroll 처리를 직접 처리해 주는 것 
//---------------------------------------------------------------------------
void CItemListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{			
	int nDelta = 0;

	switch (nSBCode) 
	{			
	case SB_PAGEUP:					
		{
			nDelta = -m_nVPageSize;
		}
		break;
				
	case SB_PAGEDOWN:
		{
			nDelta = m_nVPageSize;
		}
		break;
				
	case SB_LINEUP:
		{
			nDelta = -( GetItemHeight() + GetHGap() );
		}
		break;
				
	case SB_LINEDOWN:
		{
			nDelta = ( GetItemHeight() + GetHGap() );
		}
		break;

	case SB_THUMBTRACK:
		{
			if( int(nPos) > m_nVScrollPos )
				nDelta = abs( (int)(nPos - m_nVScrollPos) );
			else if( int(nPos) < m_nVScrollPos )
				nDelta = -abs( (int)(nPos - m_nVScrollPos) );
		}
		break;
		
	default:
		{
			return;
		}
		break;
	}

	//새로운 스크롤 위치를 계산한다.
	int nScrollPos = m_nVScrollPos + nDelta;
	int nMaxPos = m_nViewHeight - m_nVPageSize;

	if( nScrollPos < 0 )
	{
		nDelta = -m_nVScrollPos;
	}
	else if( nScrollPos > nMaxPos )
	{
		nDelta = nMaxPos - m_nVScrollPos;
	}

	if( nDelta != 0 )
	{
		m_nVScrollPos += nDelta;

		SetScrollPos( SB_VERT , m_nVScrollPos , TRUE );

		ScrollWindow( 0 , -nDelta );
		UpdateWindow();
	}
}

//---------------------------------------------------------------------------
// 함수설명 : 현재 선택된 아이템의 포인터를 반환한다.
//---------------------------------------------------------------------------
CItem * CItemListCtrl::GetSelectedItem()
{
	return m_pSelectedItem;
}

//---------------------------------------------------------------------------
// 함수설명 : 선택된 아이템을 알아온다.
//---------------------------------------------------------------------------
// 변수설명 : point : 마우스가 클릭하거나 놓인 위치를 나타낸다.
//---------------------------------------------------------------------------
// 부가설명 : point 는 논리좌표계의 포인트여야 한다.
//---------------------------------------------------------------------------
// 반환값   : 선택된 아이템이 있으면 객체 포인터를 반환하고 없으면 NULL 반환
//---------------------------------------------------------------------------
CItem * CItemListCtrl::GetClickItem( CPoint point )
{	
	POSITION pos = m_listItem.GetHeadPosition();

	while( pos != NULL )
	{
		CItem * pItem = (CItem *)m_listItem.GetNext(pos);

		CRect rcItem = pItem->GetRect();

		if( rcItem.PtInRect( point ) )
		{
			return pItem;
		}
	}

	return NULL;
}


//---------------------------------------------------------------------------
// 함수설명 : 아이템 위에 가면 아이템의 모양을 변경한다.
//---------------------------------------------------------------------------
BOOL CItemListCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{	
	CWinApp * pApp = AfxGetApp();

	CPoint point;

	//-----------------------------------------------------------------------
	// 현재 커서의 위치를 알아온다.
	//-----------------------------------------------------------------------
	if( !GetCursorPos( &point ) )
	{
		return CWnd::OnSetCursor(pWnd, nHitTest, message);
	}

	//-----------------------------------------------------------------------
	// 클라이언트 영역을 기준으로 한 좌표로 변경한다.
	//-----------------------------------------------------------------------
	ScreenToClient( &point );

	CClientDC dc(this);
	//-----------------------------------------------------------------------
	// 생성된 CDC 의 뷰 포인트를 변경한다.
	//-----------------------------------------------------------------------
	dc.SetWindowOrg( 0 , m_nVScrollPos );

	//-----------------------------------------------------------------------
	// Device 좌표계에서 Logical 좌표계의 좌표로 변경한다.
	//-----------------------------------------------------------------------
	dc.DPtoLP( &point );

	//-----------------------------------------------------------------------
	// 아이템위에 있는지 확인한다.
	//-----------------------------------------------------------------------
	CItem * pItem = GetClickItem( point );

	if( pItem != NULL )
	{	
		if( GetCursor() != m_hCursorHand )
		{
			SetCursor(m_hCursorHand);
		}
	}
	else
	{
		if( GetCursor() != m_hCursorArrow )
		{
			SetCursor(m_hCursorArrow);
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
// 함수설명 : 크기가 변경되면 포토 컨트롤을 다시 그리도록 한다.
//---------------------------------------------------------------------------
void CItemListCtrl::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

	//---------------------------------------------------------
	// 포토컨트롤을 다시 그리도록 한다.
	//---------------------------------------------------------
	ReDraw();
}

//---------------------------------------------------------------------------
// 함수설명 : 아이템을 메모리 DC 를 사용하여 화면에 출력한다.
//---------------------------------------------------------------------------
void CItemListCtrl::OnPaint() 
{
	CPaintDC dc(this);

	Draw(&dc, dc.m_ps.rcPaint);
}

//---------------------------------------------------------------------------
// 함수설명 : 아이템 리스트컨트롤을 그린다. (뷰잉영역에 속한 아이템도 출력함)
//---------------------------------------------------------------------------
// 부가설명 : pDC 는 원점을 스크롤링에 맞게 옮겨진다.
//---------------------------------------------------------------------------
void CItemListCtrl::Draw(CDC * pDC, CRect rcInvalid)
{
	//-----------------------------------------------------------------------
	// 원점을 옮긴다.
	//-----------------------------------------------------------------------
	pDC->SetWindowOrg( 0 , m_nVScrollPos );

	CRect rect;
	GetClientRect(&rect);   

	//-----------------------------------------------------------------------
	// Invalidate 된 영역의 좌표를 논리 좌표계로 변화하기 위해서 얻어온다.
	//-----------------------------------------------------------------------	
	pDC->DPtoLP( &rcInvalid );

	//-----------------------------------------------------------------------
	// 화면의 깜빡임을 줄이기 위해서 메모리 dc를 생성한다.
	//-----------------------------------------------------------------------
	CDC MemDC;
	MemDC.CreateCompatibleDC( pDC );

	CBitmap MemBitmap;
	MemBitmap.CreateCompatibleBitmap( pDC , rect.Width() , rect.Height() );

	CBitmap * pOldBitmap = (CBitmap *)MemDC.SelectObject( &MemBitmap );

	MemDC.FillRect( &rect , &m_brBackground );

	MemDC.SetWindowOrg( 0 , m_nVScrollPos );	

	//-------------------------------------------------------------------
	// 현재화면에 가로 세로 각각의 출력할 수 있는 아이템의 갯수를 얻어온다.
	//-------------------------------------------------------------------
	POSITION pos = m_listItem.GetHeadPosition();
	
	while( pos != NULL )
	{
		//----------------------------------------------------
		// Invalidate 된 영역에 포함되는 아이템만 다시 그리기 
		// 위해서 아이템이 Invalidate된 영역에 포함되는지 검사한다.
		//----------------------------------------------------				
		CItem * pItem = (CItem *)m_listItem.GetNext( pos );
		CRect rcItem = pItem->GetRect();

		if( !rcItem.IntersectRect( rcItem , rcInvalid ) )
		{
			//서로 겹치는 부분이 없다는 것을 나타낸다.
			//즉 현재 아이템은 화면에 다시 그리지 않는다.
			continue;
		}

		//아이템을 출력한다.
		pItem->Draw( &MemDC );		
	}

	pDC->BitBlt( rcInvalid.left , rcInvalid.top , rcInvalid.Width() , rcInvalid.Height() , &MemDC , rcInvalid.left , rcInvalid.top , SRCCOPY );

	//-----------------------------------------------------------
	// 위에서 사용한 GDI 리소스들을 해제한다.
	//-----------------------------------------------------------
	MemDC.SelectObject( pOldBitmap );

	MemDC.DeleteDC();
	MemBitmap.DeleteObject();
}

//---------------------------------------------------------------------------
// 함수설명 : 생성된 새로운 아이템을 PhotoListCtrl에 추가한다.
//---------------------------------------------------------------------------
bool CItemListCtrl::AddTail(CItem * pItem )
{
	if( pItem == NULL )
	{
		return false;
	}

	//---------------------------------------------------------
	// 아이템의 영역을 얻어온다. (Zero Based Index)
	//---------------------------------------------------------
	pItem->SetRect( CalcItemRect( GetItemCount() ) );
	
	m_listItem.AddTail( pItem );

	return true;
}

//---------------------------------------------------------------------------
// 함수설명 : 생성된 새로운 아이템을 PhotoListCtrl에 추가한다.
//---------------------------------------------------------------------------
bool CItemListCtrl::AddHead(CItem *pItem )
{
	if( pItem == NULL )
	{
		return false;
	}

	//---------------------------------------------------------
	// 아이템의 영역을 얻어온다.  Zero based index
	//---------------------------------------------------------	
	m_listItem.AddHead( pItem );

	return true;
}

//---------------------------------------------------------------------------
// 함수설명 : 모든 아이템을 삭제한다.
//---------------------------------------------------------------------------
void CItemListCtrl::DeleteAllItems()
{
	//-----------------------------------------------------------------------
	// 사용한 아이템을 메모리에서 해제한다.
	//-----------------------------------------------------------------------
	while( !m_listItem.IsEmpty() )
	{
		CItem * pItem = (CItem *)m_listItem.RemoveHead();
		delete pItem;
	}

	m_pSelectedItem = NULL;
}

//--------------------------------------------------------------------
// 함수설명 : 지정된 아이템을 삭제한다.
//--------------------------------------------------------------------
bool CItemListCtrl::DeleteItem( CItem * pDeleteItem )
{
	if( pDeleteItem == NULL )
		return false;
	
	//----------------------------------------------------------------
	// 지정된 아이템을 아이템 리스트에서 찾는다.
	//----------------------------------------------------------------
	POSITION pos = m_listItem.Find( pDeleteItem );

	if( pos == NULL )
	{
		return false;
	}

	if( m_pSelectedItem == pDeleteItem )
	{
		m_pSelectedItem = NULL;
	}

	//----------------------------------------------------------------
	// 지정된 아이템을 메모리에서 삭제한다.
	//----------------------------------------------------------------
	delete pDeleteItem;

	m_listItem.RemoveAt( pos );

	return true;
}

//---------------------------------------------------------------------------
// 함수설명 : 아이템을 선택하거나 선택된 아이템을 푼다.
//---------------------------------------------------------------------------
CItemList * CItemListCtrl::GetItemList()
{
	return &m_listItem;
}
	
//--------------------------------------------------------------------
// 함수설명 : 아이템의 갯수를 리턴한다.
//--------------------------------------------------------------------
long CItemListCtrl::GetItemCount()
{
	return m_listItem.GetCount();
}

//--------------------------------------------------------------------
// 함수설명 : 포토앨범의 리스트를 다시 그린다.
//--------------------------------------------------------------------
void CItemListCtrl::ReDraw()
{
	//---------------------------------------------------------
	// 스크롤 사이즈를 조정한다.
	//---------------------------------------------------------
	InitScrollSize();

	//---------------------------------------------------------
	// 아이템들의 위치를 변경한다.
	//---------------------------------------------------------
	InitItemPos();

	//---------------------------------------------------------
	// 화면을 갱신한다.
	//---------------------------------------------------------
	Invalidate();
	UpdateWindow();
}

//--------------------------------------------------------------------
// 함수설명 : PhotoListCtrl 의 백색상을 나타낸다.
//--------------------------------------------------------------------
void CItemListCtrl::SetBgColor(COLORREF clrBgColor)
{
	//----------------------------------------------------------------
	// 이전에 사용했던 GDI 객체를 해제한다.
	//----------------------------------------------------------------
	m_brBackground.DeleteObject();

	//----------------------------------------------------------------
	// 새로운 색상의 브러쉬를 생성한다.
	//----------------------------------------------------------------
	m_brBackground.CreateSolidBrush(clrBgColor);
}

//--------------------------------------------------------------------
// 함수설명 : 배경색상을 사용자 지정 색상으로 출력할 수 있는 함수
//--------------------------------------------------------------------
HBRUSH CItemListCtrl::CtlColor(CDC* pDC , UINT nCtlColor) 
{
	return m_brBackground;
}

//---------------------------------------------------------------------------
// 함수설명 : 아이템의 상태를 변경한다. ( 선택 , 비선택 )
//---------------------------------------------------------------------------
void CItemListCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	//-----------------------------------------------------------------------
	// 리스트컨트롤이 포커스를 갖도록 한다.
	//-----------------------------------------------------------------------
	SetActiveWindow();
	SetFocus();

	//-----------------------------------------------------------------------
	// 생성된 CDC 의 뷰 포인트를 변경한다.
	//-----------------------------------------------------------------------
	CClientDC dc(this);
	dc.SetWindowOrg( 0 , m_nVScrollPos );

	//-----------------------------------------------------------------------
	// Device 좌표계에서 Logical 좌표계의 좌표로 변경한다.
	//-----------------------------------------------------------------------
	dc.DPtoLP( &point );

	//-----------------------------------------------------------------------
	// 현재 선택된 아이템의 포인터를 얻어온다.
	//-----------------------------------------------------------------------
	CItem * pItem = GetClickItem( point );

	//-----------------------------------------------------------------------
	// 현재 선택되어 있는 아이템의 상태를 변경한다.
	//-----------------------------------------------------------------------
	if( m_pSelectedItem != NULL && m_pSelectedItem != pItem ) 
	{
		m_pSelectedItem->SetStatus(ITEM_NORMAL);
		m_pSelectedItem->Draw( &dc );
	}

	//-----------------------------------------------------------------------
	// 선택된 아이템으로 지정한다.
	//-----------------------------------------------------------------------
	m_pSelectedItem = pItem;  
	
	if( pItem == NULL ) return;

	//-----------------------------------------------------------------------
	// 아이템에게 LButtonDown 이 발생했다는 것을 알린다.
	//-----------------------------------------------------------------------
	if( pItem->LButtonDown( this, nFlags, point ) ) pItem->Draw( &dc );

	//-----------------------------------------------------------------------
	// 아이템이 선택됐다는 것을 부모에게 알린다.
	//-----------------------------------------------------------------------
	GetParent()->SendMessage( m_nLButtonDown , (WPARAM)pItem , 0 );
}

//---------------------------------------------------------------------------
// 함수설명 : 마우스의 눌림이 풀렸을 때 필요한 작업을 하도록 한다.
//---------------------------------------------------------------------------
void CItemListCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	//-----------------------------------------------------------------------
	// 생성된 CDC 의 뷰 포인트를 변경한다.
	//-----------------------------------------------------------------------
	CClientDC dc(this);
	dc.SetWindowOrg( 0 , m_nVScrollPos );

	//-----------------------------------------------------------------------
	// 선택된 아이템이 있다면 LButtonUp 이벤트를 알린다.
	//-----------------------------------------------------------------------
	if( m_pSelectedItem != NULL ) 
	{
		if( m_pSelectedItem->LButtonUp( this, nFlags, point) )
		{
			m_pSelectedItem->Draw( &dc );
		}
	}
}

//---------------------------------------------------------------------------
// 함수설명 : 마우스가 움직일 때 필요한 작업을 하도록 한다.
//---------------------------------------------------------------------------
void CItemListCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	//-----------------------------------------------------------------------
	// 생성된 CDC 의 뷰 포인트를 변경한다.
	//-----------------------------------------------------------------------
	CClientDC dc(this);
	dc.SetWindowOrg( 0 , m_nVScrollPos );

	//-----------------------------------------------------------------------
	// Device 좌표계에서 Logical 좌표계의 좌표로 변경한다.
	//-----------------------------------------------------------------------
	dc.DPtoLP( &point );

	//-----------------------------------------------------------------------
	// 현재 선택된 아이템의 포인터를 얻어온다.
	//-----------------------------------------------------------------------
	CItem * pItem = GetClickItem( point );

	//-----------------------------------------------------------------------
	// 아이템에게 MouseMove 가 발생했다는 것을 알린다.
	//-----------------------------------------------------------------------
	if( pItem != NULL )
	{
		if( pItem->MouseMove( this , nFlags , point ) )
		{
			pItem->Draw( &dc );
		}
	}
}

//---------------------------------------------------------------------------
// 함수설명 : 더블클릭된 아이템을 얻어와 필요한 행동을 하도록 한다.
//---------------------------------------------------------------------------
void CItemListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	//-----------------------------------------------------------------------
	// 생성된 CDC 의 뷰 포인트를 변경한다.
	//-----------------------------------------------------------------------
	CClientDC dc(this);
	dc.SetWindowOrg( 0 , m_nVScrollPos );

	//-----------------------------------------------------------------------
	// 아이템에게 MouseMove 가 발생했다는 것을 알린다.
	//-----------------------------------------------------------------------
	if( m_pSelectedItem != NULL )
	{
		if( m_pSelectedItem->LButtonDblClk( this , nFlags , point ) )
		{
			m_pSelectedItem->Draw( &dc );
		}

		//-------------------------------------------------------------------
		// 아이템이 선택됐다는 것을 부모에게 알린다.
		//-------------------------------------------------------------------
		GetParent()->SendMessage( m_nLButtonDown , (WPARAM)m_pSelectedItem , 0 );
	}
}

//---------------------------------------------------------------------------
// 함수설명 : Wheel Scroll 처리를 직접 처리해 주는 것 
//---------------------------------------------------------------------------
BOOL CItemListCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if( m_nVPageSize < m_nViewHeight )
	{
		return TRUE;
	}

	int nPos = GetScrollPos(SB_VERT);

	if( zDelta > 0 )
	{
		OnVScroll( SB_LINEUP , nPos + zDelta , NULL );
	}
	else
	{
		OnVScroll( SB_LINEDOWN , nPos + zDelta , NULL );
	}

	return TRUE;
}

//---------------------------------------------------------------------------
// 함수설명 : 수직방향의 가상화면의 크기를 계산한다.
//---------------------------------------------------------------------------	
int CItemListCtrl::CalcVerticalSpace(CRect rcClient)
{
	//-----------------------------------------------------------------------
	// 수평방향으로 출력할 수 있는 아이템의 갯수를 얻어온다.
	//-----------------------------------------------------------------------
	int nHItems =  max( 1 , ( rcClient.Width() )/(GetItemWidth()+GetItemHGap()) );

	//-----------------------------------------------------------------------
	// 전체 아이템을 출력하기 위해 필요한 라인수를 계산한다.
	//-----------------------------------------------------------------------
	int nTotalItems = GetItemCount();

	int nLines = int( double(nTotalItems) / double(nHItems));

	if( (nTotalItems%nHItems ) != 0 ) nLines++;

	return GetItemVGap() + nLines*( GetItemHeight() + GetVGap() );
}

//---------------------------------------------------------------------------
// 함수설명 : 아이템 인덱스를 이용하여 아이템의 위치를 계산한다.
//---------------------------------------------------------------------------
// 변수설명 : nItem : 아이템의 인덱스
//---------------------------------------------------------------------------		
CRect CItemListCtrl::CalcItemRect( int nItem )
{
	//-----------------------------------------------------------------------
	// 클라이언트 영역을 얻어온다.
	//-----------------------------------------------------------------------
	CRect rcClient;
	GetClientRect( rcClient );

	//-----------------------------------------------------------------------
	// 아이템의 인덱스를 이용하여 위치를 계산한다.
	//-----------------------------------------------------------------------
	int nHItems =  max( 1 , ( rcClient.Width() )/(GetItemWidth() + GetItemHGap()) );

	int nX = nItem % nHItems;
	int nY = int( double(nItem) / double( nHItems ) );

	int left = GetHGap() + nX*( GetItemWidth() + GetItemHGap() );
	int top = GetVGap() + nY*( GetItemHeight() + GetItemVGap() ); 

	CRect rcItem( CPoint( left , top ) , CSize( GetItemWidth() , GetItemHeight() ) );		

	return rcItem;
}

//---------------------------------------------------------------------------
// 함수설명 : LButton Down Message ID 를 지정한다.
//---------------------------------------------------------------------------
// 부가설명 : LButtonDown 시 부모윈도우에게 전송할 메시지 아이디를 지정함.
//---------------------------------------------------------------------------
void CItemListCtrl::SetLButtonDown(UINT nMessageID)
{
	m_nLButtonDown = nMessageID;
}

//---------------------------------------------------------------------------
// 함수설명 : LButton Double Click Message ID 를 지정한다.
//---------------------------------------------------------------------------
// 부가설명 : LButtonDblClk 시 부모윈도우에게 전송할 메시지 아이디를 지정함.
//---------------------------------------------------------------------------
void CItemListCtrl::SetLButtonDblClk(UINT nMessageID)
{
	m_nLButtonDblClk = nMessageID;
}
