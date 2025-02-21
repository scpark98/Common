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
	Gdiplus::Graphics g(dc.GetSafeHdc());

	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	Gdiplus::Color m_cr_text(255, 32, 32, 32);

	Gdiplus::Font font_bold(_T("Arial"), 14, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::Font font_normal(_T("Arial"), 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
	Gdiplus::SolidBrush br_text(m_cr_text);
	Gdiplus::StringFormat format;
	format.SetAlignment(Gdiplus::StringAlignmentNear);
	format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

	int i, j;
	int margin = 8;
	int sx = rc.left + margin;
	int sy = rc.top + margin;

	if (m.size() == 0)
		return;

	Gdiplus::Color cr_grid(GRAY_COLOR(7));

	for (i = 0; i < m.size(); i++)
	{
		draw_rectangle(g, CRect(rc.left, sy, rc.right, sy + m_line_height), Gdiplus::Color::Transparent, i % 2 ? m_cr_back_alt : m_cr_back);

		Gdiplus::Pen pen_gridH(m[i][0].line_color, m[i][0].line_thick);
		Gdiplus::Pen pen_gridV(cr_grid, 1.0);

		for (j = 0; j < m[i].size(); j++)
		{
			Gdiplus::RectF rtext = Gdiplus::RectF(sx + margin, sy, m_width[j] - margin*2, m_line_height);
			if (!m[i][j].text.IsEmpty())
				g.DrawString(m[i][j].text, -1, m[i][j].text_bold ? &font_bold : &font_normal, rtext, &format, &br_text);

			//세로 구분선
			if (j > 0)
				g.DrawLine(&pen_gridV, sx, sy, sx, sy + m_line_height);

			sx += m_width[j];
		}

		sx = rc.left + 8;
		sy += m_line_height;
		g.DrawLine(&pen_gridH, margin, sy, rc.right - margin, sy);

		sy += m[i][0].y_margin;
	}

	//마지막 라인
	if (m_draw_end_line)
		g.DrawLine(&Gdiplus::Pen(cr_grid, m[0][0].line_thick), margin, sy, rc.right - margin, sy);
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

void CTableCtrl::set_width(int num, ...)
{
	m_width.clear();

	va_list list;
	va_start(list, num);

	for (int i = 0; i < num; i++)
	{
		m_width.push_back(va_arg(list, int));
	}

	va_end(list);
}

void CTableCtrl::add_line(int num, ...)
{
	int count = 1;

	va_list list;
	va_start(list, num);

	std::deque<CTableItem> t;
	CTableItem *item = NULL;

	for (int i = 0; i < num; i++)
	{
		item = va_arg(list, CTableItem*);
		if (IsAvailableMemory((LPVOID)item) == ERROR_SUCCESS)
			t.push_back(*item);
		else
			t.push_back(CTableItem());
	}

	m.push_back(t);

	va_end(list);
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

CPoint CTableCtrl::find_string(CString find_str, bool discard_blank)
{
	int x, y;

	if (discard_blank)
		find_str.Remove(' ');

	for (y = 0; y < m.size(); y++)
	{
		for (x = 0; x < m[y].size(); x++)
		{
			CString item_text = m[y][x].text;

			if (discard_blank)
				item_text.Remove(' ');

			if (item_text == find_str)
			{
				return CPoint(x, y);
			}
		}
	}

	return CPoint(-1, -1);
}

void CTableCtrl::set_text(CString find_str, CString new_text, bool on_right_cell, bool discard_blank)
{
	CPoint pos;

	pos = find_string(find_str, discard_blank);
	if (pos.x >= 0 && pos.y >= 0)
	{
		if (on_right_cell)
			m[pos.y][pos.x + 1].text = new_text;
		else
			m[pos.y][pos.x].text = new_text;
		Invalidate();
	}
}
