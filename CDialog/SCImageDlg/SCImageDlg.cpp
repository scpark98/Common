// SCImageDlg.cpp: 구현 파일
//

//#include "afxdialogex.h"
#include "SCImageDlg.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"
#include "../../AutoFont.h"

// CSCImageDlg 대화 상자

IMPLEMENT_DYNAMIC(CSCImageDlg, CDialog)

CSCImageDlg::CSCImageDlg(CWnd* parent)
{
	//m_font_pixel.CreateFont(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
	//	DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	//	DEFAULT_QUALITY, DEFAULT_PITCH, _T("Consolas"));
	//m_font_title.CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
	//	DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	//	DEFAULT_QUALITY, DEFAULT_PITCH, _T("Consolas"));
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

	CRect r_pixel = make_rect(x + 8, y + cy - 8 - PIXEL_INFO_CY, PIXEL_INFO_CX, PIXEL_INFO_CY);
	m_static_pixel.create(_T("A 0\nR 0\nG 0\nB 0"), WS_CHILD | SS_CENTER | SS_CENTERIMAGE, r_pixel, this);
	m_static_pixel.sunken();
	m_static_pixel.set_font_name(_T("Consolas"));
	m_static_pixel.set_font_size(7);

	res = m_slider_gif.Create(WS_CHILD, make_rect(x + 8, y + cy - 8 - 10, 120, 10), this, 0);
	m_slider_gif.set_style(CSCSliderCtrl::style_progress);
	m_slider_gif.set_font_name(_T("Arial"));
	m_slider_gif.set_font_size(7);
	m_slider_gif.set_back_color(gGRAY(32));
	m_slider_gif.set_use_slide();

	m_br_zigzag = m_img.get_zigzag_pattern(32);

	return true;
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
	//ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_SETCURSOR()
	ON_WM_WINDOWPOSCHANGED()
	ON_REGISTERED_MESSAGE(Message_CGdiplusBitmap, &CSCImageDlg::on_message_from_GdiplusBitmap)
	ON_REGISTERED_MESSAGE(Message_CSCSliderCtrl, &CSCImageDlg::on_message_from_CSCSliderCtrl)
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


	//수정 필요. gif도 stretch mode에 따라 꽉차게 또는 원래 크기로 표시한다. 아직 미구현이며 현재는 zoom 표시됨.	
	if (m_img.is_valid() && m_img.is_animated_gif())
	{
		CRect r = get_ratio_rect(rc, m_img.width, m_img.height, 0, false);
		CRgn rgn_rc;
		CRgn rgn_img;
		CRgn rgn_exclude;
		rgn_rc.CreateRectRgnIndirect(&rc);
		rgn_img.CreateRectRgnIndirect(&r);
		rgn_exclude.CreateRectRgn(0, 0, 1, 1);
		rgn_exclude.CombineRgn(&rgn_rc, &rgn_img, RGN_XOR);
		dc1.SelectClipRgn(&rgn_exclude);
		dc1.FillSolidRect(rc, GRAY32);
		return;
	}

	//이 코드를 위의 gif 이미지일때의 처리 위에 하면 깜빡임이 발생하거나 의도대로 그려지지 않게 된다.
	CMemoryDC dc(&dc1, &rc);

	//기본 배경색으로 rc를 채우고
	dc.FillSolidRect(rc, GRAY32);

	//pixel info SCStatic이 깜빡이지 않도록 clip영역을 줬으나 여전히 깜빡거림. OnMouseMove에서 이전값과 다르면 갱신하도록 수정함.
	CRgn rgn_pixel_info;
	CRect rc_pixel_info = m_static_pixel.get_rect();
	if (m_static_pixel.IsWindowVisible() && rgn_pixel_info.CreateRectRgnIndirect(&rc_pixel_info))
	{
		CRgn rgn, rgn_target;
		rgn.CreateRectRgnIndirect(&rc);

		rgn_target.CreateRectRgnIndirect(&rc);
		rgn_target.CombineRgn(&rgn, &rgn_pixel_info, RGN_XOR);

		dc.SelectClipRgn(&rgn_target);
	}

	if (m_img.is_empty())
	{
		dc.SelectClipRgn(NULL);

		CRect rText;
		CString msg = _T("No images to display.");
		dc.DrawText(msg, rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_CALCRECT);

		DrawShadowText(dc.GetSafeHdc(), msg, msg.GetLength(), rText,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE, beige, black, 2, 1);

		return;
	}

	Gdiplus::Graphics g(dc.GetSafeHdc());

	g.SetInterpolationMode(m_interplationMode);	//부드럽게 보정 or 실제 픽셀
	//TRACE(_T("m_interplationMode = %d\n"), m_interplationMode);

	//확대 모드에 따른 이미지가 표시될 실제 영역을 구한다.
	if (m_fit2ctrl)
	{
		m_r_display = m_img.calc_rect(rc);// m_img.draw(g, rc);
		m_zoom = (double)m_r_display.Width() / (double)m_img.width;
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

		//TRACE(_T("offset %d, %d\n"), m_offset.x, m_offset.y);
		//if (m_screen_roi.IsEmptyArea() == false)
		//{
		//	Gdiplus::RectF screen_roi = m_screen_roi;
		//	screen_roi.Offset(m_offset.x, m_offset.y);
		//	m_screen_roi = screen_roi;
		//}

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

		m_r_display = make_rect(sx, sy, nw, nh);//m_img.draw(g, sx, sy, nw, nh);
#endif
	}

	//만약 투명 픽셀이 포함된 이미지라면 지그재그 격자를 그려준 후
	if (m_img.is_pixelformat_alpha())
	{
		g.FillRectangle(m_br_zigzag.get(), CRect2GpRect(m_r_display));
	}

	//실제 이미지를 그려준다.
	m_img.draw(g, m_r_display);


	//이미지 정보 표시
	if (m_show_info)// && m_parent->IsZoomed())
	{
		CAutoFont af(_T("맑은 고딕"));
		af.SetHeight(24);

		CFont* pOldFont = dc.SelectObject(&af);
		CString filename = get_part(m_filename, fn_name);
		CString info;

		info.Format(_T("파일 이름 : %s\n파일 크기 : %s\n수정 날짜 : %s\n이미지 정보 : %dx%dx%d\n확대 배율 : %.0f%%"),
					filename + m_alt_info,
					get_file_size_str(m_filename),
					get_datetime_str(GetFileLastModifiedTime(m_filename)),
					m_img.width, m_img.height, m_img.channel * 8,
					m_zoom * 100.0);

		//dc.GetTextExtend(info)는 멀티라인 정보를 얻을 수 없다.
		CRect rText;
		dc.DrawText(info, rText, DT_LEFT | DT_TOP | DT_CALCRECT);
		rText.OffsetRect(8, 8);

		DrawShadowText(dc.GetSafeHdc(), info, info.GetLength(),
			rText,
			DT_NOCLIP | DT_LEFT | DT_TOP, beige, black, 2, 1);

		//dc.SetBkMode(TRANSPARENT);
		//dc.SetTextColor(white);
		//dc.DrawText(m_filename, CRect(rc.left + 4, rc.top + 2, rc.left + 4 + sz.cx + 2, rc.top + 2 + sz.cy), DT_NOCLIP | DT_LEFT | DT_TOP);
		dc.SelectObject(pOldFont);
	}

	//CRect screen_roi;
	//CRect image_roi = GpRectF2CRect(m_image_roi);

	//roi를 그리거나 위치, 크기를 조정할 때는 오로지 m_screen_roi만 신경쓴다.
	CRect screen_roi = GpRectF2CRect(m_screen_roi);
	screen_roi.NormalizeRect();

	if (m_lbutton_down)// IsCtrlPressed())
	{
		draw_rectangle(&dc, screen_roi, red, NULL_BRUSH, 1, PS_DASH, R2_XORPEN);
	}
	else if (m_image_roi.IsEmptyArea() == false)
	{
		//이미지 확대 축소 등에 의해 m_r_display가 변경되면 그에 따라 m_screen_roi도 다시 계산해줘야 한다.
		get_screen_coord_from_real_coord(m_r_display, m_img.width, m_image_roi, &m_screen_roi);
		//m_screen_roi.Offset(m_offset.x, m_offset.y);
		screen_roi = GpRectF2CRect(m_screen_roi);
		//이미 offset 변경에 의한 보정은 get_screen_coord_from_real_coord()에서 해준다.
		//screen_roi.OffsetRect(m_offset);
		draw_rectangle(&dc, screen_roi, red, NULL_BRUSH, 1, PS_DASH, R2_XORPEN);
	}

	if (screen_roi.IsRectEmpty() == false)
	{
		Gdiplus::RectF image_roi;
		get_real_coord_from_screen_coord(m_r_display, m_img.width, CRect2GpRectF(screen_roi), &image_roi);
		get_resizable_handle(screen_roi, m_roi_handle, 4);

		//image_roi 역시 normalize_rect을 해줘야 한다. 그렇지 않으면 뒤집어 그릴 경우 x1,y1이 x2, y2보다 큰 좌표로 표시된다.
		normalize_rect(image_roi);

		//9군데 조절 핸들을 그려준다.
		for (int i = 0; i < 9; i++)
			//draw_rectangle(&dc, m_roi_handle[i], red, NULL_BRUSH, 0, PS_SOLID);
			draw_rectangle(g, m_roi_handle[i], Gdiplus::Color::Red, Gdiplus::Color(128, 255, 64, 0));

		if (m_show_roi_info)
		{
			CAutoFont af(_T("Arial"));
			af.SetHeight(14);

			CFont* pOldFont = dc.SelectObject(&af);

			CSize sz = dc.GetTextExtent(_T("A"));
			dc.SetBkMode(TRANSPARENT);

			//roi 영역 정보를 표시한다. 비록 image_roi는 변환식에 의해 소수점이지만 실제 이미지 픽셀은 정수로 표현해준다.
			//start
			dc.SetTextColor(yellow);
			str.Format(_T("%.0f, %.0f"), image_roi.X, image_roi.Y);
			sz = dc.GetTextExtent(str);
			DrawShadowText(dc.GetSafeHdc(), str, str.GetLength(),
				CRect(screen_roi.left + 4, screen_roi.top + 2, screen_roi.left + 4 + sz.cx + 2, screen_roi.top + 2 + sz.cy),
				DT_NOCLIP | DT_LEFT | DT_TOP, white, black, 2, 1);

			//end
			str.Format(_T("%.0f, %.00f"), image_roi.X + image_roi.Width, image_roi.Y + image_roi.Height);
			sz = dc.GetTextExtent(str);
			DrawShadowText(dc.GetSafeHdc(), str, str.GetLength(),
				CRect(screen_roi.left + screen_roi.Width() - 2 - sz.cx, screen_roi.top + screen_roi.Height() - sz.cy - 2, screen_roi.left + screen_roi.Width() - 2, screen_roi.top + screen_roi.Height()),
				DT_NOCLIP | DT_RIGHT | DT_BOTTOM, white, black, 2, 1);

			//size and distance
			dc.SetTextColor(green);
			str.Format(_T("%.0f x %.0f"), image_roi.Width, image_roi.Height);
			sz = dc.GetTextExtent(str);
			DrawShadowText(dc.GetSafeHdc(), str, str.GetLength(),
				CRect(screen_roi.CenterPoint().x - sz.cx / 2, screen_roi.CenterPoint().y + 2, screen_roi.CenterPoint().x + sz.cx / 2, screen_roi.CenterPoint().y + 2 + sz.cy),
				DT_NOCLIP | DT_CENTER | DT_TOP, yellow, black, 2, 1);

			dc.SelectObject(pOldFont);
		}
	}

	dc.SelectClipRgn(NULL);
}

//이 컨트롤에서 자체 처리할 메시지와 반드시 parent에서 처리할 메시지를 명확히 해야 한다.
//여기서 바로 처리할 수 밖에 없는 메시지라면 모를까 모두 여기서 처리하면
//범용성이 떨어질 수 있다.
BOOL CSCImageDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	//parent에서 처리해야 하는 메시지 일 경우 return FALSE;를 해줘야 한다.
	switch (pMsg->message)
	{
		case WM_SYSKEYDOWN :
		case WM_MBUTTONDOWN :
			return FALSE;
	}

	if (pMsg->message == WM_KEYDOWN)
	{
		bool is_ctrl_pressed = IsCtrlPressed();
		double interval = (IsCtrlPressed() ? 5.0 : 1.0);

		switch (pMsg->wParam)
		{
			case VK_RETURN :
			case VK_ESCAPE :
				return FALSE;
			case VK_SPACE :
				if (m_img.is_animated_gif())
				{
					m_img.pause_gif(-1);
					return TRUE;
				}
				break;
			case VK_DIVIDE :
				fit2ctrl(false);
				m_zoom = 1.0;
				Invalidate();
				return TRUE;
			case VK_MULTIPLY :
				fit2ctrl(true);
				Invalidate();
				return TRUE;

			case VK_ADD :
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.Inflate(Gdiplus::PointF(interval, interval));
					//get_screen_coord_from_real_coord(m_r_display, m_img.width, m_image_roi, &m_screen_roi);
					Invalidate();
					return TRUE;
				}
				zoom(1);
				return TRUE;
			case VK_SUBTRACT :
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.Inflate(Gdiplus::PointF(-interval, -interval));
					//get_screen_coord_from_real_coord(m_r_display, m_img.width, m_image_roi, &m_screen_roi);
					Invalidate();
					return TRUE;
				}
				zoom(-1);
				return TRUE;
			case 'C' :
				if (IsCtrlPressed())
				{
					copy_to_clipbard();
					return TRUE;
				}
				break;
			case VK_UP :
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.Y -= interval;
					Invalidate();
					return TRUE;
				}
				return FALSE;
			case VK_DOWN:
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.Y += interval;
					Invalidate();
					return TRUE;
				}
				return FALSE;
			case VK_LEFT:
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.X -= interval;
					Invalidate();
					return TRUE;
				}
				break;
			case VK_RIGHT:
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.X += interval;
					Invalidate();
					return TRUE;
				}
				break;
			default :
				return FALSE;
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

	m_image_roi = Gdiplus::RectF();
	m_screen_roi = Gdiplus::RectF();

	AfxGetApp()->WriteProfileString(_T("setting\\SCImageDlg"), _T("recent file"), sFile);
	bool res = m_img.load(sFile);

	if (m_img.is_animated_gif())
	{
		CRect rc;
		GetClientRect(rc);
		m_slider_gif.set_range(0, m_img.get_frame_count() - 1);
		m_slider_gif.set_pos(0);
		m_slider_gif.MoveWindow(rc.left + 8, rc.bottom - 8 - 10, 120, 10);
		m_slider_gif.ShowWindow(SW_SHOW);
		m_img.set_animation(m_hWnd, rc);
	}
	else
	{
		m_slider_gif.ShowWindow(SW_HIDE);
		Invalidate();
	}

	//::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImageDlg, (WPARAM)&CImageStaticMessage(this, message_loading_completed), 0);

	return res;
}

bool CSCImageDlg::load(CString sType, UINT id)
{
	m_filename.Format(_T("Resource Image(id:%d)"), id);

	m_image_roi = Gdiplus::RectF();
	m_screen_roi = Gdiplus::RectF();

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

void CSCImageDlg::set_show_info(bool show)
{
	m_show_info = show;
	Invalidate();
}

void CSCImageDlg::set_show_pixel(bool show)
{
	m_show_pixel = show;
	AfxGetApp()->WriteProfileInt(_T("setting"), _T("show pixel"), m_show_pixel);
}

bool CSCImageDlg::copy_to_clipbard()
{
	//roi가 있으면 그 영역만, 그렇지 않다면 전체 이미지를 복사한다.
	if (m_image_roi.IsEmptyArea() == false)
	{
		CGdiplusBitmap roi_img;
		m_img.deep_copy(&roi_img);
		roi_img.sub_image(m_image_roi);
		roi_img.copy_to_clipbard();
		return true;
	}
	return m_img.copy_to_clipbard();
}

Gdiplus::RectF CSCImageDlg::get_image_roi()
{
	//if (m_image_roi.IsEmptyArea())
	//	return Gdiplus::RectF(0, 0, m_img.width, m_img.height);

	return m_image_roi;
}

int CSCImageDlg::get_smooth_interpolation()
{
	return (int)m_interplationMode;
}

//이미지 부드럽게 보정
void CSCImageDlg::set_smooth_interpolation(int type)
{
	m_interplationMode = (Gdiplus::InterpolationMode)type;
	Invalidate();
}

void CSCImageDlg::fit2ctrl(bool fit)
{
	m_fit2ctrl = fit;
	AfxGetApp()->WriteProfileInt(_T("setting"), _T("fit to ctrl"), m_fit2ctrl);
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
	//get_screen_coord_from_real_coord(m_r_display, m_img.width, m_image_roi, &m_screen_roi);
	//Wait(1);
	Invalidate();
}

void CSCImageDlg::zoom(double ratio)
{
	m_fit2ctrl = false;
	m_zoom = ratio;
	Clamp(m_zoom, 0.1, 20.0);
	Invalidate();
}

void CSCImageDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_static_pixel.m_hWnd == NULL)
		return;

	if (m_img.is_animated_gif())
	{
		CRect rc;
		GetClientRect(rc);
		m_img.move_gif(rc);

		m_slider_gif.MoveWindow(rc.left + 8, rc.bottom - 8 - 10, 120, 10);
	}
	//else
	{
		Invalidate();
	}

	if (m_show_pixel)
	{
		CRect rc;
		GetClientRect(rc);

		CRect r_pixel = make_rect(rc.left + 8, rc.bottom - 8 - PIXEL_INFO_CY, PIXEL_INFO_CX, PIXEL_INFO_CY);
		m_static_pixel.MoveWindow(r_pixel);
	}
}

void CSCImageDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	Clamp(point.x, m_r_display.left, m_r_display.right);
	Clamp(point.y, m_r_display.top, m_r_display.bottom);

	//if (m_r_display.PtInRect(point) == false)
	//	return;

	m_lbutton_down = true;
	m_ptClicked = point;
	SetCapture();

	//roi 설정중
	if (IsCtrlPressed())
	{
		m_screen_roi = Gdiplus::RectF(point.x, point.y, 0, 0);
		return;
	}
	//roi의 크기를 변경중
	else if (m_handle_index >= 0)
	{

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

		//Clamp(point.x, m_r_display.left, m_r_display.right);
		//Clamp(point.y, m_r_display.top, m_r_display.bottom);

		if (IsCtrlPressed())
		{
			m_screen_roi.Width = point.x - m_screen_roi.X;
			m_screen_roi.Height = point.y - m_screen_roi.Y;
		}

		if (abs(m_screen_roi.Width) < 20 || abs(m_screen_roi.Height) < 20)
		{
			m_screen_roi = Gdiplus::RectF();
			m_image_roi = Gdiplus::RectF();
			Invalidate();
			return;
		}

		//roi가 모두 그려지면, 또는 이동, 크기조정이 완료되면 m_image_roi로 변환해준다.
		normalize_rect(m_screen_roi);
		get_real_coord_from_screen_coord(m_r_display, m_img.width, m_screen_roi, &m_image_roi);
		TRACE(_T("roi completed.\n"));
		Invalidate();
	}

	CDialog::OnLButtonUp(nFlags, point);
}

void CSCImageDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_lbutton_down)
	{
		Clamp(point.x, m_r_display.left, m_r_display.right);
		Clamp(point.y, m_r_display.top, m_r_display.bottom);

		//roi 설정중인 경우
		if (IsCtrlPressed())
		{
			m_static_pixel.ShowWindow(SW_HIDE);

			m_screen_roi.Width = point.x - m_screen_roi.X;
			m_screen_roi.Height = point.y - m_screen_roi.Y;

			//Ctrl키를 눌러서 roi가 그려지는 동안에는 이미지 확대/축소에 무관하게
			//마우스 위치가 그대로 그려져야 한다.
			//InvalidateRect(GpRectF2CRect(m_screen_roi));
			Invalidate();
		}
		//그려진 roi의 크기, 위치를 조절하는 경우
		else if (m_handle_index >= 0)
		{
			TRACE(_T("m_handle_index = %d\n"), m_handle_index);
			switch (m_handle_index)
			{
				case corner_inside :
					m_screen_roi.X = point.x - m_screen_roi.Width / 2;
					m_screen_roi.Y = point.y - m_screen_roi.Height / 2;
					adjust_rect_range(m_screen_roi, CRect2GpRectF(m_r_display));
					//if (m_screen_roi.X < m_r_display.left)
					//	m_screen_roi.X = m_r_display.left;
					//if (m_screen_roi.Y < m_r_display.top)
					//	m_screen_roi.Y = m_r_display.top;
					break;
				case corner_left :
					set_left(m_screen_roi, point.x);
					break;
				case corner_right:
					m_screen_roi.Width = point.x - m_screen_roi.X;
					break;
				case corner_top:
					set_top(m_screen_roi, point.y);
					break;
				case corner_bottom:
					m_screen_roi.Height = point.y - m_screen_roi.Y;
					break;
				case corner_topleft:
					set_top(m_screen_roi, point.y);
					set_left(m_screen_roi, point.x);
					break;
				case corner_topright :
					set_top(m_screen_roi, point.y);
					m_screen_roi.Width = point.x - m_screen_roi.X;
					break;
				case corner_bottomleft :
					m_screen_roi.Height = point.y - m_screen_roi.Y;
					set_left(m_screen_roi, point.x);
					break;
				case corner_bottomright :
					m_screen_roi.Height = point.y - m_screen_roi.Y;
					m_screen_roi.Width = point.x - m_screen_roi.X;
					break;
			}

			//m_screen_roi는 m_r_display를 벗어나지 않도록 보정한다.
			//단, 보정 할 때, corner_inside일때는 크기를 유지시키지만 그 외의 경우는 크기를 유지하지 않도록 파라미터를 줘야 한다.
			//adjust_rect_range(m_screen_roi, m_r_display, (m_handle_index == corner_inside), true);
			//get_real_coord_from_screen_coord(m_r_display, m_img.width, m_screen_roi, &m_image_roi);
			//TRACE(_T("roif = %s\n"), get_rect_info_string(m_screen_roi));
			Invalidate();
		}
		//이미지 이동을 위한 단순 드래그인 경우
		else
		{
			m_offset.x += (point.x - m_ptClicked.x);
			m_offset.y += (point.y - m_ptClicked.y);

			//int nw = m_img.width * m_zoom;
			//int nh = m_img.height * m_zoom;

			//CRect rc;
			//GetClientRect(rc);
			//Clamp(m_offset.x, (long)(rc.Width() - nw), (long)0);
			//Clamp(m_offset.y, (long)(rc.Height() - nh), (long)0);

			////마우스로 이미지를 이동해서 offset이 변경됐다면 roi도 같이 이동시켜줘야 한다.
			//if (m_screen_roi.IsEmptyArea() == false)
			//{
			//	m_screen_roi.X += m_offset.x;
			//	m_screen_roi.Y += m_offset.y;
			//	//m_screen_roi.Offset(-m_offset.x, -m_offset.y);
			//}

			Invalidate();
			m_ptClicked = point;
		}
	}
	else if (m_show_pixel)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE | TME_HOVER;
		tme.dwHoverTime = 1;
		_TrackMouseEvent(&tme);

		CPoint pt;
		get_real_coord_from_screen_coord(m_r_display, m_img.width, point, &pt);
		if (pt.x < 0 || pt.x >= m_img.width || pt.y < 0 || pt.y >= m_img.height)
		{
			if (m_static_pixel.IsWindowVisible())
				m_static_pixel.ShowWindow(SW_HIDE);
			//m_static_pixel.set_text(_T("(-,-)\nA -\nR -\nG -\nB -"));
			//TRACE(_T("out of range. %ld\n"), GetTickCount());
			return;
		}
		else
		{
			if (m_static_pixel.IsWindowVisible() == false)
				m_static_pixel.ShowWindow(SW_SHOW);

			m_cr_pixel = m_img.get_pixel(pt.x, pt.y);
			TRACE(_T("screen_pt = %d, %d  real_pt = %d, %d (%d, %d, %d, %d)\n"), point.x, point.y, pt.x, pt.y, m_cr_pixel.GetA(), m_cr_pixel.GetR(), m_cr_pixel.GetG(), m_cr_pixel.GetB());

			//픽셀 정보 표시창을 커서	위치에 맞춰서 이동시킨다. rc를 벗어나지 않도록 보정까지 한다.
			CRect r = make_rect(point.x + 24, point.y, PIXEL_INFO_CX, PIXEL_INFO_CY);

			CRect rc;
			GetClientRect(rc);

			adjust_rect_range(r, rc, true);
			m_static_pixel.MoveWindow(r);
			//m_static_pixel.SetWindowPos(NULL, r.left, r.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			CString str;

			str.Format(_T("(%d,%d)\nA %3d\nR %3d\nG %3d\nB %3d"), pt.x, pt.y, m_cr_pixel.GetA(), m_cr_pixel.GetR(), m_cr_pixel.GetG(), m_cr_pixel.GetB());

			if (m_cr_pixel.GetValue() != m_cr_pixel_old.GetValue())
			{
				m_static_pixel.set_back_color(m_cr_pixel);
				m_static_pixel.set_text_color(get_distinct_gcolor(m_cr_pixel));
				m_static_pixel.set_text(str);
				//m_static_pixel.set_textf(Gdiplus::Color::RoyalBlue, str);
				m_cr_pixel_old = m_cr_pixel;
			}
		}
	}

	::SendMessage(m_parent->m_hWnd, WM_MOUSEMOVE, 0, MAKELONG(point.x, point.y));

	CDialog::OnMouseMove(nFlags, point);
}

void CSCImageDlg::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialog::OnMouseHover(nFlags, point);
}

void CSCImageDlg::OnMouseLeave()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_static_pixel.ShowWindow(SW_HIDE);

	CDialog::OnMouseLeave();
}

BOOL CSCImageDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	//roi를 그리기 위해 Ctrl키를 누른 상태라면 cross 커서로
	if (IsCtrlPressed())
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
		return TRUE;
	}

	//roi가 그려진 상태라면 roi의 move, resize 위치에 맞는 커서로 변경한다.
	if (m_screen_roi.IsEmptyArea() == false)
	{
		CPoint pt;
		CRect rc;

		GetClientRect(rc);

		GetCursorPos(&pt);
		ScreenToClient(&pt);

		m_handle_index = -1;

		for (int i = 0; i < MAX_RECT_HANDLE; i++)
		{
			if (m_roi_handle[i].PtInRect(pt))
			{
				m_handle_index = i;
				break;
			}
		}

		if (m_handle_index != -1)
		{
			//마우스 커서 모양을 변경한다.
			switch (m_handle_index)
			{
				case corner_inside :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEALL));
					break;
				case corner_left :
				case corner_right :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
					break;
				case corner_top :
				case corner_bottom :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
					break;
				case corner_topleft :
				case corner_bottomright :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENWSE));
					break;
				case corner_topright :
				case corner_bottomleft :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENESW));
					break;
			}

			return TRUE;
		}
	}

	//ctrl키가 눌려진 것도 아니고 픽셀 정보 표시 메뉴가 check 상태라면
	if (m_show_pixel && m_cursor_dropper)
	{
		SetCursor(m_cursor_dropper);
		return TRUE;
	}

	CPoint pt;
	GetCursorPos(&pt);

	if (WindowFromPoint(pt) == this)
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
		return TRUE;
	}

	return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

//dropper(spuit) cursor 지정
void CSCImageDlg::set_dropper_cursor(UINT nID)
{
	m_cursor_dropper = AfxGetApp()->LoadCursor(nID);
}

void CSCImageDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialog::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

LRESULT CSCImageDlg::on_message_from_GdiplusBitmap(WPARAM wParam, LPARAM lParam)
{
	CGdiplusBitmapMessage* msg = (CGdiplusBitmapMessage*)wParam;
	if (!msg)
		return 0;

	if (msg->message == CGdiplusBitmap::message_gif_frame_changed)
	{
		TRACE(_T("%d / %d\n"), msg->frame_index, msg->total_frames);
		m_slider_gif.set_pos(msg->frame_index);
	}

	return 0;
}

LRESULT CSCImageDlg::on_message_from_CSCSliderCtrl(WPARAM wParam, LPARAM lParam)
{
	CSCSliderCtrlMsg* msg = (CSCSliderCtrlMsg*)wParam;
	if (msg->msg == CSCSliderCtrlMsg::msg_thumb_move)
	{
		m_img.goto_frame(msg->pos, true);
	}
	return 0;
}
