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

	resize(4, 8, false);
	m_width.resize(4);

	m_width[0] = (150);
	m_width[1] = (200);
	m_width[2] = (150);
	m_width[3] = (200);

	std::vector<CTableItem> t;

	//t.push_back(CTableItem(_T("0-0")));
	//m.push_back(t);
	m[0][0] = CTableItem(_T("0-0"));
	/*
	t.clear();
	t.push_back(CTableItem(_T("1-0")));
	t.push_back(CTableItem(_T("1-1")));
	m.push_back(t);

	t.clear();
	t.push_back(CTableItem(_T("2-0")));
	t.push_back(CTableItem(_T("2-1")));
	t.push_back(CTableItem(_T("2-2")));
	t.push_back(CTableItem(_T("2-3")));
	m.push_back(t);

	int n = m.size();


	m[2][0].text = _T("한글 텍스트 출력 긴 테스트 항목");
	*/
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

	g.SetSmoothingMode(SmoothingModeAntiAlias);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

	Color m_cr_text(255, 32, 32, 32);

	Gdiplus::Font font_bold(_T("Arial"), 14, FontStyleBold, UnitPixel);
	Gdiplus::Font font_normal(_T("Arial"), 14, FontStyleRegular, UnitPixel);
	Gdiplus::SolidBrush br_text(m_cr_text);
	StringFormat format;
	format.SetAlignment(StringAlignmentNear);
	format.SetLineAlignment(StringAlignmentCenter);

	int i, j;
	int margin = 8;
	int sx = rc.left + margin;
	int sy = rc.top + margin;

	for (i = 0; i < m.size(); i++)
	{
		Color cr_grid(m[i][0].line_color);
		Pen pen_gridY(cr_grid, m[i][0].line_thick);
		Pen pen_gridX(cr_grid, 1.0);

		g.DrawLine(&pen_gridY, margin, sy, rc.right - margin, sy);

		for (j = 0; j < m[i].size(); j++)
		{
			RectF rtext = RectF(sx + margin, sy, m_width[j] - margin*2, m_line_height);
			if (!m[i][j].text.IsEmpty())
				g.DrawString(m[i][j].text, -1, m[i][j].text_bold ? &font_bold : &font_normal, rtext, &format, &br_text);

			//세로 구분선
			if (j > 0)
				g.DrawLine(&pen_gridX, sx, sy, sx, sy + m_line_height);

			sx += m_width[j];
		}

		sx = rc.left + 8;
		sy += m_line_height;
	}

	//마지막 라인
	//g.DrawLine(&pen_grid, margin, sy, rc.right - margin, sy);
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

void CTableCtrl::resize(int cx, int cy, bool invalidate)
{
	if (m.size() == 0)
		m.resize(cy);

	for (int i = 0; i < cy; i++)
	{
		m[i].resize(cx);
	}

	if (invalidate)
		Invalidate();
}