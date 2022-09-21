// CTableCtrl.cpp: 구현 파일
//

#include "TableCtrl.h"
#include "../Common/Functions.h"
#include "../Common/MemoryDC.h"

#define CTABLECONTROL_CLASSNAME _T("CTableCtrl")

// CTableCtrl

IMPLEMENT_DYNAMIC(CTableCtrl, CWnd)

CTableCtrl::CTableCtrl()
{
	if (!RegisterWindowClass())
		return;

}

CTableCtrl::~CTableCtrl()
{
}

BOOL CTableCtrl::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, CTABLECONTROL_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		CBrush	brush;
		brush.CreateSolidBrush(RGB(255, 255, 255));

		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground = (HBRUSH)brush.GetSafeHandle();
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = CTABLECONTROL_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}

	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CTableCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
END_MESSAGE_MAP()



// CTableCtrl 메시지 처리기




BOOL CTableCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CWnd::PreCreateWindow(cs);
}


void CTableCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CWnd::PreSubclassWindow();
}


BOOL CTableCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CWnd::PreTranslateMessage(pMsg);
}


void CTableCtrl::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CWnd::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;

	GetClientRect(rc);
	CMemoryDC dc(&dc1, &rc);
	Graphics g(dc);

	DrawLine(&dc, 50, 10, rc.right, rc.bottom, red, 2);

	g.SetSmoothingMode(SmoothingModeAntiAlias);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

	Color m_cr_text(255, 32, 32, 32);
	Color m_cr_grid(128, 128, 128, 128);
	Pen pen_grid(m_cr_grid, 6.0f);
	g.DrawLine(&pen_grid, rc.right - 20, rc.top + 30, rc.left, rc.bottom);
}


BOOL CTableCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CWnd::OnEraseBkgnd(pDC);
}


void CTableCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	Invalidate();
}

void CTableCtrl::set_size(int cx, int cy)
{
	m_tb.resize(cy);

	for (int i = 0; i < m_tb.size(); i++)
		m_tb[i].resize(cx);

	Invalidate();
}