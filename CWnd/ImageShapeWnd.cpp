// ImageShapeWnd.cpp: 구현 파일
//

#include "ImageShapeWnd.h"
#include <thread>
#include "../../Common/Functions.h"

#define CIMAGESHAPEWND_CLASSNAME _T("CImageShapeWnd")

// CImageShapeWnd

IMPLEMENT_DYNAMIC(CImageShapeWnd, CWnd)

BEGIN_MESSAGE_MAP(CImageShapeWnd, CWnd)
	ON_WM_LBUTTONDOWN()
	//	ON_WM_CREATE()
	ON_WM_TIMER()
	//	ON_WM_DESTROY()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CImageShapeWnd::CImageShapeWnd()
{
	if (!RegisterWindowClass())
		return;
}

CImageShapeWnd::~CImageShapeWnd()
{
	m_run_thread_animate = false;
	Wait(100);

	DestroyWindow();
}

BOOL CImageShapeWnd::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, CIMAGESHAPEWND_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		CBrush	brush;
		brush.CreateSolidBrush(RGB(255, 255, 255));

		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ImageShapeWndMessageProc;
		//wndcls.lpfnWndProc = s_WndProc;
		//wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground = (HBRUSH)brush.GetSafeHandle();
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = CIMAGESHAPEWND_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

LRESULT CALLBACK CImageShapeWnd::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CImageShapeWnd* pThis; // our "this" pointer will go here
	if (uMsg == WM_NCCREATE) {
		// Recover the "this" pointer which was passed as a parameter
		// to CreateWindow(Ex).
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		pThis = static_cast<CImageShapeWnd*>(lpcs->lpCreateParams);
		// Put the value in a safe place for future use
		SetWindowLongPtr(hwnd, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(pThis));
	}
	else {
		// Recover the "this" pointer from where our WM_NCCREATE handler
		// stashed it.
		pThis = reinterpret_cast<CImageShapeWnd*>(
			GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}
	if (pThis) {
		// Now that we have recovered our "this" pointer, let the
		// member function finish the job.
		return pThis->WndProc(hwnd, uMsg, wParam, lParam);
	}
	// We don't know what our "this" pointer is, so just do the default
	// thing. Hopefully, we didn't need to customize the behavior yet.
	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CImageShapeWnd::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	::DefWindowProc(hwnd, uMsg, wParam, lParam);
	return 0;
}

bool CImageShapeWnd::create(HWND parentHwnd, CString file, bool auto_play, int w, int h, double fx, double fy)
{
	if (!create(parentHwnd))
		return false;

	if (!load(file, false))
		return false;

	resize(w, h, fx, fy);

	if (auto_play)
		play();
	else
		render(m_img.m_pBitmap);

	return true;
}

bool CImageShapeWnd::create(HWND parentHwnd, CString type, UINT id, bool auto_play, int w, int h, double fx, double fy)
{
	if (!create(parentHwnd))
		return false;

	if (!load(type, id, false))
		return false;

	resize(w, h, fx, fy);

	if (auto_play)
		play();
	else
		render(m_img.m_pBitmap);

	return true;
}


bool CImageShapeWnd::create(HWND parentHwnd)
{
	m_hWnd = CreateWindowEx(WS_EX_LAYERED, _T("CImageShapeWnd"), _T("CImageShapeWnd instance"),
		WS_POPUP, 0, 0, 1, 1, parentHwnd, NULL, GetModuleHandle(NULL), NULL);
	//m_hWnd = CreateWindow(_T("CImageShapeWnd"), _T("ImageShapeWnd"), WS_EX_LAYERED, 0, 0, 1, 1, parentHwnd, NULL, GetModuleHandle(NULL), NULL);

	//g_pWnd = this;

	if (m_hWnd == NULL)
		return false;

	SetTimer(timer_test, 1000, NULL);

	return true;
}

bool CImageShapeWnd::load(CString type, UINT id, bool auto_play)
{
	if (!m_img.load(type, id, true))
		return false;

	if (m_img.get_frame_count() == 1)
	{
		m_img.cvtColor32ARGB();
	}

	if (auto_play)
		play();

	return true;
}

bool CImageShapeWnd::load(CString file, bool auto_play)
{
	if (!PathFileExists(file))
	{
		AfxMessageBox(file + _T("\n\nFile not found."));
		return false;
	}

	if (!m_img.load(file, true))
		return false;

	if (auto_play)
		play();

	return true;
}

void CImageShapeWnd::resize(int w, int h, double fx, double fy)
{
	if (this == NULL)
		return;

	int nw;
	int nh;

	if (w > 1)
		nw = w;
	else if (fx > 0.0)
		nw = (double)m_img.width * fx;
	else
		nw = m_img.width;

	if (h > 1)
		nh = h;
	else if (fy > 0.0)
		nh = (double)m_img.height * fy;
	else
		nh = m_img.height;

	SetWindowPos(NULL, 0, 0, nw, nh, SWP_NOMOVE | SWP_NOZORDER);

	//if (m_img.is_animated_gif() && m_gif_frames.size())
	//	render(m_gif_frames[0]);
	//else
	//	render(&m_img);
}

void CImageShapeWnd::play()
{
	ShowWindow(SW_SHOW);

	//이미 재생중이면 리턴.
	if (m_run_thread_animate)
		return;

	if (m_img.get_frame_count() == 1)
	{
		render(m_img.m_pBitmap);
	}
	else
	{
		std::thread t(&CImageShapeWnd::thread_animate, this);
		t.detach();
	}
}

void CImageShapeWnd::set_pos(int pos)
{
	if (m_img.empty() || !m_img.is_animated_gif())
		return;

	if (m_run_thread_animate)
	{
		m_run_thread_animate = false;
		Wait(50);
	}

	GUID   pageGuid = FrameDimensionTime;
	m_img.m_pBitmap->SelectActiveFrame(&pageGuid, pos);
	render(m_img.m_pBitmap);
}

void CImageShapeWnd::set_pos(double dpos)
{
	if (m_img.empty() || !m_img.is_animated_gif())
		return;

	if (dpos < 0.0 || dpos > 1.0)
		return;

	int pos = (int)((double)m_img.get_frame_count() * dpos);
	set_pos(pos);
}


LRESULT CALLBACK ImageShapeWndMessageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	case WM_LBUTTONDOWN:
		DefWindowProc(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
		return true;
	//case WM_TIMER:
	//	DefWindowProc(hWnd, WM_TIMER, wParam, lParam);
	//	//TRACE(_T("saldkfj\n"));
	//	//g_pWnd->OnTimer(CImageShapeWnd::timer_animate);
	//	return true;
	case WM_SIZE:
		DefWindowProc(hWnd, WM_SIZE, wParam, lParam);
		return true;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}
	return nRetVal;
}

// CImageShapeWnd 메시지 처리기
void CImageShapeWnd::thread_animate()
{
	m_run_thread_animate = true;

	GUID   pageGuid = FrameDimensionTime;
	m_frame_index = 0;

	bool b = true;

	while (m_run_thread_animate)
	{
		m_img.m_pBitmap->SelectActiveFrame(&pageGuid, m_frame_index);
		//if (b)
			//m_img.m_pBitmap->RotateFlip(Gdiplus::RotateNoneFlipY);
		if (m_use_replace_color)
		{
			m_img.replace_color(m_old_color, m_new_color);
		}
		render(m_img.m_pBitmap);

		if (!m_run_thread_animate)
			break;

		long delay = ((long*)m_img.m_pPropertyItem->value)[m_frame_index] * 10;
		//if (delay < 10)
		//	delay = 10;
		TRACE(_T("%3d = %ld ms\n"), m_frame_index, delay);
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));

		m_frame_index++;

		if (m_frame_index >= m_img.get_frame_count())
		{
			b = false;
			if (m_repeat)
				m_frame_index = 0;
			else
				break;
		}
	}

	m_run_thread_animate = false;
}

void CImageShapeWnd::render(Gdiplus::Bitmap* img)
{
	if (!IsWindow(m_hWnd) || !img)
		return;

	CRect rc;
	GetWindowRect(rc);
	POINT ptSrc = { 0, 0 };
	POINT ptWinPos = { rc.left, rc.top };
	SIZE sz = { rc.Width(), rc.Height() };
	BLENDFUNCTION stBlend = { AC_SRC_OVER, 0, m_alpha, AC_SRC_ALPHA };

	HDC hDC = ::GetDC(m_hWnd);
	HDC hdcMemory = ::CreateCompatibleDC(hDC);

	BITMAPINFOHEADER bmih = { 0 };
	int nBytesPerLine = ((sz.cx * 32 + 31) & (~31)) >> 3;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = sz.cx;
	bmih.biHeight = sz.cy;
	bmih.biPlanes = 1;
	bmih.biBitCount = 32;
	bmih.biCompression = BI_RGB;
	bmih.biClrUsed = 0;
	bmih.biSizeImage = nBytesPerLine * sz.cy;

	PVOID pvBits = NULL;
	HBITMAP hbmpMem = ::CreateDIBSection(NULL, (PBITMAPINFO)&bmih, DIB_RGB_COLORS, &pvBits, NULL, 0);
	ASSERT(hbmpMem != NULL);
	memset(pvBits, 0, sz.cx * 4 * sz.cy);
	if (hbmpMem)
	{
		HGDIOBJ hbmpOld = ::SelectObject(hdcMemory, hbmpMem);
		Graphics g(hdcMemory);

		g.SetPageScale(1.0);
		g.SetPageUnit(UnitPixel);
		g.SetSmoothingMode(SmoothingModeAntiAlias);

		// Draw the dial
		g.DrawImage(img, 0, 0, sz.cx, sz.cy);
		//g.DrawRectangle(&Pen(Color(64, 255, 0, 0), 2.0f), Gdiplus::Rect(0, 0, sz.cx, sz.cy));

		::UpdateLayeredWindow(m_hWnd
			, hDC
			, &ptWinPos
			, &sz
			, hdcMemory
			, &ptSrc
			, 0
			, &stBlend
			, ULW_ALPHA
		);

		g.ReleaseHDC(hdcMemory);
		::SelectObject(hdcMemory, hbmpOld);
		::DeleteObject(hbmpMem);
	}

	::DeleteDC(hdcMemory);
	::DeleteDC(hDC);
}

void CImageShapeWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	DefWindowProc(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));

	CWnd::OnLButtonDown(nFlags, point);
}


BOOL CImageShapeWnd::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_LBUTTONDOWN)
	{
		TRACE(_T("asdf\n"));
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CImageShapeWnd::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	CWnd::PreSubclassWindow();
}


void CImageShapeWnd::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_test)
		TRACE(_T("lkasdjfl\n"));

	CWnd::OnTimer(nIDEvent);
}


void CImageShapeWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	resize(cx, cy);
}

//이미지의 x, y위치의 색상을 crNew로 대체한다.(한 점이 아닌 이미지 전체 대상)
void CImageShapeWnd::replace_color(int tx, int ty, Gdiplus::Color crNew)
{
	if (m_img.empty())
		return;

	m_use_replace_color = true;
	m_img.m_pBitmap->GetPixel(tx, ty, &m_old_color);
	m_new_color = crNew;

	if (m_img.get_frame_count() > 1)
	{
	}
	else
	{
		m_img.replace_color(tx, ty, crNew);
		render(m_img.m_pBitmap);
	}
}

