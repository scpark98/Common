// ImageStatic.cpp: 구현 파일
//

#include "ImageStatic.h"

#include "../Functions.h"
#include "../MemoryDC.h"

// CImageStatic

IMPLEMENT_DYNAMIC(CImageStatic, CStatic)

CImageStatic::CImageStatic()
{
}

CImageStatic::~CImageStatic()
{
}


BEGIN_MESSAGE_MAP(CImageStatic, CStatic)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_DROPFILES()
END_MESSAGE_MAP()



// CImageStatic 메시지 처리기

void CImageStatic::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CStatic::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	CString str;

	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);

	dc.FillSolidRect(rc, GRAY32);

	if (m_img.empty())
		return;

	Graphics g(dc.GetSafeHdc());

	g.SetInterpolationMode(m_interplationMode);	//부드럽게 보정

	if (m_fit2ctrl)
	{
		m_displayed = m_img.draw(&g, rc);
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

		m_displayed = m_img.draw(&g, sx, sy, nw, nh);
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

	DrawRectangle(&dc, m_screen_roi, red, NULL_BRUSH, 1);
	

	//TRACE(_T("before = %s\n"), get_rect_info_string(m_screen_roi, 2));
	//m_screen_roi.NormalizeRect();
	//TRACE(_T("after  = %s\n"), get_rect_info_string(m_screen_roi, 2));
	//Gdiplus::Pen pen(Gdiplus::Color(128, 255, 0, 0), 4);
	//g.DrawRectangle(&pen, m_screen_roi.left, m_screen_roi.top, m_screen_roi.Width(), m_screen_roi.Height());
}

//mode : 1(zoom in), -1(zoom out), 0(reset)
void CImageStatic::zoom(int mode)
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


void CImageStatic::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	Invalidate();
}


BOOL CImageStatic::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CStatic::OnEraseBkgnd(pDC);
}


BOOL CImageStatic::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	HCURSOR	hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor(IDC_ARROW);;

	//영역 선택시는 cross 커서로.
	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	if (IsCtrlPressed() && m_displayed.PtInRect(pt))
	{
		m_hCursor = (HCURSOR)AfxGetApp()->LoadStandardCursor(IDC_CROSS);
		::SetCursor(m_hCursor);
		return TRUE;
	}

	return CStatic::OnSetCursor(pWnd, nHitTest, message);
}


void CImageStatic::OnLButtonDown(UINT nFlags, CPoint point)
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

	CStatic::OnLButtonDown(nFlags, point);
}


void CImageStatic::OnLButtonUp(UINT nFlags, CPoint point)
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

			::SendMessage(GetParent()->GetSafeHwnd(), Message_CImageStatic, (WPARAM)&CImageStaticMessage(this, WM_LBUTTONUP), 0);
		}
	}

	CStatic::OnLButtonUp(nFlags, point);
}


void CImageStatic::OnMouseMove(UINT nFlags, CPoint point)
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

	CStatic::OnMouseMove(nFlags, point);
}


BOOL CImageStatic::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	return CStatic::OnMouseWheel(nFlags, zDelta, pt);
}


void CImageStatic::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TCHAR sfile[MAX_PATH];

	DragQueryFile(hDropInfo, 0, sfile, MAX_PATH);

	if (PathIsDirectory(sfile))
	{
		return;
	}
	else if (GetFileTypeFromFilename(sfile) == FILE_TYPE_IMAGE)
	{
		m_img.load(sfile);
		Invalidate();
	}

	CStatic::OnDropFiles(hDropInfo);
}


void CImageStatic::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CStatic::PreSubclassWindow();
}

void CImageStatic::fit2ctrl(bool fit)
{
	m_fit2ctrl = fit;
	Invalidate();
}

BOOL CImageStatic::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CStatic::PreTranslateMessage(pMsg);
}

//레지스트리에 저장된 recent file 정보가 있다면 로딩한다.
bool CImageStatic::load()
{
	CString recent = AfxGetApp()->GetProfileString(_T("setting\\ImageStatic"), _T("recent file"), _T(""));
	return load(recent);
}

bool CImageStatic::load(CString sFile, bool show_error)
{
	m_filename = sFile;

	m_image_roi.SetRectEmpty();
	m_screen_roi.SetRectEmpty();

	AfxGetApp()->WriteProfileString(_T("setting\\ImageStatic"), _T("recent file"), sFile);
	bool res = m_img.load(sFile, show_error);
	Invalidate();

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CImageStatic, (WPARAM)&CImageStaticMessage(this, message_loading_completed), 0);

	return res;
}

bool CImageStatic::load(CString sType, UINT id, bool show_error)
{
	m_filename.Format(_T("Resource Image(id:%d)"), id);

	m_image_roi.SetRectEmpty();
	m_screen_roi.SetRectEmpty();

	bool res = m_img.load(sType, id, show_error);
	Invalidate();

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CImageStatic, (WPARAM)&CImageStaticMessage(this, message_loading_completed), 0);

	return res;
}

//png일 경우는 sType을 생략할 수 있다.
bool CImageStatic::load(UINT id, bool show_error)
{
	return load(_T("PNG"), id, show_error);
}

bool CImageStatic::copy_to_clipbard()
{
	return m_img.copy_to_clipbard();
}

CRect CImageStatic::get_image_roi()
{
	if (m_image_roi.IsRectEmpty())
		return CRect(0, 0, m_img.width, m_img.height);

	return m_image_roi;
}

//이미지 부드럽게 보정
void CImageStatic::set_smooth_interpolation(bool use)
{
	if (use)
		m_interplationMode = Gdiplus::InterpolationModeHighQualityBicubic;	//부드럽게 보정
	else
		m_interplationMode = Gdiplus::InterpolationModeNearestNeighbor;	//보정 최소화
}
