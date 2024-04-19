// ImageShapeDlg.cpp: 구현 파일
//

#include "ImageShapeDlg.h"
#include "afxdialogex.h"

#include <thread>
#include "../../Functions.h"

// CImageShapeDlg 대화 상자

IMPLEMENT_DYNAMIC(CImageShapeDlg, CDialogEx)

CImageShapeDlg::CImageShapeDlg()
{

}

CImageShapeDlg::~CImageShapeDlg()
{
}

void CImageShapeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CImageShapeDlg, CDialogEx)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CImageShapeDlg 메시지 처리기


BOOL CImageShapeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


BOOL CImageShapeDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	//모든 키입력은 parent에서 처리.
	if (pMsg->message == WM_KEYDOWN)
	{
		//Traceln(_T(""));
		return false;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CImageShapeDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	DefWindowProc(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));

	CDialogEx::OnLButtonDown(nFlags, point);
}

bool CImageShapeDlg::create(CWnd* parent, int left, int top, int right, int bottom)
{
	m_parent = parent;

	DWORD dwStyle = WS_POPUP;
	//id를 1234로 주면 왜 생성이 안될까...
	//className과 windowName을 CSCMenu 등 다른걸로 주면 생성이 실패하는 이유는??
	bool res = CreateEx(WS_EX_WINDOWEDGE | WS_EX_TOPMOST, _T("listbox"), _T("listbox"), dwStyle, CRect(left, top, right, bottom), parent, 0);

	dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	dwStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED;
	SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, dwStyle);

	return res;
}

void CImageShapeDlg::set_image(CGdiplusBitmap* img, bool deep_copy)
{
	if (deep_copy)
		img->deep_copy(&m_img);

	SetWindowPos(NULL, 0, 0, m_img.width, m_img.height, SWP_NOMOVE | SWP_NOZORDER);
	ShowWindow(SW_SHOW);

	gif_play();
}

bool CImageShapeDlg::load(UINT id)
{
	m_img.load(id);
	set_image(&m_img, false);
	return false;
}

bool CImageShapeDlg::load(CString sType, UINT id)
{
	m_img.load(sType, id);
	set_image(&m_img, false);
	return false;
}

bool CImageShapeDlg::load(CString sFile)
{
	m_img.load(sFile);
	set_image(&m_img, false);
	return false;
}

//alpha = 0 ~ 255
void CImageShapeDlg::alpha(int alpha)
{
	m_alpha = alpha;

	if (!m_img.is_animated_gif())
		render(m_img.m_pBitmap);
}

void CImageShapeDlg::render(Gdiplus::Bitmap* img)
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
		g.DrawRectangle(&Pen(Color(128, 255, 0, 0), 4.0f), Gdiplus::Rect(2, 2, sz.cx - 3, sz.cy - 3));

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

//animated gif인 경우
void CImageShapeDlg::gif_play(int new_state)
{
	if (new_state == state_stop)
	{
		m_gif_state = state_stop;
		return;
	}
	else if (new_state == state_toggle)
	{
		if (m_gif_state == state_play)
			m_gif_state = state_pause;
		else if (m_gif_state == state_pause)
			m_gif_state = state_play;
		return;
	}
	else if (new_state == state_pause)
	{
		m_gif_state = state_pause;
		return;
	}

	m_gif_state = state_play;

	//단일 이미지인지 gif인지에 따라 별도 처리
	if (m_img.is_animated_gif())
	{
		m_gif_state = state_play;
		std::thread t(&CImageShapeDlg::gif_thread, this);
		t.detach();
	}
	else
	{
		render(m_img.m_pBitmap);
	}
}

void CImageShapeDlg::gif_pause()
{
	m_gif_state = state_pause;
}

void CImageShapeDlg::gif_stop()
{
	m_gif_state = state_stop;
}

void CImageShapeDlg::gif_goto(int pos, bool pause)
{

}

void CImageShapeDlg::gif_thread()
{
	GUID   pageGuid = FrameDimensionTime;
	m_gif_index = 0;

	while (m_gif_state != state_stop)
	{
		if (m_gif_state == state_pause)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			continue;
		}

		m_img.m_pBitmap->SelectActiveFrame(&pageGuid, m_gif_index);
		render(m_img.m_pBitmap);
		long delay = ((long*)m_img.m_pPropertyItem->value)[m_gif_index] * 10;
		//if (delay < 10)
		//	delay = 10;
		TRACE(_T("%3d = %ld ms\n"), m_gif_index, delay);
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		m_gif_index++;

		if (m_gif_index >= m_img.get_frame_count())
		{
			if (true)
				m_gif_index = 0;
			else
				break;
		}
	}

	Trace(_T("%s terminated."), __function__);
}
