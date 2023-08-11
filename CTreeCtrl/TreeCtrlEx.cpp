// CTreeCtrlEx.cpp: 구현 파일
//

#include "TreeCtrlEx.h"


// CTreeCtrlEx

IMPLEMENT_DYNAMIC(CTreeCtrlEx, CTreeCtrl)

CTreeCtrlEx::CTreeCtrlEx()
{

}

CTreeCtrlEx::~CTreeCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CTreeCtrlEx, CTreeCtrl)
	ON_WM_ACTIVATE()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	ON_WM_PAINT()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CTreeCtrlEx::OnTvnSelchanged)
	ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()



// CTreeCtrlEx 메시지 처리기




void CTreeCtrlEx::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CTreeCtrl::PreSubclassWindow();
}


BOOL CTreeCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CTreeCtrl::PreTranslateMessage(pMsg);
}


void CTreeCtrlEx::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CTreeCtrl::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}


BOOL CTreeCtrlEx::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	return CTreeCtrl::OnEraseBkgnd(pDC);
}


void CTreeCtrlEx::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CTreeCtrlEx::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CTreeCtrl::OnPaint()을(를) 호출하지 마십시오.
}


void CTreeCtrlEx::OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}


void CTreeCtrlEx::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CTreeCtrl::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

void CTreeCtrlEx::set_as_shell_tree(bool is_local /*= true*/)
{
	m_is_shell_tree = true;
	m_is_shell_tree_local = is_local;
	m_use_own_imagelist = true;
}
