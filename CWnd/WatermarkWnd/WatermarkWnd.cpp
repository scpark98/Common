// WatermarkWnd.cpp: 구현 파일
//

#include "WatermarkWnd.h"
#include "../../Functions.h"

#define CWATERMARKWND_CLASSNAME _T("CWatermarkWnd")

// CWatermarkWnd

IMPLEMENT_DYNAMIC(CWatermarkWnd, CWnd)

CWatermarkWnd* g_pWnd = NULL;

CWatermarkWnd::CWatermarkWnd()
{
	RegisterWindowClass();
	g_pWnd = this;
}

CWatermarkWnd::~CWatermarkWnd()
{
}

BOOL CWatermarkWnd::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, CWATERMARKWND_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		CBrush	brush;
		brush.CreateSolidBrush(RGB(255, 255, 255));

		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = WatermarkWndMessageProc;
		//wndcls.lpfnWndProc = s_WndProc;
		//wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground = (HBRUSH)brush.GetSafeHandle();
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = CWATERMARKWND_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CWatermarkWnd, CWnd)
END_MESSAGE_MAP()



LRESULT CALLBACK WatermarkWndMessageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR nRetVal = (INT_PTR)TRUE;

	switch (uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;
	case WM_ERASEBKGND:
		return FALSE;
	case WM_PAINT:
	{
		CRect rc;
		GetClientRect(hWnd, rc);

		HDC hDC = ::GetDC(hWnd);
		HGDIOBJ hOldFont = SelectObject(hDC, g_pWnd->m_font.GetSafeHandle());

		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, g_pWnd->m_font_color);

		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
		FillRect(hDC, rc, hBrush);

		int x = -200;
		int y = 100;
		int line_count = 0;

		while (true)
		{
			if ((x < rc.right) && (y < rc.bottom + 200))
				TextOut(hDC, x, y, g_pWnd->m_text, g_pWnd->m_text.GetLength());

			x += 400;

			//rc.right로 비교하면 안된다. fixed로 비교해야 함.
			if (x > 2000)
			{
				line_count++;

				//각 줄의 시작 위치를 달리한다.
				if (line_count % 2)
					x = 10;
				else
					x = -200;

				y += 300;
			}

			if (y > 1400)
				break;
		}
		
		SelectObject(hDC, hOldFont);
		break;
	}
	//case WM_LBUTTONDOWN:
		//break;
		//DefWindowProc(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
		//return true;
		//case WM_TIMER:
		//	DefWindowProc(hWnd, WM_TIMER, wParam, lParam);
		//	//TRACE(_T("saldkfj\n"));
		//	//g_pWnd->OnTimer(CImageShapeWnd::timer_animate);
		//	return true;
	//case WM_SIZE:
	//	DefWindowProc(hWnd, WM_SIZE, wParam, lParam);
	//	return true;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);;
}

// CWatermarkWnd 메시지 처리기
bool CWatermarkWnd::create(HWND parentHwnd, CString text, CRect rw)
{
	m_hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		_T("CWatermarkWnd"), _T("CWatermarkWnd instance"),
		WS_POPUP, 0, 0, 1, 1, parentHwnd, NULL, GetModuleHandle(NULL), NULL);

	SLWA pSetLayeredWindowAttributes = NULL;
	HINSTANCE hmodUSER32 = LoadLibrary(_T("USER32.DLL"));
	pSetLayeredWindowAttributes = (SLWA)GetProcAddress(hmodUSER32, "SetLayeredWindowAttributes");

	pSetLayeredWindowAttributes(m_hWnd, RGB(255, 255, 255), 255.0 * 0.2, LWA_COLORKEY | LWA_ALPHA);

	if (m_hWnd == NULL)
		return false;

	m_text = text;

	memset(&m_lf, 0, sizeof(LOGFONT));
	_tcscpy_s(m_lf.lfFaceName, _T("맑은 고딕"));
	m_lf.lfWeight = FW_MEDIUM;
	m_lf.lfHeight = get_logical_size_from_font_size(m_hWnd, m_font_size);
	m_lf.lfEscapement = m_font_angle * 10;
	m_lf.lfQuality = CLEARTYPE_QUALITY;

	reconstruct_font();

	CRect parentRC = rw;

	//
	if (rw.IsRectEmpty())
	{
		CPoint lt, rb;
		::GetClientRect(parentHwnd, &parentRC);

		lt = parentRC.TopLeft();
		rb = parentRC.BottomRight();
		::ClientToScreen(parentHwnd, &lt);
		::ClientToScreen(parentHwnd, &rb);
		parentRC = CRect(lt.x, lt.y, rb.x, rb.y);
	}

	//if (!rw.IsRectEmpty())
		SetWindowPos(NULL, parentRC.left, parentRC.top, parentRC.Width(), parentRC.Height(), SWP_NOZORDER | SWP_SHOWWINDOW);

	return true;
}

void CWatermarkWnd::reconstruct_font()
{
	m_font.DeleteObject();

	m_lf.lfHeight = get_logical_size_from_font_size(m_hWnd, m_font_size);
	m_lf.lfEscapement = m_font_angle * 10;

	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	Invalidate(false);
}

void CWatermarkWnd::set_text(CString text, int font_size, int font_angle)
{
	m_text = text;

	if (font_size > 0)
		m_font_size = font_size;

	if (font_angle >= 0)
		m_font_angle = font_angle;

	reconstruct_font();
}

void CWatermarkWnd::set_font_name(CString fontname, BYTE byCharSet)
{
	if (fontname == _T(""))
		return;

	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), fontname);
	reconstruct_font();
}

void CWatermarkWnd::set_font_size(int font_size)
{
	if (font_size >= 0)
		m_font_size = font_size;

	reconstruct_font();
}

void CWatermarkWnd::set_font_angle(int font_angle)
{
	if (font_angle >= 0)
		m_font_angle = font_angle;

	reconstruct_font();
}

void CWatermarkWnd::set_font_color(COLORREF color)
{
	m_font_color = color;
	Invalidate();
	//RedrawWindow();
}

void CWatermarkWnd::adjust_window()
{
	CRect rc;
	CWnd* parent = GetParent();

	if (!parent)
		return;

	parent->GetClientRect(rc);
	parent->ClientToScreen(rc);
	//TRACE(_T("%s\n"), get_rect_info_string(rc, 1));
	if (m_hWnd)// && m_pDrawWnd->IsWindowVisible())
	{
		MoveWindow(rc);
	}
}