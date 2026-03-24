// SCDropperDlg.cpp
#include "SCDropperDlg.h"

IMPLEMENT_DYNAMIC(CSCDropperDlg, CDialog)

BEGIN_MESSAGE_MAP(CSCDropperDlg, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

// --- 레지스트리 키 경로 (CSCColorPicker와 동일 그룹) ---
static LPCTSTR kRegSection = _T("setting\\color picker\\dropper");

CSCDropperDlg::CSCDropperDlg(CWnd* pParent)
{
}

CSCDropperDlg::~CSCDropperDlg()
{
	release_screen();
}

void CSCDropperDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void CSCDropperDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
}

// --- 설정 저장/복원 ---
void CSCDropperDlg::load_settings()
{
	CWinApp* app = AfxGetApp();
	if (!app) return;

	int val = app->GetProfileInt(kRegSection, _T("wnd_size"), kWndSizeDefault);
	m_wnd_size = max(kWndSizeMin, min(kWndSizeMax, val));

	val = app->GetProfileInt(kRegSection, _T("sample"), kSampleDefault);
	if (val % 2 == 0) val++;
	const int sample_max = (m_wnd_size % 2 == 0) ? m_wnd_size - 1 : m_wnd_size;
	m_sample = max(kSampleMin, min(sample_max, val));
}

void CSCDropperDlg::save_settings()
{
	CWinApp* app = AfxGetApp();
	if (!app) return;

	app->WriteProfileInt(kRegSection, _T("sample"), m_sample);
	app->WriteProfileInt(kRegSection, _T("wnd_size"), m_wnd_size);
}

// --- 전체 가상 데스크톱 1회 캡처 ---
void CSCDropperDlg::capture_screen()
{
	const int vx = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	const int vy = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
	const int vw = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const int vh = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

	m_screen_origin = CPoint(vx, vy);

	HDC hDesktopDC = ::GetDC(nullptr);
	m_hScreenDC = ::CreateCompatibleDC(hDesktopDC);
	m_hScreenBmp = ::CreateCompatibleBitmap(hDesktopDC, vw, vh);
	m_hOldScreenBmp = (HBITMAP)::SelectObject(m_hScreenDC, m_hScreenBmp);
	::BitBlt(m_hScreenDC, 0, 0, vw, vh, hDesktopDC, vx, vy, SRCCOPY);
	::ReleaseDC(nullptr, hDesktopDC);
}

void CSCDropperDlg::release_screen()
{
	if (m_hScreenDC)
	{
		::SelectObject(m_hScreenDC, m_hOldScreenBmp);
		::DeleteDC(m_hScreenDC);
		m_hScreenDC = nullptr;
	}
	if (m_hScreenBmp)
	{
		::DeleteObject(m_hScreenBmp);
		m_hScreenBmp = nullptr;
	}
}

// --- 생성 ---
bool CSCDropperDlg::create(CWnd* parent)
{
	load_settings();
	capture_screen();

	bool res = CreateEx(
		WS_EX_TOPMOST | WS_EX_LAYERED,
		AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW),
		nullptr,
		WS_POPUP | WS_VISIBLE,
		CRect(0, 0, m_wnd_size, m_wnd_size),
		parent, 0);

	if (!res)
	{
		release_screen();
		return false;
	}

	SetTimer(kTimerID, 16, nullptr);
	SetCursor(LoadCursor(nullptr, IDC_CROSS));
	return true;
}

// --- 타이머 -> 표시 갱신 ---
void CSCDropperDlg::OnTimer(UINT_PTR /*nIDEvent*/)
{
	update_display();
}

void CSCDropperDlg::update_display()
{
	if (!m_hScreenDC)
		return;

	CPoint cursor;
	GetCursorPos(&cursor);

	const int half = m_sample / 2;
	const int srcX = cursor.x - m_screen_origin.x - half;
	const int srcY = cursor.y - m_screen_origin.y - half;

	BITMAPINFOHEADER bih = { sizeof(bih), m_sample, -m_sample, 1, 32, BI_RGB };
	BYTE* pCapBits = nullptr;
	HBITMAP hCapDib = ::CreateDIBSection(nullptr, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&pCapBits, nullptr, 0);
	HDC hCapDC = ::CreateCompatibleDC(nullptr);
	HBITMAP hOldCap = (HBITMAP)::SelectObject(hCapDC, hCapDib);

	::BitBlt(hCapDC, 0, 0, m_sample, m_sample, m_hScreenDC, srcX, srcY, SRCCOPY);

	m_center_color = ::GetPixel(m_hScreenDC, cursor.x - m_screen_origin.x, cursor.y - m_screen_origin.y);

	::SelectObject(hCapDC, hOldCap);
	::DeleteDC(hCapDC);

	for (int i = 0, n = m_sample * m_sample; i < n; ++i)
		pCapBits[i * 4 + 3] = 0xFF;

	const int ws = m_wnd_size;

	Gdiplus::Bitmap content(ws, ws, PixelFormat32bppPARGB);
	{
		Gdiplus::Bitmap capBmp(m_sample, m_sample, m_sample * 4, PixelFormat32bppPARGB, pCapBits);

		Gdiplus::Graphics cg(&content);
		cg.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
		cg.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
		cg.DrawImage(&capBmp, 0, 0, ws, ws);
	}
	::DeleteObject(hCapDib);

	{
		Gdiplus::Graphics cg(&content);
		cg.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		const float cs = (float)ws / m_sample;

		if (cs > 4.0f)
		{
			Gdiplus::Pen gridPen(Gdiplus::Color(80, 0, 0, 0), 1.f);
			for (int i = 0; i <= m_sample; ++i)
			{
				float p = i * cs;
				cg.DrawLine(&gridPen, p, 0.f, p, (float)ws);
				cg.DrawLine(&gridPen, 0.f, p, (float)ws, p);
			}
		}

		const float cx = half * cs;
		const float cy = half * cs;
		Gdiplus::Pen centerPen(Gdiplus::Color(220, 255, 255, 255), 1.5f);
		cg.DrawRectangle(&centerPen, cx, cy, cs, cs);
	}

	Gdiplus::Bitmap canvas(ws, ws, PixelFormat32bppPARGB);
	{
		Gdiplus::Graphics g(&canvas);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		Gdiplus::TextureBrush tb(&content);
		g.FillEllipse(&tb,
			Gdiplus::RectF(0.f, 0.f, (float)ws, (float)ws));

		Gdiplus::Pen borderPen(Gdiplus::Color(200, 60, 60, 60), 2.5f);
		g.DrawEllipse(&borderPen,
			1.5f, 1.5f,
			(float)(ws - 3), (float)(ws - 3));
	}

	HBITMAP hBmp = nullptr;
	canvas.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBmp);

	HDC hDC = ::GetDC(nullptr);
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	HBITMAP hOld = (HBITMAP)::SelectObject(hMemDC, hBmp);

	BLENDFUNCTION blend = {};
	blend.BlendOp = AC_SRC_OVER;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat = AC_SRC_ALPHA;

	SIZE  sz = { ws, ws };
	POINT ptSrc = { 0, 0 };
	POINT ptDst = { cursor.x - ws / 2, cursor.y - ws / 2 };

	::UpdateLayeredWindow(m_hWnd, hDC, &ptDst, &sz,
		hMemDC, &ptSrc, 0, &blend, ULW_ALPHA);

	::SelectObject(hMemDC, hOld);
	::DeleteDC(hMemDC);
	::ReleaseDC(nullptr, hDC);
	::DeleteObject(hBmp);
}

// --- 입력 처리 ---
void CSCDropperDlg::OnLButtonDown(UINT /*nFlags*/, CPoint /*point*/)
{
	m_picked_color = Gdiplus::Color(255,
		GetRValue(m_center_color),
		GetGValue(m_center_color),
		GetBValue(m_center_color));
	m_picked = true;

	save_settings();
	KillTimer(kTimerID);
	DestroyWindow();
}

void CSCDropperDlg::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (nChar == VK_ESCAPE)
	{
		save_settings();
		KillTimer(kTimerID);
		DestroyWindow();
	}
}

BOOL CSCDropperDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint /*pt*/)
{
	if (nFlags & MK_CONTROL)
	{
		if (zDelta > 0)
			m_wnd_size = min(kWndSizeMax, m_wnd_size + kWndSizeStep);
		else
			m_wnd_size = max(kWndSizeMin, m_wnd_size - kWndSizeStep);
	}
	else
	{
		const int step = 2;
		if (zDelta > 0)
			m_sample = m_sample - step;
		else
			m_sample = m_sample + step;
	}

	// 홀수 보정 + 동적 범위 클램핑
	if (m_sample % 2 == 0)
		m_sample--;
	const int sample_max = (m_wnd_size % 2 == 0) ? m_wnd_size - 1 : m_wnd_size;
	m_sample = max(kSampleMin, min(sample_max, m_sample));

	update_display();
	return TRUE;
}