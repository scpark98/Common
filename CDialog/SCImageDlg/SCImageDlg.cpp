﻿// SCImageDlg.cpp: 구현 파일
//

//#include "afxdialogex.h"
#include "SCImageDlg.h"

#include <thread>
#include "../../Functions.h"
#include "../../MemoryDC.h"
#include "../../AutoFont.h"

// CSCImageDlg 대화 상자

IMPLEMENT_DYNAMIC(CSCImageDlg, CDialog)

CSCImageDlg::CSCImageDlg(CWnd* parent)
{
}

CSCImageDlg::~CSCImageDlg()
{
	stop_gif();
}

bool CSCImageDlg::create(CWnd* parent, int x, int y, int cx, int cy)
{
	m_parent = parent;

	LONG_PTR dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;

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
	m_static_pixel.set_font_size(10);

	res = m_slider_gif.Create(WS_CHILD, make_rect(x + 8, y + cy - 8 - GIF_SLIDER_HEIGHT, GIF_SLIDER_WIDTH, GIF_SLIDER_HEIGHT), this, 0);
	m_slider_gif.set_style(CSCSliderCtrl::style_progress);
	m_slider_gif.set_track_height(GIF_SLIDER_HEIGHT);
	m_slider_gif.set_font_name(_T("Arial"));
	m_slider_gif.set_font_size(8);
	//m_slider_gif.set_back_color(gGRAY(32));
	m_slider_gif.draw_progress_border();
	m_slider_gif.set_progress_border_color(Gdiplus::Color::DimGray);
	m_slider_gif.set_use_slide();

	m_br_zigzag = CGdiplusBitmap::get_zigzag_pattern(32, m_cr_zigzag_back, m_cr_zigzag_fore);

	m_thumb.create(this);
	m_thumb.set_color_theme(CSCColorTheme::color_theme_dark_gray);
	m_thumb.ShowWindow(m_show_thumb ? SW_SHOW : SW_HIDE);

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
	ON_REGISTERED_MESSAGE(Message_CSCThumbCtrl, &CSCImageDlg::on_message_CSCThumbCtrl)
END_MESSAGE_MAP()


// CSCImageDlg 메시지 처리기

void CSCImageDlg::OnPaint()
{
	//if (m_show_thumb)
	//	return;

	CPaintDC dc1(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CStatic::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	CString str;

	GetClientRect(rc);


	//수정 필요. gif도 stretch mode에 따라 꽉차게 또는 원래 크기로 표시한다. 아직 미구현이며 현재는 zoom 표시됨.	
	//if (m_img.is_valid() && m_img.is_animated_gif())
	//{
	//	CRect r = get_ratio_rect(rc, m_img.width, m_img.height, 0, false);
	//	CRgn rgn_rc;
	//	CRgn rgn_img;
	//	CRgn rgn_exclude;
	//	rgn_rc.CreateRectRgnIndirect(&rc);
	//	rgn_img.CreateRectRgnIndirect(&r);
	//	rgn_exclude.CreateRectRgn(0, 0, 1, 1);
	//	rgn_exclude.CombineRgn(&rgn_rc, &rgn_img, RGN_XOR);
	//	dc1.SelectClipRgn(&rgn_exclude);
	//	dc1.FillSolidRect(rc, GRAY32);
	//	return;
	//}

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

		//CRect rText = rc;
		CString msg = _T("Fail to open this image file.");
		//dc.DrawText(msg, rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_CALCRECT);

		DrawShadowText(dc.GetSafeHdc(), msg, -1, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE, maroon, black, 2, 1);

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
	if (m_img.has_alpha_pixel() == 1)
	{
		g.FillRectangle(m_br_zigzag.get(), CRect2GpRect(m_r_display));
	}

	//TRACE(_T("m_r_display = %s\n"), get_rect_info_string(m_r_display));

	//실제 이미지를 그려준다.
	//m_img.draw(g, m_r_display);

	//SetPixelOffsetMode()를 Half로 세팅하지 않으면
	//0,0에 그리라고 해도 (-0.5, -0.5) ~ (0.5, 0.5), 즉 중점이 0,0인 위치에 그리게 되므로
	//이미지가 약간 왼쪽 상단으로 밀려서 그려지게 된다.
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	g.DrawImage(m_img, CRect2GpRect(m_r_display));


	//이미지 정보 표시
	if (m_show_info)// && m_parent->IsZoomed())
	{
		CAutoFont af(_T("맑은 고딕"));
		af.SetHeight(24);

		CFont* pOldFont = dc.SelectObject(&af);
		CString filename = get_part(m_filename, fn_name);
		CString info;
		CString ratio_str;
		float ratio = m_img.get_ratio();

		if (m_img.width == m_img.height)
			ratio_str = _T("1 : 1");
		else
			ratio_str.Format(_T("%.3f : 1"), ratio);

		info.Format(_T("파일 이름 : %s\n파일 크기: %s\n수정 날짜: %s\n이미지 정보: %dx%dx%d (%s)\n이미지 비율: %s\n확대 배율: %.0f%%"),
					filename + m_alt_info,
					get_file_size_str(m_filename),
					get_datetime_str(GetFileLastModifiedTime(m_filename)),
					m_img.width, m_img.height, m_img.channel * 8,
					m_img.get_pixel_format_str(),
					ratio_str,
					m_zoom * 100.0);

		//dc.GetTextExtend(info)는 멀티라인 정보를 얻을 수 없으므로 DT_CALCRECT를 이용해야 한다.
		CRect rText;
		dc.DrawText(info, rText, DT_LEFT | DT_TOP | DT_CALCRECT);
		rText.OffsetRect(8, 8);

		DrawShadowText(dc.GetSafeHdc(), info, info.GetLength(),	rText,
						DT_NOCLIP | DT_LEFT | DT_TOP, beige, black, 2, 1);

		dc.SelectObject(pOldFont);
	}



	//roi를 그리거나 위치, 크기를 조정할 때는 오로지 m_screen_roi만 신경쓴다.
	CRect screen_roi = GpRectF2CRect(m_screen_roi);
	screen_roi.InflateRect(0, 0, 2, 2);	//이렇게 2씩 늘려줘야 roi의 right, bottom이 정확히 픽셀과 일치되게 표시된다.
	screen_roi.NormalizeRect();

	if (m_lbutton_down)// && m_drawing_roi)
	{
		//draw_rectangle(&dc, screen_roi, red, NULL_BRUSH, 1, PS_DASH, R2_XORPEN);
		draw_rectangle(g, screen_roi, Gdiplus::Color::Red, Gdiplus::Color(64, 0, 64, 255), 1);
	}
	else if (m_image_roi.Width >= 5.0f && m_image_roi.Height >= 5.0f)
	{
		//이미지 확대 축소 등에 의해 m_r_display가 변경되면 그에 따라 m_screen_roi도 다시 계산해줘야 한다.
		get_screen_coord_from_real_coord(m_r_display, m_img.width, m_image_roi, &m_screen_roi);
		//m_screen_roi.Offset(m_offset.x, m_offset.y);
		screen_roi = GpRectF2CRect(m_screen_roi);
		screen_roi.InflateRect(0, 0, 2, 2);	//이렇게 2씩 늘려줘야 roi의 right, bottom이 정확히 픽셀과 일치되게 표시된다.
		//이미 offset 변경에 의한 보정은 get_screen_coord_from_real_coord()에서 해준다.
		//screen_roi.OffsetRect(m_offset);
		//draw_rectangle(&dc, screen_roi, red, NULL_BRUSH, 1, PS_DASH, R2_XORPEN);
		draw_rectangle(g, screen_roi, Gdiplus::Color::Red, Gdiplus::Color(64, 0, 64, 255), 1);
	}

	if (!m_screen_roi.IsEmptyArea())
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
				m_thumb.stop_loading();
				return FALSE;
			case VK_SPACE :
				if (m_img.is_animated_gif())
				{
					pause_gif(-1);
					return TRUE;
				}
				break;
			case 'B':
				set_view_mode(view_mode_toggle);
				return TRUE;
				/*
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
			*/
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
		}

		return FALSE;
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

bool CSCImageDlg::load(CString sFile, bool load_thumbs)
{
	//다른 이미지를 로딩하기 전에 이전 이미지가 animated gif였다면 재생하는 thread를 중지시켜야 한다.
	stop_gif();

	m_filename = sFile;

	//m_image_roi = Gdiplus::RectF();
	//m_screen_roi = Gdiplus::RectF();

	AfxGetApp()->WriteProfileString(_T("setting\\SCImageDlg"), _T("recent file"), sFile);
	bool res = m_img.load(sFile);

	if (m_img.is_animated_gif())
	{
		CRect rc;
		GetClientRect(rc);
		m_slider_gif.set_range(0, m_img.get_frame_count() - 1);
		m_slider_gif.set_pos(0);
		m_slider_gif.MoveWindow(rc.left + 8, rc.bottom - 8 - GIF_SLIDER_HEIGHT, GIF_SLIDER_WIDTH, GIF_SLIDER_HEIGHT);
		m_slider_gif.ShowWindow(SW_SHOW);
		
		//원래 CGdiplusBitmap은 animated gif를 로딩하면 set_animation() 함수를 호출하여 자체 재생되는 기능을 포함한다.
		//하지만 CSCImageDlg에서는 roi 설정, 다른 child ctrl들과의 충돌등이 있으므로 이 클래스에서 직접 재생한다.
		m_img.set_gif_play_itself(false);

		std::thread t(&CSCImageDlg::thread_gif_animation, this);
		t.detach();
	}
	else
	{
		m_slider_gif.ShowWindow(SW_HIDE);
		Invalidate();
	}

	if (m_thumb.size() == 0 || load_thumbs)
		m_thumb.set_path(get_part(sFile, fn_folder));

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImageDlg, (WPARAM)&CSCImageDlgMessage(this, message_image_loaded), (LPARAM)&sFile);

	return res;
}

bool CSCImageDlg::load(CString sType, UINT id)
{
	m_filename.Format(_T("Resource Image(id:%d)"), id);

	m_image_roi = Gdiplus::RectF();
	m_screen_roi = Gdiplus::RectF();

	bool res = m_img.load(sType, id);
	Invalidate();

	//::SendMessage(GetParent()->GetSafeHwnd(), Message_CImageStatic, (WPARAM)&CImageStaticMessage(this, message_loading_completed), 0);

	return res;
}

//png일 경우는 sType을 생략할 수 있다.
bool CSCImageDlg::load(UINT id)
{
	return load(_T("PNG"), id);
}

void CSCImageDlg::set_filename(CString filename)
{
	m_filename = filename;

	if (m_filename.IsEmpty())
	{
		release();
	}
}

void CSCImageDlg::release()
{
	m_img.release();
	m_thumb.release_thumb(-1);
	Invalidate();
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

bool CSCImageDlg::paste_from_clipboard()
{
	stop_gif();

	if (m_img.paste_from_clipboard())
	{
		m_slider_gif.ShowWindow(SW_HIDE);
		set_filename(_T("image from clipboard"));
		set_alt_info(_T(""));
		set_view_mode(view_mode_image);
		Invalidate();
		return true;
	}

	return false;
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
		m_zoom += 0.2;
	else if (mode < 0)
		m_zoom -= 0.2;
	else
		m_zoom = 1.0;

	m_fit2ctrl = false;

	Clamp(m_zoom, 0.2, 40.0);
	//get_screen_coord_from_real_coord(m_r_display, m_img.width, m_image_roi, &m_screen_roi);
	//Wait(1);
	Invalidate();
}

void CSCImageDlg::zoom(double ratio)
{
	m_fit2ctrl = false;
	m_zoom = ratio;
	Clamp(m_zoom, 0.1, 40.0);
	Invalidate();
}

void CSCImageDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_static_pixel.m_hWnd == NULL)
		return;

	CRect rc;
	GetClientRect(rc);

	m_thumb.MoveWindow(rc);

	if (m_img.is_animated_gif())
	{
		m_img.move_gif(rc);
		m_slider_gif.MoveWindow(rc.left + 8, rc.bottom - 8 - GIF_SLIDER_HEIGHT, GIF_SLIDER_WIDTH, GIF_SLIDER_HEIGHT);
	}
	//else
	{
		Invalidate();
	}

	if (m_show_pixel)
	{
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
		m_drawing_roi = true;
		m_screen_roi = Gdiplus::RectF(point.x, point.y, 0, 0);
		
		//실제 이미지에서 1픽셀 단위로 선택되어야 하므로 float이 아닌 int로 변경한 후
		//해당 픽셀의 크기를 다시 screen_roi로 넣어줘야 픽셀 단위로 선택된다.
		Gdiplus::RectF img_roi;
		get_real_coord_from_screen_coord(m_r_display, m_img.width, m_screen_roi, &img_roi);
		img_roi.X = (int)img_roi.X;
		img_roi.Y = (int)img_roi.Y;
		get_screen_coord_from_real_coord(m_r_display, m_img.width, img_roi, &m_screen_roi);

		return;
	}
	//roi의 크기를 변경중
	else if (m_handle_index >= 0)
	{
		m_drawing_roi = false;
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CSCImageDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_lbutton_down)
	{
		ReleaseCapture();
		m_lbutton_down = false;

		//Clamp(point.x, m_r_display.left, m_r_display.right);
		//Clamp(point.y, m_r_display.top, m_r_display.bottom);

		if (m_drawing_roi)
		{
			m_drawing_roi = false;

			m_screen_roi.Width = point.x - m_screen_roi.X;
			m_screen_roi.Height = point.y - m_screen_roi.Y;

			//실제 이미지에서 1픽셀 단위로 선택되어야 하므로 float이 아닌 int로 변경한 후
			//해당 픽셀의 크기를 다시 screen_roi로 넣어줘야 픽셀 단위로 선택된다.
			Gdiplus::RectF img_roi;
			get_real_coord_from_screen_coord(m_r_display, m_img.width, m_screen_roi, &img_roi);
			img_roi.X = ROUND(img_roi.X, 0);
			img_roi.Y = ROUND(img_roi.Y, 0);
			img_roi.Width = ROUND(img_roi.Width, 0);
			img_roi.Height = ROUND(img_roi.Height, 0);
			get_screen_coord_from_real_coord(m_r_display, m_img.width, img_roi, &m_screen_roi);

			//5x5 미만일 경우는 roi를 그리지 않는다.
			if (fabs(img_roi.Width) < 5.0 || fabs(img_roi.Height) < 5.0)
			{
				m_screen_roi = Gdiplus::RectF();
				m_image_roi = Gdiplus::RectF();
				Invalidate();
				return;
			}
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
		if (m_drawing_roi)
		{
			m_static_pixel.ShowWindow(SW_HIDE);

			m_screen_roi.Width = point.x - m_screen_roi.X;
			m_screen_roi.Height = point.y - m_screen_roi.Y;

			//실제 이미지에서 1픽셀 단위로 선택되어야 하므로 float이 아닌 int로 변경한 후
			//해당 픽셀의 크기를 다시 screen_roi로 넣어줘야 픽셀 단위로 선택된다.
			Gdiplus::RectF img_roi;
			get_real_coord_from_screen_coord(m_r_display, m_img.width, m_screen_roi, &img_roi);
			img_roi.Width = ROUND(img_roi.Width, 0);
			img_roi.Height = ROUND(img_roi.Height, 0);
			get_screen_coord_from_real_coord(m_r_display, m_img.width, img_roi, &m_screen_roi);

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

			Gdiplus::RectF img_roi;
			get_real_coord_from_screen_coord(m_r_display, m_img.width, m_screen_roi, &img_roi);
			img_roi.X = ROUND(img_roi.X, 0);
			img_roi.Y = ROUND(img_roi.Y, 0);
			img_roi.Width = ROUND(img_roi.Width, 0);
			img_roi.Height = ROUND(img_roi.Height, 0);
			get_screen_coord_from_real_coord(m_r_display, m_img.width, img_roi, &m_screen_roi);

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

			Gdiplus::RectF img_roi;
			get_real_coord_from_screen_coord(m_r_display, m_img.width, m_screen_roi, &img_roi);
			img_roi.X = ROUND(img_roi.X, 0);
			img_roi.Y = ROUND(img_roi.Y, 0);
			img_roi.Width = ROUND(img_roi.Width, 0);
			img_roi.Height = ROUND(img_roi.Height, 0);
			get_screen_coord_from_real_coord(m_r_display, m_img.width, img_roi, &m_screen_roi);

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
			//TRACE(_T("screen_pt = %d, %d  real_pt = %d, %d (%d, %d, %d, %d)\n"), point.x, point.y, pt.x, pt.y, m_cr_pixel.GetA(), m_cr_pixel.GetR(), m_cr_pixel.GetG(), m_cr_pixel.GetB());

			//픽셀 정보 표시창을 커서	위치에 맞춰서 이동시킨다. rc를 벗어나지 않도록 보정까지 한다.
			CRect r = make_rect(point.x + 24, point.y, PIXEL_INFO_CX, PIXEL_INFO_CY);

			CRect rc;
			GetClientRect(rc);

			adjust_rect_range(r, rc, true);
			m_static_pixel.MoveWindow(r);
			//m_static_pixel.SetWindowPos(NULL, r.left, r.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			CString str;

			str.Format(_T("(%d,%d)\nA %3d\nR %3d\nG %3d\nB %3d"), pt.x, pt.y, m_cr_pixel.GetA(), m_cr_pixel.GetR(), m_cr_pixel.GetG(), m_cr_pixel.GetB());

			//if (m_cr_pixel.GetValue() != m_cr_pixel_old.GetValue())
			{
				m_static_pixel.set_back_color(m_cr_pixel.GetA() == 0 ? Gdiplus::Color::White : m_cr_pixel);
				m_static_pixel.set_text_color(m_cr_pixel.GetA() == 0 ? Gdiplus::Color::DimGray : get_distinct_color(m_cr_pixel));
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
	if (m_show_thumb)
		return CDialog::OnSetCursor(pWnd, nHitTest, message);

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

		for (int i = 0; i < MAX_ROI_RECT_HANDLE; i++)
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

	CPoint pt;
	GetCursorPos(&pt);

	CPoint client_pt = pt;
	ScreenToClient(&client_pt);

	//CSCImageDlg이고 이미지 표시 영역에 한해
	if (WindowFromPoint(pt) == this && m_r_display.PtInRect(client_pt))
	{
		//픽셀 정보 표시라면 스포이트를
		if (m_show_pixel && m_cursor_dropper)
			SetCursor(m_cursor_dropper);
		//그렇지 않다면 십자 커서를 표시한다.
		else
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
		return TRUE;
	}

	//기타 다른 컨트롤 또는 이미지 영역밖은 기본 커서로 표시한다.
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
		//TRACE(_T("%d / %d\n"), msg->frame_index, msg->total_frames);
		m_slider_gif.set_pos(msg->frame_index);
	}

	return 0;
}

LRESULT CSCImageDlg::on_message_from_CSCSliderCtrl(WPARAM wParam, LPARAM lParam)
{
	CSCSliderCtrlMsg* msg = (CSCSliderCtrlMsg*)wParam;
	if (msg->msg == CSCSliderCtrlMsg::msg_thumb_move)
	{
		//TRACE(_T("%ld, pos = %d\n"), GetTickCount(), msg->pos);
		goto_frame(msg->pos, true);
	}
	return 0;
}

//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
void CSCImageDlg::pause_gif(int pos)
{
	if (m_img.is_gif_play_itself())
		return;

	if (m_img.get_frame_count() < 2 || !m_run_thread_animation)
		return;

	//m_frame_count가 UINT이고 pos = -1일 경우 아래 if문은 true가 된다.
	//if (-1 >= (UINT)20) => true
	//취약하므로 m_frame_count의 type을 UINT에서 int로 변경한다.
	if (pos >= m_img.get_frame_count())
		pos = 0;

	if (pos < 0)
	{
		m_paused = !m_paused;

		//gif가 일시정지 된 상태라면 그 때의 m_pBitmap에 대한 픽셀정보를 다시 추출해줘야 한다.
		if (m_paused && m_show_pixel)
			m_img.get_raw_data();
	}
	else
	{
		goto_frame(pos, true);
	}
}

void CSCImageDlg::stop_gif()
{
	if (m_img.get_frame_count() < 2)
		return;

	m_paused = false;
	m_run_thread_animation = false;
	while (!m_thread_animation_terminated)
		Wait(100);

	if (m_hWnd)
		Invalidate();
}

void CSCImageDlg::goto_frame(int pos, bool pause)
{
	if (m_img.get_frame_count() < 2)
		return;

	if (pos >= m_img.get_frame_count())
		m_frame_index = 0;

	m_frame_index = pos;
	m_paused = pause;

	GUID   pageGuid = Gdiplus::FrameDimensionTime;	//for TIFF : Gdiplus::FrameDimensionPage
	long t0 = clock();
	m_img.m_pBitmap->SelectActiveFrame(&pageGuid, m_frame_index);
	TRACE(_T("elapse = %ld\n"), clock() - t0);

	//paused 상태라면 Invalidate()을 해줘야 하고
	//픽셀 정보를 표시중이라면 현재 프레임에 대한 data도 다시 추출해줘야 한다.
	if (m_paused)
	{
		if (m_show_pixel)
			m_img.get_raw_data();

		Invalidate();
	}
}

//지정 % 위치의 프레임으로 이동
void CSCImageDlg::goto_frame_percent(int pos, bool pause)
{
	double dpos = ((double)pos / (double)m_img.get_frame_count()) * 100.0;
	goto_frame((int)dpos, pause);
}

void CSCImageDlg::thread_gif_animation()
{
	if (m_img.get_frame_count() < 2)
		return;

	m_frame_index = 0;
	m_run_thread_animation = true;
	m_thread_animation_terminated = false;

	long t0 = clock();
	long t1 = t0;

	while (m_run_thread_animation)
	{
		GUID   pageGuid = Gdiplus::FrameDimensionTime;

		if (m_paused)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		if (!m_run_thread_animation)
			break;

		m_slider_gif.set_pos(m_frame_index);

		m_frame_index++;
		if (m_frame_index >= m_img.get_frame_count())
			m_frame_index = 0;

		m_img.m_pBitmap->SelectActiveFrame(&pageGuid, m_frame_index);

		if (!m_run_thread_animation)
			break;

		Invalidate();

		//delay는 해당 프레임에 설정된 delay를 그대로 주면 안되고
		//전 프레임부터 현 프레임까지 재생하는데 걸린 시간은 빼줘야 한다.
		t1 = clock();
		long display_delay = t1 - t0;
		long delay = ((long*)(m_img.m_pPropertyItem->value))[m_frame_index] * 10;

		if (delay > display_delay)
			std::this_thread::sleep_for(std::chrono::milliseconds((delay - display_delay)));
		t0 = clock();
	}

	m_run_thread_animation = false;
	m_thread_animation_terminated = true;
}

void CSCImageDlg::set_zigzag_color(Gdiplus::Color cr_back, Gdiplus::Color cr_fore)
{
	m_br_zigzag.release();

	m_br_zigzag = CGdiplusBitmap::get_zigzag_pattern(32, cr_back, cr_fore);
	Invalidate();
}

LRESULT CSCImageDlg::on_message_CSCThumbCtrl(WPARAM wParam, LPARAM lParam)
{
	CString str;
	CSCThumbCtrlMessage* msg = (CSCThumbCtrlMessage*)wParam;

	if (msg->msg == CSCThumbCtrl::message_thumb_lbutton_dbclicked)
	{
		//AfxMessageBox(i2S(msg->index) + _T(" dbclicked"));
		//ShellExecute(m_hWnd, _T("open"), m_thumb.m_thumb[msg->index].full_path, 0, 0, SW_SHOWNORMAL);


		m_show_thumb = false;
		m_thumb.ShowWindow(m_show_thumb ? SW_SHOW : SW_HIDE);

		//thumb를 ldbclick하면 해당 이미지를 바로 여는게 아니라 CASeeDlg에서 해당 이미지의 인덱스를 display시켜야 한다.
		//load(m_thumb.m_thumb[msg->index].full_path, false);
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImageDlg, (WPARAM)&CSCImageDlgMessage(this, message_load_image, msg->index), 0);
	}
	else if (msg->msg == CSCThumbCtrl::message_thumb_loading_completed)
	{
		//TRACE(_T("loading completed. %ldms\n"), m_thumb.get_loading_elapsed());
	}
	else if (msg->msg == CSCThumbCtrl::message_thumb_rename)
	{
		CString folder = get_part(m_thumb.m_thumb[msg->index].full_path, fn_folder);
		CString ext = get_part(m_thumb.m_thumb[msg->index].full_path, fn_ext);
		CString oldName;
		CString newName;
		bool filename_error = false;

		if (m_thumb.m_thumb[msg->index].title.FindOneOf(FILENAME_NOT_ALLOWED_CHAR) >= 0)
		{
			str.Format(_T("파일 이름에는 다음 문자를 사용할 수 없습니다.\n\n\\ : * ? \" < > |"));
			filename_error = true;
		}

		oldName.Format(_T("%s\\%s"), folder, m_thumb.get_old_title());
		newName.Format(_T("%s\\%s"), folder, m_thumb.m_thumb[msg->index].title);

		//확장자를 표시하고 있지 않았다면 title에 확장자를 붙여서 rename해야 한다.
		if (!m_thumb.get_show_file_extension())
		{
			oldName = oldName + _T(".") + ext;
			newName = newName + _T(".") + ext;
		}

		if (PathFileExists(newName))
		{
			str.Format(_T("%s\n\n동일한 이름의 파일이 이미 존재합니다."), newName);
			filename_error = true;
		}

		if (filename_error)
		{
			AfxMessageBox(str, MB_ICONEXCLAMATION);
			m_thumb.set_title(msg->index, m_thumb.get_old_title());
			return 0;
		}

		//str.Format(_T("%s\n=>\n%s 로 변경합니다."), oldName, newName);
		//int res = AfxMessageBox(str, MB_ICONQUESTION | MB_OKCANCEL);
		//if (res == IDOK)
		{
			if (MoveFile(oldName, newName))
			{
				m_thumb.select_item(-1, false, false);
				str = m_thumb.m_thumb[msg->index].title;
				m_thumb.sort_by_title();
				Wait(50);
				int index = m_thumb.find_by_title(str);
				m_thumb.select_item(index);
				return 0;
			}
			else
			{
				str.Format(_T("%s\n\n위 파일 이름으로 변경하지 못했습니다."), newName);
				AfxMessageBox(str, MB_ICONEXCLAMATION);
			}
		}

		m_thumb.set_title(msg->index, m_thumb.get_old_title());
	}
	else if (msg->msg == CSCThumbCtrl::message_thumb_reload)
	{
		CString recent_folder = AfxGetApp()->GetProfileString(_T("setting"), _T("recent folder"), _T(""));
		m_thumb.set_path(recent_folder);
	}

	return 0;
}

void CSCImageDlg::set_view_mode(int view_mode)
{
	if (view_mode == view_mode_toggle)
		m_show_thumb = !m_show_thumb;
	else if (view_mode == view_mode_image)
		m_show_thumb = false;
	else if (view_mode == view_mode_thumb)
		m_show_thumb = true;
	else
		return;

	m_thumb.select_item(m_filename);
	m_static_pixel.ShowWindow(m_show_thumb || !m_show_pixel ? SW_HIDE : SW_SHOW);
	m_slider_gif.ShowWindow(m_show_thumb || !m_img.is_animated_gif() ? SW_HIDE : SW_SHOW);
	m_thumb.ShowWindow(m_show_thumb ? SW_SHOW : SW_HIDE);
}
