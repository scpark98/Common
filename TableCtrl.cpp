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

	//resize(4, 8, false);
	m_width.resize(4);

	m_width[0] = (150);
	m_width[1] = (200);
	m_width[2] = (150);
	m_width[3] = (200);

	std::deque<CTableItem> t;

	t.clear();
	t.push_back(CTableItem(_T("티켓 정보"), true, 1.5));
	m.push_back(t);
	
	t.clear();
	t.push_back(CTableItem(_T("티켓 아이디")));
	t.push_back(CTableItem());
	m.push_back(t);

	t.clear();
	t.push_back(CTableItem(_T("고객 정보"), true, 1.5));
	m.push_back(t);

	t.clear();
	t.push_back(CTableItem(_T("신청 번호")));
	t.push_back(CTableItem());
	t.push_back(CTableItem(_T("고객 이름")));
	t.push_back(CTableItem());
	m.push_back(t);

	t.clear();
	t.push_back(CTableItem(_T("주민등록번호")));
	t.push_back(CTableItem());
	t.push_back(CTableItem(_T("고객 연락처")));
	t.push_back(CTableItem());
	m.push_back(t);

	t.clear();
	t.push_back(CTableItem(_T("주소")));
	t.push_back(CTableItem());
	m.push_back(t);

	t.clear();
	t.push_back(CTableItem(_T("신청 서비스 목록")));
	t.push_back(CTableItem());
	m.push_back(t);

	t.clear();
	t.push_back(CTableItem(_T("신분증 진위확인 특이사항")));
	t.push_back(CTableItem());
	t.push_back(CTableItem(_T("화면캡쳐여부")));
	t.push_back(CTableItem());
	m.push_back(t);

	CPoint pos = find_string(_T("주민등록번호"));
	m[pos.y][pos.x + 1].text = _T("한글123456-1234567");
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

	if (m.size() == 0)
		return;

	Color cr_grid(m[0][0].line_color);

	for (i = 0; i < m.size(); i++)
	{
		Pen pen_gridH(cr_grid, m[i][0].line_thick);
		Pen pen_gridV(cr_grid, 1.0);

		g.DrawLine(&pen_gridH, margin, sy, rc.right - margin, sy);

		for (j = 0; j < m[i].size(); j++)
		{
			RectF rtext = RectF(sx + margin, sy, m_width[j] - margin*2, m_line_height);
			if (!m[i][j].text.IsEmpty())
				g.DrawString(m[i][j].text, -1, m[i][j].text_bold ? &font_bold : &font_normal, rtext, &format, &br_text);

			//세로 구분선
			if (j > 0)
				g.DrawLine(&pen_gridV, sx, sy, sx, sy + m_line_height);

			sx += m_width[j];
		}

		sx = rc.left + 8;
		sy += m_line_height;
	}

	//마지막 라인
	if (m_draw_end_line)
		g.DrawLine(&Pen(cr_grid, m[0][0].line_thick), margin, sy, rc.right - margin, sy);
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

CPoint CTableCtrl::find_string(CString src)
{
	int x, y;

	for (y = 0; y < m.size(); y++)
	{
		for (x = 0; x < m[y].size(); x++)
		{
			if (m[y][x].text == src)
			{
				return CPoint(x, y);
			}
		}
	}

	return CPoint(-1, -1);
}