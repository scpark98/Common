// SCImageDlg.cpp: 구현 파일
//

//#include "afxdialogex.h"
#include "SCImageDlg.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

// CSCImageDlg 대화 상자

IMPLEMENT_DYNAMIC(CSCImageDlg, CDialog)

CSCImageDlg::CSCImageDlg(CWnd* parent)
{

}

CSCImageDlg::~CSCImageDlg()
{
}

bool CSCImageDlg::create(CWnd* parent, int x, int y, int cx, int cy)
{
	m_parent = parent;

	LONG_PTR dwStyle = WS_CHILD | WS_VISIBLE;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCImageDlg");
	AfxRegisterClass(&wc);

	bool res = CreateEx(NULL, wc.lpszClassName, _T("CSCImageDlg"), (DWORD)dwStyle, CRect(x, y, x + cx, y + cy), parent, 0);
	if (!res)
		return false;

	dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);
}

void CSCImageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCImageDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


// CSCImageDlg 메시지 처리기

void CSCImageDlg::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CStatic::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	CString str;

	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);

	dc.FillSolidRect(rc, GRAY32);

	if (m_img.is_empty())
		return;

	Gdiplus::Graphics g(dc.GetSafeHdc());

	g.SetInterpolationMode(m_interplationMode);	//부드럽게 보정

	if (m_fit2ctrl)
	{
		m_displayed = m_img.draw(g, rc);
		m_zoom = (double)m_displayed.Width() / (double)m_img.width;
	}
	else
	{
		//크게 확대해서 offset을 옮겨 그려주는 방식과
		//확대할 부분을 잘라 확대해서 그려주는 방식은
		//이론적으로는 후자가 더 효율적이지만 속도에 큰 차이가 느껴지진 않는다.
#if 0
		int nw = (double)rc.Width() / m_zoom;
		int nh = (double)rc.Height() / m_zoom;

		TRACE(_T("offset %d, %d\n"), m_offset.x, m_offset.y);
		Clamp(m_offset.x, (long)(rc.Width() - nw), (long)0);
		Clamp(m_offset.y, (long)(rc.Height() - nh), (long)0);
		TRACE(_T("offset %d, %d\n"), m_offset.x, m_offset.y);

		CGdiplusBitmap img;
		m_img.deep_copy(&img);
		img.sub_image(-m_offset.x, -m_offset.y, nw, nh);
		//g.DrawImage(img, 0, 0, rc.Width(), rc.Height());
		img.draw(&dc, 0, 0, rc.Width(), rc.Height());
#else
				//nw | nh가 rc보다 작으면 센터에 오도록 그려준다.
		int nw = m_img.width * m_zoom;
		int nh = m_img.height * m_zoom;

		Clamp(m_offset.x, (long)(rc.Width() - nw), (long)0);
		Clamp(m_offset.y, (long)(rc.Height() - nh), (long)0);

		int sx = m_offset.x;
		int sy = m_offset.y;

		if (nw < rc.Width())
		{
			sx = (rc.Width() - nw) / 2;
		}
		if (nh < rc.Height())
		{
			sy = (rc.Height() - nh) / 2;
		}

		m_displayed = m_img.draw(g, sx, sy, nw, nh);
#endif
	}

	//설정중인 ROI 사각형 표시
	if (IsCtrlPressed())
	{
		//DrawRectangle(&dc, m_screen_roi, red, NULL_BRUSH, 1);
	}
	else
	{
		get_screen_coord_from_real_coord(m_displayed, m_img.width, m_image_roi, &m_screen_roi);
	}

	draw_rectangle(&dc, m_screen_roi, red, NULL_BRUSH, 1);


	if (m_show_pixel)
	{
		CRect r = m_r_pixel;
		Gdiplus::SolidBrush brush(m_cr_pixel);
		g.FillRectangle(&brush, CRectTogpRectF(r));
		draw_rectangle(&dc, r, GRAY(96));
		r.DeflateRect(1, 1);
		draw_rectangle(&dc, r, GRAY(160));
	}
}

BOOL CSCImageDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
			case VK_RETURN :
			case VK_ESCAPE :
				return FALSE;

			case VK_DIVIDE :
				m_fit2ctrl = false;
				m_zoom = 1.0;
				Invalidate();
				return TRUE;
			case VK_MULTIPLY :
				m_fit2ctrl = true;
				Invalidate();
				return TRUE;

			case VK_ADD :
				zoom(1);
				return TRUE;
			case VK_SUBTRACT :
				zoom(-1);
				return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CSCImageDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CDialog::OnEraseBkgnd(pDC);
}

//레지스트리에 저장된 recent file 정보가 있다면 로딩한다.
bool CSCImageDlg::load()
{
	CString recent = AfxGetApp()->GetProfileString(_T("setting\\SCImageDlg"), _T("recent file"), _T(""));
	return load(recent);
}

bool CSCImageDlg::load(CString sFile)
{
	m_filename = sFile;

	m_image_roi.SetRectEmpty();
	m_screen_roi.SetRectEmpty();

	AfxGetApp()->WriteProfileString(_T("setting\\SCImageDlg"), _T("recent file"), sFile);
	bool res = m_img.load(sFile);
	Invalidate();

	//::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImageDlg, (WPARAM)&CImageStaticMessage(this, message_loading_completed), 0);

	return res;
}

bool CSCImageDlg::load(CString sType, UINT id)
{
	m_filename.Format(_T("Resource Image(id:%d)"), id);

	m_image_roi.SetRectEmpty();
	m_screen_roi.SetRectEmpty();

	bool res = m_img.load(sType, id);// , show_error);
	Invalidate();

	//::SendMessage(GetParent()->GetSafeHwnd(), Message_CImageStatic, (WPARAM)&CImageStaticMessage(this, message_loading_completed), 0);

	return res;
}

//png일 경우는 sType을 생략할 수 있다.
bool CSCImageDlg::load(UINT id)
{
	return load(_T("PNG"), id);
}

bool CSCImageDlg::copy_to_clipbard()
{
	return m_img.copy_to_clipbard();
}

CRect CSCImageDlg::get_image_roi()
{
	if (m_image_roi.IsRectEmpty())
		return CRect(0, 0, m_img.width, m_img.height);

	return m_image_roi;
}

//이미지 부드럽게 보정
void CSCImageDlg::set_smooth_interpolation(bool use)
{
	if (use)
		m_interplationMode = Gdiplus::InterpolationModeHighQualityBicubic;	//부드럽게 보정
	else
		m_interplationMode = Gdiplus::InterpolationModeNearestNeighbor;	//보정 최소화
}

void CSCImageDlg::fit2ctrl(bool fit)
{
	m_fit2ctrl = fit;
	Invalidate();
}

//mode : 1(zoom in), -1(zoom out), 0(reset)
void CSCImageDlg::zoom(int mode)
{
	if (mode > 0)
		m_zoom += 0.1;
	else if (mode < 0)
		m_zoom -= 0.1;
	else
		m_zoom = 1.0;

	m_fit2ctrl = false;

	Clamp(m_zoom, 0.1, 20.0);

	Invalidate();
}

void CSCImageDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	CRect rc;
	GetClientRect(rc);

	int margin = 8;
	int size = 44;
	m_r_pixel.SetRect(rc.left + margin, rc.bottom - margin - size, rc.left + margin + size, rc.bottom - margin);

	Invalidate();
}

void CSCImageDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	Clamp(point.x, m_displayed.left, m_displayed.right);
	Clamp(point.y, m_displayed.top, m_displayed.bottom);

	//if (m_displayed.PtInRect(point) == false)
	//	return;

	m_lbutton_down = true;
	m_ptClicked = point;
	SetCapture();

	if (IsCtrlPressed())
	{
		m_screen_roi.left = m_screen_roi.right = point.x;
		m_screen_roi.top = m_screen_roi.bottom = point.y;
		return;
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CSCImageDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_lbutton_down)
	{
		m_lbutton_down = false;
		ReleaseCapture();

		if (IsCtrlPressed())
		{
			Clamp(point.x, m_displayed.left, m_displayed.right);
			Clamp(point.y, m_displayed.top, m_displayed.bottom);
			m_screen_roi.right = point.x;
			m_screen_roi.bottom = point.y;

			if (abs(m_screen_roi.Width()) < 20 || abs(m_screen_roi.Height()) < 20)
			{
				m_screen_roi.SetRectEmpty();
				m_image_roi.SetRectEmpty();
				Invalidate();
				return;
			}

			//ctrl키를 떼고 roi가 모두 그려지면 m_image_roi로 변환해준다.
			m_screen_roi.NormalizeRect();
			get_real_coord_from_screen_coord(m_displayed, m_img.width, m_screen_roi, &m_image_roi);
			Invalidate();
			Wait(10);

			//::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImageDlg, (WPARAM)&CImageStaticMessage(this, WM_LBUTTONUP), 0);
		}
	}

	CDialog::OnLButtonUp(nFlags, point);
}

void CSCImageDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_lbutton_down)
	{
		//roi 설정중인 경우
		if (IsCtrlPressed())
		{
			Clamp(point.x, m_displayed.left, m_displayed.right);
			Clamp(point.y, m_displayed.top, m_displayed.bottom);
			m_screen_roi.right = point.x;
			m_screen_roi.bottom = point.y;

			//Ctrl키를 눌러서 roi가 그려지는 동안에는 이미지 확대/축소에 무관하게
			//마우스 위치가 그대로 그려져야 한다.
			Invalidate();
		}
		//이미지 이동을 위한 단순 드래그인 경우
		else
		{
			m_offset.x += (point.x - m_ptClicked.x);
			m_offset.y += (point.y - m_ptClicked.y);

			Invalidate();
			m_ptClicked = point;
		}
	}

	if (m_show_pixel)
	{
		CPoint pt;
		get_real_coord_from_screen_coord(m_displayed, m_img.width, point, &pt);
		m_cr_pixel = m_img.get_pixel(pt.x, pt.y);
		TRACE(_T("screen_pt = %d, %d  real_pt = %d, %d (%d, %d, %d, %d)\n"), point.x, point.y, pt.x, pt.y, m_cr_pixel.GetA(), m_cr_pixel.GetR(), m_cr_pixel.GetG(), m_cr_pixel.GetB());
		InvalidateRect(m_r_pixel);
		//m_static_pixel.set_back_color(m_cr_pixel);
		//m_static_pixel.set_text_color(get_complementary_gcolor(m_cr_pixel));
		//m_static_pixel.set_textf(get_complementary_gcolor(m_cr_pixel), _T("%d %d %d %d"), m_cr_pixel.GetA(), m_cr_pixel.GetR(), m_cr_pixel.GetG(), m_cr_pixel.GetB());
	}

	CDialog::OnMouseMove(nFlags, point);
}

BOOL CSCImageDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (IsCtrlPressed())
	{
		//zoom(zDelta > 0 ? -1 : 1);
	}

	return CDialog::OnMouseWheel(nFlags, zDelta, pt);
}
